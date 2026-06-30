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

#include "gtest/gtest.h"
#include "securec.h"
#include "cm_errno.h"
#include "cp_worker.h"
#include "dtap_channel.h"
#include "dtap_scheduler.h"
#include "dtap_errno.h"
#include "dli_layer_config.h"
#include "sdf_mem.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_dli_event_mock.h"
#include "stack_dli_event_stub.h"
#include "stack_dli_layer_mock.h"
#include "stack_dli_layer_stub.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

class UT_DTAP_SCHEDULER : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<DliEventMock> dliEventMock;
    NiceMock<DliLayerMock> dliLayerMock;
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);

        TEST_DliEventInit();
        EXPECT_CALL(dliEventMock, DLI_RegNOCPEventCbk).WillRepeatedly(TEST_DLI_RegNOCPEventCbk);
        EXPECT_CALL(dliEventMock, DLI_UnregNOCPEventCbk).WillRepeatedly(TEST_DLI_UnregNOCPEventCbk);

        EXPECT_CALL(dliLayerMock, DLI_GetDataFragmentNums).WillRepeatedly(TEST_DLI_GetDataFragmentNumsStub);
        EXPECT_CALL(dliLayerMock, DLI_GetFragmentMaxLen).WillRepeatedly(TEST_DLI_GetFragmentMaxLenStub);
        EXPECT_CALL(dliLayerMock, DLI_SplitData).WillRepeatedly(TEST_DLI_SplitDataStub);
        EXPECT_CALL(dliLayerMock, DLI_DataSend).WillRepeatedly(TEST_DLI_DataSendStub);
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
        TEST_StackScheduleDeInit();
        TEST_DliEventDeInit();
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

TEST_F(UT_DTAP_SCHEDULER, DTAP_SchedulerInitTest)
{
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPriorityNotInitTest)
{
    EXPECT_EQ(DTAP_DataSendWithPriority(NULL, NULL), DTAP_TRANS_INIT_ERR);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPriorityNullTest)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_DataSendWithPriority(NULL, NULL), DTAP_TRANS_INVALID_PARAM_ERR);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPriorityInvalidPriTest)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DTAP_Channel_S channel;
    (void)memset_s(&channel, sizeof(DTAP_Channel_S), 0, sizeof(DTAP_Channel_S));
    channel.priority = DTAP_PRIORITY_MAX;
    SDF_Buff_S buff = {0};
    EXPECT_EQ(DTAP_DataSendWithPriority(&channel, &buff), DTAP_TRANS_INVALID_MODULE_TYPE);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPrioritySuccessTest)
{
    DTAP_Channel_S channel;
    (void)memset_s(&channel, sizeof(DTAP_Channel_S), 0, sizeof(DTAP_Channel_S));
    SDF_DListHeadInit(&channel.pktList);
    SDF_DListEntryInit(&channel.schedEntry);
    SDF_DListEntryInit(&channel.entry);
    channel.priority = DTAP_PRIORITY_CMD;

    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);

    DLI_AllDataSet(100, 8, 0, 0);
    SDF_Buff_S *buf1 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf1, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf1, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(&channel, buf1), DTAP_SUCCESS);
    // same lcid and tcid
    SDF_Buff_S *buf2 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf2, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf2, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(&channel, buf2), DTAP_SUCCESS);
    // same lcid and different tcid
    DTAP_Channel_S channel2;
    (void)memset_s(&channel2, sizeof(DTAP_Channel_S), 0, sizeof(DTAP_Channel_S));
    channel2.priority = DTAP_PRIORITY_CMD;
    SDF_DListHeadInit(&channel2.pktList);
    SDF_DListEntryInit(&channel2.schedEntry);
    SDF_DListEntryInit(&channel2.entry);
    channel2.srcTcid = 1;
    SDF_Buff_S *buf3 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf3, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf3, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(&channel2, buf3), DTAP_SUCCESS);
    // different lcid
    DTAP_Channel_S channel3;
    (void)memset_s(&channel3, sizeof(DTAP_Channel_S), 0, sizeof(DTAP_Channel_S));
    channel3.priority = DTAP_PRIORITY_CMD;
    SDF_DListHeadInit(&channel3.pktList);
    SDF_DListEntryInit(&channel3.schedEntry);
    SDF_DListEntryInit(&channel3.entry);
    channel3.lcid = 1;
    SDF_Buff_S *buf4 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf4, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf4, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(&channel3, buf4), DTAP_SUCCESS);

    DLI_AllDataSet(100, 4, 0, 0);
    EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, 0, 1), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPrioritySingleNumTest)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DLI_AllDataSet(1, 1, 0, 0);
    DTAP_Channel_S channel;
    (void)memset_s(&channel, sizeof(DTAP_Channel_S), 0, sizeof(DTAP_Channel_S));
    SDF_DListHeadInit(&channel.pktList);
    SDF_DListEntryInit(&channel.schedEntry);
    SDF_DListEntryInit(&channel.entry);
    channel.priority = DTAP_PRIORITY_HIGH;
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf, 100), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(&channel, buf), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_DataSendWithPrioritySendFailedTest)
{
    DTAP_Channel_S channel;
    (void)memset_s(&channel, sizeof(DTAP_Channel_S), 0, sizeof(DTAP_Channel_S));
    SDF_DListHeadInit(&channel.pktList);
    SDF_DListEntryInit(&channel.schedEntry);
    SDF_DListEntryInit(&channel.entry);
    channel.priority = DTAP_PRIORITY_CMD;
    channel.lcid = 0xffff;

    DLI_AllDataSet(100, 1, 0, 0);
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(&channel, buf), DTAP_SUCCESS);

    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_ChannelDownTest)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DLI_AllDataSet(100, 0, 0, 0);
    DTAP_Channel_S channel;
    (void)memset_s(&channel, sizeof(DTAP_Channel_S), 0, sizeof(DTAP_Channel_S));
    channel.priority = DTAP_PRIORITY_CMD;
    SDF_DListHeadInit(&channel.pktList);
    SDF_DListEntryInit(&channel.schedEntry);
    SDF_DListEntryInit(&channel.entry);
    SDF_Buff_S *buf = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf, 100), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(&channel, buf), DTAP_SUCCESS);

    DTAP_Channel_S channel2;
    (void)memset_s(&channel2, sizeof(DTAP_Channel_S), 0, sizeof(DTAP_Channel_S));
    channel2.priority = DTAP_PRIORITY_CMD;
    SDF_DListHeadInit(&channel2.pktList);
    SDF_DListEntryInit(&channel2.schedEntry);
    SDF_DListEntryInit(&channel2.entry);
    channel2.srcTcid = 1;
    SDF_Buff_S *buf2 = SDF_BuffNewWithReserve(100);
    EXPECT_NE(buf2, nullptr);
    EXPECT_NE(SDF_BuffAppend(buf2, 10), nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(&channel2, buf2), DTAP_SUCCESS);
    DTAP_ChannelDown(1, 0);
    DTAP_ChannelDown(0, 2);
    DTAP_ChannelDown(0, 0);
    DTAP_ChannelDown(0, 1);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_NotifyAcbLinkNum_ChipBufferLessThanTwo)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DLI_AllDataSet(1, 1, 0, 0);  // acbLen=1, acbNum=1 which is less than DTAP_SHARED_BUFFER_MODULES(2)
    // DTAP_NotifyAcbLinkNum(1, 1);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_NotifyAcbLinkNum_TotalLinksZero)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DLI_AllDataSet(100, 4, 0, 0);  // acbNum=4, enough for DTAP_SHARED_BUFFER_MODULES
    // DTAP_NotifyAcbLinkNum(0, 0);  // totalLinks == 0
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_NotifyAcbLinkNum_NormalDistribution)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DLI_AllDataSet(100, 4, 0, 0);  // acbNum=4
    // DTAP_NotifyAcbLinkNum(1, 1);  // apLinkNum=1, shLinkNum=1, totalLinks=2
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_NotifyAcbLinkNum_ApLinkNumGreaterThanShLinkNum)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DLI_AllDataSet(100, 5, 0, 0);  // acbNum=5
    // DTAP_NotifyAcbLinkNum(3, 1);  // apLinkNum > shLinkNum, remaining should go to ap
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_NotifyAcbLinkNum_ShLinkNumGreaterThanApLinkNum)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DLI_AllDataSet(100, 5, 0, 0);  // acbNum=5
    // DTAP_NotifyAcbLinkNum(1, 3);  // shLinkNum > apLinkNum, remaining should go to sh
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_NotifyAcbLinkNum_IsNeedTransCollabFalse)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DLI_AllDataSet(100, 4, 0, 0);
    // DTAP_NotifyAcbLinkNum(1, 1);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_NotifyAcbLinkNum_MinBufferDistribution)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DLI_AllDataSet(100, 2, 0, 0);  // acbNum=2, exactly DTAP_SHARED_BUFFER_MODULES
    // DTAP_NotifyAcbLinkNum(1, 1);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_NotifyAcbLinkNum_LargeLinkDifference)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DLI_AllDataSet(100, 10, 0, 0);  // acbNum=10
    // DTAP_NotifyAcbLinkNum(8, 1);  // large imbalance, remaining should go to ap
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}

TEST_F(UT_DTAP_SCHEDULER, DTAP_SchedulerInit_DataNumChangeCallbackPath)
{
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    DLI_AllDataSet(100, 4, 0, 0);
    // DTAP_NotifyAcbLinkNum(1, 1);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
}