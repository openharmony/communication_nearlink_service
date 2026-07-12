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

#include "nearlink_host.h"

#include <mutex>
#include "nearlink_sa_manager.h"
#include "nearlink_safe_weak_list.h"
#include "log_util.h"
#include "i_nearlink_host.h"
#include "nearlink_errorcode.h"
#include "nearlink_host_load_callback.h"
#include "nearlink_host_observer_stub.h"
#include "nearlink_sle_peripheral_observer_stub.h"
#include "nearlink_device_battery_observer_stub.h"
#include "nearlink_device_rssi_observer_stub.h"
#include "nearlink_switch_module.h"
#include "nearlink_utils.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "parameters.h"
#ifdef NEARLINK_PLUGGABLE_SUPPORTED
#include "param_wrapper.h"
#endif
#ifdef NEARLINK_HOST_AVOID_SLEEP
#include "power_mgr_client.h"
#include "running_lock.h"
#endif

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int32_t LOAD_NEARLINK_SA_TIMEOUT_MS = 20000;

#ifdef NEARLINK_HOST_AVOID_SLEEP
    const uint16_t WAKE_TIME = 3000; //3s
#endif
    constexpr int32_t NEARLINK_UNKNOWN = -1;
    constexpr int32_t NEARLINK_NOT_SUPPORTED = 0;
    constexpr int32_t NEARLINK_SUPPORTED = 1;
    const char *NEARLINK_HOST = "NearlinkHost";
#ifdef NEARLINK_PLUGGABLE_SUPPORTED
    const char* NEARLINK_PLUGGABLE_STATE = "persist.bluetooth.pluggable.state";
    const char* NEARLINK_PLUGGABLE_STATE_EMPLACE = "1";
#endif
}

struct NearlinkHost::impl : public std::enable_shared_from_this<impl> {
    impl();
    ~impl();

    void Init();
    bool LoadNearlinkHostService(void);
    void LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject);
    void LoadSystemAbilityFail();

    // host observer,  registered to the service process.
    class NearlinkHostObserverImp;
    sptr<NearlinkHostObserverImp> hostObserverImp_ = nullptr;

    // remote observer,  registered to the service process.
    class NearlinkSlePeripheralCallbackImp;
    sptr<NearlinkSlePeripheralCallbackImp> remoteObserverImp_ = nullptr;

    // remote observer,  registered to the service process.
    class NearlinkDeviceBatteryObserverImp;
    sptr<NearlinkDeviceBatteryObserverImp> deviceBatteryObserverImp_ = nullptr;

    class NearlinkDeviceRssiObserverImp;
    sptr<NearlinkDeviceRssiObserverImp> deviceRssiObserverImp_ = nullptr;

    // Stores the observers registered by the application.
    NearlinkSafeWeakList<NearlinkHostObserver> hostObserverList;

    // Stores the remote device observers registered by the application.
    NearlinkSafeWeakList<NearlinkRemoteDeviceObserver> remoteObserverList;

    // Stores the remote device battery observers registered by the application.
    NearlinkSafeWeakList<NearlinkRemoteDeviceBatteryObserver> remoteBatteryObserverList;

    // Stores the remote device rssi observers registered by the application.
    NearlinkSafeWeakList<NearlinkRemoteDeviceRssiObserver> rssiObserverList;

    class NearlinkSwitchAction;
    std::shared_ptr<NearlinkSwitchModule> switchModule_ { nullptr };

    void SyncRandomAddrToService(void);
#ifdef NEARLINK_HOST_AVOID_SLEEP
    std::shared_ptr<OHOS::PowerMgr::RunningLock> AcquireWakeLock(void);
#endif

    std::mutex loadServiceMutex_;
    std::mutex isNearlinkEnableMutex_;
    std::mutex isNearlinkSupportFrame4Mutex_;
    std::mutex isNearlinkAudioEnableMutex_;
    std::string stagingRealAddr_;
    std::string stagingRandomAddr_;
    int32_t profileRegisterId_{0};
    int isNearlinkEnable_ = NEARLINK_UNKNOWN;
    int32_t isNearlinkSupportFrame4_ = NEARLINK_UNKNOWN;
    int32_t isNearlinkAudioEnable_ = NEARLINK_UNKNOWN;

private:
    std::condition_variable proxyConVar_;
    std::weak_ptr<NearlinkHost::impl> slefHostImplWeak_;
};

class NearlinkHost::impl::NearlinkHostObserverImp : public NearlinkHostObserverStub {
public:
    explicit NearlinkHostObserverImp(std::weak_ptr<NearlinkHost::impl> hostImpl) : hostImpl_(hostImpl){};
    ~NearlinkHostObserverImp() override{};

    void OnStateChanged(int32_t transport, int32_t status) override
    {
        HILOGD("nearlink state, transport: %{public}s, status: %{public}s",
            GetTransportString(transport).c_str(), GetStateString(status).c_str());
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        if (status == SleStateID::STATE_TURN_ON) {
            hostImpl->SyncRandomAddrToService();
        }
        NearlinkSaManager::GetInstance().OnStateChanged(transport, status);
        hostImpl->hostObserverList.IterateAsync([transport, status](
            std::shared_ptr<NearlinkHostObserver> observer) -> void {
            observer->OnStateChanged(transport, status);
        });
    }

    void OnFullStateChanged(int32_t transport, int32_t status) override
    {
        HILOGD("nearlink state, transport: %{public}s, status: %{public}s",
            GetTransportString(transport).c_str(), GetStateString(status).c_str());
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        hostImpl->hostObserverList.IterateAsync([transport, status](
            std::shared_ptr<NearlinkHostObserver> observer) -> void {
            observer->OnFullStateChanged(transport, status);
        });
    }

    void OnSwitchStateChanged(int32_t status) override
    {
        HILOGD("nearlink status: %{public}s", GetStateString(status).c_str());
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        if (status == SleStateID::STATE_TURN_ON) {
            hostImpl->switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_ON);
        }
        if (status == SleStateID::STATE_TURN_OFF) {
            hostImpl->switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_OFF);
        }
        if (status == SleStateID::STATE_TURN_HALF) {
            hostImpl->switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_HALF);
        }
    }

    void OnDisableResponse(bool isHalfDisable) override
    {
        HILOGI("isHalfDisable: %{public}d", isHalfDisable);
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        if (isHalfDisable) {
            hostImpl->switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_TO_HALF_RESPONSE);
        } else {
            hostImpl->switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_TO_OFF_RESPONSE);
        }
    }

    void OnPairConfirmed(const int32_t transport, const NearlinkRawAddress &device, int reqType, int number) override
    {
        HILOGI("enter, transport: %{public}d, device: %{public}s, reqType: %{public}d, number: %{public}d",
            transport, GetEncryptAddr((device).GetAddress()).c_str(), reqType, number);
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        NearlinkRemoteDevice remoteDevice(device.GetAddress(), transport);
        hostImpl->hostObserverList.IterateAsync(
            [remoteDevice, reqType, number](std::shared_ptr<NearlinkHostObserver> observer) -> void {
            observer->OnPairConfirmed(remoteDevice, reqType, number);
        });
    }

    void OnDeviceNameChanged(const std::string &deviceName) override
    {
        HILOGI("enter, deviceName length: %{public}lu", deviceName.length());
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        hostImpl->hostObserverList.IterateAsync([deviceName](std::shared_ptr<NearlinkHostObserver> observer) -> void {
            observer->OnDeviceNameChanged(deviceName);
        });
    }

    void OnDeviceAddrChanged(const std::string &address) override
    {
        HILOGD("enter");
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        hostImpl->hostObserverList.IterateAsync([address](std::shared_ptr<NearlinkHostObserver> observer) -> void {
            observer->OnDeviceAddrChanged(address);
        });
    }

private:
    std::weak_ptr<NearlinkHost::impl> hostImpl_;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkHostObserverImp);
};

class NearlinkHost::impl::NearlinkSlePeripheralCallbackImp : public NearlinkSlePeripheralObserverStub {
public:
    explicit NearlinkSlePeripheralCallbackImp(std::weak_ptr<NearlinkHost::impl> hostImpl) : hostImpl_(hostImpl){};
    ~NearlinkSlePeripheralCallbackImp() override = default;

    void OnAcbStateChanged(const NearlinkRawAddress &device, int state, int reason) override
    {
        HILOGD("enter, device=%{public}s, state=%{public}d, reason=%{public}u",
            GetEncryptAddr((device).GetAddress()).c_str(), state, reason);
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        NearlinkRemoteDevice remoteDevice(device.GetAddress(), static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        hostImpl->remoteObserverList.IterateAsync(
            [remoteDevice, state, reason](std::shared_ptr<NearlinkRemoteDeviceObserver> observer) -> void {
                observer->OnAcbStateChanged(remoteDevice, state, reason);
        });
    }

    void OnPairingRequest(const NearlinkRawAddress &device, const std::string &passkey, int type) override
    {
        HILOG_COMM_INFO("[%{public}s:%{public}d]enter, device=%{public}s, type=%{public}d",
            __FUNCTION__, __LINE__, GetEncryptAddr((device).GetAddress()).c_str(), type);
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        NearlinkRemoteDevice remoteDevice(device.GetAddress(), static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        hostImpl->remoteObserverList.IterateAsync(
            [remoteDevice, passkey, type](std::shared_ptr<NearlinkRemoteDeviceObserver> observer) -> void {
            observer->OnPairingRequest(remoteDevice, passkey, type);
        });
    }

    void OnConnectionStateChanged(const NearlinkRawAddress &device, int preState, int state, int reason) override
    {
        HILOGD("enter, device=%{public}s, state=%{public}d, reason=%{public}u",
            GetEncryptAddr((device).GetAddress()).c_str(), state, reason);
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        NearlinkRemoteDevice remoteDevice(device.GetAddress(), static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        hostImpl->remoteObserverList.IterateAsync(
            [remoteDevice, preState, state, reason](std::shared_ptr<NearlinkRemoteDeviceObserver> observer) -> void {
                observer->OnConnectionStateChanged(remoteDevice, preState, state, reason);
        });
    }

    void OnPairStatusChanged(const NearlinkRawAddress &device, int preState, int state, int reason) override
    {
        HILOGI("enter, device=%{public}s, state=%{public}d, reason=%{public}u",
            GetEncryptAddr((device).GetAddress()).c_str(), state, reason);
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        NearlinkRemoteDevice remoteDevice(device.GetAddress(), static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        hostImpl->remoteObserverList.IterateAsync(
            [remoteDevice, preState, state, reason](std::shared_ptr<NearlinkRemoteDeviceObserver> observer) -> void {
                observer->OnPairStatusChanged(remoteDevice, preState, state, reason);
        });
    }

    void OnReadRemoteRssiEvent(const NearlinkRawAddress &device, int rssi, int status) override
    {
        HILOGI("enter, device=%{public}s, rssi=%{public}d, status=%{public}d",
            GetEncryptAddr((device).GetAddress()).c_str(), rssi, status);
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        NearlinkRemoteDevice remoteDevice(device.GetAddress(), static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        hostImpl->remoteObserverList.IterateAsync(
            [remoteDevice, rssi, status](std::shared_ptr<NearlinkRemoteDeviceObserver> observer) -> void {
                observer->OnReadRemoteRssiEvent(remoteDevice, rssi, status);
        });
    }

    void OnLinkFreqBandChanged(const NearlinkRawAddress &device, int32_t freqBand) override
    {
        HILOGI("enter, device: %{public}s, reqType: %{public}d", GET_ENCRYPT_ADDR(device), freqBand);
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        NearlinkRemoteDevice remoteDevice(device.GetAddress(), static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        hostImpl->remoteObserverList.IterateAsync(
            [remoteDevice, freqBand](std::shared_ptr<NearlinkRemoteDeviceObserver> observer) -> void {
                observer->OnLinkFreqBandChanged(remoteDevice, freqBand);
        });
    }

private:
    std::weak_ptr<NearlinkHost::impl> hostImpl_;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkSlePeripheralCallbackImp);
};

class NearlinkHost::impl::NearlinkDeviceBatteryObserverImp : public NearlinkDeviceBatteryObserverStub {
public:
    explicit NearlinkDeviceBatteryObserverImp(std::weak_ptr<NearlinkHost::impl> hostImpl) : hostImpl_(hostImpl){};
    ~NearlinkDeviceBatteryObserverImp() override{};

    void OnGetBatteryLevelEvent(const NearlinkRawAddress &device, int32_t batteryLevel) override
    {
        HILOGI("enter, device: %{public}s, batterylevel: %{public}d", GET_ENCRYPT_ADDR(device), batteryLevel);
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        NearlinkRemoteDevice remoteDevice(device.GetAddress(), static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        auto func = [remoteDevice, batteryLevel](std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer) {
            observer->OnGetBatteryLevelEvent(remoteDevice, batteryLevel);
        };
        hostImpl->remoteBatteryObserverList.IterateAsync(func);
    }

    void OnBatteryLevelChanged(const NearlinkRawAddress &device, int32_t batteryLevel) override
    {
        HILOGI("enter, device: %{public}s, batterylevel: %{public}d", GET_ENCRYPT_ADDR(device), batteryLevel);
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        NearlinkRemoteDevice remoteDevice(device.GetAddress(), static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        hostImpl->remoteBatteryObserverList.IterateAsync(
            [remoteDevice, batteryLevel](std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer) {
                observer->OnBatteryLevelChanged(remoteDevice, batteryLevel);
            });
    }

private:
    std::weak_ptr<NearlinkHost::impl> hostImpl_;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkDeviceBatteryObserverImp);
};

class NearlinkHost::impl::NearlinkDeviceRssiObserverImp : public NearlinkDeviceRssiObserverStub {
public:
    explicit NearlinkDeviceRssiObserverImp(std::weak_ptr<NearlinkHost::impl> hostImpl) : hostImpl_(hostImpl){};
    ~NearlinkDeviceRssiObserverImp() override{};

    void OnReadRemoteRssiEvent(const NearlinkRawAddress &device, int rssi, int status) override
    {
        HILOGI("frameworkenter, device: %{public}s, rssi: %{public}d", GET_ENCRYPT_ADDR(device), rssi);
        auto hostImpl = hostImpl_.lock();
        NL_CHECK_RETURN(hostImpl, "hostImpl has been destroyed");
        NearlinkRemoteDevice remoteDevice(device.GetAddress(), static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
        auto func = [remoteDevice, rssi, status](std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer) {
            observer->OnReadRemoteRssiEvent(remoteDevice, rssi, status);
        };
        hostImpl->rssiObserverList.IterateAsync(func);
    }

private:
    std::weak_ptr<NearlinkHost::impl> hostImpl_;
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkDeviceRssiObserverImp);
};

class NearlinkHost::impl::NearlinkSwitchAction : public INearlinkSwitchAction {
public:
    NearlinkSwitchAction() = default;
    ~NearlinkSwitchAction() override = default;

    NlErrCode EnableNearlink(SleAutoConnectPolicy autoConnPolicy) override
    {
        NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().pimpl->LoadNearlinkHostService(),
            NL_ERR_INTERNAL_ERROR, "load nearlink service failed.");
        sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
        NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
        return proxy->EnableSle(autoConnPolicy);
    }

    NlErrCode DisableNearlink() override
    {
        sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
        NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
        return proxy->DisableSle();
    }

    NlErrCode DisableNearlinkToOff() override
    {
        sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
        NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
        return proxy->DisableSleToOff();
    }

    NlErrCode EnableNearlinkToHalf() override
    {
        NL_CHECK_RETURN_RET(NearlinkHost::GetInstance().pimpl->LoadNearlinkHostService(),
            NL_ERR_INTERNAL_ERROR, "load nearlink service failed.");
        sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
        NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
        return proxy->EnableSleToHalf();
    }
};

NearlinkHost::impl::impl()
{
    HILOGI("start");
}

NearlinkHost::impl::~impl()
{
    HILOGI("starts");
    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId_);
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN(proxy, "proxy is nullptr");
    NL_CHECK_RETURN(hostObserverImp_, "hostObserverImp_ is nullptr");
    proxy->DeregisterSleAdapterObserver(hostObserverImp_);
    NL_CHECK_RETURN(remoteObserverImp_, "remoteObserverImp_ is nullptr");
    proxy->DeregisterSlePeripheralCallback(remoteObserverImp_);
    NL_CHECK_RETURN(deviceBatteryObserverImp_, "deviceBatteryObserverImp_ is nullptr");
    proxy->DeregisterDeviceBatteryObserver(deviceBatteryObserverImp_);
    NL_CHECK_RETURN(deviceRssiObserverImp_, "deviceRssiObserverImp_ is nullptr");
    proxy->DeregisterDeviceRssiObserver(deviceRssiObserverImp_);
}

void NearlinkHost::impl::Init()
{
    HILOGI("starts");
    slefHostImplWeak_ = shared_from_this();
    hostObserverImp_ = new (std::nothrow) NearlinkHostObserverImp(slefHostImplWeak_);
    remoteObserverImp_ = new (std::nothrow) NearlinkSlePeripheralCallbackImp(slefHostImplWeak_);
    deviceBatteryObserverImp_ = new (std::nothrow) NearlinkDeviceBatteryObserverImp(slefHostImplWeak_);
    deviceRssiObserverImp_ = new (std::nothrow) NearlinkDeviceRssiObserverImp(slefHostImplWeak_);

    auto switchActionPtr = std::make_unique<NearlinkSwitchAction>();
    switchModule_ = std::make_shared<NearlinkSwitchModule>(std::move(switchActionPtr));

    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(NEARLINK_HOST);
    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        sptr<INearlinkHost> proxy = iface_cast<INearlinkHost>(remote);
        NL_CHECK_RETURN(proxy, "proxy is nullptr");
        NL_CHECK_RETURN(hostObserverImp_, "hostObserverImp_ is nullptr");
        proxy->RegisterSleAdapterObserver(hostObserverImp_);
        NL_CHECK_RETURN(remoteObserverImp_, "remoteObserverImp_ is nullptr");
        proxy->RegisterSlePeripheralCallback(remoteObserverImp_);
        NL_CHECK_RETURN(deviceBatteryObserverImp_, "deviceBatteryObserverImp_ is nullptr");
        proxy->RegisterDeviceBatteryObserver(deviceBatteryObserverImp_);
        NL_CHECK_RETURN(deviceRssiObserverImp_, "deviceRssiObserverImp_ is nullptr");
        proxy->RegisterDeviceRssiObserver(deviceRssiObserverImp_);
        bool isSleEnabled = false;
        NlErrCode ret = proxy->IsSleEnabled(isSleEnabled);
        NL_CHECK_RETURN(ret == NL_NO_ERROR, "IsSleEnabled failed, error code: %{public}d", ret);
        if (isSleEnabled) {
            HILOGW("execute serviceStartedFunc_, sle is enabled, maybe app is freezed before.");
            hostObserverList.IterateAsync([](std::shared_ptr<NearlinkHostObserver> observer) -> void {
                observer->OnStateChanged(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_ON);
                observer->OnFullStateChanged(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_ON);
            });
        }
    };

    info->serviceStoppedFunc_ = [this]() -> void {
        hostObserverList.IterateAsync([](std::shared_ptr<NearlinkHostObserver> observer) -> void {
            observer->OnStateChanged(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_OFF);
            observer->OnFullStateChanged(SleTransport::ADAPTER_SLE, SleStateID::STATE_TURN_OFF);
        });

        NL_CHECK_RETURN(switchModule_ != nullptr, "switchModule is nullptr");
        switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_OFF);
    };

    profileRegisterId_ = NearlinkSaManager::GetInstance().RegisterFunc(info);
    if (profileRegisterId_ == INVALID_PROFILE_ID) {
        HILOGE("profileRegisterId_ is invalid");
    }
}

bool NearlinkHost::impl::LoadNearlinkHostService()
{
    std::unique_lock<std::mutex> lock(loadServiceMutex_);
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    NL_CHECK_RETURN_RET(samgrProxy, false, "samgrProxy is nullptr");

    sptr<IRemoteObject> hostRemote = NearlinkSaManager::GetInstance().GetRemoteProfile(NEARLINK_HOST);
    if (hostRemote != nullptr) {
        return true;
    }

    sptr<NearlinkHostLoadCallBack> loadCallback = new (std::nothrow) NearlinkHostLoadCallBack();
    if (loadCallback == nullptr) {
        HILOGE("loadCallback is nullptr.");
        return false;
    }
    int32_t ret = samgrProxy->LoadSystemAbility(NEARLINK_HOST_SYS_ABILITY_ID, loadCallback);
    if (ret != ERR_OK) {
        HILOGE("Failed to load nearlink systemAbility");
        return false;
    }
    auto waitStatus = proxyConVar_.wait_for(
        lock, std::chrono::milliseconds(LOAD_NEARLINK_SA_TIMEOUT_MS), []() -> bool {
            sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
            return proxy != nullptr;
        });
    if (!waitStatus) {
        HILOGE("load nearlink systemAbility timeout");
        return false;
    }
    return true;
}

NlErrCode NearlinkHost::GetSleMaxAdvertisingDataLength(uint32_t &maxAdvDataLen) const
{
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->GetSleMaxAdvertisingDataLength(maxAdvDataLen);
}

void NearlinkHost::impl::LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject)
{
    HILOGI("LoadSystemAbilitySuccess FinishStart SA");
    proxyConVar_.notify_one();
}

void NearlinkHost::impl::LoadSystemAbilityFail()
{
    HILOGI("LoadSystemAbilityFail FinishStart SA");
    proxyConVar_.notify_one();
}

#ifdef NEARLINK_HOST_AVOID_SLEEP
std::shared_ptr<OHOS::PowerMgr::RunningLock> NearlinkHost::impl::AcquireWakeLock(void)
{
    auto runningLock = PowerMgr::PowerMgrClient::GetInstance().CreateRunningLock(
        "NearlinkEnableWakeLock", PowerMgr::RunningLockType::RUNNINGLOCK_BACKGROUND_TASK);
    if (runningLock != nullptr) {
        runningLock->Lock(WAKE_TIME);
        HILOGI("AcquireWakeLock sle timeOutMs: %{public}d", WAKE_TIME);
    } else {
        HILOGE("AcquireWakeLock sle Create runningLock failed.");
    }
    return runningLock;
}
#endif

void NearlinkHost::impl::SyncRandomAddrToService(void)
{
    HILOGD("SyncRandomAddrToService.");
    if (!IsValidAddress(stagingRealAddr_)) {
        HILOGE("stagingRealAddr_ is invalid.");
        return;
    }
    if (!IsValidAddress(stagingRandomAddr_)) {
        HILOGE("stagingRandomAddr_ is invalid.");
        return;
    }
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN(proxy, "proxy is nullptr");
    stagingRealAddr_ = "";
    stagingRandomAddr_ = "";
}

NearlinkHost::NearlinkHost()
{
    pimpl = std::make_shared<impl>();
    pimpl->Init();
}

NearlinkHost::~NearlinkHost() {}

NearlinkHost &NearlinkHost::GetInstance()
{
    // C++11 static local variable initialization is thread-safe.
    static NearlinkHost host;
    return host;
}

NlErrCode NearlinkHost::RegisterObserver(std::shared_ptr<NearlinkHostObserver> observer)
{
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl != nullptr, NL_ERR_IMPL_ERROR, "piml is nullptr");
    pimpl->hostObserverList.Insert(observer);
    return NL_NO_ERROR;
}

NlErrCode NearlinkHost::DeregisterObserver(std::shared_ptr<NearlinkHostObserver> observer)
{
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl != nullptr, NL_ERR_IMPL_ERROR, "piml is nullptr");
    pimpl->hostObserverList.Erase(observer);
    return NL_NO_ERROR;
}

NlErrCode NearlinkHost::LoadNearlinkSa()
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl != nullptr, NL_ERR_INTERNAL_ERROR, "pimpl is nullptr.");
    NL_CHECK_RETURN_RET(pimpl->LoadNearlinkHostService(), NL_ERR_INTERNAL_ERROR,
        "load nearlink service failed.");
    return NL_NO_ERROR;
}

NlErrCode NearlinkHost::EnableNl(const SleAutoConnectPolicy autoConnPolicy)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl != nullptr, NL_ERR_INTERNAL_ERROR, "pimpl is nullptr.");
    NL_CHECK_RETURN_RET(pimpl->switchModule_, NL_ERR_INTERNAL_ERROR, "switchModule is nullptr");
#ifdef NEARLINK_HOST_AVOID_SLEEP
    auto runningLock = pimpl->AcquireWakeLock();
#endif
    return pimpl->switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK, autoConnPolicy);
}

NlErrCode NearlinkHost::DisableNl()
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl != nullptr, NL_ERR_INTERNAL_ERROR, "pimpl is nullptr.");
    NL_CHECK_RETURN_RET(pimpl->switchModule_, NL_ERR_INTERNAL_ERROR, "switchModule is nullptr");
    return pimpl->switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK);
}

NlErrCode NearlinkHost::DisableNlToOff()
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl != nullptr, NL_ERR_INTERNAL_ERROR, "pimpl is nullptr.");
    NL_CHECK_RETURN_RET(pimpl->switchModule_, NL_ERR_INTERNAL_ERROR, "switchModule is nullptr");
    return pimpl->switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF);
}

NlErrCode NearlinkHost::EnableNlToHalf()
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl != nullptr, NL_ERR_INTERNAL_ERROR, "pimpl is nullptr.");
    NL_CHECK_RETURN_RET(pimpl->switchModule_, NL_ERR_INTERNAL_ERROR, "switchModule is nullptr");
    return pimpl->switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF);
}

SleStateID NearlinkHost::GetSleFullState()
{
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), SleStateID::STATE_TURN_OFF, "nearlink is not support.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, SleStateID::STATE_TURN_OFF, "proxy is nullptr");
    int sleCurrentState = static_cast<int>(SleStateID::STATE_TURN_OFF);
    NlErrCode ret = proxy->GetSleFullState(sleCurrentState);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, SleStateID::STATE_TURN_OFF,
        "GetSleFullState failed, error code: %{public}d", ret);
    return static_cast<SleStateID>(sleCurrentState);
}

bool NearlinkHost::IsSleEnabled() const
{
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, false, "proxy is nullptr");
    bool isSleEnabled = false;
    NlErrCode ret = proxy->IsSleEnabled(isSleEnabled);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false, "IsSleEnabled failed, error code: %{public}d", ret);
    return isSleEnabled;
}

bool NearlinkHost::IsSleHalfDisabled() const
{
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, false, "proxy is nullptr");
    bool isSleHalfDisabled = false;
    NlErrCode ret = proxy->IsSleHalfDisabled(isSleHalfDisabled);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false, "IsSleHalfDisabled failed, error code: %{public}d", ret);
    return isSleHalfDisabled;
}

bool NearlinkHost::IsSleDisabled() const
{
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, true, "proxy is nullptr");
    bool isSleDisabled = true;
    NlErrCode ret = proxy->IsSleDisabled(isSleDisabled);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, true, "IsSleDisabled failed, error code: %{public}d", ret);
    return isSleDisabled;
}

bool NearlinkHost::IsSleAvailableToCaller() const
{
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, false, "proxy is nullptr");
    bool isSleAvailable = false;
    NlErrCode ret = proxy->IsSleAvailableToCaller(isSleAvailable);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false, "IsSleAvailableToCaller failed, error code: %{public}d", ret);
    return isSleAvailable;
}

bool NearlinkHost::IsNearlinkSupport() const
{
    NL_CHECK_RETURN_RET(pimpl != nullptr, false, "piml is nullptr");
    std::unique_lock<std::mutex> lock(pimpl->isNearlinkEnableMutex_);
    if (pimpl->isNearlinkEnable_ == NEARLINK_UNKNOWN) {
        pimpl->isNearlinkEnable_ = OHOS::system::GetBoolParameter("const.nearlink.enable", false) ?
            NEARLINK_SUPPORTED : NEARLINK_NOT_SUPPORTED;
    }
    NL_CHECK_RETURN_RET(pimpl->isNearlinkEnable_ == NEARLINK_SUPPORTED, false,
                        "IsNearlinkSupport not enable");
    bool isSupported = true;
#ifdef NEARLINK_PLUGGABLE_SUPPORTED
    std::string state = NEARLINK_PLUGGABLE_STATE_EMPLACE;
    OHOS::system::GetStringParameter(NEARLINK_PLUGGABLE_STATE, state, NEARLINK_PLUGGABLE_STATE_EMPLACE);
    isSupported = state == NEARLINK_PLUGGABLE_STATE_EMPLACE;
#endif
    return isSupported;
}

bool NearlinkHost::IsNearlinkAudioSupport() const
{
    NL_CHECK_RETURN_RET(pimpl != nullptr, false, "piml is nullptr");
    std::unique_lock<std::mutex> lock(pimpl->isNearlinkAudioEnableMutex_);
    if (pimpl->isNearlinkAudioEnable_ == NEARLINK_UNKNOWN) {
        pimpl->isNearlinkAudioEnable_ = OHOS::system::GetIntParameter("const.nearlink.audio", 0) ?
            NEARLINK_SUPPORTED : NEARLINK_NOT_SUPPORTED;
    }
    return pimpl->isNearlinkAudioEnable_ == NEARLINK_SUPPORTED;
}

NlErrCode NearlinkHost::GetAdapterConnectState(int32_t &state) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->GetAdapterConnectState(state);
}

NlErrCode NearlinkHost::GetProfileConnState(const std::string &remoteAddr, int32_t &state) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(IsValidAddress(remoteAddr), NL_ERR_INTERNAL_ERROR, "Invalid sle remote device");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->GetProfileConnState(remoteAddr, state);
}

NlErrCode NearlinkHost::GetLocalName(std::string &localDeviceName) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->GetLocalName(localDeviceName);
}

NlErrCode NearlinkHost::SetLocalName(const std::string &name)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->SetLocalName(name);
}

NlErrCode NearlinkHost::GetLocalAddress(std::string &localDeviceAddres) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");

    return proxy->GetLocalAddress(localDeviceAddres);
}

NlErrCode NearlinkHost::GetPairedDevices(int transport, std::vector<NearlinkRemoteDevice> &pairedDevices) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    std::vector<NearlinkRawAddress> pairedAddr;
    NlErrCode ret = proxy->GetPairedDevices(pairedAddr);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, ret, "get paired device failed, ret=%{public}d", ret);

    for (auto it = pairedAddr.begin(); it != pairedAddr.end(); it++) {
        NearlinkRemoteDevice device((*it).GetAddress(), transport);
        pairedDevices.emplace_back(device);
    }
    return NL_NO_ERROR;
}

NlErrCode NearlinkHost::SetConnectionMode(int32_t connectionMode, int32_t duration)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    NL_CHECK_RETURN_RET(connectionMode >= static_cast<int>(SLEConnectionMode::CONNECTION_MODE_UNCONNECTABLE) &&
                        connectionMode <= static_cast<int>(SLEConnectionMode::CONNECTION_MODE_CONNECTABLE),
                        NL_ERR_INVALID_PARAM, "connection mode is invalid");
    NL_CHECK_RETURN_RET(duration >= 0, NL_ERR_INVALID_PARAM, "duration is invalid");
    return proxy->SetConnectionMode(connectionMode, duration);
}

NlErrCode NearlinkHost::RemovePair(const NearlinkRemoteDevice &device)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), NL_ERR_INVALID_STATE, "Invalid remote device.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");

    sptr<NearlinkRawAddress> rawAddr = new (std::nothrow) NearlinkRawAddress(device.GetDeviceAddr());
    NL_CHECK_RETURN_RET(rawAddr, NL_ERR_INVALID_STATE, "rawAddr is nullptr");

    return proxy->RemovePair(rawAddr);
}

NlErrCode NearlinkHost::RemoveAllPairs()
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->RemoveAllPairs();
}

NlErrCode NearlinkHost::GetLinkRole(const NearlinkRemoteDevice &device, uint8_t &role) const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(device.IsValidNearlinkRemoteDevice(), NL_ERR_INVALID_PARAM, "Invalid remote device");
    NL_CHECK_RETURN_RET(IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->GetLinkRole(device.GetTransportType(), device.GetDeviceAddr(), role);
}

NlErrCode NearlinkHost::RegisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl, NL_ERR_IMPL_ERROR, "pimpl is nullptr");
    pimpl->remoteObserverList.Insert(observer);
    return NL_NO_ERROR;
}

NlErrCode NearlinkHost::DeregisterRemoteDeviceObserver(std::shared_ptr<NearlinkRemoteDeviceObserver> observer)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl, NL_ERR_IMPL_ERROR, "pimpl is nullptr");
    pimpl->remoteObserverList.Erase(observer);
    return NL_NO_ERROR;
}

NlErrCode NearlinkHost::RegisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
{
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");

    NL_CHECK_RETURN_RET(IsBasSupported(), NL_ERR_FEATURE_NOT_SUPPORT, "Battery service is not supported.");
    NL_CHECK_RETURN_RET(pimpl, NL_ERR_IMPL_ERROR, "pimpl is nullptr");
    pimpl->remoteBatteryObserverList.Insert(observer);
    
    return NL_NO_ERROR;
}

NlErrCode NearlinkHost::DeregisterBatteryObserver(std::shared_ptr<NearlinkRemoteDeviceBatteryObserver> observer)
{
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsBasSupported(), NL_ERR_FEATURE_NOT_SUPPORT, "Battery service is not supported.");
    NL_CHECK_RETURN_RET(pimpl, NL_ERR_IMPL_ERROR, "pimpl is nullptr");
    pimpl->remoteBatteryObserverList.Erase(observer);
    return NL_NO_ERROR;
}

NlErrCode NearlinkHost::RegisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
{
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl, NL_ERR_IMPL_ERROR, "pimpl is nullptr");
    pimpl->rssiObserverList.Insert(observer);

    return NL_NO_ERROR;
}

NlErrCode NearlinkHost::DeregisterRssiObserver(std::shared_ptr<NearlinkRemoteDeviceRssiObserver> observer)
{
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl, NL_ERR_IMPL_ERROR, "pimpl is nullptr");
    pimpl->rssiObserverList.Erase(observer);
    return NL_NO_ERROR;
}

void NearlinkHost::LoadSystemAbilitySuccess(const sptr<IRemoteObject> &remoteObject)
{
    NL_CHECK_RETURN(pimpl, "pimpl is nullptr");
    pimpl->LoadSystemAbilitySuccess(remoteObject);
}

void NearlinkHost::LoadSystemAbilityFail()
{
    NL_CHECK_RETURN(pimpl, "pimpl is nullptr");
    pimpl->LoadSystemAbilityFail();
}

NlErrCode NearlinkHost::ConnectAllowedProfiles(const std::string &remoteAddr) const
{
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(IsValidAddress(remoteAddr), NL_ERR_INTERNAL_ERROR, "Invalid sle remote device addr");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->ConnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHost::DisconnectAllowedProfiles(const std::string &remoteAddr) const
{
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(IsValidAddress(remoteAddr), NL_ERR_INTERNAL_ERROR, "Invalid sle remote device addr");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->DisconnectAllowedProfiles(remoteAddr);
}

NlErrCode NearlinkHost::NearlinkFactoryReset()
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    if (!IS_SLE_ENABLED() && !IsSleHalfDisabled()) {
        NL_CHECK_RETURN_RET(pimpl && pimpl->LoadNearlinkHostService(), NL_ERR_INTERNAL_ERROR,
            "pimpl is null or load nearlink service failed.");
    }
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->FactoryReset();
}

NlErrCode NearlinkHost::SetFreqHopping(const std::vector<uint8_t> &freq)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    if (freq.size() != SLE_FREQ_HOPPING_LEN) {
        HILOGE("Param_Freq len is invalid!");
        return NL_ERR_INVALID_PARAM;
    }
    NL_CHECK_RETURN_RET(IsSleAvailableToCaller(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->SetFreqHopping(freq);
}

NlErrCode NearlinkHost::UpdateSleVirtualDevice(int32_t cmd, const std::string &address)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(IsValidAddress(address), NL_ERR_INTERNAL_ERROR, "Invalid sle virtual device");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->UpdateSleVirtualDevice(cmd, address);
}

NlErrCode NearlinkHost::UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime)
{
    NlErrCode result = NL_ERR_API_NOT_SUPPORT;
#ifdef NEARLINK_KIA_ENABLE
    HILOGI("enter, protocolType:%{public}d, pid:%{public}d, refuseTime:%{public}ld", protocolType, pid, refuseTime);
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    result = proxy->UpdateRefusePolicy(protocolType, pid, refuseTime);
#endif
    return result;
}

NlErrCode NearlinkHost::CheckPermissionForNapi(const std::string &permission, bool &isGranted)
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->CheckPermissionForNapi(permission, isGranted);
}

NlErrCode NearlinkHost::GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr)
{
	NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(btAddr), NL_ERR_INTERNAL_ERROR, "Invalid bt remote device");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    NlErrCode ret = proxy->GetSleAddrByBtAddr(btAddr, sleAddr);
    HILOGD("btAddr=%{public}s, sleAddr=%{public}s", GetEncryptAddr(btAddr).c_str(), GetEncryptAddr(sleAddr).c_str());
    return ret;
}

NlErrCode NearlinkHost::GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr)
{
	NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IsValidAddress(sleAddr), NL_ERR_INTERNAL_ERROR, "Invalid sle remote device");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    NlErrCode ret = proxy->GetBtAddrBySleAddr(sleAddr, btAddr);
    HILOGD("btAddr=%{public}s, sleAddr=%{public}s", GetEncryptAddr(btAddr).c_str(), GetEncryptAddr(sleAddr).c_str());
    return ret;
}

NlErrCode NearlinkHost::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr)
{
    HILOGD("btAddr=%{public}s, sleAddr=%{public}s", GetEncryptAddr(btAddr).c_str(), GetEncryptAddr(sleAddr).c_str());
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), NL_ERR_SLE_OFF, "nearlink is off.");
    NL_CHECK_RETURN_RET(IsValidAddress(sleAddr), NL_ERR_INTERNAL_ERROR, "Invalid sle remote device");
    NL_CHECK_RETURN_RET(IsValidAddress(btAddr), NL_ERR_INTERNAL_ERROR, "Invalid bt remote device");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, NL_ERR_UNAVAILABLE_PROXY, "proxy is nullptr");
    return proxy->SetBtAddrBySleAddr(sleAddr, btAddr);
}

bool NearlinkHost::IsNearlinkSupportFrame4() const
{
    HILOGD("enter");
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), false, "nearlink is not support.");
    NL_CHECK_RETURN_RET(pimpl != nullptr, false, "piml is nullptr");

    std::unique_lock<std::mutex> lock(pimpl->isNearlinkSupportFrame4Mutex_);
    if (pimpl->isNearlinkSupportFrame4_ == NEARLINK_UNKNOWN) {
        pimpl->isNearlinkSupportFrame4_ = OHOS::system::GetBoolParameter("const.nearlink.support_frame4", false) ?
            NEARLINK_SUPPORTED : NEARLINK_NOT_SUPPORTED;
    }
    return pimpl->isNearlinkSupportFrame4_ == NEARLINK_SUPPORTED;
}

bool NearlinkHost::IsFeatureSupported(SleFeatureSupported feature)
{
    HILOGD("enter");
    if (feature == SleFeatureSupported::SLE_RADIO_FRAME_TYPE_4) {
        return IsNearlinkSupportFrame4();
    }
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), false, "nearlink is not support.");
    NL_CHECK_RETURN_RET(IS_SLE_ENABLED(), false, "nearlink is off.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, false, "proxy is nullptr");
    bool isSupported = false;
    NlErrCode ret = proxy->IsFeatureSupported(static_cast<int32_t>(feature), isSupported);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, false, "get isFeatureSupported failed, error code: %{public}d", ret);
    return isSupported;
}

bool NearlinkHost::IsConnectionExist()
{
    NL_CHECK_RETURN_RET(IsNearlinkSupport(), false, "nearlink is not support.");
    sptr<INearlinkHost> proxy = GetProxy<INearlinkHost>(NEARLINK_HOST);
    NL_CHECK_RETURN_RET(proxy, false, "proxy is nullptr");
    bool isConnectionExist = true;
    NlErrCode ret = proxy->IsConnectionExist(isConnectionExist);
    NL_CHECK_RETURN_RET(ret == NL_NO_ERROR, true, "get isConnectionExist failed, error code: %{public}d", ret);
    return isConnectionExist;
}

bool NearlinkHost::IsBasSupported()
{
#ifdef NEARLINK_BAS_ENABLE
    return IsNearlinkSupport();
#endif
    return false;
}

} // namespace Nearlink
} // namespace OHOS
