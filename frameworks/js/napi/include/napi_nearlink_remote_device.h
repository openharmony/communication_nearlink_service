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
#ifndef NAPI_NEARLINK_REMOTE_DEVICE_H_
#define NAPI_NEARLINK_REMOTE_DEVICE_H_

#include "log.h"
#include "napi_native_object.h"
#include "nearlink_remote_device.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "nearlink_device_model.h"
#include "nearlink_device_information.h"

namespace OHOS {
namespace Nearlink {
class NapiNativeDeviceModel : public NapiNativeObject {
public:
    explicit NapiNativeDeviceModel(DeviceModel &model) : deviceModel_(model) {}
    ~NapiNativeDeviceModel() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    DeviceModel deviceModel_;
};

class NapiNativeDeviceInformation : public NapiNativeObject {
public:
    explicit NapiNativeDeviceInformation(const DeviceInformation &information) : deviceInformation_(information) {}
    ~NapiNativeDeviceInformation() override = default;

    napi_value ToNapiValue(napi_env env) const override;
private:
    DeviceInformation deviceInformation_;
};

class NapiNearlinkRemoteDevice {
public:
    static void DefineRemoteDeviceJSClass(napi_env env, napi_value exports);
    static napi_value CreateRemoteDevice(napi_env env, napi_callback_info info);
    static napi_value RemoteDeviceConstructor(napi_env env, napi_callback_info info);

    static napi_value StartPairing(napi_env env, napi_callback_info info);
    static napi_value StartCrediblePairing(napi_env env, napi_callback_info info);
    static napi_value RemovePairedDevice(napi_env env, napi_callback_info info);
    static napi_value CancelDevicePairing(napi_env env, napi_callback_info info);
    static napi_value SetPairingPassCode(napi_env env, napi_callback_info info);
    static napi_value SetPairingConfirmation(napi_env env, napi_callback_info info);
    static napi_value Connect(napi_env env, napi_callback_info info);
    static napi_value Disconnect(napi_env env, napi_callback_info info);
    static napi_value GetPairingState(napi_env env, napi_callback_info info);
    static napi_value GetDeviceName(napi_env env, napi_callback_info info);
    static napi_value GetDeviceAlias(napi_env env, napi_callback_info info);
    static napi_value SetDeviceAlias(napi_env env, napi_callback_info info);
    static napi_value GetDeviceClass(napi_env env, napi_callback_info info);
    static napi_value GetConnectionState(napi_env env, napi_callback_info info);
    static napi_value GetAcbState(napi_env env, napi_callback_info info);
    static napi_value GetDeviceModel(napi_env env, napi_callback_info info);
    static napi_value GetDeviceInformation(napi_env env, napi_callback_info info);
    static napi_value SetConnectionInterval(napi_env env, napi_callback_info info);
    static napi_value GetRssiValue(napi_env env, napi_callback_info info);
    static napi_value OnPairingRequest(napi_env env, napi_callback_info info);
    static napi_value OffPairingRequest(napi_env env, napi_callback_info info);
    static napi_value OnPairingStateChange(napi_env env, napi_callback_info info);
    static napi_value OffPairingStateChange(napi_env env, napi_callback_info info);
    static napi_value OnConnectionStateChange(napi_env env, napi_callback_info info);
    static napi_value OffConnectionStateChange(napi_env env, napi_callback_info info);
    static napi_value OnAcbStateChange(napi_env env, napi_callback_info info);
    static napi_value OffAcbStateChange(napi_env env, napi_callback_info info);

    std::shared_ptr<NearlinkRemoteDevice> GetDevice()
    {
        return device_;
    }

    explicit NapiNearlinkRemoteDevice(std::string &deviceId)
    {
        device_ = std::make_shared<NearlinkRemoteDevice>(deviceId, 1);
    }
    ~NapiNearlinkRemoteDevice() = default;

    static thread_local napi_ref consRef_;

private:
    std::shared_ptr<NearlinkRemoteDevice> device_ = nullptr;
};
} // namespace Nearlink
} // namespace OHOS
#endif /* NAPI_NEARLINK_REMOTE_DEVICE_H_ */