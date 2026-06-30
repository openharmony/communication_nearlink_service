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

#ifndef SLE_SWITCH_DEPENDENCY_H
#define SLE_SWITCH_DEPENDENCY_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "BaseDef.h"
#include "SleCommonEventSubscriber.h"
#include "system_ability_definition.h"
#include "system_ability_status_change_stub.h"

namespace OHOS {
namespace Nearlink {
class SleSwitchDependency;
class SystemAbilityStatusListener : public SystemAbilityStatusChangeStub {
public:
    explicit SystemAbilityStatusListener(std::weak_ptr<SleSwitchDependency> ptr);
    ~SystemAbilityStatusListener() = default;

    void OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;
    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;

private:
    std::weak_ptr<SleSwitchDependency> ptr_;
};

class SleSwitchDependency : public std::enable_shared_from_this<SleSwitchDependency> {
public:
    using DependencyCallback = std::function<void(void)>;

    explicit SleSwitchDependency(DependencyCallback callback);
    ~SleSwitchDependency();
    int Init(void);

    void OnAddSystemAbility(int32_t systemAbilityId);
private:
    void RegisterDataShareReadyCommonEvent(void);
    void OnDataShareReadyEvent(void);
    void CheckAllDependencySatisfied(void);

    std::mutex isCallbackTriggeredMutex_ {};
    bool isCallbackTriggered_ { false };
    DependencyCallback dependencyCallback_ {};
    sptr<SystemAbilityStatusListener> systemAbilityStatusListener_ { nullptr };

    mutable std::mutex dependedSystemAbilityMapMutex_ {};
    std::map<int32_t, bool> dependedSystemAbilityMap_ {
        {COMMON_EVENT_SERVICE_ID, false},
        {DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID, false},
        {NEARLINK_HOST_SYS_ABILITY_ID, false},
    };

    std::atomic_bool isDataShareReady_ { false };
    std::shared_ptr<SleCommonEventSubscriber> dataShareReadySubscribe_ { nullptr };

    SLE_DISALLOW_COPY_AND_ASSIGN(SleSwitchDependency);
    SLE_DISALLOW_MOVE_AND_ASSIGN(SleSwitchDependency);
};

class SleDataShareCheckUtils final {
public:
    SleDataShareCheckUtils();
    ~SleDataShareCheckUtils() = default;
    static bool IsDataShareReady();
};

}  // namespace Nearlink
}  // namespace OHOS
#endif  // SLE_SWITCH_DEPENDENCY_H