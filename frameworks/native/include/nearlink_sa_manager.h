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
#ifndef NEARLINK_SA_MANAGER_H
#define NEARLINK_SA_MANAGER_H

#include <condition_variable>
#include <string>
#include <mutex>
#include "iremote_broker.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "system_ability_status_change_stub.h"
#include "nearlink_safe_map.h"

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

class NearlinkSaManager {
public:
    /**
     * @brief Get default host device.
     *
     * @return Returns the singleton instance.
     * @since 6
     */
    static NearlinkSaManager &GetInstance();

    /**
     * @brief Get the Remote object of the Profile.
     *
     * @param profileName the profileName of profile
     *
     * @return Returns the Remote of the Profile.
     */
    sptr<IRemoteObject> GetRemoteProfile(const std::string &profileName);

    /**
     * @brief register function for profile to get proxy when profile is init
     *
     * @param Info the register info
     *
     * @return Returns the profileId of the Profile.
     */
    int32_t RegisterFunc(std::shared_ptr<NearlinkRegisterInfo> info);

    /**
     * @brief Deregister function for profile, ensure that there is a deregister after register.
     *
     * @param profileId the id of profile
     */
    void DeregisterFunc(int32_t profileId);

    /**
     * @brief Notify state change.
     *
     * @param transport Transport type when state change.
     *        SleTransport::ADAPTER_SLB : classic;
     *        SleTransport::ADAPTER_SLE : sle.
     * @param state Change to the new state.
     *        SleStateID::STATE_TURNING_ON;
     *        SleStateID::STATE_TURN_ON;
     *        SleStateID::STATE_TURNING_OFF;
     *        SleStateID::STATE_TURN_OFF.
     */
    void OnStateChanged(const int transport, const int status);

    NearlinkSaManager();

    ~NearlinkSaManager();
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
        SERVICE_STATE_NOT_SUBSCRIBED, // 未订阅服务启动
        SERVICE_STATE_STARTED, // 服务已启动
        SERVICE_STATE_STOPPED, // 服务已停止
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
    return iface_cast<T>(NearlinkSaManager::GetInstance().GetRemoteProfile(profileName));
};
} // namespace Nearlink
} // namespace OHOS
#endif // NEARLINK_SA_MANAGER_H