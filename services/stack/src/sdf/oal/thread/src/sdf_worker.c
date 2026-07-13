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

#include "sdf_worker.h"
#include "securec.h"

SDF_Worker_S *SDF_CreateWorker(void)
{
    SDF_Worker_S *worker;

    worker = (SDF_Worker_S*)SDF_MemZalloc(sizeof(SDF_Worker_S));
    if (worker == NULL) {
        return NULL;
    }
    SDF_DListHeadInit(&worker->head);
    worker->mutex = (SDF_MutexLock)SDF_MemZalloc(SDF_MUTEX_LOCK_SIZE);
    if (worker->mutex == NULL) {
        SDF_MemFree(worker);
        return NULL;
    }
    if (SDF_RecursiveMutexInit(worker->mutex) != SDF_OK) {
        SDF_MemFree(worker->mutex);
        SDF_MemFree(worker);
        return NULL;
    }

    return worker;
}

void SDF_DestroyWorker(SDF_Worker_S *worker)
{
    if (worker == NULL) {
        return;
    }

    SDF_Work_S *work = NULL;
    SDF_MutexLock(worker->mutex);
    SDF_DListElmForeach(work, &worker->head, entry) {
        if (work->freeCb != NULL) {
            work->freeCb(work->arg);
        }
    }
    SDF_Work_S *tmp = NULL;
    SDF_DListElmAllFree(work, tmp, &worker->head, entry, SDF_MemFree);
    SDF_MutexUnlock(worker->mutex);
    SDF_MutexDeinit(worker->mutex);
    SDF_MemFree(worker->mutex);
    SDF_MemFree(worker);
    worker = NULL;
    return;
}

uint32_t SDF_AddWork(SDF_Worker_S *worker, SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (worker == NULL) {
        return SDF_WORKER_ERROR_ADD_WORK;
    }
    SDF_Work_S *work = (SDF_Work_S*)SDF_MemZalloc(sizeof(SDF_Work_S));
    if (work == NULL) {
        return SDF_WORKER_ERROR_ADD_WORK;
    }
    SDF_DListEntryInit(&work->entry);
    work->cb = cb;
    work->arg = arg;
    work->freeCb = freeCb;
    SDF_MutexLock(worker->mutex);
    SDF_DListElmTailInsert(&worker->head, work, entry);
    SDF_MutexUnlock(worker->mutex);

    return SDF_OK;
}

void SDF_WorkerRunOnce(SDF_Worker_S *worker)
{
    if (worker == NULL) {
        return;
    }
    SDF_Work_S *work = NULL;
    while (1) {
        SDF_MutexLock(worker->mutex);
        if (SDF_DListIsEmpty(&worker->head)) {
            SDF_MutexUnlock(worker->mutex);
            return;
        }
        SDF_DListElmHeadDel(&worker->head, work, entry);
        SDF_MutexUnlock(worker->mutex);
        if (work->cb != NULL) {
            work->cb(work->arg);
        }
        if (work->freeCb != NULL) {
            work->freeCb(work->arg);
        }
        SDF_MemFree(work);
        work = NULL;
    }
}