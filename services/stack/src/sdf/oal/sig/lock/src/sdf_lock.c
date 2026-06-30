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
 * @file sdf_lock.c
 * @brief 锁实现
 * @version 1.0
 * @date 2024-8-7
 */
#include <stddef.h>
#include "sdf_mutex.h"
#include "sdf_lock_api.h"
#include "sdf_lock_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 互斥锁 */
uint32_t SDF_MutexReg(const SDF_MutexLockHooks_S *pHooks)
{
    if (pHooks != NULL) {
        g_mutexLockHooks.initHook = pHooks->initHook;
        g_mutexLockHooks.deinitHook = pHooks->deinitHook;
        g_mutexLockHooks.lockHook = pHooks->lockHook;
        g_mutexLockHooks.tryLockHook = pHooks->tryLockHook;
        g_mutexLockHooks.unlockHook = pHooks->unlockHook;
        g_mutexLockHooks.lockSize = pHooks->lockSize;
        g_mutexLockHooks.attrInitHook = pHooks->attrInitHook;
        g_mutexLockHooks.attrSetHook = pHooks->attrSetHook;
        return SDF_OK;
    }
    return SDF_LOCK_ERROR_INVALID_PARAM;
}

#ifdef __cplusplus
}
#endif
