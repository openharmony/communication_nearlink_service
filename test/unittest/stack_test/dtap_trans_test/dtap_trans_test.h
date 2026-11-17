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

#ifndef DTAP_TRANS_TEST_H
#define DTAP_TRANS_TEST_H

#include <cstdint>
#include <cstring>

#include "gtest/gtest.h"
#include "securec.h"

#include "cm_trans_channel_api.h"
#include "cm_logic_link_api.h"
#include "dli_layer_config.h"
#include "dtap.h"
#include "dtap_channel.h"
#include "dtap_errno.h"
#include "dtap_frame.h"
#include "dtap_tcid.h"
#include "dtap_trans.h"
#include "dtap_scheduler.h"
#include "cm_trans_channel_mgr.h"
#include "dtap_scheduler.h"
#include "sdf_mem.h"

#include "CP_Timer_mocker.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dli_event_mock.h"
#include "stack_dli_event_stub.h"
#include "stack_dli_layer_mock.h"
#include "stack_dli_layer_stub.h"
#include "stack_sdf_mem_mock.h"
#include "stack_sdf_mem_stub.h"
#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"

#include "cm_dli_adapter.h"
#include "collab_ext_func_wrapper.h"
#include "dli_callback.h"
#include "dli_layer_stru.h"
#include "dli_cmd.h"
#include "dli_opcode.h"
#include "byte_codec.h"

#ifdef __cplusplus
extern "C" {
#endif
void TEST_DTAP_SetMonoTimeMs(uint64_t ms);
void TEST_DTAP_CollabReset(void);
void TEST_DTAP_SetApBufferNum(uint8_t num);
void TEST_DTAP_AcbNumChange(uint16_t type, uint16_t num);
void TEST_DTAP_DliConnectDo(uint16_t connHandle, uint16_t status);
void TEST_DTAP_DliDisconnectDo(uint16_t connHandle);

void TEST_DTAP_CollabCntReset(void);
uint32_t TEST_DTAP_PreAssignCnt(void);
uint32_t TEST_DTAP_ContinueAssignCnt(void);

void TEST_DTAP_StatusCbkReset(void);
void TEST_DTAP_RegisterStatusCbk(void);
uint32_t TEST_DTAP_StatusCnt(void);
uint8_t TEST_DTAP_LastStatusResult(void);

void TEST_DTAP_SentFramesReset(void);
uint32_t TEST_DTAP_DataSendCapture(DLI_DataStru *data);
uint8_t TEST_DTAP_SentFrameType(size_t idx);
uint8_t TEST_DTAP_SentFrameByte(size_t idx, size_t off);
size_t TEST_DTAP_SentFrameCnt(void);

void TEST_DTAP_RecvCbReset(int ret);
uint32_t TEST_DTAP_RecvCbCnt(void);
int TEST_DTAP_RecvCb(DTAP_Data_Info_S *info, SDF_Buff_S *buff);
#ifdef __cplusplus
}
#endif


namespace DtapTransTest {
constexpr uint16_t TEST_CRC_INIT = 0x5555;

inline SDF_Buff_S *NewBuff(uint16_t len)
{
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(len);
    if (buff != nullptr && SDF_BuffAppend(buff, len) == nullptr) {
        SDF_BuffFree(buff);
        return nullptr;
    }
    return buff;
}

inline DTAP_Frame_S *MakeDataFrame(uint8_t type, uint16_t txSeq, uint16_t payloadLen,
    uint16_t reqSeq = 0xFFFF, uint8_t sar = DTAP_SAR_UNSEG, uint8_t bits = DTAP_FRAME_C_BIT)
{
    DTAP_Frame_S *frame = DTAP_CreateFrame(type);
    if (frame == nullptr) {
        return nullptr;
    }
    frame->bits = bits;
    frame->pi = 0;
    frame->enhance.sar = sar;
    frame->enhance.txSeq = txSeq;
    frame->enhance.reqSeq = reqSeq;
    frame->buff = NewBuff(payloadLen);
    if (frame->buff == nullptr) {
        DTAP_DestroyFrame(frame);
        return nullptr;
    }
    if (frame->ctx.buildFrame(&frame->ctx, TCID_SLE_CUTC, TEST_CRC_INIT) != DTAP_SUCCESS) {
        DTAP_DestroyFrame(frame);
        return nullptr;
    }
    frame->header = SDF_DataOffset(frame->buff);
    frame->headerLen = frame->ctx.getHeaderLen();
    return frame;
}

inline DTAP_Frame_S *MakeAckFrame(uint16_t reqSeq, bool sBit)
{
    DTAP_Frame_S *frame = DTAP_CreateFrame(DTAP_FRAME_ACK);
    if (frame == nullptr) {
        return nullptr;
    }
    frame->bits = DTAP_FRAME_C_BIT;
    frame->ack.reqSeq = reqSeq;
    frame->ack.sBit = sBit;
    if (frame->ctx.buildFrame(&frame->ctx, TCID_SLE_CUTC, TEST_CRC_INIT) != DTAP_SUCCESS) {
        DTAP_DestroyFrame(frame);
        return nullptr;
    }
    return frame;
}

inline void InitReliableChannel(DTAP_Channel_S &chan, DTAP_ReliableChannel_S &reliable,
    uint8_t window, uint8_t maxTxThreshold)
{
    (void)memset_s(&chan, sizeof(chan), 0, sizeof(chan));
    (void)memset_s(&reliable, sizeof(reliable), 0, sizeof(reliable));
    SDF_DListHeadInit(&chan.pktList);
    SDF_DListEntryInit(&chan.schedEntry);
    SDF_DListEntryInit(&chan.entry);
    chan.priority = DTAP_PRIORITY_CMD;
    chan.lcid = 1;
    chan.srcTcid = TCID_SLE_CUTC;
    chan.dstTcid = TCID_SLE_CUTC;
    chan.mode = CM_TRANS_MODE_RELIABLE;
    chan.mtu = UINT16_MAX;
    chan.mps = 100;
    chan.attr = &reliable;
    reliable.crcInit = TEST_CRC_INIT;
    reliable.maxTxThreshold = maxTxThreshold;
    reliable.retransTimeout = 1000;
    reliable.rspTimeout = 1000;
    reliable.reorderTimeout = 1000;
    reliable.reorderTimer = INVALID_TIMER_HANDLE;
    reliable.retransTimer = INVALID_TIMER_HANDLE;
    reliable.rspTimer = INVALID_TIMER_HANDLE;
    reliable.txWindow.size = window;
    reliable.rxWindow.size = window;
    SDF_DListHeadInit(&reliable.txWindow.txList);
    SDF_DListHeadInit(&reliable.rxWindow.rxList);
}

inline void InitStreamChannel(DTAP_Channel_S &chan, DTAP_Stream_Channel_S &stream)
{
    (void)memset_s(&chan, sizeof(chan), 0, sizeof(chan));
    (void)memset_s(&stream, sizeof(stream), 0, sizeof(stream));
    SDF_DListHeadInit(&chan.pktList);
    SDF_DListEntryInit(&chan.schedEntry);
    SDF_DListEntryInit(&chan.entry);
    chan.priority = DTAP_PRIORITY_CMD;
    chan.lcid = 1;
    chan.srcTcid = TCID_SLE_CUTC;
    chan.dstTcid = TCID_SLE_CUTC;
    chan.mode = CM_TRANS_MODE_STREAM;
    chan.mtu = UINT16_MAX;
    chan.mps = UINT16_MAX;
    chan.attr = &stream;
    stream.crcInit = TEST_CRC_INIT;
    stream.reorderTimerHandle = INVALID_TIMER_HANDLE;
    stream.expectedSeq = DTAP_FRAME_SEQ_INIT;
    stream.maxExpectedSeq = DTAP_FRAME_SEQ_INIT;
    SDF_DListHeadInit(&stream.txList);
    SDF_DListHeadInit(&stream.rxList);
}

inline void TxInsert(DTAP_ReliableChannel_S &reliable, DTAP_Frame_S *frame)
{
    SDF_DListElmTailInsert(&reliable.txWindow.txList, frame, entry);
}

inline void TxListClear(DTAP_ReliableChannel_S &reliable)
{
    DTAP_Frame_S *node = nullptr;
    DTAP_Frame_S *tmp = nullptr;
    SDF_DListElmAllFree(node, tmp, &reliable.txWindow.txList, entry, DTAP_DestroyFrame);
}

inline CM_TransChan_S *MakeChanParam(uint16_t lcid, uint8_t srcTcid, uint8_t dstTcid, uint8_t mode)
{
    CM_TransChan_S *chan = (CM_TransChan_S *)SDF_MemZalloc(sizeof(CM_TransChan_S));
    if (chan == nullptr) {
        return nullptr;
    }
    chan->castMode = CM_ACCESS_TRANS_MODE_UNICAST;
    chan->lcid = lcid;
    chan->srcTcid = srcTcid;
    chan->dstTcid = dstTcid;
    chan->config.transMode = mode;
    chan->config.mtu = UINT16_MAX;
    chan->config.mps = UINT16_MAX;
    return chan;
}

inline void ChannelInject(CM_TransChannelStateList_S &state, uint16_t lcid, uint8_t srcTcid,
    uint8_t dstTcid, uint8_t mode)
{
    SDF_Traits traits = {.dtor = SDF_MemFree};
    state.channelVector = SDF_CreateVector(traits);
    CM_TransChan_S *chan = MakeChanParam(lcid, srcTcid, dstTcid, mode);
    if (chan != nullptr) {
        (void)SDF_VectorEmplaceBack(state.channelVector, chan);
    }
    state.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    TEST_CM_TransChannelDo(&state);
}

inline void ChannelRelease(CM_TransChannelStateList_S &state)
{
    state.result = CM_TRANS_CHANNEL_STATE_RELEASED;
    TEST_CM_TransChannelDo(&state);
    SDF_DestroyVector(state.channelVector);
}
} // namespace DtapTransTest

#define NEW_FRAME(var, ...)     auto *var = DtapTransTest::MakeDataFrame(__VA_ARGS__);     ASSERT_NE(var, nullptr)
#define NEW_BUFF(var, n)     auto *var = DtapTransTest::NewBuff(n);     ASSERT_NE(var, nullptr)

class UT_DTAP_TRANS : public testing::Test {
public:
    testing::NiceMock<OHOS::ScheduleMock> scheduleMock;
    testing::NiceMock<OHOS::DliEventMock> dliEventMock;
    testing::NiceMock<OHOS::DliLayerMock> dliLayerMock;
    testing::NiceMock<OHOS::CmMock> cmMock;
    testing::NiceMock<OHOS::SdfMemMock> sdfMemMock;

protected:
    void SetUp() override
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(TEST_ScheduleTimerAddStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(TEST_ScheduleTimerDelStub);

        TEST_DliEventInit();
        EXPECT_CALL(dliEventMock, DLI_RegNOCPEventCbk).WillRepeatedly(TEST_DLI_RegNOCPEventCbk);
        EXPECT_CALL(dliEventMock, DLI_UnregNOCPEventCbk).WillRepeatedly(TEST_DLI_UnregNOCPEventCbk);

        EXPECT_CALL(dliLayerMock, DLI_GetDataFragmentNums).WillRepeatedly(TEST_DLI_GetDataFragmentNumsStub);
        EXPECT_CALL(dliLayerMock, DLI_GetFragmentMaxLen).WillRepeatedly(TEST_DLI_GetFragmentMaxLenStub);
        EXPECT_CALL(dliLayerMock, DLI_SplitData).WillRepeatedly(TEST_DLI_SplitDataStub);
        EXPECT_CALL(dliLayerMock, DLI_DataSend).WillRepeatedly(TEST_DTAP_DataSendCapture);
        ON_CALL(sdfMemMock, SDF_MemZalloc).WillByDefault(TEST_SDF_MemZalloc);

        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegTransChannelListener).WillRepeatedly(TEST_CM_RegTransChannelListener);
        EXPECT_CALL(cmMock, CM_UnRegTransChannelListener).WillRepeatedly(TEST_CM_UnRegTransChannelListener);
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_GetLogicLinkConnectedSize).WillRepeatedly(TEST_CM_GetLogicLinkConnectedSize);

        TEST_DTAP_CollabReset();
        TEST_DTAP_CollabCntReset();
        TEST_DTAP_StatusCbkReset();
        TEST_DTAP_SentFramesReset();
        TEST_DTAP_RecvCbReset(0);
        TEST_DTAP_SetMonoTimeMs(0);
        DLI_AllDataSet(600, 8, 0, 0);
        EXPECT_EQ(DTAP_Init(), DTAP_SUCCESS);
        CM_TransChannelMgrInit();
        TEST_DTAP_SetApBufferNum(1);
    }

    void TearDown() override
    {
        CP_TimerSetExecCallbackAtOnce(false);
        CP_TimerSetExecCallbackPostponed(false);
        CP_TimerSetAddFailed(false);
        DTAP_DeInit();
        CM_TransChannelMgrDeInit();
        DTAP_UnRegisterDataSendCbks();
        TEST_CM_DeInit();
        TEST_DliEventDeInit();
        TEST_StackScheduleDeInit();
    }
};

#endif
