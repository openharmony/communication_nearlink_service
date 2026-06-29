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
#include "MicService.h"

#include "SleInterfaceProfileManager.h"
#include "ThreadUtil.h"
#include "BaseObserverList.h"
#include "ClassCreator.h"
#include "SleServiceFfrtLog.h"
#include "MicClientStackAdapter.h"

namespace OHOS {
namespace Nearlink {
struct MicService::impl {
    impl() = default;
    ~impl() = default;
    bool isEnabled_ = false;
    MicClientStackAdapter clientStackAdapter_;
    BaseObserverList<MicObserver> micObservers_ {};
    BaseObserverList<MicStateObserver> micStateObservers_ {};
    NearlinkSafeMap<std::string, MicState> deviceMicState_;
};

MicService::MicService() : utility::Context(PROFILE_NAME_MIC, "1.0.0"), pimpl(std::make_unique<impl>())
{
    HILOGD("[MicService]%{public}s:%{public}s Create", PROFILE_NAME_MIC.c_str(), Name().c_str());
    // 客户端向协议栈注册回调
    const int ret = pimpl->clientStackAdapter_.RegisterCallBackToStack();
    NL_CHECK_RETURN(ret == MIC_SUCCESS, "[MicAdapter]register cb to stack failed.");
}

MicService::~MicService()
{
    HILOGD("[MicService]%{public}s:%{public}s Destroy", PROFILE_NAME_MIC.c_str(), Name().c_str());
}

utility::Context *MicService::GetContext()
{
    return this;
}

MicService *MicService::GetService()
{
    return static_cast<MicService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_MIC));
}

void MicService::RegisterObserver(MicObserver &serviceObserver)
{
    HILOGD("[MicService]Enter");
    pimpl->micObservers_.Register(serviceObserver);
}

void MicService::DeregisterObserver(MicObserver &serviceObserver)
{
    HILOGD("[MicService]Enter");
    pimpl->micObservers_.Deregister(serviceObserver);
}

void MicService::RegisterMicStateObserver(MicStateObserver &observer)
{
    HILOGD("[MicService]Enter");
    pimpl->micStateObservers_.Register(observer);
}

void MicService::DeregisterMicStateObserver(MicStateObserver &observer)
{
    HILOGD("[MicService]Enter");
    pimpl->micStateObservers_.Deregister(observer);
}

int MicService::GetConnectState()
{
    return static_cast<int>(SleConnectState::DISCONNECTED);
}

std::list<RawAddress> MicService::GetConnectDevices()
{
    return std::list<RawAddress>();
}

void MicService::Enable()
{
    HILOGD("[MicService]Enter");
    DoInMicThread([this]() -> void {
        HILOGI("[MicService] start enable mic service");
        if (pimpl->isEnabled_) {
            GetContext()->OnEnable(PROFILE_NAME_MIC, true);
            HILOGW("[MicService] MicService has already been enabled before.");
            return;
        }
        GetContext()->OnEnable(PROFILE_NAME_MIC, true);

        pimpl->isEnabled_ = true;
        HILOGI("[MicService] mic service enabled");
    });
}

void MicService::Disable()
{
    HILOGD("[MicService]Enter");
    DoInMicThread([this]() -> void {
        HILOGI("[MicService] start disable mic service");
        if (!pimpl->isEnabled_) {
            GetContext()->OnDisable(PROFILE_NAME_MIC, true);
            HILOGW("[MicService] MicService has already been disabled before.");
            return;
        }
        GetContext()->OnDisable(PROFILE_NAME_MIC, true);

        pimpl->isEnabled_ = false;
        HILOGI("[MicService] mic service disabled");
    });
}

int MicService::Connect(const RawAddress &device)
{
    HILOGD("[MicService]Enter");
    DoInMicThread([this, addr = device]() -> void {
        HILOGI("[MicService] Connect addr(%{public}s)", GET_ENCRYPT_ADDR(addr));
        pimpl->clientStackAdapter_.Connect(addr);
    });
    return MIC_SUCCESS;
}

int MicService::Disconnect(const RawAddress &device)
{
    HILOGD("[MicService]Enter");
    DoInMicThread([this, addr = device]() -> void {
        HILOGI("[MicService] Disconnect addr(%{public}s)", GET_ENCRYPT_ADDR(addr));
        pimpl->clientStackAdapter_.Disconnect(addr);
    });
    return MIC_SUCCESS;
}

void MicService::UpdateMicState(const RawAddress &device, uint8_t micState)
{
    HILOGD("[MicService]Enter");
    DoInMicThread([this, device, micState]() -> void {
        MicState newState = static_cast<MicState>(micState);
        auto fun = [this, device, newState](const std::string& addr, MicState &state) -> void {
            if (state == newState) {
                HILOGI("[MicService]Update mic state, but state is same: device=%{public}s, micState=%{public}u",
                    GET_ENCRYPT_ADDR(device), static_cast<uint8_t>(state));
                return;
            }
            HILOGI("[MicService]Update mic state: device=%{public}s, micState=%{public}u -> %{public}u",
                GET_ENCRYPT_ADDR(device), static_cast<uint8_t>(state), static_cast<uint8_t>(newState));
            state = newState;

            pimpl->micStateObservers_.ForEach([device, state](MicStateObserver &observer) {
                observer.OnMicStateChanged(device, state);
            });
        };
        pimpl->deviceMicState_.GetValueAndOpt(device.GetAddress(), fun);
    });
}

bool MicService::IsMicOpen(const RawAddress &device)
{
    HILOGD("[MicService]Enter");
    MicState micState;
    if (pimpl->deviceMicState_.GetValue(device.GetAddress(), micState)) {
        HILOGD("[MicService]device(%{public}s) micState is %{public}s",
            GET_ENCRYPT_ADDR(device), micState == MIC_ON ? "ON" : "OFF");
        return micState == MIC_ON;
    }
    return false;
}

void MicService::NotifyStateChanged(const RawAddress &device, SleConnectState curState, SleConnectState preState)
{
    HILOGD("[MicService]Enter");
    DoInMicThread([this, device, curState, preState]() -> void {
        HILOGD("[MicService]NotifyStateChanged curState(%{public}d), preState(%{public}d)", curState, preState);
        if (curState == SleConnectState::CONNECTED) {
            pimpl->deviceMicState_.EnsureInsert(device.GetAddress(), MIC_UNKNOWN);
        } else if (curState == SleConnectState::DISCONNECTED) {
            pimpl->deviceMicState_.Erase(device.GetAddress());
        }

        pimpl->micObservers_.ForEach([device, curState, preState](MicObserver &observer) {
            observer.OnConnectionStateChanged(device, static_cast<int>(curState), static_cast<int>(preState));
        });
    });
}

REGISTER_CLASS_CREATOR(MicService);

}
}
