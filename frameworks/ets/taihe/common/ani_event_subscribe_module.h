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
 
#ifndef ANI_EVENT_SUBSCRIBE_MODULE_H
#define ANI_EVENT_SUBSCRIBE_MODULE_H
 
#include <algorithm>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>
 
#include "taihe/callback.hpp"
#include "taihe/optional.hpp"
#include "ani_event_module.h"
 
namespace OHOS {
namespace Nearlink {
 
class EventModuleBase {
public:
    virtual ~EventModuleBase() = default;
};
 
template<typename T>
class EventModuleWrapper : public EventModuleBase {
public:
    EventModule<T> module;
};
 
class AniEventSubscribeModule {
public:
    AniEventSubscribeModule(const char *validEventName, const char *moduleName)
        : validEventNameVec_(std::vector<std::string>{validEventName}), moduleName_(moduleName)
    {}
    AniEventSubscribeModule(std::vector<std::string> validEventNameVec, const char *moduleName)
        : validEventNameVec_(validEventNameVec), moduleName_(moduleName)
    {}
 
    template<typename T>
    void RegisterEvent(std::string name, ::taihe::callback_view<T> callback)
    {
        std::unique_lock<std::shared_mutex> guard(lock_);
        GetOrCreateModule<T>(name).RegisterEvent(callback);
    }
 
    template<typename T>
    void DeregisterEvent(std::string name, ::taihe::optional_view<::taihe::callback<T>> callback)
    {
        std::unique_lock<std::shared_mutex> guard(lock_);
        auto it = eventModules_.find(name);
        if (it != eventModules_.end()) {
            auto *wrapper = static_cast<EventModuleWrapper<T> *>(it->second.get());
            wrapper->module.DeregisterEvent(callback);
        }
    }
 
    void DeregisterAllCallback(std::string name)
    {
        std::unique_lock<std::shared_mutex> guard(lock_);
        eventModules_.erase(name);
    }
 
    template<typename T>
    EventModule<T> &GetModule(std::string name)
    {
        std::unique_lock<std::shared_mutex> guard(lock_);
        return GetOrCreateModule<T>(name);
    }
 
    bool IsValidEventName(const std::string &eventName) const
    {
        return std::find(validEventNameVec_.begin(), validEventNameVec_.end(), eventName) !=
            validEventNameVec_.end();
    }
 
private:
    template<typename T>
    EventModule<T> &GetOrCreateModule(std::string name)
    {
        auto it = eventModules_.find(name);
        if (it == eventModules_.end()) {
            auto wrapper = std::make_shared<EventModuleWrapper<T>>();
            auto &ref = wrapper->module;
            eventModules_[name] = std::move(wrapper);
            return ref;
        }
        return static_cast<EventModuleWrapper<T> *>(it->second.get())->module;
    }
 
    std::map<std::string, std::shared_ptr<EventModuleBase>> eventModules_;
    std::vector<std::string> validEventNameVec_;
    std::string moduleName_;
    std::shared_mutex lock_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // ANI_EVENT_SUBSCRIBE_MODULE_H