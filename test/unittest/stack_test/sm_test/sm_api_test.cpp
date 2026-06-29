/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "sdf_mem.h"
#include "sdf_addr.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dli_mock.h"
#include "stack_dli_stub.h"

#include "nlstk_public_define_ext.h"
#include "nlstk_api_type_ext.h"
#include "cp_worker.h"
#include "securec.h"
#include "sm.h"
#include "nlstk_sm_api.h"
#include "nlstk_sm.h"
#include "nlstk_schedule.h"
#include "sm_stub_test.h"
#include "dli_event_struct.h"
#include "dli_errno.h"
#include "sm_dft.h"
#include "sm_struct.h"
#include "sm_stm.h"

#define STACK_MAX_THREAD 5
#define SM_PASSCODE_RAND_NUM_MOD 1000000

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

uint8_t g_psk[SM_PSK_SEC_KEY_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x10, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F};

class UT_SM_API_Test : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DliMock> dliMock;
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        TEST_ScheduleInit();
        ON_CALL(scheduleMock, SchedulePostTask).WillByDefault(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        
        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);

        TEST_DLI_Init();
        EXPECT_CALL(dliMock, DLI_CmdCbkReg).WillRepeatedly(TEST_DLI_CmdCbkReg);
        EXPECT_CALL(dliMock, DLI_CmdCbkUnReg).WillRepeatedly(TEST_DLI_CmdCbkUnReg);
        SmInit();
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
        SmDeInit();

        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
        TEST_DLI_DeInit();
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

/*****************************************************************************************
                                    Test Functions
*****************************************************************************************/
TEST_F(UT_SM_API_Test, TestNLSTKSmApi)
{
    NLSTK_ERRCODE errcode = NLSTK_ERRCODE_MAX;
    NLSTK_SmCallbacks_S cbks = {
        .startCbk = Test_SmPairStartCbk,
        .removeCbk = Test_SmPairRemoveCbk,
        .requestCbk = Test_SmPairRequestCbk,
        .authCbk = Test_SmAuthCmpCbk,
        .encCbk = Test_SmEncCmpCbk,
        .imgMsgCbk = Test_SmImgMsgCbk,
    };
    errcode = NLSTK_SmRegExternalCbks(&cbks);
    EXPECT_EQ(errcode, NLSTK_ERRCODE_SUCCESS);

    NLSTK_Errcode_E ret = NLSTK_ERRCODE_MAX;
    NLSTK_SmPassWord_S passWordIn = {};
    ret = NLSTK_SmSetPassWord(NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_POINTER_NULL);
    ret = NLSTK_SmSetPassWord(&passWordIn);

    NLSTK_SmPsk_S pskIn = {};
    (void)memcpy_s(&pskIn.addr, sizeof(SLE_Addr_S), &g_rmtAddr, sizeof(SLE_Addr_S));
    (void)memcpy_s(pskIn.psk, SM_PSK_SEC_KEY_LEN, g_psk, SM_PSK_SEC_KEY_LEN);
    ret = NLSTK_SmSetLocalPsk(NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_POINTER_NULL);
    ret = NLSTK_SmSetLocalPsk(&pskIn);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    SmSLink_S *slink = SmFindSLink(&g_rmtAddr);    // 新增的slink会在用例teardown时使用SmDeInit释放
    ASSERT_NE(slink, NULL);
    EXPECT_EQ(memcmp(g_psk, slink->psk, SM_PSK_SEC_KEY_LEN), 0);

    NLSTK_SmImgSecuConfig_S config = {};
    ret = NLSTK_SmSendImgSecuConfig(&config);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    EXPECT_FALSE(SmIsSLinkAuthComplete(0xFF));
    EXPECT_FALSE(SmIsSLinkEncryptComplete(0xFF));
}

TEST_F(UT_SM_API_Test, TESTSmPostTaskFail)
{
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);
    SLE_Addr_S sleAddr = {.type = PUBLIC_ADDRESS, .addr = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };
    NLSTK_Errcode_E ret = NLSTK_SmStartPairing(&sleAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);

    ret = NLSTK_SmRemovePairing(&sleAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);

    ret = NLSTK_SmSetConfirm(&sleAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);

    NLSTK_SmPassCode_S passCodeIn = {{0}};
    ret = NLSTK_SmSetPassCode(&passCodeIn);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);

    NLSTK_SmPsk_S pskIn = {{0}};
    ret = NLSTK_SmSetLocalPsk(&pskIn);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);

    NLSTK_SmRecoverKeyParam_S recoverKey ={{0}};
    uint8_t keyNum = 1;
    ret = NLSTK_SmRecoverKey(&recoverKey, keyNum);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);

    NLSTK_SmLocalParams_S paramsIn ={0};
    ret = NLSTK_SmSetSecurityParams(&paramsIn);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
}

TEST_F(UT_SM_API_Test, TestSmRedeinit)
{
    SmDeInit();
    SmDeInit();
}

TEST_F(UT_SM_API_Test, TestSmSendCbk)
{
    DLI_ExecuteCmdRetParam cmdRes = {0};
    DLI_ControllerDataEvt eventParameter = {0};
    cmdRes.eventParameter = &eventParameter;
    uint16_t status = DLI_SUCCESS;
    TEST_DLI_EventCbk(DLI_CBK_SEND_CONTROLLER_DATA, status, &cmdRes);

    status = DLI_UNKNOWN_COMMAND;
    TEST_DLI_EventCbk(DLI_CBK_SEND_CONTROLLER_DATA, status, &cmdRes);
}

TEST_F(UT_SM_API_Test, TestSmChangeCbk)
{
    DLI_ExecuteCmdRetParam cmdRes = {0};
    DLI_EncryptChangeEvt eventParameter = {0};
    cmdRes.eventParameter = &eventParameter;
    uint16_t status = DLI_SUCCESS;
    TEST_DLI_EventCbk(DLI_CBK_ENCRYPT_CHANGE, status, &cmdRes);
}

TEST_F(UT_SM_API_Test, TestSmDft)
{
    SLE_Addr_S addr = {0};
    SmState_E curState = SM_STATE_NEGO;
    SmNodeType_E nodeType = SM_G_NODE;
    uint16_t errVal = 0;
    SmDftReport(&addr, curState, nodeType, errVal);

    curState = SM_STATE_NEGO;
    nodeType = SM_T_NODE;
    SmDftReport(&addr, curState, nodeType, errVal);

    curState = SM_STATE_AUTH;
    nodeType = SM_G_NODE;
    SmDftReport(&addr, curState, nodeType, errVal);

    curState = SM_STATE_AUTH;
    nodeType = SM_T_NODE;
    SmDftReport(&addr, curState, nodeType, errVal);

    curState = SM_STATE_ENCP;
    nodeType = SM_T_NODE;
    SmDftReport(&addr, curState, nodeType, errVal);
}