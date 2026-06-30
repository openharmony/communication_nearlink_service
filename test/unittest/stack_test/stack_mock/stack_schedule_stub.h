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

#ifndef STACK_SCHEDULE_STUB_H
#define STACK_SCHEDULE_STUB_H

#include "sdf_addr.h"
#include "sdf_timer.h"
#include "nlstk_sm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

void TEST_ScheduleInit();
void TEST_StackScheduleDeInit();

uint32_t TEST_ScheduleTimerAddStub(int *handle, SDF_TimerParam *param);
void TEST_ScheduleTimerDelStub(int handle);
uint32_t TEST_SchedulePostTaskStub(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb);
uint32_t TEST_SchedulePostTaskStubFail(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb);
uint32_t TEST_SchedulePostTaskBlockedStub(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout);
uint32_t TEST_SchedulePostTaskBlockedStubFail(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout);
uint32_t TEST_SchedulePostTaskQueueStub(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb);

void TEST_RunQueueStubSchedule();
void TEST_RunQueueStubScheduleOnce();

#ifdef __cplusplus
}
#endif

#endif

