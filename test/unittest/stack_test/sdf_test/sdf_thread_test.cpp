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

#include <atomic>
#include <sched.h>

#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_worker.h"

extern SDF_MemHooks_S g_stMemfunc;

namespace {
std::atomic<int> g_hookDone{0};
std::atomic<uint32_t> g_selfIdInThread{0};

void HookRecordSelf(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;
    g_selfIdInThread = SDF_ThreadSelfId();
    g_hookDone = static_cast<int>(arg0);
}

void HookNoop(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;
    (void)arg3;
}

SDF_ThreadCreateParam_S MakeParam(const char *name, SDF_ThreadFunc hook, uintptr_t arg0 = 0)
{
    SDF_ThreadCreateParam_S param = {};
    param.name = name;
    param.policy = SDF_THREAD_SCHED_OTHER;
    param.priority = 0;
    param.stackSize = 0;
    param.hook = hook;
    param.args[0] = arg0;
    return param;
}
} // namespace

TEST(SdfThreadTest, InitRejectsZeroAndDoubleInit)
{
    // 上限为 0 或重复初始化被拒绝；销毁未初始化的环境是空操作
    SDF_ThreadDeinit();
    EXPECT_EQ(SDF_ThreadInit(0), SDF_THREAD_ERROR_INVALID_PARAM);
    ASSERT_EQ(SDF_ThreadInit(2), SDF_OK);
    EXPECT_EQ(SDF_ThreadInit(2), SDF_THREAD_ERROR_ALREAD_INIT);
    SDF_ThreadDeinit();
    SDF_ThreadDeinit();
}

class SdfThreadEnvTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_EQ(SDF_ThreadInit(8), SDF_OK);
    }
    void TearDown() override
    {
        SDF_ThreadDeinit();
    }
};

TEST_F(SdfThreadEnvTest, CreateRejectsInvalidParams)
{
    // 名称、回调、调度策略、优先级任一非法即拒绝创建
    uint32_t id = 0;
    SDF_ThreadCreateParam_S param = MakeParam("ut", HookNoop);
    EXPECT_EQ(SDF_ThreadCreate(nullptr, &param), SDF_THREAD_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_ThreadCreate(&id, nullptr), SDF_THREAD_ERROR_INVALID_PARAM);

    SDF_ThreadCreateParam_S noName = MakeParam(nullptr, HookNoop);
    EXPECT_EQ(SDF_ThreadCreate(&id, &noName), SDF_THREAD_ERROR_INVALID_PARAM);

    SDF_ThreadCreateParam_S noHook = MakeParam("ut", nullptr);
    EXPECT_EQ(SDF_ThreadCreate(&id, &noHook), SDF_THREAD_ERROR_INVALID_PARAM);

    SDF_ThreadCreateParam_S badPolicy = MakeParam("ut", HookNoop);
    badPolicy.policy = static_cast<SDF_ThreadSchedPolicy_E>(SDF_THREAD_SCHED_RR + 1);
    EXPECT_EQ(SDF_ThreadCreate(&id, &badPolicy), SDF_THREAD_ERROR_INVALID_PARAM);

    SDF_ThreadCreateParam_S badPriority = MakeParam("ut", HookNoop);
    badPriority.policy = SDF_THREAD_SCHED_FIFO;
    badPriority.priority = sched_get_priority_min(SDF_THREAD_SCHED_FIFO) - 1; // 低于策略下限
    EXPECT_EQ(SDF_ThreadCreate(&id, &badPriority), SDF_THREAD_ERROR_INVALID_PARAM);
}

TEST_F(SdfThreadEnvTest, CreateRejectsTooSmallStack)
{
    // 栈大小低于线程库下限时属性设置失败，创建报线程初始化错误并归还槽位
    uint32_t id = 0;
    SDF_ThreadCreateParam_S param = MakeParam("ut_small_stack", HookNoop);
    param.stackSize = 1;
    EXPECT_EQ(SDF_ThreadCreate(&id, &param), SDF_THREAD_ERROR_THREAD_INIT);

    SDF_ThreadCreateParam_S normal = MakeParam("ut_normal_stack", HookNoop);
    ASSERT_EQ(SDF_ThreadCreate(&id, &normal), SDF_OK);
    ASSERT_EQ(SDF_ThreadJoin(id), SDF_OK);
}

TEST_F(SdfThreadEnvTest, CreateRunJoinRoundTrip)
{
    // 创建后回调在线程内执行并取到有效自 id，主线程自 id 无效，join 回收
    g_hookDone = 0;
    g_selfIdInThread = 0;
    uint32_t id = 0;
    SDF_ThreadCreateParam_S param = MakeParam("ut_roundtrip", HookRecordSelf, 42);
    ASSERT_EQ(SDF_ThreadCreate(&id, &param), SDF_OK);
    EXPECT_NE(id, SDF_THREAD_INVALID_ID);

    ASSERT_EQ(SDF_ThreadJoin(id), SDF_OK);
    EXPECT_EQ(g_hookDone.load(), 42);
    EXPECT_NE(g_selfIdInThread.load(), SDF_THREAD_INVALID_ID);
    EXPECT_EQ(SDF_ThreadSelfId(), SDF_THREAD_INVALID_ID); // 主线程未注册
}

TEST_F(SdfThreadEnvTest, JoinRejectsInvalidAndUnusedIds)
{
    // 无效 id 与未使用槽位的 join 被拒绝
    EXPECT_EQ(SDF_ThreadJoin(SDF_THREAD_INVALID_ID), SDF_THREAD_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_ThreadJoin(99), SDF_THREAD_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_ThreadJoin(5), SDF_THREAD_ERROR_INVALID_THREADID); // 未创建线程的槽位
}

TEST_F(SdfThreadEnvTest, ExhaustedFreelistRejectsCreate)
{
    // 线程槽耗尽后创建被拒；join 释放槽位后可再次创建
    uint32_t idA = 0;
    uint32_t idB = 0;
    SDF_ThreadCreateParam_S param = MakeParam("ut_only", HookNoop);

    SDF_ThreadDeinit();
    ASSERT_EQ(SDF_ThreadInit(1), SDF_OK); // 单槽环境
    ASSERT_EQ(SDF_ThreadCreate(&idA, &param), SDF_OK);
    EXPECT_EQ(SDF_ThreadCreate(&idB, &param), SDF_THREAD_ERROR_INVALID_PARAM);
    ASSERT_EQ(SDF_ThreadJoin(idA), SDF_OK);
    EXPECT_EQ(SDF_ThreadCreate(&idB, &param), SDF_OK);
    ASSERT_EQ(SDF_ThreadJoin(idB), SDF_OK);

    SDF_ThreadDeinit();
    ASSERT_EQ(SDF_ThreadInit(8), SDF_OK); // 恢复夹具环境供后续用例
}

TEST_F(SdfThreadEnvTest, LocalKeyLifecycle)
{
    // 线程局部键空指针被拒；正常路径设置、读取、删除闭环
    // 删除后再操作的行为 pthread 实现不一（musl 宽松 glibc 报错），不作断言
    int value = 7;
    SDF_ThreadLocalKey key = nullptr;
    EXPECT_EQ(SDF_ThreadLocalKeyInit(nullptr, nullptr), SDF_THREAD_ERROR_INVALID_PARAM);
    ASSERT_EQ(SDF_ThreadLocalKeyInit(&key, nullptr), SDF_OK);
    ASSERT_EQ(SDF_ThreadSetLocal(key, &value), SDF_OK);
    EXPECT_EQ(SDF_ThreadGetLocal(key), &value);
    ASSERT_EQ(SDF_ThreadCleanupLocal(key), SDF_OK);
}

TEST_F(SdfThreadEnvTest, SleepAndYieldAreSafe)
{
    // 让出与毫秒级睡眠是空操作语义，调用即通过
    SDF_ThreadYield();
    SDF_ThreadSleep(1);
    SUCCEED();
}

TEST(SdfWorkerTest, AddWorkRejectsNullWorker)
{
    // 空工作队列添加任务失败
    int arg = 0;
    EXPECT_NE(SDF_AddWork(nullptr, nullptr, &arg, nullptr), SDF_OK);
}

TEST(SdfWorkerTest, RunOnceToleratesNullWorker)
{
    // 空工作队列执行是空操作
    SDF_WorkerRunOnce(nullptr);
}

namespace {
std::atomic<int> g_workCalls{0};
std::atomic<int> g_freeCalls{0};
int g_lastWorkArg = 0;

void CountingWork(void *arg)
{
    g_workCalls++;
    g_lastWorkArg = *static_cast<int *>(arg);
}

void CountingFreeArg(void *arg)
{
    g_freeCalls++;
}
} // namespace

TEST(SdfWorkerTest, RunOnceDrainsQueueInFifoOrder)
{
    // 任务按入队顺序执行并被释放；回调为空的任务只走释放回调
    SDF_Worker_S *worker = SDF_CreateWorker();
    ASSERT_NE(worker, nullptr);

    g_workCalls = 0;
    g_lastWorkArg = 0;
    int first = 1;
    int second = 2;
    ASSERT_EQ(SDF_AddWork(worker, CountingWork, &first, nullptr), SDF_OK);
    ASSERT_EQ(SDF_AddWork(worker, CountingWork, &second, nullptr), SDF_OK);
    int noCb = 3;
    ASSERT_EQ(SDF_AddWork(worker, nullptr, &noCb, CountingFreeArg), SDF_OK);

    SDF_WorkerRunOnce(worker);
    EXPECT_EQ(g_workCalls.load(), 2);
    EXPECT_EQ(g_lastWorkArg, 2); // 后入队后执行
    SDF_WorkerRunOnce(worker); // 空队列再跑一次
    SDF_DestroyWorker(worker);
}

TEST(SdfWorkerTest, DestroyFreesPendingWorksWithHook)
{
    // 销毁未清空的工作队列时逐个调用释放回调并回收内存
    SDF_Worker_S *worker = SDF_CreateWorker();
    ASSERT_NE(worker, nullptr);

    g_freeCalls = 0;
    int arg = 0;
    ASSERT_EQ(SDF_AddWork(worker, nullptr, &arg, CountingFreeArg), SDF_OK);
    ASSERT_EQ(SDF_AddWork(worker, nullptr, &arg, CountingFreeArg), SDF_OK);
    SDF_DestroyWorker(worker);
    EXPECT_EQ(g_freeCalls.load(), 2);
    SDF_DestroyWorker(nullptr);
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

class SdfThreadFaultTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_EQ(SDF_ThreadInit(4), SDF_OK);
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
        SDF_ThreadDeinit();
    }
    SDF_MemHooks_S m_saved = {};
};

TEST_F(SdfThreadFaultTest, CreateDupnameAllocFails)
{
    // 线程名复制分配失败时创建报内存错误并归还槽位，恢复后可创建
    uint32_t id = 0;
    SDF_ThreadCreateParam_S param = MakeParam("ut_dup_fail", HookNoop);

    g_allocSkip = 0;
    EXPECT_EQ(SDF_ThreadCreate(&id, &param), SDF_THREAD_ERROR_MEM_ALLOC);
    g_allocSkip = -1;

    ASSERT_EQ(SDF_ThreadCreate(&id, &param), SDF_OK);
    ASSERT_EQ(SDF_ThreadJoin(id), SDF_OK);
}

TEST_F(SdfThreadFaultTest, InitDescAllocFails)
{
    // 描述数组分配失败时初始化报内存错误，恢复后可正常初始化
    SDF_ThreadDeinit();

    g_allocSkip = 0;
    EXPECT_EQ(SDF_ThreadInit(2), SDF_THREAD_ERROR_MEM_ALLOC);
    g_allocSkip = -1;
    EXPECT_EQ(SDF_ThreadInit(2), SDF_OK);
}
