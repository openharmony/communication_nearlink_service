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
#ifndef ADAPTER_MANAGER_H
#define ADAPTER_MANAGER_H

#include <memory>
#include <string>
#include <vector>
#include "nearlink_def.h"
#include "SleInterfaceAdapter.h"
#include "SleInterfaceManager.h"
#include "context.h"
#include "nearlink_timer.h"
#include "SleSwitchDependency.h"
#if (defined(DEVICE_MANAGER))
#include "device_manager.h"
#endif
namespace OHOS {
namespace Nearlink {

/**
 * @brief Represents adapter manager.
 *
 * @since 6
 */
class SleServiceManager : public SleInterfaceManager {
public:
    /**
     * @brief Get adapter manager singleton instance pointer.
     *
     * @return Returns the singleton instance pointer.
     * @since 6
     */
    static SleServiceManager *GetInstance();

    // framework function
    /**
     * @brief Get adapter.
     *
     * @param transport Adapter transport.
     * @return Returns Basic adapter pointer.
     * @since 6
     */
    SleInterfaceAdapter *GetAdapter(const SleTransport transport) const override;

    /**
     * @brief nearlink adapter start.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool Start() override;

    /**
     * @brief Stop nearlink service.
     *
     * @since 6
     */
    void Stop() const override;

    /**
     * @brief Reset nearlink service.
     *
     * @since 6
     */
    void Reset() const override;

    /**
     * @brief Factory reset nearlink service.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool FactoryReset() const override;

    /**
     * @brief Enable nearlink service.
     *
     * @param transport Adapter transport.
     * @param reason The reason of nearlink enable.
     * @param callerInfo Informations of function caller.
     * @return Returns <b>NL_NO_ERROR</b> if the operation succeeded;
     *         returns <b>Other</b> if the operation is rejected.
     * @since 6
     */
    int32_t Enable(const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo,
        const SleAutoConnectPolicy autoConnPolicy = SleAutoConnectPolicy::AUTO_CONN_GENERAL) const override;

    /**
     * @brief Disable nearlink service, switch nearlink to disabled or half-disabled.
     *
     * @param transport Adapter transport.
     * @param reason The reason of nearlink disable.
     * @param callerInfo Informations of function caller.
     * @return Returns <b>NL_NO_ERROR</b> if the operation succeeded;
     *         returns <b>Other</b> if the operation is rejected.
     * @since 6
     */
    int32_t Disable(const SleTransport transport, const SleEventType reason,
        const SwitchCallerInfo callerInfo) const override;

    /**
     * @brief Switch nearlink state to disabled.
     *
     * @param transport Adapter transport.
     * @param reason The reason of disabling nearlink to off.
     * @param callerInfo Informations of function caller.
     * @return Returns <b>NL_NO_ERROR</b> if the operation succeeded;
     *         returns <b>Other</b> if the operation is rejected.
     */
    int32_t DisableToOff(const SleTransport transport, const SleEventType reason,
        const SwitchCallerInfo callerInfo) const override;

    /**
     * @brief Switch nearlink state to half-disabled.
     *
     * @param transport Adapter transport.
     * @param reason The reason of enabling nearlink to half.
     * @param callerInfo Informations of function caller.
     * @return Returns <b>NL_NO_ERROR</b> if the operation succeeded;
     *         returns <b>Other</b> if the operation is rejected.
     */
    int32_t EnableToHalf(const SleTransport transport, const SleEventType reason,
        const SwitchCallerInfo callerInfo) const override;

    /**
     * @brief Get adapter enable/disable state.
     *
     * @param transport Adapter transport.
     * @return Returns current state of nearlink.
     * @since 6
     */
    SleStateID GetState(const SleTransport transport) const override;

    /**
     * @brief Get adapter connects state.
     *
     * @return Returns adapter connects state.
     * @since 6
     */
    SleConnectState GetAdapterConnectState() const override;

    /**
     * @brief Register adapter state observer.
     *
     * @param observer Class IAdapterStateObserver pointer to register observer.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool RegisterStateObserver(IAdapterStateObserver &observer) const override;

    /**
     * @brief Deregister adapter state observer.
     *
     * @param observer Class IAdapterStateObserver pointer to deregister observer.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool DeregisterStateObserver(IAdapterStateObserver &observer) const override;

    /**
     * @brief Register system state observer.
     *
     * @param observer Class ISystemStateObserver pointer to register observer.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool RegisterSystemStateObserver(ISystemStateObserver &observer) const override;

    /**
     * @brief Deregister system state observer.
     *
     * @param observer Class ISystemStateObserver pointer to deregister observer.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool DeregisterSystemStateObserver(ISystemStateObserver &observer) const override;

    /**
     * @brief Get max audio connected devices number.
     *
     * @return Returns max device number that audio can connect.
     * @since 6
     */
    int GetMaxNumConnectedAudioDevices() const override;

    /**
     * @brief Stop nearlink adapter and profile service.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool AdapterStop() const;

    /**
     * @brief Clear all storage.
     *
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool ClearAllStorage() const;

    /**
     * @brief System state change.
     *
     * @param state Change to a new state.
     * @since 6
     */
    void OnSysStateChange(const std::string &state) const;

    /**
     * @brief System state exit.
     *
     * @param state Exit the old state.
     * @since 6
     */
    void OnSysStateExit(const std::string &state) const;

    /**
     * @brief Adapter state change.
     *
     * @param transport Adapter transport.
     * @param state Change to a new state.
     * @since 6
     */
    void OnAdapterStateChange(const SleTransport transport, const SleStateID state) const;

    /**
     * @brief Profile services enable complete notify.
     *
     * @param transport Adapter transport.
     * @param ret Profile services enable operation result.
     * @since 6
     */
    void OnProfileServicesEnableComplete(const SleTransport transport, const bool ret) const;

    /**
     * @brief Profile services disable complete notify.
     *
     * @param transport Adapter transport.
     * @param ret Profile services disable operation result.
     * @since 6
     */
    void OnProfileServicesDisableComplete(const SleTransport transport, const bool ret) const;

    /**
     * @brief Pair devices remove notify.
     *
     * @param transport Adapter transport.
     * @param devices The vector of removed devices.
     * @since 6
     */
    void OnPairDevicesRemoved(const SleTransport transport, const std::vector<RawAddress> &devices) const;

    /**
     * @brief Check nearlink switch_enable system parameter.
     *
     * @return Returns <b>true</b> if the parameter is on;
     *         returns <b>false</b> if the parameter is off.
     * @since 6
     */
    SleStateID GetNearlinkSwitchStateFromSystemParameter() const;

    /**
     * @brief Update nearlink switch_enable system parameter.
     *
     * @param transport Adapter transport.
     * @since 6
     */
    void UpdateNearlinkSwitchSystemParameter(const SleTransport transport, const SleStateID state) const;

    /**
     * @brief Check whether the given state is stable.
     *
     * @param sleState SleStateID.
     * @return Boolean value, true means the state is stable.
     */
    static bool IsStateStable(SleStateID sleState);

    /**
     * @brief Check whether nearlink is restricted.
     *        Nearlink switch can be operated only by particular callers in restricted state.
     *
     * @return Boolean value, true means nearlink is restricted.
     */
    bool IsSleSwitchRestricted() const;

    /**
     * @brief Chip reset notify.
     */
    void OnChipResetNotify() const;

    // 如需直接调用以下命名以Task结尾的函数，应确保函数运行在THREAD_ID_SVC_MGR线程
    int32_t EnableTask(
        const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo,
        const SleAutoConnectPolicy autoConnPolicy = SleAutoConnectPolicy::AUTO_CONN_GENERAL) const;
    int32_t DisableTask(
        const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo) const;
    int32_t SwitchToHalfTask(
        const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo) const;
    int32_t SwitchToOffTask(
        const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo) const;

    void OnAdapterStateChangeTask(const SleTransport transport, const SleStateID state) const;

    bool StartTask();

    bool IsEnableAutoConnectAudioDevices() const;
    bool IsEnableAutoConnectUserDisconnectedDevices() const;

    bool IsDisabling() const;

private:
    SleServiceManager();
    ~SleServiceManager();
    void CreateAdapters() const;
    bool IsNearlinkNeedTurnHalfFromCollaboration() const;
    bool IsEnableNeeded(const uint32_t tokenId, const int32_t uid) const;
    int32_t ProcessEnableDisallowed(const uint32_t tokenId, const int32_t uid,
        const SleTransport &transport, const SleEventType &reason) const;
    std::string GetSysState() const;
    bool OutputSetting() const;
    void RegisterDliResetCallback();
    void RegisterDriverLoadCallback();
    void DriverLoadStatusChange(const char *key, const char *value);
    void WaitDriverLoadCompleted() const;
    void DeregisterDliResetCallback() const;
    void RemoveDeviceProfileConfig(const SleTransport transport, const std::vector<RawAddress> &devices) const;
    void StartRestoreSwitchStatusTimer() const;
    void RestoreSwitchStatus() const;
    void RestoreSleFromCollaboration() const;
    int32_t RestoreSleFromSystemParameter(const SleTransport transport, const SleEventType reason) const;
    int32_t EnableInner(const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo,
        const SleAutoConnectPolicy autoConnPolicy = SleAutoConnectPolicy::AUTO_CONN_GENERAL) const;
#ifdef WATCH_STANDARD
    void AdapterStateChangeNotifyDataShare(const SleTransport transport, const SleStateID state) const;
#endif
#ifdef NEARLINK_HOST_DYNAMIC_RUNING
    bool LoadNearlinkHostDevice() const;
    void UnLoadNearlinkHostDevice() const;
#endif
    void CheckAndStopService() const;
    void SendDisableResponse(bool isHalfDisable) const;
    void ResetNearlinkService() const;
    void AdapterStateChangeNotify(const SleTransport transport, const SleStateID state) const;

    void WaitForAllSwitchDependency();
    void InitializeAfterAllDependencyOn();

    static void DliFailedReset();

    std::shared_ptr<SleSwitchDependency> switchDependency_ { nullptr };
    mutable std::atomic_bool isRestoreNeeded_ = false; // 星闪被特定调用方全关的情况下，后续是否需要恢复全关前的状态
    mutable SleAutoConnectPolicy autoConnPolicy_ = SleAutoConnectPolicy::AUTO_CONN_GENERAL;
    mutable SwitchCallerInfo disableCallerInfo_ {}; // 调用DisableNl接口的调用方信息，仅在THREAD_ID_SVC_MGR线程使用
    SLE_DISALLOW_COPY_AND_ASSIGN(SleServiceManager);
    DECLARE_IMPL();
};
#if (defined(DEVICE_MANAGER))
class DmInitCallback : public OHOS::DistributedHardware::DmInitCallback {
public:
    DmInitCallback() : OHOS::DistributedHardware::DmInitCallback() {}
    virtual ~DmInitCallback() {}
    void OnRemoteDied() override {}
};
#endif
}  // namespace Nearlink
}  // namespace OHOS

#endif  // ADAPTER_MANAGER_H