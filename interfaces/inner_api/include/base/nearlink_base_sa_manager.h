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
#ifndef NEARLINK_SA_MANAGER_BASE_H
#define NEARLINK_SA_MANAGER_BASE_H

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "iremote_broker.h"
#include "nearlink_def.h"
#include "nearlink_safe_map.h"
#include "system_ability_status_change_stub.h"

namespace OHOS {
namespace Nearlink {

constexpr int32_t INVALID_PROFILE_ID = -1;

class NearlinkRegisterInfo {
public:
    explicit NearlinkRegisterInfo(std::string name) : profileName_(name)
    {}
    ~NearlinkRegisterInfo() = default;

    std::string profileName_ = "";
    std::function<void(sptr<IRemoteObject>)> serviceStartedFunc_;
    std::function<void(void)> serviceStoppedFunc_;
    std::function<void(sptr<IRemoteObject>)> stateOnFunc_;
    std::function<void(sptr<IRemoteObject>)> stateOffFunc_;
};

/**
 * @brief base.so implementation class of framework SA manager.
 *
 * Kept in sync with NearlinkSaManager declared in nearlink_sa_manager.h.
 * GetProxy binds to this class and is shared with the framework.
 */
class NEARLINK_API NearlinkBaseSaManager {
public:
    static NearlinkBaseSaManager &GetInstance();

    sptr<IRemoteObject> GetRemoteProfile(const std::string &profileName);
    int32_t RegisterFunc(std::shared_ptr<NearlinkRegisterInfo> info);
    void DeregisterFunc(int32_t profileId);
    void OnStateChanged(const int transport, const int status);

    NearlinkBaseSaManager();
    ~NearlinkBaseSaManager();
private:
    void SubscribeNearlinkSa();
    void UnsubscribeNearlinkSa();
    void OnServiceStarted();
    void OnServiceStopped();
    sptr<IRemoteObject> GetRemoteHost();
    int32_t AllocateProfileId();

    NearlinkSafeMap<std::string, sptr<IRemoteObject>> profileRemoteMap_;
    NearlinkSafeMap<int32_t, std::shared_ptr<NearlinkRegisterInfo>> profileIdFuncMap_;

    enum class ServiceState : uint32_t {
        SERVICE_STATE_NOT_SUBSCRIBED,
        SERVICE_STATE_STARTED,
        SERVICE_STATE_STOPPED,
    };
    ServiceState serviceState_ = ServiceState::SERVICE_STATE_NOT_SUBSCRIBED;

    class NearlinkSa : public SystemAbilityStatusChangeStub {
        void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
        void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    };

    sptr<NearlinkSa> nearlinkSa_ = nullptr;
    std::mutex serviceStateMutex_;
    std::condition_variable cvfull_;
};

template <typename T>
sptr<T> GetProxy(const std::string &profileName)
{
    return iface_cast<T>(NearlinkBaseSaManager::GetInstance().GetRemoteProfile(profileName));
};
} // namespace Nearlink
} // namespace OHOS
#endif // NEARLINK_SA_MANAGER_BASE_H
