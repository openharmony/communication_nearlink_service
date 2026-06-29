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
#include "sdf_mem.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dli_mock.h"
#include "stack_dli_stub.h"

#include "securec.h"
#include "sm.h"
#include "nlstk_sm.h"
#include "nlstk_schedule.h"
#include "sm_stub_test.h"
#include "sm_numcmp.h"
#include "sm_noentry.h"
#include "nlstk_log.h"
#include "sm_struct.h"

extern NLSTK_SmCryptoAlgoFuncs_S g_algoFuncs;

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

class UT_SM_CMN_Test : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DliMock> dliMock;
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
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

void ProcessMessageStub(struct tagStateMachine *stm, struct tagMessage msg)
{
    return;
}

void TransitionStub(struct tagState *state, const char *targetStateName)
{
    return;
}

/*****************************************************************************************
                                    Test Functions
*****************************************************************************************/

// 3
TEST_F(UT_SM_CMN_Test, SmNumCmpAuthPkgDispatcher)
{
    SmSLink_S slink;
    uint8_t pkg[] = {0x00};
    memset_s(&slink, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    SmNumCmpAuthPkgDispatcher(SM_AUTH_G_NODE_DHKEY, &slink, pkg, sizeof(pkg));

    SmNumCmpAuthPkgDispatcher(SM_AUTH_T_NODE_DHKEY, &slink, pkg, sizeof(pkg));

    SmNumCmpAuthPkgDispatcher(SM_PAIR_FAIL_MESSAGE_OPCODE, &slink, pkg, sizeof(pkg));
}

// 2
TEST_F(UT_SM_CMN_Test, SmNumCmpAuthStart)
{
    SmSLink_S slink;
    SmStateMachine_S state;
    memset_s(&slink, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    slink.role = (SmNodeType_E)1;
    slink.stm = &state;
    memset_s(&state, sizeof(SmStateMachine_S), 0, sizeof(SmStateMachine_S));
    state.base.ProcessMessage = ProcessMessageStub;
    NLSTK_LOG_ERROR("[SM][NOENTRY] SmNumCmpAuthStart 0, role: %d", slink.role);

    g_algoFuncs.randNumFunc = NULL;
    SmNumCmpAuthStart(&slink);
    NLSTK_LOG_ERROR("[SM][NOENTRY] SmNumCmpAuthStart 1");

    g_algoFuncs.randNumFunc = Test_CryptoRandNumGenerate;
    g_algoFuncs.derivedKeyFunc = NULL;
    SmNumCmpAuthStart(&slink);
    NLSTK_LOG_ERROR("[SM][NOENTRY] SmNumCmpAuthStart 2");
}

// 3
TEST_F(UT_SM_CMN_Test, SmNoEntryAuthStart)
{
    SmSLink_S slink;
    SmStateMachine_S state;
    memset_s(&slink, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    slink.role = SM_G_NODE;
    slink.stm = &state;
    memset_s(&state, sizeof(SmStateMachine_S), 0, sizeof(SmStateMachine_S));
    state.base.ProcessMessage = ProcessMessageStub;
    NLSTK_LOG_ERROR("[SM][NOENTRY] SmNoEntryAuthStart 0");

    g_algoFuncs.randNumFunc = NULL;
    SmNoEntryAuthStart(&slink);
    NLSTK_LOG_ERROR("[SM][NOENTRY] SmNoEntryAuthStart 1");

    slink.role = (SmNodeType_E)1;
    SmNoEntryAuthStart(&slink);
    NLSTK_LOG_ERROR("[SM][NOENTRY] SmNoEntryAuthStart 2");

    g_algoFuncs.randNumFunc = Test_CryptoRandNumGenerate;
    g_algoFuncs.derivedKeyFunc = NULL;
    SmNoEntryAuthStart(&slink);
    NLSTK_LOG_ERROR("[SM][NOENTRY] SmNoEntryAuthStart 3");
}

// 3
TEST_F(UT_SM_CMN_Test, SmNoEntryAuthPkgDispatcher)
{
    SmSLink_S slink;
    uint8_t pkg[SM_DHKEY_AUTHCODE_LEN] = {0x00};
    memset_s(&slink, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    slink.uapiTimerHandle = 0;
    SmNoEntryAuthPkgDispatcher(SM_AUTH_G_NODE_DHKEY, &slink, pkg, 1);

    SmNoEntryAuthPkgDispatcher(SM_AUTH_G_NODE_DHKEY, &slink, pkg, sizeof(SmAuthDhkeyMsg_S));

    SmNoEntryAuthPkgDispatcher(SM_PAIR_FAIL_MESSAGE_OPCODE, &slink, pkg, sizeof(pkg));
}

// 2
TEST_F(UT_SM_CMN_Test, SmNoEntryContinueNoentry)
{
    SmSLink_S slink;
    SmStateMachine_S state;
    memset_s(&slink, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    slink.role = (SmNodeType_E)1;
    slink.tNode.recvFlag = false;
    slink.stm = &state;
    memset_s(&state, sizeof(SmStateMachine_S), 0, sizeof(SmStateMachine_S));
    state.base.ProcessMessage = ProcessMessageStub;
    NLSTK_LOG_ERROR("[SM][NOENTRY] SmNoEntryContinueNoentry 0");

    g_algoFuncs.secKeyFunc = Test_CryptoSecKeyGenerate;
    SmNoEntryContinueNoentry(&slink);
    NLSTK_LOG_ERROR("[SM][NOENTRY] SmNoEntryContinueNoentry 1");

    slink.tNode.recvFlag = true;
    SmNoEntryContinueNoentry(&slink);
    NLSTK_LOG_ERROR("[SM][NOENTRY] SmNoEntryContinueNoentry 2");
}

// 5
TEST_F(UT_SM_CMN_Test, MissDispatch)
{
    SmSLink_S link;
    struct tagMessage msg;
    memset_s(&link, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    memset_s(&msg, sizeof(struct tagMessage), 0, sizeof(struct tagMessage));

    SmStateMachine_S *stm = SmStateMachineCtor(&link);
    EXPECT_EQ(stm == NULL, 0);

    State *state = stm->base.states_->next->next; // miss
    state->Transition = TransitionStub;
    stm->base.current_ = state;
    NLSTK_LOG_ERROR("[SM][NOENTRY] MissDispatch 0");

    msg.what = SM_MISS_HANDLE;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] MissDispatch 1");

    msg.what = SM_REMOVE_PAIR;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] MissDispatch 2");

    msg.what = SM_LCHANNEL_DISCONN;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] MissDispatch 3");

    msg.what = SM_PASSIVE_START;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] MissDispatch 4");

    state->stm_ = (StateMachine *)stm;
    state->Entry(state);
}

// 5
TEST_F(UT_SM_CMN_Test, FullDispatch)
{
    SmSLink_S link;
    struct tagMessage msg;
    memset_s(&link, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    memset_s(&msg, sizeof(struct tagMessage), 0, sizeof(struct tagMessage));

    SmStateMachine_S *stm = SmStateMachineCtor(&link);
    EXPECT_EQ(stm == NULL, 0);

    State *state = stm->base.states_->next; // full
    state->Transition = TransitionStub;
    stm->base.current_ = state;
    NLSTK_LOG_ERROR("[SM][NOENTRY] FullDispatch 0");

    msg.what = SM_LCHANNEL_DISCONN;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] FullDispatch 1");

    msg.what = SM_REMOVE_PAIR;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] FullDispatch 2");

    msg.what = SM_ACTIVE_START;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] FullDispatch 3");

    msg.what = SM_PASSIVE_START;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] FullDispatch 4");

    state->stm_ = (StateMachine *)stm;
    state->Entry(state);
}

// 6
TEST_F(UT_SM_CMN_Test, EncpDispatch)
{
    SmSLink_S link;
    struct tagMessage msg;
    memset_s(&link, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    memset_s(&msg, sizeof(struct tagMessage), 0, sizeof(struct tagMessage));

    SmStateMachine_S *stm = SmStateMachineCtor(&link);
    EXPECT_EQ(stm == NULL, 0);

    State *state = stm->base.states_->next->next->next; // encp
    state->Transition = TransitionStub;
    stm->base.current_ = state;
    NLSTK_LOG_ERROR("[SM][NOENTRY] EncpDispatch 0");

    msg.what = SM_ENCP_SUCCESS;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] EncpDispatch 1");

    msg.what = SM_ENCP_PARAM_REQ_REPLY;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] EncpDispatch 2");

    msg.what = SM_LCHANNEL_DISCONN;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] EncpDispatch 3");

    msg.what = SM_ENCP_FAIL;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] EncpDispatch 4");

    msg.what = SM_REMOVE_PAIR;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] EncpDispatch 5");

    state->stm_ = (StateMachine *)stm;
    state->Entry(state);
}

// 5
TEST_F(UT_SM_CMN_Test, NegoDispatch)
{
    SmSLink_S link;
    struct tagMessage msg;
    memset_s(&link, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    memset_s(&msg, sizeof(struct tagMessage), 0, sizeof(struct tagMessage));

    SmStateMachine_S *stm = SmStateMachineCtor(&link);
    EXPECT_EQ(stm == NULL, 0);

    State *state = stm->base.states_->next->next->next->next->next; // nego
    state->Transition = TransitionStub;
    stm->base.current_ = state;
    NLSTK_LOG_ERROR("[SM][NOENTRY] NegoDispatch 0");

    msg.what = SM_TIMEOUT;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] NegoDispatch 1");

    msg.what = SM_ENCP_PARAM_REQ_REPLY;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] NegoDispatch 2");

    msg.what = SM_LCHANNEL_DISCONN;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] NegoDispatch 3");

    msg.what = SM_PASSIVE_START;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] NegoDispatch 4");

    msg.what = SM_ENCP_FAIL;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] NegoDispatch 5");

    state->stm_ = (StateMachine *)stm;
    state->Entry(state);
}

// 5
TEST_F(UT_SM_CMN_Test, InitDispatch)
{
    SmSLink_S link;
    struct tagMessage msg;
    memset_s(&link, sizeof(SmSLink_S), 0, sizeof(SmSLink_S));
    memset_s(&msg, sizeof(struct tagMessage), 0, sizeof(struct tagMessage));

    SmStateMachine_S *stm = SmStateMachineCtor(&link);
    EXPECT_EQ(stm == NULL, 0);

    State *state = stm->base.states_->next->next->next->next->next->next; // init
    state->Transition = TransitionStub;
    stm->base.current_ = state;
    NLSTK_LOG_ERROR("[SM][NOENTRY] InitDispatch 0");

    msg.what = SM_LCHANNEL_DISCONN;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] InitDispatch 1");

    msg.what = SM_ENCP_PARAM_REQ_REPLY;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] InitDispatch 2");

    msg.what = SM_PASSIVE_START;
    stm->base.ProcessMessage((StateMachine *)stm, msg);
    NLSTK_LOG_ERROR("[SM][NOENTRY] InitDispatch 3");

    state->stm_ = (StateMachine *)stm;
    state->Entry(state);
}
