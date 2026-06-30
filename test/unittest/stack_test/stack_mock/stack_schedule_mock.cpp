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

#include <gtest/gtest.h>
#include <securec.h>
 
#include "stack_schedule_mock.h"

using namespace testing;
using namespace testing::ext;
 
namespace OHOS {
void *g_stackScheduleMock;
 
ScheduleMock::ScheduleMock()
{
    g_stackScheduleMock = reinterpret_cast<void *>(this);
}
 
ScheduleMock::~ScheduleMock()
{
    g_stackScheduleMock = nullptr;
}
 
static ScheduleMockInterface *ScheduleMock()
{
    return reinterpret_cast<ScheduleMockInterface *>(g_stackScheduleMock);
}
 
extern "C" {

uint32_t SchedulePostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    return ScheduleMock()->SchedulePostTask(cb, arg, freeCb);
}

uint32_t SchedulePostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    return ScheduleMock()->SchedulePostTaskBlocked(cb, arg, freeCb, timeout);
}

uint32_t ScheduleTimerAdd(int *handle, SDF_TimerParam *param)
{
    return ScheduleMock()->ScheduleTimerAdd(handle, param);
}

void ScheduleTimerDel(int handle)
{
    return ScheduleMock()->ScheduleTimerDel(handle);
}

}
} // namespace OHOS