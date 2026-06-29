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

#include "parameter_manager.h"
#include "parameter_default_provider.h"
#include "DynamicLibraryLoader.h"
#include "log.h"
#include <mutex>

namespace OHOS {
namespace Nearlink {

namespace {
const char* const CLOUDPUSH_EXT_LIB_NAME = "libnearlink_cloud_push.z.so";
const char* const LOAD_PROVIDER_FUNC_NAME = "LoadParameterProvider";
}

std::shared_ptr<ParameterProviderInterface> ParameterManager::provider_ = nullptr;
std::mutex ParameterManager::providerMutex_;

std::shared_ptr<ParameterProviderInterface> ParameterManager::GetProvider()
{
    std::lock_guard<std::mutex> lock(providerMutex_);
    if (!provider_) {
        DefaultParameterProvider& instance = DefaultParameterProvider::GetInstance();
        provider_ = std::shared_ptr<ParameterProviderInterface>(&instance, [](ParameterProviderInterface*) {});
    }
    return provider_;
}

void ParameterManager::RegisterProvider(std::shared_ptr<ParameterProviderInterface> provider)
{
    if (!provider) {
        return;
    }
    std::lock_guard<std::mutex> lock(providerMutex_);
    provider_ = std::move(provider);
}

uint8_t ParameterManager::Get5GFeatureEnable()
{
    return GetProvider()->Get5GFeatureEnable();
}

uint8_t ParameterManager::GetDevicePowerLevel()
{
    return GetProvider()->GetDevicePowerLevel();
}

uint8_t ParameterManager::GetMuteControlEnable()
{
    return GetProvider()->GetMuteControlEnable();
}

void ParameterManager::NotifyTriggerInitialization()
{
    GetProvider()->TriggerInitialization();
}

void ParameterManager::LoadProvider()
{
    CDynamicLibraryLoader loader(CLOUDPUSH_EXT_LIB_NAME);
    if (!loader.IsLibraryLoaded()) {
        loader.OpenLib();
        HILOGI("[PARAM_EXT] load cloud_push ext lib");
    }

    auto loadFn = reinterpret_cast<void(*)()>(loader.GetSymbol(LOAD_PROVIDER_FUNC_NAME));
    if (loadFn != nullptr) {
        loadFn();
        HILOGI("[PARAM_EXT] parameter provider loaded");
    } else {
        HILOGE("[PARAM_EXT] LoadParameterProvider not found, fallback to default");
    }
}
}
}