/*
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

#include <unistd.h>
#include "cstdint"
#include "log.h"
#include "SleDliCallbacks.h"
#include "SleDliSnoop.h"

static const int SLE_THREAD_PRIORITY = 1;
thread_local bool SleDliCallbacks::isThreadPromoted = false;

int32_t SleDliCallbacks::initializationComplete(SleStatus status)
{
    if ((callbacks_ != nullptr) && (callbacks_->initializationComplete)) {
        SetRTSchedule();
        SleInitStatus initStatus = SleInitStatus::SUCCESS;
        if (status != SleStatus::SUCCESS) {
            initStatus = SleInitStatus::INITIALIZATION_ERROR;
        }
        callbacks_->initializationComplete(initStatus);
    }
    return 0;
}

static void dliPacketPrint(const std::vector<uint8_t> &data)
{
    #define MAX_PRINT_LEN 50 // 50 is max print len
    char eventStr[MAX_PRINT_LEN + MAX_PRINT_LEN + 1] = { 0 };
    int count = 0;
    for (auto iter = data.begin(); iter != data.end(); iter++) {
        uint8_t temp = *iter;
        (void)sprintf_s(&eventStr[2 * count], (MAX_PRINT_LEN - count) * 2, "%02x", temp); // 2 hex char
        if (++count >= MAX_PRINT_LEN - 1) {
            break;
        }
    }
    LOG_DEBUG("len=%{public}zu, dli=%{public}s", data.size(), eventStr);
}

int32_t SleDliCallbacks::hciPacketReceived(uint32_t type, const std::vector<uint8_t> &data)
{
    if ((callbacks_ != nullptr) && (callbacks_->dliPacketReceived)) {
        SetRTSchedule();
        SlePacket packet = {
            .data = (data.size() ? (uint8_t *)&data[0] : nullptr),
            .size = static_cast<uint32_t>(data.size()),
        };
        SleDliSnoop::GetInstance().DliSnoopCapture(type, data, true);
        dliPacketPrint(data);
        const SlePacket *slePacket = &packet;
        callbacks_->dliPacketReceived((SlePacketType)type, slePacket);
    }
    return 0;
}

void SleDliCallbacks::SetRTSchedule()
{
    if (isThreadPromoted) {
        return;
    }
    pid_t tid = gettid();
    struct sched_param rtParams = {.sched_priority = SLE_THREAD_PRIORITY};
    int rc = sched_setscheduler(tid, SCHED_FIFO, &rtParams);
    isThreadPromoted = (rc != 0) ? false : true;
}