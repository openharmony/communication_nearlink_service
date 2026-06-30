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

#include "PortService.h"
#include "PortClientStackAdapter.h"
#include "PortServerStackAdapter.h"

#include "ClassCreator.h"
#include "SleServiceFfrtLog.h"
#include "SleUtils.h"
#include "ThreadUtil.h"
#include "SleRemoteDeviceAdapter.h"

#include <future>

namespace OHOS {
namespace Nearlink {
struct PortService::impl {
    impl() = default;
    ~impl() = default;
    bool isEnabled_ = false;
    // client
    PortClientStackAdapter clientStackAdapter_;
    BaseObserverList<PortObserver> portObservers_ {};
    // server
    PortServerStackAdapter serverStackAdapter_;
};

PortService::PortService()
    : utility::Context(PROFILE_NAME_PORT, "1.0.0"), pimpl(std::make_unique<PortService::impl>())
{
    HILOGI("[PORT Service]%{public}s:%{public}s Create", PROFILE_NAME_PORT.c_str(), Name().c_str());

    pimpl->clientStackAdapter_.RegisterCallBackToStack();
}

PortService::~PortService()
{
    HILOGI("[PORT Service]%{public}s:%{public}s Destroy", PROFILE_NAME_PORT.c_str(), Name().c_str());

    pimpl->clientStackAdapter_.DeregisterCallBackToStack();
}

utility::Context *PortService::GetContext()
{
    return this;
}

PortService *PortService::GetPortService()
{
    return static_cast<PortService *>(
        SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_PORT));
}

void PortService::RegisterObserver(PortObserver &portObserver)
{
    HILOGI("[Port Service] Enter");
    pimpl->portObservers_.Register(portObserver);
}

void PortService::DeregisterObserver(PortObserver &portObserver)
{
    HILOGI("[Port Service] Enter");
    pimpl->portObservers_.Deregister(portObserver);
}

void PortService::NotifyStateChanged(const RawAddress &device, SleConnectState curState, SleConnectState preState)
{
    DoInPortThread([this, device, curState, preState]() -> void {
        HILOGD("[PORT Service], curState(%{public}d), preState(%{public}d)", curState, preState);
        pimpl->portObservers_.ForEach([device, curState, preState](PortObserver &observer) {
            observer.OnConnectionStateChanged(device, static_cast<int>(curState), static_cast<int>(preState));
        });
    });
}

void PortService::Enable()
{
    DoInPortThread([this]() -> void {
        HILOGI("[PORT Service] start enable port service");
        if (pimpl->isEnabled_) {
            GetContext()->OnEnable(PROFILE_NAME_PORT, true);
            HILOGW("[PORT Service] PortService has already been enabled before.");
            return;
        }
        GetContext()->OnEnable(PROFILE_NAME_PORT, true);
        pimpl->isEnabled_ = true;
    });

    HILOGI("[PORT Service] port service enabled");
}

void PortService::Disable()
{
    DoInPortThread([this]() -> void {
        HILOGI("[PORT Service] start disable port service");
        if (!pimpl->isEnabled_) {
            GetContext()->OnDisable(PROFILE_NAME_PORT, true);
            HILOGW("[PORT Service] PortService has already been disabled before.");
            return;
        }
        GetContext()->OnDisable(PROFILE_NAME_PORT, true);
        pimpl->isEnabled_ = false;
    });
}

int PortService::Connect(const RawAddress &device)
{
    DoInPortThread([this, device]() -> void {
        SleRemoteDeviceAdapter::GetInstance()->SetConnDirectActive(device);
        HILOGD("[Port Service] Connect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
        int state = static_cast<int>(SleConnectState::DISCONNECTED);
        pimpl->clientStackAdapter_.GetConnectState(device, state);
        if (state == static_cast<int>(SleConnectState::CONNECTED)) {
            pimpl->portObservers_.ForEach([device](PortObserver &observer) {
                observer.OnConnectionStateChanged(device, static_cast<int>(SleConnectState::CONNECTED),
                    static_cast<int>(SleConnectState::CONNECTED));
            });
            return;
        }
        pimpl->clientStackAdapter_.Connect(device);
    });
    return PORT_SUCCESS;
}

int PortService::ConnectWithParam(const RawAddress &device, const PortServiceConnParam &param)
{
    DoInPortThread([this, device, param]() -> void {
        HILOGI("[Port Service] Connect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
        int state = static_cast<int>(SleConnectState::DISCONNECTED);
        pimpl->clientStackAdapter_.GetConnectState(device, state);
        if (state == static_cast<int>(SleConnectState::CONNECTED)) {
            pimpl->portObservers_.ForEach([device](PortObserver &observer) {
                observer.OnConnectionStateChanged(device, static_cast<int>(SleConnectState::CONNECTED),
                    static_cast<int>(SleConnectState::CONNECTED));
            });
            return;
        }
        NLSTK_ConnParam_S connParam = {};
        connParam.frameType = param.GetFrameType();
        pimpl->clientStackAdapter_.Connect(device, connParam);
    });
    return PORT_SUCCESS;
}

int PortService::Disconnect(const RawAddress &device)
{
    DoInPortThread([this, device]() -> void {
        HILOGI("[Port Service] Disconnect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
        pimpl->clientStackAdapter_.Disconnect(device);
    });
    return PORT_SUCCESS;
}


int PortService::AddPortByUuid(const Uuid::UUID128Bit& uuid, uint16_t portId)
{
    NL_CHECK_RETURN_RET((uuid.size() == static_cast<std::size_t>(Uuid::UUID128_BYTES_TYPE)),
        static_cast<int>(ReturnValue::RET_BAD_PARAM), "add port but uuid is err!");

    DoInPortThread([this, uuid, portId]() -> void {
        HILOGI("[Port Service] add portId(%{public}hu, uuid:%{public}s)",
            portId, Uuid::ConvertFrom128Bits(uuid).GetEncryptUuid().c_str());
        uint16_t manufactureId = pimpl->serverStackAdapter_.LoadManufactureInfo();
        pimpl->serverStackAdapter_.AddPortByUuid(uuid, manufactureId, portId);
    });
    return PORT_SUCCESS;
}

int PortService::DeletePortByUuid(const Uuid::UUID128Bit& uuid, uint16_t portId)
{
    NL_CHECK_RETURN_RET((uuid.size() == static_cast<std::size_t>(Uuid::UUID128_BYTES_TYPE)),
        static_cast<int>(ReturnValue::RET_BAD_PARAM), "delete port but uuid is err!");
    DoInPortThread([this, uuid, portId]() {
        HILOGI("[Port Service] delete portId(%{public}hu, uuid:%{public}s)",
            portId, Uuid::ConvertFrom128Bits(uuid).GetEncryptUuid().c_str());
        pimpl->serverStackAdapter_.DeletePortByUuid(uuid, portId);
    });
    return PORT_SUCCESS;
}

uint16_t PortService::GetRemotePortByUuid(const RawAddress &device, const Uuid::UUID128Bit& uuid)
{
    HILOGD("[Port Service] get remote port by uuid:%{public}s)",
        Uuid::ConvertFrom128Bits(uuid).GetEncryptUuid().c_str());
    NL_CHECK_RETURN_RET((uuid.size() == static_cast<std::size_t>(Uuid::UUID128_BYTES_TYPE)),
        static_cast<int>(ReturnValue::RET_BAD_PARAM), "get remote port by uuid but uuid is err!");

    std::promise<uint16_t> promise;

    DoInPortThread([this, &device, &uuid, &promise]() -> void {
        uint16_t result = pimpl->clientStackAdapter_.GetRemotePortByUuid(device, uuid);
        HILOGD("[Port Service] GetRemotePortByUuid result:%{public}d", result);
        promise.set_value(result);
    });
    return promise.get_future().get();
}

int PortService::GetConnectState(const RawAddress &device)
{
    std::promise<int> promise;
    DoInPortThread([this, &device, &promise]() -> void {
        int state = static_cast<int>(SleConnectState::DISCONNECTED);
        int result = pimpl->clientStackAdapter_.GetConnectState(device, state);
        HILOGD("[Port Service] GetConnectState result:%{public}d", result);
        promise.set_value(static_cast<int>(state));
    });
    return promise.get_future().get();
}

int PortService::GetConnectState()
{
    return PORT_STATE_DISCONNECTED;
}

std::list<RawAddress> PortService::GetConnectDevices(void)
{
    return std::list<RawAddress>();
}

REGISTER_CLASS_CREATOR(PortService);

} // namespace Nearlink
} // namespace OHOS