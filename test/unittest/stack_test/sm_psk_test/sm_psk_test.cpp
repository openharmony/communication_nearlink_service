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

static DLI_ControllerDataEvt *PSK_FillPairingReqRsqMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PSK_FillPairingCfmMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PSK_FillPairingInitInfoMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PSK_FillAuthRandomAMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PSK_FillAuthRandomBMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PSK_FillAuthGCfmMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PSK_FillAuthTCfmMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PSK_FillAuthDhKeyGMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *PSK_FillAuthDhKeyTMsg(uint16_t connHandle, uint16_t ctrlDataIndex);

class SM_PSK_Test : public testing::Test {
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

TEST_F(SM_PSK_Test, TEST_SM_PSK_001)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_MAX;

    NLSTK_SmLocalParams_S params = {
        .ioAbility = 0x04,
        .authMethodMask = 0xFF,
        .oobDataFlag = 0,
        .secKeyMaxLen = 16,
        .distIrkFlag = 0,
        .distAddrFlag = 0,
        .codeAlgoCap = {2, 2, 2, 2},
        .pskFlag = 1,
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

    NLSTK_SmPsk_S pskT = {
        .addr = g_gAddr,
        .psk = {0},
    };
    ret = NLSTK_SmSetLocalPsk(&pskT);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    EXPECT_CALL(cfgdbMock, NLSTK_CfgdbGetPublicAddress).WillRepeatedly(NLSTK_CfgdbGetPublicAddressForTStub);
    EXPECT_CALL(cmMock, CM_GetLogicLinkByAddr).WillRepeatedly(CM_GetLogicLinkByAddrForTStub);
    EXPECT_CALL(cmMock, CM_GetLogicLinkByLcid).WillRepeatedly(CM_GetLogicLinkByLcidForTStub);

    ret = NLSTK_SmStartPairing(&g_gAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
}

TEST_F(SM_PSK_Test, TEST_SM_PSK_002)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_MAX;

    NLSTK_SmLocalParams_S params = {
        .ioAbility = 0x04,
        .authMethodMask = 0xFF,
        .oobDataFlag = 0,
        .secKeyMaxLen = 16,
        .distIrkFlag = 0,
        .distAddrFlag = 0,
        .codeAlgoCap = {2, 2, 2, 2},
        .pskFlag = 1,
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

    NLSTK_SmPsk_S pskG = {
        .addr = g_tAddr,
        .psk = {0},
    };
    ret = NLSTK_SmSetLocalPsk(&pskG);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    EXPECT_CALL(cfgdbMock, NLSTK_CfgdbGetPublicAddress).WillRepeatedly(NLSTK_CfgdbGetPublicAddressForGStub);
    EXPECT_CALL(cmMock, CM_GetLogicLinkByAddr).WillRepeatedly(CM_GetLogicLinkByAddrForGStub);
    EXPECT_CALL(cmMock, CM_GetLogicLinkByLcid).WillRepeatedly(CM_GetLogicLinkByLcidForGStub);

    ret = NLSTK_SmStartPairing(&g_tAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
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
            buff = PSK_FillPairingReqRsqMsg(slink->lcid, SM_NEGO_PAIRING_REQUEST);
            break;
        case SM_NEGO_PAIRING_RESPONSE:
            buff = PSK_FillPairingReqRsqMsg(slink->lcid, SM_NEGO_PAIRING_RESPONSE);
            break;
        case SM_NEGO_PAIRING_CONFIRM:
            buff = PSK_FillPairingCfmMsg(slink->lcid, SM_NEGO_PAIRING_CONFIRM);
            break;
        case SM_NEGO_PAIRING_INIT_INFO:
            buff = PSK_FillPairingInitInfoMsg(slink->lcid, SM_NEGO_PAIRING_INIT_INFO);
            break;
        case SM_AUTH_RAND_NUM_RA:
            buff = PSK_FillAuthRandomAMsg(slink->lcid, SM_AUTH_RAND_NUM_RA);
            break;
        case SM_AUTH_RAND_NUM_RB:
            buff = PSK_FillAuthRandomBMsg(slink->lcid, SM_AUTH_RAND_NUM_RB);
            break;
        case SM_AUTH_G_NODE_CFM:
            buff = PSK_FillAuthGCfmMsg(slink->lcid, SM_AUTH_G_NODE_CFM);
            break;
        case SM_AUTH_T_NODE_CFM:
            buff = PSK_FillAuthTCfmMsg(slink->lcid, SM_AUTH_T_NODE_CFM);
            break;
        case SM_AUTH_G_NODE_DHKEY:
            buff = PSK_FillAuthDhKeyGMsg(slink->lcid, SM_AUTH_G_NODE_DHKEY);
            break;
        case SM_AUTH_T_NODE_DHKEY:
            buff = PSK_FillAuthDhKeyTMsg(slink->lcid, SM_AUTH_T_NODE_DHKEY);
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
    NLSTK_CHECK_RETURN(slink->uapiTimerHandle == TIMER_NO_USED_VALUE,
                         false, "[SM] The requested timerHandle is in use.");
    SDF_TimerParam param = {
        .expires = timeout,
        .period = false,
        .callback = WaitTimeoutCbk,
        .args = slink,
    };
    return CP_TimerAdd(&slink->uapiTimerHandle, &param) == CP_OK;
}

static DLI_ControllerDataEvt *PSK_FillPairingReqRsqMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmPairReqRspMsg_S msg = {
        .ioAbility = 0x04,
        .oobDataFlag = 0,
        .authReq = { .secAttribute  = 1, .mitmDefend = 1, .kpressNotif = 0 },
        .secKeyMaxLen = 16,
        .secInfoDis = 0,
        .codeAlgoCap = {2, 2, 2, 2},
        .pskFlag = 1,
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

static DLI_ControllerDataEvt *PSK_FillPairingCfmMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    NLSTK_SmKeyPair_S keyPair = {
        .algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2,
    };
    SmGenPubPriKey(&keyPair);
    SmPairCfmMsg_S msg = {
        .secKeyLen = 16,
        .authMethod = SM_AUTH_PSK,
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

static DLI_ControllerDataEvt *PSK_FillPairingInitInfoMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
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

static DLI_ControllerDataEvt *PSK_FillAuthRandomAMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    uint8_t randNum[SM_RANDOM_NUMBER_R_LEN];
    for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
        randNum[i] = i;
    }
    SmAuthRandomAMsg_S msg = {0};
    (void)memcpy_s(msg.authData, SM_RANDOM_NUMBER_R_LEN, randNum, SM_RANDOM_NUMBER_R_LEN);
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)SDF_MemAlloc(
        sizeof(DLI_ControllerDataEvt) + sizeof(SmAuthRandomAMsg_S));
    if (buff == NULL) {
        NLSTK_LOG_ERROR("[SM] Mem alloc error.");
        return NULL;
    }
    buff->connHandle = connHandle;
    buff->ctrlDataIndex = ctrlDataIndex;
    buff->len = sizeof(SmAuthRandomAMsg_S);
    (void)memcpy_s(buff->data, buff->len, &msg, sizeof(SmAuthRandomAMsg_S));
    return buff;
}

static DLI_ControllerDataEvt *PSK_FillAuthRandomBMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    uint8_t randNum[SM_RANDOM_NUMBER_R_LEN];
    for (int i = 0; i < SM_RANDOM_NUMBER_R_LEN; i++) {
        randNum[i] = i;
    }
    SmAuthRandomBMsg_S msg = {0};
    (void)memcpy_s(msg.authData, SM_RANDOM_NUMBER_R_LEN, randNum, SM_RANDOM_NUMBER_R_LEN);
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)SDF_MemAlloc(
        sizeof(DLI_ControllerDataEvt) + sizeof(SmAuthRandomBMsg_S));
    if (buff == NULL) {
        NLSTK_LOG_ERROR("[SM] Mem alloc error.");
        return NULL;
    }
    buff->connHandle = connHandle;
    buff->ctrlDataIndex = ctrlDataIndex;
    buff->len = sizeof(SmAuthRandomBMsg_S);
    (void)memcpy_s(buff->data, buff->len, &msg, sizeof(SmAuthRandomBMsg_S));
    return buff;
}

static DLI_ControllerDataEvt *PSK_FillAuthGCfmMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmAuthGCfmMsg_S msg = {0};
    uint8_t confirm[SM_CONFIRM_NUMBER_LEN] = {0};
    (void)memcpy_s(msg.authData, SM_CONFIRM_NUMBER_LEN, confirm, SM_CONFIRM_NUMBER_LEN);
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)SDF_MemAlloc(
        sizeof(DLI_ControllerDataEvt) + sizeof(SmAuthGCfmMsg_S));
    if (buff == NULL) {
        NLSTK_LOG_ERROR("[SM] Mem alloc error.");
        return NULL;
    }
    buff->connHandle = connHandle;
    buff->ctrlDataIndex = ctrlDataIndex;
    buff->len = sizeof(SmAuthGCfmMsg_S);
    (void)memcpy_s(buff->data, buff->len, &msg, sizeof(SmAuthGCfmMsg_S));
    return buff;
}

static DLI_ControllerDataEvt *PSK_FillAuthTCfmMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmAuthTCfmMsg_S msg = {0};
    uint8_t confirm[SM_CONFIRM_NUMBER_LEN] = {0};
    (void)memcpy_s(msg.authData, SM_CONFIRM_NUMBER_LEN, confirm, SM_CONFIRM_NUMBER_LEN);
    DLI_ControllerDataEvt *buff = (DLI_ControllerDataEvt *)SDF_MemAlloc(
        sizeof(DLI_ControllerDataEvt) + sizeof(SmAuthTCfmMsg_S));
    if (buff == NULL) {
        NLSTK_LOG_ERROR("[SM] Mem alloc error.");
        return NULL;
    }
    buff->connHandle = connHandle;
    buff->ctrlDataIndex = ctrlDataIndex;
    buff->len = sizeof(SmAuthTCfmMsg_S);
    (void)memcpy_s(buff->data, buff->len, &msg, sizeof(SmAuthTCfmMsg_S));
    return buff;
}

static DLI_ControllerDataEvt *PSK_FillAuthDhKeyGMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
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

static DLI_ControllerDataEvt *PSK_FillAuthDhKeyTMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
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