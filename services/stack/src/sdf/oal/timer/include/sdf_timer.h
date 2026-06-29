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
 */

#ifndef SDF_TIMER_H
#define SDF_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/timerfd.h>
#include "sdf_evc.h"
#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDF_TIMER_ERROR_ALREADY_INIT            SDF_MAKE_BSL_TIMER_ERRNO(2)
#define SDF_TIMER_ERROR_MEM_FAILED              SDF_MAKE_BSL_TIMER_ERRNO(3)
#define SDF_TIMER_ERROR_MAX_INSTANCE            SDF_MAKE_BSL_TIMER_ERRNO(4)
#define SDF_TIMER_ERROR_TIMER_CREATE_FAILED     SDF_MAKE_BSL_TIMER_ERRNO(5)
#define SDF_TIMER_ERROR_TIMER_SET_FAILED        SDF_MAKE_BSL_TIMER_ERRNO(6)
#define SDF_TIMER_ERROR_EVC_FAILED              SDF_MAKE_BSL_TIMER_ERRNO(7)
#define SDF_TIMER_ERROR_MUTEX_FAILED            SDF_MAKE_BSL_TIMER_ERRNO(8)

uint32_t SDF_TimerAdd(int *handle, SDF_TimerParam *param);
void SDF_TimerDel(int handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
