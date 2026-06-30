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
#include "cp_worker.h"
#include "securec.h"
#include "dli_errno.h"
#include "dli_thread.h"
#include "dli.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_dli_stub.h"

static void Test_DLI_ExecuteCmdCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    return;
}

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

class UT_DLI_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        TEST_DLI_Init();
        DLI_Init();
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
        DLI_DeInit();
        TEST_StackScheduleDeInit();
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {}

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {}
};

TEST_F(UT_DLI_TEST, DLI_Init)
{
    EXPECT_EQ(DLI_Init(), DLI_STACK_INITED_ERRNO);
}

TEST_F(UT_DLI_TEST, DLI_CmdCbkReg)
{
    DLI_CbkLineStru table = {};
    // table.opcode = DLI_CBK_CHANINFO; // TDD修改DLI_CBK_CHANINFO找不到定义的位置
    table.func = Test_DLI_ExecuteCmdCbk;
    EXPECT_NE(DLI_CmdCbkReg(DEVD, nullptr, 0, &table, 0x00), DLI_SUCCESS);
    EXPECT_EQ(DLI_CmdCbkReg(DEVD, nullptr, 0, &table, 0x01), DLI_SUCCESS);
    DLI_CmdCbkUnReg(DEVD, nullptr, 0, &table, 0x00);
    DLI_CmdCbkUnReg(DEVD, nullptr, 0, &table, 0x01);
}

TEST_F(UT_DLI_TEST, DLI_CmdCbkReg_Twice)
{
    DLI_CbkLineStru table = {};
    // table.opcode = DLI_CBK_CHANINFO; // TDD修改DLI_CBK_CHANINFO找不到定义的位置
    table.func = Test_DLI_ExecuteCmdCbk;
    EXPECT_EQ(DLI_CmdCbkReg(SM, nullptr, 0, &table, 0x01), DLI_SUCCESS);
    EXPECT_EQ(DLI_CmdCbkReg(HADM, nullptr, 0, &table, 0x01), DLI_SUCCESS);
    DLI_CmdCbkUnReg(SM, nullptr, 0, &table, 0x01);
    DLI_CmdCbkUnReg(HADM, nullptr, 0, &table, 0x01);
}

TEST_F(UT_DLI_TEST, DLI_CmdCbkReg_Third)
{
#define SL EXT_START
    DLI_CbkLineStru table = {};
    // table.opcode = DLI_CBK_CHANINFO; // TDD修改DLI_CBK_CHANINFO找不到定义的位置
    table.func = Test_DLI_ExecuteCmdCbk;
    EXPECT_EQ(DLI_CmdCbkReg(NBC, nullptr, 0, &table, 0x01), DLI_SUCCESS);
    EXPECT_EQ(DLI_CmdCbkReg(SL, nullptr, 0, &table, 0x01), DLI_INVALID_PARAMETERS);
    DLI_CmdCbkUnReg(NBC, nullptr, 0, &table, 0x01);
    DLI_CmdCbkUnReg(SL, nullptr, 0, &table, 0x01);
}