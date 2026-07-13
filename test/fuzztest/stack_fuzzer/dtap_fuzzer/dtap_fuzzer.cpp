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
#include "dtap_fuzzer.h"
#include "dtap_errno.h"
#include "cm_dli_adapter.h"
#include "cm_errno.h"
#include "cm_trans_channel_api.h"
#include "cm_dyn_trans_channel_api.h"
#include "cm_trans_channel_mgr.h"
#include "cp_worker.h"
#include "dli_errno.h"
#include "dli_layer.h"
#include "dli_layer_callback.h"
#include "dli_layer_config.h"
#include "dli_layer_stru.h"
#include "dli_layer_utils.h"
#include "dpfwk_log.h"
#include "dtap.h"
#include "dtap_scheduler.h"
#include "dtap_tcid.h"
#include "dtap_trans.h"
#include "securec.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "nlstk_schedule.h"

#define ENCODE2BYTE_LITTLE(_ptr, data) do { \
    *(uint8_t *)((_ptr) + 1) = (uint8_t)((uint16_t)(data) >> 8); \
    *(uint8_t *)(_ptr) = (uint8_t)(data); \
} while (0)
#define DECODE2BYTE_LITTLE(_ptr)     (uint16_t)(*(uint8_t *)(_ptr) | (*(uint8_t *)((_ptr) + 1) << 8))

#define TEST_DEFAULT_NUM 30
#define MAX_LCID 2
#define MAX_TCID 5

#define TEST_DEFAULT_ACB_NUM 100
#define TEST_DEFAULT_ACB_LEN 1500

#define TEST_CRC16_INIT 0x5555

typedef struct {
    SDF_DListEntry_S entry;
    uint8_t sar; /* 分片状态指示 */
    uint16_t txSeq;
    SDF_Buff_S *buff;
    uint64_t timeStamp;
    uint8_t pi;
    uint16_t lcid;
    uint8_t tcid;
} StreamStubNode;

extern "C" uint32_t CM_GetTransChannelByLcid(uint16_t lcid, CM_TransChan_S *transChan)
{
	return CM_SUCCESS;
}

extern "C" uint32_t CM_GetTransChannelBySrcTcid(uint16_t lcid, uint8_t srcTcid, CM_TransChan_S *transChan)
{
    return CM_SUCCESS;
}

static CM_LogicLinkCbks_S g_logicLinkCbks = {0};
extern "C" uint32_t CM_RegLogicLinkListener(CM_LogicLinkCbks_S *cbks)
{
    if (cbks != nullptr) {
        g_logicLinkCbks = *cbks;
    }
    return CM_SUCCESS;
}

extern "C" uint32_t CM_GetLogicLinkConnectedSize(void)
{
    return 0;
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

DLI_RecvAcbHandlerPtr g_FuzzDtapDataRecv = DTAP_DataRecv;
CM_TransChannelCbk g_FuzzTransChannelCbk = nullptr;
uint8_t g_headerLen = 0;
uint8_t g_frameType = 0;

extern "C" int DTAP_DataRecvCbStub(DTAP_Data_Info_S *info, SDF_Buff_S *buff)
{
    return 0;
}

extern "C" void CM_UnRegTransChannelListener(void)
{
    return;
}

extern "C" uint32_t CM_SignalingInit(void)
{
    return 0;
}

extern "C" uint32_t CM_RegTransChannelListener(CM_TransChannelCbk cbk)
{
    g_FuzzTransChannelCbk = cbk;
    return 0;
}

extern "C" uint32_t CM_UnRegLogicLinkListener(uint8_t moduleId)
{
    return CM_SUCCESS;
}

extern "C" void CM_SignalingDeInit(void)
{
    return;
}

extern "C" void CM_SetSendSignalingDataCbk(uint32_t (*)(uint8_t pi, uint8_t tcid,
    uint16_t lcid, SDF_Buff_S *buff))
{
    return;
}

extern "C" void CM_RecvSignalingData(DTAP_Data_Info_S *info, SDF_Buff_S *buff)
{
    return;
}

extern "C" uint32_t DLI_ReadLocalMeasureCaps(void)
{
    return 0;
}

extern "C" SleTransLcid_S *CM_FindTransChannelByLocalTcid(uint16_t lcid, uint8_t tcid)
{
    return NULL;
}

extern "C" uint32_t CM_DynTransChannelReleaseReq(const CM_DynTransChannelReleaseParamReq_S *param)
{
    return 0;
}

extern "C" uint16_t DLI_DataNumGet(DLI_DataType type)
{
    return TEST_DEFAULT_ACB_NUM;
}

extern "C" uint16_t DLI_DataLenGet(DLI_DataType type)
{
    return TEST_DEFAULT_ACB_LEN;
}

extern "C" void DLI_DataNumChangeRegister(DLI_DataNumChangecbk cbk)
{}

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
    SDF_Buff_S *FuzzDtapCreateBuff(uint8_t* data, size_t size)
    {
        SDF_Buff_S *buff = SDF_BuffNewWithReserve(static_cast<uint32_t>(size));
        if (buff == nullptr) {
            return nullptr;
        }
        uint8_t *buffData = SDF_BuffAppend(buff, static_cast<uint32_t>(size));
        if (buffData == nullptr) {
            SDF_BuffFree(buff);
            return nullptr;
        }

        memcpy_s(buffData, static_cast<uint32_t>(size), data, static_cast<uint32_t>(size));
        return buff;
    }

    DTAP_Frame_S* FuzzDtapGenFrame(uint8_t* data, size_t size, uint8_t frameType)
    {
        FuzzedDataProvider provider(data, size);
        DTAP_Frame_S* dtapFrame = DTAP_CreateFrame(frameType);
        if (dtapFrame == nullptr) {
            return nullptr;
        }
        uint16_t payloadLen = size > DTAP_MAX_PAYLOAD_LEN ? DTAP_MAX_PAYLOAD_LEN : size;
        dtapFrame->pi = provider.ConsumeIntegral<uint8_t>();
        dtapFrame->buff = FuzzDtapCreateBuff(data, payloadLen);
        if (dtapFrame->buff == nullptr) {
            DTAP_DestroyFrame(dtapFrame);
            return nullptr;
        }
        uint8_t tcid = provider.ConsumeIntegral<uint8_t>() % MAX_TCID;
        uint16_t crcInit = TEST_CRC16_INIT;
        if (dtapFrame->ctx.buildFrame(&dtapFrame->ctx, tcid, crcInit) != DTAP_SUCCESS) {
            DTAP_DestroyFrame(dtapFrame);
            return nullptr;
        }
        dtapFrame->header = (uint8_t *)SDF_DataOffset(dtapFrame->buff);
        if (dtapFrame->ctx.parseFrame(&dtapFrame->ctx) != DTAP_SUCCESS) {
            DTAP_DestroyFrame(dtapFrame);
            return nullptr;
        }
        DTAP_BasicFrameHeader_S *basicHeader = (DTAP_BasicFrameHeader_S *)dtapFrame->header;
        uint8_t *typeBits = &(basicHeader->header.typeBits);
        *typeBits = provider.ConsumeIntegral<uint8_t>();
        basicHeader->header.frameType = frameType;
        DTAP_SimplexFragFrameHeader_S *simFragHeader = nullptr;
        DTAP_DuplexFragFrameHeader_S *dupFragHeader = nullptr;
        DTAP_AckFrameHeader_S *ackHeader = nullptr;
        switch(dtapFrame->ctx.getFrameType()) {
            case DTAP_FRAME_BASIC:
                break;
            case DTAP_FRAME_SIMPLEX_AGGR:
            case DTAP_FRAME_SIMPLEX_FRAG:
                simFragHeader = (DTAP_SimplexFragFrameHeader_S *)dtapFrame->header;
                simFragHeader->txSeq = provider.ConsumeIntegral<uint16_t>() & 0xFFFC;
                simFragHeader->sar = provider.ConsumeIntegral<uint16_t>() & 0x0003;
                break;
            case DTAP_FRAME_DUPLEX_AGGR:
            case DTAP_FRAME_DUPLEX_FRAG:
                dupFragHeader = (DTAP_DuplexFragFrameHeader_S *)dtapFrame->header;
                dupFragHeader->txSeq = provider.ConsumeIntegral<uint16_t>() & 0xFFFC;
                dupFragHeader->sar = provider.ConsumeIntegral<uint16_t>() & 0x0003;
                dupFragHeader->reqSeq = provider.ConsumeIntegral<uint16_t>() & 0xFFFC;
                break;
            case DTAP_FRAME_ACK:
                ackHeader = (DTAP_AckFrameHeader_S *)dtapFrame->header;
                ackHeader->reqSeq = provider.ConsumeIntegral<uint16_t>() & 0xFFFC;
                ackHeader->sBit = provider.ConsumeIntegral<uint16_t>() & 0x0001;
                break;
            default:
                break;
        }
        if (frameType != DTAP_FRAME_BASIC) {
            (void)DTAP_ReCalculateCrcValue(crcInit, dtapFrame);
        }
        return dtapFrame;
    }

    void FuzzDtapOperateChannel(uint8_t castMode, uint16_t lcid, uint8_t tcid,
        uint8_t mode, CM_TransChannelState_E result)
    {
        if (g_FuzzTransChannelCbk == nullptr) {
            return;
        }
        SDF_Traits traits = { .dtor = nullptr };
        SDF_Vector_S *channelVector = SDF_CreateVector(traits);
        if (channelVector == NULL) {
            return;
        }
        CM_TransChan_S chan = {0};
        chan.castMode = castMode;
        chan.lcid = lcid;
        chan.srcTcid = tcid;
        chan.dstTcid = tcid;
        chan.config.transMode = mode;
        chan.config.mps = UINT16_MAX;
        chan.config.mtu = UINT16_MAX;
        if (chan.config.transMode == CM_TRANS_MODE_STREAM) {
            chan.config.streamMode.reorderTimeout = UINT16_MAX;
            chan.config.streamMode.crcInit = TEST_CRC16_INIT;
            chan.config.streamMode.flushTimeout = UINT16_MAX;
        } else if (chan.config.transMode == CM_TRANS_MODE_RELIABLE) {
            chan.config.reliableMode.reorderTimeout = UINT16_MAX;
            chan.config.reliableMode.crcInit = TEST_CRC16_INIT;
            chan.config.reliableMode.retransTimeout = UINT16_MAX;
            chan.config.reliableMode.rspTimeout = UINT16_MAX;
            chan.config.reliableMode.txWindow = UINT8_MAX;
            chan.config.reliableMode.maxTxThreshold = UINT8_MAX;
        }
        SDF_VectorEmplaceBack(channelVector, &chan);
        CM_TransChannelStateList_S param = {0};
        param.channelVector = channelVector;
        param.result = result;
        g_FuzzTransChannelCbk(&param);
        SDF_DestroyVector(channelVector);
        return;
    }

    int recvCbStub(DTAP_Data_Info_S *info, SDF_Buff_S *buf)
    {
        (void)info;
        (void)buf;
        return 0;
    }

    void FuzzDtapGetFrameCtx(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        DTAP_RegisterBasicFrameCtx();
        DTAP_RegisterEnhanceFrameCtx();
        uint8_t frameType = provider.ConsumeIntegral<uint8_t>();
        DTAP_GetFrameCtx(frameType);
    }

    bool FuzzDtapBuildFrame(uint8_t* data, size_t size, DTAP_Frame_S* dtapFrame)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t tcid = provider.ConsumeIntegral<uint8_t>() % (2 * TCID_FTC_RFU_END);
        uint16_t crcInit = provider.ConsumeIntegral<uint16_t>();
        if (dtapFrame->ctx.buildFrame(&dtapFrame->ctx, tcid, crcInit) != 0) {
            return false;
        }
        return true;
    }

    DTAP_Data_S *FuzzDtapCreateData(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        static DTAP_Data_S dtapData = {};
        (void)memset_s(&dtapData, sizeof(dtapData), 0, sizeof(dtapData));
        if (data == nullptr) {
            return nullptr;
        }

        dtapData.pi = provider.ConsumeIntegral<uint8_t>();
        dtapData.lcid = provider.ConsumeIntegral<uint16_t>() % MAX_LCID;
        dtapData.tcid = provider.ConsumeIntegral<uint8_t>() % MAX_TCID;
        dtapData.buff = FuzzDtapCreateBuff(data, size);
        return &dtapData;
    }

    void FuzzDtapRegisterRecvCb(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t tcid = provider.ConsumeIntegral<uint8_t>();
        DTAP_Data_Send_Cbks_S cbks = {0};

        DTAP_UnregisterDataRecvCb(tcid);
        DTAP_RegisterDataRecvCb(tcid, DTAP_DataRecvCbStub);
        DTAP_RegisterDataRecvCb(tcid, DTAP_DataRecvCbStub);
        DTAP_UnregisterDataRecvCb(tcid);
        DTAP_RegisterProtoRecvCbk(tcid, DTAP_DataRecvCbStub);
        DTAP_UnregisterProtoRecvCbk(tcid);
        DTAP_RegisterProtoRecvCbk(tcid, DTAP_DataRecvCbStub);
        DTAP_RegisterDataRecvCb(tcid, DTAP_DataRecvCbStub);
        DTAP_RegisterDataSendCbks(&cbks);
        DTAP_UnRegisterDataSendCbks();
    }

    void FuzzDtapDataSend(uint8_t* data, size_t size)
    {
        uint32_t ret = 0;
        DTAP_Data_S *dtapData = FuzzDtapCreateData(data, size);

        static uint8_t transMode = 0;
        FuzzDtapOperateChannel(0, 0, 0, transMode % (CM_TRANS_MODE_MAX + 1), CM_TRANS_CHANNEL_STATE_ACTIVATED);
        ret = DTAP_DataSend(dtapData);
        if (ret != 0 && dtapData != nullptr) {
            SDF_BuffFree(dtapData->buff);
        }
        FuzzDtapOperateChannel(0, 0, 0, transMode++, CM_TRANS_CHANNEL_STATE_RELEASED);
    }

    void FuzzDtapDataRecv(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint8_t tcid = 0;
        for (uint8_t transMode = CM_TRANS_MODE_BASIC; transMode <= CM_TRANS_MODE_MAX; transMode++) {
            FuzzDtapOperateChannel(0, 0, tcid++, transMode, CM_TRANS_CHANNEL_STATE_ACTIVATED);
        }

        for (uint8_t frameType = DTAP_FRAME_BASIC; frameType <= DTAP_FRAME_MAX; frameType++) {
            DTAP_Frame_S *frame = FuzzDtapGenFrame(data, size, frameType);
            if (frame == NULL) {
                continue;
            }

            if (g_FuzzDtapDataRecv != NULL) {
                g_FuzzDtapDataRecv(0, frame->buff);
            }

            DTAP_DestroyFrame(frame);
        }

        for (uint8_t transMode = CM_TRANS_MODE_BASIC; transMode <= CM_TRANS_MODE_MAX; transMode++) {
            FuzzDtapOperateChannel(0, 0, tcid--, transMode, CM_TRANS_CHANNEL_STATE_ACTIVATED);
        }
    }

    void FuzzDtapTransTransBasicMode(DTAP_TransMode_S *trans, uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        FuzzDtapOperateChannel(0, 0, TCID_SLE_SMTC, CM_TRANS_MODE_BASIC, CM_TRANS_CHANNEL_STATE_ACTIVATED);
        DTAP_Channel_S *transChan = DTAP_ChannelSearch(0, TCID_SLE_SMTC);
        if (transChan == NULL) {
            return;
        }

        DTAP_Frame_S *frame = FuzzDtapGenFrame(data, size, DTAP_FRAME_BASIC);
        if (frame == NULL) {
            return;
        }

        if (trans->checkFrame != NULL) {
            if (trans->checkFrame(transChan, frame) && trans->recvFrame != NULL) {
                (void)trans->recvFrame(transChan, frame, recvCbStub);
                if (g_logicLinkCbks.logicLinkCbk != nullptr) {
                    CM_LogicLinkState_S state = {.lcid = 0, .result = CM_LINK_STATE_CONNECTED};
                    g_logicLinkCbks.logicLinkCbk(&state);
                }
                (void)trans->recvFrame(transChan, frame, recvCbStub);
            }
        }

        if (trans->sendFrame != NULL) {
            if (trans->sendFrame(transChan, frame->pi, frame->buff) == 0) {
                frame->buff = NULL;
            }
        }
        DTAP_DestroyFrame(frame);
        FuzzDtapOperateChannel(0, 0, 0, CM_TRANS_MODE_BASIC, CM_TRANS_CHANNEL_STATE_RELEASED);
    }

    void FuzzDtapTransTransparentMode(DTAP_TransMode_S *trans, uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        FuzzDtapOperateChannel(0, 0, 0, CM_TRANS_MODE_TRANSPARENT, CM_TRANS_CHANNEL_STATE_ACTIVATED);
        DTAP_Channel_S *transChan = DTAP_ChannelSearch(0, 0);
        if (transChan == NULL) {
            return;
        }

        uint8_t pi = provider.ConsumeIntegral<uint8_t>();
        uint8_t buffLen = provider.ConsumeIntegral<uint16_t>();
        std::vector<uint8_t> buffData = provider.ConsumeBytes<uint8_t>(buffLen);
        SDF_Buff_S *buf = FuzzDtapCreateBuff(buffData.data(), buffData.size());
        if (buf == NULL) {
            return;
        }
        if (trans->sendFrame != NULL) {
            if (trans->sendFrame(transChan, pi, buf) == 0) {
                buf = NULL;
            }
        }
        if (trans->transFrame != NULL) {
            (void)trans->transFrame(transChan, buf, recvCbStub);
        }
        if (buf != NULL) {
            SDF_BuffFree(buf);
        }

        FuzzDtapOperateChannel(0, 0, 0, CM_TRANS_MODE_TRANSPARENT, CM_TRANS_CHANNEL_STATE_RELEASED);
    }

    void FuzzDtapTransTransStreamMode(DTAP_TransMode_S *trans, uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        FuzzDtapOperateChannel(0, 0, 0, CM_TRANS_MODE_STREAM, CM_TRANS_CHANNEL_STATE_ACTIVATED);
        DTAP_Channel_S *transChan = DTAP_ChannelSearch(0, 0);
        if (transChan == NULL) {
            return;
        }

        DTAP_Frame_S *frame = FuzzDtapGenFrame(data, size, DTAP_FRAME_SIMPLEX_FRAG);
        if (frame == NULL) {
            return;
        }

        if (trans->checkFrame != NULL) {
            if (trans->checkFrame(transChan, frame) && trans->recvFrame != NULL) {
                (void)trans->recvFrame(transChan, frame, recvCbStub);
            }
        }

        if (trans->sendFrame != NULL) {
            if (trans->sendFrame(transChan, frame->pi, frame->buff) == 0) {
                frame->buff = NULL;
            }
        }
        DTAP_DestroyFrame(frame);
        FuzzDtapOperateChannel(0, 0, 0, CM_TRANS_MODE_STREAM, CM_TRANS_CHANNEL_STATE_RELEASED);
    }

    void FuzzDtapTransTransReliableMode(DTAP_TransMode_S *trans, uint8_t* data, size_t size) {
        FuzzedDataProvider provider(data, size);
        FuzzDtapOperateChannel(0, 0, 0, CM_TRANS_MODE_RELIABLE, CM_TRANS_CHANNEL_STATE_ACTIVATED);
        DTAP_Channel_S *transChan = DTAP_ChannelSearch(0, 0);
        if (transChan == NULL) {
            return;
        }
        transChan->mps = 100;
        DTAP_ReliableChannel_S *reliableChan = (DTAP_ReliableChannel_S *)transChan->attr;
        reliableChan->txWindow.size = 2;
        reliableChan->rxWindow.size = 1;

        for (uint8_t frameType = DTAP_FRAME_BASIC; frameType < DTAP_FRAME_MAX; frameType++) {
            DTAP_Frame_S *frame = FuzzDtapGenFrame(data, size, frameType);
            if (frame == NULL) {
                continue;
            }

            if (trans->checkFrame != NULL) {
                if (trans->checkFrame(transChan, frame) && trans->recvFrame != NULL) {
                    (void)trans->recvFrame(transChan, frame, recvCbStub);
                }
            }

            if (trans->sendFrame != NULL) {
                if (trans->sendFrame(transChan, frame->pi, frame->buff) == 0) {
                    frame->buff = NULL;
                }
            }

            if (trans->setTransChannelStatus != NULL) {
                (void)trans->setTransChannelStatus(transChan, 0, recvCbStub);
            }

            DTAP_DestroyFrame(frame);
            DTAP_RspTimerExpireProc(transChan);
        }
        DTAP_TransChannelStatusChange(transChan, 0);
        reliableChan->txWindow.size = 100;
        DTAP_TransChannelStatusChange(transChan, 0);
        FuzzDtapOperateChannel(0, 0, 0, CM_TRANS_MODE_RELIABLE, CM_TRANS_CHANNEL_STATE_RELEASED);
    }

    void FuzzDtapTransModeApi(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        static uint8_t mode = 0;
        DTAP_TransMode_S *trans = DTAP_GetTransMode(mode % (CM_TRANS_MODE_MAX));
        if (trans == NULL) {
            return;
        }
        if (trans->getModeType == NULL) {
            return;
        }
        uint8_t transMode = trans->getModeType();
        switch (transMode) {
            case CM_TRANS_MODE_BASIC:
                FuzzDtapTransTransBasicMode(trans, data, size);
                break;
            case CM_TRANS_MODE_TRANSPARENT:
                FuzzDtapTransTransparentMode(trans, data, size);
                break;
            case CM_TRANS_MODE_STREAM:
                FuzzDtapTransTransStreamMode(trans, data, size);
                break;
            case CM_TRANS_MODE_RELIABLE:
                FuzzDtapTransTransReliableMode(trans, data, size);
                break;
            default:
                break;
        }
        mode++;
    }

    void FuzzDtapChannelSearch(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        static DTAP_Data_S dtapData = {};
        (void)memset_s(&dtapData, sizeof(dtapData), 0, sizeof(dtapData));
        if (data == nullptr) {
            return;
        }

        dtapData.pi = provider.ConsumeIntegral<uint8_t>();
        dtapData.lcid = provider.ConsumeIntegral<uint16_t>();
        dtapData.tcid = provider.ConsumeIntegral<uint8_t>();
        dtapData.buff = nullptr;
        (void)DTAP_ChannelSearch(dtapData.lcid, dtapData.tcid);
    }

    void FuzzDtapChannelStateChangeCbk(uint8_t* data, size_t size)
    {
        CM_TransChannelStateList_S param = {0};
        FuzzedDataProvider provider(data, size);
        if (data == nullptr || g_FuzzTransChannelCbk == nullptr) {
            return;
        }

        g_FuzzTransChannelCbk(nullptr);
        SDF_Traits traits = { .dtor = nullptr };
        SDF_Vector_S *channelVector = SDF_CreateVector(traits);
        if (channelVector == nullptr) {
            return;
        }
        CM_TransChan_S chan;
        chan.config.transMode = provider.ConsumeIntegral<uint8_t>() & (CM_TRANS_MODE_MAX + 1);
        SDF_VectorEmplaceBack(channelVector, &chan);

        param.channelVector = channelVector;
        for (uint8_t result = 0; result <= CM_TRANS_CHANNEL_STATE_RELEASED; result++) {
            param.result = result;
            g_FuzzTransChannelCbk(&param);
        }
        SDF_DestroyVector(channelVector);
    }

    void FuzzDtapRecvAggregateFrame(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);

        DTAP_Data_Info_S info;
        uint16_t payloadLen = size % DTAP_MAX_PAYLOAD_LEN;
        SDF_Buff_S *buff = FuzzDtapCreateBuff(data, payloadLen);
        if (buff == nullptr) {
            return;
        }
        DTAP_Frame_S dtapFrame;
        memset_s(&dtapFrame, sizeof(dtapFrame), 0, sizeof(dtapFrame));
        dtapFrame.payloadLen = payloadLen;
        dtapFrame.payload = SDF_DataOffset(buff);
        dtapFrame.buff = buff;
        DTAP_RecvAggregateFrame(&dtapFrame, &info, DTAP_DataRecvCbStub);
        SDF_BuffFree(buff);
    }

    void FuzzDtapRecvFragmentFrame(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        DTAP_Data_Info_S info;

        DTAP_Frame_S* dtapFrame = FuzzDtapGenFrame(data, size, DTAP_FRAME_SIMPLEX_FRAG);
        if (dtapFrame == nullptr) {
            return;
        }

        SDF_Buff_S *buffs = nullptr;
        for (uint8_t sar = DTAP_SAR_UNSEG; sar <= DTAP_SAR_LAST + 1; sar++) {
            dtapFrame->enhance.sar = sar;
            (void)DTAP_RecvFragmentFrame(&buffs, dtapFrame, &info, DTAP_DataRecvCbStub);
            (void)DTAP_RecvFragmentFrame(&buffs, dtapFrame, &info, DTAP_DataRecvCbStub);
        }

        if (buffs != nullptr) {
            SDF_BuffFree(buffs);
        }
        DTAP_DestroyFrame(dtapFrame);
    }

    void FuzzDtapChannelSetStatus(uint8_t* data, size_t size)
    {
        FuzzedDataProvider provider(data, size);
        uint16_t lcid = provider.ConsumeIntegral<uint16_t>() % MAX_LCID;
        uint8_t tcid = provider.ConsumeIntegral<uint8_t>() % MAX_TCID;
        uint16_t result = provider.ConsumeIntegral<uint16_t>() % 2;
        static uint8_t transMode = 0;
        static uint8_t castMode = 0;
        FuzzDtapOperateChannel(castMode % (CM_ACCESS_TRANS_MODE_MAX + 1), 0, 0,
            transMode % (CM_TRANS_MODE_MAX + 1), CM_TRANS_CHANNEL_STATE_ACTIVATED);
        DTAP_ChannelSetStatus(lcid, tcid, result);
        FuzzDtapOperateChannel(castMode++, 0, 0, transMode++, CM_TRANS_CHANNEL_STATE_RELEASED);
    }

    void FuzzDtapTransApi(uint8_t* data, size_t size)
    {
        // dtap trans fuzz
        FuzzDtapTransModeApi(data, size);

        // dtap frame fuzz
        FuzzDtapRecvAggregateFrame(data, size);
        FuzzDtapRecvFragmentFrame(data, size);

        // dtap channel fuzz
        FuzzDtapChannelSearch(data, size);
        FuzzDtapChannelStateChangeCbk(data, size);

        // dtap framework fuzz
        FuzzDtapRegisterRecvCb(data, size);
        FuzzDtapChannelSetStatus(data, size);
        FuzzDtapDataSend(data, size);
        FuzzDtapDataRecv(data, size);
    }
}

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;

    SDF_ThreadInit(TEST_DEFAULT_NUM);
    SDF_EvcInit();
    ScheduleEnable();
    DTAP_LOGI("dtap_fuzzer init success");
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
    DTAP_Init();
    OHOS::FuzzDtapTransApi(static_cast<uint8_t *>(fuzzData), size);
    DTAP_DeInit();
    free(fuzzData);
    return 0;
}