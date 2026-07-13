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

#ifndef SDF_SEM_H
#define SDF_SEM_H

#include "sdf_sem_api.h"

#ifdef __cplusplus
extern "C" {
#endif

extern SDF_SemHooks_S g_semHooks;

#define SDF_SemInit(pSem, flag, value)         g_semHooks.initHook((pSem), (flag), (value))
#define SDF_SemDeinit(pSem)                    g_semHooks.deinitHook((pSem))
#define SDF_SemPost(pSem)                      g_semHooks.postHook((pSem))
#define SDF_SemWait(pSem)                      g_semHooks.waitHook((pSem))
#define SDF_SemTryWait(pSem)                   g_semHooks.tryWaitHook((pSem))
#define SDF_SemTimeWait(pSem, timeout)         g_semHooks.timeWaitHook((pSem), (timeout))
#define SDF_SEM_SIZE                           g_semHooks.semSize

#ifdef __cplusplus
}
#endif

#endif /* SDF_SEM_H */
