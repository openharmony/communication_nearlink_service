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
#include "napi_nearlink_remote_device.h"
#include <unistd.h>
#include "nearlink_errorcode.h"
#include "nearlink_host.h"
#include "log.h"
#include "napi_async_callback.h"
#include "napi_nearlink_error.h"
#include "napi_nearlink_utils.h"
#include "napi_parser_utils.h"
#include "nearlink_remote_device.h"
#include "napi_ha_manager.h"
#include "napi_nearlink_remote_device_rssi_observer.h"
#include "napi_nearlink_remote_device_callback.h"

namespace OHOS {
namespace Nearlink {
namespace {
    const size_t DEVICE_NAME_MAX_LENGTH = 64;
    const size_t PASS_CODE_LEN = 6;

    bool IsValidPassCode(const std::string& passCode)
    {
        if (passCode.length() != PASS_CODE_LEN) {
            return false;
        }
        for (char c : passCode) {
            if (!std::isdigit(c)) {
                return false;
            }
        }
        return true;
    }

    std::shared_ptr<NapiRemoteDeviceCallback> g_nearlinkRemoteDeviceCallback =
        std::make_shared<NapiRemoteDeviceCallback>();
}
napi_value NapiNativeDeviceModel::ToNapiValue(napi_env env) const
{
    napi_value object;
    napi_create_object(env, &object);
    DeviceModel innerModel(deviceModel_);

    napi_value modelId;
    napi_create_string_utf8(env, innerModel.GetModelId().c_str(), NAPI_AUTO_LENGTH, &modelId);
    napi_set_named_property(env, object, "modelId", modelId);

    napi_value subModelId;
    napi_create_string_utf8(env, innerModel.GetSubModelId().c_str(), NAPI_AUTO_LENGTH, &subModelId);
    napi_set_named_property(env, object, "subModelId", subModelId);

    napi_value iconId;
    napi_create_string_utf8(env, innerModel.GetIconId().c_str(), NAPI_AUTO_LENGTH, &iconId);
    napi_set_named_property(env, object, "iconId", iconId);

    return object;
}

napi_value NapiNativeDeviceInformation::ToNapiValue(napi_env env) const
{
    napi_value object;

    napi_create_object(env, &object);
    DeviceInformation innerInformation(deviceInformation_);

    napi_value manufacture;
    napi_create_string_utf8(env, deviceInformation_.GetManufacturerData().c_str(), NAPI_AUTO_LENGTH, &manufacture);
    napi_set_named_property(env, object, "manufacturerData", manufacture);

    napi_value model;
    napi_create_string_utf8(env, deviceInformation_.GetModelData().c_str(), NAPI_AUTO_LENGTH, &model);
    napi_set_named_property(env, object, "modelData", model);

    return object;
}

thread_local napi_ref NapiNearlinkRemoteDevice::consRef_ = nullptr;

void NapiNearlinkRemoteDevice::DefineRemoteDeviceJSClass(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("startPairing", StartPairing),
        DECLARE_NAPI_FUNCTION("startCrediblePairing", StartCrediblePairing),
        DECLARE_NAPI_FUNCTION("removePairedDevice", RemovePairedDevice),
        DECLARE_NAPI_FUNCTION("cancelDevicePairing", CancelDevicePairing),
        DECLARE_NAPI_FUNCTION("setPairingPasscode", SetPairingPassCode),
        DECLARE_NAPI_FUNCTION("setPairingConfirmation", SetPairingConfirmation),
        DECLARE_NAPI_FUNCTION("connect", Connect),
        DECLARE_NAPI_FUNCTION("disconnect", Disconnect),
        DECLARE_NAPI_FUNCTION("getPairingState", GetPairingState),
        DECLARE_NAPI_FUNCTION("getDeviceName", GetDeviceName),
        DECLARE_NAPI_FUNCTION("getDeviceAlias", GetDeviceAlias),
        DECLARE_NAPI_FUNCTION("setDeviceAlias", SetDeviceAlias),
        DECLARE_NAPI_FUNCTION("getDeviceClass", GetDeviceClass),
        DECLARE_NAPI_FUNCTION("getConnectionState", GetConnectionState),
        DECLARE_NAPI_FUNCTION("getAcbState", GetAcbState),
        DECLARE_NAPI_FUNCTION("getDeviceModel", GetDeviceModel),
        DECLARE_NAPI_FUNCTION("getDeviceInformation", GetDeviceInformation),
        DECLARE_NAPI_FUNCTION("setConnectionInterval", SetConnectionInterval),
        DECLARE_NAPI_FUNCTION("getRssiValue", GetRssiValue),
    };

    napi_value constructor = nullptr;
    napi_define_class(env, "RemoteDevice", NAPI_AUTO_LENGTH, RemoteDeviceConstructor, nullptr,
        sizeof(properties) / sizeof(properties[0]), properties, &constructor);
    napi_create_reference(env, constructor, 1, &consRef_);

    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("createRemoteDevice", CreateRemoteDevice),
        DECLARE_NAPI_FUNCTION("onPairingRequest", OnPairingRequest),
        DECLARE_NAPI_FUNCTION("offPairingRequest", OffPairingRequest),
        DECLARE_NAPI_FUNCTION("onPairingStateChange", OnPairingStateChange),
        DECLARE_NAPI_FUNCTION("offPairingStateChange", OffPairingStateChange),
        DECLARE_NAPI_FUNCTION("onConnectionStateChange", OnConnectionStateChange),
        DECLARE_NAPI_FUNCTION("offConnectionStateChange", OffConnectionStateChange),
        DECLARE_NAPI_FUNCTION("onAcbStateChange", OnAcbStateChange),
        DECLARE_NAPI_FUNCTION("offAcbStateChange", OffAcbStateChange),
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}

napi_value NapiNearlinkRemoteDevice::OnPairingRequest(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.RegisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_PAIRING_REQUEST);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    HILOGD("end");
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkRemoteDevice::OffPairingRequest(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.DeregisterWithName(env, info,
        SLE_REMOTE_DEVICE_CALLBACK_PAIRING_REQUEST);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkRemoteDevice::OnPairingStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.RegisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_PAIRING_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    HILOGD("end");
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkRemoteDevice::OffPairingStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.DeregisterWithName(env, info,
        SLE_REMOTE_DEVICE_CALLBACK_PAIRING_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkRemoteDevice::OnConnectionStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.RegisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_CONNECTION_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    HILOGD("end");
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkRemoteDevice::OffConnectionStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.DeregisterWithName(env, info,
        SLE_REMOTE_DEVICE_CALLBACK_CONNECTION_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkRemoteDevice::OnAcbStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.RegisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_ACB_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    HILOGD("end");
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkRemoteDevice::OffAcbStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.DeregisterWithName(env, info,
        SLE_REMOTE_DEVICE_CALLBACK_ACB_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

static napi_status CheckCreateRemoteDeviceParams(napi_env env, napi_callback_info info, napi_value &outResult)
{
    HILOGI("start");
    size_t expectedArgsCount = ARGS_SIZE_ONE;
    size_t argc = expectedArgsCount;
    napi_value argv[ARGS_SIZE_ONE] = {0};

    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    NAPI_NL_RETURN_IF(argc != expectedArgsCount, "expect 1 args", napi_invalid_arg);

    std::string deviceId {};
    if (!ParseString(env, deviceId, argv[0])) {
        HILOGE("expect string");
        return napi_string_expected;
    }
    if (!IsValidAddress(deviceId)) {
        HILOGE("Invalid deviceId");
        HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_ADDRESS);
        return napi_invalid_arg;
    }

    napi_value constructor = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_reference_value(env, NapiNearlinkRemoteDevice::consRef_, &constructor));
    NAPI_NL_CALL_RETURN(napi_new_instance(env, constructor, argc, argv, &outResult));
    return napi_ok;
}

napi_value NapiNearlinkRemoteDevice::CreateRemoteDevice(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    napi_value result;
    auto status = CheckCreateRemoteDeviceParams(env, info, result);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return result;
}

napi_value NapiNearlinkRemoteDevice::RemoteDeviceConstructor(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    napi_value thisVar = nullptr;
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};

    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);

    std::string deviceId;
    if (!ParseString(env, deviceId, argv[PARAM0])) {
        HILOGE("RemoteDeviceConstructor ParseString failed, deviceId is invalid");
        return nullptr;
    }
    NapiNearlinkRemoteDevice *remoteDevice = new (std::nothrow) NapiNearlinkRemoteDevice(deviceId);
    if (remoteDevice == nullptr) {
        HILOGE("remoteDevice is nullptr");
        return nullptr;
    }

    auto finalize = [](napi_env env, void* data, void* hint) {
        NapiNearlinkRemoteDevice* device = static_cast<NapiNearlinkRemoteDevice*>(data);
        if (device != nullptr) {
            delete device;
            device = nullptr;
        }
    };

    napi_status status = napi_wrap(env, thisVar, remoteDevice, finalize, nullptr, nullptr);
    if (status != napi_ok) {
        HILOGE("napi_wrap failed");
        delete remoteDevice;
        remoteDevice = nullptr;
        return nullptr;
    }
    HILOGI("Constructor remoteDevice success.");
    return thisVar;
}

static NapiNearlinkRemoteDevice *NapiGetRemoteDevice(napi_env env, napi_value thisVar)
{
    NapiNearlinkRemoteDevice *remoteDevice = nullptr;
    auto status = napi_unwrap(env, thisVar, reinterpret_cast<void **>(&remoteDevice));
    if (status != napi_ok) {
        return nullptr;
    }
    return remoteDevice;
}

static napi_status CheckRemoteDeviceNoArgc(napi_env env, napi_callback_info info,
    NapiNearlinkRemoteDevice **outRemoteDevice)
{
    size_t argc = ARGS_SIZE_ZERO;
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, nullptr, &thisVar, nullptr));
    NAPI_NL_RETURN_IF(argc != 0, "No need arguments.", napi_invalid_arg);
    NapiNearlinkRemoteDevice *remoteDevice = NapiGetRemoteDevice(env, thisVar);
    NAPI_NL_RETURN_IF(remoteDevice == nullptr || outRemoteDevice == nullptr,
        "remoteDevice is nullptr.", napi_invalid_arg);

    *outRemoteDevice = remoteDevice;
    return napi_ok;
}

napi_value NapiNearlinkRemoteDevice::StartPairing(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("startPairing", beginTime, true);
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    auto func = [device = remoteDevice->GetDevice()]() {
        if (device == nullptr) {
            HILOGE("nearlinkRemoteDevice is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        NlErrCode err = device->StartPair();
        HILOGI("ret: %{public}d", err);
        return NapiAsyncWorkRet(err);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkRemoteDevice::StartCrediblePairing(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("StartCrediblePairing", 0, false);
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    auto func = [device = remoteDevice->GetDevice()]() {
        if (device == nullptr) {
            HILOGE("nearlinkRemoteDevice is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        NlErrCode err = device->StartCrediblePair();
        HILOGI("ret: %{public}d", err);
        return NapiAsyncWorkRet(err);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkRemoteDevice::GetPairingState(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    std::shared_ptr<NearlinkRemoteDevice> device = remoteDevice->GetDevice();
    NAPI_NL_ASSERT_RETURN_FALSE(env, device != nullptr, NL_ERR_INTERNAL_ERROR);

    int state;
    NlErrCode err = device->GetPairState(state);
    int outstate = NapiToJsPairState(state);
    HILOGI("state: %{public}d", outstate);
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);
    napi_value ret = nullptr;
    napi_create_int32(env, state, &ret);
    return ret;
}

napi_value NapiNearlinkRemoteDevice::RemovePairedDevice(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context =
        std::make_shared<AsyncWorkContext>("removePairedDevice", beginTime, true);
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    auto func = [device = remoteDevice->GetDevice()]() {
        if (device == nullptr) {
            HILOGE("nearlinkRemoteDevice is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        int32_t ret = NearlinkHost::GetInstance().RemovePair(*device);
        HILOGI("ret: %{public}d", ret);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkRemoteDevice::CancelDevicePairing(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("CancelDevicePairing", 0, false);
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    auto func = [device = remoteDevice->GetDevice()]() {
        if (device == nullptr) {
            HILOGE("nearlinkRemoteDevice is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        NlErrCode err = device->CancelDevicePairing();
        HILOGI("ret: %{public}d", err);
        return NapiAsyncWorkRet(err);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkRemoteDevice::SetPairingPassCode(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("SetPairingPassCode", 0, false);
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE];
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    NapiNearlinkRemoteDevice* remoteDevice = NapiGetRemoteDevice(env, thisVar);
    NAPI_NL_ASSERT_RETURN_FALSE(env, remoteDevice != nullptr, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_FALSE(env, argc == ARGS_SIZE_ONE, NL_ERR_INVALID_PARAM);
    std::string passCode = "";
    NAPI_NL_ASSERT_RETURN_FALSE(env, ParseString(env, passCode, argv[0]), NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_FALSE(env, IsValidPassCode(passCode), NL_ERR_INVALID_PASSCODE);
    auto func = [device = remoteDevice->GetDevice(), passCode]() -> NapiAsyncWorkRet {
        if (device == nullptr) {
            HILOGE("nearlinkRemoteDevice is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        NlErrCode ret = device->SetPairingPassCode(passCode);
        HILOGI("ret: %{public}d", ret);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkRemoteDevice::SetPairingConfirmation(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE];
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    NapiNearlinkRemoteDevice* remoteDevice = NapiGetRemoteDevice(env, thisVar);
    NAPI_NL_ASSERT_RETURN_FALSE(env, remoteDevice != nullptr, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_FALSE(env, argc == ARGS_SIZE_ONE, NL_ERR_INVALID_PARAM);

    std::shared_ptr<NearlinkRemoteDevice> device = remoteDevice->GetDevice();
    NAPI_NL_ASSERT_RETURN_FALSE(env, device != nullptr, NL_ERR_INTERNAL_ERROR);
    bool cfm;
    NAPI_NL_ASSERT_RETURN_FALSE(env, ParseBool(env, cfm, argv[0]), NL_ERR_INVALID_PARAM);
    NlErrCode ret = device->SetPairingConfirmation(cfm);
    NAPI_NL_ASSERT_RETURN_FALSE(env, ret == NL_NO_ERROR, ret);

    napi_value value = nullptr;
    napi_create_int32(env, ret, &value);
    return value;
}

napi_value NapiNearlinkRemoteDevice::Connect(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context =
        std::make_shared<AsyncWorkContext>("RemoteDevice.connect", beginTime, true);
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    auto func = [device = remoteDevice->GetDevice()]() {
        if (device == nullptr) {
            HILOGE("nearlinkRemoteDevice is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        NlErrCode ret = NearlinkHost::GetInstance().ConnectAllowedProfiles(device->GetDeviceAddr());
        HILOGI("ret: %{public}d", ret);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkRemoteDevice::Disconnect(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context =
        std::make_shared<AsyncWorkContext>("RemoteDevice.disconnect", beginTime, true);
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    auto func = [device = remoteDevice->GetDevice()]() {
        if (device == nullptr) {
            HILOGE("nearlinkRemoteDevice is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        NlErrCode ret = NearlinkHost::GetInstance().DisconnectAllowedProfiles(device->GetDeviceAddr());
        HILOGI("ret: %{public}d", ret);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkRemoteDevice::GetDeviceName(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    std::shared_ptr<NearlinkRemoteDevice> device = remoteDevice->GetDevice();
    NAPI_NL_ASSERT_RETURN_FALSE(env, device != nullptr, NL_ERR_INTERNAL_ERROR);
    std::string name;
    NlErrCode err = device->GetDeviceName(name);
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);

    napi_value value = nullptr;
    napi_create_string_utf8(env, name.c_str(), NAPI_AUTO_LENGTH, &value);
    return value;
}

napi_value NapiNearlinkRemoteDevice::GetDeviceAlias(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    std::shared_ptr<NearlinkRemoteDevice> device = remoteDevice->GetDevice();
    NAPI_NL_ASSERT_RETURN_FALSE(env, device != nullptr, NL_ERR_INTERNAL_ERROR);
    std::string name;
    NlErrCode err = device->GetDeviceAlias(name);
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);

    napi_value value = nullptr;
    napi_create_string_utf8(env, name.c_str(), NAPI_AUTO_LENGTH, &value);
    return value;
}

napi_value NapiNearlinkRemoteDevice::SetDeviceAlias(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE];
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    NAPI_NL_ASSERT_RETURN_FALSE(env, argc == ARGS_SIZE_ONE, NL_ERR_INVALID_PARAM);
    NapiNearlinkRemoteDevice* remoteDevice = NapiGetRemoteDevice(env, thisVar);
    NAPI_NL_ASSERT_RETURN_FALSE(env, remoteDevice != nullptr, NL_ERR_INVALID_PARAM);

    std::shared_ptr<NearlinkRemoteDevice> device = remoteDevice->GetDevice();
    NAPI_NL_ASSERT_RETURN_FALSE(env, device != nullptr, NL_ERR_INVALID_PARAM);
    std::string deviceName;
    NAPI_NL_ASSERT_RETURN_FALSE(env, ParseString(env, deviceName, argv[0]), NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_FALSE(env, !deviceName.empty(), NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_FALSE(env, deviceName.length() <= DEVICE_NAME_MAX_LENGTH, NL_ERR_STRING_LENGTH_LIMITED);
    NlErrCode ret = device->SetDeviceAlias(deviceName);
    NAPI_NL_ASSERT_RETURN_FALSE(env, ret == NL_NO_ERROR, ret);
    HILOGI("ret: %{public}d", static_cast<int>(ret));

    napi_value value = nullptr;
    napi_create_int32(env, ret, &value);
    return value;
}

napi_value NapiNearlinkRemoteDevice::GetDeviceClass(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    std::shared_ptr<NearlinkRemoteDevice> device = remoteDevice->GetDevice();
    NAPI_NL_ASSERT_RETURN_FALSE(env, device != nullptr, NL_ERR_INTERNAL_ERROR);
    int deviceAppearance;
    NlErrCode err = device->GetDeviceAppearance(deviceAppearance);
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);
    HILOGI("deviceAppearance: %{public}d", deviceAppearance);

    napi_value value = nullptr;
    napi_create_int32(env, deviceAppearance, &value);
    return value;
}

napi_value NapiNearlinkRemoteDevice::GetConnectionState(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    std::shared_ptr<NearlinkRemoteDevice> device = remoteDevice->GetDevice();
    NAPI_NL_ASSERT_RETURN_FALSE(env, device != nullptr, NL_ERR_INTERNAL_ERROR);
    int connState;
    NlErrCode err = NearlinkHost::GetInstance().GetProfileConnState(device->GetDeviceAddr(), connState);
    HILOGI("errCode: %{public}d", err);
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);
    napi_value value = nullptr;
    napi_create_int32(env, connState, &value);
    return value;
}

napi_value NapiNearlinkRemoteDevice::GetAcbState(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);
    std::shared_ptr<NearlinkRemoteDevice> device = remoteDevice->GetDevice();
    NAPI_NL_ASSERT_RETURN_FALSE(env, device != nullptr, NL_ERR_INTERNAL_ERROR);

    int acbState;
    NlErrCode err = device->GetAcbState(acbState);
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);
    int outAcbState = NapiToJsAcbState(acbState);
    HILOGI("acbState: %{public}d", outAcbState);

    napi_value value = nullptr;
    napi_create_int32(env, outAcbState, &value);
    return value;
}

napi_value NapiNearlinkRemoteDevice::GetDeviceModel(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);
    std::shared_ptr<NearlinkRemoteDevice> device = remoteDevice->GetDevice();
    NAPI_NL_ASSERT_RETURN_FALSE(env, device != nullptr, NL_ERR_INTERNAL_ERROR);

    DeviceModel model;
    NlErrCode err = device->GetDeviceModel(model);
    HILOGI("GetDeviceModel model=[modelId=%{public}s, subModelId=%{public}s, iconId=%{public}s]",
           model.GetModelId().c_str(), model.GetSubModelId().c_str(), model.GetIconId().c_str());
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);

    auto object = std::make_shared<NapiNativeDeviceModel>(model);
    return object->ToNapiValue(env);
}

napi_value NapiNearlinkRemoteDevice::GetDeviceInformation(napi_env env, napi_callback_info info)
{
    HILOGI("enter");
    NapiNearlinkRemoteDevice* remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);

    std::shared_ptr<NearlinkRemoteDevice> device = remoteDevice->GetDevice();
    NAPI_NL_ASSERT_RETURN_FALSE(env, device != nullptr, NL_ERR_INTERNAL_ERROR);

    DeviceInformation information;

    NlErrCode err = device->GetDeviceInformation(information);
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);

    HILOGI("ManufacturerData=%{public}s, ModelData=%{public}s]",
        information.GetManufacturerData().c_str(), information.GetModelData().c_str());

    auto object = std::make_shared<NapiNativeDeviceInformation>(information);
    return object->ToNapiValue(env);

}

napi_value NapiNearlinkRemoteDevice::SetConnectionInterval(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE];
    napi_value thisVar = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    NAPI_NL_ASSERT_RETURN_FALSE(env, argc == ARGS_SIZE_ONE, NL_ERR_INVALID_PARAM);
    NapiNearlinkRemoteDevice *remoteDevice = NapiGetRemoteDevice(env, thisVar);
    NAPI_NL_ASSERT_RETURN_FALSE(env, remoteDevice != nullptr, NL_ERR_INVALID_PARAM);

    std::shared_ptr<NearlinkRemoteDevice> device = remoteDevice->GetDevice();
    NAPI_NL_ASSERT_RETURN_FALSE(env, device != nullptr, NL_ERR_INVALID_PARAM);
    int32_t intervalType;
    NAPI_NL_ASSERT_RETURN_FALSE(env, ParseInt32(env, intervalType, argv[PARAM0]), NL_ERR_INVALID_PARAM);
    HILOGI("intervalType: %{public}d", intervalType);
    NlErrCode err = device->UpdateConnectInterval(static_cast<ConnectionInterval>(intervalType));
    NAPI_NL_ASSERT_RETURN_FALSE(env, err == NL_NO_ERROR, err);
    NapiHaManager::GetInstance().ReportEvent("SetConnectionInterval", beginTime, NL_NO_ERROR);
    napi_value value = nullptr;
    napi_create_int32(env, err, &value);
    return value;
}

napi_value NapiNearlinkRemoteDevice::GetRssiValue(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    std::shared_ptr<AsyncWorkContext> context =
            std::make_shared<AsyncWorkContext>("RemoteDevice.getRssiValue", beginTime, true);
    NapiNearlinkRemoteDevice *remoteDevice = nullptr;
    auto status = CheckRemoteDeviceNoArgc(env, info, &remoteDevice);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_FALSE(env, status == napi_ok, NL_ERR_INTERNAL_ERROR);
    NearlinkHost::GetInstance().RegisterRssiObserver(NapiRemoteDeviceRssiObserver::GetInstance());

    auto func = [device = remoteDevice->GetDevice()]() {
        if (device == nullptr) {
            HILOGE("nearlinkRemoteDevice is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        NlErrCode err = device->ReadRemoteRssiValue();
        return NapiAsyncWorkRet(err);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    bool success = NapiRemoteDeviceRssiObserver::GetInstance()->asyncPromiseMap_.TryPush(
            NapiAsyncType::GET_REMOTE_DEVICE_RSSI, asyncWork);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, success, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value OnPairingRequest(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.RegisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_PAIRING_REQUEST);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value OffPairingRequest(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.DeregisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_PAIRING_REQUEST);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value OnPairingStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.RegisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_PAIRING_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value OffPairingStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.DeregisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_PAIRING_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value OnConnectionStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.RegisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_CONNECTION_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value OffConnectionStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.DeregisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_CONNECTION_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value OnAcbStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.RegisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_ACB_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value OffAcbStateChange(napi_env env, napi_callback_info info)
{
    HILOGI("start");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = g_nearlinkRemoteDeviceCallback->eventSubscribe.DeregisterWithName(
        env, info, SLE_REMOTE_DEVICE_CALLBACK_ACB_STATE_CHANGE);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

EXTERN_C_START
/*
 * Module initialization function
 */
static napi_value Init(napi_env env, napi_value exports)
{
    HILOGI("-----RemoteDevice Init start------");

    NearlinkHost::GetInstance().RegisterRemoteDeviceObserver(g_nearlinkRemoteDeviceCallback);
    NapiNearlinkRemoteDevice::DefineRemoteDeviceJSClass(env, exports);

    napi_value pairingReason = PairingReasonInit(env);
    napi_value pairingType = PairingTypeInit(env);
    napi_value connectionReason = ConnectionReasonInit(env);
    napi_property_descriptor exportProperty[] = {
        DECLARE_NAPI_PROPERTY("PairingReason", pairingReason),
        DECLARE_NAPI_PROPERTY("PairingType", pairingType),
        DECLARE_NAPI_PROPERTY("ConnectionReason", connectionReason),
    };
    napi_define_properties(env, exports, sizeof(exportProperty) / sizeof(*exportProperty), exportProperty);

    HILOGI("-----RemoteDevice Init end------");
    return exports;
}
EXTERN_C_END

/*
 * Module define
 */
static napi_module nearlinkRemoteDeviceModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "nearlink.remoteDevice",
    .nm_priv = ((void *)0),
    .reserved = {0}};

/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    HILOGI("Register nearlinkRemoteDeviceModule nm_modname:%{public}s", nearlinkRemoteDeviceModule.nm_modname);
    napi_module_register(&nearlinkRemoteDeviceModule);
}
} // namespace Nearlink
} // namespace OHOS