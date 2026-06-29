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
*/

#ifndef SL_REG_EXT_FUNC_H
#define SL_REG_EXT_FUNC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t (*SL_HostInitPtr)(void);
typedef void (*SL_HostDeinitPtr)(void);

typedef struct {
    SL_HostInitPtr hostInit;
    SL_HostDeinitPtr hostDeinit;
} SL_ExtFuncList;

void SL_RegisterExtFunc(void *soHandle);

void SL_DeregisterExtFunc(void);

SL_ExtFuncList *SL_GetExtFuncList(void);

#ifdef __cplusplus
}
#endif

#endif