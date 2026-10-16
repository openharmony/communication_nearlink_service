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

/*
 * 配对/回连接口职责分离的TDD用例.
 * NLSTK_SmStartPairing(配对): 任何状态复位回INIT后重走完整配对流程;
 * NLSTK_SmRequestSecurity(回连): 按状态隐式分流, ENCP态用旧key加密.
 * 仅保留两类不可替代用例: 已配对(FULL)设备重配的核心验收, 以及换名后
 * 回连接口的接线守护. INIT态配对行为由既有 sm_api_test/sm_passcode_test/
 * sm_oob_test/sm_psk_test 覆盖(换名后自动生效), API错误路径由 sm_api_test
 * 的 TESTSmPostTaskFail 覆盖.
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
#include "sdf_addr.h"
#include "cm_logic_link_api.h"
#include "nlstk_public_define_ext.h"
#include "nlstk_api_type_ext.h"
#include "dli_event_struct.h"
#include "dli_cmd_struct.h"
#include "dli_errno.h"

#include "sm.h"
#include "sm_nego.h"
#include "sm_stm.h"
#include "sm_struct.h"
#include "nlstk_sm.h"
#include "nlstk_sm_api.h"
#include "nlstk_schedule.h"
#include "sm_stub_test.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace {
bool IsLinkKeyCleared(const SmSLink_S *slink)
{
    for (int i = 0; i < SM_LINK_KEY_LEN; i++) {
        if (slink->linkKey[i] != 0) {
            return false;
        }
    }
    return true;
}

void FillLinkKey(SmSLink_S *slink, uint8_t seed)
{
    for (int i = 0; i < SM_LINK_KEY_LEN; i++) {
        slink->linkKey[i] = seed + i;
    }
}

/* 创建slink并经真实Transition进入指定状态, Entry会正确维护curStateIndex */
SmSLink_S *MakeSlinkInState(SLE_Addr_S *addr, SmState_E state)
{
    SmSLink_S *slink = SmFindOrCreateSLink(addr);
    if (slink == nullptr) {
        return nullptr;
    }
    STM_MFUNC(slink->stm, Transition, g_smStateName[state]);
    return slink;
}

/* 匹配G节点发出的配对请求报文(opcode校验) */
MATCHER_P(IsPairingReqMsg, opcode, "controller data carries expected pairing opcode")
{
    return arg->opcode == opcode;
}
} // namespace

class UT_SM_StartPairing_Test : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DliMock> dliMock;
protected:
    virtual void SetUp()
    {
        Test_ResetData();
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

        NLSTK_SmCallbacks_S cbks = {
            .startCbk = Test_SmPairStartCbk,
            .removeCbk = Test_SmPairRemoveCbk,
            .requestCbk = Test_SmPairRequestCbk,
            .authCbk = Test_SmAuthCmpCbk,
            .encCbk = Test_SmEncCmpCbk,
            .imgMsgCbk = Test_SmImgMsgCbk,
        };
        NLSTK_SmRegExternalCbks(&cbks);
    }

    virtual void TearDown()
    {
        SmDeInit();

        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
        TEST_DLI_DeInit();
    }

    static void SetUpTestCase()
    {
    }

    static void TearDownTestCase()
    {
    }
};

/*
 * 核心用例: FULL态(已配对)发起配对, 清空旧linkKey并重走完整协商流程
 */
TEST_F(UT_SM_StartPairing_Test, StartPairingOnFullStateRenego)
{
    SmSLink_S *slink = MakeSlinkInState(&g_rmtAddr, SM_STATE_FULL);
    ASSERT_NE(slink, nullptr);
    FillLinkKey(slink, 0xA5);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_FULL);

    EXPECT_CALL(dliMock, DLI_SetControllerData(IsPairingReqMsg(SM_NEGO_PAIRING_REQUEST))).Times(1);
    NLSTK_Errcode_E ret = NLSTK_SmStartPairing(&g_rmtAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    slink = SmFindSLink(&g_rmtAddr);
    ASSERT_NE(slink, nullptr);
    EXPECT_TRUE(IsLinkKeyCleared(slink));
    EXPECT_EQ(slink->curStateIndex, SM_STATE_NEGO);
    EXPECT_EQ(slink->dispatcher, SmNegoPkgDispatcher);
    EXPECT_EQ(slink->expectOpCode, SM_NEGO_PAIRING_RESPONSE);
    EXPECT_NE(slink->timerHandle, TIMER_NO_USED_VALUE); // 等待协商响应的定时器已布防
}

/*
 * 回连接口接线守护: ENCP态(回连恢复过key)调用NLSTK_SmRequestSecurity,
 * 仍用旧key走加密使能, 不清key也不发配对报文
 */
TEST_F(UT_SM_StartPairing_Test, RequestSecurityOnEncpStateStillEncryptWithOldKey)
{
    SmSLink_S *slink = MakeSlinkInState(&g_rmtAddr, SM_STATE_ENCP);
    ASSERT_NE(slink, nullptr);
    FillLinkKey(slink, 0x3C);

    EXPECT_CALL(dliMock, DLI_SetControllerData).Times(0);
    EXPECT_CALL(dliMock, DLI_EnableEncryption).Times(1);
    NLSTK_Errcode_E ret = NLSTK_SmRequestSecurity(&g_rmtAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    slink = SmFindSLink(&g_rmtAddr);
    ASSERT_NE(slink, nullptr);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_ENCP);
    EXPECT_FALSE(IsLinkKeyCleared(slink));
}
