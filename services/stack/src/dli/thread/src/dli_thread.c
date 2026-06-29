/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "dli_thread.h"

#include "securec.h"
#include "dli_errno.h"
#include "dli_log.h"
#include "dli_layer_callback.h"
#include "sdf_evc.h"
#include "sdf_event.h"
#include "sdf_worker.h"
#include "sdf_thread.h"
#include "sdf_sem.h"
#include "sdf_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

struct DLI_ThreadStru {
    int evcHandle;
    int eventHandle;
    SDF_Worker_S *worker;
};

static struct DLI_ThreadStru g_dliThreadStru = {
    .evcHandle = 0,
    .eventHandle = 0,
    .worker = NULL
};

static void DliWorkerRunOnce(void *args)
{
    (void)args;
    SDF_WorkerRunOnce(g_dliThreadStru.worker);
}

static uint32_t DliMainThreadInit()
{
    uint32_t ret = SDF_EvcInstanceCreate(&g_dliThreadStru.evcHandle, "DLI");
    DLI_CHECK_RETURN_RET(ret == 0, DLI_STACK_EVC_CREATE_ERRNO, "SDF_EvcInstanceCreate errno %u", ret);
    g_dliThreadStru.worker = SDF_CreateWorker();
    if (g_dliThreadStru.worker == NULL) {
        SDF_EvcInstanceClose(g_dliThreadStru.evcHandle);
        DLI_LOGE("SDF_CreateWorker err");
        return DLI_STACK_WORKER_CREATE_ERRNO;
    }
    SDF_EventParam param = {g_dliThreadStru.evcHandle, DliWorkerRunOnce, NULL};
    ret = SDF_EventAdd(&g_dliThreadStru.eventHandle, &param);
    if (ret != 0) {
        DLI_LOGE("SDF_EventAdd errno %u", ret);
        SDF_DestroyWorker(g_dliThreadStru.worker);
        g_dliThreadStru.worker = NULL;
        SDF_EvcInstanceClose(g_dliThreadStru.evcHandle);
        return DLI_STACK_EVENT_ADD_ERRNO;
    }
    return DLI_SUCCESS;
}

uint32_t DLI_ThreadAddWork(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    DLI_CHECK_RETURN_RET(
        g_dliThreadStru.worker, DLI_STACK_NOINIT_ERRNO, "worker is null");
    return SDF_AddWork(g_dliThreadStru.worker, cb, arg, freeCb);
}

void DLI_ThreadPostEvent(void)
{
    SDF_EventPost(g_dliThreadStru.eventHandle);
}

uint32_t DLI_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    uint32_t ret = DLI_ThreadAddWork(cb, arg, freeCb);
    if (ret == 0) {
        DLI_ThreadPostEvent();
    }
    return ret;
}

static void DLI_RunOnceBlockTask(void *arg)
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

static void DLI_FreeBlockTask(void *arg)
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

static void DLI_FreeSem(void *arg)
{
    SDF_Sem sem = (SDF_Sem)arg;
    SDF_SemDeinit(sem);
    SDF_MemFree(sem);
}

uint32_t DLI_BlockPostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    SDF_BlockTask_S *task = (SDF_BlockTask_S *)SDF_MemZalloc(sizeof(SDF_BlockTask_S));
    if (task == NULL) {
        if (freeCb != NULL) {
            freeCb(arg);
        }
        return DLI_STACK_POST_BLOCK_ERROR;
    }
    task->func = cb;
    task->freeCb = freeCb;
    task->arg = arg;
    SDF_Sem sem = (SDF_Sem)SDF_MemAlloc(SDF_SEM_SIZE);
    if (sem == NULL) {
        DLI_FreeBlockTask(task);
        return DLI_STACK_POST_BLOCK_ERROR;
    }
    task->sem = sem;
    if (SDF_SemInit(task->sem, 0, 0) != SDF_OK) {
        SDF_MemFree(sem);
        DLI_FreeBlockTask(task);
        return DLI_STACK_POST_BLOCK_ERROR;
    }
    if (DLI_PostTask(DLI_RunOnceBlockTask, task, DLI_FreeBlockTask) != 0) {
        DLI_FreeSem(sem);
        DLI_FreeBlockTask(task);
        return DLI_STACK_POST_BLOCK_ERROR;
    }
    if (SDF_SemTimeWait(sem, timeout) != SDF_OK) {
        if (DLI_PostTask(NULL, sem, DLI_FreeSem) != 0) {
            // Post Task 内部不做资源释放，失败情况下，需要外部主动释放
            DLI_LOGE("post task failed");
            DLI_FreeSem(sem);
        }
        return DLI_STACK_TASK_TIMEOUT;
    }
    DLI_FreeSem(sem);
    return DLI_SUCCESS;
}

static void DLI_FreeArg(void *arg, SDF_FreeWorkArg freeCb)
{
    if (freeCb != NULL && arg != NULL) {
        freeCb(arg);
    }
}

uint32_t DLI_PostOtherThread(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    DLI_Callback *cbk = DLI_GetCallback();
    if (cbk == NULL || cbk->postOtherThread == NULL) {
        DLI_LOGE("postOtherThread is null");
        DLI_FreeArg(arg, freeCb);
        return DLI_STACK_POST_BLOCK_ERROR;
    }

    return cbk->postOtherThread(cb, arg, freeCb);
}

uint32_t DLI_PostOtherBlockedThread(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    DLI_Callback *cbk = DLI_GetCallback();
    if (cbk == NULL || cbk->postOtherBlockedThread == NULL) {
        DLI_LOGE("postOtherBlockedThread is null");
        DLI_FreeArg(arg, freeCb);
        return DLI_STACK_POST_BLOCK_ERROR;
    }

    return cbk->postOtherBlockedThread(cb, arg, freeCb, timeout);
}

int DLI_ThreadEvcHandleGet(void)
{
    return g_dliThreadStru.evcHandle;
}

// shoud after NpdkInit
uint32_t DLI_ThreadInit(void)
{
    DLI_CHECK_RETURN_RET(
        (!g_dliThreadStru.worker), DLI_STACK_INITED_ERRNO, "DLI_Init has been inited");
    return DliMainThreadInit();
}

void DLI_ThreadDeinit(void)
{
    DLI_CHECK_RETURN(g_dliThreadStru.worker, "DLI_Deinit not init");
    SDF_EvcInstanceClose(g_dliThreadStru.evcHandle);
    SDF_DestroyWorker(g_dliThreadStru.worker);
    (void)memset_s(&g_dliThreadStru, sizeof(g_dliThreadStru), 0, sizeof(g_dliThreadStru));
}

#ifdef __cplusplus
}
#endif