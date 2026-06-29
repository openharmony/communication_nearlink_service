/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
#ifndef STACK_SCHEDULE_MOCK_H
#define STACK_SCHEDULE_MOCK_H
 
#include <gmock/gmock.h>
#include <stdint.h>
#include "sdf_timer.h"
#include "sdf_worker.h"
 
namespace OHOS {
class ScheduleMockInterface {
public:
    ScheduleMockInterface() {};
    virtual ~ScheduleMockInterface() {};
    virtual uint32_t SchedulePostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb) = 0;
    virtual uint32_t SchedulePostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout) = 0;
    virtual uint32_t ScheduleTimerAdd(int *handle, SDF_TimerParam *param) = 0;
    virtual void ScheduleTimerDel(int handle) = 0;
};
 
class ScheduleMock : public ScheduleMockInterface {
public:
    ScheduleMock();
    ~ScheduleMock() override;
    MOCK_METHOD(uint32_t, SchedulePostTask, (SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb), (override));
    MOCK_METHOD(uint32_t, SchedulePostTaskBlocked, (SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout),
        (override));
    MOCK_METHOD(uint32_t, ScheduleTimerAdd, (int *handle, SDF_TimerParam *param), (override));
    MOCK_METHOD(void, ScheduleTimerDel, (int handle), (override));
    static ScheduleMock& GetMock();
 
private:
    static ScheduleMock *gMock;
};
 
}; // namespace OHOS
#endif