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

#ifndef SDF_EVENT_H
#define SDF_EVENT_H

#include <stdint.h>
#include <stdbool.h>
#include "sdf_evc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDF_EVENT_ERROR_ALREADY_INIT           SDF_MAKE_BSL_EVC_ERRNO(21)
#define SDF_EVENT_ERROR_MEM_FAILED             SDF_MAKE_BSL_EVC_ERRNO(22)
#define SDF_EVENT_ERROR_MAX_INSTANCE           SDF_MAKE_BSL_EVC_ERRNO(23)
#define SDF_EVENT_ERROR_CREATE_FD_FAILED       SDF_MAKE_BSL_EVC_ERRNO(24)
#define SDF_EVENT_ERROR_EVC_FAILED             SDF_MAKE_BSL_EVC_ERRNO(25)
#define SDF_EVENT_ERROR_WRONG_HANDLE           SDF_MAKE_BSL_EVC_ERRNO(26)
#define SDF_EVENT_ERROR_POST_FAILED            SDF_MAKE_BSL_EVC_ERRNO(27)
#define SDF_EVENT_ERROR_MUTEX_FAILED           SDF_MAKE_BSL_EVC_ERRNO(28)

typedef void (*SDF_EventCallback)(void *arg);

typedef struct {
    int handle;
    SDF_EventCallback callback;
    void *args;
} SDF_EventParam;

uint32_t SDF_EventAdd(int *handle, SDF_EventParam *param);
uint32_t SDF_EventPost(int handle);
void SDF_EventDel(int handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
