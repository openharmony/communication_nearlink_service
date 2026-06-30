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

#include "gtest/gtest.h"
#include "securec.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_dli_cmd_mock.h"
#include "stack_dli_cmd_stub.h"
#include "dli_callback.h"
#include "cp_worker.h"

#include "nlstk_init_api.h"
#include "nlstk_cfgdb.h"
#include "dli_event_struct.h"
#include "nlstk_log.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static void TEST_DtapRecvAcbHandler(uint16_t lcid, SDF_Buff_S *buf)
{
}

static void TestDliInit(void)
{
    DLI_Callback dliCallback = {0};
    dliCallback.postOtherThread = CP_PostTask;
    dliCallback.postOtherBlockedThread = CP_PostTaskBlocked;
    dliCallback.dftReportKill = NULL;
    dliCallback.recvAcbHandler = TEST_DtapRecvAcbHandler;
    DLI_SetCallback(&dliCallback);
    return;
}

class UT_CFGDB_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<DliCmdMock> dliCmdMock;
protected:
    void SetUp() override
    {
        // 步骤1：初始化桩函数
        TestDliInit();
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
    }

    void TearDown() override
    {
        TEST_StackScheduleDeInit();
    }
};

// 调用DLI_ReadLocalVersion返回失败
TEST_F(UT_CFGDB_TEST, UT_CFGDB_001)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersionFail);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}

// 调用DLI_GetPublicAddress返回失败
TEST_F(UT_CFGDB_TEST, UT_CFGDB_002)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersion);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}

TEST_F(UT_CFGDB_TEST, UT_CFGDB_003)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersion);
    EXPECT_CALL(dliCmdMock, DLI_ReadMaximumAdvDataLen).WillRepeatedly(TEST_DLI_ReadMaximumAdvDataLenFail);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}

TEST_F(UT_CFGDB_TEST, UT_CFGDB_004)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersion);
    EXPECT_CALL(dliCmdMock, DLI_ReadMaximumAdvDataLen).WillRepeatedly(TEST_DLI_ReadMaximumAdvDataLen);
    EXPECT_CALL(dliCmdMock, DLI_ReadAdvSetsNum).WillRepeatedly(TEST_DLI_ReadAdvSetsNumFail);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}

TEST_F(UT_CFGDB_TEST, UT_CFGDB_005)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersion);
    EXPECT_CALL(dliCmdMock, DLI_ReadMaximumAdvDataLen).WillRepeatedly(TEST_DLI_ReadMaximumAdvDataLen);
    EXPECT_CALL(dliCmdMock, DLI_ReadAdvSetsNum).WillRepeatedly(TEST_DLI_ReadAdvSetsNum);
    EXPECT_CALL(dliCmdMock, DLI_ReadSupportCryptoAlgo).WillRepeatedly(TEST_DLI_ReadSupportCryptoAlgoFail);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}

TEST_F(UT_CFGDB_TEST, UT_CFGDB_006)
{
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalVersion).WillRepeatedly(TEST_DLI_ReadLocalVersion);
    EXPECT_CALL(dliCmdMock, DLI_ReadMaximumAdvDataLen).WillRepeatedly(TEST_DLI_ReadMaximumAdvDataLen);
    EXPECT_CALL(dliCmdMock, DLI_ReadAdvSetsNum).WillRepeatedly(TEST_DLI_ReadAdvSetsNum);
    EXPECT_CALL(dliCmdMock, DLI_ReadSupportCryptoAlgo).WillRepeatedly(TEST_DLI_ReadSupportCryptoAlgo);
    EXPECT_CALL(dliCmdMock, DLI_ReadLocalFeatures).WillRepeatedly(TEST_DLI_ReadLocalFeaturesFail);
    uint32_t ret = CfgdbInit();
    ASSERT_EQ(ret, 0);

    ret = CfgdbReadCommConfigValue();
    ASSERT_EQ(ret, 0);

    CfgdbDeinit();
}