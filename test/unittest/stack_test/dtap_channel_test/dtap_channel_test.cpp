/*
 * Copyright (C) 2025-2025 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "securec.h"
#include "cm_trans_channel_api.h"
#include "cm_errno.h"
#include "cp_worker.h"
#include "dtap_channel.h"
#include "dtap_errno.h"
#include "sdf_mem.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

class UT_DTAP_CHANNEL : public testing::Test {
protected:
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);

        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegTransChannelListener).WillRepeatedly(TEST_CM_RegTransChannelListener);
        EXPECT_CALL(cmMock, CM_UnRegTransChannelListener).WillRepeatedly(TEST_CM_UnRegTransChannelListener);
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_GetLogicLinkConnectedSize).WillRepeatedly(TEST_CM_GetLogicLinkConnectedSize);
        (void)DTAP_ChannelInit();
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
        DTAP_ChannelDeInit();
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
    }
};

static CM_TransChan_S *CM_TransChanCreate(uint8_t castMode, uint16_t lcid, uint8_t srcTcid,
    uint8_t dstTcid, uint8_t mode)
{
    CM_TransChan_S *channel = (CM_TransChan_S *)SDF_MemZalloc(sizeof(CM_TransChan_S));
    if (channel == NULL) {
        return NULL;
    }
    channel->castMode = castMode;
    channel->lcid = lcid;
    channel->srcTcid = srcTcid;
    channel->dstTcid = dstTcid;
    channel->config.transMode = mode;
    channel->config.mtu = UINT16_MAX;
    channel->config.mps = UINT16_MAX;

    return channel;
}

TEST_F(UT_DTAP_CHANNEL, DTAP_ChannelsStreamModeTest)
{
    CM_TransChannelStateList_S stateLists;
    SDF_Traits transChannelTraits = {.dtor = SDF_MemFree};
    stateLists.channelVector = SDF_CreateVector(transChannelTraits);
    EXPECT_NE(stateLists.channelVector, nullptr);

    CM_TransChan_S *chan = CM_TransChanCreate(0, 0, 0, 0, CM_TRANS_MODE_STREAM);
    EXPECT_NE(chan, nullptr);
    bool isSuccess = SDF_VectorEmplaceBack(stateLists.channelVector, chan);
    EXPECT_EQ(isSuccess, true);

    stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    TEST_CM_TransChannelDo(&stateLists);
    stateLists.result = CM_TRANS_CHANNEL_STATE_RELEASED;
    TEST_CM_TransChannelDo(&stateLists);

    chan->config.streamMode.flushTimeout = DTAP_STREAM_FLUSH_TIMEOUT;
    stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    TEST_CM_TransChannelDo(&stateLists);
    stateLists.result = CM_TRANS_CHANNEL_STATE_RELEASED;
    TEST_CM_TransChannelDo(&stateLists);

    chan->config.streamMode.reorderTimeout = DTAP_STREAM_REORDER_TIMEOUT;
    stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    TEST_CM_TransChannelDo(&stateLists);
    stateLists.result = CM_TRANS_CHANNEL_STATE_RELEASED;
    TEST_CM_TransChannelDo(&stateLists);

    SDF_DestroyVector(stateLists.channelVector);
}

TEST_F(UT_DTAP_CHANNEL, DTAP_ChannelsReliableModeTest)
{
    CM_TransChannelStateList_S stateLists;
    stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    SDF_Traits transChannelTraits = {.dtor = SDF_MemFree};
    stateLists.channelVector = SDF_CreateVector(transChannelTraits);
    EXPECT_NE(stateLists.channelVector, nullptr);

    CM_TransChan_S *chan = CM_TransChanCreate(0, 0, 0, 0, CM_TRANS_MODE_RELIABLE);
    EXPECT_NE(chan, nullptr);
    bool isSuccess = SDF_VectorEmplaceBack(stateLists.channelVector, chan);
    EXPECT_EQ(isSuccess, true);
    TEST_CM_TransChannelDo(&stateLists);
    stateLists.result = CM_TRANS_CHANNEL_STATE_RELEASED;
    TEST_CM_TransChannelDo(&stateLists);

    stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    TEST_CM_TransChannelDo(&stateLists);
    DTAP_Channel_S *channel = DTAP_ChannelSearch(0, 0);
    EXPECT_NE(channel, nullptr);
    EXPECT_NE(channel->attr, nullptr);
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)channel->attr;
    reliable->frags = SDF_BuffNew(1);
    EXPECT_NE(reliable->frags, nullptr);
    reliable->cacheBuff = SDF_BuffNew(1);
    EXPECT_NE(reliable->cacheBuff, nullptr);
    stateLists.result = CM_TRANS_CHANNEL_STATE_RELEASED;
    TEST_CM_TransChannelDo(&stateLists);
    SDF_DestroyVector(stateLists.channelVector);
}

TEST_F(UT_DTAP_CHANNEL, DTAP_ChannelsBasicModeTest)
{
    CM_TransChannelStateList_S stateLists;
    stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    SDF_Traits transChannelTraits = {.dtor = SDF_MemFree};
    stateLists.channelVector = SDF_CreateVector(transChannelTraits);
    EXPECT_NE(stateLists.channelVector, nullptr);

    CM_TransChan_S *chan = CM_TransChanCreate(0, 0, 0, 0, CM_TRANS_MODE_BASIC);
    EXPECT_NE(chan, nullptr);
    bool isSuccess = SDF_VectorEmplaceBack(stateLists.channelVector, chan);
    EXPECT_EQ(isSuccess, true);
    TEST_CM_TransChannelDo(&stateLists);
    stateLists.result = CM_TRANS_CHANNEL_STATE_RELEASED;
    TEST_CM_TransChannelDo(&stateLists);
    SDF_DestroyVector(stateLists.channelVector);
}

TEST_F(UT_DTAP_CHANNEL, DTAP_ChannelsDuplicate)
{
    CM_TransChannelStateList_S stateLists;
    stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    SDF_Traits transChannelTraits = {.dtor = SDF_MemFree};
    stateLists.channelVector = SDF_CreateVector(transChannelTraits);
    EXPECT_NE(stateLists.channelVector, nullptr);

    CM_TransChan_S *chan = CM_TransChanCreate(0, 0, 0, 0, 0);
    EXPECT_NE(chan, nullptr);
    bool isSuccess = SDF_VectorEmplaceBack(stateLists.channelVector, chan);
    EXPECT_EQ(isSuccess, true);
    TEST_CM_TransChannelDo(&stateLists);
    TEST_CM_TransChannelDo(&stateLists);
    stateLists.result = CM_TRANS_CHANNEL_STATE_RELEASED;
    TEST_CM_TransChannelDo(&stateLists);
    SDF_DestroyVector(stateLists.channelVector);
}

TEST_F(UT_DTAP_CHANNEL, DTAP_ChannelSearchTest)
{
    CM_TransChannelStateList_S stateLists;
    stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    SDF_Traits transChannelTraits = {.dtor = SDF_MemFree};
    stateLists.channelVector = SDF_CreateVector(transChannelTraits);
    EXPECT_NE(stateLists.channelVector, nullptr);

    CM_TransChan_S *chan = CM_TransChanCreate(0, 0, 0, 0, 0);
    EXPECT_NE(chan, nullptr);
    bool isSuccess = SDF_VectorEmplaceBack(stateLists.channelVector, chan);
    EXPECT_EQ(isSuccess, true);

    TEST_CM_TransChannelDo(&stateLists);
    DTAP_Channel_S *channel = DTAP_ChannelSearch(0, 0);
    EXPECT_NE(channel, nullptr);
    DTAP_ChannelSetStatus(0, 0, 0);

    channel = DTAP_ChannelSearch(0, 1);
    EXPECT_EQ(channel, nullptr);

    stateLists.result = CM_TRANS_CHANNEL_STATE_RELEASED;
    TEST_CM_TransChannelDo(&stateLists);
    channel = DTAP_ChannelSearch(0, 0);
    EXPECT_EQ(channel, nullptr);
    SDF_DestroyVector(stateLists.channelVector);
}

TEST_F(UT_DTAP_CHANNEL, DTAP_ChannelStateChangeCbkTest)
{
    CM_TransChannelStateList_S stateLists;
    stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    SDF_Traits transChannelTraits = {.dtor = SDF_MemFree};
    uint8_t castModes[] = {CM_ACCESS_TRANS_MODE_UNICAST, CM_ACCESS_TRANS_MODE_DATA_MCST,
        CM_ACCESS_TRANS_MODE_SEND_BCST, CM_ACCESS_TRANS_MODE_MAX};
    uint8_t idx = 0;
    uint8_t srcTcids[] = {CM_TCID_SLE_CMTC, CM_TCID_SLE_CUTC, CM_TCID_BC_BEGIN, CM_TCID_MAX};
    for (uint8_t transMode = CM_TRANS_MODE_BASIC; transMode <= CM_TRANS_MODE_MAX; transMode++) {
        stateLists.channelVector = SDF_CreateVector(transChannelTraits);
        EXPECT_NE(stateLists.channelVector, nullptr);
        CM_TransChan_S *chan = CM_TransChanCreate(castModes[idx % sizeof(castModes)],
            0, srcTcids[idx % sizeof(srcTcids)], 0, transMode);
        idx++;
        EXPECT_NE(chan, nullptr);
        bool isSuccess = SDF_VectorEmplaceBack(stateLists.channelVector, chan);
        EXPECT_EQ(isSuccess, true);
        stateLists.result = CM_TRANS_CHANNEL_STATE_RELEASED;
        TEST_CM_TransChannelDo(&stateLists);

        stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
        TEST_CM_TransChannelDo(&stateLists);

        CM_LogicLinkState_S linkState;
        (void)memset_s(&linkState, sizeof(CM_LogicLinkState_S), 0, sizeof(CM_LogicLinkState_S));
        linkState.result = CM_LINK_STATE_CONNECTED;
        TEST_CM_CreateLogicLink(CM_MODULE_DTAP, &linkState);

        stateLists.result = CM_TRANS_CHANNEL_STATE_RELEASED;
        TEST_CM_TransChannelDo(&stateLists);
        stateLists.result = CM_TRANS_CHANNEL_STATE_INIT;
        TEST_CM_TransChannelDo(&stateLists);
        SDF_DestroyVector(stateLists.channelVector);
    }
}

TEST_F(UT_DTAP_CHANNEL, DTAP_LogicLinkStateChangeCbkTest)
{
    TEST_CM_TransChannelDo(NULL);

    CM_LogicLinkState_S linkState;
    (void)memset_s(&linkState, sizeof(CM_LogicLinkState_S), 0, sizeof(CM_LogicLinkState_S));
    linkState.result = CM_LINK_STATE_CONNECTED;
    TEST_CM_CreateLogicLink(CM_MODULE_DTAP, &linkState);
    linkState.result = CM_LINK_STATE_DISCONNECTED;
    TEST_CM_CreateLogicLink(CM_MODULE_DTAP, &linkState);
}

TEST_F(UT_DTAP_CHANNEL, DTAP_ChannelResidualTest)
{
    CM_TransChannelStateList_S stateLists;
    stateLists.result = CM_TRANS_CHANNEL_STATE_ACTIVATED;
    SDF_Traits transChannelTraits = {.dtor = SDF_MemFree};
    stateLists.channelVector = SDF_CreateVector(transChannelTraits);
    EXPECT_NE(stateLists.channelVector, nullptr);

    CM_TransChan_S *chan = CM_TransChanCreate(0, 0, 0, 0, CM_TRANS_MODE_BASIC);
    EXPECT_NE(chan, nullptr);
    bool isSuccess = SDF_VectorEmplaceBack(stateLists.channelVector, chan);
    EXPECT_EQ(isSuccess, true);
    TEST_CM_TransChannelDo(&stateLists);
    SDF_DestroyVector(stateLists.channelVector);
}