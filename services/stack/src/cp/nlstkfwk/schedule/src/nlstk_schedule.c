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
#include "sdf_evc.h"
#include "sdf_event.h"
#include "sdf_worker.h"
#include "sdf_thread.h"
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STACK_MAX_THREAD 2

int g_scheduleEvcHandle = -1;
int g_scheduleEventHandle = -1;
SDF_Worker_S *g_schedule = NULL;

static void ScheduleRunOnce(void *args)
{
    (void)args;
    SDF_WorkerRunOnce(g_schedule);
}

static uint32_t ScheduleThreadCreate(void)
{
    g_schedule = SDF_CreateWorker();
    if (g_schedule == NULL) {
        goto Fail;
    }
    if (SDF_EvcInstanceCreate(&g_scheduleEvcHandle, "SCHEDULE") != NLSTK_OK) {
        goto Fail;
    }
    SDF_EventParam param = {g_scheduleEvcHandle, ScheduleRunOnce, NULL};
    if (SDF_EventAdd(&g_scheduleEventHandle, &param) != NLSTK_OK) {
        goto Fail;
    }
    return NLSTK_OK;
Fail:
    if (g_scheduleEvcHandle >= 0) {
        SDF_EvcInstanceClose(g_scheduleEvcHandle);
        g_scheduleEvcHandle = -1;
    }
    if (g_schedule != NULL) {
        SDF_DestroyWorker(g_schedule);
        g_schedule = NULL;
    }
    return NLSTK_ERR;
}

uint32_t ScheduleEnable(void)
{
    if (g_schedule != NULL) {
        return NLSTK_OK;
    }
    return ScheduleThreadCreate();
}

void ScheduleDisable(void)
{
    SDF_EvcInstanceClose(g_scheduleEvcHandle);
    g_scheduleEventHandle = -1;
    g_scheduleEvcHandle = -1;
    SDF_DestroyWorker(g_schedule);
    g_schedule = NULL;
}

uint32_t SchedulePostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (g_schedule == NULL || g_scheduleEventHandle < 0) {
        goto Fail;
    }
    if (SDF_AddWork(g_schedule, cb, arg, freeCb) != NLSTK_OK) {
        goto Fail;
    }
    if (SDF_EventPost(g_scheduleEventHandle) != NLSTK_OK) {
        // 不返回失败，下次POST成功时任务依旧能执行
        NLSTK_LOG_ERROR("SDF_EventPost Failed.");
    }
    return NLSTK_OK;
Fail:
    if (freeCb != NULL) {
        freeCb(arg);
    }
    return NLSTK_ERR;
}

static void RunTaskBlocked(void *arg)
{
    SDF_BlockTask_S *task = (SDF_BlockTask_S *)arg;
    if (task == NULL) {
        return;
    }
    if (task->func != NULL) {
        task->func(task->arg);
    }
    if (task->sem != NULL) {
        SDF_SemPost(task->sem);
    }
}

static void FreeBlockTask(void *arg)
{
    SDF_BlockTask_S *task = (SDF_BlockTask_S *)arg;
    if (task == NULL) {
        return;
    }
    if (task->freeCb != NULL) {
        task->freeCb(task->arg);
    }
    SDF_MemFree(task);
}

static void FreeSem(void *arg)
{
    SDF_Sem sem = (SDF_Sem)arg;
    SDF_SemDeinit(sem);
    SDF_MemFree(sem);
}

uint32_t SchedulePostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    SDF_BlockTask_S *task = (SDF_BlockTask_S *)SDF_MemZalloc(sizeof(SDF_BlockTask_S));
    if (task == NULL) {
        if (freeCb != NULL) {
            freeCb(arg);
        }
        return NLSTK_ERR;
    }
    task->func = cb;
    task->freeCb = freeCb;
    task->arg = arg;
    SDF_Sem sem = (SDF_Sem)SDF_MemAlloc(SDF_SEM_SIZE);
    if (sem == NULL) {
        FreeBlockTask(task);
        return NLSTK_ERR;
    }
    task->sem = sem;
    if (SDF_SemInit(task->sem, 0, 0) != SDF_OK) {
        SDF_MemFree(sem);
        FreeBlockTask(task);
        return NLSTK_ERR;
    }
    if (SchedulePostTask(RunTaskBlocked, task, FreeBlockTask) != NLSTK_OK) {
        FreeSem(sem);
        return NLSTK_ERR;
    }
    if (SDF_SemTimeWait(sem, timeout) != SDF_OK) {
        if (SchedulePostTask(NULL, sem, FreeSem) != NLSTK_OK) {
            NLSTK_LOG_ERROR("SchedulePostTask FreeSem Failed.");
        }
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    }
    FreeSem(sem);

    return NLSTK_OK;
}

uint32_t ScheduleTimerAdd(int *handle, SDF_TimerParam *param)
{
    param->handle = g_scheduleEvcHandle;
    return SDF_TimerAdd(handle, param);
}

void ScheduleTimerDel(int handle)
{
    SDF_TimerDel(handle);
}

#ifdef __cplusplus
}
#endif