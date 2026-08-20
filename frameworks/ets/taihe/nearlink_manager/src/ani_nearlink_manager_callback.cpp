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

#include "ani_nearlink_manager_callback.h"
#include "ohos.nearlink.constant.proj.hpp"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {
namespace {
std::shared_ptr<AniNearlinkManagerObserver> g_aniNearlinkManagerObserver =
    std::make_shared<AniNearlinkManagerObserver>();
std::shared_ptr<AniRemoteDeviceObserver> g_aniNearlinkRemoteDeviceObserver =
    std::make_shared<AniRemoteDeviceObserver>();
}

std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::NearlinkState data)>>>
    g_stateChangedObserverVec {};
std::shared_mutex g_stateChangedMutex;
std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::PairingRequestParam const&)>>>
    g_pairingRequestObserverVec {};
std::shared_mutex g_pairingRequestMutex;
std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::PairingStateParam const&)>>>
    g_pairStatusChangedObserverVec {};
std::shared_mutex g_pairStatusChangedMutex;
std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::ConnectionStateParam const&)>>>
    g_connectionStateChangedObserverVec {};
std::shared_mutex g_connectionStateChangedMutex;
std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::AcbStateParam const&)>>>
    g_acbStateChangedObserverVec {};
std::shared_mutex g_acbStateChangedMutex;

void AniNearlinkManagerObserver::OnStateChanged(const int transport, const int status)
{
    HILOGI("transport is %{public}d, status is %{public}d", transport, status);
    if (status < static_cast<int>(SleStateID::STATE_TURN_OFF) ||
        status > static_cast<int>(SleStateID::STATE_TURN_ON)) {
            HILOGE("Invalid status value: %{public}d", status);
            return;
    }
    ::ohos::nearlink::manager::NearlinkState result =
        static_cast<::ohos::nearlink::manager::NearlinkState::key_t>(status);
    std::shared_lock<std::shared_mutex> guard(g_stateChangedMutex);
    for (auto callback : g_stateChangedObserverVec) {
        if(callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniRemoteDeviceObserver::OnPairingRequest(const NearlinkRemoteDevice &device, const std::string &passkey, int type)
{
    HILOGI("device is %{public}s, type is %{public}d", GET_ENCRYPT_DEVICE_ADDR(device), type);
    ::ohos::nearlink::manager::PairingRequestParam result = {
        .address = static_cast<::taihe::string>(device.GetDeviceAddr()),
        .passkey = static_cast<::taihe::string>(passkey),
        .pairingType = static_cast<::ohos::nearlink::manager::PairingType::key_t>(type)
    };
    std::shared_lock<std::shared_mutex> guard(g_pairingRequestMutex);
    for (auto callback : g_pairingRequestObserverVec) {
        if(callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniRemoteDeviceObserver::OnPairStatusChanged(const NearlinkRemoteDevice &device,
    int preState, int state, int reason)
{
    HILOGI("device is %{public}s, preState is %{public}d, state is %{public}d, reason is %{public}d",
           GET_ENCRYPT_DEVICE_ADDR(device), preState, state, reason);
    ::ohos::nearlink::manager::PairingStateParam result = {
        .address = static_cast<::taihe::string>(device.GetDeviceAddr()),
        .preState = static_cast<::ohos::nearlink::constant::PairingState::key_t>(preState),
        .state = static_cast<::ohos::nearlink::constant::PairingState::key_t>(state),
        .reason = static_cast<::ohos::nearlink::manager::PairingReason::key_t>(reason)
    };
    std::shared_lock<std::shared_mutex> guard(g_pairStatusChangedMutex);
    for (auto callback : g_pairStatusChangedObserverVec) {
        if(callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniRemoteDeviceObserver::OnConnectionStateChanged(const NearlinkRemoteDevice &device,
    int preState, int state, int reason)
{
    HILOGI("device is %{public}s, preState is %{public}d, state is %{public}d, reason is %{public}d",
        GET_ENCRYPT_DEVICE_ADDR(device), preState, state, reason);
    ::ohos::nearlink::manager::ConnectionStateParam result = {
        .address = static_cast<::taihe::string>(device.GetDeviceAddr()),
        .preState = static_cast<::ohos::nearlink::constant::ConnectionState::key_t>(preState),
        .state = static_cast<::ohos::nearlink::constant::ConnectionState::key_t>(state),
        .connectionReason = static_cast<::ohos::nearlink::manager::ConnectionReason::key_t>(reason)
    };
    std::shared_lock<std::shared_mutex> guard(g_connectionStateChangedMutex);
    for (auto callback : g_connectionStateChangedObserverVec) {
        if(callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniRemoteDeviceObserver::OnAcbStateChanged(const NearlinkRemoteDevice &device, int state, int reason)
{
    HILOGI("device is %{public}s, state is %{public}d, reason is %{public}d",
        GET_ENCRYPT_DEVICE_ADDR(device), state, reason);
    ::ohos::nearlink::manager::AcbStateParam result = {
        .address = static_cast<::taihe::string>(device.GetDeviceAddr()),
        .state = static_cast<::ohos::nearlink::constant::AcbState::key_t>(state)
    };
    std::shared_lock<std::shared_mutex> guard(g_acbStateChangedMutex);
    for (auto callback : g_acbStateChangedObserverVec) {
        if(callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniNearlinkManager::CallbackInit()
{
    HILOGI("enter");
    NearlinkHost::GetInstance().RegisterObserver(g_aniNearlinkManagerObserver);
    NearlinkHost::GetInstance().RegisterRemoteDeviceObserver(g_aniNearlinkRemoteDeviceObserver);
}
}
}