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

#include "ani_nearlink_ssap_server_callback.h"
#include "log_util.h"
#include "ani_nearlink_ssap_utils.h"

namespace OHOS {
namespace Nearlink {

void AniSsapServerCallback::OnConnectionStateUpdate(const NearlinkRemoteDevice &device, int state, int reason)
{
    HILOGI("enter, state: %{public}d, remote device address: %{public}s reason: 0x%{public}x",
        state, GET_ENCRYPT_DEVICE_ADDR(device), reason);
    auto result = ConvertChangeStateToTaihe(device, state);
    auto connectionState = connectionStateEvent_.GetCallbacks();
    for (const auto &callback : connectionState) {
        if (callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniSsapServerCallback::OnPropertyReadRequest(
    const NearlinkRemoteDevice &device, const SsapProperty &property, int requestId)
{
    auto result = ConvertReadRequestToTaihe(device, property, requestId);
    auto propertyReadEvent = propertyReadEvent_.GetCallbacks();
    for (const auto &callback : propertyReadEvent) {
        if (callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniSsapServerCallback::OnPropertyWriteRequest(
    const NearlinkRemoteDevice &device, const SsapProperty &property, int requestId)
{
    auto result = ConvertWriteRequestToTaihe(device, property, requestId);
    auto propertyWriteEvent = propertyWriteEvent_.GetCallbacks();
    for (const auto &callback : propertyWriteEvent) {
        if (callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniSsapServerCallback::OnMtuUpdate(const NearlinkRemoteDevice &device, int mtu)
{
    auto mtuEvent = mtuChangeEvent_.GetCallbacks();
    for (const auto &callback : mtuEvent) {
        if (callback.has_value()) {
            (*callback)(mtu);
        }
    }
}
}  // namespace Nearlink
}  // namespace OHOS
