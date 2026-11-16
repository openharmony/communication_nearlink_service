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

/* dli 回环层与线程投递用例：回环初始化检查、命令回环应答、按包长分发、阻塞投递的失败兜底 */
#include <gtest/gtest.h>
#include "securec.h"

#include "sdf_mem.h"
#include "sdf_buff.h"
#include "sdf_evc.h"
#include "sdf_thread.h"
#include "sdf_sem.h"
#include "dli_full_memhook.h"

#include "dli.h"
#include "dli_cmd_struct.h"
#include "dli_errno.h"
#include "dli_opcode.h"
#include "dli_layer_stru.h"
#include "dli_layer_utils.h"
#include "dli_layer_callback.h"
#include "dli_thread.h"
#include "dli_sapi.h"
#include "dli_data_stub.h"

#define ST_TEST_SLEEP_MS 150
#define ST_TEST_PK_CAP 8

static volatile int g_stPkCount = 0;
static volatile int g_stInitDoneCount = 0;
static volatile bool g_stHalInited = false;
static volatile int g_stWorkCount = 0;
static volatile int g_stFreeArgCount = 0;
static SlePacketType g_stPkTypes[ST_TEST_PK_CAP];
static uint32_t g_stPkSizes[ST_TEST_PK_CAP];
static uint8_t g_stPkPbs[ST_TEST_PK_CAP];

static void StCountReset()
{
    g_stPkCount = 0;
    g_stInitDoneCount = 0;
    g_stWorkCount = 0;
    g_stFreeArgCount = 0;
    for (uint32_t i = 0; i < ST_TEST_PK_CAP; i++) {
        g_stPkTypes[i] = (SlePacketType)0;
        g_stPkSizes[i] = 0;
        g_stPkPbs[i] = 0xFF;
    }
}

static void StInitComplete(SleInitStatus status)
{
    if (status == SUCCESS) {
        g_stInitDoneCount++;
    }
}

static void StPkReceived(SlePacketType type, const SlePacket *packet)
{
    if (packet == NULL || g_stPkCount >= ST_TEST_PK_CAP) {
        return;
    }
    g_stPkTypes[g_stPkCount] = type;
    g_stPkSizes[g_stPkCount] = packet->size;
    if (packet->data != NULL && packet->size >= 2) {
        g_stPkPbs[g_stPkCount] = (uint8_t)((DLI_DECODE2BYTE(packet->data) >> 12) & 0x03);
    }
    g_stPkCount++;
}

static void StWorkCb(void *arg)
{
    (void)arg;
    g_stWorkCount++;
}

static void StFreeArg(void *arg)
{
    (void)arg;
    g_stFreeArgCount++;
}

static uint32_t StSyncPost(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (cb != NULL) {
        cb(arg);
    }
    if (freeCb != NULL && arg != NULL) {
        freeCb(arg);
    }
    return 0;
}

static uint32_t StSyncPostBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    (void)timeout;
    return StSyncPost(cb, arg, freeCb);
}

static void StRecvAcb(uint16_t lcid, SDF_Buff_S *buf)
{
    (void)lcid;
    (void)buf;
}

static uint32_t StSemInitFailStub(SDF_Sem pSem, int flag, uint32_t value)
{
    (void)pSem;
    (void)flag;
    (void)value;
    return 1;
}

static void StHalInit()
{
    SleDliCallbackFunc callbacks;
    callbacks.dliPacketReceived = StPkReceived;
    callbacks.initializationComplete = StInitComplete;
    ASSERT_EQ(0, SleHalInit(&callbacks));
    ASSERT_EQ(1, g_stInitDoneCount);
    g_stHalInited = true;
}

class UT_DLI_STUBHAL : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_EQ(0, SDF_EvcInit());
        EXPECT_EQ(0, SDF_ThreadInit(2));
    }

    static void TearDownTestCase()
    {
        SDF_ThreadDeinit();
        SDF_EvcDeinit();
    }

    void SetUp()
    {
        StCountReset();
        g_stHalInited = false;
    }

    void TearDown()
    {
        if (g_stHalInited) {
            SleHalClose();
            g_stHalInited = false;
        }
    }
};

class UT_DLI_THREADBND : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_EQ(0, SDF_EvcInit());
        EXPECT_EQ(0, SDF_ThreadInit(2));
    }

    static void TearDownTestCase()
    {
        SDF_ThreadDeinit();
        SDF_EvcDeinit();
    }

    void SetUp()
    {
        StCountReset();
        ASSERT_EQ(0, DLI_ThreadInit());
        DLI_Callback cbk = {0};
        cbk.postOtherThread = StSyncPost;
        cbk.postOtherBlockedThread = StSyncPostBlocked;
        cbk.recvAcbHandler = StRecvAcb;
        DLI_SetCallback(&cbk);
    }

    void TearDown()
    {
        DLI_ThreadDeinit();
        DLI_SetCallback(NULL);
    }
};

// 校验回环层初始化检查：空回调、缺收包函数、缺完成函数、重复初始化都被拒绝，完整回调注册成功
TEST_F(UT_DLI_STUBHAL, TestCaseHalInitParam)
{
    EXPECT_EQ(INITIALIZATION_ERROR, SleHalInit(NULL));

    SleDliCallbackFunc callbacks;
    (void)memset_s(&callbacks, sizeof(callbacks), 0, sizeof(callbacks));
    callbacks.initializationComplete = StInitComplete;
    EXPECT_EQ(INITIALIZATION_ERROR, SleHalInit(&callbacks));

    (void)memset_s(&callbacks, sizeof(callbacks), 0, sizeof(callbacks));
    callbacks.dliPacketReceived = StPkReceived;
    EXPECT_EQ(INITIALIZATION_ERROR, SleHalInit(&callbacks));

    StHalInit();
    EXPECT_EQ(INITIALIZATION_ERROR, SleHalInit(&callbacks));
}

// 校验回环层的收包闸门：未初始化时收包入口直接返回，初始化后同一入口能把包送到注册的回调
TEST_F(UT_DLI_STUBHAL, TestCaseSendToDliStubGate)
{
    uint8_t data[6] = {0xA3, 0x00, 0x11, 0x00, 0x02, 0x00};
    SlePacket packet = {data, sizeof(data)};
    SleSendToDliStub(PACKET_TYPE_SLE_ACB, &packet);
    EXPECT_EQ(0, g_stPkCount);

    StHalInit();
    SleSendToDliStub(PACKET_TYPE_SLE_ACB, &packet);
    EXPECT_EQ(1, g_stPkCount);
    EXPECT_EQ(PACKET_TYPE_SLE_ACB, g_stPkTypes[0]);
}

// 校验发包入口的参数检查：空包与空数据指针被拒绝，合法包提交给回环工作线程
TEST_F(UT_DLI_STUBHAL, TestCaseSendDliPacketParam)
{
    StHalInit();
    EXPECT_EQ(-1, SleSendDliPacket(NULL));

    SlePacket empty = {NULL, 4};
    EXPECT_EQ(-1, SleSendDliPacket(&empty));

    uint8_t data[4] = {PACKET_TYPE_SLE_ICB, 0, 0, 0};
    SlePacket packet = {data, sizeof(data)};
    EXPECT_EQ(0, SleSendDliPacket(&packet));
    SDF_ThreadSleep(ST_TEST_SLEEP_MS);
}

// 校验命令回环应答：已知命令触发预置应答事件，未知命令不产生任何回包
TEST_F(UT_DLI_STUBHAL, TestCaseCmdLoopReply)
{
    StHalInit();
    uint8_t known[3] = {PACKET_TYPE_SLE_CMD, 0x06, 0x0C};
    SlePacket knownCmd = {known, sizeof(known)};
    EXPECT_EQ(0, SleSendDliPacket(&knownCmd));
    SDF_ThreadSleep(ST_TEST_SLEEP_MS);
    EXPECT_EQ(1, g_stPkCount);
    EXPECT_EQ(PACKET_TYPE_SLE_EVENT, g_stPkTypes[0]);
    EXPECT_EQ(5, g_stPkSizes[0]);

    uint8_t unknown[3] = {PACKET_TYPE_SLE_CMD, 0x99, 0x99};
    SlePacket unknownCmd = {unknown, sizeof(unknown)};
    EXPECT_EQ(0, SleSendDliPacket(&unknownCmd));
    SDF_ThreadSleep(ST_TEST_SLEEP_MS);
    EXPECT_EQ(1, g_stPkCount);
}

// 校验回环层按包长分发：完整长度包触发全部预置应答，短包全部早退，分片长度包回放分片序列
TEST_F(UT_DLI_STUBHAL, TestCaseAcbSizeMatrix)
{
    StHalInit();
    uint8_t full[17] = {PACKET_TYPE_SLE_ACB};
    SlePacket fullPkt = {full, sizeof(full)};
    EXPECT_EQ(0, SleSendDliPacket(&fullPkt));
    SDF_ThreadSleep(ST_TEST_SLEEP_MS);
    EXPECT_EQ(5, g_stPkCount);

    StCountReset();
    uint8_t shortBuf[5] = {PACKET_TYPE_SLE_ACB};
    SlePacket shortPkt = {shortBuf, sizeof(shortBuf)};
    EXPECT_EQ(0, SleSendDliPacket(&shortPkt));
    SDF_ThreadSleep(ST_TEST_SLEEP_MS);
    EXPECT_EQ(0, g_stPkCount);

    StCountReset();
    uint8_t frag[16] = {PACKET_TYPE_SLE_ACB};
    SlePacket fragPkt = {frag, sizeof(frag)};
    EXPECT_EQ(0, SleSendDliPacket(&fragPkt));
    SDF_ThreadSleep(ST_TEST_SLEEP_MS);
    EXPECT_EQ(5, g_stPkCount);
    EXPECT_EQ(DLI_FULL_FRAGMENT, g_stPkPbs[0]);
    EXPECT_EQ(DLI_LAST_FRAGMENT, g_stPkPbs[1]);
    EXPECT_EQ(DLI_MIDDLE_FRAGMENT, g_stPkPbs[2]);
    EXPECT_EQ(DLI_MIDDLE_FRAGMENT, g_stPkPbs[3]);
    EXPECT_EQ(DLI_FIRST_FRAGMENT, g_stPkPbs[4]);
}

// 校验阻塞投递的空任务体：任务函数为空时只发信号量不执行任务，投递方在超时内正常返回
TEST_F(UT_DLI_THREADBND, TestCaseBlockPostNullArg)
{
    EXPECT_EQ(DLI_SUCCESS, DLI_BlockPostTask(NULL, NULL, NULL, 3000));
}

// 校验阻塞投递的分配失败兜底：任务或信号量内存不足、信号量初始化失败时入参被释放并返回错误
TEST_F(UT_DLI_THREADBND, TestCaseBlockPostAllocFail)
{
    uint32_t arg = 0x1234;

    // 任务体申请失败且带释放函数：入参被补偿释放
    DhMemHooksInstall();
    g_dhFailAt = 1;
    EXPECT_NE(DLI_SUCCESS, DLI_BlockPostTask(StWorkCb, &arg, StFreeArg, 1));
    EXPECT_EQ(1, g_stFreeArgCount);
    DhMemHooksRestore();

    // 任务体申请失败且无释放函数：直接返回错误
    DhMemHooksInstall();
    g_dhFailAt = 1;
    EXPECT_NE(DLI_SUCCESS, DLI_BlockPostTask(StWorkCb, &arg, NULL, 1));
    DhMemHooksRestore();

    // 信号量内存申请失败：任务体连带释放
    DhMemHooksInstall();
    g_dhFailAt = 2;
    g_stFreeArgCount = 0;
    EXPECT_NE(DLI_SUCCESS, DLI_BlockPostTask(StWorkCb, &arg, StFreeArg, 1));
    EXPECT_EQ(1, g_stFreeArgCount);
    DhMemHooksRestore();

    SDF_SemInitHook originHook = g_semHooks.initHook;
    g_semHooks.initHook = StSemInitFailStub;
    g_stFreeArgCount = 0;
    EXPECT_NE(DLI_SUCCESS, DLI_BlockPostTask(StWorkCb, &arg, StFreeArg, 1));
    EXPECT_EQ(1, g_stFreeArgCount);
    g_semHooks.initHook = originHook;
}

// 校验跨线程投递入口的兜底：未注册投递回调时按规则释放入参并返回错误，注册后正常转发
TEST_F(UT_DLI_THREADBND, TestCasePostOtherFreeArg)
{
    DLI_SetCallback(NULL);
    uint32_t arg = 0x5678;
    EXPECT_NE(DLI_SUCCESS, DLI_PostOtherThread(StWorkCb, &arg, StFreeArg));
    EXPECT_EQ(1, g_stFreeArgCount);
    EXPECT_NE(DLI_SUCCESS, DLI_PostOtherThread(StWorkCb, &arg, NULL));
    EXPECT_EQ(1, g_stFreeArgCount);
    EXPECT_NE(DLI_SUCCESS, DLI_PostOtherBlockedThread(StWorkCb, &arg, StFreeArg, 1));
    EXPECT_EQ(2, g_stFreeArgCount);

    DLI_Callback cbk = {0};
    cbk.postOtherThread = StSyncPost;
    cbk.postOtherBlockedThread = StSyncPostBlocked;
    cbk.recvAcbHandler = StRecvAcb;
    DLI_SetCallback(&cbk);
    EXPECT_EQ(DLI_SUCCESS, DLI_PostOtherThread(StWorkCb, NULL, NULL));
    EXPECT_EQ(1, g_stWorkCount);
}
