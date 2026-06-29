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

#include <cstring>
#include <stdlib.h>
#include "securec.h"
#include "gtest/gtest.h" 

#include "sm_stub_common.h"
#include "nlstk_sm_api.h"
#include "dli_event_struct.h"
#include "nlstk_sm.h"
#include "sle_crypto.h"
#include "nlstk_log.h"
#include "sdf_string.h"
#include "sm_algos.h"

#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dli_mock.h"
#include "stack_dli_stub.h"
#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cfgdb_mock.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static DLI_ControllerDataEvt *PassCode_FillPairingReqRsqMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PassCode_FillPairingCfmMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PassCode_FillPairingInitInfoMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PassCode_FillAuthGCfmWithRaMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PassCode_FillAuthTCfmWithRbMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PassCode_FillAuthDhKeyGMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PassCode_FillAuthDhKeyTMsg(uint16_t connHandle, uint16_t ctrlDataIndex);

class SM_PassCode_Test : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DliMock> dliMock;
    NiceMock<CfgdbMock> cfgdbMock;
protected:
    void SetUp() override
    {
        // 步骤1：初始化桩函数
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);

        TEST_DLI_Init();
        EXPECT_CALL(dliMock, DLI_CmdCbkReg).WillRepeatedly(DLI_CmdCbkRegStub);
        EXPECT_CALL(dliMock, DLI_CmdCbkUnReg).WillRepeatedly(TEST_DLI_CmdCbkUnReg);

        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);

        SmInit();
        SmEnable();
    }

    void TearDown() override
    {
        SmDeInit();
    }
};

TEST_F(SM_PassCode_Test, TEST_SM_PASSCODE_001)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_MAX;

    NLSTK_SmLocalParams_S params = {
        .ioAbility = 0x02,
        .authMethodMask = 0xFF,
        .oobDataFlag = 0,
        .secKeyMaxLen = 16,
        .distIrkFlag = 0,
        .distAddrFlag = 0,
        .codeAlgoCap = {2, 2, 2, 2},
        .pskFlag = 0,
        .authReq = { .secAttribute  = 1, .mitmDefend = 1, .kpressNotif = 0 },
    };
    ret = NLSTK_SmSetSecurityParams(&params);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    NLSTK_SmCryptoAlgoFuncs_S algoFuncs = {
        .randNumFunc = Crypto_RandNumGenerateStub,
        .pubPriKeyFunc = Crypto_PubPriKeyPairGenerateStub,
        .secKeyFunc = Crypto_SecKeyGenerateStub,
        .derivedKeyFunc = Crypto_DerivedKeyGenerateStub,
    };
    ret = NLSTK_SmRegAlgoFuncs(&algoFuncs);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    EXPECT_CALL(cfgdbMock, NLSTK_CfgdbGetPublicAddress).WillRepeatedly(NLSTK_CfgdbGetPublicAddressForTStub);
    EXPECT_CALL(cmMock, CM_GetLogicLinkByAddr).WillRepeatedly(CM_GetLogicLinkByAddrForTStub);
    EXPECT_CALL(cmMock, CM_GetLogicLinkByLcid).WillRepeatedly(CM_GetLogicLinkByLcidForTStub);

    ret = NLSTK_SmStartPairing(&g_gAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    SLE_Addr_S addr = { .type = PUBLIC_ADDRESS, .addr = { 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD } };
    SmSLink_S *slink = SmSLinkCtor(&addr);
    slink->gNode.ioAbility = 0x00;
    slink->tNode.ioAbility = 0x02;
    SmPassCodeAuthStart(slink);
    EXPECT_EQ(slink->expectOpCode, SM_AUTH_T_NODE_CFM_WITH_RB);
    SmSLinkDtor(slink);
}

TEST_F(SM_PassCode_Test, TEST_SM_PASSCODE_002)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_MAX;

    NLSTK_SmLocalParams_S params = {
        .ioAbility = 0x02,
        .authMethodMask = 0xFF,
        .oobDataFlag = 0,
        .secKeyMaxLen = 16,
        .distIrkFlag = 0,
        .distAddrFlag = 0,
        .codeAlgoCap = {2, 2, 2, 2},
        .pskFlag = 0,
        .authReq = { .secAttribute  = 1, .mitmDefend = 1, .kpressNotif = 0 },
    };
    ret = NLSTK_SmSetSecurityParams(&params);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    NLSTK_SmCryptoAlgoFuncs_S algoFuncs = {
        .randNumFunc = Crypto_RandNumGenerateStub,
        .pubPriKeyFunc = Crypto_PubPriKeyPairGenerateStub,
        .secKeyFunc = Crypto_SecKeyGenerateStub,
        .derivedKeyFunc = Crypto_DerivedKeyGenerateStub,
    };
    ret = NLSTK_SmRegAlgoFuncs(&algoFuncs);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    EXPECT_CALL(cfgdbMock, NLSTK_CfgdbGetPublicAddress).WillRepeatedly(NLSTK_CfgdbGetPublicAddressForGStub);
    EXPECT_CALL(cmMock, CM_GetLogicLinkByAddr).WillRepeatedly(CM_GetLogicLinkByAddrForGStub);
    EXPECT_CALL(cmMock, CM_GetLogicLinkByLcid).WillRepeatedly(CM_GetLogicLinkByLcidForGStub);

    ret = NLSTK_SmStartPairing(&g_tAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    SLE_Addr_S tAddr = { .type = PUBLIC_ADDRESS, .addr = { 0x23, 0x45, 0x56, 0x78, 0xAB, 0xCD } };
    SmSLink_S *tSlink = SmSLinkCtor(&tAddr);
    tSlink->gNode.ioAbility = 0x02;
    tSlink->tNode.ioAbility = 0x00;
    tSlink->role = SM_T_NODE;
    SmPassCodeAuthStart(tSlink);
    EXPECT_EQ(tSlink->expectOpCode, SM_AUTH_G_NODE_CFM_WITH_RA);
    SmSLinkDtor(tSlink);
}

/*****************************************************************************************
                                    Stub Functions
*****************************************************************************************/

bool SmSLinkWaitExpectOpCode(SmSLink_S *slink, uint16_t expectOpCode, time_t timeout)
{
    SDF_UNUSED(timeout);
    slink->expectOpCode = expectOpCode;

    DLI_ControllerDataEvt *buff = NULL;
    switch (expectOpCode) {
        case SM_NEGO_PAIRING_REQUEST:
            buff = PassCode_FillPairingReqRsqMsg(slink->lcid, SM_NEGO_PAIRING_REQUEST);
            break;
        case SM_NEGO_PAIRING_RESPONSE:
            buff = PassCode_FillPairingReqRsqMsg(slink->lcid, SM_NEGO_PAIRING_RESPONSE);
            break;
        case SM_NEGO_PAIRING_CONFIRM:
            buff = PassCode_FillPairingCfmMsg(slink->lcid, SM_NEGO_PAIRING_CONFIRM);
            break;
        case SM_NEGO_PAIRING_INIT_INFO:
            buff = PassCode_FillPairingInitInfoMsg(slink->lcid, SM_NEGO_PAIRING_INIT_INFO);
            break;
        case SM_AUTH_G_NODE_CFM_WITH_RA:
            buff = PassCode_FillAuthGCfmWithRaMsg(slink->lcid, SM_AUTH_G_NODE_CFM_WITH_RA);
            break;
        case SM_AUTH_T_NODE_CFM_WITH_RB:
            buff = PassCode_FillAuthTCfmWithRbMsg(slink->lcid, SM_AUTH_T_NODE_CFM_WITH_RB);
            break;
        case SM_AUTH_G_NODE_DHKEY:
            buff = PassCode_FillAuthDhKeyGMsg(slink->lcid, SM_AUTH_G_NODE_DHKEY);
            break;
        case SM_AUTH_T_NODE_DHKEY:
            buff = PassCode_FillAuthDhKeyTMsg(slink->lcid, SM_AUTH_T_NODE_DHKEY);
            break;
        default:
            return true;
    }
    
    if (buff != NULL) {
        SendBuffer(buff);
        SDF_MemFree(buff);
    }
    return true;
}

bool SmSLinkWaitUapiInput(SmSLink_S *slink, time_t timeout)
{
    NLSTK_SmPassCode_S passCode = {{0}};
    (void)memcpy_s(&passCode.addr, sizeof(SLE_Addr_S), &slink->rmtAddr, sizeof(SLE_Addr_S));
    passCode.passCode = 100016;
    NLSTK_SmSetPassCode(&passCode);
    return true;
}

static DLI_ControllerDataEvt *PassCode_FillPairingReqRsqMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmPairReqRspMsg_S msg = {
        .ioAbility = 0,
        .oobDataFlag = 0,
        .authReq = { .secAttribute  = 1, .mitmDefend = 1, .kpressNotif = 0 },
        .secKeyMaxLen = 16,
        .secInfoDis = 0,
        .codeAlgoCap = {2, 2, 2, 2},
        .pskFlag = 0,
    };
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)SDF_MemAlloc(
        sizeof(DLI_ControllerDataEvt) + sizeof(SmPairReqRspMsg_S));
    if (buff == NULL) {
        NLSTK_LOG_ERROR("[SM] Mem alloc error.");
        return NULL;
    }
    buff->connHandle = connHandle;
    buff->ctrlDataIndex = ctrlDataIndex;
    buff->len = sizeof(SmPairReqRspMsg_S);
    (void)memcpy_s(buff->data, buff->len, &msg, sizeof(SmPairReqRspMsg_S));
    return buff;
}

static DLI_ControllerDataEvt *PassCode_FillPairingCfmMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    NLSTK_SmKeyPair_S keyPair = {
        .algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2,
    };
    SmGenPubPriKey(&keyPair);
    SmPairCfmMsg_S msg = {
        .secKeyLen = 16,
        .authMethod = SM_AUTH_PASSCODE,
        .codeAlgoCap = {2, 2, 2, 2},
    };
    (void)memcpy_s(msg.gNodePubKey, SM_PUBLIC_KEY_LEN, keyPair.localPubKey, SM_PUBLIC_KEY_LEN);
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)SDF_MemAlloc(
        sizeof(DLI_ControllerDataEvt) + sizeof(SmPairCfmMsg_S));
    if (buff == NULL) {
        NLSTK_LOG_ERROR("[SM] Mem alloc error.");
        return NULL;
    }
    buff->connHandle = connHandle;
    buff->ctrlDataIndex = ctrlDataIndex;
    buff->len = sizeof(SmPairCfmMsg_S);
    (void)memcpy_s(buff->data, buff->len, &msg, sizeof(SmPairCfmMsg_S));
    return buff;
}

static DLI_ControllerDataEvt *PassCode_FillPairingInitInfoMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    NLSTK_SmKeyPair_S keyPair = {
        .algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2,
    };
    SmGenPubPriKey(&keyPair);
    SmPairInitInfoMsg_S msg = {0};
    (void)memcpy_s(msg.tNodePubKey, SM_PUBLIC_KEY_LEN, keyPair.localPubKey, SM_PUBLIC_KEY_LEN);
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)SDF_MemAlloc(
        sizeof(DLI_ControllerDataEvt) + sizeof(SmPairInitInfoMsg_S));
    if (buff == NULL) {
        NLSTK_LOG_ERROR("[SM] Mem alloc error.");
        return NULL;
    }
    buff->connHandle = connHandle;
    buff->ctrlDataIndex = ctrlDataIndex;
    buff->len = sizeof(SmPairInitInfoMsg_S);
    (void)memcpy_s(buff->data, buff->len, &msg, sizeof(SmPairInitInfoMsg_S));
    return buff;
}

static DLI_ControllerDataEvt *PassCode_FillAuthGCfmWithRaMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmAuthGCfmWithRaMsg_S msg = {{0}};
    (void)memset_s(msg.gNodeCfm, SM_CONFIRM_NUMBER_LEN, 0, SM_CONFIRM_NUMBER_LEN);
    for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
        msg.randomA[i] = i;
    }
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)SDF_MemAlloc(
        sizeof(DLI_ControllerDataEvt) + sizeof(SmAuthGCfmWithRaMsg_S));
    if (buff == NULL) {
        NLSTK_LOG_ERROR("[SM] Mem alloc error.");
        return NULL;
    }
    buff->connHandle = connHandle;
    buff->ctrlDataIndex = ctrlDataIndex;
    buff->len = sizeof(SmAuthGCfmWithRaMsg_S);
    (void)memcpy_s(buff->data, buff->len, &msg, sizeof(SmAuthGCfmWithRaMsg_S));
    return buff;
}

static DLI_ControllerDataEvt *PassCode_FillAuthTCfmWithRbMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmAuthTCfmWithRbMsg_S msg = {{0}};
    (void)memset_s(msg.tNodeCfm, SM_CONFIRM_NUMBER_LEN, 0, SM_CONFIRM_NUMBER_LEN);
    for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
        msg.randomB[i] = i;
    }
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)SDF_MemAlloc(
        sizeof(DLI_ControllerDataEvt) + sizeof(SmAuthTCfmWithRbMsg_S));
    if (buff == NULL) {
        NLSTK_LOG_ERROR("[SM] Mem alloc error.");
        return NULL;
    }
    buff->connHandle = connHandle;
    buff->ctrlDataIndex = ctrlDataIndex;
    buff->len = sizeof(SmAuthTCfmWithRbMsg_S);
    (void)memcpy_s(buff->data, buff->len, &msg, sizeof(SmAuthTCfmWithRbMsg_S));
    return buff;
}

static DLI_ControllerDataEvt *PassCode_FillAuthDhKeyGMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmAuthDhkeyMsg_S msg = {0};
    uint8_t dhkeyCode[SM_DHKEY_AUTHCODE_LEN] = {0};
    (void)memcpy_s(msg.authData, SM_DHKEY_AUTHCODE_LEN, dhkeyCode, SM_DHKEY_AUTHCODE_LEN);
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)SDF_MemAlloc(
        sizeof(DLI_ControllerDataEvt) + sizeof(SmAuthDhkeyMsg_S));
    if (buff == NULL) {
        NLSTK_LOG_ERROR("[SM] Mem alloc error.");
        return NULL;
    }
    buff->connHandle = connHandle;
    buff->ctrlDataIndex = ctrlDataIndex;
    buff->len = sizeof(SmAuthDhkeyMsg_S);
    (void)memcpy_s(buff->data, buff->len, &msg, sizeof(SmAuthDhkeyMsg_S));
    return buff;
}

static DLI_ControllerDataEvt *PassCode_FillAuthDhKeyTMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmAuthDhkeyMsg_S msg = {0};
    uint8_t dhkeyCode[SM_DHKEY_AUTHCODE_LEN] = {0};
    (void)memcpy_s(msg.authData, SM_DHKEY_AUTHCODE_LEN, dhkeyCode, SM_DHKEY_AUTHCODE_LEN);
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)SDF_MemAlloc(
        sizeof(DLI_ControllerDataEvt) + sizeof(SmAuthDhkeyMsg_S));
    if (buff == NULL) {
        NLSTK_LOG_ERROR("[SM] Mem alloc error.");
        return NULL;
    }
    buff->connHandle = connHandle;
    buff->ctrlDataIndex = ctrlDataIndex;
    buff->len = sizeof(SmAuthDhkeyMsg_S);
    (void)memcpy_s(buff->data, buff->len, &msg, sizeof(SmAuthDhkeyMsg_S));
    return buff;
}