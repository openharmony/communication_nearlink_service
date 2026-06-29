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

#include "nearlink_plugin_loader.h"
#include <dlfcn.h>
#include <atomic>
#include "ffrt_inner.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

namespace {
using PluginInitFunc = void(*)();
std::atomic<bool> pluginLoaded_{false};
void* pluginHandle_ = nullptr;
ffrt::mutex pluginMutex_;
}

void LoadHadmClosedPlugin()
{
    std::lock_guard<ffrt::mutex> lock(pluginMutex_);

    if (pluginLoaded_.load()) {
        HILOGI("Plugin already loaded, skip");
        return;
    }

    const char* pluginPath = "libnearlink_hadm_closed_plugin.z.so";

    pluginHandle_ = dlopen(pluginPath, RTLD_NOW | RTLD_LOCAL);
    if (pluginHandle_ == nullptr) {
        const char* error = dlerror();
        HILOGE("Failed to load Hadm closed plugin: %{public}s", error ? error : "unknown error");
        return;
    }

    auto initFunc = reinterpret_cast<PluginInitFunc>(dlsym(pluginHandle_, "InitHadmPlugins"));
    if (initFunc != nullptr) {
        initFunc();
        HILOGI("Plugin initialization completed");
    } else {
        HILOGW("No initialization function found in plugin");
    }

    pluginLoaded_.store(true);
    HILOGI("Successfully loaded Hadm closed plugin: %{public}s", pluginPath);
}

void UnloadHadmClosedPlugin()
{
    std::lock_guard<ffrt::mutex> lock(pluginMutex_);

    if (!pluginLoaded_.load()) {
        HILOGI("Plugin not loaded, skip unloading");
        return;
    }

    if (pluginHandle_ != nullptr) {
        dlclose(pluginHandle_);
        pluginHandle_ = nullptr;
        HILOGI("Plugin handle closed");
    }

    pluginLoaded_.store(false);
    HILOGI("Successfully unloaded Hadm closed plugin");
}

bool IsPluginLoaded()
{
    return pluginLoaded_.load();
}

}  // namespace Nearlink
}  // namespace OHOS