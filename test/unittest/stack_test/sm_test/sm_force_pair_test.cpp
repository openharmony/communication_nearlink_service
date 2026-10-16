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
 * 强制配对接口(NLSTK_SmForceStartPairing/SmForceStartPairing)TDD用例.
 * 核心语义: 无论slink处于何种状态, 强制配对都清空旧linkKey并复位回INIT,
 * 复用既有 SM_ACTIVE_START 首配路径重走协商流程;
 * 原有 NLSTK_SmStartPairing 的回连语义(FULL短路/ENCP旧key加密)保持不变.
 * 复位逻辑仅依赖curStateIndex, 与具体状态无关, 故FULL态用例已覆盖
 * ENCP/MISS等其余非INIT状态的复位分支; 协商之后的鉴权/加密完整流程
 * 由 sm_dispatch_test/sm_passcode_test 覆盖.
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
    if (state != SM_STATE_INIT) {
        STM_MFUNC(slink->stm, Transition, g_smStateName[state]);
    }
    return slink;
}

/* 匹配G节点发出的配对请求报文(opcode校验) */
MATCHER_P(IsPairingReqMsg, opcode, "controller data carries expected pairing opcode")
{
    return arg->opcode == opcode;
}
} // namespace

class UT_SM_ForcePair_Test : public testing::Test {
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
 * API空指针校验
 */
TEST_F(UT_SM_ForcePair_Test, ForceStartPairingNullPointer)
{
    NLSTK_Errcode_E ret = NLSTK_SmForceStartPairing(nullptr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_POINTER_NULL);
}

/*
 * 新设备(INIT态)强配与既有首配路径行为一致:
 * 绑链路成功后进入NEGO, G节点发出配对请求报文, 协商管线就绪
 */
TEST_F(UT_SM_ForcePair_Test, ForceStartPairingOnInitStateEqualsFirstPair)
{
    EXPECT_CALL(dliMock, DLI_SetControllerData(IsPairingReqMsg(SM_NEGO_PAIRING_REQUEST))).Times(1);
    NLSTK_Errcode_E ret = NLSTK_SmForceStartPairing(&g_rmtAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    SmSLink_S *slink = SmFindSLink(&g_rmtAddr);
    ASSERT_NE(slink, nullptr);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_NEGO);
    EXPECT_EQ(slink->dispatcher, SmNegoPkgDispatcher);
    EXPECT_EQ(slink->expectOpCode, SM_NEGO_PAIRING_RESPONSE);
    EXPECT_NE(slink->lcid, CM_INVALID_LCID);
}

/*
 * 绑链路失败时强配走既有REMV路径, slink被移除并触发remove回调
 */
TEST_F(UT_SM_ForcePair_Test, ForceStartPairingBindLinkFail)
{
    EXPECT_CALL(cmMock, CM_GetLogicLinkByAddr(_, _)).WillRepeatedly(Return(CM_FAIL));
    EXPECT_CALL(dliMock, DLI_SetControllerData).Times(0);

    NLSTK_Errcode_E ret = NLSTK_SmForceStartPairing(&g_rmtAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    EXPECT_EQ(SmFindSLink(&g_rmtAddr), nullptr);
    EXPECT_TRUE(g_isRecvCbk);
}

/*
 * 核心用例: FULL态(已配对)强配时清空旧linkKey, 复位重走协商, 管线重新布防
 */
TEST_F(UT_SM_ForcePair_Test, ForceStartPairingOnFullStateRenego)
{
    SmSLink_S *slink = MakeSlinkInState(&g_rmtAddr, SM_STATE_FULL);
    ASSERT_NE(slink, nullptr);
    FillLinkKey(slink, 0xA5);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_FULL);

    EXPECT_CALL(dliMock, DLI_SetControllerData(IsPairingReqMsg(SM_NEGO_PAIRING_REQUEST))).Times(1);
    NLSTK_Errcode_E ret = NLSTK_SmForceStartPairing(&g_rmtAddr);
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
 * FULL态旧接口回归保护: 仍短路返回配对成功, 不重走协商
 */
TEST_F(UT_SM_ForcePair_Test, StartPairingOnFullStateStillShortCircuit)
{
    SmSLink_S *slink = MakeSlinkInState(&g_rmtAddr, SM_STATE_FULL);
    ASSERT_NE(slink, nullptr);
    FillLinkKey(slink, 0x5A);
    Test_ResetData();

    EXPECT_CALL(dliMock, DLI_SetControllerData).Times(0);
    NLSTK_Errcode_E ret = NLSTK_SmStartPairing(&g_rmtAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    slink = SmFindSLink(&g_rmtAddr);
    ASSERT_NE(slink, nullptr);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_FULL);
    EXPECT_FALSE(IsLinkKeyCleared(slink));
    EXPECT_TRUE(g_isRecvCbk); // encCbk(SM_PAIR_OK)短路回调
}

/*
 * ENCP态(回连恢复过key)旧接口回归保护: 仍用旧key走加密使能(回连路径)
 */
TEST_F(UT_SM_ForcePair_Test, StartPairingOnEncpStateStillRequestSecurity)
{
    SmSLink_S *slink = MakeSlinkInState(&g_rmtAddr, SM_STATE_ENCP);
    ASSERT_NE(slink, nullptr);
    FillLinkKey(slink, 0x3C);

    EXPECT_CALL(dliMock, DLI_SetControllerData).Times(0);
    EXPECT_CALL(dliMock, DLI_EnableEncryption).Times(1);
    NLSTK_Errcode_E ret = NLSTK_SmStartPairing(&g_rmtAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    slink = SmFindSLink(&g_rmtAddr);
    ASSERT_NE(slink, nullptr);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_ENCP);
    EXPECT_FALSE(IsLinkKeyCleared(slink));
}

/*
 * 协商进行中(NEGO态)再次强配: 清理挂起定时器并重新发起协商
 * (复位逻辑仅依赖curStateIndex, AUTH态行为与之等价)
 */
TEST_F(UT_SM_ForcePair_Test, ForceStartPairingOnNegoStateRestart)
{
    NLSTK_Errcode_E ret = NLSTK_SmForceStartPairing(&g_rmtAddr); // 第一次: INIT -> NEGO
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    SmSLink_S *slink = SmFindSLink(&g_rmtAddr);
    ASSERT_NE(slink, nullptr);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_NEGO);
    int pendingTimer = slink->timerHandle;
    EXPECT_NE(pendingTimer, TIMER_NO_USED_VALUE);

    // 第二次强配: 清理挂起定时器并重新发起协商
    EXPECT_CALL(scheduleMock, ScheduleTimerDel(pendingTimer)).Times(1);
    EXPECT_CALL(dliMock, DLI_SetControllerData(IsPairingReqMsg(SM_NEGO_PAIRING_REQUEST))).Times(1);
    ret = NLSTK_SmForceStartPairing(&g_rmtAddr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    slink = SmFindSLink(&g_rmtAddr);
    ASSERT_NE(slink, nullptr);
    EXPECT_EQ(slink->curStateIndex, SM_STATE_NEGO);
    EXPECT_EQ(slink->expectOpCode, SM_NEGO_PAIRING_RESPONSE);
}
