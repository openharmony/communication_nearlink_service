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
 * @file lock_mutex.c
 * @brief 互斥锁实现
 * @version 1.0
 * @date 2024-8-7
 */
#include <stddef.h>
#include <pthread.h>
#include "sdf_mutex.h"
#include "sdf_lock_api.h"
#include "sdf_lock_err.h"
#include <string.h>
#include "sdf_log.h"

#ifdef __cplusplus
extern "C" {
#endif

static uint32_t MutexInit(SDF_MutexLock pLock, const SDF_MutexLockAttr pAttr)
{
    pthread_mutex_t *mutex = (pthread_mutex_t *)pLock;
    if (mutex == NULL) {
        return SDF_LOCK_ERROR_INVALID_PARAM;
    }

    if (pthread_mutex_init(mutex, (pthread_mutexattr_t *)pAttr) != 0) {
        return SDF_LOCK_ERROR_INIT_FAIL;
    }
    return SDF_OK;
}

static uint32_t MutexAttrInit(SDF_MutexLockAttr pAttr)
{
    pthread_mutexattr_t *attr = (pthread_mutexattr_t *)pAttr;
    if (pAttr == NULL) {
        return SDF_LOCK_ERROR_INVALID_PARAM;
    }
    if (pthread_mutexattr_init(attr) != 0) {
        return SDF_LOCK_ERROR_INIT_FAIL;
    }
    return SDF_OK;
}

static uint32_t SDF_MutexAttrSet(SDF_MutexLockAttr pAttr, SDF_MutexType_E pType)
{
    pthread_mutexattr_t *attr = (pthread_mutexattr_t *)pAttr;
    if (pAttr == NULL) {
        return SDF_LOCK_ERROR_INVALID_PARAM;
    }
    if (pthread_mutexattr_settype(attr, pType) != 0) {
        return SDF_LOCK_ERROR_INIT_FAIL;
    }
    return SDF_OK;
}

static void MutexDeinit(SDF_MutexLock pLock)
{
    if (pLock == NULL) {
        return;
    }
    pthread_mutex_destroy((pthread_mutex_t *)pLock);
}

static uint32_t MutexLock(SDF_MutexLock pLock)
{
    if (pLock == NULL) {
        return SDF_LOCK_ERROR_INVALID_PARAM;
    }
    int ret = pthread_mutex_lock((pthread_mutex_t *)pLock);
    if (ret != 0) {
        SDF_LOG_ERROR("pthread_mutex_lock failed, error code: %d", ret);
        return SDF_LOCK_ERROR_MUTEX_FAIL;
    }
    return SDF_OK;
}

static uint32_t MutexTryLock(SDF_MutexLock pLock)
{
    if (pLock == NULL) {
        return SDF_LOCK_ERROR_INVALID_PARAM;
    }
    int ret = pthread_mutex_trylock((pthread_mutex_t *)pLock);
    if (ret != 0) {
        SDF_LOG_ERROR("pthread_mutex_trylock failed, error code: %d", ret);
        return SDF_LOCK_ERROR_MUTEX_FAIL;
    }
    return SDF_OK;
}

static void MutexUnlock(SDF_MutexLock pLock)
{
    if (pLock == NULL) {
        return;
    }
    (void)pthread_mutex_unlock((pthread_mutex_t *)pLock);
}

uint32_t SDF_RecursiveMutexInit(SDF_MutexLock pLock)
{
    pthread_mutex_t *mutex = (pthread_mutex_t *)pLock;

    if (mutex == NULL) {
        return SDF_LOCK_ERROR_INVALID_PARAM;
    }

    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0) {
        return SDF_LOCK_ERROR_INIT_FAIL;
    }
    if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0) {
        return SDF_LOCK_ERROR_INIT_FAIL;
    }

    if (pthread_mutex_init(mutex, &attr) != 0) {
        return SDF_LOCK_ERROR_INIT_FAIL;
    }
    return SDF_OK;
}

SDF_MutexLockHooks_S g_mutexLockHooks = {
    .initHook = MutexInit,
    .deinitHook = MutexDeinit,
    .lockHook = MutexLock,
    .tryLockHook = MutexTryLock,
    .unlockHook = MutexUnlock,
    .attrInitHook = MutexAttrInit,
    .attrSetHook = SDF_MutexAttrSet,
    .lockSize = sizeof(pthread_mutex_t), // 结构体大小
};

#ifdef __cplusplus
}
#endif
