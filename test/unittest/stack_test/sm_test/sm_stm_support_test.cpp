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
#include "stack_sdf_mem_mock.h"
#include "stack_sdf_mem_stub.h"

#include "sdf_mem.h"
#include "sle_crypto.h"
#include "nlstk_sm_api.h"
#include "nlstk_sm_algos.h"
#include "sm.h"
#include "sm_algos.h"
#include "nlstk_sm.h"
#include "nlstk_schedule.h"
#include "sm_nego.h"
#include "sm_numcmp.h"
#include "sm_errcode.h"  // verify-fix: SM_ERR_ACTIVE_CANCEL
#include "sm_psk.h"  // verify-fix: SmPskAuthStart  // verify-fix: SmNumCmpAuthPkgDispatcher
#include "sm_auth.h"
#include "sm_encp.h"
#include "sm_img.h"
#include "sm_dhkey.h"
#include "sm_stm.h"
#include "sm_slink.h"
#include "sm_struct.h"
#include "sm_stub_test.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static uint32_t g_authCbkCnt;
static uint8_t g_authCbkStatus;
static uint32_t g_removeCbkCnt;
static uint32_t g_encCbkCnt;
static uint8_t g_encCbkStatus;
static uint32_t g_imgMsgCbkCnt;
static uint8_t g_imgMsgCbkStatus;

static void Test_AuthCbkCapture(NLSTK_SmAuthComplete_S *params)
{
    g_authCbkCnt++;
    g_authCbkStatus = params->authStatus;
}

static void Test_EncCbkCapture(NLSTK_SmEncComplete_S *params)
{
    g_encCbkCnt++;
    g_encCbkStatus = params->encStatus;
}

static void Test_ImgMsgCbkCapture(NLSTK_SmSendImgMsgCmpl_S *params)
{
    g_imgMsgCbkCnt++;
    g_imgMsgCbkStatus = params->sendStatus;
}

static void Test_PairStartCbkCapture(NLSTK_SmPairingStart_S *params) {}
static void Test_RemoveCbkCapture(NLSTK_SmPairingRemove_S *params)
{
    g_removeCbkCnt++;
}
static void Test_RequestCbkCapture(NLSTK_SmPairingRequest_S *params) {}

static uint32_t g_randOkCount;

static bool Test_CryptoRandNTimesOk(uint8_t *out, uint8_t len)
{
    if (g_randOkCount == 0) {
        return false;
    }
    g_randOkCount--;
    for (uint8_t i = 0; i < len; i++) {
        out[i] = i;
    }
    return true;
}

static bool Test_CryptoDerivedFail(NLSTK_SmDerivedMac_S *input, uint8_t *output, uint8_t outputLen)
{
    return false;
}

static void FillSeq(uint8_t *buff, uint8_t len, uint8_t seed)
{
    for (uint8_t i = 0; i < len; i++) {
        buff[i] = seed + i;
    }
}

/* 第 allocSeq 次内存分配失败，其余放行：先声明失败期望、再声明放行期望，放行饱和后失败生效 */
static void ExpectAllocFailOn(NiceMock<SdfMemMock> &mock, uint32_t allocSeq)
{
    EXPECT_CALL(mock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(mock, SDF_MemZalloc).Times(allocSeq - 1)
        .WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
}

class UT_SM_STMSUPP_Test : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DliMock> dliMock;
    NiceMock<SdfMemMock> sdfMemMock;
protected:
    NLSTK_SmCryptoAlgoFuncs_S savedAlgos;

    virtual void SetUp()
    {
        ON_CALL(sdfMemMock, SDF_MemZalloc).WillByDefault(TEST_SDF_MemZalloc);
        g_authCbkCnt = 0;
        g_authCbkStatus = 0xFF;
        g_removeCbkCnt = 0;
        g_encCbkCnt = 0;
        g_encCbkStatus = 0xFF;
        g_imgMsgCbkCnt = 0;
        g_imgMsgCbkStatus = 0xFF;
        g_randOkCount = 0;
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
        NLSTK_SmLocalParams_S *localParams = SmGetLocalParams();
        ASSERT_NE(localParams, nullptr);
        localParams->codeAlgoCap[SM_ENC_ALGO_ABILITY] = SM_ENCRYPTION_ALGORITHM_ABILITY_AC2;
        localParams->codeAlgoCap[SM_INTG_PROTECT_ALGO_ABILITY] = SM_INTEGRITY_PROTECTION_ALGORITHM_ABILITY_AC2;
        localParams->codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
        localParams->codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;

        NLSTK_SmCallbacks_S smCbks = {
            .startCbk = Test_PairStartCbkCapture,
            .removeCbk = Test_RemoveCbkCapture,
            .requestCbk = Test_RequestCbkCapture,
            .authCbk = Test_AuthCbkCapture,
            .encCbk = Test_EncCbkCapture,
            .imgMsgCbk = Test_ImgMsgCbkCapture,
        };
        SmRegExternalCbks(&smCbks);
        savedAlgos = *SmGetAlgoFuncs();

        NLSTK_SmCryptoAlgoFuncs_S cbks = {
            .randNumFunc = Test_CryptoRandNumGenerate,
            .pubPriKeyFunc = Test_CryptoPubPriKeyPairGenerate,
            .secKeyFunc = Test_CryptoSecKeyGenerate,
            .derivedKeyFunc = Test_CryptoDerivedKeyGenerate,
        };
        ASSERT_EQ(NLSTK_SmRegAlgoFuncs(&cbks), NLSTK_ERRCODE_SUCCESS);
    }

    virtual void TearDown()
    {
        *SmGetAlgoFuncs() = savedAlgos;
        SmDeInit();
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
        TEST_DLI_DeInit();
    }
};

static void RegisterRealAlgos(void)
{
    NLSTK_SmCryptoAlgoFuncs_S cbks = {
        .randNumFunc = Crypto_RandNumGenerate,
        .pubPriKeyFunc = Crypto_PubPriKeyPairGenerate,
        .secKeyFunc = Crypto_SecKeyGenerate,
        .derivedKeyFunc = Crypto_DerivedKeyGenerate,
        .sha256HashFunc = Crypto_Sha256,
    };
    ASSERT_EQ(NLSTK_SmRegAlgoFuncs(&cbks), NLSTK_ERRCODE_SUCCESS);
}

/* 栈上链路对象挂真实状态机：INIT 态吞掉鉴权成功/内部错误以外的消息，错误则安全走拆除 */
static SmStateMachine_S *MakeStackSlinkWithStm(SmSLink_S *slink)
{
    (void)memset_s(slink, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    slink->negoParams.authMethod = SM_AUTH_METHOD_MAX;
    slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
    slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
    slink->lcid = 0;
    SmStateMachine_S *stm = SmStateMachineCtor(slink);
    if (stm != nullptr) {
        slink->stm = stm;
    }
    return stm;
}

/*****************************************************************************************
                                    Test Functions
*****************************************************************************************/

TEST_F(UT_SM_STMSUPP_Test, DtorNullArgsAreSafe)
{
    // 状态机与链路对象的析构对空指针入参必须安全返回；鉴权方式越界查表返回空
    SmStateMachineDtor(nullptr);
    SmSLinkDtor(nullptr);
    EXPECT_EQ(SmGetAuthPkgDispatcher(SM_AUTH_METHOD_MAX), nullptr);
    EXPECT_EQ(SmGetAuthStartFunc(SM_AUTH_METHOD_MAX), nullptr);
    EXPECT_EQ(SmGetAuthPkgDispatcher(SM_AUTH_NUMBER_COMPARE), &SmNumCmpAuthPkgDispatcher);
    EXPECT_EQ(SmGetAuthStartFunc(SM_AUTH_PSK), &SmPskAuthStart);
}

TEST_F(UT_SM_STMSUPP_Test, CtorFailOnEachAllocationPoint)
{
    // 构造链上任何一次内存分配失败都返回空指针：链路对象、状态机壳与七个状态逐一注入
    SLE_Addr_S addr = g_rmtAddr;
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_EQ(SmSLinkCtor(&addr), nullptr);

    for (uint32_t seq = 2; seq <= 15; seq++) {
        ExpectAllocFailOn(sdfMemMock, seq);
        SmSLink_S stackSlink;
        EXPECT_EQ(SmStateMachineCtor(&stackSlink), nullptr) << "alloc seq " << seq;
    }

    ExpectAllocFailOn(sdfMemMock, 2);
    EXPECT_EQ(SmSLinkCtor(&addr), nullptr);
}

TEST_F(UT_SM_STMSUPP_Test, InitBindSuccessAndFailureTransition)
{
    // INIT 态被动/主动启动：绑定逻辑链成功迁移协商态，失败迁移拆除态
    SmSLink_S stackSlink;
    SmStateMachine_S *stm = MakeStackSlinkWithStm(&stackSlink);
    ASSERT_NE(stm, nullptr);
    SmSLink_S *slink = &stackSlink;

    ::Message msg = {.what = SM_PASSIVE_START};
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_NEGO);

    stm->base.current_ = stm->base.states_->next->next->next->next->next->next;
    slink->timerHandle = TIMER_NO_USED_VALUE;
    EXPECT_CALL(cmMock, CM_GetLogicLinkByAddr).WillOnce(Return(1)).RetiresOnSaturation();
    msg.what = SM_ACTIVE_START;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_removeCbkCnt, 1);
    SmStateMachineDtor(stm);
}

TEST_F(UT_SM_STMSUPP_Test, InitRecoverAndErrorTransition)
{
    // INIT 态收恢复启动迁移加密态，收内部错误迁移拆除态
    SmSLink_S stackSlink;
    SmStateMachine_S *stm = MakeStackSlinkWithStm(&stackSlink);
    ASSERT_NE(stm, nullptr);
    SmSLink_S *slink = &stackSlink;

    ::Message msg = {.what = SM_RECOVER_START};
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_ENCP);

    stm->base.current_ = stm->base.states_->next->next->next->next->next->next;
    msg.what = SM_INTERNAL_ERROR;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_removeCbkCnt, 1);
    SmStateMachineDtor(stm);
}

TEST_F(UT_SM_STMSUPP_Test, NegoMsgTransitionLegs)
{
    // NEGO 态：协商成功迁移鉴权态；超时与断链报错误回调并拆除；主动取消还要给对端发失败报文
    SmSLink_S stackSlink;
    SmStateMachine_S *stm = MakeStackSlinkWithStm(&stackSlink);
    ASSERT_NE(stm, nullptr);
    SmSLink_S *slink = &stackSlink;
    State *negoState = stm->base.states_->next->next->next->next->next;

    ::Message msg = {.what = SM_NEGO_SUCCESS};
    stm->base.current_ = negoState;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_AUTH);

    stm->base.current_ = negoState;
    slink->timerHandle = TIMER_NO_USED_VALUE;
    msg.what = SM_REMOVE_PAIR;
    msg.extData = (void *)(uintptr_t)SM_ERR_ACTIVE_CANCEL;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_ACTIVE_CANCEL);

    stm->base.current_ = negoState;
    slink->timerHandle = TIMER_NO_USED_VALUE;
    msg.what = SM_TIMEOUT;
    msg.extData = NULL;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_ERROR);

    stm->base.current_ = negoState;
    slink->timerHandle = TIMER_NO_USED_VALUE;
    msg.what = SM_LCHANNEL_DISCONN;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_LINK_DISCONNCTED);
    EXPECT_EQ(g_removeCbkCnt, 3);
    SmStateMachineDtor(stm);
}

TEST_F(UT_SM_STMSUPP_Test, AuthMsgTransitionLegs)
{
    // AUTH 态：鉴权成功回调成功并迁移加密态；主动取消给对端发失败报文并拆除
    SmSLink_S stackSlink;
    SmStateMachine_S *stm = MakeStackSlinkWithStm(&stackSlink);
    ASSERT_NE(stm, nullptr);
    SmSLink_S *slink = &stackSlink;
    State *authState = stm->base.states_->next->next->next->next;

    ::Message msg = {.what = SM_AUTH_SUCCESS};
    stm->base.current_ = authState;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_ENCP);
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_OK);

    stm->base.current_ = authState;
    slink->timerHandle = TIMER_NO_USED_VALUE;
    msg.what = SM_REMOVE_PAIR;
    msg.extData = (void *)(uintptr_t)SM_ERR_ACTIVE_CANCEL;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_ACTIVE_CANCEL);
    EXPECT_EQ(g_removeCbkCnt, 1);
    SmStateMachineDtor(stm);
}

TEST_F(UT_SM_STMSUPP_Test, EncpMsgTransitionLegs)
{
    // ENCP 态：加密成功迁移完成态；密钥丢失先迁密钥缺失态再自动续跑绑定回协商态
    SmSLink_S stackSlink;
    SmStateMachine_S *stm = MakeStackSlinkWithStm(&stackSlink);
    ASSERT_NE(stm, nullptr);
    SmSLink_S *slink = &stackSlink;
    State *encpState = stm->base.states_->next->next->next;

    ::Message msg = {.what = SM_ENCP_SUCCESS};
    stm->base.current_ = encpState;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_FULL);
    EXPECT_EQ(g_encCbkStatus, SM_PAIR_OK);

    stm->base.current_ = encpState;
    slink->timerHandle = TIMER_NO_USED_VALUE;
    msg.what = SM_ENCP_FAIL;
    msg.extData = (void *)(uintptr_t)SM_ERR_STATUS_KEY_MISSING;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_NEGO);

    stm->base.current_ = encpState;
    slink->timerHandle = TIMER_NO_USED_VALUE;
    msg.extData = (void *)(uintptr_t)0x01;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(g_encCbkStatus, SM_PAIR_ERROR);
    SmStateMachineDtor(stm);
}

TEST_F(UT_SM_STMSUPP_Test, MissHandleBindLegs)
{
    // 密钥缺失态续跑：绑定成功迁回协商态，绑定失败保持原态
    SmSLink_S stackSlink;
    SmStateMachine_S *stm = MakeStackSlinkWithStm(&stackSlink);
    ASSERT_NE(stm, nullptr);
    SmSLink_S *slink = &stackSlink;
    State *missState = stm->base.states_->next->next;

    ::Message msg = {.what = SM_MISS_HANDLE};
    stm->base.current_ = missState;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_NEGO);

    stm->base.current_ = missState;
    slink->timerHandle = TIMER_NO_USED_VALUE;
    EXPECT_CALL(cmMock, CM_GetLogicLinkByAddr).WillOnce(Return(1)).RetiresOnSaturation();
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_MISS);
    SmStateMachineDtor(stm);
}

TEST_F(UT_SM_STMSUPP_Test, AlgoWrapperNullAndFailLegs)
{
    // 算法包装层：算法未注册时一律报失败，注册后按算法结果返回
    NLSTK_SmKeyPair_S keyPair = {.algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2};
    uint8_t secKey[SM_DHKEY_LEN] = {0};
    NLSTK_VariableData_S value = {.len = sizeof(secKey), .data = secKey};
    NLSTK_SmSha256Hash hash = {0};

    SmGetAlgoFuncs()->pubPriKeyFunc = nullptr;
    EXPECT_FALSE(SmGenPubPriKey(&keyPair));
    SmGetAlgoFuncs()->secKeyFunc = nullptr;
    EXPECT_FALSE(SmGenDhKey(&keyPair, secKey, sizeof(secKey)));
    SmGetAlgoFuncs()->sha256HashFunc = nullptr;
    EXPECT_FALSE(SmGenSha256(&value, &hash));

    SmGetAlgoFuncs()->pubPriKeyFunc = Test_CryptoPubPriKeyPairGenerate;
    SmGetAlgoFuncs()->secKeyFunc = Test_CryptoSecKeyGenerate;
    SmGetAlgoFuncs()->sha256HashFunc = Crypto_Sha256;
    EXPECT_TRUE(SmGenPubPriKey(&keyPair));
    EXPECT_TRUE(SmGenDhKey(&keyPair, secKey, sizeof(secKey)));
    EXPECT_TRUE(SmGenSha256(&value, &hash));
}

TEST_F(UT_SM_STMSUPP_Test, DhKeySaltAndBuffFailLegs)
{
    // DHKEY 确认码材料：五种鉴权方式的取盐路径都能生成并发送；确认码缓冲分配失败安全拆除
    SmSLink_S stackSlink;
    SmStateMachine_S *stm = MakeStackSlinkWithStm(&stackSlink);
    ASSERT_NE(stm, nullptr);
    SmSLink_S *slink = &stackSlink;
    slink->tNode.passCode = 123456;
    FillSeq(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, 0x10);
    FillSeq(slink->tNode.randomR, SM_RANDOM_NUMBER_R_LEN, 0x20);

    const uint8_t authMethods[] = {
        SM_AUTH_NUMBER_COMPARE, SM_AUTH_NO_ENTRY, SM_AUTH_OUT_OF_BAND, SM_AUTH_PSK, SM_AUTH_PASSCODE,
        SM_AUTH_PASSWORD_ENTRY,
    };
    for (size_t i = 0; i < sizeof(authMethods) / sizeof(authMethods[0]); i++) {
        slink->negoParams.authMethod = authMethods[i];
        slink->curStateIndex = SM_STATE_INIT;
        stm->base.current_ = stm->base.states_->next->next->next->next->next->next;
        SmSendTNodeDhKey(slink); // 成功发送后投递鉴权成功消息，INIT 态忽略
        EXPECT_EQ(slink->curStateIndex, SM_STATE_INIT);
    }

    slink->negoParams.authMethod = SM_AUTH_PSK;
    stm->base.current_ = stm->base.states_->next->next->next->next->next->next;
    slink->curStateIndex = SM_STATE_INIT;
    ExpectAllocFailOn(sdfMemMock, 2); // 输入密钥材料分配成功、确认码缓冲分配失败
    SmSendTNodeDhKey(slink);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV);
    SmStateMachineDtor(stm);
}

TEST_F(UT_SM_STMSUPP_Test, EncpAlgoCapSelectLegs)
{
    // 加密能力选择：加密与完整性能力相等取交集值，不等时回退到无效算法标记，均不触发内部错误
    SmSLink_S stackSlink;
    SmStateMachine_S *stm = MakeStackSlinkWithStm(&stackSlink);
    ASSERT_NE(stm, nullptr);
    SmSLink_S *slink = &stackSlink;
    stm->base.current_ = stm->base.states_->next->next->next;

    slink->negoParams.codeAlgoCap[SM_ENC_ALGO_ABILITY] = SM_ENCRYPTION_ALGORITHM_ABILITY_AC2;
    slink->negoParams.codeAlgoCap[SM_INTG_PROTECT_ALGO_ABILITY] = SM_INTEGRITY_PROTECTION_ALGORITHM_ABILITY_AC2;
    SmEncpEnable(slink);

    slink->negoParams.codeAlgoCap[SM_INTG_PROTECT_ALGO_ABILITY] = 0x04;
    SmEncpEnable(slink);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_INIT);
    EXPECT_EQ(g_authCbkCnt, 0);
    SmStateMachineDtor(stm);
}

TEST_F(UT_SM_STMSUPP_Test, ImgKeyGenFailureLegs)
{
    // 组播密钥材料生成：随机数或派生算法在任何一步失败，输出保持全零；成功路径输出非全零
    uint8_t groupKey[SM_OCTETS_16] = {0};
    uint64_t giv = 0;

    SmGetAlgoFuncs()->randNumFunc = nullptr; // 第一次随机数失败
    SmGenerateImgSecuConfig(SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2, groupKey, &giv);
    EXPECT_EQ(groupKey[0], 0);

    g_randOkCount = 1; // 第二次随机数失败
    SmGetAlgoFuncs()->randNumFunc = Test_CryptoRandNTimesOk;
    SmGenerateImgSecuConfig(SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2, groupKey, &giv);
    EXPECT_EQ(groupKey[0], 0);

    SmGetAlgoFuncs()->randNumFunc = Test_CryptoRandNumGenerate;
    SmGetAlgoFuncs()->derivedKeyFunc = Test_CryptoDerivedFail; // 派生算法失败
    SmGenerateImgSecuConfig(SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2, groupKey, &giv);
    EXPECT_EQ(groupKey[0], 0);

    g_randOkCount = 2; // 组密钥两笔随机数成功、组播向量随机数失败
    SmGetAlgoFuncs()->randNumFunc = Test_CryptoRandNTimesOk;
    SmGetAlgoFuncs()->derivedKeyFunc = Test_CryptoDerivedKeyGenerate;
    SmGenerateImgSecuConfig(SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2, groupKey, &giv);
    EXPECT_EQ(groupKey[0], 0);

    RegisterRealAlgos();
    SmGenerateImgSecuConfig(SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2, groupKey, &giv);
    EXPECT_NE(groupKey[0] | groupKey[SM_OCTETS_15], 0);
}

TEST_F(UT_SM_STMSUPP_Test, ImgSendFailAndOkLegs)
{
    // 组播安全配置下发：随机数失败时报错误完成事件，材料齐全时报成功完成事件
    SLE_Addr_S addr = g_rmtAddr;
    SmSLink_S *slink = SmFindOrCreateSLink(&addr);
    ASSERT_NE(slink, nullptr);
    NLSTK_SmImgSecuConfig_S *config = (NLSTK_SmImgSecuConfig_S *)SDF_MemZalloc(sizeof(NLSTK_SmImgSecuConfig_S));
    ASSERT_NE(config, nullptr);
    config->addr = addr;
    config->keyDerivAlgo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
    config->cryptoAlgo = SM_ENCRYPTION_ALGORITHM_ABILITY_AC2;
    config->intgChkInd = SM_ENCRYPTION_INTEGRITY_PROTECTION;
    FillSeq(config->groupKey, SM_OCTETS_16, 0x30);

    SmGetAlgoFuncs()->randNumFunc = Test_CryptoRandNumGenerate;
    SmSendImgMsg(config);
    EXPECT_EQ(g_imgMsgCbkCnt, 1);
    EXPECT_EQ(g_imgMsgCbkStatus, SM_IMG_OK);

    g_imgMsgCbkCnt = 0;
    FillSeq(config->groupKey, SM_OCTETS_16, 0x30); // 上一腿产品清零了输出缓冲，重新填
    SmGetAlgoFuncs()->randNumFunc = nullptr; // Kg 随机数失败
    SmSendImgMsg(config);
    EXPECT_EQ(g_imgMsgCbkCnt, 1);
    EXPECT_EQ(g_imgMsgCbkStatus, SM_IMG_ERROR);

    g_imgMsgCbkCnt = 0;
    FillSeq(config->groupKey, SM_OCTETS_16, 0x30);
    SmGetAlgoFuncs()->randNumFunc = Test_CryptoRandNumGenerate;
    ExpectAllocFailOn(sdfMemMock, 1); // 材料齐全但发送缓冲分配失败，链路随之被拆除
    SmSendImgMsg(config);
    EXPECT_EQ(g_imgMsgCbkCnt, 1);
    EXPECT_EQ(g_imgMsgCbkStatus, SM_IMG_ERROR);
    SDF_MemFree(config);
}

TEST_F(UT_SM_STMSUPP_Test, ImgEnableAlgoMappingLegs)
{
    // 组播加密使能：算法编号大于零时减一映射，等于零时直用零号算法
    NLSTK_SmImgEncpParam_S enable = {0};
    uint8_t capturedAlgo[2] = {0};
    int idx = 0;

    enable.imgHandle = 1;
    enable.cryptoAlgo = 3;
    EXPECT_CALL(dliMock, DLI_EnableIMGEncryption)
        .WillOnce(Invoke([&](DLI_IMGEncryptParam *param) {
            capturedAlgo[idx++] = param->algo;
            return 0;
        }))
        .RetiresOnSaturation();
    SmEnableImgEncryption(&enable);
    EXPECT_EQ(capturedAlgo[0], 2);

    enable.cryptoAlgo = 0;
    EXPECT_CALL(dliMock, DLI_EnableIMGEncryption)
        .WillOnce(Invoke([&](DLI_IMGEncryptParam *param) {
            capturedAlgo[idx++] = param->algo;
            return 0;
        }))
        .RetiresOnSaturation();
    SmEnableImgEncryption(&enable);
    EXPECT_EQ(capturedAlgo[1], 0);
}
