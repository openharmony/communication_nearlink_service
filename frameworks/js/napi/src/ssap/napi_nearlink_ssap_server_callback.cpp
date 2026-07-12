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
#include <uv.h>
#include "log_util.h"
#include "napi_nearlink_ssap_server.h"
#include "napi_nearlink_ssap_server_callback.h"

namespace OHOS {
namespace Nearlink {

NapiNearlinkSsapServerCallback::NapiNearlinkSsapServerCallback()
    : eventSubscribe({SLE_SSAP_SERVER_CALLBACK_CONNECT_STATE_CHANGE,
        SLE_SSAP_SERVER_CALLBACK_PROPERTY_WRITE,
        SLE_SSAP_SERVER_CALLBACK_PROPERTY_READ,
        SLE_SSAP_SERVER_CALLBACK_MTU_CHANGE},
        NL_MODULE_NAME)
{}

void NapiNearlinkSsapServerCallback::OnConnectionStateUpdate(const NearlinkRemoteDevice &device, int state, int reason)
{
    HILOGI("enter, state: %{public}d, remote device address: %{public}s reason: 0x%{public}x",
        state, GET_ENCRYPT_DEVICE_ADDR(device), reason);
    auto napiNative = std::make_shared<NapiNativeSsapConnectionState>(device.GetDeviceAddr(), state);
    eventSubscribe.PublishEvent(SLE_SSAP_SERVER_CALLBACK_CONNECT_STATE_CHANGE, napiNative);
}

void NapiNearlinkSsapServerCallback::OnPropertyReadRequest(
    const NearlinkRemoteDevice &device, const SsapProperty &property, int ret)
{
    SsapProperty ssapProperty(property);
    auto napiNative = std::make_shared<NapiNativeSsapPropertyReadRequest>(device.GetDeviceAddr(), ssapProperty, ret);
    eventSubscribe.PublishEvent(SLE_SSAP_SERVER_CALLBACK_PROPERTY_READ, napiNative);
}

void NapiNearlinkSsapServerCallback::OnPropertyWriteRequest(
    const NearlinkRemoteDevice &device, const SsapProperty &property, int ret)
{
    SsapProperty ssapProperty(property);
    auto napiNative = std::make_shared<NapiNativeSsapPropertyWriteRequest>(
        device.GetDeviceAddr(), ssapProperty, ret, property.GetWriteType());
    eventSubscribe.PublishEvent(SLE_SSAP_SERVER_CALLBACK_PROPERTY_WRITE, napiNative);
}

void NapiNearlinkSsapServerCallback::OnMtuUpdate(const NearlinkRemoteDevice &device, int mtu)
{
    auto napiNative = std::make_shared<NapiNativeInt>(mtu);
    eventSubscribe.PublishEvent(SLE_SSAP_SERVER_CALLBACK_MTU_CHANGE, napiNative);
}
} // namespace Nearlink
} // namespace OHOS
