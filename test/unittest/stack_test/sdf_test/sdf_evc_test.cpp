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
#include <sys/eventfd.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "sdf_event.h"
#include "sdf_evc.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_timer.h"

extern SDF_MemHooks_S g_stMemfunc;

namespace {
std::atomic<int> g_eventCalls{0};
std::atomic<int> g_timerCalls{0};
std::atomic<int> g_pipeCalls{0};

void CountingEventCb(void *arg)
{
    (void)arg;
    g_eventCalls++;
}

void CountingTimerCb(void *arg)
{
    (void)arg;
    g_timerCalls++;
}

void DrainPipeCb(int handle, void *arg)
{
    (void)arg;
    char byte = 0;
    if (read(handle, &byte, 1) == 1) {
        g_pipeCalls++;
    }
}

uint32_t BumpArgs(void *arg)
{
    (static_cast<std::atomic<int> *>(arg))->operator++();
    return SDF_OK;
}

bool WaitDeadline(const std::atomic<int> &counter, int expect, int timeoutMs)
{
    for (int waited = 0; waited < timeoutMs; waited += 5) {
        if (counter.load() >= expect) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return counter.load() >= expect;
}
} // namespace

TEST(SdfEvcTest, OpsBeforeInitFailCleanly)
{
    // 未初始化时反初始化安全，监听、修改参数、删除事件、关闭实例均安全返回
    SDF_EvcDeinit();

    SDF_EvcEvent event = {};
    event.type = SDF_EVC_EVENT;
    event.eventHandle = 5;
    EXPECT_EQ(SDF_EvcListenEvent(5, &event), SDF_EVC_ERROR_WRONG_HANDLE);

    EXPECT_EQ(SDF_ModifyEventArgs(5, BumpArgs), SDF_EVC_ERROR_WRONG_HANDLE);
    EXPECT_EQ(SDF_ModifyEventArgs(-1, BumpArgs), SDF_EVC_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_ModifyEventArgs(5, nullptr), SDF_EVC_ERROR_INVALID_PARAM);

    SDF_EvcCancelEvent(-1);
    SDF_EvcCancelEvent(9999);

    int handle = 0;
    EXPECT_EQ(SDF_EvcInstanceCreate(nullptr, "ut"), SDF_EVC_ERROR_INVALID_PARAM);
    EXPECT_EQ(SDF_EvcInstanceCreate(&handle, nullptr), SDF_EVC_ERROR_INVALID_PARAM);
    SDF_EvcInstanceClose(-1);
}

class SdfEvcEnvTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_EQ(SDF_ThreadInit(4), SDF_OK);
        ASSERT_EQ(SDF_EvcInit(), SDF_OK);
    }
    void TearDown() override
    {
        SDF_EvcDeinit();
        SDF_ThreadDeinit();
    }
};

TEST_F(SdfEvcEnvTest, DoubleInitRejected)
{
    // 已初始化环境下再次初始化被拒
    EXPECT_EQ(SDF_EvcInit(), SDF_EVC_ERROR_ALREADY_INIT);
}

TEST_F(SdfEvcEnvTest, InstanceCreateHonorsMaxLimit)
{
    // 连续创建直到实例上限被拒（后台模块可能占用槽位，不预设空槽数量）；
    // 关闭全部自有实例后槽位可复用
    int mine[SDF_EVC_MAX_INSTANCE_NUM + 1] = {0};
    int held = 0;
    int extra = 0;
    for (int i = 0; i < SDF_EVC_MAX_INSTANCE_NUM + 1; i++) {
        int ret = SDF_EvcInstanceCreate(&mine[held], "ut_evc");
        if (ret == SDF_OK) {
            held++;
        } else {
            EXPECT_EQ(ret, SDF_EVC_ERROR_MAX_INSTANCE);
            extra = mine[held];
            break;
        }
    }
    EXPECT_GE(held, 1);

    for (int i = 0; i < held; i++) {
        SDF_EvcInstanceClose(mine[i]);
    }
    EXPECT_EQ(SDF_EvcInstanceCreate(&extra, "ut_reuse"), SDF_OK);
    SDF_EvcInstanceClose(extra);
}

TEST_F(SdfEvcEnvTest, ListenEventRejectsWrongHandleAndDuplicate)
{
    // 未知名柄监听失败；同一事件句柄重复监听被拒；删除后可重新监听
    int evcHandle = 0;
    ASSERT_EQ(SDF_EvcInstanceCreate(&evcHandle, "ut_listen"), SDF_OK);

    EXPECT_EQ(SDF_EvcListenEvent(-1, nullptr), SDF_EVC_ERROR_INVALID_PARAM);

    int eFd = eventfd(0, EFD_NONBLOCK);
    ASSERT_GE(eFd, 0);
    std::atomic<int> bumped{0};
    SDF_EvcEvent event = {};
    event.type = SDF_EVC_EVENT;
    event.eventHandle = eFd;
    event.callback = nullptr;
    event.args = &bumped; // 监听时整体拷贝，须在成功监听前填好
    event.freeFunc = nullptr;
    EXPECT_EQ(SDF_EvcListenEvent(12345, &event), SDF_EVC_ERROR_WRONG_HANDLE);
    EXPECT_EQ(SDF_EvcListenEvent(evcHandle, &event), SDF_OK);
    EXPECT_EQ(SDF_EvcListenEvent(evcHandle, &event), SDF_EVC_ERROR_ALREADY_LISTENED);

    EXPECT_EQ(SDF_ModifyEventArgs(eFd, BumpArgs), SDF_OK);
    EXPECT_EQ(bumped.load(), 1);

    SDF_EvcCancelEvent(eFd); // 删除后事件不再可寻
    EXPECT_EQ(SDF_ModifyEventArgs(eFd, BumpArgs), SDF_EVC_ERROR_WRONG_HANDLE);
    close(eFd);
    SDF_EvcInstanceClose(evcHandle);
}

TEST_F(SdfEvcEnvTest, EventPostDeliversAndDelStops)
{
    // 事件投递后由事件循环线程回调；删除事件后投递报未知句柄
    int evcHandle = 0;
    ASSERT_EQ(SDF_EvcInstanceCreate(&evcHandle, "ut_event"), SDF_OK);

    g_eventCalls = 0;
    SDF_EventParam param = {};
    param.handle = evcHandle;
    param.callback = CountingEventCb;
    param.args = nullptr;
    int eFd = 0;
    ASSERT_EQ(SDF_EventAdd(&eFd, &param), SDF_OK);
    ASSERT_GE(eFd, 0);

    // 连续投递会被合并成一次派发（事件循环每次唤醒只读一次计数并回调一次），
    // 按产品实际语义串行投递、逐次等待回调到齐
    EXPECT_EQ(SDF_EventPost(eFd), SDF_OK);
    EXPECT_TRUE(WaitDeadline(g_eventCalls, 1, 5000));
    EXPECT_EQ(SDF_EventPost(eFd), SDF_OK);
    EXPECT_TRUE(WaitDeadline(g_eventCalls, 2, 5000));

    SDF_EventDel(eFd);
    EXPECT_EQ(SDF_EventPost(eFd), SDF_EVC_ERROR_WRONG_HANDLE);
    SDF_EvcInstanceClose(evcHandle);
}

TEST_F(SdfEvcEnvTest, EventAddWithUnknownEvcHandleFails)
{
    // 挂到不存在的 evc 句柄上时事件创建失败
    SDF_EventParam param = {};
    param.handle = 12345;
    param.callback = CountingEventCb;
    param.args = nullptr;
    int eFd = 0;
    EXPECT_EQ(SDF_EventAdd(&eFd, &param), SDF_EVENT_ERROR_EVC_FAILED);
}

TEST_F(SdfEvcEnvTest, OneShotTimerFiresOnceAndSelfDeletes)
{
    // 单次定时器到期触发一次回调后自删，不再重复
    int evcHandle = 0;
    ASSERT_EQ(SDF_EvcInstanceCreate(&evcHandle, "ut_timer1"), SDF_OK);

    g_timerCalls = 0;
    SDF_TimerParam param = {};
    param.handle = evcHandle;
    param.expires = 10;
    param.period = false;
    param.callback = CountingTimerCb;
    param.args = nullptr;
    int tFd = 0;
    ASSERT_EQ(SDF_TimerAdd(&tFd, &param), SDF_OK);
    ASSERT_GE(tFd, 0);
    EXPECT_TRUE(WaitDeadline(g_timerCalls, 1, 5000));

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    EXPECT_EQ(g_timerCalls.load(), 1);
    // 不再对已自删的句柄重复删除：句柄已关闭，编号可能被复用成别的活动事件
    SDF_EvcInstanceClose(evcHandle);
}

TEST_F(SdfEvcEnvTest, PeriodicTimerRepeatsUntilDeleted)
{
    // 周期定时器连续触发，手动删除后停止；周期放宽到 200 毫秒以容忍
    // 高负载设备上的分发延迟
    int evcHandle = 0;
    ASSERT_EQ(SDF_EvcInstanceCreate(&evcHandle, "ut_timer2"), SDF_OK);

    g_timerCalls = 0;
    SDF_TimerParam param = {};
    param.handle = evcHandle;
    param.expires = 200;
    param.period = true;
    param.callback = CountingTimerCb;
    param.args = nullptr;
    int tFd = 0;
    ASSERT_EQ(SDF_TimerAdd(&tFd, &param), SDF_OK);
    EXPECT_TRUE(WaitDeadline(g_timerCalls, 2, 5000));

    SDF_TimerDel(tFd);
    int afterDelete = g_timerCalls.load();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // 删除瞬间可能有一次已在途的触发，只断言不再持续增长
    EXPECT_LE(g_timerCalls.load(), afterDelete + 1);
    SDF_EvcInstanceClose(evcHandle);
}

TEST_F(SdfEvcEnvTest, TimerWithoutCallbackIsSafe)
{
    // 无回调的周期定时器到期只走空回调臂，手动删除停止
    int evcHandle = 0;
    ASSERT_EQ(SDF_EvcInstanceCreate(&evcHandle, "ut_timer3"), SDF_OK);

    SDF_TimerParam param = {};
    param.handle = evcHandle;
    param.expires = 50;
    param.period = true;
    param.callback = nullptr;
    param.args = nullptr;
    int tFd = 0;
    ASSERT_EQ(SDF_TimerAdd(&tFd, &param), SDF_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    SDF_TimerDel(tFd);
    SDF_EvcInstanceClose(evcHandle);
}

TEST_F(SdfEvcEnvTest, TimerAddWithUnknownEvcHandleFails)
{
    // 挂到不存在的 evc 句柄上时定时器创建失败
    int evcHandle = 0;
    ASSERT_EQ(SDF_EvcInstanceCreate(&evcHandle, "ut_timer4"), SDF_OK);

    SDF_TimerParam param = {};
    param.handle = 12345;
    param.expires = 10;
    param.period = false;
    param.callback = CountingTimerCb;
    param.args = nullptr;
    int tFd = 0;
    EXPECT_EQ(SDF_TimerAdd(&tFd, &param), SDF_TIMER_ERROR_EVC_FAILED);
    SDF_EvcInstanceClose(evcHandle);
}

namespace {
pthread_t g_mainThread;
int g_allocSkip = -1;
int g_allocSeq = 0;

// 故障注入分配钩子：仅主线程从第 skip+1 次分配起失败，后台线程放行
void *MainThreadFailAlloc(size_t size)
{
    if (g_allocSkip >= 0 && pthread_equal(pthread_self(), g_mainThread) && ++g_allocSeq > g_allocSkip) {
        return nullptr;
    }
    return malloc(size);
}
} // namespace

class SdfEvcFaultTest : public testing::Test {
protected:
    void SetUp() override
    {
        ASSERT_EQ(SDF_ThreadInit(4), SDF_OK);
        ASSERT_EQ(SDF_EvcInit(), SDF_OK);
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
        SDF_EvcDeinit();
        SDF_ThreadDeinit();
    }
    void FailAfterMainAllocs(int skip)
    {
        g_allocSeq = 0;
        g_allocSkip = skip;
    }
    SDF_MemHooks_S m_saved = {};
};

TEST_F(SdfEvcFaultTest, InstanceCreateDescVectorAllocFails)
{
    // 描述符第二级分配（事件向量）失败时创建报内存错误，环境保持可用
    FailAfterMainAllocs(1);
    int handle = 0;
    EXPECT_EQ(SDF_EvcInstanceCreate(&handle, "ut_fault"), SDF_EVC_ERROR_MEM_FAILED);
    g_allocSkip = -1;
    EXPECT_EQ(SDF_EvcInstanceCreate(&handle, "ut_fault_ok"), SDF_OK);
    SDF_EvcInstanceClose(handle);
}

TEST_F(SdfEvcFaultTest, EventAddDescAllocFails)
{
    // 事件描述符分配失败时报内存错误，不影响既有事件循环
    int evcHandle = 0;
    ASSERT_EQ(SDF_EvcInstanceCreate(&evcHandle, "ut_evt_fault"), SDF_OK);

    FailAfterMainAllocs(0);
    SDF_EventParam param = {};
    param.handle = evcHandle;
    param.callback = CountingEventCb;
    param.args = nullptr;
    int eFd = 0;
    EXPECT_EQ(SDF_EventAdd(&eFd, &param), SDF_EVENT_ERROR_MEM_FAILED);
    g_allocSkip = -1;
    SDF_EvcInstanceClose(evcHandle);
}

TEST_F(SdfEvcFaultTest, EvcInitAllocFailurePathsClean)
{
    // 反初始化后分别注入首二级分配失败：初始化报内存错误并走回收分支，
    // 恢复分配后可重新初始化
    g_stMemfunc = m_saved;
    SDF_EvcDeinit();

    g_mainThread = pthread_self();
    SDF_MemHooks_S hook = {.mAlloc = MainThreadFailAlloc, .mFree = m_saved.mFree};
    ASSERT_EQ(SDF_MemHookReg(&hook), SDF_OK);
    FailAfterMainAllocs(0);
    EXPECT_EQ(SDF_EvcInit(), SDF_EVC_ERROR_MEM_FAILED); // 描述向量分配失败
    FailAfterMainAllocs(1);
    EXPECT_EQ(SDF_EvcInit(), SDF_EVC_ERROR_MEM_FAILED); // 锁分配失败，向量已建被回收
    g_allocSkip = -1;
    EXPECT_EQ(SDF_EvcInit(), SDF_OK);
}

TEST_F(SdfEvcEnvTest, UserEventCallbackConsumesItself)
{
    // 用户事件分发不代读句柄，由回调自行消费就绪数据；管道写一次触发一次
    int evcHandle = 0;
    ASSERT_EQ(SDF_EvcInstanceCreate(&evcHandle, "ut_user"), SDF_OK);
    int fds[2] = {-1, -1};
    ASSERT_EQ(pipe(fds), 0);
    SDF_EvcEvent event = {};
    event.type = SDF_EVC_USER_EVENT;
    event.eventHandle = fds[0];
    event.callback = DrainPipeCb;
    event.args = nullptr;
    event.freeFunc = nullptr;
    ASSERT_EQ(SDF_EvcListenEvent(evcHandle, &event), SDF_OK);

    g_pipeCalls = 0;
    ASSERT_EQ(write(fds[1], "x", 1), 1);
    EXPECT_TRUE(WaitDeadline(g_pipeCalls, 1, 5000));

    SDF_EvcCancelEvent(fds[0]);
    close(fds[0]);
    close(fds[1]);
    SDF_EvcInstanceClose(evcHandle);
}
