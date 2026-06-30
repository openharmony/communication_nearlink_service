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
 * @file sdf_mutex.h
 * @brief 锁模块对外接口
 * @version 1.0
 * @date 2024-8-7
 */

#ifndef SDF_MUTEX_H
#define SDF_MUTEX_H

#include "sdf_lock_api.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SDF_MutexLockHooks_S g_mutexLockHooks;

#define SDF_MutexInit(pLock, pAttr)     g_mutexLockHooks.initHook((pLock), (pAttr))
#define SDF_MutexDeinit(pLock)          g_mutexLockHooks.deinitHook((pLock))
#define SDF_MutexLock(pLock)            g_mutexLockHooks.lockHook((pLock))
#define SDF_MutexTryLock(pLock)         g_mutexLockHooks.tryLockHook((pLock))
#define SDF_MutexUnlock(pLock)          g_mutexLockHooks.unlockHook((pLock))
#define SDF_MUTEX_LOCK_SIZE             g_mutexLockHooks.lockSize

uint32_t SDF_RecursiveMutexInit(SDF_MutexLock pLock);

#ifdef __cplusplus
}
#endif

#endif /* SDF_MUTEX_H */
