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
 * @file lock_sem.c
 * @brief 信号量实现
 * @version 1.0
 * @date 2024-8-7
 */
#include <stddef.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "sdf_sem.h"
#include "sdf_sem_api.h"
#include "sdf_lock_err.h"
#include <string.h>
#include "sdf_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SEC_TO_MILLISEC 1000
#define MILLISEC_TO_NANOSEC 1000000
#define SEC_TO_NANOSEC 1000000000

typedef struct {
    SDF_SemType_E type;
    struct uinon {
        sem_t sem;
    } data;
} Sem_S;

static inline uint32_t SemCountInit(Sem_S *pSem, uint32_t value)
{
    pSem->type = SDF_SEM_COUNT_TYPE;

    int ret = sem_init(&(pSem->data.sem), 0, value);
    if (ret != 0) {
        SDF_LOG_ERROR("SemCountInit sem_init failed, error code: %d", ret);
        return SDF_LOCK_ERROR_INIT_FAIL;
    }
    return SDF_OK;
}

#define BINARY_SEM_MAX_VALUE 1
static inline uint32_t SemBinaryInit(Sem_S *pSem, uint32_t value)
{
    if (value > BINARY_SEM_MAX_VALUE) {
        return SDF_LOCK_ERROR_SEM_INIT_FAIL;
    }
    pSem->type = SDF_SEM_BINARY_TYPE;
    int ret = sem_init(&(pSem->data.sem), 0, value);
    if (ret != 0) {
        SDF_LOG_ERROR("SemBinaryInit sem_init failed, error code: %d", ret);
        return SDF_LOCK_ERROR_INIT_FAIL;
    }
    return SDF_OK;
}

typedef uint32_t (*SemInitHook)(Sem_S *, uint32_t);
static SemInitHook g_semInitHook[] = { SemCountInit, SemBinaryInit};

static uint32_t SemInit(SDF_Sem pSem, int flag, uint32_t value)
{
    int flagTmp = flag;
    if (pSem == NULL || flagTmp >= SDF_SEM_TYPE_BUTT || flagTmp < 0) {
        return SDF_LOCK_ERROR_INVALID_PARAM;
    }

    return g_semInitHook[flagTmp](pSem, value);
}

static void SemDeinit(SDF_Sem pSem)
{
    Sem_S *pSemS = (Sem_S *)pSem;
    if (pSem == NULL) {
        return;
    }
    sem_destroy(&(pSemS->data.sem));
}

static uint32_t SemWait(SDF_Sem pSem)
{
    Sem_S *pSemS = (Sem_S *)pSem;
    if (pSem == NULL) {
        return SDF_LOCK_ERROR_INVALID_PARAM;
    }

    int ret = sem_wait(&(pSemS->data.sem));
    if (ret != 0) {
        SDF_LOG_ERROR("sem_wait failed, error code: %d", ret);
        return SDF_LOCK_ERROR_SEM_WAIT_FAIL;
    }
    return SDF_OK;
}

static uint32_t SemTryWait(SDF_Sem pSem)
{
    int ret;
    Sem_S *pSemS = (Sem_S *)pSem;
    if (pSem == NULL) {
        return SDF_LOCK_ERROR_INVALID_PARAM;
    }

    ret = sem_trywait(&(pSemS->data.sem));
    if (ret != 0) {
        SDF_LOG_ERROR("sem_trywait failed, error code: %d", ret);
        return SDF_LOCK_ERROR_SEM_WAIT_FAIL;
    }
    return SDF_OK;
}

static void MsToTs(struct timespec *ts, int timeout)
{
    int nsec, sec;
    if (ts == NULL) {
        return;
    }
    // timeout单位ms毫秒, 转换为s秒和ns纳秒
    sec  = (timeout >= SEC_TO_MILLISEC) ? (timeout / SEC_TO_MILLISEC) : 0;
    nsec = (timeout - sec * SEC_TO_MILLISEC) * MILLISEC_TO_NANOSEC;
    // 将转换后的结果存入ts结构内
    ts->tv_sec += sec;
    ts->tv_nsec += nsec;
    if (ts->tv_nsec > SEC_TO_NANOSEC) {
        ts->tv_sec++;
        ts->tv_nsec -= SEC_TO_NANOSEC;
    }
}

static uint32_t SemTimeWait(SDF_Sem pSem, int timeout)
{
    struct timespec ts; // 持有间隔的结构，被分解为秒和纳秒
    int ret;
    Sem_S *pSemS = (Sem_S *)pSem;
    if (pSem == NULL) {
        return SDF_LOCK_ERROR_INVALID_PARAM;
    }

    if (timeout < 0) {
        ret = sem_wait(&(pSemS->data.sem));
        if (ret != 0) {
            SDF_LOG_ERROR("sem_wait failed, error code: %d", ret);
            return SDF_LOCK_ERROR_SEM_WAIT_FAIL;
        }
        return SDF_OK;
    }

    ret = clock_gettime(CLOCK_REALTIME, &ts);
    if (ret != 0) {
        SDF_LOG_ERROR("clock_gettime failed, error code: %d", ret);
        return SDF_LOCK_ERROR_TIME_FAIL;
    }

    MsToTs(&ts, timeout);

    while ((ret = sem_timedwait(&(pSemS->data.sem), &ts)) == -1 && errno == EINTR) {
        SDF_LOG_WARN("sem_timedwait errno: EINTR, continue waiting");
        continue;
    }

    if (ret == -1 && errno == ETIMEDOUT) {
        struct timespec currentTime;
        clock_gettime(CLOCK_REALTIME, &currentTime);
        SDF_LOG_ERROR("sem_timedwait timed out, current time: %lld_%lld, time limit: %lld_%lld",
            currentTime.tv_sec, currentTime.tv_nsec, ts.tv_sec, ts.tv_nsec);
        return SDF_LOCK_ERROR_TIME_FAIL;
    }
    return SDF_OK;
}

static uint32_t SemPost(SDF_Sem pSem)
{
    int val;
    Sem_S *pSemS = (Sem_S *)pSem;
    if (pSem == NULL) {
        return SDF_LOCK_ERROR_INVALID_PARAM;
    }

    if (pSemS->type == SDF_SEM_BINARY_TYPE) {
        if (sem_getvalue(&(pSemS->data.sem), &val) != 0 || val == BINARY_SEM_MAX_VALUE) {
            return SDF_LOCK_ERROR_SEM_POST_FAIL;
        }
    }
    int ret = sem_post(&(pSemS->data.sem));
    if (ret != 0) {
        SDF_LOG_ERROR("sem_post failed, error code: %d", ret);
        return SDF_LOCK_ERROR_SEM_POST_FAIL;
    }
    return SDF_OK;
}

SDF_SemHooks_S g_semHooks = {
    .initHook = SemInit,
    .deinitHook = SemDeinit,
    .postHook = SemPost,
    .waitHook = SemWait,
    .tryWaitHook = SemTryWait,
    .timeWaitHook = SemTimeWait,
    .semSize = sizeof(Sem_S),
};

#ifdef __cplusplus
}
#endif
