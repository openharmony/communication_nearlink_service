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
#include "BasService.h"
#include <vector>
#include "nearlink_safe_list.h"
#include "SleInterfaceManager.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int BAS_SUCCESS = 0;
constexpr int BAS_FAILURE = 1;
}  // namespace

struct BasService::impl {
    impl() = default;
    ~impl() = default;
    bool isEnabled_ = false;

    BasClientStackAdapter clientStackAdapter_;
    BaseObserverList<BasObserver> basObservers_{};
    BaseObserverList<IDeviceBatteryCallback> deviceBatteryObservers_{};
};

BasService::BasService() : utility::Context(PROFILE_NAME_BAS, "1.0.0"), pimpl(std::make_unique<BasService::impl>())
{
    HILOGI("[BAS Service]%{public}s:%{public}s Create", PROFILE_NAME_BAS.c_str(), Name().c_str());
    // 客户端向协议栈注册回调
    if (pimpl->clientStackAdapter_.RegisterCallBackToStack() != BAS_SUCCESS) {
        HILOGE("[BAS Service] register cb to stack failed.");
    }
}

BasService::~BasService()
{
    HILOGI("[BAS Service]%{public}s:%{public}s Destroy", PROFILE_NAME_BAS.c_str(), Name().c_str());
}

utility::Context *BasService::GetContext()
{
    return this;
}

void BasService::Enable()
{
    DoInBasThread([this]() -> void {
        HILOGI("[BAS Service] start enable dis service");
        if (pimpl->isEnabled_) {
            GetContext()->OnEnable(PROFILE_NAME_BAS, true);
            HILOGW("[BAS Service] BasService has already been enabled before.");
            return;
        }
        GetContext()->OnEnable(PROFILE_NAME_BAS, true);
    });
}

void BasService::Disable()
{
    DoInBasThread([this]() -> void {
        HILOGI("[BAS Service] start disable bas service");
        if (!pimpl->isEnabled_) {
            GetContext()->OnDisable(PROFILE_NAME_BAS, true);
            HILOGW("[BAS Service] BasService has already been disabled before.");
            return;
        }
        GetContext()->OnDisable(PROFILE_NAME_BAS, true);
    });
}

BasService *BasService::GetInstance()
{
    return static_cast<BasService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_BAS));
}

ProfileBas *ProfileBas::GetInstance()
{
    return BasService::GetInstance();
}

void BasService::RegisterObserver(BasObserver &basObserver)
{
    HILOGI("[BAS Service] Enter");
    pimpl->basObservers_.Register(basObserver);
}

void BasService::DeregisterObserver(BasObserver &basObserver)
{
    HILOGD("[BAS Service] Enter");
    pimpl->basObservers_.Deregister(basObserver);
}

void BasService::RegisterDeviceObserver(IDeviceBatteryCallback &deviceBatteryObserver)
{
    HILOGI("[BAS Service] Enter");
    pimpl->deviceBatteryObservers_.Register(deviceBatteryObserver);
}

void BasService::DeregisterDeviceObserver(IDeviceBatteryCallback &deviceBatteryObserver)
{
    HILOGD("[BAS Service] Enter");
    pimpl->deviceBatteryObservers_.Deregister(deviceBatteryObserver);
}

int BasService::Connect(const RawAddress &device)
{
    DoInBasThread([this, device]() -> void {
        HILOGI("[BAS Service] Connect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
        pimpl->clientStackAdapter_.Connect(device);
    });
    return BAS_SUCCESS;
}

int BasService::Disconnect(const RawAddress &device)
{
    DoInBasThread([this, device]() -> void {
        HILOGI("[BAS Service] Disconnect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
        pimpl->clientStackAdapter_.Disconnect(device);
    });
    return BAS_SUCCESS;
}

void BasService::GetDeviceBatteryLevel(const RawAddress &device)
{
    DoInBasThread([this, device]() -> void {
        HILOGI("[BAS Service] Get Device addr(%{public}s) Battery Level", GET_ENCRYPT_ADDR(device));
        pimpl->clientStackAdapter_.GetDeviceBatteryLevel(device);
    });
}

void BasService::NotifyStateChanged(const RawAddress &device, SleConnectState curState, SleConnectState preState)
{
    DoInBasThread([this, device, curState, preState]() -> void {
        HILOGI("[BAS Service] addr(%{public}s), curState(%{public}d), preState(%{public}d)",
            GET_ENCRYPT_ADDR(device), curState, preState);
        if (curState == SleConnectState::CONNECTED && preState == SleConnectState::CONNECTING) {
        }

        pimpl->basObservers_.ForEach([device, curState, preState](BasObserver &observer) {
            observer.OnConnectionStateChanged(device, static_cast<int>(curState), static_cast<int>(preState));
        });
    });
}

void BasService::NotifyBatteryLevelEvent(const RawAddress &device, int8_t batteryLevel)
{
    DoInBasThread([this, device, batteryLevel]() -> void {
        HILOGI("[BAS Service] addr(%{public}s), batteryLevel(%{public}d)", GET_ENCRYPT_ADDR(device), batteryLevel);
        pimpl->deviceBatteryObservers_.ForEach(
            [device, batteryLevel](IDeviceBatteryCallback &observer) -> void {
                observer.OnGetBatteryLevelEvent(device, batteryLevel);
            });
    });
}

void BasService::NotifyBatteryLevelChanged(const RawAddress &device, int8_t batteryLevel)
{
    DoInBasThread([this, device, batteryLevel]() -> void {
        HILOGI("[BAS Service] addr(%{public}s), batteryLevel(%{public}d)", GET_ENCRYPT_ADDR(device), batteryLevel);
        pimpl->deviceBatteryObservers_.ForEach([device, batteryLevel](IDeviceBatteryCallback &observer) -> void {
            observer.OnBatteryLevelChanged(device, batteryLevel);
        });
    });
}

std::list<RawAddress> BasService::GetConnectDevices()
{
    return std::list<RawAddress>();
}

int BasService::GetConnectState()
{
    return static_cast<int>(SleConnectState::DISCONNECTED);
}

REGISTER_CLASS_CREATOR(BasService);

}  // namespace Nearlink
}  // namespace OHOS
