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
#include "log.h"
#include "napi_async_work.h"
#include "napi_native_object.h"
#include "nearlink_errorcode.h"
#include "napi_nearlink_remote_device_callback.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {
NapiRemoteDeviceCallback::NapiRemoteDeviceCallback()
    : eventSubscribe({SLE_REMOTE_DEVICE_CALLBACK_PAIRING_STATE_CHANGE,
        SLE_REMOTE_DEVICE_CALLBACK_CONNECTION_STATE_CHANGE,
        SLE_REMOTE_DEVICE_CALLBACK_PAIRING_REQUEST,
        SLE_REMOTE_DEVICE_CALLBACK_ACB_STATE_CHANGE},
        NL_MODULE_NAME)
{}

void NapiRemoteDeviceCallback::OnPairingRequest(const NearlinkRemoteDevice &device,
    const std::string &passkey, int type)
{
    HILOGI("start");
    auto napiNative = std::make_shared<PairingRequestParam>(device.GetDeviceAddr(), passkey, type);
    eventSubscribe.PublishEvent(SLE_REMOTE_DEVICE_CALLBACK_PAIRING_REQUEST, napiNative);
    HILOGI("end");
}

void NapiRemoteDeviceCallback::OnPairStatusChanged(const NearlinkRemoteDevice &device,
    int preState, int state, int reason)
{
    HILOGI("start");
    int outPreState = NapiToJsPairState(preState);
    int outState = NapiToJsPairState(state);
    auto napiNative = std::make_shared<PairingStateParam>(device.GetDeviceAddr(), outPreState, outState, reason);
    eventSubscribe.PublishEvent(SLE_REMOTE_DEVICE_CALLBACK_PAIRING_STATE_CHANGE, napiNative);

    HILOGI("end");
}

void NapiRemoteDeviceCallback::OnConnectionStateChanged(const NearlinkRemoteDevice &device,
    int preState, int state, int reason)
{
    HILOGI("start");
    auto napiNative = std::make_shared<ConnectionStateParam>(device.GetDeviceAddr(), preState, state, reason);
    eventSubscribe.PublishEvent(SLE_REMOTE_DEVICE_CALLBACK_CONNECTION_STATE_CHANGE, napiNative);

    HILOGI("end");
}

void NapiRemoteDeviceCallback::OnAcbStateChanged(const NearlinkRemoteDevice &device, int state, int reason)
{
    HILOGI("enter, device: %{public}s, state: %{public}d", GET_ENCRYPT_DEVICE_ADDR(device), state);
    int outAcbState = NapiToJsAcbState(state);
    auto napiNative = std::make_shared<AcbStateParam>(device.GetDeviceAddr(), outAcbState);
    eventSubscribe.PublishEvent(SLE_REMOTE_DEVICE_CALLBACK_ACB_STATE_CHANGE, napiNative);
    HILOGI("end");
}

void NapiRemoteDeviceCallback::OnRemoteUuidChanged(const NearlinkRemoteDevice &device,
    const std::vector<UUID> &uuids)
{
    HILOGI("start");
}

void NapiRemoteDeviceCallback::OnRemoteNameChanged(const NearlinkRemoteDevice &device, const std::string &deviceName)
{
    HILOGI("start");
}

void NapiRemoteDeviceCallback::OnRemoteAliasChanged(const NearlinkRemoteDevice &device, const std::string &alias)
{
    HILOGI("start");
}

void NapiRemoteDeviceCallback::OnReadRemoteRssiEvent(const NearlinkRemoteDevice &device, int rssi, int status)
{
    HILOGI("start");
}

napi_value PairingStateParam::ToNapiValue(napi_env env) const
{
    napi_value object;
    napi_create_object(env, &object);
    napi_value value = nullptr;
    napi_create_string_utf8(env, address_.c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, object, "address", value);
    napi_create_int32(env, preState_, &value);
    napi_set_named_property(env, object, "preState", value);
    napi_create_int32(env, state_, &value);
    napi_set_named_property(env, object, "state", value);
    napi_create_int32(env, reason_, &value);
    napi_set_named_property(env, object, "reason", value);
    return object;
}

napi_value ConnectionStateParam::ToNapiValue(napi_env env) const
{
    napi_value object;
    napi_create_object(env, &object);
    napi_value value = nullptr;
    napi_create_string_utf8(env, address_.c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, object, "address", value);
    napi_create_int32(env, preState_, &value);
    napi_set_named_property(env, object, "preState", value);
    napi_create_int32(env, state_, &value);
    napi_set_named_property(env, object, "state", value);
    napi_create_int32(env, connectionReason_, &value);
    napi_set_named_property(env, object, "connectionReason", value);
    return object;
}

napi_value PairingRequestParam::ToNapiValue(napi_env env) const
{
    napi_value object;
    napi_create_object(env, &object);
    napi_value value = nullptr;
    napi_create_string_utf8(env, address_.c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, object, "address", value);
    napi_create_string_utf8(env, passKey_.c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, object, "passKey", value);
    napi_create_int32(env, pairingType_, &value);
    napi_set_named_property(env, object, "pairingType", value);
    return object;
}

napi_value AcbStateParam::ToNapiValue(napi_env env) const
{
    napi_value object;
    napi_create_object(env, &object);
    napi_value value = nullptr;
    napi_create_string_utf8(env, address_.c_str(), NAPI_AUTO_LENGTH, &value);
    napi_set_named_property(env, object, "address", value);
    napi_create_int32(env, state_, &value);
    napi_set_named_property(env, object, "state", value);
    return object;
}

napi_value PairingReasonInit(napi_env env)
{
    HILOGD("enter");
    napi_value reason = nullptr;
    napi_create_object(env, &reason);
    SetNamedPropertyByInteger(env, reason, static_cast<int>(PairingReason::PAIRING_SUCCESS),
        "PAIRING_REASON_SUCCESS");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(PairingReason::PAIRING_FAILURE),
        "PAIRING_REASON_FAILURE");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(PairingReason::PAIRING_ACB_CONNECTION_FAILED),
        "PAIRING_REASON_ACB_CONNECTION_FAIL");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(PairingReason::PAIRING_EXCEED_ACB_MAX),
        "PAIRING_REASON_EXCEED_ACB_MAX");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(PairingReason::PAIRING_REMOTE_CANCELED),
        "PAIRING_REASON_REMOTE_CANCELED");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(PairingReason::PAIRING_LOCAL_CANCELED),
        "PAIRING_REASON_LOCAL_CANCELED");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(PairingReason::PAIRING_AUTH_FAILED),
        "PAIRING_REASON_AUTH_FAIL");
    return reason;
}

napi_value PairingTypeInit(napi_env env)
{
    HILOGD("enter");
    napi_value reason = nullptr;
    napi_create_object(env, &reason);
    SetNamedPropertyByInteger(env, reason, static_cast<int>(PairingType::NO_PASSKEY_CONFIRMATION),
        "NO_PASSKEY_CONFIRMATION");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(PairingType::PAIRING_TYPE_PASSCODE),
        "PAIRING_TYPE_PASSCODE");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(PairingType::PAIRING_TYPE_NUMBER_COMPARE),
        "PAIRING_TYPE_NUMBER_COMPARE");
    return reason;
}

napi_value ConnectionReasonInit(napi_env env)
{
    HILOGD("enter");
    napi_value reason = nullptr;
    napi_create_object(env, &reason);
    SetNamedPropertyByInteger(env, reason, static_cast<int>(ConnectionReason::CONNECTION_SUCCESS),
        "CONNECTION_SUCCESS");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(ConnectionReason::CONNECTION_FAIL),
        "CONNECTION_FAILURE");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(ConnectionReason::CONNECTION_LOCAL_DISCONNECT),
        "CONNECTION_LOCAL_DISCONNECT");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(ConnectionReason::CONNECTION_REMOTE_DISCONNECT),
        "CONNECTION_REMOTE_DISCONNECT");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(ConnectionReason::CONNECTION_FAIL_ACB_CONNECTION),
        "CONNECTION_FAIL_ACB_CONNECTION");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(ConnectionReason::CONNECTION_FAIL_SERVICE_DISCOVERY),
        "CONNECTION_FAIL_SERVICE_DISCOVERY");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(ConnectionReason::CONNECTION_FAIL_NO_AVAILABLE_SERVICE),
        "CONNECTION_FAIL_NO_AVAILABLE_SERVICE");
    SetNamedPropertyByInteger(env, reason, static_cast<int>(ConnectionReason::CONNECTION_FAIL_CONNECTION_NUM_LIMITED),
        "CONNECTION_FAIL_CONNECTION_NUM_LIMITED");
    return reason;
}

} // namespace Nearlink
} // namespace OHOS
