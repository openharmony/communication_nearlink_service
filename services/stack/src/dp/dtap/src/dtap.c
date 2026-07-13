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

#include "dtap.h"

#include <arpa/inet.h>

#include "securec.h"

#include "byte_codec.h"
#include "cm_errno.h"
#include "cm_trans_channel_api.h"
#include "cm_signaling_internal.h"
#include "cp_worker.h"
#include "dpfwk_log.h"
#include "dtap_channel.h"
#include "dtap_errno.h"
#include "dtap_frame.h"
#include "dtap_tcid.h"
#include "dtap_trans.h"
#include "sdf_errdef.h"
#include "sdf_mem.h"

enum {
    DTAP_REGISTER_DATA_RECV_CBK = 0,
    DTAP_UNREGISTER_DATA_RECV_CBK
};

typedef struct DTAP_DataRecvCbkEntry {
    SDF_DListEntry_S entry;
    uint8_t tcid;
    DTAP_DataRecvCb cb;
} DTAP_DataRecvCbkEntry_S;

// 固定传输通道收包回调
static SDF_DListHead_S g_dtapRecvCbks = {
    .list = {&(g_dtapRecvCbks.list), &(g_dtapRecvCbks.list)},
    .size = 0,
};

static volatile bool g_dtapIsInited = false;
static DTAP_DataRecvCb g_dtapProtoRecvCbks[DTAP_PI_MAX] = {NULL};   // 上层协议收包回调

static DTAP_DataRecvCb DTAP_GetProtoRecvCbk(uint8_t pi)
{
    return pi < DTAP_PI_MAX ? g_dtapProtoRecvCbks[pi] : NULL;
}
static DTAP_Data_Send_Cbks_S g_dtapDataSendCbks = { NULL };

void DTAP_RegisterDataSendCbks(const DTAP_Data_Send_Cbks_S *cbks)
{
    CHECK_AND_RETURN_LOG(DTAP_TAG, cbks != NULL && cbks->transChannelStatusChangeCbk != NULL, "cbks is null.");
    g_dtapDataSendCbks = *cbks;
}

void DTAP_UnRegisterDataSendCbks(void)
{
    DTAP_Data_Send_Cbks_S cbks = {NULL};
    g_dtapDataSendCbks = cbks;
}

uint32_t DTAP_RegisterProtoRecvCbk(uint8_t pi, DTAP_DataRecvCb cbk)
{
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, g_dtapIsInited, DTAP_MODULE_NOT_INITED_ERR, "dtap module has not been inited.");
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, pi < DTAP_PI_MAX, DTAP_REGISTER_RECV_CB_INVALID_PI, "invalid pi: %hhu.", pi);
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, cbk != NULL, DTAP_REGISTER_RECV_CB_NULL_ERR, "cbk is null.");
    g_dtapProtoRecvCbks[pi] = cbk;
    return DTAP_SUCCESS;
}

uint32_t DTAP_UnregisterProtoRecvCbk(uint8_t pi)
{
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, g_dtapIsInited, DTAP_MODULE_NOT_INITED_ERR, "dtap module has not been inited.");
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, pi < DTAP_PI_MAX, DTAP_REGISTER_RECV_CB_INVALID_PI, "invalid pi: %hhu.", pi);
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, g_dtapProtoRecvCbks[pi] != NULL, DTAP_REGISTER_RECV_CB_INVALID_PI,
        "pi: %hhu don't register recv cbk before.", pi);
    g_dtapProtoRecvCbks[pi] = NULL;
    return DTAP_SUCCESS;
}

static void DTAP_DataRecvCbksInit(void)
{
    SDF_DListHeadInit(&g_dtapRecvCbks);
}

static void DTAP_ClearDataRecvCbks(void)
{
    DTAP_DataRecvCbkEntry_S *cbkEntry = NULL;
    DTAP_DataRecvCbkEntry_S *tmp = NULL;
    SDF_DListElmSafeForeach(cbkEntry, tmp, &g_dtapRecvCbks, entry) {
        SDF_DListElmDel(&g_dtapRecvCbks, cbkEntry, entry);
        DTAP_LOGI("clear data recv cbk entry, tcid: %hhu.", cbkEntry->tcid);
        SDF_MemFree(cbkEntry);
    }
}

static DTAP_DataRecvCb DTAP_GetDataRecvCbkByTcid(uint8_t tcid)
{
    DTAP_DataRecvCbkEntry_S *cbkEntry = NULL;
    SDF_DListElmForeach(cbkEntry, &g_dtapRecvCbks, entry) {
        if (cbkEntry->tcid == tcid) {
            return cbkEntry->cb;
        }
    }

    return NULL;
}

static uint32_t DTAP_AddDataRecvCbkEntry(uint8_t tcid, DTAP_DataRecvCb cb)
{
    DTAP_DataRecvCbkEntry_S *cbkEntry = NULL;
    SDF_DListElmForeach(cbkEntry, &g_dtapRecvCbks, entry) {
        if (cbkEntry->tcid == tcid) {
            cbkEntry->cb = cb;
            DTAP_LOGI("update data recv cbk entry, tcid: %hhu.", tcid);
            return DTAP_SUCCESS;
        }
    }

    cbkEntry = SDF_MemZalloc(sizeof(DTAP_DataRecvCbkEntry_S));
    if (cbkEntry == NULL) {
        DTAP_LOGE("add data recv cbk entry failed, tcid: %hhu.", tcid);
        return DTAP_REGISTER_RECV_CB_ALLOC_ERR;
    }

    cbkEntry->tcid = tcid;
    cbkEntry->cb = cb;
    SDF_DListEntryInit(&cbkEntry->entry);
    SDF_DListElmHeadInsert(&g_dtapRecvCbks, cbkEntry, entry);
    return DTAP_SUCCESS;
}

static uint32_t DTAP_DelDataRecvCbkEntry(uint8_t tcid)
{
    DTAP_DataRecvCbkEntry_S *cbkEntry = NULL;
    DTAP_DataRecvCbkEntry_S *tmp = NULL;
    SDF_DListElmSafeForeach(cbkEntry, tmp, &g_dtapRecvCbks, entry) {
        if (cbkEntry->tcid == tcid) {
            SDF_DListElmDel(&g_dtapRecvCbks, cbkEntry, entry);
            SDF_MemFree(cbkEntry);
            return DTAP_SUCCESS;
        }
    }
    DTAP_LOGW("delete data recv cbk entry failed, tcid: %hhu not exist.", tcid);
    return DTAP_UNREGISTER_RECV_CB_TCID_NOT_EXIST;
}

static uint32_t DTAP_RegisterDataRecvCbInner(uint8_t tcid, DTAP_DataRecvCb cb, uint8_t action)
{
    uint32_t ret = DTAP_SUCCESS;
    const char *str = (action == DTAP_REGISTER_DATA_RECV_CBK ? "register" : "unregister");
    DTAP_LOGI("start to %s dtap data recv cb, tcid: %hhu", str, tcid);

    switch (action) {
        case DTAP_REGISTER_DATA_RECV_CBK:
            ret = DTAP_AddDataRecvCbkEntry(tcid, cb);
            break;
        case DTAP_UNREGISTER_DATA_RECV_CBK:
            ret = DTAP_DelDataRecvCbkEntry(tcid);
            break;
        default:
            DTAP_LOGW("unknown register data recv callback action: %hhu", action);
            break;
    }
    return ret;
}

uint32_t DTAP_RegisterDataRecvCb(uint8_t tcid, DTAP_DataRecvCb cb)
{
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, g_dtapIsInited, DTAP_MODULE_NOT_INITED_ERR, "dtap module has not been inited.");
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, cb != NULL, DTAP_REGISTER_RECV_CB_NULL_ERR, "cbk is null.");
    return DTAP_RegisterDataRecvCbInner(tcid, cb, DTAP_REGISTER_DATA_RECV_CBK);
}

uint32_t DTAP_UnregisterDataRecvCb(uint8_t tcid)
{
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, g_dtapIsInited, DTAP_MODULE_NOT_INITED_ERR, "dtap module has not been inited.");
    return DTAP_RegisterDataRecvCbInner(tcid, NULL, DTAP_UNREGISTER_DATA_RECV_CBK);
}

static void DTAP_DataRecvFrame(DTAP_Channel_S *transChan, const DTAP_TransMode_S *transMode,
                               SDF_Buff_S *buff, DTAP_DataRecvCb recvCb)
{
    uint32_t ret;
    uint64_t frameLen;
    uint16_t mps = transChan->mps;
    DTAP_Frame_S frame = { 0 };
    uint64_t len = SDF_DataLenGet(buff);
    DTAP_DataRecvCb recvCbTmp = NULL;
    if (len < DTAP_BASIC_HEADER_LEN) {
        DTAP_LOGE("data len %llu is less than %u.", len, DTAP_BASIC_HEADER_LEN);
        return;
    }

    DTAP_BasicHeader_S *basicHeader = (DTAP_BasicHeader_S *)SDF_DataOffset(buff);
    frameLen = (uint64_t)(DECODE2BYTE_LITTLE((uint8_t *)&basicHeader->length) + DTAP_BASIC_HEADER_LEN);
    if (len != frameLen) {
        DTAP_LOGE("data len %llu is not equal to frameLen %llu.", len, frameLen);
        return;
    }

    ret = DTAP_ParseFrame(basicHeader->frameType, buff, &frame);
    if (ret != DTAP_SUCCESS) {
        DTAP_LOGE("parse frame[%hhu] failed, result: %u.", basicHeader->frameType, ret);
        return;
    }

    if (frame.payloadLen > mps) {
        DTAP_LOGE("payload len %hu is greater than trans mps %hu.", frame.payloadLen, mps);
        return;
    }

    if (!frame.ctx.checkFrameHeader(&frame.ctx)) {
        DTAP_LOGE("check frame header failed.");
        return;
    }

    if (!transMode->checkFrame(transChan, &frame)) {
        DTAP_LOGE("check frame failed.");
        return;
    }

    recvCbTmp = recvCb != NULL ? recvCb : DTAP_GetProtoRecvCbk(frame.pi);
    ret = transMode->recvFrame(transChan, &frame, recvCbTmp);
    if (ret != 0) {
        DTAP_LOGE("receive frame failed, result: 0x%08x, submod: %u, errno: %u.", ret,
            SDF_ErrGetSubmodId(ret), SDF_ErrGetErrId(ret));
        return;
    }
}

void DTAP_DataRecv(uint16_t lcid, SDF_Buff_S *buf)
{
    uint8_t tcid;
    uint8_t modeType;
    DTAP_Channel_S *transChan = NULL;
    DTAP_TransMode_S *transMode = NULL;
    DTAP_DataRecvCb recvCb = NULL;
    uint64_t dataLen = 0;

    if (buf == NULL || SDF_DataLenGet(buf) == 0) {
        DTAP_LOGE("data is null or data length is invalid.");
        return;
    }

    dataLen = SDF_DataLenGet(buf);
    tcid = *(uint8_t *)SDF_DataOffset(buf);
    DTAP_LOGD("recv dtap data, lcid: %hu, tcid: 0x%02x, len: %llu.", lcid, tcid, dataLen);
    transChan = DTAP_ChannelSearch(lcid, tcid);
    if (transChan == NULL) {
        DTAP_LOGE("search transmission channel failed, lcid: %hu, tcid: 0x%02x", lcid, tcid);
        return;
    }

    modeType = transChan->mode;
    transMode = DTAP_GetTransMode(modeType);
    if (transMode == NULL) {
        DTAP_LOGE("get transmission mode failed, mode type: %hhu", modeType);
        return;
    }

    recvCb = DTAP_GetDataRecvCbkByTcid(transChan->srcTcid);
    // 透传模式不需要解析帧，直接透传给上层业务
    if (transMode->transFrame != NULL) {
        if (dataLen > transChan->mps) {
            DTAP_LOGE("data len %llu is greater than trans channel mps %hu.", dataLen, transChan->mps);
            return;
        }
        transMode->transFrame(transChan, buf, recvCb);
        return;
    }

    DTAP_DataRecvFrame(transChan, transMode, buf, recvCb);
}

void DTAP_ChannelSetStatus(uint16_t lcid, uint8_t tcid, uint16_t result)
{
    uint8_t modeType;
    DTAP_Channel_S *transChan = NULL;
    DTAP_TransMode_S *transMode = NULL;
    DTAP_DataRecvCb recvCb = NULL;
    transChan = DTAP_ChannelSearch(lcid, tcid);
    if (transChan == NULL) {
        DTAP_LOGE("search transmission channel failed");
        return;
    }
    recvCb = DTAP_GetDataRecvCbkByTcid(transChan->srcTcid);
    modeType = transChan->mode;
    transMode = DTAP_GetTransMode(modeType);
    if (transMode == NULL) {
        DTAP_LOGE("get transmission mode failed, mode type: %hhu", modeType);
        return;
    }
    if (transMode->setTransChannelStatus == NULL) {
        DTAP_LOGE("setTransChannelStatus failed");
        return;
    }
    transMode->setTransChannelStatus(transChan, result, recvCb);
}

uint32_t DTAP_DataSend(DTAP_Data_S *data)
{
    uint8_t modeType;
    DTAP_Channel_S *transChan = NULL;
    DTAP_TransMode_S *transMode = NULL;
    uint64_t dataLen = 0;

    if (data == NULL || data->buff == NULL) {
        DTAP_LOGE("data or data buff is null");
        return DTAP_TRANS_INVALID_DATA;
    }

    dataLen = SDF_DataLenGet(data->buff);
    if (dataLen == 0 || dataLen > DTAP_MAX_PAYLOAD_LEN) {
        DTAP_LOGE("invalid data len: %llu", dataLen);
        return DTAP_TRANS_INVALID_DATA;
    }

    DTAP_LOGD("send dtap data, pi: %hhu, lcid: %hu, tcid: 0x%02x, len: %llu.", data->pi, data->lcid,
              data->tcid, dataLen);
    transChan = DTAP_ChannelSearch(data->lcid, data->tcid);
    if (transChan == NULL) {
        DTAP_LOGE("search transmission channel failed");
        return DTAP_TRANS_FIND_CHANNEL_ERR;
    }

    if (dataLen > transChan->mtu) {
        DTAP_LOGE("dataLens %llu is greater than mtu %hu.", dataLen, transChan->mtu);
        return DTAP_TRANS_EXCEED_MTU_ERR;
    }

    modeType = transChan->mode;
    transMode = DTAP_GetTransMode(modeType);
    if (transMode == NULL) {
        DTAP_LOGE("get transmission mode failed, mode type: %hhu", modeType);
        return DTAP_TRANS_INVALID_MODULE_TYPE;
    }

    return transMode->sendFrame(transChan, data->pi, data->buff);
}

static inline uint32_t DTAP_CMSignalingDataSendCbk(uint8_t pi, uint8_t tcid,
    uint16_t lcid, SDF_Buff_S *buff)
{
    DTAP_Data_S data;
    data.pi = pi;
    data.tcid = tcid;
    data.lcid = lcid;
    data.buff = buff;
    return DTAP_DataSend(&data);
}

uint32_t DTAP_Init(void)
{
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, !g_dtapIsInited, DTAP_MODULE_DUP_INITED_ERR, "dtap module has been inited.");
    g_dtapIsInited = true;

    // 初始化数据接收回调链表
    DTAP_DataRecvCbksInit();

    // 注册帧格式
    DTAP_RegisterBasicFrameCtx();
    DTAP_RegisterEnhanceFrameCtx();

    // 注册传输模式
    DTAP_RegisterBasicTransMode();
    DTAP_RegisterTransparentMode();
    DTAP_RegisterStreamMode();
    DTAP_RegisterReliableTransMode();

    uint32_t ret = CM_SignalingInit();
    if (ret != CM_SUCCESS) {
        DTAP_LOGE("dtap module init cm signalling not success.");
        g_dtapIsInited = false;
        return ret;
    }

    CM_SetSendSignalingDataCbk(DTAP_CMSignalingDataSendCbk);
    // 注册CM收包回调
    ret = DTAP_RegisterDataRecvCb(TCID_SLE_CMTC, CM_RecvSignalingData);
    if (ret != DTAP_SUCCESS) {
        DTAP_LOGE("register cm signaling recv cbk failed.");
        g_dtapIsInited = false;
        CM_SignalingDeInit();
        return ret;
    }

    ret = DTAP_ChannelInit();
    if (ret != DTAP_SUCCESS) {
        DTAP_LOGE("dtap channel init failed.");
        g_dtapIsInited = false;
        CM_SignalingDeInit();
        DTAP_ClearDataRecvCbks();
        return ret;
    }

    DTAP_LOGI("dtap module init success.");
    return DTAP_SUCCESS;
}

void DTAP_DeInit(void)
{
    CHECK_AND_RETURN_LOG(DTAP_TAG, g_dtapIsInited, "dtap module has not been inited.");
    DTAP_ChannelDeInit();
    CM_SignalingDeInit();
    DTAP_ClearDataRecvCbks();

    DTAP_LOGI("dtap module deinit success.");
    g_dtapIsInited = false;
}

static uint8_t DTAP_CovertErrorCode(uint32_t result)
{
    switch (result) {
        case DTAP_SUCCESS:
            return DTAP_SUCCESS;
        case DTAP_TRANS_INVALID_DATA:
            return DTAP_TRANS_RESULT_INVALID_DATA;
        case DTAP_TRANS_FIND_CHANNEL_ERR:
            return DTAP_TRANS_RESULT_CHANNEL_NOT_FOUND;
        case DTAP_TRANS_EXCEED_MTU_ERR:
            return DTAP_TRANS_RESULT_EXCEED_MTU_ERR;
        case DTAP_TRANS_RELIABLE_TX_WINDOW_FULL:
            return DTAP_TRANS_RESULT_TX_CACHE_FULL;
        default:
            return DTAP_TRANS_RESULT_INTERNAL_FAULT;
    }
}

static void DTAP_TransSendCacheBuff(DTAP_Channel_S *channel, DTAP_ReliableChannel_S *reliable, uint8_t result)
{
    if (channel == NULL || reliable == NULL) {
        return;
    }
    DTAP_TransMode_S *transMode = DTAP_GetTransMode(channel->mode);
    if (transMode == NULL) {
        DTAP_LOGE("get transmission mode failed, mode type: %hhu", channel->mode);
        return;
    }
    // 缓存之前已对长度进行校验，无需在做额外判断
    uint8_t headerLen = reliable->isRecvFrame ? DTAP_DUPLEX_FRAG_FRAME_HEADER_LEN : DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN;
    uint16_t mps = channel->mps - headerLen - DTAP_CRC_LEN;
    uint16_t frameCnt = DTAP_GetFragmentFramesNum(SDF_DataLenGet(reliable->cacheBuff), mps);
    // 如果当前这个包会塞满发送队列或超过发送队列，则等待下次触发发送
    if (SDF_DListCount(&(reliable->txWindow.txList)) + frameCnt >= reliable->txWindow.size) {
        DTAP_LOGD("cacheBuff could not be sent because the transmission window is not large enough.");
        return;
    }
    // 此时txWindow不满，且缓存的包不会导致txWindow满，需要将缓存的包发送出去
    reliable->isTxWindowFull = false;
    uint32_t ret = transMode->sendFrame(channel, reliable->cachePi, reliable->cacheBuff);
    uint8_t transResult = DTAP_CovertErrorCode(ret);
    DTAP_LOGD("cacheBuff is sent and return %hhu", transResult);
    if (transResult != DTAP_SUCCESS) {
        // 如果满足发送队列长度条件后仍未发送成功，将报文丢弃
        SDF_BuffFree(reliable->cacheBuff);
        DTAP_LOGW("send cacheBuff failed, ret = %hhu", transResult);
    }
    // 如果发送成功，在sendFrame中已释放buff，仅需将cacheBuff置为NULL
    reliable->cacheBuff = NULL;
    if (g_dtapDataSendCbks.transChannelStatusChangeCbk != NULL) {
        g_dtapDataSendCbks.transChannelStatusChangeCbk(channel->lcid, channel->srcTcid, result);
    }
}

void DTAP_TransChannelStatusChange(DTAP_Channel_S *channel, uint8_t result)
{
    if (channel->mode != CM_TRANS_MODE_RELIABLE) {
        DTAP_LOGD("channel->mode is not Reliable");
        return;
    }
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)channel->attr;
    if (!reliable->isTxWindowFull) {
        DTAP_LOGD("TxWindow is not full");
        return;
    }

    // 缓存的Buff已经校验，所以格式等都不会有问题
    if (reliable->cacheBuff != NULL) {
        DTAP_TransSendCacheBuff(channel, reliable, result);
        return;
    }
    DTAP_LOGD("reliable->cacheBuff is NULL");

    if (SDF_DListCount(&(reliable->txWindow.txList)) < reliable->txWindow.size) {
        DTAP_LOGD("reliable->isTxWindowFull is set false");
        reliable->isTxWindowFull = false;
        if (g_dtapDataSendCbks.transChannelStatusChangeCbk != NULL) {
            g_dtapDataSendCbks.transChannelStatusChangeCbk(channel->lcid, channel->srcTcid, result);
        }
    }
}