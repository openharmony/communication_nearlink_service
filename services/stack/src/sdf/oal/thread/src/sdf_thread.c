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
 * @brief        SDF BSL Thread source file.
 */

#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <time.h>

#include "sdf_mem.h"
#include "sdf_util.h"
#include "sdf_thread.h"
#include "sdf_log.h"

#define SECS_STEP 1000
#define ARG0 0
#define ARG1 1
#define ARG2 2
#define ARG3 3
#define SLE_THREAD_PRIORITY 1

typedef struct {
    char *name;
    SDF_ThreadFunc hook;
    uintptr_t args[SDF_THREAD_ARGS_NUM];
    uint32_t event;
    uint32_t threadId;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ThreadDesc;

static uint32_t g_maxNum = 0;
static ThreadDesc *g_threadDesc = NULL;
static ThreadDesc *g_freelist = NULL;
static pthread_rwlock_t g_rwlock;
static pthread_key_t g_key;

uint32_t SDF_ThreadInit(uint16_t maxNum)
{
    uint32_t i;
    uint32_t num;
    uint32_t ret;
    if (g_threadDesc != NULL) {
        return SDF_THREAD_ERROR_ALREAD_INIT;
    }
    if (maxNum == 0) {
        return SDF_THREAD_ERROR_INVALID_PARAM;
    }
    num = maxNum + 1;
    g_threadDesc = SDF_MemZalloc(num * sizeof(ThreadDesc));
    if (g_threadDesc == NULL) {
        return SDF_THREAD_ERROR_MEM_ALLOC;
    }
    if (pthread_key_create(&g_key, NULL) != 0) {
        ret = SDF_THREAD_ERROR_THREAD_KEY_CREATE;
        goto ERR0;
    }
    if (pthread_rwlock_init(&g_rwlock, NULL) != 0) {
        ret = SDF_THREAD_ERROR_LOCK_INIT;
        goto ERR1;
    }
    for (i = 1; i < num; i++) {
        g_threadDesc[i].threadId = i;
    }
    for (i = 1; i < num - 1; i++) {
        *(ThreadDesc **)&g_threadDesc[i] = &g_threadDesc[i + 1];
    }
    *(ThreadDesc **)&g_threadDesc[i] = NULL;
    g_freelist = &g_threadDesc[1];
    g_maxNum = num;
    return SDF_OK;

ERR1:
    (void)pthread_key_delete(g_key);
ERR0:
    SDF_MemFree(g_threadDesc);
    g_threadDesc = NULL;
    return ret;
}

void SDF_ThreadDeinit(void)
{
    if (g_threadDesc != NULL) {
        g_freelist = NULL;
        g_maxNum = 0;
        SDF_MemFree(g_threadDesc);
        g_threadDesc = NULL;
        (void)pthread_rwlock_destroy(&g_rwlock);
        (void)pthread_key_delete(g_key);
    }
    return;
}

static void ThreadDescFree2List(ThreadDesc *desc)
{
    (void)pthread_rwlock_wrlock(&g_rwlock);
    *(ThreadDesc **)desc = g_freelist;
    g_freelist = desc;
    (void)pthread_rwlock_unlock(&g_rwlock);
    return;
}

static void ThreadDescFree(ThreadDesc *desc)
{
    uint32_t i;
    (void)pthread_cond_destroy(&desc->cond);
    (void)pthread_mutex_destroy(&desc->mutex);
    for (i = 0; i < SDF_THREAD_ARGS_NUM; i++) {
        desc->args[i] = 0;
    }
    desc->hook = NULL;
    desc->event = 0;
    SDF_MemFree(desc->name);
    desc->name = NULL;
    ThreadDescFree2List(desc);
    return;
}

static void SetRealTimePriority(pid_t tid)
{
    struct sched_param params = {.sched_priority = SLE_THREAD_PRIORITY};

    if (sched_setscheduler(tid, SDF_THREAD_SCHED_FIFO, &params) != 0) {
        SDF_LOG_ERROR("set rt priority fail: %{public}s", strerror(errno));
    }
}

static void *ThreadFunc(void *arg)
{
    SetRealTimePriority(gettid());
    ThreadDesc *desc = (ThreadDesc *)arg;
    if (pthread_setspecific(g_key, desc) == 0) {
        desc->hook(desc->args[ARG0], desc->args[ARG1], desc->args[ARG2], desc->args[ARG3]);
    }
    return NULL;
}

static uint32_t ThreadAttrInit(pthread_attr_t *attr, const SDF_ThreadCreateParam_S *param)
{
    struct sched_param schedParam;
    if (pthread_attr_setschedpolicy(attr, param->policy) != 0) {
        return SDF_THREAD_ERROR_SET_ATTR;
    }
    schedParam.sched_priority = param->priority;
    if (pthread_attr_setschedparam(attr, &schedParam) != 0) {
        return SDF_THREAD_ERROR_SET_ATTR;
    }
    if ((param->stackSize > 0) && (pthread_attr_setstacksize(attr, param->stackSize) != 0)) {
        return SDF_THREAD_ERROR_SET_ATTR;
    }
    return SDF_OK;
}

static ThreadDesc *GetOneThreadDesc(void)
{
    ThreadDesc *ret;
    if (pthread_rwlock_wrlock(&g_rwlock) != 0) {
        return NULL;
    }
    ret = g_freelist;
    if (ret != NULL) {
        g_freelist = *(ThreadDesc **)ret;
    }
    (void)pthread_rwlock_unlock(&g_rwlock);
    return ret;
}

static uint32_t ThreadDescInit(ThreadDesc *desc, const SDF_ThreadCreateParam_S *param)
{
    uint32_t j;
    pthread_condattr_t condattr;
    uint32_t ret;
    char *dupname = SDF_StrDup(param->name);
    if (dupname == NULL) {
        return SDF_THREAD_ERROR_MEM_ALLOC;
    }
    if (pthread_mutex_init(&desc->mutex, NULL) != 0) {
        ret = SDF_THREAD_ERROR_MUTEX_INIT;
        goto ERR0;
    }
    ret = SDF_THREAD_ERROR_COND_INIT;
    if (pthread_condattr_init(&condattr) != 0) {
        goto ERR1;
    }
    if (pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC) != 0) {
        goto ERR2;
    }
    if (pthread_cond_init(&desc->cond, &condattr) != 0) {
        goto ERR2;
    }
    (void)pthread_condattr_destroy(&condattr);
    desc->name = dupname;
    for (j = 0; j < SDF_THREAD_ARGS_NUM; j++) {
        desc->args[j] = param->args[j];
    }
    desc->event = 0;
    desc->hook = param->hook;
    return SDF_OK;
ERR2:
    (void)pthread_condattr_destroy(&condattr);
ERR1:
    (void)pthread_mutex_destroy(&desc->mutex);
ERR0:
    SDF_MemFree(dupname);
    dupname = NULL;
    return ret;
}

uint32_t SDF_ThreadCreate(uint32_t *threadId, const SDF_ThreadCreateParam_S *param)
{
    pthread_attr_t attr;
    ThreadDesc *desc;
    int ret;
    uint32_t uRet;

    bool invalid = (g_freelist == NULL) || (threadId == NULL) || (param == NULL) || (param->name == NULL) ||
        (param->hook == NULL) || (param->policy < SDF_THREAD_SCHED_OTHER) || (param->policy > SDF_THREAD_SCHED_RR) ||
        (param->priority > sched_get_priority_max(param->policy)) ||
        (param->priority < sched_get_priority_min(param->policy));
    if (invalid) {
        return SDF_THREAD_ERROR_INVALID_PARAM;
    }

    *threadId = SDF_THREAD_INVALID_ID;

    desc = GetOneThreadDesc();
    if (desc == NULL) {
        return SDF_THREAD_ERROR_LOCK_ACQUIRE;
    }

    uRet = ThreadDescInit(desc, param);
    if (uRet != SDF_OK) {
        ThreadDescFree2List(desc);
        return uRet;
    }

    uRet = SDF_THREAD_ERROR_THREAD_INIT;
    if (pthread_attr_init(&attr) != 0) {
        goto ERR;
    }
    if (ThreadAttrInit(&attr, param) != SDF_OK) {
        (void)pthread_attr_destroy(&attr);
        goto ERR;
    }

    ret = pthread_create(&desc->thread, &attr, ThreadFunc, desc);
    (void)pthread_attr_destroy(&attr);
    if (ret != 0) {
        goto ERR;
    }

    *threadId = desc->threadId;
    return SDF_OK;
ERR:
    ThreadDescFree(desc);
    return uRet;
}

uint32_t SDF_ThreadSelfId(void)
{
    ThreadDesc *desc = pthread_getspecific(g_key);
    if (desc == NULL) {
        return SDF_THREAD_INVALID_ID;
    }
    return desc->threadId;
}

void SDF_ThreadYield(void)
{
    (void)sched_yield();
    return;
}

void SDF_ThreadSleep(uint32_t ms)
{
    (void)usleep(ms * SECS_STEP);
    return;
}

uint32_t SDF_ThreadJoin(uint32_t threadId)
{
    bool invalid = (threadId == SDF_THREAD_INVALID_ID) || (threadId >= g_maxNum);
    if (invalid) {
        return SDF_THREAD_ERROR_INVALID_PARAM;
    }
    if (g_threadDesc[threadId].hook == NULL) {
        return SDF_THREAD_ERROR_INVALID_THREADID;
    }
    if (pthread_join(g_threadDesc[threadId].thread, NULL) != 0) {
        return SDF_THREAD_ERROR_THREAD_JOIN;
    }
    ThreadDescFree(&g_threadDesc[threadId]);
    return SDF_OK;
}

uint32_t SDF_ThreadLocalKeyInit(SDF_ThreadLocalKey *key, void (*cleanup)(void *))
{
    if (key == NULL) {
        return SDF_THREAD_ERROR_INVALID_PARAM;
    }
    if (pthread_key_create((pthread_key_t *)key, cleanup) != 0) {
        return SDF_THREAD_ERROR_THREAD_KEY_CREATE;
    }
    return SDF_OK;
}

uint32_t SDF_ThreadSetLocal(SDF_ThreadLocalKey key, void *data)
{
    if (pthread_setspecific((pthread_key_t)(uintptr_t)key, data) != 0) {
        return SDF_THREAD_ERROR_SET_SPECIFIC;
    }
    return SDF_OK;
}

void *SDF_ThreadGetLocal(SDF_ThreadLocalKey key)
{
    return pthread_getspecific((pthread_key_t)(uintptr_t)key);
}

uint32_t SDF_ThreadCleanupLocal(SDF_ThreadLocalKey key)
{
    if (pthread_key_delete((pthread_key_t)(uintptr_t)key) != 0) {
        return SDF_THREAD_ERROR_KEY_DELETE;
    }
    return SDF_OK;
}
