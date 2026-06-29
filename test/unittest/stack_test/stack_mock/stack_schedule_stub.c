/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <stdint.h>
#include "sdf_timer.h"
#include "sdf_worker.h"
#include "sdf_vector.h"

uint32_t TEST_SchedulePostTaskStub(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (cb != NULL) {
        cb(arg);
    }
    if (freeCb != NULL) {
        freeCb(arg);
    }
    return 0;
}

uint32_t TEST_SchedulePostTaskStubFail(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    (void)cb;
    (void)arg;
    if (freeCb != NULL) {
        freeCb(arg);
    }
    return 1;
}

uint32_t TEST_SchedulePostTaskBlockedStub(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    if (cb != NULL) {
        cb(arg);
    }
    if (freeCb != NULL) {
        freeCb(arg);
    }
    return 0;
}

uint32_t TEST_SchedulePostTaskBlockedStubFail(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    (void)cb;
    (void)arg;
    (void)freeCb;
    (void)timeout;
    return 1;
}

typedef struct StubWorker {
    SDF_WorkCb cb;
    void *arg;
    SDF_FreeWorkArg freeCb;
} StubWorker_S;

static SDF_Vector_S *g_stubWorkerVector;

uint32_t TEST_SchedulePostTaskQueueStub(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    StubWorker_S *worker = (StubWorker_S *)SDF_MemZalloc(sizeof(StubWorker_S));
    worker->cb = cb;
    worker->arg = arg;
    worker->freeCb = freeCb;
    SDF_VectorEmplaceBack(g_stubWorkerVector, worker);
    return 0;
}

void TEST_RunQueueStubSchedule()
{
    while (g_stubWorkerVector->size > 0) {
        StubWorker_S *worker = (StubWorker_S *)SDF_VectorPopElement(g_stubWorkerVector, 0);
        if (worker->cb != NULL) {
            worker->cb(worker->arg);
        }
        if (worker->freeCb != NULL) {
            worker->freeCb(worker->arg);
        }
        SDF_MemFree(worker);
    }
}

void TEST_RunQueueStubScheduleOnce()
{
    if (g_stubWorkerVector->size > 0) {
        StubWorker_S *worker = (StubWorker_S *)SDF_VectorPopElement(g_stubWorkerVector, 0);
        if (worker->cb != NULL) {
            worker->cb(worker->arg);
        }
        if (worker->freeCb != NULL) {
            worker->freeCb(worker->arg);
        }
        SDF_MemFree(worker);
    }
}

void TEST_ScheduleInit()
{
    g_stubWorkerVector = SDF_CreateVector(MAKE_TRAITS(SDF_MemFree, NULL));
}

void TEST_StackScheduleDeInit()
{
    if (g_stubWorkerVector != NULL) {
        SDF_DestroyVector(g_stubWorkerVector);
        g_stubWorkerVector = NULL;
    }
}

uint32_t TEST_ScheduleTimerAddStub(int *handle, SDF_TimerParam *param)
{
    return 0;
}

void TEST_ScheduleTimerDelStub(int handle)
{
}