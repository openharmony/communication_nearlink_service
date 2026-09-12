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

#include "ani_nearlink_remote_device_callback.h"
#include "ohos.nearlink.constant.proj.hpp"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {
namespace {
std::shared_ptr<AniRemoteDeviceObserver> g_aniNearlinkRemoteDeviceObserver =
    std::make_shared<AniRemoteDeviceObserver>();
}

std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::PairingRequestParam const&)>>>
    g_pairingRequestObserverVec {};
std::shared_mutex g_pairingRequestMutex;
std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::PairingStateParam const&)>>>
    g_pairStatusChangedObserverVec {};
std::shared_mutex g_pairStatusChangedMutex;
std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::ConnectionStateParam const&)>>>
    g_connectionStateChangedObserverVec {};
std::shared_mutex g_connectionStateChangedMutex;
std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::remoteDevice::AcbStateParam const&)>>>
    g_acbStateChangedObserverVec {};
std::shared_mutex g_acbStateChangedMutex;

void AniRemoteDeviceObserver::OnPairingRequest(const NearlinkRemoteDevice &device, const std::string &passkey, int type)
{
    HILOGI("device is %{public}s, type is %{public}d", GET_ENCRYPT_DEVICE_ADDR(device), type);
    ::ohos::nearlink::remoteDevice::PairingRequestParam result = {
        .address = static_cast<::taihe::string>(device.GetDeviceAddr()),
        .passkey = static_cast<::taihe::string>(passkey),
        .pairingType = ohos::nearlink::remoteDevice::PairingType::from_value(type)
    };
    decltype(g_pairingRequestObserverVec) callbacks;
    {
        std::shared_lock<std::shared_mutex> guard(g_pairingRequestMutex);
        callbacks = g_pairingRequestObserverVec;
    }
    for (auto callback : callbacks) {
        if (callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniRemoteDeviceObserver::OnPairStatusChanged(const NearlinkRemoteDevice &device,
    int preState, int state, int reason)
{
    HILOGI("device is %{public}s, preState is %{public}d, state is %{public}d, reason is %{public}d",
           GET_ENCRYPT_DEVICE_ADDR(device), preState, state, reason);
    ::ohos::nearlink::remoteDevice::PairingStateParam result = {
        .address = static_cast<::taihe::string>(device.GetDeviceAddr()),
        .preState = ohos::nearlink::constant::PairingState::from_value(preState),
        .state = ohos::nearlink::constant::PairingState::from_value(state),
        .reason = ohos::nearlink::remoteDevice::PairingReason::from_value(reason)
    };
    decltype(g_pairStatusChangedObserverVec) callbacks;
    {
        std::shared_lock<std::shared_mutex> guard(g_pairStatusChangedMutex);
        callbacks = g_pairStatusChangedObserverVec;
    }
    for (auto callback : callbacks) {
        if (callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniRemoteDeviceObserver::OnConnectionStateChanged(const NearlinkRemoteDevice &device,
    int preState, int state, int reason)
{
    HILOGI("device is %{public}s, preState is %{public}d, state is %{public}d, reason is %{public}d",
        GET_ENCRYPT_DEVICE_ADDR(device), preState, state, reason);
    ::ohos::nearlink::remoteDevice::ConnectionStateParam result = {
        .address = static_cast<::taihe::string>(device.GetDeviceAddr()),
        .preState = ohos::nearlink::constant::ConnectionState::from_value(preState),
        .state = ohos::nearlink::constant::ConnectionState::from_value(state),
        .connectionReason = ::ohos::nearlink::remoteDevice::ConnectionReason::from_value(reason)
    };
    decltype(g_connectionStateChangedObserverVec) callbacks;
    {
        std::shared_lock<std::shared_mutex> guard(g_connectionStateChangedMutex);
        callbacks = g_connectionStateChangedObserverVec;
    }
    for (auto callback : callbacks) {
        if (callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniRemoteDeviceObserver::OnAcbStateChanged(const NearlinkRemoteDevice &device, int state, int reason)
{
    HILOGI("device is %{public}s, state is %{public}d, reason is %{public}d",
        GET_ENCRYPT_DEVICE_ADDR(device), state, reason);
    ::ohos::nearlink::remoteDevice::AcbStateParam result = {
        .address = static_cast<::taihe::string>(device.GetDeviceAddr()),
        .state = ohos::nearlink::constant::AcbState::from_value(state)
    };
    decltype(g_acbStateChangedObserverVec) callbacks;
    {
        std::shared_lock<std::shared_mutex> guard(g_acbStateChangedMutex);
        callbacks = g_acbStateChangedObserverVec;
    }
    for (auto callback : callbacks) {
        if (callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniRemoteDeviceObserver::CallbackInit()
{
    HILOGI("enter");
    NearlinkHost::GetInstance().RegisterRemoteDeviceObserver(g_aniNearlinkRemoteDeviceObserver);
}
}
}