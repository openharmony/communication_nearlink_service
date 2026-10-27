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

#include <string>
#include "iremote_broker.h"
#include "nearlink_base_sa_manager.h"

namespace OHOS {
namespace Nearlink {

/**
 * Keeps in sync with NearlinkBaseSaManager declared in nearlink_base_sa_manager.h.
 * The singleton holds a reference to NearlinkBaseSaManager in base.so and
 * forwards all calls to it.
 */
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
    NearlinkBaseSaManager &impl_;
};
} // namespace Nearlink
} // namespace OHOS
#endif // NEARLINK_SA_MANAGER_H