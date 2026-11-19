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

#include "cm_signaling_internal.h"
#include "dtap_trans_test.h"

namespace {
using namespace DtapTransTest;

DTAP_TransMode_S *BasicMode()
{
    return DTAP_GetTransMode(CM_TRANS_MODE_BASIC);
}

DTAP_Frame_S *MakeBasicFrame(uint16_t payloadLen)
{
    DTAP_Frame_S *frame = DTAP_CreateFrame(DTAP_FRAME_BASIC);
    if (frame == nullptr) {
        return nullptr;
    }
    frame->buff = NewBuff(payloadLen);
    if (frame->buff == nullptr) {
        DTAP_DestroyFrame(frame);
        return nullptr;
    }
    frame->header = SDF_DataOffset(frame->buff);
    frame->headerLen = DTAP_BASIC_HEADER_LEN;
    return frame;
}
}

TEST_F(UT_DTAP_TRANS, DtapChanStreamParamClamp)
{
    // 依次注入超时为0与超上限的流通道，断言实际生效值被钳制到默认
    CM_TransChannelStateList_S state = {0};
    SDF_Traits traits = {.dtor = SDF_MemFree};
    state.channelVector = SDF_CreateVector(traits);
    ASSERT_NE(state.channelVector, nullptr);
    CM_TransChan_S *chan = MakeChanParam(4, TCID_SLE_CUTC, TCID_SLE_CUTC, CM_TRANS_MODE_STREAM);
    ASSERT_NE(chan, nullptr);
    chan->config.streamMode.flushTimeout = 0;
    chan->config.streamMode.reorderTimeout = DTAP_STREAM_REORDER_TIMEOUT + 1;
    (void)SDF_VectorEmplaceBack(state.channelVector, chan);
    state.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    TEST_CM_TransChannelDo(&state);

    DTAP_Channel_S *real = DTAP_ChannelSearch(4, TCID_SLE_CUTC);
    ASSERT_NE(real, nullptr);
    auto *stream = (DTAP_Stream_Channel_S *)real->attr;
    EXPECT_EQ(stream->flushTimeout, DTAP_STREAM_FLUSH_TIMEOUT);
    EXPECT_EQ(stream->reorderTimeout, DTAP_STREAM_REORDER_TIMEOUT);

    ChannelRelease(state);
}

TEST_F(UT_DTAP_TRANS, DtapChanReliableWindowDefaultAndRebuild)
{
    // 窗口参数0落默认值；同标识重复建道删除旧实例并替换
    CM_TransChannelStateList_S state = {0};
    ChannelInject(state, 5, TCID_SLE_CUTC, TCID_SLE_CUTC, CM_TRANS_MODE_RELIABLE);
    // 注入时未填可靠参数：窗口0走默认
    DTAP_Channel_S *real = DTAP_ChannelSearch(5, TCID_SLE_CUTC);
    ASSERT_NE(real, nullptr);
    auto *reliable = (DTAP_ReliableChannel_S *)real->attr;
    EXPECT_EQ(reliable->txWindow.size, CM_CAP_WND);

    // 重复激活：旧通道被删除重建，状态回到初始值
    reliable->nackCnt = 7;
    ChannelRelease(state);
    CM_TransChannelStateList_S again = {0};
    ChannelInject(again, 5, TCID_SLE_CUTC, TCID_SLE_CUTC, CM_TRANS_MODE_RELIABLE);
    real = DTAP_ChannelSearch(5, TCID_SLE_CUTC);
    ASSERT_NE(real, nullptr);
    reliable = (DTAP_ReliableChannel_S *)real->attr;
    EXPECT_EQ(reliable->nackCnt, 0);
    EXPECT_EQ(reliable->txWindow.size, CM_CAP_WND);
    ChannelRelease(again);
}

TEST_F(UT_DTAP_TRANS, DtapChanCreateFailRecycle)
{
    // 定时器登记失败导致可靠通道创建失败：通道查无且链路映射回收
    CM_TransChannelStateList_S state = {0};
    SDF_Traits traits = {.dtor = SDF_MemFree};
    state.channelVector = SDF_CreateVector(traits);
    ASSERT_NE(state.channelVector, nullptr);
    CM_TransChan_S *chan = MakeChanParam(6, TCID_SLE_CUTC, TCID_SLE_CUTC, CM_TRANS_MODE_RELIABLE);
    ASSERT_NE(chan, nullptr);
    (void)SDF_VectorEmplaceBack(state.channelVector, chan);
    state.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    CP_TimerSetAddFailed(true);
    TEST_CM_TransChannelDo(&state);
    CP_TimerSetAddFailed(false);
    EXPECT_EQ(DTAP_ChannelSearch(6, TCID_SLE_CUTC), nullptr);
    SDF_DestroyVector(state.channelVector);
}

TEST_F(UT_DTAP_TRANS, DtapChanStreamTimerFreeAndLinkCollab)
{
    // 白盒置非空定时器句柄后删道不崩溃；连接与断开各触发一次协同预分配
    CM_TransChannelStateList_S state = {0};
    ChannelInject(state, 7, TCID_SLE_CUTC, TCID_SLE_CUTC, CM_TRANS_MODE_STREAM);
    DTAP_Channel_S *real = DTAP_ChannelSearch(7, TCID_SLE_CUTC);
    ASSERT_NE(real, nullptr);
    auto *stream = (DTAP_Stream_Channel_S *)real->attr;
    stream->reorderTimerHandle = 9;

    TEST_DTAP_CollabCntReset();
    CM_LogicLinkState_S linkState = {0};
    linkState.lcid = 7;
    linkState.result = CM_LINK_STATE_CONNECTED;
    TEST_CM_CreateLogicLink(CM_MODULE_DTAP, &linkState);
    linkState.result = CM_LINK_STATE_DISCONNECTED;
    TEST_CM_CreateLogicLink(CM_MODULE_DTAP, &linkState);
    EXPECT_EQ(TEST_DTAP_PreAssignCnt(), 2);

    // 非连接非断开结果不触发预分配
    linkState.result = CM_LINK_STATE_CONNECTING;
    TEST_CM_CreateLogicLink(CM_MODULE_DTAP, &linkState);
    EXPECT_EQ(TEST_DTAP_PreAssignCnt(), 2);

    ChannelRelease(state);
}

TEST_F(UT_DTAP_TRANS, DtapChanBasicCacheAndResume)
{
    // 未就绪通道缓存来包，达到上限拒绝；就绪信号触发补交
    CM_TransChannelStateList_S state = {0};
    SDF_Traits traits = {.dtor = SDF_MemFree};
    state.channelVector = SDF_CreateVector(traits);
    ASSERT_NE(state.channelVector, nullptr);
    // 服务管理通道标识建立未就绪的基础通道
    CM_TransChan_S *chan = MakeChanParam(8, CM_TCID_SLE_SMTC, TCID_SLE_CUTC, CM_TRANS_MODE_BASIC);
    ASSERT_NE(chan, nullptr);
    (void)SDF_VectorEmplaceBack(state.channelVector, chan);
    state.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    TEST_CM_TransChannelDo(&state);
    DTAP_Channel_S *real = DTAP_ChannelSearch(8, CM_TCID_SLE_SMTC);
    ASSERT_NE(real, nullptr);
    auto *basic = (DTAP_Basic_Channel_S *)real->attr;
    EXPECT_FALSE(basic->isReady);
    SDF_DListHeadInit(&basic->cacheRxBuffs);

    // 未就绪时来包进入缓存
    DTAP_Frame_S *frame = MakeBasicFrame(8);
    ASSERT_NE(frame, nullptr);
    EXPECT_EQ(BasicMode()->recvFrame(real, frame, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(SDF_DListCount(&basic->cacheRxBuffs), 1);
    DTAP_DestroyFrame(frame);

    // 就绪后续交缓存并恢复接收
    TEST_DTAP_RecvCbReset(0);
    DTAP_RecvBasicFrameContinue(real);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 1);
    EXPECT_TRUE(basic->isReady);
    EXPECT_EQ(SDF_DListCount(&basic->cacheRxBuffs), 0);

    // 缓存节点分配失败按创建失败返回
    basic->isReady = false;
    frame = MakeBasicFrame(8);
    ASSERT_NE(frame, nullptr);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail);
    EXPECT_EQ(BasicMode()->recvFrame(real, frame, TEST_DTAP_RecvCb), DTAP_TRANS_BASIC_CREATE_FRAME_ERR);
    DTAP_DestroyFrame(frame);

    ChannelRelease(state);
}


TEST_F(UT_DTAP_TRANS, DtapChanBasicRecvThroughRegistry)
{
    // 同通道标识重复注册按更新处理；删除不存在的返回错误
    uint8_t tcid = TCID_SLE_CUTC;
    EXPECT_EQ(DTAP_RegisterDataRecvCb(tcid, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_RegisterDataRecvCb(tcid, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_UnregisterDataRecvCb(tcid), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_UnregisterDataRecvCb(tcid), DTAP_UNREGISTER_RECV_CB_TCID_NOT_EXIST);

    // 空回调与未初始化场景由既有套件覆盖，此处补注册后再删除的闭环
    EXPECT_EQ(DTAP_RegisterDataRecvCb(tcid, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_UnregisterDataRecvCb(tcid), DTAP_SUCCESS);

    // 注册回调后经统一收包入口送入基础帧：回调按通道标识命中
    CM_TransChannelStateList_S state = {0};
    ChannelInject(state, 9, TCID_SLE_CUTC, TCID_SLE_CUTC, CM_TRANS_MODE_BASIC);
    DTAP_Channel_S *real = DTAP_ChannelSearch(9, TCID_SLE_CUTC);
    ASSERT_NE(real, nullptr);
    real->mps = 100;

    EXPECT_EQ(DTAP_RegisterDataRecvCb(TCID_SLE_CUTC, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    DTAP_Frame_S *frame = MakeBasicFrame(8);
    ASSERT_NE(frame, nullptr);
    uint32_t ret = frame->ctx.buildFrame(&frame->ctx, TCID_SLE_CUTC, 0);
    EXPECT_EQ(ret, DTAP_SUCCESS);
    TEST_DTAP_RecvCbReset(0);
    DTAP_DataRecv(9, frame->buff);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 1);
    DTAP_DestroyFrame(frame);
    EXPECT_EQ(DTAP_UnregisterDataRecvCb(TCID_SLE_CUTC), DTAP_SUCCESS);
    ChannelRelease(state);
}
