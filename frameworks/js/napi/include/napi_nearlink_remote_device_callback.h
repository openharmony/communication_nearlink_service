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
#ifndef NAPI_NEARLINK_REMOTE_DEVICE_CALLBACK_H_
#define NAPI_NEARLINK_REMOTE_DEVICE_CALLBACK_H_

#include <shared_mutex>
#include "nearlink_host.h"
#include "napi_async_callback.h"
#include "napi_nearlink_utils.h"
#include "nearlink_safe_map.h"
#include "napi_event_subscribe_module.h"
namespace OHOS {
namespace Nearlink {
const char *const SLE_REMOTE_DEVICE_CALLBACK_PAIRING_REQUEST = "pairingRequest";
const char *const SLE_REMOTE_DEVICE_CALLBACK_PAIRING_STATE_CHANGE = "pairingStateChange";
const char *const SLE_REMOTE_DEVICE_CALLBACK_CONNECTION_STATE_CHANGE = "connectionStateChange";
const char *const SLE_REMOTE_DEVICE_CALLBACK_ACB_STATE_CHANGE = "acbStateChange";
const char *const SLE_REMOTE_DEVICE_CALLBACK_REMOTE_UUID_CHANGED = "remoteUuidChanged";
const char *const SLE_REMOTE_DEVICE_CALLBACK_REMOTE_NAME_CHANGED = "remoteNameChanged";
const char *const SLE_REMOTE_DEVICE_CALLBACK_REMOTE_ALIAS_CHANGED = "remoteAliasChanged";
const char *const SLE_REMOTE_DEVICE_CALLBACK_READ_REMOTE_RSSIEVENT = "readRemoteRssievent";

class NapiRemoteDeviceCallback : public NearlinkRemoteDeviceObserver {
public:
    void OnPairingRequest(const NearlinkRemoteDevice &device, const std::string &passkey, int type) override;
    void OnPairStatusChanged(const NearlinkRemoteDevice &device, int preState, int state, int reason) override;
    void OnConnectionStateChanged(const NearlinkRemoteDevice &device, int preState, int state, int reason) override;
    void OnAcbStateChanged(const NearlinkRemoteDevice &device, int state, int reason) override;
    void OnRemoteUuidChanged(const NearlinkRemoteDevice &device, const std::vector<UUID> &uuids) override;
    void OnRemoteNameChanged(const NearlinkRemoteDevice &device, const std::string &deviceName) override;
    void OnRemoteAliasChanged(const NearlinkRemoteDevice &device, const std::string &alias) override;
    void OnReadRemoteRssiEvent(const NearlinkRemoteDevice &device, int rssi, int status) override;

    NapiRemoteDeviceCallback();
    ~NapiRemoteDeviceCallback() override = default;

    NapiEventSubscribeModule eventSubscribe;
};

class PairingStateParam : public NapiNativeObject {
public:
    PairingStateParam(std::string address, int preState, int state, int reason)
        : address_(address),
          preState_(preState),
          state_(state),
          reason_(reason)
    {
    }
    ~PairingStateParam() override = default;

    napi_value ToNapiValue(napi_env env) const override;

private:
    std::string address_;
    int preState_;
    int state_;
    int reason_;
};

class ConnectionStateParam : public NapiNativeObject {
public:
    ConnectionStateParam(std::string address, int preState, int state, int connectionReason)
        : address_(address),
          preState_(preState),
          state_(state),
          connectionReason_(connectionReason)
    {
    }
    ~ConnectionStateParam() override = default;

    napi_value ToNapiValue(napi_env env) const override;

private:
    std::string address_;
    int preState_;
    int state_;
    int connectionReason_;
};

class PairingRequestParam : public NapiNativeObject {
public:
    PairingRequestParam(std::string address, std::string passKey, int pairingType)
        : address_(address),
          passKey_(passKey),
          pairingType_(pairingType)
    {
    }
    ~PairingRequestParam() override = default;

    napi_value ToNapiValue(napi_env env) const override;

private:
    std::string address_;
    std::string passKey_;
    int pairingType_;
};

class AcbStateParam : public NapiNativeObject {
public:
    AcbStateParam(std::string address, int state)
        : address_(address),
          state_(state)
    {
    }
    ~AcbStateParam() override = default;

    napi_value ToNapiValue(napi_env env) const override;

private:
    std::string address_;
    int state_;
};

enum class PairingReason {
    PAIRING_SUCCESS = 0,
    PAIRING_FAILURE = 1,
    PAIRING_ACB_CONNECTION_FAILED = 2,
    PAIRING_EXCEED_ACB_MAX = 3,
    PAIRING_REMOTE_CANCELED = 4,
    PAIRING_LOCAL_CANCELED = 5,
    PAIRING_AUTH_FAILED = 6,
    PAIRING_INVALID_REASON = 0xFF
};

enum class PairingType {
    NO_PASSKEY_CONFIRMATION = 0,             // Without PASSKEY, the user needs to accept or reject the pairing request.
    PAIRING_TYPE_PASSCODE = 1,               // The user needs to enter the passcode displayed on the peer device.
    PAIRING_TYPE_NUMBER_COMPARE = 2          // The user needs to compare the number displayed on both devices.
};

enum class ConnectionReason {
    CONNECTION_NONE = -1,
    CONNECTION_SUCCESS = 0,
    CONNECTION_FAIL,
    CONNECTION_LOCAL_DISCONNECT,
    CONNECTION_REMOTE_DISCONNECT,
    CONNECTION_FAIL_ACB_CONNECTION,
    CONNECTION_FAIL_SERVICE_DISCOVERY,
    CONNECTION_FAIL_NO_AVAILABLE_SERVICE,
    CONNECTION_FAIL_CONNECTION_NUM_LIMITED
};

enum class ConnectionMode {
    SLE_MODE_UNCONNECTABLE = 0,             // Indicates that the device is not connectable.
    SLE_MODE_CONNECTABLE = 1                // Indicates that the device is connectable.
};

napi_value PairingReasonInit(napi_env env);
napi_value PairingTypeInit(napi_env env);
napi_value ConnectionReasonInit(napi_env env);

}  // namespace Nearlink
}  // namespace OHOS
#endif /* NAPI_NEARLINK_SSAP_CLIENT_CALLBACK_H_ */