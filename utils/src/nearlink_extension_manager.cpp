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

#include "nearlink_extension_manager.h"
#include <unordered_map>
#include "ffrt_inner.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

ExtensionManager &ExtensionManager::instance()
{
    HILOGD(" instance enter");
    static ExtensionManager mgr;
    return mgr;
}

void ExtensionManager::registerExtension(const std::string &point, ExtFunc func)
{
    HILOGD(" registerExtension enter, point=%{public}s", point.c_str());
    std::lock_guard<ffrt::mutex> lock(mutex_);
    extensions_.map[point].emplace_back(std::move(func));
}

void ExtensionManager::runExtensions(const std::string &point, void *context)
{
    HILOGD(" runExtensions enter, point=%{public}s", point.c_str());
    std::vector<ExtFunc> extensionsToRun;

    {
        std::lock_guard<ffrt::mutex> lock(mutex_);
        auto it = extensions_.map.find(point);
        if (it != extensions_.map.end()) {
            HILOGD(" runExtensions found %{public}zu extensions", it->second.size());
            extensionsToRun = it->second;
        }
    }

    for (auto &fn : extensionsToRun) {
        fn(context);
    }
}

void ExtensionManager::clearExtensions()
{
    HILOGD(" clearExtensions enter");
    std::lock_guard<ffrt::mutex> lock(mutex_);
    extensions_.map.clear();
}

}  // namespace Nearlink
}  // namespace OHOS