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
#include <future>
#include "BaseObserverList.h"
#include "ClassCreator.h"
#include "IcceDefines.h"
#include "SleInterfaceProfileManager.h"
#include "SleServiceFfrtLog.h"
#include "ThreadUtil.h"
#include "nearlink_dft_excep.h"
#include "ProfileServiceManager.h"
#include "IcceClientStackAdapter.h"
#include "IcceServerStackAdapter.h"
#include "nearlink_dft_exception.h"
#include "IcceService.h"
#include "nearlink_dft_exception.h"
#include "nearlink_dft_database.h"
#include "nearlink_system_config.h"

namespace OHOS {
namespace Nearlink {
constexpr uint8_t ICCE_MAX_CONNECTION_NUM = 6;

struct IcceService::impl {
    class IcceClientCallback : public IcceClientStackCallback {
    public:
        IcceClientCallback(impl &pimpl) : pimpl_(pimpl) {}
        void OnConnectionStateChanged(const RawAddress &device, int curState, int prevState) override;
    private:
        impl &pimpl_;
    };

    impl() : clientCallback_(*this), clientAdapter_(clientCallback_) {}
    ~impl() = default;

    bool isRunningState_ = false;
    BaseObserverList<IcceObserver> icceObservers_;
    IcceClientCallback clientCallback_;
    IcceClientStackAdapter clientAdapter_;
    IcceServerStackAdapter serverAdapter_;
};

void IcceService::impl::IcceClientCallback::OnConnectionStateChanged(const RawAddress &device, int curState,
    int prevState)
{
    if (curState == ICCE_CONNECTED && prevState == ICCE_CONNECTING) {
        DftCacheIcceFinish(device.GetAddress());
    }
    pimpl_.icceObservers_.ForEach([device, curState, prevState](IcceObserver &observer) {
        observer.OnConnectionStateChanged(device, curState, prevState);
    });
}

IcceService* IcceService::GetService()
{
    return static_cast<IcceService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ICCE));
}

IcceService::IcceService()
    : utility::Context(PROFILE_NAME_ICCE, "1.0.0"), pimpl(std::make_unique<IcceService::impl>())
{
    HILOGD("[ICCE Profile]%{public}s:%{public}s Create", PROFILE_NAME_ICCE.c_str(), Name().c_str());
}

IcceService::~IcceService()
{
    HILOGD("[ICCE Profile]%{public}s:%{public}s Destroy", PROFILE_NAME_ICCE.c_str(), Name().c_str());
}

utility::Context *IcceService::GetContext()
{
    return this;
}

void IcceService::EnableTask()
{
    if (pimpl->isRunningState_) {
        GetContext()->OnEnable(PROFILE_NAME_ICCE, true);
        HILOGW("[Icce Service] IcceService has already been enabled before.");
        return;
    }
    pimpl->clientAdapter_.RegisterCallBackToStack();
    pimpl->serverAdapter_.CreateServerInfo();
    pimpl->isRunningState_ = true;
    GetContext()->OnEnable(PROFILE_NAME_ICCE, true);
}

void IcceService::Enable()
{
    HILOGI("[ICCE Profile] Enable");
    DoInIcceThread([this]() -> void {
        this->EnableTask();
    });
}

void IcceService::DisableTask()
{
    if (!pimpl->isRunningState_) {
        GetContext()->OnDisable(PROFILE_NAME_ICCE, true);
        HILOGW("[Icce Service] IcceService has already been disabled before.");
        return;
    }
    pimpl->serverAdapter_.DestroyServerInfo();
    pimpl->isRunningState_ = false;
    GetContext()->OnDisable(PROFILE_NAME_ICCE, true);
}

void IcceService::Disable()
{
    HILOGD("[ICCE Profile] Disable");
    DoInIcceThread([this]() -> void {
        this->DisableTask();
    });
}

void IcceService::ConnectTask(const RawAddress &device)
{
    NL_CHECK_RETURN(pimpl->isRunningState_, "[ICCE Service]:IcceService is shutting down");
    if (GetConnectionsDeviceNum() >= ICCE_MAX_CONNECTION_NUM) {
        DftReportPairInfo(device.GetAddress(), PAIR_CONN_PATH_ICCE, ICCE_FAILURE, PAIR_ICCE_REACH_MAX_NUM);
        HILOGE("[ICCE Service]:Max connection number has reached!");
        return;
    }

    if (!NearlinkSystemConfig::IsIcceProfileSupported()) {
        DftReportPairInfo(device.GetAddress(), PAIR_CONN_PATH_ICCE, ICCE_FAILURE, PAIR_ICCE_NOT_SUPPORT);
        HILOGE("[ICCE Profile] not support icce profile.");
        return;
    }
    pimpl->clientAdapter_.Connect(device);
}

int IcceService::Connect(const RawAddress &device)
{
    HILOGI("[ICCE Service] Connect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
    DftCacheIcceStart(device.GetAddress());
    DoInIcceThread(([this, device] ()-> void {
        this->ConnectTask(device);
    }));
    return ICCE_SUCCESS;
}

void IcceService::DisconnectTask(const RawAddress &device)
{
    pimpl->clientAdapter_.Disconnect(device);
}

int IcceService::Disconnect(const RawAddress &device)
{
    HILOGI("[ICCE Service] Disconnect addr(%{public}s)", GET_ENCRYPT_ADDR(device));
    DoInIcceThread([this, device] ()-> void {
        this->DisconnectTask(device);
    });
    return ICCE_SUCCESS;
}

int32_t IcceService::GetPortTask(const RawAddress &device)
{
    return pimpl->clientAdapter_.GetPort(device);
}

int32_t IcceService::GetPort(const RawAddress &device)
{
    std::promise<int32_t> promise;
    DoInIcceThread([this, device, &promise] ()-> void {
        int32_t port = GetPortTask(device);
        promise.set_value(port);
    });
    return promise.get_future().get();
}

void IcceService::RegisterObserver(IcceObserver &icceObserver)
{
    pimpl->icceObservers_.Register(icceObserver);
}

void IcceService::DeregisterObserver(IcceObserver &icceObserver)
{
    pimpl->icceObservers_.Deregister(icceObserver);
}

uint8_t IcceService::GetConnectionsDeviceNum()
{
    return pimpl->clientAdapter_.GetConnectionsDeviceNum();
}

REGISTER_CLASS_CREATOR(IcceService);

} // namespace Nearlink
} // namespace OHOS