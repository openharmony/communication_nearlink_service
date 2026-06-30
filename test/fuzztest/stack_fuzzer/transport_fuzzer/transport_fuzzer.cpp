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

#include <cstdint>

#include "fuzzer/FuzzedDataProvider.h"
#include "transport_fuzzer.h"
#include "transport.h"
#include "transport_internal.h"
#include "transport_proto.h"
#include "transport_errno.h"
#include "transport_cltp.h"
#include "cm_dli_adapter.h"
#include "cm_errno.h"
#include "cm_trans_channel_api.h"
#include "dli_errno.h"
#include "dli_layer.h"
#include "dli_layer_callback.h"
#include "dli_layer_stru.h"
#include "dli_layer_utils.h"
#include "dpfwk_log.h"
#include "dtap_tcid.h"
#include "dtap_trans.h"
#include "securec.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_worker.h"

DTAP_DataRecvCb g_datpDataRecvCb = nullptr;

extern "C" void DTAP_ChannelSetStatus(uint16_t lcid, uint16_t tcid, uint16_t result)
{
    return;
}

extern "C" void DTAP_RegisterDataSendCbks(const DTAP_Data_Send_Cbks_S *cbks)
{
    return;
}

extern "C" void DTAP_UnRegisterDataSendCbks(void)
{
    return;
}

extern "C" uint32_t DTAP_RegisterDataRecvCb(uint8_t tcid, DTAP_DataRecvCb cb)
{
	g_datpDataRecvCb = cb;
    return 0;
}

extern "C" uint32_t DTAP_UnregisterDataRecvCb(uint8_t tcid)
{
	g_datpDataRecvCb = nullptr;
    return 0;
}

extern "C" uint32_t DTAP_RegisterProtoRecvCbk(uint8_t pi, DTAP_DataRecvCb cbk)
{
    return 0;
}

extern "C" uint32_t DTAP_UnregisterProtoRecvCbk(uint8_t pi)
{
    return 0;
}

extern "C" uint32_t DTAP_DataSend(DTAP_Data_S *data)
{
    return 0;
}

extern "C" uint32_t CM_GetLogicLinkByAddr(SLE_Addr_S *addr, CM_LogicLink_S *logicLink)
{
    return 1;
}

extern "C" uint32_t CM_GetLogicLinkByLcid(uint16_t lcid, CM_LogicLink_S *logicLink)
{
    return 0;
}

extern "C" uint32_t CP_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (cb != NULL) {
        cb(arg);
    }

    if (freeCb != NULL) {
        freeCb(arg);
    }

    return 0;
}

extern "C" uint32_t CP_PostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    return 1;
}

static int RecvDataCbk(const TRANS_Addr_S *addr, uint8_t *data, uint16_t len)
{
    return 0;
}

static void SendDataCbk(const SLE_Addr_S *addr, uint8_t tcid, uint16_t srcPort, uint8_t result)
{
    return;
}

extern "C" uint32_t CP_TimerAdd(int *handle, SDF_TimerParam *param)
{
    if (param != NULL && param->callback != NULL) {
         param->callback(param->args);
    }
    return 0;
}

extern "C" void CP_TimerDel(int handle)
{
    return;
}

extern "C" DLI_DataStru *DLI_DefaultDataStruCreate(uint16_t lcid, uint16_t type,
                                                   uint8_t ts, uint8_t prio, SDF_Buff_S *buf)
{
    DLI_DataStru *dliData = (DLI_DataStru *)SDF_MemZalloc(sizeof(DLI_DataStru));
    if (dliData == nullptr) {
        return nullptr;
    }

    dliData->lcid = lcid;
    dliData->type = type;
    dliData->ts = ts;
    dliData->prio = prio;
    dliData->buf = buf;
    return dliData;
}

extern "C" uint32_t DLI_DataSend(DLI_DataStru *data)
{
    if (data == nullptr) {
        return DLI_STACK_PARAMS_ERRNO;
    }

    SDF_BuffFree(data->buf);
    SDF_MemFree(data);
    DTAP_LOGI("DLI_DataSend success");
    return 0;
}

extern "C" uint32_t CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type, CM_DliCbk cbk)
{
    return 0;
}

extern "C" uint32_t CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type)
{
    return 0;
}

extern "C" uint32_t DLI_GetDataFragmentNums(SDF_Buff_S *buf)
{
    return 1;
}

extern "C" uint32_t DLI_GetFragmentMaxLen(void)
{
    return 605;
}

extern "C" uint32_t DLI_SplitData(DLI_DataStru *data, SDF_Buff_S *fragmentBuf[], uint32_t fragmentCnt)
{
    return 0;
}

namespace OHOS {
    SDF_Buff_S *FuzzDtapRecvBuff(uint8_t* data, size_t size)
    {
        SDF_Buff_S *buff = SDF_BuffNew(static_cast<uint32_t>(size));
        if (buff == nullptr) {
            return nullptr;
        }
        uint8_t *recvData = SDF_BuffAppend(buff, static_cast<uint32_t>(size));
        if (recvData == nullptr) {
            return buff;
        }

        memcpy_s(recvData, static_cast<uint32_t>(size), data, static_cast<uint32_t>(size));
        return buff;
    }

    void FuzzTransChannelSetStatus(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t tcid = provider.ConsumeIntegral<uint8_t>();
        uint8_t result = provider.ConsumeIntegral<uint8_t>();
        std::vector<uint8_t> addrData = provider.ConsumeBytes<uint8_t>(sizeof(SLE_Addr_S));
        SLE_Addr_S addr = {0};
        memcpy_s(&addr, sizeof(SLE_Addr_S), addrData.data(), addrData.size());
        TRANS_ChannelSetStatus(&addr, tcid, result);
    }

    void FuzzTransCltpHeaderBuild(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t srcPort = provider.ConsumeIntegral<uint16_t>();
        uint16_t dstPort = provider.ConsumeIntegral<uint16_t>();
        uint16_t buffSize = provider.ConsumeIntegral<uint16_t>();
        std::vector<uint8_t> buffData = provider.ConsumeBytes<uint8_t>(buffSize);
        if (buffData.size() < sizeof(TRANS_CltpHeaderOpts_S)) {
            return;
        }
        SDF_Buff_S *buff = FuzzDtapRecvBuff(buffData.data(), buffData.size());
        
        TRANS_CltpHeaderBuild(srcPort, dstPort, buff);
        SDF_BuffFree(buff);
    }

    void FuzzTransCltpPktProc(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);

        uint16_t buffSize = provider.ConsumeIntegral<uint16_t>();
        std::vector<uint8_t> buffData = provider.ConsumeBytes<uint8_t>(buffSize);
        if (buffData.size() < sizeof(TRANS_ProtoBasicHeader_S)) {
            return;
        }
        SDF_Buff_S *buff = FuzzDtapRecvBuff(buffData.data(), buffData.size());
        TRANS_ProtoBasicHeader_S *basicHeader = (TRANS_ProtoBasicHeader_S *)SDF_DataOffset(buff);
        basicHeader->version = TRANS_PROTO_VERSION;

        TRANS_CltpPktProc(buff);
        SDF_BuffFree(buff);
    }

    void FuzzTransDataRecvCbk(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        size_t minLen = sizeof(DTAP_Data_Info_S) + sizeof(SDF_Buff_S) + sizeof(TRANS_ProtoBasicHeader_S);
        if (size < minLen) {
            return;
        }

        DTAP_Data_Info_S info = {0};
        std::vector<uint8_t> infoData = provider.ConsumeBytes<uint8_t>(sizeof(DTAP_Data_Info_S));
        memcpy_s(&info, sizeof(DTAP_Data_Info_S), infoData.data(), infoData.size());
        info.pi = info.pi % 2 + 3;

        uint16_t buffSize = provider.ConsumeIntegral<uint16_t>();
        std::vector<uint8_t> buffData = provider.ConsumeBytes<uint8_t>(buffSize);
        if (buffData.size() < sizeof(TRANS_ProtoBasicHeader_S)) {
            return;
        }
        SDF_Buff_S *buff = FuzzDtapRecvBuff(buffData.data(), buffData.size());
        TRANS_ProtoBasicHeader_S *basicHeader = (TRANS_ProtoBasicHeader_S *)SDF_DataOffset(buff);
        basicHeader->version = TRANS_PROTO_VERSION;

        g_datpDataRecvCb(&info, buff);
        SDF_BuffFree(buff);
    }

    void FuzzTransRegisterCbks()
    {
        TRANS_RegisterCbks(nullptr);

        const TRANS_Cbks_S cbk = {
            .recvDataCbk = RecvDataCbk,
            .sendDataCbk = SendDataCbk,
        };
        TRANS_RegisterCbks(&cbk);
        TRANS_UnregisterCbks();
    }

    void FuzzTransSendData(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);

        TRANS_Addr_S addr = {};
        std::vector<uint8_t> addrData = provider.ConsumeBytes<uint8_t>(sizeof(TRANS_Addr_S));
        if (addrData.size() < sizeof(TRANS_Addr_S)) {
            return;
        }
        memcpy_s(&addr, sizeof(TRANS_Addr_S), addrData.data(), addrData.size());
        addr.proto = (TRANS_Protocol_t)(addr.proto % 2);

        uint16_t transSize = provider.ConsumeIntegral<uint16_t>();
        std::vector<uint8_t> transData = provider.ConsumeBytes<uint8_t>(transSize);
        if (transData.empty()) {
            return;
        }
        uint8_t *trans = transData.data();
        uint16_t dataLen = static_cast<uint16_t>(transData.size());
        TRANS_SendData(&addr, trans, dataLen);
    }

    void FuzzTansportLastApi()
    {
        TRANS_Init();
        TRANS_DeInit();
        TRANS_Init();
    }

    void FuzzTransportApi(uint8_t* data, size_t size)
    {
        FuzzTransCltpHeaderBuild(data, size);
        FuzzTransCltpPktProc(data, size);
        FuzzTransChannelSetStatus(data, size);
        FuzzTransDataRecvCbk(data, size);
        FuzzTransRegisterCbks();
        FuzzTransSendData(data, size);
        FuzzTansportLastApi();
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;

    int ret = TRANS_Init();
    if (ret != TRANS_SUCCESS) {
        TP_LOGE("transport_fuzzer init failed: %08x", ret);
        return -1;
    }
    TP_LOGE("transport_fuzzer init success");
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    uint8_t *fuzzData = (uint8_t*)malloc(size);
    if (fuzzData == nullptr) {
        return 0;
    }
    memcpy_s(fuzzData, size, data, size);
    OHOS::FuzzTransportApi(static_cast<uint8_t *>(fuzzData), size);
    free(fuzzData);
    return 0;
}