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

#include "parameter_default_provider.h"
#include <mutex>

namespace OHOS {
namespace Nearlink {

DefaultParameterProvider& DefaultParameterProvider::GetInstance()
{
    static DefaultParameterProvider instance;
    return instance;
}

uint8_t DefaultParameterProvider::Get5GFeatureEnable() const
{
    return feature5GEnable_.load(std::memory_order_acquire);
}

uint8_t DefaultParameterProvider::GetDevicePowerLevel() const
{
    return devicePowerLevel_.load(std::memory_order_acquire);
}

uint8_t DefaultParameterProvider::GetMuteControlEnable() const
{
    return muteControlEnable_.load(std::memory_order_acquire);
}

void DefaultParameterProvider::RegisterParameterChangedCallback(ParameterChangedCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

void DefaultParameterProvider::UnregisterParameterChangedCallback()
{
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = nullptr;
}

void DefaultParameterProvider::Update5GFeatureEnable(uint8_t value)
{
    feature5GEnable_.store(value, std::memory_order_release);
}

void DefaultParameterProvider::UpdateDevicePowerLevel(uint8_t value)
{
    devicePowerLevel_.store(value, std::memory_order_release);
}

void DefaultParameterProvider::UpdateMuteControlEnable(uint8_t value)
{
    muteControlEnable_.store(value, std::memory_order_release);
}

void DefaultParameterProvider::TriggerInitialization()
{
}

}
}