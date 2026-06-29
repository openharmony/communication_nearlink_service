/**
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
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include "securec.h"
#include "sdf_mem.h"
#include "sdf_thread.h"
#include "sdf_vector.h"
#include "sdf_mutex.h"
#include "sdf_log.h"
#include "sdf_evc.h"

typedef struct {
    int handle;
    SDF_Vector_S *eventVector;
    uint32_t threadId;
    int closeEventHandle;
} SDF_EvcDesc;

static SDF_Vector_S *g_evcDescVector = NULL;
static SDF_MutexLock g_evcLock = NULL;

void DectoryEvcDesc(void *data)
{
    if (data == NULL) {
        return;
    }
    SDF_EvcDesc *evcDesc = (SDF_EvcDesc *)data;
    if (evcDesc->handle >= 0) {
        close(evcDesc->handle);
    }
    if (evcDesc->closeEventHandle >= 0) {
        close(evcDesc->closeEventHandle);
    }
    SDF_DestroyVector(evcDesc->eventVector);
    SDF_MemFree(evcDesc);
}

void DestoryEvcEvent(void *data)
{
    if (data == NULL) {
        return;
    }
    SDF_EvcEvent *evcEvent = (SDF_EvcEvent *)data;
    if (evcEvent->freeFunc != NULL) {
        evcEvent->freeFunc(evcEvent->args);
    }
    SDF_MemFree(evcEvent);
}

SDF_EvcDesc *CreateEvcDesc(void)
{
    SDF_EvcDesc *evcDesc = (SDF_EvcDesc *)SDF_MemZalloc(sizeof(SDF_EvcDesc));
    if (evcDesc == NULL) {
        return NULL;
    }
    SDF_Traits traits = {.dtor = DestoryEvcEvent};
    evcDesc->eventVector = SDF_CreateVectorByCapacity(SDF_EVC_MAX_EVENT_NUM, traits);
    if (evcDesc->eventVector == NULL) {
        SDF_MemFree(evcDesc);
        return NULL;
    }
    evcDesc->closeEventHandle = -1;
    evcDesc->handle = -1;
    return evcDesc;
}

uint32_t SDF_EvcInit(void)
{
    if (g_evcDescVector != NULL) {
        return SDF_EVC_ERROR_ALREADY_INIT;
    }
    uint32_t ret = SDF_OK;
    SDF_Traits traits = {.dtor = DectoryEvcDesc};
    g_evcDescVector = SDF_CreateVectorByCapacity(SDF_EVC_MAX_INSTANCE_NUM, traits);
    if (g_evcDescVector == NULL) {
        ret = SDF_EVC_ERROR_MEM_FAILED;
        goto FAIL;
    }
    g_evcLock = (SDF_MutexLock)SDF_MemZalloc(SDF_MUTEX_LOCK_SIZE);
    if (g_evcLock == NULL) {
        ret = SDF_EVC_ERROR_MEM_FAILED;
        goto FAIL;
    }
    if (SDF_RecursiveMutexInit(g_evcLock) != SDF_OK) {
        ret = SDF_EVC_ERROR_MUTEX_FAILED;
        goto FAIL;
    }
    return SDF_OK;
FAIL:
    if (g_evcDescVector != NULL) {
        SDF_DestroyVector(g_evcDescVector);
        g_evcDescVector = NULL;
    }
    if (g_evcLock != NULL) {
        SDF_MemFree(g_evcLock);
        g_evcLock = NULL;
    }
    return ret;
}

static bool CompEvcDesc(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    SDF_EvcDesc *evcDesc = (SDF_EvcDesc *)ptr;
    int *handle = (int *)args;
    return evcDesc->handle == *handle;
}

static bool CompEvcEvent(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    SDF_EvcEvent *evcEvent = (SDF_EvcEvent *)ptr;
    int *handle = (int *)args;
    return evcEvent->eventHandle == *handle;
}

static SDF_EvcDesc *FindEvcDesc(int handle)
{
    if (g_evcDescVector == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < g_evcDescVector->size; i++) {
        SDF_EvcDesc *evcDesc = SDF_VectorElementAt(g_evcDescVector, i);
        if (evcDesc->handle == handle) {
            return evcDesc;
        }
    }
    return NULL;
}

static SDF_EvcEvent *FindEvcEvent(int eventHandle)
{
    if (g_evcDescVector == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < g_evcDescVector->size; i++) {
        SDF_EvcDesc *evcDesc = SDF_VectorElementAt(g_evcDescVector, i);
        for (size_t j = 0; j < evcDesc->eventVector->size; j++) {
            SDF_EvcEvent *evcEvent = SDF_VectorElementAt(evcDesc->eventVector, j);
            if (evcEvent->eventHandle == eventHandle) {
                return evcEvent;
            }
        }
    }
    return NULL;
}

static SDF_EvcDesc *FindEvcByEventId(int eventHandle)
{
    if (g_evcDescVector == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < g_evcDescVector->size; i++) {
        SDF_EvcDesc *evcDesc = SDF_VectorElementAt(g_evcDescVector, i);
        for (size_t j = 0; j < evcDesc->eventVector->size; j++) {
            SDF_EvcEvent *evcEvent = SDF_VectorElementAt(evcDesc->eventVector, j);
            if (evcEvent->eventHandle == eventHandle) {
                return evcDesc;
            }
        }
    }
    return NULL;
}

static void EvcProcessEvent(struct epoll_event *event, int eventHandle)
{
    uint64_t readInfo = 0;
    int readLen = 0;
    SDF_MutexLock(g_evcLock);
    SDF_EvcEvent *sdfEvent = FindEvcEvent(eventHandle);
    SDF_MutexUnlock(g_evcLock);
    if (sdfEvent == NULL) {
        return;
    }
    switch (sdfEvent->type) {
        case SDF_EVC_TIMER:
        case SDF_EVC_EVENT:
            if (event->events & EPOLLIN) {
                readLen = read(sdfEvent->eventHandle, &readInfo, sizeof(uint64_t));
                if (readLen != (int)(sizeof(uint64_t))) {
                    break;
                }
                if (sdfEvent->callback != NULL) {
                    sdfEvent->callback(sdfEvent->eventHandle, sdfEvent->args);
                }
            }
            break;
        case SDF_EVC_USER_EVENT:
            if ((event->events & EPOLLIN) && (sdfEvent->callback != NULL)) {
                sdfEvent->callback(sdfEvent->eventHandle, sdfEvent->args);
            }
            break;
        default:
            break;
    }
}

static void EvcLoop(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3)
{
    (void)arg1;
    (void)arg2;
    (void)arg3;
    SDF_EvcDesc *evcDesc = (SDF_EvcDesc *)arg0;
    if (evcDesc == NULL) {
        return;
    }
    struct epoll_event ev[SDF_EVC_MAX_EVENT_NUM];
    bool isRunning = true;
    while (isRunning) {
        int num = epoll_wait(evcDesc->handle, ev, SDF_EVC_MAX_EVENT_NUM, -1);
        if (num <= 0) {
            continue;
        }
        for (int i = 0; i < num; i++) {
            struct epoll_event *event = &ev[i];
            int eventHandle = event->data.fd;
            if (eventHandle == evcDesc->closeEventHandle) {
                isRunning = false;
                continue;
            }
            EvcProcessEvent(event, eventHandle);
        }
    }
}

static bool SDF_EvcInstanceAddCloseEvent(SDF_EvcDesc *evcDesc, uint32_t *ret)
{
    evcDesc->closeEventHandle = eventfd(0, EFD_NONBLOCK);
    if (evcDesc->closeEventHandle < 0) {
        *ret = SDF_EVC_ERROR_FD_CREATE_FAILED;
        return false;
    }
    struct epoll_event epEvent = {
        .events = EPOLLIN,
        .data.fd = evcDesc->closeEventHandle,
    };
    if (epoll_ctl(evcDesc->handle, EPOLL_CTL_ADD, evcDesc->closeEventHandle, &epEvent) != 0) {
        *ret = SDF_EVC_ERROR_EPOLL_CTL_FAILED;
        return false;
    }
    *ret = SDF_OK;
    return true;
}

uint32_t SDF_EvcInstanceCreate(int *handle, const char *name)
{
    SDF_CHECK_LOG_RETURN((handle != NULL && name != NULL), SDF_EVC_ERROR_INVALID_PARAM,
        "SDF Evc Instance Create invalid param.");
    SDF_MutexLock(g_evcLock);
    if (g_evcDescVector == NULL) {
        SDF_MutexUnlock(g_evcLock);
        return SDF_EVC_ERROR_NOT_INIT;
    }
    if (g_evcDescVector->size >= SDF_EVC_MAX_INSTANCE_NUM) {
        SDF_MutexUnlock(g_evcLock);
        return SDF_EVC_ERROR_MAX_INSTANCE;
    }
    uint32_t ret = SDF_OK;
    SDF_EvcDesc *evcDesc = CreateEvcDesc();
    if (evcDesc == NULL) {
        ret = SDF_EVC_ERROR_MEM_FAILED;
        goto Fail3;
    }
    evcDesc->handle = epoll_create(SDF_EVC_MAX_EVENT_NUM);
    if (evcDesc->handle < 0) {
        ret = SDF_EVC_ERROR_EPOLL_CREATE_FAILED;
        goto Fail2;
    }
    if (!SDF_VectorEmplaceBack(g_evcDescVector, evcDesc)) {
        ret = SDF_EVC_ERROR_VECTOR_FAIL;
        goto Fail2;
    }
    if (!SDF_EvcInstanceAddCloseEvent(evcDesc, &ret)) {
        goto Fail1;
    }
    SDF_ThreadCreateParam_S threadParam = {name, SDF_THREAD_SCHED_OTHER, 0, 0,
        EvcLoop, {(uintptr_t)evcDesc}};
    if (SDF_ThreadCreate(&evcDesc->threadId, &threadParam) != SDF_OK) {
        ret = SDF_EVC_ERROR_THREAD_FAILED;
        goto Fail1;
    }
    *handle = evcDesc->handle;
    SDF_MutexUnlock(g_evcLock);
    return SDF_OK;
Fail1:
    SDF_VectorRemoveLast(g_evcDescVector);
Fail2:
    DectoryEvcDesc(evcDesc);
Fail3:
    SDF_MutexUnlock(g_evcLock);
    return ret;
}

static void SDF_EvcTriggerCloseEvent(SDF_EvcDesc *evcDesc)
{
    uint64_t writeVal = 1;
    SDF_CHECK_LOG_RETURN_VOID(write(evcDesc->closeEventHandle, &writeVal, sizeof(writeVal)) == sizeof(writeVal),
                            "Failed to writeVal");
}

void SDF_EvcInstanceClose(int handle)
{
    if (handle < 0) {
        return;
    }
    size_t index = 0;
    SDF_MutexLock(g_evcLock);
    if (!SDF_VectorFindFirst(g_evcDescVector, CompEvcDesc, &handle, &index)) {
        SDF_MutexUnlock(g_evcLock);
        return;
    }
    SDF_EvcDesc *evcDesc = SDF_VectorElementAt(g_evcDescVector, index);
    uint32_t tempThreadId = evcDesc->threadId;
    SDF_EvcTriggerCloseEvent(evcDesc);
    SDF_MutexUnlock(g_evcLock);
    SDF_ThreadJoin(tempThreadId);
    SDF_MutexLock(g_evcLock);
    SDF_VectorRemove(g_evcDescVector, index);
    SDF_MutexUnlock(g_evcLock);
}

uint32_t SDF_EvcListenEvent(int handle, SDF_EvcEvent *event)
{
    SDF_CHECK_LOG_RETURN((handle >= 0 && event != NULL), SDF_EVC_ERROR_INVALID_PARAM,
        "SDF Evc Listen invalid param.");
    SDF_MutexLock(g_evcLock);
    uint32_t ret = SDF_OK;
    SDF_EvcDesc *evcDesc = FindEvcDesc(handle);
    if (evcDesc == NULL) {
        ret = SDF_EVC_ERROR_WRONG_HANDLE;
        goto FAIL3;
    }
    if (evcDesc->eventVector->size >= SDF_EVC_MAX_EVENT_NUM) {
        ret = SDF_EVC_ERROR_WRONG_HANDLE;
        goto FAIL3;
    }
    if (FindEvcEvent(event->eventHandle) != NULL) {
        ret = SDF_EVC_ERROR_ALREADY_LISTENED;
        goto FAIL3;
    }
    SDF_EvcEvent *evcEvent = (SDF_EvcEvent *)SDF_MemZalloc(sizeof(SDF_EvcEvent));
    if (evcEvent == NULL) {
        ret = SDF_EVC_ERROR_MEM_FAILED;
        goto FAIL3;
    }
    (void)memcpy_s(evcEvent, sizeof(SDF_EvcEvent), event, sizeof(SDF_EvcEvent));
    if (!SDF_VectorEmplaceBack(evcDesc->eventVector, evcEvent)) {
        ret = SDF_EVC_ERROR_VECTOR_FAIL;
        goto FAIL2;
    }
    struct epoll_event epEvent = {
        .events = EPOLLIN,
        .data.fd = evcEvent->eventHandle,
    };
    if (epoll_ctl(handle, EPOLL_CTL_ADD, evcEvent->eventHandle, &epEvent) != 0) {
        ret = SDF_EVC_ERROR_EPOLL_CTL_FAILED;
        goto FAIL1;
    }
    SDF_MutexUnlock(g_evcLock);
    return SDF_OK;
FAIL1:
    SDF_VectorRemoveLast(evcDesc->eventVector);
FAIL2:
    SDF_MemFree(evcEvent);
FAIL3:
    SDF_MutexUnlock(g_evcLock);
    return ret;
}

uint32_t SDF_ModifyEventArgs(int eventHandle, SDF_EvcEventModifyFunc func)
{
    SDF_CHECK_LOG_RETURN((eventHandle >= 0 && func != NULL), SDF_EVC_ERROR_INVALID_PARAM,
        "SDF Evc Modify invalid param.");
    SDF_MutexLock(g_evcLock);
    SDF_EvcDesc *evcDesc = FindEvcByEventId(eventHandle);
    if (evcDesc == NULL) {
        SDF_MutexUnlock(g_evcLock);
        return SDF_EVC_ERROR_WRONG_HANDLE;
    }
    SDF_EvcEvent *evcEvent = FindEvcEvent(eventHandle);
    if (evcEvent == NULL) {
        SDF_MutexUnlock(g_evcLock);
        return SDF_EVC_ERROR_WRONG_HANDLE;
    }
    uint32_t ret = func((void *)evcEvent->args);
    SDF_MutexUnlock(g_evcLock);
    return ret;
}

void SDF_EvcCancelEvent(int eventHandle)
{
    SDF_CHECK_LOG_RETURN_VOID(eventHandle >= 0, "SDF Evc Modify invalid param.");
    SDF_MutexLock(g_evcLock);
    SDF_EvcDesc *evcDesc = FindEvcByEventId(eventHandle);
    if (evcDesc == NULL) {
        SDF_MutexUnlock(g_evcLock);
        return;
    }
    size_t index = 0;
    if (!SDF_VectorFindFirst(evcDesc->eventVector, CompEvcEvent, &eventHandle, &index)) {
        SDF_MutexUnlock(g_evcLock);
        return;
    }
    (void)epoll_ctl(evcDesc->handle, EPOLL_CTL_DEL, eventHandle, NULL);
    SDF_VectorRemove(evcDesc->eventVector, index);
    SDF_MutexUnlock(g_evcLock);
    return;
}

void SDF_EvcDeinit(void)
{
    SDF_MutexLock(g_evcLock);
    if (g_evcDescVector == NULL) {
        SDF_MutexUnlock(g_evcLock);
        return;
    }
    uint32_t tempHandle[SDF_EVC_MAX_EVENT_NUM] = {0};
    size_t tempCount = g_evcDescVector->size;
    for (size_t i = 0; i < g_evcDescVector->size; i++) {
        SDF_EvcDesc *tmpDesc = SDF_VectorElementAt(g_evcDescVector, i);
        tempHandle[i] = tmpDesc->threadId;
        SDF_EvcTriggerCloseEvent(tmpDesc);
    }
    SDF_MutexUnlock(g_evcLock);
    for (size_t i = 0; i < tempCount; i++) {
        SDF_ThreadJoin(tempHandle[i]);
    }
    SDF_MutexLock(g_evcLock);
    SDF_DestroyVector(g_evcDescVector);
    g_evcDescVector = NULL;
    SDF_MutexUnlock(g_evcLock);
    SDF_MutexDeinit(g_evcLock);
    SDF_MemFree(g_evcLock);
    g_evcLock = NULL;
    return;
}