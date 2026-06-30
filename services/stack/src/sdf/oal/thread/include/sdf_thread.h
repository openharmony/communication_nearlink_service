/**
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
 * @file         sdf_thread.h
 * @brief        SDF Thread head file.
 */

#ifndef SDF_THREAD_H
#define SDF_THREAD_H

#include <stdint.h>
#include "sdf_errno_base.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Thread Error Code
/* The parameter is invalid. */
#define SDF_THREAD_ERROR_INVALID_PARAM     SDF_MAKE_BSL_THREAD_ERRNO(2)
/* The event notification is not satisfied. */
#define SDF_THREAD_ERROR_EV_NOT_SATISFIED  SDF_MAKE_BSL_THREAD_ERRNO(3)
/* The thread is already initialized. */
#define SDF_THREAD_ERROR_ALREAD_INIT       SDF_MAKE_BSL_THREAD_ERRNO(4)
/* Memory allocation failure. */
#define SDF_THREAD_ERROR_MEM_ALLOC         SDF_MAKE_BSL_THREAD_ERRNO(5)
/* pthread_key_create failed. */
#define SDF_THREAD_ERROR_THREAD_KEY_CREATE SDF_MAKE_BSL_THREAD_ERRNO(6)
/* pthread_rwlock_init failed. */
#define SDF_THREAD_ERROR_LOCK_INIT         SDF_MAKE_BSL_THREAD_ERRNO(7)
/* Lock acquisition failure. */
#define SDF_THREAD_ERROR_LOCK_ACQUIRE      SDF_MAKE_BSL_THREAD_ERRNO(8)
/* Mutex initialization failure. */
#define SDF_THREAD_ERROR_MUTEX_INIT        SDF_MAKE_BSL_THREAD_ERRNO(9)
/* Condition attrs initialization failure. */
#define SDF_THREAD_ERROR_COND_INIT         SDF_MAKE_BSL_THREAD_ERRNO(10)
/* Thread initialization failure. */
#define SDF_THREAD_ERROR_THREAD_INIT       SDF_MAKE_BSL_THREAD_ERRNO(11)
/* pthread_join failure. */
#define SDF_THREAD_ERROR_THREAD_JOIN       SDF_MAKE_BSL_THREAD_ERRNO(12)
/* clock_gettime failure. */
#define SDF_THREAD_ERROR_GETTIME           SDF_MAKE_BSL_THREAD_ERRNO(13)
/* Time Overflow. */
#define SDF_THREAD_ERROR_TIME_OVERFLOW     SDF_MAKE_BSL_THREAD_ERRNO(14)
/* pthread_cond_timedwait failure. */
#define SDF_THREAD_ERROR_COND_WAIT         SDF_MAKE_BSL_THREAD_ERRNO(15)
/* The thread ID is invalid. */
#define SDF_THREAD_ERROR_INVALID_THREADID  SDF_MAKE_BSL_THREAD_ERRNO(16)
/* pthread_attr_set failure. */
#define SDF_THREAD_ERROR_SET_ATTR          SDF_MAKE_BSL_THREAD_ERRNO(16)
/* pthread_getspecific failure. */
#define SDF_THREAD_ERROR_GET_SPECIFIC      SDF_MAKE_BSL_THREAD_ERRNO(17)
/* pthread_setspecific failure. */
#define SDF_THREAD_ERROR_SET_SPECIFIC      SDF_MAKE_BSL_THREAD_ERRNO(18)
/* pthread_key_delete failure. */
#define SDF_THREAD_ERROR_KEY_DELETE        SDF_MAKE_BSL_THREAD_ERRNO(19)
/**
 * @brief Number of thread callback parameters
 */
#define SDF_THREAD_ARGS_NUM 4

#define SDF_THREAD_EV_ANY ((uint32_t)1 << 26)

#define SDF_THREAD_NO_WAIT ((uint32_t)1 << 31)

/**
 * @brief Scheduling policy
 */
typedef enum {
    /* System default scheduling */
    SDF_THREAD_SCHED_OTHER = 0,
    /* First in first out */
    SDF_THREAD_SCHED_FIFO,
    /* Round robin */
    SDF_THREAD_SCHED_RR,
} SDF_ThreadSchedPolicy_E;

#define SDF_THREAD_INVALID_ID ((uint32_t)0)

/* Thread callback function type */
typedef void (*SDF_ThreadFunc)(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3);

/**
 * @brief Thread local variable type
 */
typedef void *SDF_ThreadLocalKey;

/**
 * @brief Thread init parameters
 */
typedef struct {
    const char *name;
    SDF_ThreadSchedPolicy_E policy;
    /* Scheduling priority */
    int32_t priority;
    uint32_t stackSize;
    /* Thread callback function */
    SDF_ThreadFunc hook;
    /* Thread callback function args list */
    uintptr_t args[SDF_THREAD_ARGS_NUM];
} SDF_ThreadCreateParam_S;

/**
 * @brief Thread init
 * @attention Non-thread safe
 * @param maxNum [IN] Maximum number of threads
 * @retval #SDF_OK
 * @retval #SDF_THREAD_ERROR_INVALID_PARAM
 * @retval #SDF_THREAD_ERROR_ALREAD_INIT
 * @retval #SDF_THREAD_ERROR_MEM_ALLOC
 * @retval #SDF_THREAD_ERROR_THREAD_KEY_CREATE
 */
uint32_t SDF_ThreadInit(uint16_t maxNum);

/**
 * @brief Thread Deinit
 * @attention Non-thread safe
 * @attention Ensure that all threads have been executed before deinit.
 * @return void
 */
void SDF_ThreadDeinit(void);

/**
 * @brief Thread create and run
 * @attention Thread safe
 * @param threadId [OUT] Returned thread logical ID
 * @param param [IN] SDF_ThreadCreateParam_S
 * @retval #SDF_OK
 * @retval #SDF_THREAD_ERROR_INVALID_PARAM
 * @retval #SDF_THREAD_ERROR_LOCK_ACQUIRE
 * @retval #SDF_THREAD_ERROR_MEM_ALLOC
 * @retval #SDF_THREAD_ERROR_MUTEX_INIT
 * @retval #SDF_THREAD_ERROR_COND_INIT
 * @retval #SDF_THREAD_ERROR_THREAD_INIT
 */
uint32_t SDF_ThreadCreate(uint32_t *threadId, const SDF_ThreadCreateParam_S *param);

/**
 * @brief Get logical ID of current thread
 * @return Thread handle ID
 */
uint32_t SDF_ThreadSelfId(void);

/**
 * @brief Thread sleep
 *
 * @param ms [IN] sleep time(ms)
 * @return void
 */
void SDF_ThreadSleep(uint32_t ms);

/**
 * @brief Current thread yielding CPU
 * @return void
 */
void SDF_ThreadYield(void);

/**
 * @brief Exit and clear thread resources after all threads have been executed
 * @param threadId [IN] Thread logical ID
 * @retval #SDF_OK
 * @retval #SDF_THREAD_ERROR_INVALID_PARAM
 * @retval #SDF_THREAD_ERROR_THREAD_JOIN
 */
uint32_t SDF_ThreadJoin(uint32_t threadId);

/**
 * @brief Local key Initialization
 *
 * @param key [IN] Thread-local key
 * @param cleanup [IN] Cleanup callback function
 * @retval #SDF_OK
 * @retval #SDF_THREAD_ERROR_INVALID_PARAM
 * @retval #SDF_THREAD_ERROR_THREAD_KEY_CREATE
 */
uint32_t SDF_ThreadLocalKeyInit(SDF_ThreadLocalKey *key, void (*cleanup)(void *));

/**
 * @brief Set local key
 *
 * @param key [IN] Thread-local key
 * @param data [IN] Binding date
 * @retval #SDF_OK
 * @retval #SDF_ERR
 */
uint32_t SDF_ThreadSetLocal(SDF_ThreadLocalKey key, void *data);

/**
 * @brief Get local key
 *
 * @param key [IN] Thread-local key
 * @return Data
 */
void *SDF_ThreadGetLocal(SDF_ThreadLocalKey key);

/**
 * @brief Cleanup local key
 *
 * @param key [IN] Thread-local key
 * @retval #SDF_OK
 * @retval #SDF_ERR
 */
uint32_t SDF_ThreadCleanupLocal(SDF_ThreadLocalKey key);

#ifdef __cplusplus
}
#endif

#endif // SDF_THREAD_H
