/*
 * Copyright (C) 2024-2024 Huawei Device Co., Ltd.
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
#include "SleSwitchDependency.h"

#include "ThreadUtil.h"
#include "datashare_helper.h"
#include "iservice_registry.h"
#include "log.h"
#include "SleServiceFfrtLog.h"

namespace OHOS {
namespace Nearlink {
constexpr const char *SETTINGS_DATASHARE_URI =
    "datashare:///com.ohos.settingsdata/entry/settingsdata/SETTINGSDATA?Proxy=true";
constexpr const char *SETTINGS_DATASHARE_EXTENSION_URI = "datashare://com.ohos.settingsdata.DataAbility";

SleSwitchDependency::SleSwitchDependency(DependencyCallback callback)
{
    dependencyCallback_ = [this, callback]() {
        {
            std::lock_guard<std::mutex> lock(isCallbackTriggeredMutex_);
            if (isCallbackTriggered_) {
                HILOGW("sle switch dependency callback is triggered");
                return;
            }
            isCallbackTriggered_ = true;
        }
        if (callback) {
            callback();
        }
    };
}

SleSwitchDependency::~SleSwitchDependency()
{
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        HILOGE("failed to get samgrProxy");
        return;
    }
    if (systemAbilityStatusListener_ == nullptr) {
        HILOGE("systemAbilityStatusListener_ is nullptr!");
        return;
    }
    std::lock_guard<std::mutex> lock(dependedSystemAbilityMapMutex_);
    for (auto [systemAbilityId, _] : dependedSystemAbilityMap_) {
        samgrProxy->UnSubscribeSystemAbility(systemAbilityId, systemAbilityStatusListener_);
    }

    if (dataShareReadySubscribe_) {
        EventFwk::CommonEventManager::UnSubscribeCommonEvent(dataShareReadySubscribe_);
    }
    systemAbilityStatusListener_ = nullptr;
}

int SleSwitchDependency::Init()
{
    HILOGI("enter");
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        HILOGE("failed to get samgrProxy");
        return -1;
    }

    systemAbilityStatusListener_ = new (std::nothrow) SystemAbilityStatusListener(shared_from_this());
    if (systemAbilityStatusListener_ == nullptr) {
        HILOGE("systemAbilityStatusListener_ is nullptr!");
        return -1;
    }

    {
        std::lock_guard<std::mutex> lock(dependedSystemAbilityMapMutex_);
        for (auto [systemAbilityId, _] : dependedSystemAbilityMap_) {
            HILOGI("subscribe systemAbilityId = %{public}d", systemAbilityId);
            int32_t ret = samgrProxy->SubscribeSystemAbility(systemAbilityId, systemAbilityStatusListener_);
            if (ret != ERR_OK) {
                HILOGE("subscribe systemAbilityId(%{public}d) failed!", systemAbilityId);
                systemAbilityStatusListener_ = nullptr;
                return -1;
            }
        }
    }
    return 0;
}

void SleSwitchDependency::RegisterDataShareReadyCommonEvent()
{
    HILOGI("enter");
    uint32_t coreEventPriority = 1;
    EventFwk::MatchingSkills matchingSkills;
    std::string commonEventDatashareReady = "usual.event.DATA_SHARE_READY";
    matchingSkills.AddEvent(commonEventDatashareReady);

    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    subscribeInfo.SetPriority(coreEventPriority);

    auto func = [ptr = weak_from_this()](const OHOS::EventFwk::CommonEventData &data) {
        // Work in CommonEvent thread, need post task first.
        DoInServiceManagerThread([ptr]() -> void {
            auto switchDependencyPtr = ptr.lock();
            if (switchDependencyPtr == nullptr) {
                HILOGE("switchDependencyPtr is nullptr");
                return;
            }
            switchDependencyPtr->OnDataShareReadyEvent();
        });
    };
    dataShareReadySubscribe_ =
        std::make_shared<SleCommonEventSubscriber>(subscribeInfo, commonEventDatashareReady, func);
    if (!EventFwk::CommonEventManager::SubscribeCommonEvent(dataShareReadySubscribe_)) {
        HILOGE("Subscribe DATA_SHARE_READY common event failed");
        // no need return
    }
}

void SleSwitchDependency::CheckAllDependencySatisfied()
{
    bool isAllDependencyOn = true;
    {
        // Check all dependended system ability is on
        std::lock_guard<std::mutex> lock(dependedSystemAbilityMapMutex_);
        for (auto [_, isSystemAbilityOn] : dependedSystemAbilityMap_) {
            if (!isSystemAbilityOn) {
                isAllDependencyOn = false;
            }
        }
    }
    isAllDependencyOn = isDataShareReady_.load() ? isAllDependencyOn : false;
    if (isAllDependencyOn) {
        HILOGI("CheckAllDependencySatisfied success");
        dependencyCallback_();
    }
}

void SleSwitchDependency::OnDataShareReadyEvent()
{
    HILOGI("DataShare is ready");
    isDataShareReady_ = true;
    CheckAllDependencySatisfied();
}

static std::pair<int, std::shared_ptr<DataShare::DataShareHelper>> CreateDataShareHelper()
{
    HILOGI("enter");
    sptr<ISystemAbilityManager> saManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (saManager == nullptr) {
        HILOGE("GetSystemAbilityManager failed.");
        return std::make_pair(DataShare::E_DATA_SHARE_NOT_READY, nullptr);
    }
    sptr<IRemoteObject> remoteObj = saManager->GetSystemAbility(COMM_NET_CONN_MANAGER_SYS_ABILITY_ID);
    if (remoteObj == nullptr) {
        HILOGE("GetSystemAbility Service Failed.");
        return std::make_pair(DataShare::E_DATA_SHARE_NOT_READY, nullptr);
    }
    std::pair<int, std::shared_ptr<DataShare::DataShareHelper>> helperPair =
        DataShare::DataShareHelper::Create(remoteObj, SETTINGS_DATASHARE_URI, SETTINGS_DATASHARE_EXTENSION_URI);
    if (helperPair.first != DataShare::E_OK) {
        HILOGE("DataShareHelper create failed, ret: %{public}d", helperPair.first);
    }
    return helperPair;
}

void SleSwitchDependency::OnAddSystemAbility(int32_t systemAbilityId)
{
    HILOGI("SleSwitchDependency OnAddSystemAbility = %{public}d", systemAbilityId);
    // Must wait common event sa started
    if (systemAbilityId == COMMON_EVENT_SERVICE_ID && !isDataShareReady_.load()) {
        RegisterDataShareReadyCommonEvent();
    }
    if (systemAbilityId == DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID) {
        isDataShareReady_ = SleDataShareCheckUtils::IsDataShareReady();
        HILOGI("datashare is ready: %{public}d", isDataShareReady_.load());
    }

    {
        // Update systemAbility information
        std::lock_guard<std::mutex> lock(dependedSystemAbilityMapMutex_);
        auto it = dependedSystemAbilityMap_.find(systemAbilityId);
        if (it == dependedSystemAbilityMap_.end()) {
            HILOGE("systemAbilityId(%{public}d) is not the listener id", systemAbilityId);
            return;
        }
        it->second = true;
    }
    CheckAllDependencySatisfied();
}

bool SleDataShareCheckUtils::IsDataShareReady()
{
    auto [ret, _] = CreateDataShareHelper();
    return ret != DataShare::E_DATA_SHARE_NOT_READY;
}

SystemAbilityStatusListener::SystemAbilityStatusListener(std::weak_ptr<SleSwitchDependency> ptr) : ptr_(ptr)
{
    HILOGI("SystemAbilityStatusListener constructed");
}

void SystemAbilityStatusListener::OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    HILOGI("systemAbilityId(%{public}d) is added", systemAbilityId);
    DoInServiceManagerThread([systemAbilityId, ptr = ptr_]() -> void {
        auto switchDependencyPtr = ptr.lock();
        if (switchDependencyPtr == nullptr) {
            HILOGE("switchDependencyPtr is nullptr");
            return;
        }
        switchDependencyPtr->OnAddSystemAbility(systemAbilityId);
    });
}

void SystemAbilityStatusListener::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId)
{
    HILOGI("systemAbilityId(%{public}d) is removed", systemAbilityId);
}
}  // namespace Nearlink
}  // namespace OHOS