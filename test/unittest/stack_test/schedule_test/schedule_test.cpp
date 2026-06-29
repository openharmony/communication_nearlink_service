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

#include "sdf_mem.h"

#include "sdf_event.h"
#include "sdf_worker.h"
#include "sdf_thread.h"
#include "sdf_sem.h"
#include "nlstk_init_api.h"
#include "nlstk_schedule.h"
#include "cp_worker.h"

using namespace testing;
using namespace testing::ext;

extern void* (*g_sdfMemZallocFunc)(size_t size);

static std::atomic<int> g_count(0);
static SDF_Sem g_sem = NULL;

class UT_SCHEDULE_TEST : public testing::Test {
protected:
    virtual void SetUp()
    {
        g_sem = (SDF_Sem)SDF_MemZalloc(SDF_SEM_SIZE);
        SDF_SemInit(g_sem, 0, 0);
    }
    virtual void TearDown()
    {
        SDF_MemFree(g_sem);
        g_sem = NULL;
    }
};

void TaskFunc(void *arg)
{
    g_count.store(100, std::memory_order_acquire);
    SDF_SemPost(g_sem);
}

TEST_F(UT_SCHEDULE_TEST, Test_ScheduleInit)
{
    uint32_t ret = ScheduleEnable();
    ASSERT_EQ(ret, NLSTK_ERR);

    SDF_ThreadInit(3);
    ret = ScheduleEnable();
    ASSERT_EQ(ret, NLSTK_ERR);

    SDF_EvcInit();
    ret = ScheduleEnable();
    ASSERT_EQ(ret, NLSTK_OK);
    ret = ScheduleEnable();
    ASSERT_EQ(ret, NLSTK_OK);

    ScheduleDisable();

    SDF_EvcDeinit();
    SDF_ThreadDeinit();
}

TEST_F(UT_SCHEDULE_TEST, Test_SchedulePostTask)
{
    SDF_ThreadInit(3);
    SDF_EvcInit();

    g_count.store(0);
    uint32_t ret = SchedulePostTask(TaskFunc, NULL, NULL);
    ASSERT_EQ(ret, NLSTK_ERR);
    ret = SchedulePostTaskBlocked(TaskFunc, NULL, NULL, 1000);
    ASSERT_EQ(ret, NLSTK_ERR);

    ret = ScheduleEnable();
    ASSERT_EQ(ret, NLSTK_OK);
    ret = SchedulePostTask(TaskFunc, NULL, NULL);
    ASSERT_EQ(ret, NLSTK_OK);
    SDF_SemTimeWait(g_sem, 1000);
    ASSERT_EQ(g_count.load(), 100);

    g_count.store(0);
    SDF_SemInit(g_sem, 0, 0);
    ret = SchedulePostTaskBlocked(TaskFunc, NULL, NULL, 1000);
    ASSERT_EQ(ret, NLSTK_OK);
    ASSERT_EQ(g_count.load(), 100);
    
    ScheduleDisable();
    SDF_EvcDeinit();
    SDF_ThreadDeinit();
}

TEST_F(UT_SCHEDULE_TEST, Test_ScheduleTimer)
{
    SDF_ThreadInit(3);
    SDF_EvcInit();
    ScheduleEnable();

    g_count.store(0);
    SDF_TimerParam param = {
        .expires = 1,
        .period = false,
        .callback = TaskFunc,
        .args = NULL,
    };
    int handle = 0;
    uint32_t ret = ScheduleTimerAdd(&handle, &param);
    ASSERT_EQ(ret, NLSTK_OK);
    
    SDF_SemTimeWait(g_sem, 1000);
    ASSERT_EQ(g_count.load(), 100);

    ScheduleDisable();
    SDF_EvcDeinit();
    SDF_ThreadDeinit();
}

TEST_F(UT_SCHEDULE_TEST, Test_CpWorker)
{
    SDF_ThreadInit(3);
    SDF_EvcInit();
    ScheduleEnable();

    g_count.store(0);
    uint32_t ret = CP_PostTask(TaskFunc, NULL, NULL);
    ASSERT_EQ(ret, NLSTK_OK);
    SDF_SemTimeWait(g_sem, 1000);
    ASSERT_EQ(g_count.load(), 100);

    g_count.store(0);
    SDF_SemInit(g_sem, 0, 0);
    ret = CP_PostTaskBlocked(TaskFunc, NULL, NULL, 1000);
    ASSERT_EQ(ret, NLSTK_OK);
    ASSERT_EQ(g_count.load(), 100);

    g_count.store(0);
    SDF_SemInit(g_sem, 0, 0);
    SDF_TimerParam param = {
        .expires = 1,
        .period = false,
        .callback = TaskFunc,
        .args = NULL,
    };
    int handle = 0;
    ret = CP_TimerAdd(&handle, &param);
    ASSERT_EQ(ret, NLSTK_OK);
    SDF_SemTimeWait(g_sem, 5000);
    ASSERT_EQ(g_count.load(), 100);

    ScheduleDisable();
    SDF_EvcDeinit();
    SDF_ThreadDeinit();
}

TEST_F(UT_SCHEDULE_TEST, Test_SchedulePostTaskFail)
{
    SDF_ThreadInit(3);
    SDF_EvcInit();
    uint32_t ret = ScheduleEnable();
    ASSERT_EQ(ret, NLSTK_OK);

    g_count.store(0);
    SDF_SemInit(g_sem, 0, 0);
    g_sdfMemZallocFunc = TEST_SDF_MemZallocFail;
    ret = SchedulePostTaskBlocked(TaskFunc, NULL, NULL, 1000);
    ASSERT_EQ(ret, NLSTK_ERR);
    g_sdfMemZallocFunc = TEST_SDF_MemZalloc;

    ScheduleDisable();
    SDF_EvcDeinit();
    SDF_ThreadDeinit();
}

TEST_F(UT_SCHEDULE_TEST, Test_SchedulePostTaskFail2)
{
    uint8_t *num = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    SDF_ThreadInit(3);
    SDF_EvcInit();
    uint32_t ret = ScheduleEnable();
    ASSERT_EQ(ret, NLSTK_OK);

    g_count.store(0);
    SDF_SemInit(g_sem, 0, 0);
    g_sdfMemZallocFunc = TEST_SDF_MemZallocFail;
    ret = SchedulePostTaskBlocked(TaskFunc, NULL, SDF_MemFree, 1000);
    ASSERT_EQ(ret, NLSTK_ERR);
    g_sdfMemZallocFunc = TEST_SDF_MemZalloc;
    if(num != NULL) {
        SDF_MemFree(num);
    }
    ScheduleDisable();
    SDF_EvcDeinit();
    SDF_ThreadDeinit();
}

