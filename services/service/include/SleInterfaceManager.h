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
#ifndef SLE_INTERFACE_MANAGER_H
#define SLE_INTERFACE_MANAGER_H

#include <memory>
#include "SleInterfaceAdapter.h"
#include "IAdapterStateObserver.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief System state define, using to OnSystemChange()...
 */
enum class SleSystemState : int {
    OFF = 0,
    ON,
};

/**
 * @brief Represents system state change observer during start/stop/factoryReset/reset.
 *
 * @since 6
 */
class ISystemStateObserver {
public:
    /**
     * @brief A destructor used to delete the <b>ISystemStateObserver</b> instance.
     *
     * @since 6
     */
    virtual ~ISystemStateObserver() = default;

    /**
     * @brief OnSystemStateChange state change function.
     *
     * @param transport Transport type when state change.
     * @param state Change to
     * @since 6
     */
    virtual void OnSystemStateChange(const SleSystemState state) = 0;
};

/**
 * @brief Represents interface adapter manager.
 *
 * @since 6
 */
class NEARLINK_API SleInterfaceManager {
public:
    /**
     * @brief A destructor used to delete the <b>SleInterfaceManager</b> instance.
     *
     * @since 6
     */
    virtual ~SleInterfaceManager() = default;

    /**
     * @brief Get adapter manager singleton instance pointer.
     *
     * @return Returns the singleton instance pointer.
     * @since 6
     */
    static SleInterfaceManager *GetInstance();

    static SwitchCallerInfo GetCallerInfo();

    /**
     * @brief Reset nearlink service.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual void Reset() const = 0;

    /**
     * @brief Start nearlink service.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool Start() = 0;

    /**
     * @brief Stop nearlink service.
     *
     * @since 6
     */
    virtual void Stop() const = 0;

    /**
     * @brief Factory reset nearlink service.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool FactoryReset() const = 0;

    /**
     * @brief Enable nearlink service.
     *
     * @param transport Enable sle.
     * @param reason The reason of sle enable.
     * @param callerInfo Informations of function caller.
     * @return Returns <b>NL_NO_ERROR</b> if the operation succeeded;
     *         returns <b>Other</b> if the operation is rejected.
     * @since 6
     */
    virtual int32_t Enable(const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo,
        const SleAutoConnectPolicy autoConnPolicy = SleAutoConnectPolicy::AUTO_CONN_GENERAL) const = 0;

    /**
     * @brief Disable nearlink service.
     *
     * @param transport Disable sle.
     * @param reason The reason of sle disable.
     * @param callerInfo Informations of function caller.
     * @return Returns <b>NL_NO_ERROR</b> if the operation succeeded;
     *         returns <b>Other</b> if the operation is rejected.
     * @since 6
     */
    virtual int32_t Disable(const SleTransport transport, const SleEventType reason,
        const SwitchCallerInfo callerInfo) const = 0;

    /**
     * @brief Disable nearlink state to STATE_TURN_OFF.
     *
     * @param transport SLE or SLB.
     * @param reason The reason of switching sle off.
     * @param callerInfo Informations of function caller.
     * @return Returns <b>NL_NO_ERROR</b> if the operation succeeded;
     *         returns <b>Other</b> if the operation is rejected.
     */
    virtual int32_t DisableToOff(const SleTransport transport, const SleEventType reason,
        const SwitchCallerInfo callerInfo) const = 0;

    /**
     * @brief Enable nearlink state to STATE_TURN_HALF.
     *
     * @param transport SLE or SLB.
     * @param reason The reason of switching sle half.
     * @param callerInfo Informations of function caller.
     * @return Returns <b>NL_NO_ERROR</b> if the operation succeeded;
     *         returns <b>Other</b> if the operation is rejected.
     */
    virtual int32_t EnableToHalf(const SleTransport transport, const SleEventType reason,
        const SwitchCallerInfo callerInfo) const = 0;

    /**
     * @brief Get adapter enable/disable state.
     *
     * @param transport Disable sle.
     * @return Returns adapter enable/disable state.
     * @since 6
     */
    virtual SleStateID GetState(const SleTransport transport) const = 0;

    /**
     * @brief Get adapter connects state.
     *
     * @return Returns adapter connects state.
     * @since 6
     */
    virtual SleConnectState GetAdapterConnectState() const = 0;

    /**
     * @brief Register adapter state observer.
     *
     * @param observer Class IAdapterStateObserver pointer to register observer.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool RegisterStateObserver(IAdapterStateObserver &observer) const = 0;

    /**
     * @brief Deregister adapter state observer.
     *
     * @param observer Class IAdapterStateObserver pointer to deregister observer.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool DeregisterStateObserver(IAdapterStateObserver &observer) const = 0;

    /**
     * @brief Register system state observer.
     *
     * @param observer Class ISystemStateObserver pointer to register observer.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool RegisterSystemStateObserver(ISystemStateObserver &observer) const = 0;

    /**
     * @brief Deregister system state observer.
     *
     * @param observer Class ISystemStateObserver pointer to deregister observer.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    virtual bool DeregisterSystemStateObserver(ISystemStateObserver &observer) const = 0;

    /**
     * @brief Get max audio connected devices number.
     *
     * @return Returns max device number that audio can connect.
     * @since 6
     */
    virtual int GetMaxNumConnectedAudioDevices() const = 0;

    /**
     * @brief Get sle adapter.
     *
     * @param transport classic or adapter.
     * @return Returns Basic adapter pointer.
     * @since 6
     */
    virtual SleInterfaceAdapter *GetAdapter(const SleTransport transport) const = 0;
};
}  // namespace Sle
}  // namespace OHOS

#endif  // SLE_INTERFACE_MANAGER_H