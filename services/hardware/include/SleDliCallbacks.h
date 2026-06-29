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
#ifndef SLE_DLI_CALLBACK_H
#define SLE_DLI_CALLBACK_H

#pragma once

#include <stdint.h>
#include <cstdint>
#include <v1_0/sle_hci_types.h>
#include <v1_0/isle_hci_callback.h>
#include "SleDliLayerAdapter.h"
#include "vector"

using OHOS::HDI::Nearlink::Hci::V1_0::ISleHciCallback;
using OHOS::HDI::Nearlink::Hci::V1_0::SleStatus;

class SleDliCallbacks : public ISleHciCallback {
public:
    SleDliCallbacks(SleDliCallbackFunc *callbacks) : callbacks_(callbacks) {}
    virtual ~SleDliCallbacks() {}

    int32_t initializationComplete(SleStatus status) override;

    int32_t hciPacketReceived(uint32_t type, const std::vector<uint8_t> &data) override;

private:
    void SetRTSchedule();

    SleDliCallbackFunc *callbacks_;
    thread_local static bool isThreadPromoted;
};

#endif