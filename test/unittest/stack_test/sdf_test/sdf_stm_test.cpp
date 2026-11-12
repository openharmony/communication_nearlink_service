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

#include <gtest/gtest.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "sdf_mem.h"
#include "sdf_stm.h"

extern SDF_MemHooks_S g_stMemfunc;

namespace {
int g_entryCount = 0;
int g_dispatchCount = 0;

void CountingEntry(State *state)
{
    g_entryCount++;
}

void CountingDispatch(State *state, Message msg)
{
    g_dispatchCount++;
}

void AddState(StateMachine *stm, const char *name, void (*entry)(State *), void (*dispatch)(State *, Message))
{
    State *state = StateCtor(stm, name);
    ASSERT_NE(state, nullptr);
    if (entry != nullptr) {
        state->Entry = entry;
    }
    if (dispatch != nullptr) {
        state->Dispatch = dispatch;
    }
    EXPECT_TRUE(stm->EmplaceNewState(stm, state));
}
} // namespace

TEST(SdfStmTest, CtorInitializesEmptyMachine)
{
    // 新构造的状态机无当前状态，查询状态名返回未知名占位
    StateMachine stm;
    EXPECT_TRUE(StateMachineSoftBaseCtor(&stm));
    EXPECT_EQ(stm.states_, nullptr);
    EXPECT_EQ(stm.current_, nullptr);
    EXPECT_STREQ(stm.GetCurrentStateName(&stm), "UnknownStateName");
    EXPECT_NE(stm.Transition, nullptr);
    EXPECT_NE(stm.ProcessMessage, nullptr);
    StateMachineSoftBaseDtor(&stm);
}

TEST(SdfStmTest, StateDtorToleratesNull)
{
    // 空指针析构不崩溃
    StateDtor(nullptr);
}

TEST(SdfStmTest, TransitionToKnownStateRunsEntry)
{
    // 跳到已注册状态会触发其进入回调并更新当前状态名
    g_entryCount = 0;
    StateMachine stm;
    ASSERT_TRUE(StateMachineSoftBaseCtor(&stm));
    AddState(&stm, "idle", nullptr, nullptr);
    AddState(&stm, "active", CountingEntry, nullptr);

    stm.Transition(&stm, "active");
    EXPECT_EQ(g_entryCount, 1);
    EXPECT_STREQ(stm.GetCurrentStateName(&stm), "active");

    StateMachineSoftBaseDtor(&stm);
}

TEST(SdfStmTest, TransitionToUnknownStateKeepsCurrent)
{
    // 跳到未注册状态时当前状态不变，也不触发任何进入回调
    g_entryCount = 0;
    StateMachine stm;
    ASSERT_TRUE(StateMachineSoftBaseCtor(&stm));
    AddState(&stm, "idle", nullptr, nullptr);
    AddState(&stm, "active", CountingEntry, nullptr);
    stm.Transition(&stm, "active");
    EXPECT_EQ(g_entryCount, 1);

    stm.Transition(&stm, "missing");
    EXPECT_EQ(g_entryCount, 1);
    EXPECT_STREQ(stm.GetCurrentStateName(&stm), "active");

    StateMachineSoftBaseDtor(&stm);
}

TEST(SdfStmTest, TransitionExitsPreviousState)
{
    // 二次跳转时先执行旧状态的退出回调再进入新状态
    g_entryCount = 0;
    StateMachine stm;
    ASSERT_TRUE(StateMachineSoftBaseCtor(&stm));
    AddState(&stm, "first", nullptr, nullptr);
    AddState(&stm, "second", nullptr, nullptr);
    stm.Transition(&stm, "first");

    stm.Transition(&stm, "second");
    EXPECT_STREQ(stm.GetCurrentStateName(&stm), "second");
    // 默认退出回调为空实现，这里只验证跳转本身安全完成
    StateMachineSoftBaseDtor(&stm);
}

TEST(SdfStmTest, ProcessMessageWithoutCurrentStateIsSafe)
{
    // 没有当前状态时投递消息是空操作；有当前状态时派发给其处理函数
    g_dispatchCount = 0;
    Message msg = {1, nullptr};
    StateMachine stm;
    ASSERT_TRUE(StateMachineSoftBaseCtor(&stm));
    stm.ProcessMessage(&stm, msg); // 无当前状态

    AddState(&stm, "idle", nullptr, CountingDispatch);
    stm.Transition(&stm, "idle");
    stm.ProcessMessage(&stm, msg);
    EXPECT_EQ(g_dispatchCount, 1);

    StateMachineSoftBaseDtor(&stm);
}

TEST(SdfStmTest, StateNameIsCopiedOnCtor)
{
    // 构造状态时名字被复制，改源字符串不影响状态名
    char nameBuf[8] = "idle";
    StateMachine stm;
    ASSERT_TRUE(StateMachineSoftBaseCtor(&stm));
    State *state = StateCtor(&stm, nameBuf);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(stm.EmplaceNewState(&stm, state));

    nameBuf[0] = 'X';
    EXPECT_STREQ(stm.GetCurrentStateName(&stm), "UnknownStateName");
    stm.Transition(&stm, "idle");
    EXPECT_STREQ(stm.GetCurrentStateName(&stm), "idle");

    StateMachineSoftBaseDtor(&stm);
}

namespace {
pthread_t g_mainThread;
int g_allocSkip = -1;
int g_allocSeq = 0;

// 故障注入分配钩子：仅主线程从第 skip+1 次分配起失败
void *MainThreadFailAlloc(size_t size)
{
    if (g_allocSkip >= 0 && pthread_equal(pthread_self(), g_mainThread) && ++g_allocSeq > g_allocSkip) {
        return nullptr;
    }
    return malloc(size);
}
} // namespace

class SdfStmFaultTest : public testing::Test {
protected:
    void SetUp() override
    {
        g_mainThread = pthread_self();
        m_saved = g_stMemfunc;
        SDF_MemHooks_S hook = {.mAlloc = MainThreadFailAlloc, .mFree = m_saved.mFree};
        ASSERT_EQ(SDF_MemHookReg(&hook), SDF_OK);
        g_allocSkip = -1;
        g_allocSeq = 0;
    }
    void TearDown() override
    {
        g_allocSkip = -1;
        g_stMemfunc = m_saved;
    }
    SDF_MemHooks_S m_saved = {};
};

TEST_F(SdfStmFaultTest, StateCtorStateAllocFails)
{
    // 状态结构分配失败时构造返回空
    StateMachine stm;
    ASSERT_TRUE(StateMachineSoftBaseCtor(&stm));
    g_allocSkip = 0;
    EXPECT_EQ(StateCtor(&stm, "ut"), nullptr);
    g_allocSkip = -1;
    StateMachineSoftBaseDtor(&stm);
}

TEST_F(SdfStmFaultTest, StateCtorNameAllocFails)
{
    // 名字缓冲分配失败时构造回收状态结构并返回空
    StateMachine stm;
    ASSERT_TRUE(StateMachineSoftBaseCtor(&stm));
    g_allocSkip = 1;
    EXPECT_EQ(StateCtor(&stm, "ut"), nullptr);
    g_allocSkip = -1;
    StateMachineSoftBaseDtor(&stm);
}
