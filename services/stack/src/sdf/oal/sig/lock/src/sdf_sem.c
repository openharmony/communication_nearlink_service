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
 * @file sdf_lock.c
 * @brief 信号量实现
 * @version 1.0
 * @date 2024-8-7
 */
#include <stddef.h>
#include "sdf_sem.h"
#include "sdf_sem_api.h"
#include "sdf_lock_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 信号量 */
uint32_t SDF_SemReg(const SDF_SemHooks_S *pHooks)
{
    if (pHooks != NULL) {
        g_semHooks.initHook = pHooks->initHook;
        g_semHooks.deinitHook = pHooks->deinitHook;
        g_semHooks.postHook = pHooks->postHook;
        g_semHooks.waitHook = pHooks->waitHook;
        g_semHooks.tryWaitHook = pHooks->tryWaitHook;
        g_semHooks.timeWaitHook = pHooks->timeWaitHook;
        g_semHooks.semSize = pHooks->semSize;
        return SDF_OK;
    }
    return SDF_LOCK_ERROR_INVALID_PARAM;
}

#ifdef __cplusplus
}
#endif
