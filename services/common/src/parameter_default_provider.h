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

#ifndef NEARLINK_DEFAULT_PARAMETER_PROVIDER_H
#define NEARLINK_DEFAULT_PARAMETER_PROVIDER_H

#include "parameter_provider_interface.h"
#include <mutex>
#include <atomic>

namespace OHOS {
namespace Nearlink {

class DefaultParameterProvider : public ParameterProviderInterface {
public:
    static DefaultParameterProvider& GetInstance();

    uint8_t Get5GFeatureEnable() const override;
    uint8_t GetDevicePowerLevel() const override;
    uint8_t GetMuteControlEnable() const override;
    void RegisterParameterChangedCallback(ParameterChangedCallback callback) override;
    void UnregisterParameterChangedCallback() override;

    void Update5GFeatureEnable(uint8_t value) override;
    void UpdateDevicePowerLevel(uint8_t value) override;
    void UpdateMuteControlEnable(uint8_t value) override;

    void TriggerInitialization() override;

private:
    DefaultParameterProvider() = default;
    ~DefaultParameterProvider() = default;
    DefaultParameterProvider(const DefaultParameterProvider&) = delete;
    DefaultParameterProvider& operator=(const DefaultParameterProvider&) = delete;

    mutable std::mutex mutex_;

    std::atomic<uint8_t> feature5GEnable_{0};
    std::atomic<uint8_t> devicePowerLevel_{0};
    std::atomic<uint8_t> muteControlEnable_{0};

    ParameterChangedCallback callback_;
};

}
}

#endif