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

static DLI_ControllerDataEvt *OOB_FillPairingReqRsqMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *OOB_FillPairingCfmMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *OOB_FillPairingInitInfoMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *OOB_FillAuthGCfmWithRaMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *OOB_FillAuthTCfmWithRbMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *OOB_FillAuthDhKeyGMsg(uint16_t connHandle, uint16_t ctrlDataIndex);
static DLI_ControllerDataEvt *OOB_FillAuthDhKeyTMsg(uint16_t connHandle, uint16_t ctrlDataIndex);

class SM_OOB_Test : public testing::Test {
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

TEST_F(SM_OOB_Test, TEST_SM_OOB_001)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_MAX;

    NLSTK_SmLocalParams_S params = {
        .ioAbility = 0x04,
        .authMethodMask = 0xFF,
        .oobDataFlag = 1,
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
        .derivedKeyFunc = Crypto_DerivedKeyGenerate,
    };
    ret = NLSTK_SmRegAlgoFuncs(&algoFuncs);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    EXPECT_CALL(cfgdbMock, NLSTK_CfgdbGetPublicAddress).WillRepeatedly(NLSTK_CfgdbGetPublicAddressForTStub);
    EXPECT_CALL(cmMock, CM_GetLogicLinkByAddr).WillRepeatedly(CM_GetLogicLinkByAddrForTStub);
    EXPECT_CALL(cmMock, CM_GetLogicLinkByLcid).WillRepeatedly(CM_GetLogicLinkByLcidForTStub);

    ret = NLSTK_SmStartPairing(&g_gAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
}

TEST_F(SM_OOB_Test, TEST_SM_OOB_002)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_MAX;

    NLSTK_SmLocalParams_S params = {
        .ioAbility = 0x04,
        .authMethodMask = 0xFF,
        .oobDataFlag = 1,
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
        .derivedKeyFunc = Crypto_DerivedKeyGenerate,
    };
    ret = NLSTK_SmRegAlgoFuncs(&algoFuncs);
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
    NLSTK_LOG_ERROR("[Test]Enter SmSLinkWaitExpectOpCode");
    SDF_UNUSED(timeout);
    slink->expectOpCode = expectOpCode;

    DLI_ControllerDataEvt *buff = NULL;
    switch (expectOpCode) {
        case SM_NEGO_PAIRING_REQUEST:
            buff = OOB_FillPairingReqRsqMsg(slink->lcid, SM_NEGO_PAIRING_REQUEST);
            break;
        case SM_NEGO_PAIRING_RESPONSE:
            buff = OOB_FillPairingReqRsqMsg(slink->lcid, SM_NEGO_PAIRING_RESPONSE);
            break;
        case SM_NEGO_PAIRING_CONFIRM:
            buff = OOB_FillPairingCfmMsg(slink->lcid, SM_NEGO_PAIRING_CONFIRM);
            break;
        case SM_NEGO_PAIRING_INIT_INFO:
            buff = OOB_FillPairingInitInfoMsg(slink->lcid, SM_NEGO_PAIRING_INIT_INFO);
            break;
        case SM_AUTH_G_NODE_CFM_WITH_RA:
            buff = OOB_FillAuthGCfmWithRaMsg(slink->lcid, SM_AUTH_G_NODE_CFM_WITH_RA);
            break;
        case SM_AUTH_T_NODE_CFM_WITH_RB:
            buff = OOB_FillAuthTCfmWithRbMsg(slink->lcid, SM_AUTH_T_NODE_CFM_WITH_RB);
            break;
        case SM_AUTH_G_NODE_DHKEY:
            buff = OOB_FillAuthDhKeyGMsg(slink->lcid, SM_AUTH_G_NODE_DHKEY);
            break;
        case SM_AUTH_T_NODE_DHKEY:
            buff = OOB_FillAuthDhKeyTMsg(slink->lcid, SM_AUTH_T_NODE_DHKEY);
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

static DLI_ControllerDataEvt *OOB_FillPairingReqRsqMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmPairReqRspMsg_S msg = {
        .ioAbility = 0x04,
        .oobDataFlag = 1,
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

static DLI_ControllerDataEvt *OOB_FillPairingCfmMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    NLSTK_SmKeyPair_S keyPair = {
        .algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2,
    };
    SmGenPubPriKey(&keyPair);
    SmPairCfmMsg_S msg = {
        .secKeyLen = 16,
        .authMethod = SM_AUTH_OUT_OF_BAND,
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

static DLI_ControllerDataEvt *OOB_FillPairingInitInfoMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
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

static DLI_ControllerDataEvt *OOB_FillAuthGCfmWithRaMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmAuthGCfmWithRaMsg_S msg = {{0}};
    uint8_t confirm[SM_CONFIRM_NUMBER_LEN] = {
        0x78, 0xB8, 0x7D, 0x26, 0xF3, 0x01, 0xC4, 0x5D, 0x53, 0x40, 0xF1, 0x98, 0x52, 0x97, 0xEC, 0xE9
    };
    (void)memcpy_s(msg.gNodeCfm, SM_CONFIRM_NUMBER_LEN, confirm, SM_CONFIRM_NUMBER_LEN);
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

static DLI_ControllerDataEvt *OOB_FillAuthTCfmWithRbMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmAuthTCfmWithRbMsg_S msg = {{0}};
    uint8_t confirm[SM_CONFIRM_NUMBER_LEN] = {
        0x78, 0xB8, 0x7D, 0x26, 0xF3, 0x01, 0xC4, 0x5D, 0x53, 0x40, 0xF1, 0x98, 0x52, 0x97, 0xEC, 0xE9
    };
    (void)memcpy_s(msg.tNodeCfm, SM_CONFIRM_NUMBER_LEN, confirm, SM_CONFIRM_NUMBER_LEN);
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

static DLI_ControllerDataEvt *OOB_FillAuthDhKeyGMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmAuthDhkeyMsg_S msg = {0};
    uint8_t dhkeyCode[SM_DHKEY_AUTHCODE_LEN] = {
        0xA6, 0x95, 0x04, 0x8D, 0x33, 0xDD, 0x73, 0x17, 0x6F, 0x0C, 0x5E, 0x07, 0x87, 0x09, 0x66, 0x54
    };
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

static DLI_ControllerDataEvt *OOB_FillAuthDhKeyTMsg(uint16_t connHandle, uint16_t ctrlDataIndex)
{
    SmAuthDhkeyMsg_S msg = {0};
    uint8_t dhkeyCode[SM_DHKEY_AUTHCODE_LEN] = {
        0xA6, 0x95, 0x04, 0x8D, 0x33, 0xDD, 0x73, 0x17, 0x6F, 0x0C, 0x5E, 0x07, 0x87, 0x09, 0x66, 0x54
    };
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