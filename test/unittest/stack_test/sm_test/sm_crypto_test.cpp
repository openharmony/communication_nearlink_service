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

#include "securec.h"
#include "gtest/gtest.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dli_mock.h"
#include "stack_dli_stub.h"

#include "nlstk_api_type_ext.h"

#include "nlstk_sm.h"
#include "sm.h"
#include "sm_algos.h"
#include "sm_stub_test.h"
#include "nlstk_sm_algos.h"
#include "nlstk_public_define_ext.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

#define STACK_MAX_THREAD 5

class UT_SM_Crypto_Test : public testing::Test {
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
    {}

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {}
};

SLE_Addr_S g_gNodeAddr = { .type = PUBLIC_ADDRESS, .addr = { 0x18, 0xda, 0x8f, 0x00, 0x01, 0x00, }, };
SLE_Addr_S g_tNodeAddr = { .type = PUBLIC_ADDRESS, .addr = { 0x00, 0x01, 0x0c, 0xb3, 0x33, 0x47, }, };

uint8_t g_randNum[SM_RANDOM_NUMBER_R_LEN] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
};

uint8_t g_gNodePriKey[SM_PRIVATE_KEY_LEN] = {
    0x72, 0x53, 0x01, 0x65, 0x9f, 0xe0, 0xdb, 0xf1, 0xdf, 0x65, 0x55, 0x1b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

uint8_t g_gNodePubKey[SM_PUBLIC_KEY_LEN] = {
    0x5b, 0x75, 0xdf, 0xb2, 0x5d, 0x78, 0xa0, 0x74, 0xa4, 0x37, 0x8d, 0xad, 0xc9, 0x02, 0x1e, 0xd1,
    0xd6, 0x2e, 0x42, 0xb8, 0xe7, 0x76, 0x88, 0x07, 0x95, 0x04, 0x83, 0x95, 0x37, 0x13, 0x81, 0xd5,
    0x85, 0x8d, 0xda, 0xbd, 0x99, 0xad, 0x9c, 0x2b, 0x6a, 0x16, 0x8e, 0x43, 0xcd, 0x7a, 0x6f, 0xc4,
    0xfd, 0x65, 0x72, 0x11, 0x7d, 0x62, 0xb1, 0xd1, 0x0c, 0x14, 0x59, 0x82, 0x13, 0xfe, 0x3c, 0x9c,
};

uint8_t g_tNodePubKey[SM_PUBLIC_KEY_LEN] = {
    0xc6, 0x0a, 0xe7, 0x16, 0xbd, 0x4a, 0x16, 0xa8, 0xdd, 0x7c, 0x8b, 0x22, 0x4d, 0x62, 0x82, 0x20,
    0xc1, 0xc6, 0x53, 0xec, 0x4e, 0x33, 0x87, 0xcb, 0x7c, 0xc8, 0x5f, 0xfb, 0x4f, 0x59, 0x05, 0x70,
    0x7a, 0x3c, 0xfc, 0x3c, 0xfc, 0xfc, 0x8d, 0xba, 0xc3, 0x4e, 0x8f, 0x8a, 0x0d, 0xcf, 0x40, 0xe9,
    0x8a, 0x2d, 0x40, 0xfb, 0xc1, 0xf0, 0x7b, 0x97, 0x1d, 0xc1, 0x27, 0xb1, 0xa1, 0xa6, 0x23, 0x11,
};

uint8_t g_dhKey[SM_DHKEY_LEN] = {
    0xdf, 0x21, 0x93, 0x16, 0x77, 0x5c, 0xa0, 0x96, 0x64, 0x4f, 0x40, 0x1d, 0x77, 0x39, 0x0a, 0x43,
    0xdb, 0xeb, 0xfa, 0x26, 0x27, 0xd8, 0xe7, 0x99, 0xc2, 0x06, 0x09, 0x5b, 0x4f, 0x81, 0xb9, 0xd7,
};

uint8_t g_linkKey[SM_LINK_KEY_LEN] = {0};

TEST_F(UT_SM_Crypto_Test, TestCryptoAlgos)
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
        .rmtAddr = g_tNodeAddr,
        .localAddr = g_gNodeAddr,
        .role = SM_G_NODE,
    };
    SmGenRandNum(slink.gNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    SmGenRandNum(slink.tNode.randomR, SM_RANDOM_NUMBER_R_LEN);
    EXPECT_EQ(memcmp(slink.gNode.randomR, g_randNum, SM_RANDOM_NUMBER_R_LEN), 0);
    EXPECT_EQ(memcmp(slink.tNode.randomR, g_randNum, SM_RANDOM_NUMBER_R_LEN), 0);

    NLSTK_SmKeyPair_S keyPair;
    keyPair.algo = SM_KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2;
    SmGenPubPriKey(&keyPair);
    uint8_t dhKey[SM_DHKEY_LEN];
    SmGenDhKey(&keyPair, dhKey, SM_DHKEY_LEN);
    EXPECT_EQ(memcmp(dhKey, g_dhKey, SM_DHKEY_LEN), 0);

    (void)memcpy_s(slink.dhKey, SM_DHKEY_LEN, dhKey, SM_DHKEY_LEN);
    SmGenLinkKey(&slink);
    EXPECT_EQ(memcmp(slink.linkKey, g_linkKey, SM_LINK_KEY_LEN), 0);
}