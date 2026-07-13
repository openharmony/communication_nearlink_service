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

#include "SleServiceManager.h"

#include "log_util.h"
#include "SleAdapter.h"

namespace OHOS {
namespace Nearlink {
SleInterfaceManager *SleInterfaceManager::GetInstance()
{
    HILOGI("[SleServiceManager Mocker] SleInterfaceManager GetInstance");
    return SleServiceManager::GetInstance();
}

SleServiceManager *SleServiceManager::GetInstance()
{
    HILOGI("[SleServiceManager Mocker] GetInstance");
    static SleServiceManager instance;
    SleServiceManager *ptrInstance = &instance;
    return &instance;
}

struct SleServiceManager::impl {
    impl();
    ~impl();

    std::atomic<SleStateID> state_ = SleStateID::STATE_TURN_OFF;
    std::shared_ptr<SleInterfaceAdapter> instance_ = nullptr;
};

SleServiceManager::impl::impl()
{}

SleServiceManager::impl::~impl()
{}

SleServiceManager::SleServiceManager() : pimpl(std::make_unique<SleServiceManager::impl>())
{
    HILOGI("[SleServiceManager Mocker] construct");
}

SleServiceManager::~SleServiceManager()
{
    HILOGI("~SleServiceManager Mocker");
}

bool SleServiceManager::Start()
{
    HILOGI("[SleServiceManager Mocker] Start");
    return true;
}

void SleServiceManager::Stop() const
{
    HILOGI("[SleServiceManager Mocker] Stop");
}

bool SleServiceManager::FactoryReset() const
{
    return true;
}

void SleServiceManager::Reset() const
{
    HILOGI("[SleServiceManager Mocker] Reset");
}

int32_t SleServiceManager::Enable(const SleTransport transport, const SleEventType reason,
    const SwitchCallerInfo callerInfo, const SleAutoConnectPolicy autoConnPolicy) const
{
    return 0;
}

int32_t SleServiceManager::Disable(
    const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo) const
{
    return 0;
}

int32_t SleServiceManager::DisableToOff(
    const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo) const
{
    return 0;
}

int32_t SleServiceManager::EnableToHalf(
    const SleTransport transport, const SleEventType reason, const SwitchCallerInfo callerInfo) const
{
    return 0;
}

SleStateID SleServiceManager::GetState(const SleTransport transport) const
{
    return pimpl->state_.load();
}

SleConnectState SleServiceManager::GetAdapterConnectState() const
{
    return SleConnectState::CONNECTED;
}

bool SleServiceManager::RegisterStateObserver(IAdapterStateObserver &observer) const
{
    return true;
}

bool SleServiceManager::DeregisterStateObserver(IAdapterStateObserver &observer) const
{
    return true;
}

bool SleServiceManager::RegisterSystemStateObserver(ISystemStateObserver &observer) const
{
    return true;
}

bool SleServiceManager::DeregisterSystemStateObserver(ISystemStateObserver &observer) const
{
    return true;
}

int SleServiceManager::GetMaxNumConnectedAudioDevices() const
{
    return 0;
}

void SleServiceManager::OnAdapterStateChangeTask(const SleTransport transport, const SleStateID state) const
{
    HILOGI("[SleServiceManager Mocker] OnAdapterStateChangeTask");
    pimpl->state_.store(state);
}

SleInterfaceAdapter *SleServiceManager::GetAdapter(const SleTransport transport) const
{
    if (pimpl == nullptr) {
        HILOGI("pimpl is nullptr");
        return nullptr;
    }
    if (pimpl->instance_ == nullptr) {
        std::shared_ptr<SleInterfaceAdapter> adapter = std::make_shared<SleAdapter>();
        pimpl->instance_ = adapter;
    }

    return pimpl->instance_.get();
}
} // namespace Nearlink
} // namespace OHOS