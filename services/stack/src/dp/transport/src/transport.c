/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "transport.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "securec.h"

#include "byte_codec.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "cm_trans_channel_mgr.h"
#include "cp_errno_base.h"
#include "cp_worker.h"
#include "dli_layer.h"
#include "dpfwk_log.h"
#include "dtap.h"
#include "dtap_errno.h"
#include "dtap_tcid.h"
#include "sdf_addr.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "nlstk_public_define.h"
#include "transport_cltp.h"
#include "transport_errno.h"
#include "transport_internal.h"
#include "transport_proto.h"

#define TRANS_SEND_DATA_TIMEOUT_MS 500

typedef struct TRANS_Data {
    TRANS_Addr_S addr;
    SDF_Buff_S *buff;
    TRANS_Result_t result;
    atomic_int refCount;
} TRANS_Data_S;


static TRANS_Cbks_S g_transportCbks = {NULL};
static volatile bool g_transIsInited = false;

static void TRANS_Data_Retain(TRANS_Data_S *data)
{
    if (data == NULL) {
        return;
    }
    atomic_fetch_add_explicit(&data->refCount, 1, memory_order_acq_rel);
}

static void TRANS_Data_Release(TRANS_Data_S *data)
{
    if (data == NULL) {
        return;
    }
    atomic_fetch_sub_explicit(&data->refCount, 1, memory_order_acq_rel);
}

static void TRANS_TransportDataFree(TRANS_Data_S *data)
{
    if (data == NULL) {
        return;
    }
    if (atomic_fetch_sub_explicit(&data->refCount, 1, memory_order_acq_rel) == 1) {
        // 如果TRANS_Data_S递交给协议栈主线程，则data->buff由协议栈进行释放，该处不释放
        SDF_MemFree(data);
    }
}

static void TRANS_ChannelStatusChangeCbk(uint16_t lcid, uint16_t tcid, uint8_t result)
{
    TP_LOGI("TRANS_ChannelStatusChangeCbk, lcid: %hu, tcid: %hu, result:%hhu", lcid, tcid, result);
    SleTransLcid_S *node = CM_FindTransChannelByLocalTcid(lcid, tcid);
    if (node == NULL) {
        TP_LOGE("Failed to FindTransChannelByLocalTcid, lcid: %hu, tcid: %hu", lcid, tcid);
        return;
    }
    uint16_t src_port = node->params->srcPort;
    SLE_Addr_S *addr = &node->addr;
    if (g_transportCbks.sendDataCbk != NULL) {
        g_transportCbks.sendDataCbk(addr, tcid, src_port, result);
    }
}

static void TRANS_RegisterDtapCbks(void)
{
    DTAP_Data_Send_Cbks_S cbks = {0};

    cbks.transChannelStatusChangeCbk = TRANS_ChannelStatusChangeCbk;
    DTAP_RegisterDataSendCbks(&cbks);
}

uint32_t TRANS_RegisterCbks(const TRANS_Cbks_S *cbks)
{
    if (cbks == NULL || cbks->recvDataCbk == NULL || cbks->sendDataCbk == NULL) {
        TP_LOGE("cbks is null");
        return TRANS_REGISTER_CBKS_NULL_PTR;
    }

    g_transportCbks = *cbks;
    TRANS_RegisterDtapCbks();
    return TRANS_SUCCESS;
}

void TRANS_UnregisterCbks(void)
{
    TRANS_Cbks_S cbks = { NULL };

    g_transportCbks = cbks;
    DTAP_UnRegisterDataSendCbks();
}

static int32_t TRANS_FillAddr(DTAP_Data_Info_S *info, SDF_Buff_S *buff, TRANS_Addr_S *addr)
{
    TRANS_ProtoBasicHeader_S *basicHeader = (TRANS_ProtoBasicHeader_S *)SDF_DataOffset(buff);
    CM_LogicLink_S logicLink = { 0 };

    if (CM_GetLogicLinkByLcid(info->lcid, &logicLink) != CM_SUCCESS) {
        TP_LOGE("get logic link failed, lcid: %hu", info->lcid);
        return TRANS_FAIL;
    }

    (void)memcpy_s(&addr->devAddr, sizeof(SLE_Addr_S), &logicLink.addr, sizeof(SLE_Addr_S));
    addr->tcid = info->tcid;
    addr->proto = (info->pi == DTAP_PI_LWCLTP) ? TRANS_PROTO_CONNECTIONLESS : TRANS_PROTO_CONNECTION;
    addr->srcPort = DECODE2BYTE_BIG((uint8_t *)&basicHeader->srcPort);
    addr->dstPort = DECODE2BYTE_BIG((uint8_t *)&basicHeader->dstPort);
    return TRANS_SUCCESS;
}

static bool TRANS_CheckBasicHeader(SDF_Buff_S *buff)
{
    TRANS_ProtoBasicHeader_S *basicHeader = NULL;
    uint64_t dataLen = SDF_DataLenGet(buff);
    if (dataLen < sizeof(TRANS_ProtoBasicHeader_S)) {
        TP_LOGE("invalid dataLen = %llu", dataLen);
        return false;
    }

    basicHeader = (TRANS_ProtoBasicHeader_S *)SDF_DataOffset(buff);
    if (basicHeader->version != TRANS_PROTO_VERSION) {
        TP_LOGE("invalid transport protocol version = %hhu", basicHeader->version);
        return false;
    }
    return true;
}

static int32_t TRANS_PktProc(uint8_t pi, SDF_Buff_S *buff)
{
    int ret = TRANS_FAIL;

    switch (pi) {
        case DTAP_PI_LWCLTP:
            ret = TRANS_CltpPktProc(buff);
            break;
        case DTAP_PI_LWCTP:
            TP_LOGE("connection-mode transport protocol does not support yet");
            break;
        default:
            TP_LOGE("unsupported pi = %hhu", pi);
            break;
    }

    return ret;
}

static int TRANS_DataRecvCbk(DTAP_Data_Info_S *info, SDF_Buff_S *buff)
{
    uint8_t *data = NULL;
    uint64_t dataLen = 0;
    TRANS_Addr_S addr = { 0 };

    if (info == NULL || buff == NULL || SDF_DataLenGet(buff) == 0) {
        TP_LOGE("invalid transport protocol data");
        return TRANS_RESULT_INTERNAL_FAULT;
    }

    if (info->pi != DTAP_PI_LWCLTP && info->pi != DTAP_PI_LWCTP) {
        TP_LOGE("invalid transport protocol: %hhu", info->pi);
        return TRANS_RESULT_INTERNAL_FAULT;
    }

    if (!TRANS_CheckBasicHeader(buff)) {
        return TRANS_RESULT_INTERNAL_FAULT;
    }

    if (TRANS_FillAddr(info, buff, &addr) != TRANS_SUCCESS) {
        return TRANS_RESULT_INTERNAL_FAULT;
    }

    if (TRANS_PktProc(info->pi, buff) != TRANS_SUCCESS) {
        return TRANS_RESULT_INTERNAL_FAULT;
    }

    data = SDF_DataOffset(buff);
    dataLen = SDF_DataLenGet(buff);
    TP_LOGD("recv transport data, dataLen = %llu, device addr: %s, proto: %hhu, "
            "tcid: %hhu, src port: %hu, dst port: %hu",
            dataLen, GET_ENC_ADDR(&addr.devAddr), addr.proto, addr.tcid, addr.srcPort, addr.dstPort);
    if (g_transportCbks.recvDataCbk != NULL) {
        int result = g_transportCbks.recvDataCbk(&addr, data, dataLen);
        return result;
    }
    return TRANS_RESULT_SUCCESS;
}

static void TRANS_ChannelSetStatusProc(void *arg)
{
    TRANS_Addr_S *trans_addr = (TRANS_Addr_S *)arg;
    CM_LogicLink_S logicLink = { 0 };
    if (CM_GetLogicLinkByAddr(&trans_addr->devAddr, &logicLink) != CM_SUCCESS) {
        TP_LOGE("get logic link failed");
        return;
    }
    uint16_t lcid = logicLink.lcid;
    uint16_t tcid = trans_addr->tcid;
    uint16_t result = trans_addr->srcPort;
    DTAP_ChannelSetStatus(lcid, tcid, result);
}

void TRANS_ChannelSetStatus(SLE_Addr_S *addr, uint8_t tcid, uint8_t result)
{
    if (addr == NULL) {
        TP_LOGE("addr is null");
        return;
    }

    TRANS_Addr_S *trans_addr = NULL;
    trans_addr = SDF_MemAlloc(sizeof(TRANS_Addr_S));
    if (trans_addr == NULL) {
        TP_LOGE("malloc failed");
        return;
    }
    if (memcpy_s(&trans_addr->devAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S)) != EOK) {
        TP_LOGE("memcpy failed");
        SDF_MemFree(trans_addr);
        return;
    }
    trans_addr->tcid = tcid;
    trans_addr->srcPort = result;
    uint32_t ret = CP_PostTask(TRANS_ChannelSetStatusProc, trans_addr, SDF_MemFree);
    if (ret != CP_OK) {
        TP_LOGE("post task failed, ret: %08x", ret);
        return;
    }
    return;
}

static uint8_t TRANS_CovertDtapErrorCode(uint32_t result)
{
    switch (result) {
        case DTAP_SUCCESS:
            return TRANS_RESULT_SUCCESS;
        case DTAP_TRANS_INVALID_DATA:
            return TRANS_RESULT_INVALID_DATA;
        case DTAP_TRANS_FIND_CHANNEL_ERR:
            return TRANS_RESULT_CHANNEL_NOT_FOUND;
        case DTAP_TRANS_EXCEED_MTU_ERR:
            return TRANS_RESULT_EXCEED_MTU_ERR;
        case DTAP_TRANS_RELIABLE_TX_WINDOW_FULL:
            return TRANS_RESULT_TX_CACHE_FULL;
        default:
            return TRANS_RESULT_INTERNAL_FAULT;
    }
}

static uint32_t TRANS_SendDataToDtap(uint8_t pi, uint16_t lcid, uint8_t tcid, SDF_Buff_S *buff)
{
    DTAP_Data_S data = { 0 };
    data.pi = pi;
    data.lcid = lcid;
    data.tcid = tcid;
    data.buff = buff;
    return DTAP_DataSend(&data);
}

static void TRANS_SendDataProc(void *arg)
{
    uint8_t pi;
    uint8_t result = TRANS_RESULT_SUCCESS;
    CM_LogicLink_S logicLink = { 0 };
    TRANS_Data_S *transportData = (TRANS_Data_S *)arg;

    TP_LOGD("send transport data, dataLen = %llu, device addr: %s, proto: %hhu, "
            "tcid: %hhu, src port: %hu, dst port: %hu",
            SDF_DataLenGet(transportData->buff), GET_ENC_ADDR(&transportData->addr.devAddr),
            transportData->addr.proto, transportData->addr.tcid, transportData->addr.srcPort,
            transportData->addr.dstPort);
    uint32_t ret = CM_GetLogicLinkByAddr(&transportData->addr.devAddr, &logicLink);
    if (ret != CM_SUCCESS) {
        result = TRANS_RESULT_LOGIC_LINK_NOT_FOUND;
        SDF_BuffFree(transportData->buff);
        goto END;
    }

    switch (transportData->addr.proto) {
        case TRANS_PROTO_CONNECTIONLESS:
            pi = DTAP_PI_LWCLTP;
            ret = TRANS_CltpHeaderBuild(transportData->addr.srcPort, transportData->addr.dstPort,
                                        transportData->buff);
            break;
        case TRANS_PROTO_CONNECTION:
        default:
            TP_LOGE("unsupported proto = %hhu", transportData->addr.proto);
            result = TRANS_RESULT_INTERNAL_FAULT;
            SDF_BuffFree(transportData->buff);
            goto END;
    }

    if (ret != TRANS_SUCCESS) {
        result = TRANS_RESULT_INTERNAL_FAULT;
        SDF_BuffFree(transportData->buff);
        goto END;
    }

    ret = TRANS_SendDataToDtap(pi, logicLink.lcid, transportData->addr.tcid, transportData->buff);
    if (ret != DTAP_SUCCESS) {
        SDF_BuffFree(transportData->buff);
        TP_LOGE("send data to dtap failed, ret: 0x%08x", ret);
    }
    result = TRANS_CovertDtapErrorCode(ret);
END:
    transportData->buff = NULL;
    transportData->result = result;
    // transportData->buff的释放在TRANS_SendDataToDtap内和返回值判断中闭环
    TRANS_TransportDataFree(transportData);
}

static TRANS_Data_S *TRANS_InitTransportData(TRANS_Addr_S *addr)
{
    if (addr == NULL) {
        return NULL;
    }
    TRANS_Data_S *transportData = SDF_MemAlloc(sizeof(TRANS_Data_S));
    if (transportData == NULL) {
        return NULL;
    }
    atomic_init(&transportData->refCount, 1);
    (void)memcpy_s(&transportData->addr, sizeof(TRANS_Addr_S), addr, sizeof(TRANS_Addr_S));
    return transportData;
}

static void TRANS_PostTaskToSendDataFailed(uint32_t postResult, TRANS_Data_S *transportData)
{
    if (postResult != NLSTK_ERRCODE_TASK_TIMEOUT) {
        // 若任务未成功递交给发送线程，则主动清理buff资源
        TRANS_Data_Release(transportData);
        SDF_BuffFree(transportData->buff);
    }
    TP_LOGE("post task failed, ret: %08x", postResult);
    // 如果返回错误码是超时，则说明buff数据已递交协议栈主线程，此处不做buff清理以免use-after-free
    TRANS_TransportDataFree(transportData);
}

uint32_t TRANS_SendData(TRANS_Addr_S *addr, const uint8_t *data, uint16_t dataLen)
{
    uint32_t ret;
    uint16_t headerSize;
    SDF_Buff_S *buff = NULL;
    uint8_t *payload = NULL;
    TRANS_Data_S *transportData = NULL;

    if (addr == NULL || data == NULL || dataLen == 0) {
        TP_LOGE("invalid addr or params");
        return TRANS_SEND_DATA_INVALID_PARAMS;
    }

    headerSize = addr->proto == TRANS_PROTO_CONNECTIONLESS ? TRANS_CltpHeaderSize() : 0;
    if (dataLen > DTAP_MAX_PAYLOAD_LEN - headerSize) {
        TP_LOGE("invalid data len: %hu", dataLen);
        return TRANS_SEND_DATA_INVALID_PARAMS;
    }

    transportData = TRANS_InitTransportData(addr);
    if (transportData == NULL) {
        TP_LOGE("transport data malloc failed");
        return TRANS_SEND_DATA_MALLOC_FAILED;
    }

    buff = SDF_BuffNewWithExtraReserve(dataLen, headerSize);
    if (buff == NULL) {
        TP_LOGE("buff new failed");
        TRANS_TransportDataFree(transportData);
        return TRANS_SEND_DATA_MALLOC_FAILED;
    }
    transportData->buff = buff;

    payload = SDF_BuffAppend(buff, dataLen);
    if (payload == NULL) {
        TP_LOGE("buff append failed");
        SDF_BuffFree(transportData->buff);
        TRANS_TransportDataFree(transportData);
        return TRANS_SEND_DATA_APPEND_FAILED;
    }
    (void)memcpy_s(payload, dataLen, data, dataLen);

    // 存在Task超时仍未开始处理的情况，故此处就需持有transportData，若投递任务失败则将资源计数器次数减去。
    TRANS_Data_Retain(transportData);
    ret = CP_PostTaskBlocked(TRANS_SendDataProc, transportData, NULL, TRANS_SEND_DATA_TIMEOUT_MS);
    if (ret != CP_OK) {
        TRANS_PostTaskToSendDataFailed(ret, transportData);
        return TRANS_SEND_DATA_POST_FAILED;
    }
    TP_LOGD("transportData->result ret : %d.", transportData->result);
    ret = transportData->result;
    // 如果CP_PostTaskBlocked超时，此处增加计数器判断，进行延迟释放
    TRANS_TransportDataFree(transportData);
    return ret;
}

uint32_t TRANS_Init(void)
{
    uint32_t ret;

    if (g_transIsInited) {
        TP_LOGE("trans module has been inited.");
        return TRANS_INIT_DUPLICATE_FAILED;
    }

    ret = DTAP_RegisterProtoRecvCbk(DTAP_PI_LWCLTP, TRANS_DataRecvCbk);
    if (ret != DTAP_SUCCESS) {
        return TRANS_INIT_REGISTER_FAILED;
    }

    g_transIsInited = true;
    return TRANS_SUCCESS;
}

void TRANS_DeInit(void)
{
    if (!g_transIsInited) {
        TP_LOGE("trans module has not been inited.");
        return;
    }

    (void)DTAP_UnregisterProtoRecvCbk(DTAP_PI_LWCLTP);
    g_transIsInited = false;
}