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
#include "sm_noentry.h"
#include "sm_passcode.h"
#include "sm_password.h"
#include "sm_oob.h"
#include "sm_psk.h"
#include "sm_numcmp.h"
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
static uint8_t g_authCbkLinkKey[SM_LINK_KEY_LEN];
static uint32_t g_requestCbkCnt;
static uint32_t g_removeCbkCnt;

static void Test_AuthCbkCapture(NLSTK_SmAuthComplete_S *params)
{
    g_authCbkCnt++;
    g_authCbkStatus = params->authStatus;
    (void)memcpy_s(g_authCbkLinkKey, SM_LINK_KEY_LEN, params->linkKey, SM_LINK_KEY_LEN);
}

static void Test_RequestCbkCapture(NLSTK_SmPairingRequest_S *params)
{
    g_requestCbkCnt++;
}

static void Test_RemoveCbkCapture(NLSTK_SmPairingRemove_S *params)
{
    g_removeCbkCnt++;
}

static void Test_PairStartCbkCapture(NLSTK_SmPairingStart_S *params) {}
static void Test_EncCbkCapture(NLSTK_SmEncComplete_S *params) {}
static void Test_ImgMsgCbkCapture(NLSTK_SmSendImgMsgCmpl_S *params) {}

static void ResetAuthCapture(void)
{
    g_authCbkCnt = 0;
    g_authCbkStatus = 0xFF;
    (void)memset_s(g_authCbkLinkKey, SM_LINK_KEY_LEN, 0, SM_LINK_KEY_LEN);
    g_requestCbkCnt = 0;
    g_removeCbkCnt = 0;
}

class UT_SM_AUTHFLOW_Test : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DliMock> dliMock;
    NiceMock<SdfMemMock> sdfMemMock;
protected:
    NLSTK_SmCryptoAlgoFuncs_S savedAlgos;

    virtual void SetUp()
    {
        ResetAuthCapture();
        ON_CALL(sdfMemMock, SDF_MemZalloc).WillByDefault(TEST_SDF_MemZalloc);
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

/* 构造不入 g_slinkMap 的 T 节点链路：公私钥用真实算法生成，保证 DH 计算可用 */
static SmSLink_S *MakeTNodeSlink(void)
{
    SLE_Addr_S addr = g_rmtAddr;
    SmSLink_S *slink = SmSLinkCtor(&addr);
    if (slink == nullptr) {
        return nullptr;
    }
    slink->role = SM_T_NODE;
    slink->negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
    slink->negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;

    NLSTK_SmKeyPair_S keyPair = {.algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2};
    if (!Crypto_PubPriKeyPairGenerate(&keyPair)) {
        SmSLinkDtor(slink);
        return nullptr;
    }
    (void)memcpy_s(slink->priKey, SM_PRIVATE_KEY_LEN, keyPair.priKey, SM_PRIVATE_KEY_LEN);
    (void)memcpy_s(slink->gNode.pubKey, SM_PUBLIC_KEY_LEN, keyPair.localPubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(slink->tNode.pubKey, SM_PUBLIC_KEY_LEN, keyPair.localPubKey, SM_PUBLIC_KEY_LEN);
    return slink;
}

/* 不触发 Entry 副作用，把状态机当前态指到 AUTH 并同步状态索引，让错误消息走 AuthDispatch */
static State *EnterAuthState(SmSLink_S *slink)
{
    State *authState = slink->stm->base.states_->next->next->next->next;
    slink->stm->base.current_ = authState;
    slink->curStateIndex = SM_STATE_AUTH;
    return authState;
}

/* 通行码确认码 = CMAC(键=G随机数, 数据=G公钥||T公钥||T混淆码) */
static bool MakePasscodeConfirm(const uint8_t *ra, const SmSLink_S *slink, uint8_t *confirm)
{
    uint8_t buff[SM_PUBLIC_KEY_LEN * 2 + SM_MIX_PASSCODE_LEN];
    (void)memcpy_s(buff, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(buff + SM_PUBLIC_KEY_LEN, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(buff + SM_PUBLIC_KEY_LEN * 2, SM_MIX_PASSCODE_LEN, slink->tNode.mixCode, SM_MIX_PASSCODE_LEN);
    NLSTK_SmDerivedMac_S input = {
        .algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2,
        .buff = buff,
        .buffSize = sizeof(buff),
    };
    (void)memcpy_s(input.key, SM_OCTETS_16, ra, SM_OCTETS_16);
    return SmCmacGenerate(&input, confirm, SM_OCTETS_16);
}

/* 口令确认码 = CMAC(键=G随机数, 数据=G公钥||T公钥||T混淆口令) */
static bool MakePasswordConfirm(const uint8_t *key, const SmSLink_S *slink, const uint8_t *mixWord,
    uint8_t *confirm)
{
    uint8_t buff[SM_PUBLIC_KEY_LEN * 2 + SM_MIX_PASSWORD_LEN];
    (void)memcpy_s(buff, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(buff + SM_PUBLIC_KEY_LEN, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(buff + SM_PUBLIC_KEY_LEN * 2, SM_MIX_PASSWORD_LEN, mixWord, SM_MIX_PASSWORD_LEN);
    NLSTK_SmDerivedMac_S input = {
        .algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2,
        .buff = buff,
        .buffSize = sizeof(buff),
    };
    (void)memcpy_s(input.key, SM_OCTETS_16, key, SM_OCTETS_16);
    return SmCmacGenerate(&input, confirm, SM_OCTETS_16);
}

/* 带外确认码 = CMAC(键=G随机数, 数据=G公钥||T公钥)，不掺混淆码 */
static bool MakeOobConfirm(const uint8_t *key, const SmSLink_S *slink, uint8_t *confirm)
{
    uint8_t buff[SM_PUBLIC_KEY_LEN * 2];
    (void)memcpy_s(buff, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(buff + SM_PUBLIC_KEY_LEN, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    NLSTK_SmDerivedMac_S input = {
        .algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2,
        .buff = buff,
        .buffSize = sizeof(buff),
    };
    (void)memcpy_s(input.key, SM_OCTETS_16, key, SM_OCTETS_16);
    return SmCmacGenerate(&input, confirm, SM_OCTETS_16);
}

/* PSK 确认码 = CMAC(键=预共享密钥前16字节, 数据=G公钥||T公钥||G随机数) */
static bool MakePskConfirm(const uint8_t *psk, const SmSLink_S *slink, const uint8_t *randomNum,
    uint8_t *confirm)
{
    uint8_t buff[SM_PUBLIC_KEY_LEN * 2 + SM_RANDOM_NUMBER_R_LEN];
    (void)memcpy_s(buff, SM_PUBLIC_KEY_LEN, slink->gNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(buff + SM_PUBLIC_KEY_LEN, SM_PUBLIC_KEY_LEN, slink->tNode.pubKey, SM_PUBLIC_KEY_LEN);
    (void)memcpy_s(buff + SM_PUBLIC_KEY_LEN * 2, SM_RANDOM_NUMBER_R_LEN, randomNum, SM_RANDOM_NUMBER_R_LEN);
    NLSTK_SmDerivedMac_S input = {
        .algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2,
        .buff = buff,
        .buffSize = sizeof(buff),
    };
    (void)memcpy_s(input.key, SM_OCTETS_16, psk, SM_OCTETS_16);
    return SmCmacGenerate(&input, confirm, SM_OCTETS_16);
}

static void FillSeq(uint8_t *buff, uint8_t len, uint8_t seed)
{
    for (uint8_t i = 0; i < len; i++) {
        buff[i] = seed + i;
    }
}

/*****************************************************************************************
                                    Test Functions
*****************************************************************************************/

TEST_F(UT_SM_AUTHFLOW_Test, PasscodeCfmAcceptAndReject)
{
    // 真实算法下 T 节点校验 G 确认码：合法则回 Rb 确认并等待 DHKEY，翻转一位则中止并只报错误回调
    RegisterRealAlgos();
    uint8_t ra[SM_RANDOM_NUMBER_R_LEN];
    uint8_t mixCode[SM_MIX_PASSCODE_LEN];
    FillSeq(ra, sizeof(ra), 0x10);
    FillSeq(mixCode, sizeof(mixCode), 0x00);

    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    (void)memcpy_s(slink->tNode.mixCode, SM_MIX_PASSCODE_LEN, mixCode, SM_MIX_PASSCODE_LEN);
    (void)EnterAuthState(slink);

    SmAuthGCfmWithRaMsg_S msg = {};
    ASSERT_TRUE(MakePasscodeConfirm(ra, slink, msg.gNodeCfm));  // braces not needed: gNodeCfm is array (decays)
    (void)memcpy_s(msg.randomA, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);

    SmPassCodeAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink, (const uint8_t *)&msg, sizeof(msg) - 1);
    EXPECT_EQ(slink->tNode.recvFlag, false);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_AUTH);

    SmPassCodeAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(slink->curStateIndex, SM_STATE_AUTH);
    EXPECT_EQ(slink->expectOpCode, SM_AUTH_G_NODE_DHKEY);
    EXPECT_EQ(slink->tNode.recvFlag, false);
    EXPECT_EQ(g_authCbkCnt, 0);
    SmSLinkDtor(slink);

    msg.gNodeCfm[3] ^= 0x01; // 翻转一位确认码
    SmSLink_S *reject = MakeTNodeSlink();
    ASSERT_NE(reject, nullptr);
    (void)memcpy_s(reject->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    (void)memcpy_s(reject->tNode.mixCode, SM_MIX_PASSCODE_LEN, mixCode, SM_MIX_PASSCODE_LEN);
    (void)EnterAuthState(reject);
    SmPassCodeAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, reject, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(reject->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_authCbkCnt, 1);
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_ERROR);
    EXPECT_EQ(g_authCbkLinkKey[0], 0);
    EXPECT_EQ(g_authCbkLinkKey[SM_LINK_KEY_LEN - 1], 0);
    EXPECT_EQ(g_removeCbkCnt, 1); // 拆除完成回调恰一次
    SmSLinkDtor(reject);
}

TEST_F(UT_SM_AUTHFLOW_Test, PasscodeRecvPendingWhenUserNotInput)
{
    // 北向输入定时器未释放时收到 G 确认码：只挂起置位接收标志，不校验也不发送
    RegisterRealAlgos();
    uint8_t ra[SM_RANDOM_NUMBER_R_LEN];
    uint8_t mixCode[SM_MIX_PASSCODE_LEN];
    FillSeq(ra, sizeof(ra), 0x20);
    FillSeq(mixCode, sizeof(mixCode), 0x00);

    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    (void)memcpy_s(slink->tNode.mixCode, SM_MIX_PASSCODE_LEN, mixCode, SM_MIX_PASSCODE_LEN);
    (void)EnterAuthState(slink);
    slink->uapiTimerHandle = 7; // 模拟北向输入定时器仍在计时

    SmAuthGCfmWithRaMsg_S msg = {};
    ASSERT_TRUE(MakePasscodeConfirm(ra, slink, msg.gNodeCfm));  // braces not needed: gNodeCfm is array (decays)
    (void)memcpy_s(msg.randomA, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    SmPassCodeAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(slink->tNode.recvFlag, true);
    EXPECT_EQ(slink->expectOpCode, 0);
    EXPECT_EQ(g_authCbkCnt, 0);
    SmSLinkDtor(slink);
}

TEST_F(UT_SM_AUTHFLOW_Test, PasscodeSetTPassCodeConsumesRecvFlag)
{
    // 用户后输入口令时消费先到的接收标志：确认码不一致则中止，接收标志必须复位
    RegisterRealAlgos();
    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)EnterAuthState(slink);
    slink->tNode.passCode = 654321; // 合法六位数字
    slink->tNode.recvFlag = true;
    FillSeq(slink->gNode.confirm, SM_CONFIRM_NUMBER_LEN, 0x30); // 与本地计算必然不一致
    SmPassCodeSetTPassCode(slink);
    EXPECT_EQ(slink->tNode.recvFlag, false);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_ERROR);
    EXPECT_EQ(g_removeCbkCnt, 1);

    slink->tNode.recvFlag = false; // 未提前收到确认码时补输入只生成材料，不发不迁移
    SmPassCodeSetTPassCode(slink);
    EXPECT_EQ(slink->tNode.recvFlag, false);
    SmSLinkDtor(slink);
}

TEST_F(UT_SM_AUTHFLOW_Test, PasscodePatternSelectAndInvalid)
{
    // 输入输出能力组合决定谁显示谁输入：两种合法组合走等待输入分支，非法组合直接中止
    RegisterRealAlgos();
    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)EnterAuthState(slink);

    slink->negoParams.authMethod = SM_AUTH_PASSCODE;
    slink->gNode.ioAbility = SM_IO_KEYBOARD_ONLY; // T 显示 G 输入
    slink->tNode.ioAbility = SM_IO_NO_INPUT_AND_OUTPUT;
    slink->role = SM_G_NODE;
    SmPassCodeAuthStart(slink);
    EXPECT_EQ(g_requestCbkCnt, 1);
    EXPECT_NE(slink->uapiTimerHandle, TIMER_NO_USED_VALUE);

    slink->tNode.ioAbility = SM_IO_KEYBOARD_ONLY; // G 显示 T 输入
    slink->gNode.ioAbility = SM_IO_NO_INPUT_AND_OUTPUT;
    slink->role = SM_T_NODE;
    slink->uapiTimerHandle = TIMER_NO_USED_VALUE;
    slink->timerHandle = TIMER_NO_USED_VALUE;
    g_requestCbkCnt = 0;
    SmPassCodeAuthStart(slink);
    EXPECT_EQ(g_requestCbkCnt, 1);
    EXPECT_NE(slink->uapiTimerHandle, TIMER_NO_USED_VALUE);

    slink->gNode.ioAbility = SM_IO_NO_INPUT_AND_OUTPUT; // 两端都无法输入显示
    slink->tNode.ioAbility = SM_IO_NO_INPUT_AND_OUTPUT;
    slink->uapiTimerHandle = TIMER_NO_USED_VALUE;
    slink->timerHandle = TIMER_NO_USED_VALUE;
    SmPassCodeAuthStart(slink);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_authCbkCnt, 1);
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_ERROR);
    EXPECT_EQ(g_removeCbkCnt, 1);
    SmSLinkDtor(slink);
}

TEST_F(UT_SM_AUTHFLOW_Test, PasscodeSixDigitGenFailAborts)
{
    // 随机数算法不可用时六位数字生成为零，T 侧启动直接中止进入拆除
    RegisterRealAlgos();
    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)EnterAuthState(slink);

    slink->negoParams.authMethod = SM_AUTH_PASSCODE;
    slink->gNode.ioAbility = SM_IO_KEYBOARD_ONLY; // T 显示 G 输入组合，T 节点要生成六位数字
    slink->tNode.ioAbility = SM_IO_NO_INPUT_AND_OUTPUT;
    slink->role = SM_T_NODE;
    SmGetAlgoFuncs()->randNumFunc = nullptr;
    SmPassCodeAuthStart(slink);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_removeCbkCnt, 1);
    SmSLinkDtor(slink);
}

TEST_F(UT_SM_AUTHFLOW_Test, OobCfmAcceptAndReject)
{
    // 真实算法下 T 节点校验带外确认码：合法则回 Rb 确认并等待 DHKEY，篡改则中止
    RegisterRealAlgos();
    uint8_t ra[SM_RANDOM_NUMBER_R_LEN];
    FillSeq(ra, sizeof(ra), 0x40);

    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    (void)EnterAuthState(slink);

    SmAuthGCfmWithRaMsg_S msg = {};
    ASSERT_TRUE(MakeOobConfirm(ra, slink, msg.gNodeCfm));
    (void)memcpy_s(msg.randomA, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);

    SmOobAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink, (const uint8_t *)&msg, sizeof(msg) - 1);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_AUTH);

    SmOobAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(slink->curStateIndex, SM_STATE_AUTH);
    EXPECT_EQ(slink->expectOpCode, SM_AUTH_G_NODE_DHKEY);
    EXPECT_EQ(g_authCbkCnt, 0);
    SmSLinkDtor(slink);

    msg.gNodeCfm[0] ^= 0x80;
    SmSLink_S *reject = MakeTNodeSlink();
    ASSERT_NE(reject, nullptr);
    (void)memcpy_s(reject->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    (void)EnterAuthState(reject);
    SmOobAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, reject, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(reject->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_authCbkCnt, 1);
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_ERROR);
    EXPECT_EQ(g_removeCbkCnt, 1);
    SmSLinkDtor(reject);
}

TEST_F(UT_SM_AUTHFLOW_Test, PskCfmAcceptAndReject)
{
    // 真实算法下 T 节点用预共享密钥校验 G 确认码：合法则回确认并等待 DHKEY，篡改则中止
    RegisterRealAlgos();
    uint8_t ra[SM_RANDOM_NUMBER_R_LEN];
    FillSeq(ra, sizeof(ra), 0x50);

    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    FillSeq(slink->psk, SM_PSK_SEC_KEY_LEN, 0x60);
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    (void)EnterAuthState(slink);

    SmAuthGCfmMsg_S msg = {0};
    ASSERT_TRUE(MakePskConfirm(slink->psk, slink, slink->gNode.randomR, msg.authData));
    SmPskAuthPkgDispatcher(SM_AUTH_G_NODE_CFM, slink, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(slink->curStateIndex, SM_STATE_AUTH);
    EXPECT_EQ(slink->expectOpCode, SM_AUTH_G_NODE_DHKEY);
    EXPECT_EQ(g_authCbkCnt, 0);
    SmSLinkDtor(slink);

    msg.authData[5] ^= 0x10;
    SmSLink_S *reject = MakeTNodeSlink();
    ASSERT_NE(reject, nullptr);
    FillSeq(reject->psk, SM_PSK_SEC_KEY_LEN, 0x60);
    (void)memcpy_s(reject->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    (void)EnterAuthState(reject);
    SmPskAuthPkgDispatcher(SM_AUTH_G_NODE_CFM, reject, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(reject->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_authCbkCnt, 1);
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_ERROR);
    EXPECT_EQ(g_removeCbkCnt, 1);
    SmSLinkDtor(reject);
}

TEST_F(UT_SM_AUTHFLOW_Test, PasswordCfmAcceptAndReject)
{
    // 真实算法下 T 节点校验口令确认码：北向定时器已释放则立即校验，合法回 Rb、篡改中止
    RegisterRealAlgos();
    uint8_t ra[SM_RANDOM_NUMBER_R_LEN];
    uint8_t mixWord[SM_MIX_PASSWORD_LEN];
    FillSeq(ra, sizeof(ra), 0x70);
    FillSeq(mixWord, sizeof(mixWord), 0x00);

    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    (void)memcpy_s(slink->tNode.mixWord, SM_MIX_PASSWORD_LEN, mixWord, SM_MIX_PASSWORD_LEN);
    (void)EnterAuthState(slink);

    SmAuthGCfmWithRaMsg_S msg = {};
    ASSERT_TRUE(MakePasswordConfirm(ra, slink, mixWord, msg.gNodeCfm));
    (void)memcpy_s(msg.randomA, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    SmPassWordAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(slink->curStateIndex, SM_STATE_AUTH);
    EXPECT_EQ(slink->expectOpCode, SM_AUTH_G_NODE_DHKEY);
    EXPECT_EQ(g_authCbkCnt, 0);
    SmSLinkDtor(slink);

    msg.gNodeCfm[7] ^= 0x04;
    SmSLink_S *reject = MakeTNodeSlink();
    ASSERT_NE(reject, nullptr);
    (void)memcpy_s(reject->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    (void)memcpy_s(reject->tNode.mixWord, SM_MIX_PASSWORD_LEN, mixWord, SM_MIX_PASSWORD_LEN);
    (void)EnterAuthState(reject);
    SmPassWordAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, reject, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(reject->curStateIndex, SM_STATE_REMV);
    EXPECT_EQ(g_authCbkCnt, 1);
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_ERROR);
    EXPECT_EQ(g_removeCbkCnt, 1);
    SmSLinkDtor(reject);
}

TEST_F(UT_SM_AUTHFLOW_Test, PasswordRecvPendingWhenUserNotInput)
{
    // 北向输入定时器未释放时收到 G 确认码：只置位接收标志等待用户输入
    RegisterRealAlgos();
    uint8_t ra[SM_RANDOM_NUMBER_R_LEN];
    uint8_t mixWord[SM_MIX_PASSWORD_LEN];
    FillSeq(ra, sizeof(ra), 0x80);
    FillSeq(mixWord, sizeof(mixWord), 0x00);

    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    (void)memcpy_s(slink->tNode.mixWord, SM_MIX_PASSWORD_LEN, mixWord, SM_MIX_PASSWORD_LEN);
    (void)EnterAuthState(slink);
    slink->uapiTimerHandle = 9;

    SmAuthGCfmWithRaMsg_S msg = {};
    ASSERT_TRUE(MakePasswordConfirm(ra, slink, mixWord, msg.gNodeCfm));
    (void)memcpy_s(msg.randomA, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);
    SmPassWordAuthPkgDispatcher(SM_AUTH_G_NODE_CFM_WITH_RA, slink, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(slink->tNode.recvFlag, true);
    EXPECT_EQ(slink->expectOpCode, 0);
    EXPECT_EQ(g_authCbkCnt, 0);
    SmSLinkDtor(slink);
}

TEST_F(UT_SM_AUTHFLOW_Test, PasswordSetTPassWordRecvFlagOrder)
{
    // 口令后输入时消费先到的接收标志：确认码按同一口令推导，合法则回 Rb 继续流程
    RegisterRealAlgos();
    uint8_t ra[SM_RANDOM_NUMBER_R_LEN];
    FillSeq(ra, sizeof(ra), 0x90);

    char wordVal[] = "1234";
    NLSTK_SmPassWord_S *passWord = (NLSTK_SmPassWord_S *)SDF_MemZalloc(sizeof(NLSTK_SmPassWord_S) +
        strlen(wordVal));
    ASSERT_NE(passWord, nullptr);
    passWord->passWordLen = strlen(wordVal);
    (void)memcpy_s(passWord->passWord, strlen(wordVal), wordVal, strlen(wordVal));

    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)EnterAuthState(slink);
    slink->tNode.recvFlag = true;
    (void)memcpy_s(slink->gNode.randomR, SM_RANDOM_NUMBER_R_LEN, ra, SM_RANDOM_NUMBER_R_LEN);

    // 先按产品公式算出混淆口令，再据此预置合法的 G 确认码
    NLSTK_SmDerivedMac_S mixInput = {
        .algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2,
        .buff = (uint8_t *)passWord->passWord,  // verify-fix: flex array to ptr
        .buffSize = passWord->passWordLen,
    };
    (void)memcpy_s(mixInput.key, SM_OCTETS_16, slink->gNode.pubKey, SM_OCTETS_16);
    uint8_t mixWord[SM_MIX_PASSWORD_LEN] = {0};
    ASSERT_TRUE(SmCmacGenerate(&mixInput, mixWord, SM_MIX_PASSWORD_LEN));
    ASSERT_TRUE(MakePasswordConfirm(ra, slink, mixWord, slink->gNode.confirm));

    slink->timerHandle = TIMER_NO_USED_VALUE;
    SmPassWordSetTPassWord(slink, passWord);
    EXPECT_EQ(slink->tNode.recvFlag, false);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_AUTH);
    EXPECT_EQ(slink->expectOpCode, SM_AUTH_G_NODE_DHKEY);
    SmSLinkDtor(slink);
    SDF_MemFree(passWord);
}

TEST_F(UT_SM_AUTHFLOW_Test, NumcmpDhkeyRecvLegs)
{
    // 数字比较鉴权收 DHKEY：北向定时器已释放则直接校验（校验失败即拆除），未释放则缓存报文
    RegisterRealAlgos();
    SmAuthDhkeyMsg_S msg = {0};
    FillSeq(msg.authData, SM_DHKEY_AUTHCODE_LEN, 0xA0);

    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)EnterAuthState(slink);
    slink->uapiTimerHandle = TIMER_NO_USED_VALUE;
    SmNumCmpAuthPkgDispatcher(SM_AUTH_G_NODE_DHKEY, slink, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV); // 校验码与本地计算不一致，中止
    EXPECT_EQ(g_authCbkStatus, SM_PAIR_ERROR);
    EXPECT_EQ(g_removeCbkCnt, 1);
    SmSLinkDtor(slink);

    SmSLink_S *pending = MakeTNodeSlink();
    ASSERT_NE(pending, nullptr);
    (void)EnterAuthState(pending);
    pending->uapiTimerHandle = 11;
    SmNumCmpAuthPkgDispatcher(SM_AUTH_G_NODE_DHKEY, pending, (const uint8_t *)&msg, 1);
    EXPECT_EQ(pending->tNode.recvFlag, true); // 长度不足的报文被拒收，只保留接收标志
    SmNumCmpAuthPkgDispatcher(SM_AUTH_G_NODE_DHKEY, pending, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(pending->tNode.recvFlag, true);
    EXPECT_EQ(pending->expectOpCode, 0);
    EXPECT_EQ(g_authCbkCnt, 1); // 计数不增加，仍只有上一腿的中止回调

    pending->uapiTimerHandle = TIMER_NO_USED_VALUE;
    pending->tNode.recvFlag = false;
    SmNumCmpContinueNumComparison(pending); // 未提前收包时补确认只算密钥，不消费缓存
    EXPECT_EQ(pending->tNode.recvFlag, false);
    EXPECT_EQ(pending->curStateIndex, SM_STATE_AUTH);
    EXPECT_EQ(g_removeCbkCnt, 1);
    SmSLinkDtor(pending);
}

TEST_F(UT_SM_AUTHFLOW_Test, NumcmpContinueConsumesCachedDhkey)
{
    // 数字比较鉴权用户后确认：缓存的对端 DHKEY 被消费，接收标志复位后按校验结果迁移
    RegisterRealAlgos();
    SmAuthDhkeyMsg_S msg = {0};
    FillSeq(msg.authData, SM_DHKEY_AUTHCODE_LEN, 0xB0);

    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)EnterAuthState(slink);
    slink->uapiTimerHandle = 12;
    SmNumCmpAuthPkgDispatcher(SM_AUTH_G_NODE_DHKEY, slink, (const uint8_t *)&msg, sizeof(msg));
    EXPECT_EQ(slink->tNode.recvFlag, true);

    g_authCbkCnt = 0;
    SmNumCmpContinueNumComparison(slink);
    EXPECT_EQ(slink->tNode.recvFlag, false); // 缓存被消费
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV); // 缓存校验码不一致，中止
    EXPECT_EQ(g_authCbkCnt, 1);
    SmSLinkDtor(slink);
}

TEST_F(UT_SM_AUTHFLOW_Test, NoentryContinueConsumesRecvFlag)
{
    // 免输入鉴权用户确认时消费先到的接收标志：标志复位后按缓存校验码迁移
    RegisterRealAlgos();
    SmSLink_S *slink = MakeTNodeSlink();
    ASSERT_NE(slink, nullptr);
    (void)EnterAuthState(slink);
    slink->negoParams.authMethod = SM_AUTH_NO_ENTRY;
    slink->tNode.recvFlag = true;
    g_authCbkCnt = 0;
    SmNoEntryContinueNoentry(slink);
    EXPECT_EQ(slink->tNode.recvFlag, false);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_REMV); // 缓存校验码不一致，中止
    EXPECT_EQ(g_authCbkCnt, 1);
    EXPECT_EQ(g_removeCbkCnt, 1);
    SmSLinkDtor(slink);
}

static void ProcessMessageNoop(struct tagStateMachine *stm, struct tagMessage msg)
{
    return;
}

TEST_F(UT_SM_AUTHFLOW_Test, DispatcherOpcodeRotationAndGuards)
{
    // 六条鉴权流的分发器：全量命令轮换、未知命令与截断报文都不引发异常迁移
    RegisterRealAlgos();
    SmSLink_S slink;
    SmStateMachine_S state;
    (void)memset_s(&slink, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    (void)memset_s(&state, sizeof(SmStateMachine_S), 0, sizeof(SmStateMachine_S));
    slink.stm = &state;
    state.base.ProcessMessage = ProcessMessageNoop;
    slink.negoParams.codeAlgoCap[SM_KEY_DERIV_ALGO_ABILITY] = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
    slink.negoParams.codeAlgoCap[SM_KEY_NEGO_ALGO_ABILITY] = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;

    uint8_t full[SM_PUBLIC_KEY_LEN * 2] = {0};
    const uint16_t passcodeOpcodes[] = {
        SM_AUTH_G_NODE_CFM_WITH_RA, SM_AUTH_T_NODE_CFM_WITH_RB, SM_AUTH_G_NODE_DHKEY, SM_AUTH_T_NODE_DHKEY,
        SM_PAIR_FAIL_MESSAGE_OPCODE,
    };
    for (size_t i = 0; i < sizeof(passcodeOpcodes) / sizeof(passcodeOpcodes[0]); i++) {
        SmPassCodeAuthPkgDispatcher(passcodeOpcodes[i], &slink, full, sizeof(SmAuthGCfmWithRaMsg_S));
        SmPassCodeAuthPkgDispatcher(passcodeOpcodes[i], &slink, full, 1);
        SmPassWordAuthPkgDispatcher(passcodeOpcodes[i], &slink, full, sizeof(SmAuthGCfmWithRaMsg_S));
        SmPassWordAuthPkgDispatcher(passcodeOpcodes[i], &slink, full, 1);
        SmOobAuthPkgDispatcher(passcodeOpcodes[i], &slink, full, sizeof(SmAuthGCfmWithRaMsg_S));
        SmOobAuthPkgDispatcher(passcodeOpcodes[i], &slink, full, 1);
    }
    const uint16_t pskOpcodes[] = {
        SM_AUTH_RAND_NUM_RA, SM_AUTH_RAND_NUM_RB, SM_AUTH_G_NODE_CFM, SM_AUTH_T_NODE_CFM,
        SM_AUTH_G_NODE_DHKEY, SM_AUTH_T_NODE_DHKEY, SM_PAIR_FAIL_MESSAGE_OPCODE,
    };
    for (size_t i = 0; i < sizeof(pskOpcodes) / sizeof(pskOpcodes[0]); i++) {
        SmPskAuthPkgDispatcher(pskOpcodes[i], &slink, full, sizeof(SmAuthRandomAMsg_S));
        SmPskAuthPkgDispatcher(pskOpcodes[i], &slink, full, 1);
        SmNumCmpAuthPkgDispatcher(pskOpcodes[i], &slink, full, sizeof(SmAuthRandomAMsg_S));
        SmNumCmpAuthPkgDispatcher(pskOpcodes[i], &slink, full, 1);
        SmNoEntryAuthPkgDispatcher(pskOpcodes[i], &slink, full, sizeof(SmAuthDhkeyMsg_S));
        SmNoEntryAuthPkgDispatcher(pskOpcodes[i], &slink, full, 1);
    }
    EXPECT_EQ(g_authCbkCnt, 0);
}
