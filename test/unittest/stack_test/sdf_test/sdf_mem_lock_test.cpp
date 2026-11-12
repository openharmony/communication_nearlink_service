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

#include "sdf_lock_err.h"
#include "sdf_mem.h"
#include "sdf_mem_api.h"
#include "sdf_mutex.h"
#include "sdf_sem.h"

extern SDF_MemHooks_S g_stMemfunc;

namespace {
int g_allocCalls = 0;
int g_freeCalls = 0;
int g_semPostCalls = 0;

void *CountingAlloc(size_t size)
{
    // 直接用系统分配：注册后 SDF_MemAlloc 已指向本钩子，经它会无限递归
    g_allocCalls++;
    return malloc(size);
}

void CountingFree(void *ptr)
{
    g_freeCalls++;
    free(ptr);
}

uint32_t CountingSemPost(SDF_Sem pSem)
{
    g_semPostCalls++;
    return SDF_OK;
}

uint32_t StubSemInit(SDF_Sem pSem, int flag, uint32_t value)
{
    return SDF_OK;
}

void StubSemVoid(SDF_Sem pSem) {}

uint32_t StubSemWait(SDF_Sem pSem)
{
    return SDF_OK;
}

uint32_t StubSemTimeWait(SDF_Sem pSem, int timeout)
{
    return SDF_OK;
}
} // namespace

TEST(SdfMemTest, AlignAllocTreatsZeroAlignAsOne)
{
    // 对齐值 0 按 1 处理，分配释放流程安全完成
    void *ptr = SDF_MemAlignAlloc(32, 0);
    ASSERT_NE(ptr, nullptr);
    SDF_MemAlignFree(ptr);
}

TEST(SdfMemTest, AlignAllocHonorsAlignment)
{
    // 指定对齐值时返回地址按该值对齐，写入读回不丢失
    void *ptr = SDF_MemAlignAlloc(64, 64);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr) % 64, 0U);
    static_cast<uint8_t *>(ptr)[0] = 0x5A;
    EXPECT_EQ(static_cast<uint8_t *>(ptr)[0], 0x5A);
    SDF_MemAlignFree(ptr);
    SDF_MemAlignFree(nullptr);
}

TEST(SdfMemTest, FreeToleratesNull)
{
    // 空指针释放是空操作
    SDF_MemFree(nullptr);
}

TEST(SdfMemTest, ZallocReturnsZeroedLargeBlock)
{
    // 大块分配整体清零，任意抽样位置都是 0
    const size_t bigSize = 4 * 1024 * 1024;
    uint8_t *block = static_cast<uint8_t *>(SDF_MemZalloc(bigSize));
    ASSERT_NE(block, nullptr);
    for (size_t i = 0; i < bigSize; i += 4096) {
        EXPECT_EQ(block[i], 0);
    }
    SDF_MemFree(block);
}

TEST(SdfMemTest, HookRegRejectsIncompleteHooks)
{
    // 钩子结构缺失任一函数指针时注册被拒绝
    SDF_MemHooks_S missingAlloc = {.mAlloc = nullptr, .mFree = SDF_MemFree};
    SDF_MemHooks_S missingFree = {.mAlloc = SDF_MemAlloc, .mFree = nullptr};
    EXPECT_EQ(SDF_MemHookReg(nullptr), SDF_MEMM_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_MemHookReg(&missingAlloc), SDF_MEMM_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_MemHookReg(&missingFree), SDF_MEMM_ERROR_INVALID_PARAM);
}

TEST(SdfMemTest, HookRegRoutesAllocThroughCustomHooks)
{
    // 注册自定义钩子后分配释放经过钩子转发；注销恢复默认路径
    SDF_MemHooks_S saved = g_stMemfunc;
    SDF_MemHooks_S counting = {.mAlloc = CountingAlloc, .mFree = CountingFree};
    ASSERT_EQ(SDF_MemHookReg(&counting), SDF_OK);

    g_allocCalls = 0;
    g_freeCalls = 0;
    void *ptr = SDF_MemZalloc(16);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(g_allocCalls, 1);
    SDF_MemFree(ptr);
    EXPECT_EQ(g_freeCalls, 1);

    g_stMemfunc = saved; // 恢复默认钩子，避免影响同进程其他套件
    ptr = SDF_MemZalloc(16);
    ASSERT_NE(ptr, nullptr);
    EXPECT_EQ(g_allocCalls, 1);
    SDF_MemFree(ptr);
}

TEST(SdfMutexTest, HooksRejectNullTargets)
{
    // 锁与属性操作对空指针统一返回非法参数
    EXPECT_EQ(SDF_MutexInit(nullptr, nullptr), SDF_LOCK_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_MutexLock(nullptr), SDF_LOCK_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_MutexTryLock(nullptr), SDF_LOCK_ERROR_INVALID_PARAM);
    SDF_MutexDeinit(nullptr);
    SDF_MutexUnlock(nullptr);
    EXPECT_EQ(g_mutexLockHooks.attrInitHook(nullptr), SDF_LOCK_ERROR_INVALID_PARAM);
    EXPECT_EQ(g_mutexLockHooks.attrSetHook(nullptr, SDF_MUTEX_NORMAL), SDF_LOCK_ERROR_INVALID_PARAM);
}

TEST(SdfMutexTest, RecursiveMutexLocksNested)
{
    // 递归互斥锁空指针被拒；同一线程可重复加锁并按次数解锁
    EXPECT_EQ(SDF_RecursiveMutexInit(nullptr), SDF_LOCK_ERROR_INVALID_PARAM);

    void *buf = SDF_MemZalloc(SDF_MUTEX_LOCK_SIZE);
    ASSERT_NE(buf, nullptr);
    SDF_MutexLock mutex = static_cast<SDF_MutexLock>(buf);
    ASSERT_EQ(SDF_RecursiveMutexInit(mutex), SDF_OK);
    EXPECT_EQ(SDF_MutexLock(mutex), SDF_OK);
    EXPECT_EQ(SDF_MutexLock(mutex), SDF_OK); // 递归加锁
    EXPECT_EQ(SDF_MutexTryLock(mutex), SDF_OK); // 持有者尝试加锁仍成功
    SDF_MutexUnlock(mutex);
    SDF_MutexUnlock(mutex);
    SDF_MutexUnlock(mutex);
    SDF_MutexDeinit(mutex);
    SDF_MemFree(buf);
}

TEST(SdfMutexTest, NormalMutexTryLockContended)
{
    // 普通锁被持有时同线程尝试加锁失败，解锁后可再加
    void *buf = SDF_MemZalloc(SDF_MUTEX_LOCK_SIZE);
    ASSERT_NE(buf, nullptr);
    SDF_MutexLock mutex = static_cast<SDF_MutexLock>(buf);

    // 属性对象用定长栈缓冲承载，平台属性结构不大于互斥锁结构
    uint8_t attrSpace[64] = {0};
    SDF_MutexLockAttr attr = static_cast<SDF_MutexLockAttr>(attrSpace);
    ASSERT_EQ(g_mutexLockHooks.attrInitHook(attr), SDF_OK);
    ASSERT_EQ(g_mutexLockHooks.attrSetHook(attr, SDF_MUTEX_NORMAL), SDF_OK);
    ASSERT_EQ(SDF_MutexInit(mutex, attr), SDF_OK);

    EXPECT_EQ(SDF_MutexLock(mutex), SDF_OK);
    EXPECT_NE(SDF_MutexTryLock(mutex), SDF_OK); // 已被持有，尝试失败
    SDF_MutexUnlock(mutex);
    EXPECT_EQ(SDF_MutexTryLock(mutex), SDF_OK);
    SDF_MutexUnlock(mutex);

    SDF_MutexDeinit(mutex);
    SDF_MemFree(buf);
}

class SdfSemTest : public testing::Test {
protected:
    void SetUp() override
    {
        sem = static_cast<SDF_Sem>(SDF_MemZalloc(SDF_SEM_SIZE));
        ASSERT_NE(sem, nullptr);
    }
    void TearDown() override
    {
        if (sem != nullptr) {
            SDF_SemDeinit(sem);
            SDF_MemFree(sem);
            sem = nullptr;
        }
    }
    SDF_Sem sem = nullptr;
};

TEST_F(SdfSemTest, InitRejectsNullAndBadFlag)
{
    // 空指针或类型越界的初始化被拒绝
    EXPECT_EQ(SDF_SemInit(nullptr, SDF_SEM_COUNT_TYPE, 1), SDF_LOCK_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_SemInit(sem, SDF_SEM_TYPE_BUTT, 1), SDF_LOCK_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_SemInit(sem, -1, 1), SDF_LOCK_ERROR_INVALID_PARAM);
}

TEST_F(SdfSemTest, BinaryInitRejectsValueAboveOne)
{
    // 二值信号量初值大于 1 被拒绝
    EXPECT_EQ(SDF_SemInit(sem, SDF_SEM_BINARY_TYPE, 2), SDF_LOCK_ERROR_SEM_INIT_FAIL);
}

TEST_F(SdfSemTest, CountSemWaitPostCycle)
{
    // 计数信号量初值 1：等待消耗、投递补回，空指针操作被拒
    ASSERT_EQ(SDF_SemInit(sem, SDF_SEM_COUNT_TYPE, 1), SDF_OK);
    EXPECT_EQ(SDF_SemWait(nullptr), SDF_LOCK_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_SemWait(sem), SDF_OK);
    EXPECT_EQ(SDF_SemTryWait(sem), SDF_LOCK_ERROR_SEM_WAIT_FAIL); // 已空，尝试失败
    EXPECT_EQ(SDF_SemPost(nullptr), SDF_LOCK_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_SemPost(sem), SDF_OK);
    EXPECT_EQ(SDF_SemTryWait(sem), SDF_OK);
}

TEST_F(SdfSemTest, BinarySemPostSaturates)
{
    // 二值信号量在 1 时再投递被拒，避免越过二值上限
    ASSERT_EQ(SDF_SemInit(sem, SDF_SEM_BINARY_TYPE, 1), SDF_OK);
    EXPECT_EQ(SDF_SemPost(sem), SDF_LOCK_ERROR_SEM_POST_FAIL);
    EXPECT_EQ(SDF_SemWait(sem), SDF_OK);
    EXPECT_EQ(SDF_SemPost(sem), SDF_OK);
}

TEST_F(SdfSemTest, TimeWaitTimesOutWhenEmpty)
{
    // 空信号量限时等待到期返回失败；空指针立即被拒
    ASSERT_EQ(SDF_SemInit(sem, SDF_SEM_COUNT_TYPE, 0), SDF_OK);
    EXPECT_EQ(SDF_SemTimeWait(nullptr, 10), SDF_LOCK_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_SemTimeWait(sem, 50), SDF_LOCK_ERROR_TIME_FAIL);
}

TEST_F(SdfSemTest, TimeWaitForeverConsumesToken)
{
    // 负超时走无限等待路径，有令牌时立即成功
    ASSERT_EQ(SDF_SemInit(sem, SDF_SEM_COUNT_TYPE, 1), SDF_OK);
    EXPECT_EQ(SDF_SemTimeWait(sem, SEM_ALWAYS_WAIT), SDF_OK);
}

TEST_F(SdfSemTest, DeinitToleratesNull)
{
    // 空指针销毁是空操作
    SDF_SemDeinit(nullptr);
}

TEST_F(SdfSemTest, RegRejectsNullAndRoutesPost)
{
    // 空钩子注册被拒；注册自定义钩子后投递走新钩子，随后恢复默认
    SDF_SemHooks_S saved = g_semHooks;
    EXPECT_EQ(SDF_SemReg(nullptr), SDF_LOCK_ERROR_INVALID_PARAM);

    SDF_SemHooks_S stub = {};
    stub.initHook = StubSemInit;
    stub.deinitHook = StubSemVoid;
    stub.postHook = CountingSemPost;
    stub.waitHook = StubSemWait;
    stub.tryWaitHook = StubSemWait;
    stub.timeWaitHook = StubSemTimeWait;
    stub.semSize = saved.semSize;
    ASSERT_EQ(SDF_SemReg(&stub), SDF_OK);

    g_semPostCalls = 0;
    EXPECT_EQ(SDF_SemPost(nullptr), SDF_OK); // 走自定义投递钩子，不校验空指针
    EXPECT_EQ(g_semPostCalls, 1);

    g_semHooks = saved; // 恢复默认实现，避免影响同进程其他套件
    EXPECT_EQ(g_semPostCalls, 1);
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

TEST(SdfMemTest, AlignAllocFailsCleanlyWhenOom)
{
    // 底层分配失败时对齐分配返回空，恢复后可正常分配
    g_mainThread = pthread_self();
    SDF_MemHooks_S saved = g_stMemfunc;
    SDF_MemHooks_S hook = {.mAlloc = MainThreadFailAlloc, .mFree = saved.mFree};
    ASSERT_EQ(SDF_MemHookReg(&hook), SDF_OK);

    g_allocSkip = 0;
    EXPECT_EQ(SDF_MemAlignAlloc(64, 64), nullptr);
    g_allocSkip = -1;
    void *ptr = SDF_MemAlignAlloc(64, 64);
    ASSERT_NE(ptr, nullptr);
    SDF_MemAlignFree(ptr);

    g_stMemfunc = saved;
}

TEST_F(SdfSemTest, CountSemInitRejectsValueAboveLimit)
{
    // 计数信号量初值越过系统上限时初始化失败
    EXPECT_EQ(SDF_SemInit(sem, SDF_SEM_COUNT_TYPE, 0x80000000u), SDF_LOCK_ERROR_INIT_FAIL);
}
