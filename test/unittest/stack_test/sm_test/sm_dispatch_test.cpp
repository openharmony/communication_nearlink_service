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
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dli_mock.h"
#include "stack_dli_stub.h"

#include "sdf_mem.h"
#include "cpfwk_log.h"
#include "nlstk_api_type_ext.h"
#include "nlstk_public_define_ext.h"

#include "sm.h"
#include "nlstk_sm.h"
#include "nlstk_schedule.h"
#include "sm_nego.h"
#include "sm_noentry.h"
#include "sm_passcode.h"
#include "sm_password.h"
#include "sm_oob.h"
#include "sm_psk.h"
#include "sm_numcmp.h"
#include "sm_dhkey.h"
#include "sm_stub_test.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

#define STACK_MAX_THREAD 5
#define SM_PASSCODE_RAND_NUM_MOD 1000000

typedef enum {
    NOENTRY_T_NODE_CFM    = 0x0138,  /* T 节点确认码 */
    NOENTRY_RAND_NUM_RA   = 0x0139,  /* 随机数 Ra */
    NOENTRY_RAND_NUM_RB   = 0x013A,  /* 随机数 Rb */
} NOENTRY_Opcode;

typedef enum {
    NUMCMP_T_NODE_CFM_WITH_RB   = 0x013C,  /* 携带Rb的T节点确认码 */
    NUMCMP_RAND_NUM_RA          = 0x0139,  /* 随机数 Ra */
} NUMCMP_Opcode;

class UT_SM_DISPATCH_Test : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DliMock> dliMock;
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(TEST_ScheduleTimerAddWithHandleStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(TEST_ScheduleTimerDelWithHandleStub);
        
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

TEST_F(UT_SM_DISPATCH_Test, TestSmNegoDispatch)
{
    NLSTK_ERRCODE ret = NLSTK_ERRCODE_MAX;
    NLSTK_SmCryptoAlgoFuncs_S cbks = {
        .randNumFunc = Test_CryptoRandNumGenerate,
        .pubPriKeyFunc = Test_CryptoPubPriKeyPairGenerate,
        .secKeyFunc = Test_CryptoSecKeyGenerate,
        .derivedKeyFunc = Test_CryptoDerivedKeyGenerate,
    };
    ret = NLSTK_SmRegAlgoFuncs(&cbks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    SmSLink_S slink = {
        .rmtAddr = g_rmtAddr,
        .role = SM_T_NODE,
        .curStateIndex = SM_STATE_NEGO,
        .dispatcher = SmNegoPkgDispatcher,
        .lcid = 0,
        .expectOpCode = 0,
        .timerHandle = TIMER_NO_USED_VALUE,
        .uapiTimerHandle = TIMER_NO_USED_VALUE,
    };
    SmStateMachine_S *stm = SmStateMachineCtor(&slink);
    slink.stm = stm;
    STM_MFUNC(slink.stm, Transition, g_smStateName[SM_STATE_NEGO]);

    SmNegoStart(&slink);
    EXPECT_EQ(slink.expectOpCode, SM_NEGO_PAIRING_REQUEST);

    uint8_t pkg = {0};
    slink.timerHandle = TIMER_NO_USED_VALUE;
    SmNegoPkgDispatcher(SM_NEGO_PAIRING_START, &slink, &pkg, sizeof(SmPairStartMsg_S));
    EXPECT_EQ(slink.expectOpCode, SM_NEGO_PAIRING_RESPONSE);

    SmPairReqRspMsg_S reqMsg = {
        .ioAbility = 0x04,
        .oobDataFlag = 0,
        .authReq = { .secAttribute = 1, .mitmDefend = 0, .kpressNotif = 0 },
        .secKeyMaxLen = 16,
        .secInfoDis = 0,
        .codeAlgoCap = {2, 2, 2, 2},
        .pskFlag = 0,
    };
    uint8_t reqBuffer[sizeof(SmPairReqRspMsg_S)];
    (void)memcpy_s(reqBuffer, sizeof(SmPairReqRspMsg_S), &reqMsg, sizeof(SmPairReqRspMsg_S));
    slink.timerHandle = TIMER_NO_USED_VALUE;
    SmNegoPkgDispatcher(SM_NEGO_PAIRING_REQUEST, &slink, reqBuffer, sizeof(SmPairReqRspMsg_S));
    EXPECT_EQ(slink.expectOpCode, SM_NEGO_PAIRING_CONFIRM);

    SmPairReqRspMsg_S repMsg = {
        .ioAbility = 0x04,
        .oobDataFlag = 0,
        .authReq = { .secAttribute = 1, .mitmDefend = 0, .kpressNotif = 0 },
        .secKeyMaxLen = 16,
        .secInfoDis = 0,
        .codeAlgoCap = {2, 2, 2, 2},
        .pskFlag = 0,
    };
    uint8_t repBuffer[sizeof(SmPairReqRspMsg_S)];
    (void)memcpy_s(repBuffer, sizeof(SmPairReqRspMsg_S), &repMsg, sizeof(SmPairReqRspMsg_S));
    slink.timerHandle = TIMER_NO_USED_VALUE;
    SmNegoPkgDispatcher(SM_NEGO_PAIRING_RESPONSE, &slink, repBuffer, sizeof(SmPairReqRspMsg_S));
    EXPECT_EQ(slink.expectOpCode, SM_NEGO_PAIRING_INIT_INFO);

    SmPairCfmMsg_S cfmMsg = {
        .secKeyLen = 16,
        .authMethod = SM_AUTH_NO_ENTRY,
        .codeAlgoCap = {2, 2, 2, 2},
    };
    uint8_t pubKey[SM_PUBLIC_KEY_LEN] = {0};
    (void)memcpy_s(cfmMsg.gNodePubKey, SM_PUBLIC_KEY_LEN, pubKey, SM_PUBLIC_KEY_LEN);
    uint8_t cfmBuffer[sizeof(SmPairCfmMsg_S)];
    (void)memcpy_s(cfmBuffer, sizeof(SmPairCfmMsg_S), &cfmMsg, sizeof(SmPairCfmMsg_S));
    slink.timerHandle = TIMER_NO_USED_VALUE;
    slink.negoParams.authMethod = SM_AUTH_NO_ENTRY;
    slink.negoParams.secKeyMaxLen = 16;
    SmNegoPkgDispatcher(SM_NEGO_PAIRING_CONFIRM, &slink, cfmBuffer, sizeof(SmPairCfmMsg_S));
    EXPECT_EQ(slink.curStateIndex, SM_STATE_AUTH);

    SmPairInitInfoMsg_S initInfoMsg = {0};
    (void)memcpy_s(initInfoMsg.tNodePubKey, SM_PUBLIC_KEY_LEN, pubKey, SM_PUBLIC_KEY_LEN);
    uint8_t initInfoBuffer[sizeof(SmPairInitInfoMsg_S)];
    (void)memcpy_s(initInfoBuffer, sizeof(SmPairInitInfoMsg_S), &initInfoMsg, sizeof(SmPairInitInfoMsg_S));
    SmNegoPkgDispatcher(SM_NEGO_PAIRING_INIT_INFO, &slink, initInfoBuffer, sizeof(SmPairInitInfoMsg_S));
    EXPECT_EQ(slink.curStateIndex, SM_STATE_AUTH);

    SmStateMachineDtor(stm);
}

TEST_F(UT_SM_DISPATCH_Test, TestSmNoEntryDispatch)
{
    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] enter TestSmNoEntryDispatch");
    NLSTK_ERRCODE ret = NLSTK_ERRCODE_MAX;
    NLSTK_SmCryptoAlgoFuncs_S cbks = {
        .randNumFunc = Test_CryptoRandNumGenerate,
        .pubPriKeyFunc = Test_CryptoPubPriKeyPairGenerate,
        .secKeyFunc = Test_CryptoSecKeyGenerate,
        .derivedKeyFunc = Test_CryptoDerivedKeyGenerate,
    };
    ret = NLSTK_SmRegAlgoFuncs(&cbks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    SmSLink_S slink = {
        .rmtAddr = g_rmtAddr,
        .role = SM_G_NODE,
        .curStateIndex = SM_STATE_AUTH,
        .dispatcher = SmNoEntryAuthPkgDispatcher,
        .lcid = 0,
        .expectOpCode = 0,
        .timerHandle = TIMER_NO_USED_VALUE,
    };
    SmStateMachine_S *stm = SmStateMachineCtor(&slink);
    slink.stm = stm;
    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] TestSmNoEntryDispatch transition to auth");
    STM_MFUNC(slink.stm, Transition, g_smStateName[SM_STATE_AUTH]);

    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] TestSmNoEntryDispatch gNode enter SmNoEntryAuthStart");
    slink.timerHandle = TIMER_NO_USED_VALUE;
    SmNoEntryAuthStart(&slink);
    EXPECT_EQ(slink.expectOpCode, NOENTRY_T_NODE_CFM);

    slink.role = SM_T_NODE;
    slink.timerHandle = TIMER_NO_USED_VALUE;
    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] TestSmNoEntryDispatch tNode enter SmNoEntryAuthStart");
    SmNoEntryAuthStart(&slink);
    EXPECT_EQ(slink.expectOpCode, NOENTRY_RAND_NUM_RA);

    uint8_t pkg = {0};
    slink.timerHandle = TIMER_NO_USED_VALUE;
    SmNoEntryAuthPkgDispatcher(NOENTRY_T_NODE_CFM, &slink, &pkg, sizeof(SmAuthTCfmMsg_S));
    EXPECT_EQ(slink.expectOpCode, NOENTRY_RAND_NUM_RB);

    uint8_t randNum[SM_RANDOM_NUMBER_R_LEN];
    for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
        randNum[i] = i;
    }
    SmAuthRandomAMsg_S raMsg = {0};
    (void)memcpy_s(raMsg.authData, SM_RANDOM_NUMBER_R_LEN, randNum, SM_RANDOM_NUMBER_R_LEN);
    uint8_t raBuffer[sizeof(SmAuthRandomAMsg_S)];
    (void)memcpy_s(raBuffer, sizeof(SmAuthRandomAMsg_S), &raMsg, sizeof(SmAuthRandomAMsg_S));
    slink.timerHandle = TIMER_NO_USED_VALUE;
    SmNoEntryAuthPkgDispatcher(NOENTRY_RAND_NUM_RA, &slink, raBuffer, sizeof(SmAuthRandomAMsg_S));
    EXPECT_EQ(slink.expectOpCode, SM_AUTH_G_NODE_DHKEY);

    SmAuthRandomBMsg_S rbMsg = {0};
    (void)memcpy_s(rbMsg.authData, SM_RANDOM_NUMBER_R_LEN, randNum, SM_RANDOM_NUMBER_R_LEN);
    uint8_t rbBuffer[sizeof(SmAuthRandomBMsg_S)];
    (void)memcpy_s(rbBuffer, sizeof(SmAuthRandomBMsg_S), &rbMsg, sizeof(SmAuthRandomBMsg_S));
    slink.timerHandle = TIMER_NO_USED_VALUE;
    SmNoEntryAuthPkgDispatcher(NOENTRY_RAND_NUM_RB, &slink, rbBuffer, sizeof(SmAuthRandomBMsg_S));
    EXPECT_NE(slink.uapiTimerHandle, TIMER_NO_USED_VALUE);

    slink.role = SM_G_NODE;
    slink.timerHandle = TIMER_NO_USED_VALUE;
    slink.uapiTimerHandle = TIMER_NO_USED_VALUE;
    SmNoEntryContinueNoentry(&slink);
    EXPECT_EQ(slink.expectOpCode, SM_AUTH_T_NODE_DHKEY);

    slink.role = SM_T_NODE;
    SmAuthDhkeyMsg_S dhkeyMsg = {0};
    uint8_t dhkeyCode[SM_DHKEY_AUTHCODE_LEN] = {0};
    (void)memcpy_s(dhkeyMsg.authData, SM_DHKEY_AUTHCODE_LEN, dhkeyCode, SM_DHKEY_AUTHCODE_LEN);
    uint8_t dhkeyBuffer[sizeof(SmAuthDhkeyMsg_S)];
    (void)memcpy_s(dhkeyBuffer, sizeof(SmAuthDhkeyMsg_S), &dhkeyMsg, sizeof(SmAuthDhkeyMsg_S));
    slink.timerHandle = TIMER_NO_USED_VALUE;
    SmNoEntryAuthPkgDispatcher(SM_AUTH_G_NODE_DHKEY, &slink, dhkeyBuffer, sizeof(SmAuthDhkeyMsg_S));
    EXPECT_EQ(slink.curStateIndex, SM_STATE_ENCP);

    slink.role = SM_G_NODE;
    slink.timerHandle = TIMER_NO_USED_VALUE;
    SmNoEntryAuthPkgDispatcher(SM_AUTH_T_NODE_DHKEY, &slink, dhkeyBuffer, sizeof(SmAuthDhkeyMsg_S));
    EXPECT_EQ(slink.curStateIndex, SM_STATE_ENCP);

    SmStateMachineDtor(stm);
}

TEST_F(UT_SM_DISPATCH_Test, TestSmNumCmpDispatch)
{
    NLSTK_ERRCODE ret = NLSTK_ERRCODE_MAX;
    NLSTK_SmCryptoAlgoFuncs_S cbks = {
        .randNumFunc = Test_CryptoRandNumGenerate,
        .pubPriKeyFunc = Test_CryptoPubPriKeyPairGenerate,
        .secKeyFunc = Test_CryptoSecKeyGenerate,
        .derivedKeyFunc = Test_CryptoDerivedKeyGenerate,
    };
    ret = NLSTK_SmRegAlgoFuncs(&cbks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    SmSLink_S slink = {
        .rmtAddr = g_rmtAddr,
        .role = SM_G_NODE,
        .curStateIndex = SM_STATE_AUTH,
        .dispatcher = SmNumCmpAuthPkgDispatcher,
        .lcid = 0,
        .expectOpCode = 0,
        .timerHandle = TIMER_NO_USED_VALUE,
    };
    SmStateMachine_S *stm = SmStateMachineCtor(&slink);
    slink.stm = stm;
    STM_MFUNC(slink.stm, Transition, g_smStateName[SM_STATE_AUTH]);

    SmNumCmpAuthStart(&slink);
    EXPECT_EQ(slink.expectOpCode, NUMCMP_T_NODE_CFM_WITH_RB);

    slink.role = SM_T_NODE;
    slink.timerHandle = TIMER_NO_USED_VALUE;
    SmNumCmpAuthStart(&slink);
    EXPECT_EQ(slink.expectOpCode, NUMCMP_RAND_NUM_RA);

    SmAuthTCfmWithRbMsg_S tCfmWithRbMsg;
    (void)memset_s(tCfmWithRbMsg.tNodeCfm, SM_CONFIRM_NUMBER_LEN, 0, SM_CONFIRM_NUMBER_LEN);
    for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
        tCfmWithRbMsg.randomB[i] = i;
    }
    uint8_t tCfmWithRbBuffer[sizeof(SmAuthTCfmWithRbMsg_S)];
    (void)memcpy_s(tCfmWithRbBuffer, sizeof(SmAuthTCfmWithRbMsg_S), &tCfmWithRbMsg, sizeof(SmAuthTCfmWithRbMsg_S));
    slink.timerHandle = TIMER_NO_USED_VALUE;
    SmNumCmpAuthPkgDispatcher(NUMCMP_T_NODE_CFM_WITH_RB, &slink, tCfmWithRbBuffer, sizeof(SmAuthTCfmWithRbMsg_S));
    EXPECT_NE(slink.uapiTimerHandle, TIMER_NO_USED_VALUE);

    uint8_t randNum[SM_RANDOM_NUMBER_R_LEN];
    for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
        randNum[i] = i;
    }
    SmAuthRandomAMsg_S raMsg = {0};
    (void)memcpy_s(raMsg.authData, SM_RANDOM_NUMBER_R_LEN, randNum, SM_RANDOM_NUMBER_R_LEN);
    uint8_t raBuffer[sizeof(SmAuthRandomAMsg_S)];
    (void)memcpy_s(raBuffer, sizeof(SmAuthRandomAMsg_S), &raMsg, sizeof(SmAuthRandomAMsg_S));
    slink.timerHandle = TIMER_NO_USED_VALUE;
    slink.uapiTimerHandle = TIMER_NO_USED_VALUE;
    SmNumCmpAuthPkgDispatcher(NUMCMP_RAND_NUM_RA, &slink, raBuffer, sizeof(SmAuthRandomAMsg_S));
    EXPECT_EQ(slink.expectOpCode, SM_AUTH_G_NODE_DHKEY);

    slink.role = SM_G_NODE;
    slink.timerHandle = TIMER_NO_USED_VALUE;
    slink.uapiTimerHandle = TIMER_NO_USED_VALUE;
    SmNumCmpContinueNumComparison(&slink);
    EXPECT_EQ(slink.expectOpCode, SM_AUTH_T_NODE_DHKEY);

    slink.role = SM_T_NODE;
    slink.timerHandle = TIMER_NO_USED_VALUE;
    slink.uapiTimerHandle = TIMER_NO_USED_VALUE;
    slink.tNode.recvFlag = true;
    SmNumCmpContinueNumComparison(&slink);
    EXPECT_EQ(slink.curStateIndex, SM_STATE_AUTH);

    SmStateMachineDtor(stm);
}

TEST_F(UT_SM_DISPATCH_Test, TestSmPasswordDispatch)
{
    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] enter TestSmPasswordDispatch");
    NLSTK_ERRCODE ret = NLSTK_ERRCODE_MAX;

    NLSTK_SmCryptoAlgoFuncs_S cbks = {
        .randNumFunc = Test_CryptoRandNumGenerate,
        .pubPriKeyFunc = Test_CryptoPubPriKeyPairGenerate,
        .secKeyFunc = Test_CryptoSecKeyGenerate,
        .derivedKeyFunc = Test_CryptoDerivedKeyGenerate,
    };
    ret = NLSTK_SmRegAlgoFuncs(&cbks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    NLSTK_SmCallbacks_S smCbks = {
        .startCbk = Test_SmPairStartCbk,
        .removeCbk = Test_SmPairRemoveCbk,
        .requestCbk = Test_SmPairRequestCbk,
        .authCbk = Test_SmAuthCmpCbk,
        .encCbk = Test_SmEncCmpCbk,
        .imgMsgCbk = Test_SmImgMsgCbk,
    };
    SmRegExternalCbks(&smCbks);

    // G节点在鉴权过程中预期会收到T节点2次pkg:
    // 1. 收到携带Rb的T节点确认码SM_AUTH_T_NODE_CFM_WITH_RB，发送G 节点 DHKey 验证码SM_AUTH_G_NODE_DHKEY
    // 2. 收到T发送的DHKEY验证码消息，本地状态机处理SM_AUTH_SUCCESS，成功后切到加密状态
    SmSLink_S *slink = SmFindOrCreateSLink(&g_rmtAddr);
    ASSERT_NE(slink, NULL);
    slink->negoParams.authMethod = SM_AUTH_PASSWORD_ENTRY;  // 协商后采用的鉴权方式

    SmStateMachine_S *stm = slink->stm;
    EXPECT_NE(stm, NULL);
    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] stm Transition to SM_STATE_AUTH");
    STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_AUTH]); // enter SmPassWordAuthStart
    EXPECT_EQ(slink->uapiTimerHandle, TIMER_NO_USED_VALUE + 1);

    char passWordVal[] = "1234";
    NLSTK_SmPassWord_S *passWodeIn = (NLSTK_SmPassWord_S *)SDF_MemZalloc(sizeof(NLSTK_SmPassWord_S) +
        strlen(passWordVal));
    passWodeIn->passWordLen = strlen(passWordVal);
    (void)memcpy_s(passWodeIn->passWord, strlen(passWordVal), passWordVal, strlen(passWordVal));
    (void)memcpy_s(&passWodeIn->addr, sizeof(SLE_Addr_S), &g_rmtAddr, sizeof(SLE_Addr_S));
    NLSTK_Errcode_E result = NLSTK_ERRCODE_MAX;
    result = NLSTK_SmSetPassWord(passWodeIn);  // enter gNode SmSetPassWord
    EXPECT_EQ(result, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(slink->uapiTimerHandle, TIMER_NO_USED_VALUE);
    EXPECT_EQ(slink->expectOpCode, SM_AUTH_T_NODE_CFM_WITH_RB);

    SmAuthTCfmWithRbMsg_S tCfmWithRbMsg;
    (void)memset_s(tCfmWithRbMsg.tNodeCfm, SM_CONFIRM_NUMBER_LEN, 0, SM_CONFIRM_NUMBER_LEN);
    for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
        tCfmWithRbMsg.randomB[i] = i;
    }
    uint8_t tCfmWithRbBuffer[sizeof(SmAuthTCfmWithRbMsg_S)];
    (void)memcpy_s(tCfmWithRbBuffer, sizeof(SmAuthTCfmWithRbMsg_S), &tCfmWithRbMsg, sizeof(SmAuthTCfmWithRbMsg_S));
    slink->timerHandle = TIMER_NO_USED_VALUE;
    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] step1: gNode recv SM_AUTH_T_NODE_CFM_WITH_RB");
    SmPassWordAuthPkgDispatcher(SM_AUTH_T_NODE_CFM_WITH_RB, slink, tCfmWithRbBuffer, sizeof(SmAuthTCfmWithRbMsg_S));
    EXPECT_EQ(slink->expectOpCode, SM_AUTH_T_NODE_DHKEY);

    SmAuthDhkeyMsg_S dhkeyMsg = {0};
    uint8_t dhkeyCode[SM_DHKEY_AUTHCODE_LEN] = {0};
    (void)memcpy_s(dhkeyMsg.authData, SM_DHKEY_AUTHCODE_LEN, dhkeyCode, SM_DHKEY_AUTHCODE_LEN);
    uint8_t dhkeyBuffer[sizeof(SmAuthDhkeyMsg_S)];
    (void)memcpy_s(dhkeyBuffer, sizeof(SmAuthDhkeyMsg_S), &dhkeyMsg, sizeof(SmAuthDhkeyMsg_S));
    slink->timerHandle = TIMER_NO_USED_VALUE;
    Test_ResetData();
    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] step2: gNode recv SM_AUTH_T_NODE_DHKEY");
    SmPassWordAuthPkgDispatcher(SM_AUTH_T_NODE_DHKEY, slink, dhkeyBuffer, sizeof(SmAuthDhkeyMsg_S));
    EXPECT_EQ(slink->curStateIndex, SM_STATE_ENCP);
    EXPECT_EQ(g_isRecvCbk, true);

    // T节点在鉴权过程中预期会收到G节点2次pkg:
    // 1. 收到携带Ra的G节点确认码SM_AUTH_G_NODE_CFM_WITH_RA，发送携带Rb的T节点确认码
    // 2. 收到G 节点 DHKey 验证码，发送T 节点 DHKey 验证码，本地状态机处理SM_AUTH_SUCCESS，成功后切到加密状态
    slink->role = SM_T_NODE;
    slink->timerHandle = TIMER_NO_USED_VALUE;
    slink->uapiTimerHandle = TIMER_NO_USED_VALUE;
    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] tNode enter SmPassWordAuthStart");
    STM_MFUNC(slink->stm, Transition, g_smStateName[SM_STATE_AUTH]); // enter SmPassWordAuthStart
    EXPECT_EQ(slink->expectOpCode, SM_AUTH_G_NODE_CFM_WITH_RA);

    result = NLSTK_SmSetPassWord(passWodeIn);  // enter tNode SmSetPassWord
    EXPECT_EQ(result, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(slink->uapiTimerHandle, TIMER_NO_USED_VALUE);

    SmAuthGCfmWithRaMsg_S gCfmWithRaMsg;
    (void)memset_s(gCfmWithRaMsg.gNodeCfm, SM_CONFIRM_NUMBER_LEN, 0, SM_CONFIRM_NUMBER_LEN);
    for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
        gCfmWithRaMsg.randomA[i] = i;
    }
    uint8_t gCfmWithRaBuffer[sizeof(SmAuthGCfmWithRaMsg_S)];
    (void)memcpy_s(gCfmWithRaBuffer, sizeof(SmAuthGCfmWithRaMsg_S), &gCfmWithRaMsg, sizeof(SmAuthGCfmWithRaMsg_S));
    slink->timerHandle = TIMER_NO_USED_VALUE;
    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] step1: tNode recv SM_AUTH_G_NODE_CFM_WITH_RA");
    SmPassWordAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink, gCfmWithRaBuffer, sizeof(SmAuthGCfmWithRaMsg_S));
    EXPECT_EQ(slink->expectOpCode, SM_AUTH_G_NODE_DHKEY);

    slink->timerHandle = TIMER_NO_USED_VALUE;
    Test_ResetData();
    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] step2: tNode recv SM_AUTH_G_NODE_DHKEY");
    SmPassWordAuthPkgDispatcher(SM_AUTH_G_NODE_DHKEY, slink, dhkeyBuffer, sizeof(SmAuthDhkeyMsg_S));
    EXPECT_EQ(slink->curStateIndex, SM_STATE_ENCP);
    EXPECT_EQ(g_isRecvCbk, true);

    SDF_MemFree(passWodeIn);
    CP_LOG_INFO("[TEST][UT_SM_DISPATCH_Test] TestSmPasswordDispatch complete");
}