/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "nlstk_schedule.h"
#include "cp_worker.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t CP_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    return SchedulePostTask(cb, arg, freeCb);
}

uint32_t CP_PostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    return SchedulePostTaskBlocked(cb, arg, freeCb, timeout);
}

uint32_t CP_TimerAdd(int *handle, SDF_TimerParam *param)
{
    return ScheduleTimerAdd(handle, param);
}

void CP_TimerDel(int handle)
{
    return ScheduleTimerDel(handle);
}

#ifdef __cplusplus
}
#endif