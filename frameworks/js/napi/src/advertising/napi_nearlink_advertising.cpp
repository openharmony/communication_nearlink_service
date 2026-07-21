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

#include <memory>
#include "napi_nearlink_advertising_callback.h"
#include "napi_nearlink_error.h"
#include "napi_nearlink_utils.h"
#include "nearlink_sle_advertiser.h"
#include "nearlink_errorcode.h"
#include "log_util.h"
#include "napi_parser_utils.h"
#include "napi_nearlink_advertising.h"
#include "nearlink_host.h"
#include "napi_ha_manager.h"
namespace OHOS {
namespace Nearlink {

constexpr size_t MANUFACTURER_DATA_MAX_NUM = 0xFFFF;
constexpr size_t SERVICE_DATA_MAX_NUM = 0xFFFF;
constexpr size_t SERVICE_UUID_MAX_NUM = 0xFFFF;

namespace {
std::shared_ptr<SleAdvertiser> SleAdvertiserGetInstance(void)
{
    static std::shared_ptr<SleAdvertiser> instance = SleAdvertiser::CreateSleAdvertiser();
    return instance;
}
}  // namespace {}

napi_value NapiNearlinkAdvertising::OnAdvertisingStateChange(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = NapiNearlinkAdvertiseCallback::GetInstance()->eventSubscribe.RegisterWithName(
        env, info, REGISTER_ADVERTISING_STATE_INFO_NAME);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkAdvertising::OffAdvertisingStateChange(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = NapiNearlinkAdvertiseCallback::GetInstance()->eventSubscribe.DeregisterWithName(
        env, info, REGISTER_ADVERTISING_STATE_INFO_NAME);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

void NapiNearlinkAdvertising::DefineAdvertisingJSObject(napi_env env, napi_value exports)
{
    HILOGD("enter");
    PropertyInit(env, exports);
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("startAdvertising", StartAdvertising),
        DECLARE_NAPI_FUNCTION("stopAdvertising", StopAdvertising),
        DECLARE_NAPI_FUNCTION("onAdvertisingStateChange", OnAdvertisingStateChange),
        DECLARE_NAPI_FUNCTION("offAdvertisingStateChange", OffAdvertisingStateChange),
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}

static uint8_t ConvertPowerMode(int32_t powerParam)
{
    uint8_t txPowerMode = static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_LOW);
    switch (powerParam) {
        case static_cast<int32_t>(AdevertiseMode::ADV_TX_POWER_LOW):
            txPowerMode = static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_LOW);
            break;
        case static_cast<int32_t>(AdevertiseMode::ADV_TX_POWER_MEDIUM):
            txPowerMode = static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_MEDIUM);
            break;
        case static_cast<int32_t>(AdevertiseMode::ADV_TX_POWER_HIGH):
            txPowerMode = static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_HIGH);
            break;
        default:
            break;
    }
    return txPowerMode;
}

static napi_status ParseAdvertisingSettingsParameters(
    const napi_env &env, const napi_value &object, SleAdvertiserSettings &outSettings)
{
    HILOGD("enter");
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object,
        {"interval", "power", "isConnectable", "frameType"}));

    bool exist = false;
    uint32_t intervalInSlot = 0;
    NAPI_NL_CALL_RETURN(NapiParseObjectUint32Optional(env, object, "interval", intervalInSlot, exist));
    if (exist) {
        if (intervalInSlot < static_cast<uint32_t>(AdvInterval::ADV_INTERVAL_MIN) ||
            intervalInSlot > static_cast<uint32_t>(AdvInterval::ADV_INTERVAL_MAX)) {
            HILOGE("Invalid interval: %{public}d", intervalInSlot);
            HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_INTERGER);
            return napi_invalid_arg;
        }
        HILOGI("interval: %{public}u", intervalInSlot);
        outSettings.SetInterval(intervalInSlot);
    }
    int32_t power = 0;
    NAPI_NL_CALL_RETURN(NapiParseObjectInt32Optional(env, object, "power", power, exist));
    if (exist) {
        HILOGI("power is %{public}d", power);
        if (power < static_cast<int32_t>(AdevertiseMode::ADV_TX_POWER_LOW) ||
            power > static_cast<int32_t>(AdevertiseMode::ADV_TX_POWER_HIGH)) {
            HILOGE("Invalid power: %{public}d", power);
            return napi_invalid_arg;
        }
        outSettings.SetTxPower(ConvertPowerMode(power));
    }
    bool isConnectable;
    NAPI_NL_CALL_RETURN(NapiParseObjectBooleanOptional(env, object, "isConnectable", isConnectable, exist));
    if (exist) {
        outSettings.SetConnectable(isConnectable);
    }
    int32_t frameTypeValue;
    NAPI_NL_CALL_RETURN(NapiParseObjectInt32Optional(env, object, "frameType", frameTypeValue, exist));
    if (exist) {
        HILOGI("Advertising frameType is %{public}d", frameTypeValue);
        if (frameTypeValue < static_cast<int32_t>(FrameType::FRAME_TYPE_1) ||
            frameTypeValue > static_cast<int32_t>(FrameType::FRAME_TYPE_4)) {
            HILOGE("Invalid advertising frameType: %{public}d", frameTypeValue);
            return napi_invalid_arg;
        }
        uint8_t frameType = (frameTypeValue == static_cast<int32_t>(FrameType::FRAME_TYPE_4)) ?
            static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_4) :
            static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_1);
        outSettings.SetPrimaryFrameType(frameType);
    }
    return napi_ok;
}

static napi_status ParseServiceUuidParameters(napi_env env, napi_value object, SleAdvertiserData &outData)
{
    HILOGD("enter");
    bool exist = false;
    std::vector<UUID> vec {};

    NAPI_NL_CALL_RETURN(NapiParseArrayOptional(env, object, "serviceUuids", vec, exist));
    NAPI_NL_RETURN_IF(vec.size() > SERVICE_UUID_MAX_NUM, "Too many service uuids", napi_invalid_arg);
    if (exist) {
        for (size_t i = 0; i < vec.size(); ++i) {
            outData.AddServiceUuid(vec[i]);
            HILOGI("Service Uuid = %{public}s", GET_ENCRYPT_UUID(vec[i]));
        }
    }

    return napi_ok;
}

static napi_status ParseManufactureDataParameters(napi_env env, napi_value object, SleAdvertiserData &outData)
{
    HILOGD("enter");
    bool exist = false;
    std::vector<NapiAdvManufactureData> vec {};

    NAPI_NL_CALL_RETURN(NapiParseArrayOptional(env, object, "manufacturerData", vec, exist));
    NAPI_NL_RETURN_IF(vec.size() > MANUFACTURER_DATA_MAX_NUM, "Too much manufacturer data", napi_invalid_arg);
    if (exist) {
        for (size_t i = 0; i < vec.size(); ++i) {
            outData.AddManufacturerData(vec[i].id, vec[i].value);
        }
    }

    return napi_ok;
}

static napi_status ParseServiceDataParameters(napi_env env, napi_value object, SleAdvertiserData &outData)
{
    HILOGD("enter");
    bool exist = false;
    std::vector<NapiAdvServiceData> vec {};

    NAPI_NL_CALL_RETURN(NapiParseArrayOptional(env, object, "serviceData", vec, exist));
    NAPI_NL_RETURN_IF(vec.size() > SERVICE_DATA_MAX_NUM, "Too much service data", napi_invalid_arg);
    if (exist) {
        for (size_t i = 0; i < vec.size(); ++i) {
            outData.AddServiceData(
                UUID::FromString(vec[i].uuid),
                std::string(vec[i].value.begin(), vec[i].value.end()));
        }
    }

    return napi_ok;
}

static napi_status ParseAdvertisDataParameters(const napi_env &env,
    const napi_value &object, SleAdvertiserData &outData)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(
        env, object, {"serviceUuids", "manufacturerData", "serviceData", "includeDeviceName"}));

    NAPI_NL_CALL_RETURN(ParseServiceUuidParameters(env, object, outData));
    NAPI_NL_CALL_RETURN(ParseManufactureDataParameters(env, object, outData));
    NAPI_NL_CALL_RETURN(ParseServiceDataParameters(env, object, outData));
    bool exist = false;
    bool includeDeviceName = false;
    NAPI_NL_CALL_RETURN(NapiParseObjectBooleanOptional(env, object, "includeDeviceName", includeDeviceName, exist));
    HILOGI("includeDeviceName: %{public}d", includeDeviceName);
    outData.SetIncludeDeviceName(includeDeviceName);

    return napi_ok;
}

napi_status CheckAdvertisingParamWith(napi_env env, napi_value object, SleAdvertiserSettings &outSettings,
    SleAdvertiserData &outAdvData, SleAdvertiserData &outRspData)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object, {"advertisingSettings", "advertisingData",
        "advertisingResponse"}));
    napi_value advSettingsObject;
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "advertisingSettings", advSettingsObject));
    SleAdvertiserSettings settings;
    NAPI_NL_CALL_RETURN(ParseAdvertisingSettingsParameters(env, advSettingsObject, settings));

    napi_value advDataObject;
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "advertisingData", advDataObject));
    SleAdvertiserData advData;
    NAPI_NL_CALL_RETURN(ParseAdvertisDataParameters(env, advDataObject, advData));
    SleAdvertiserData advRsp;
    if (NapiIsObjectPropertyExist(env, object, "advertisingResponse")) {
        napi_value advRspObject;
        NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "advertisingResponse", advRspObject));
        NAPI_NL_CALL_RETURN(ParseAdvertisDataParameters(env, advRspObject, advRsp));
    }
    outSettings = std::move(settings);
    outAdvData = std::move(advData);
    outRspData = std::move(advRsp);
    return napi_ok;
}

napi_value NapiNearlinkAdvertising::StartAdvertising(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("startAdvertising", beginTime, true);
    size_t argc = ARGS_SIZE_THREE;
    napi_value argv[ARGS_SIZE_THREE] = {nullptr};
    napi_value thisVar = nullptr;
    auto getCbRes = napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL);
    NAPI_HA_EXCEP_REPORT(getCbRes == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, getCbRes == napi_ok, NL_ERR_INVALID_PARAM);
    napi_value res = nullptr;
    napi_get_undefined((env), &res);
    NAPI_HA_EXCEP_REPORT(argc == 1, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_RETURN_IF(argc != 1, "Requires 1 arguments.", res);
    bool ret = NapiIsObjectPropertyExist(env, argv[PARAM0], "advertisingSettings");
    NAPI_HA_EXCEP_REPORT(ret, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, ret, NL_ERR_INVALID_PARAM);
    SleAdvertiserSettings settings;
    SleAdvertiserData advData;
    SleAdvertiserData rspData;
    uint16_t duration = 0;
    auto status = CheckAdvertisingParamWith(env, argv[PARAM0], settings, advData, rspData);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    NAPI_HA_EXCEP_REPORT(checkResult == NL_NO_ERROR, context->apiName, beginTime, checkResult);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, checkResult == NL_NO_ERROR, checkResult);
    NAPI_HA_EXCEP_REPORT(isGranted, context->apiName, beginTime, NL_ERR_PERMISSION_FAILED);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, isGranted, NL_ERR_PERMISSION_FAILED);
    auto func = [settings, advData, rspData, duration]() {
        auto sleAdvertiser = SleAdvertiserGetInstance();
        if (sleAdvertiser == nullptr) {
            HILOGE("sleAdvertiser is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        int ret = sleAdvertiser->StartAdvertising(
            settings, advData, rspData, duration, NapiNearlinkAdvertiseCallback::GetInstance());
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    bool success = NapiNearlinkAdvertiseCallback::GetInstance()->asyncPromiseMap_.TryPush(
        NapiAsyncType::GET_ADVERTISING_HANDLE, asyncWork);
    NAPI_HA_EXCEP_REPORT(success, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, success, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_status CheckStopAdvWithAdvId(napi_env env, napi_value object, uint32_t &outAdvHandle)
{
    NAPI_NL_CALL_RETURN(NapiParseUint32(env, object, outAdvHandle));
    uint8_t advHandle;
    SleAdvertiserGetInstance()->GetAdvHandle(NapiNearlinkAdvertiseCallback::GetInstance(), advHandle);
    if (outAdvHandle != advHandle) {
        HILOGE("Invalid outAdvHandle: %{public}d", outAdvHandle);
        HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_ADV_ID);
        return napi_invalid_arg;
    }
    return napi_ok;
}

napi_value NapiNearlinkAdvertising::StopAdvertising(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGD("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("StopAdvertising", beginTime, true);
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    auto checkRes = napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL);
    NAPI_HA_EXCEP_REPORT(checkRes == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, checkRes == napi_ok, NL_ERR_INVALID_PARAM);

    auto status = napi_ok;
    napi_value res = nullptr;
    napi_get_undefined((env), &res);
    NAPI_HA_EXCEP_REPORT(argc == 1, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_RETURN_IF(argc != 1, "Requires 1 arguments.", res);
    uint32_t advHandle = 0xFF;
    status = CheckStopAdvWithAdvId(env, argv[PARAM0], advHandle);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    NAPI_HA_EXCEP_REPORT(checkResult == NL_NO_ERROR, context->apiName, beginTime, checkResult);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, checkResult == NL_NO_ERROR, checkResult);
    NAPI_HA_EXCEP_REPORT(isGranted, context->apiName, beginTime, NL_ERR_PERMISSION_FAILED);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, isGranted, NL_ERR_PERMISSION_FAILED);

    auto func = []() {
        auto sleAdvertiser = SleAdvertiserGetInstance();
        if (sleAdvertiser == nullptr) {
            HILOGE("sleAdvertiser is nullptr");
            return NapiAsyncWorkRet(NL_ERR_INTERNAL_ERROR);
        }
        int ret = sleAdvertiser->StopAdvertising(NapiNearlinkAdvertiseCallback::GetInstance());
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkAdvertising::PropertyInit(napi_env env, napi_value exports)
{
    HILOGD("enter");
    napi_value adverPowerObj = nullptr;
    napi_create_object(env, &adverPowerObj);
    SetNamedPropertyByInteger(
        env, adverPowerObj, static_cast<int32_t>(AdevertiseMode::ADV_TX_POWER_LOW), "ADV_TX_POWER_LOW");
    SetNamedPropertyByInteger(
        env, adverPowerObj, static_cast<int32_t>(AdevertiseMode::ADV_TX_POWER_MEDIUM), "ADV_TX_POWER_MEDIUM");
    SetNamedPropertyByInteger(
        env, adverPowerObj, static_cast<int32_t>(AdevertiseMode::ADV_TX_POWER_HIGH), "ADV_TX_POWER_HIGH");
    napi_value advertisingStateObj = nullptr;
    napi_create_object(env, &advertisingStateObj);
    SetNamedPropertyByInteger(
        env, advertisingStateObj, static_cast<int32_t>(AdvertisingState::STARTED), "STARTED");
    SetNamedPropertyByInteger(
        env, advertisingStateObj, static_cast<int32_t>(AdvertisingState::STOPPED), "STOPPED");

    napi_value frameTypeObj = nullptr;
    napi_create_object(env, &frameTypeObj);
    SetNamedPropertyByInteger(env, frameTypeObj,
        static_cast<int32_t>(FrameType::FRAME_TYPE_1), "FRAME_TYPE_1");
    SetNamedPropertyByInteger(env, frameTypeObj,
        static_cast<int32_t>(FrameType::FRAME_TYPE_4), "FRAME_TYPE_4");

    napi_property_descriptor exportFuncs[] = {
        DECLARE_NAPI_PROPERTY("TxPowerMode", adverPowerObj),
        DECLARE_NAPI_PROPERTY("AdvertisingState", advertisingStateObj),
        DECLARE_NAPI_PROPERTY("FrameType", frameTypeObj),
    };
    napi_define_properties(env, exports, sizeof(exportFuncs) / sizeof(*exportFuncs), exportFuncs);
    return exports;
}

EXTERN_C_START
/*
 * Module initialization function
 */
static napi_value Init(napi_env env, napi_value exports)
{
    HILOGI("-----Advertising Init start------");

    NapiNearlinkAdvertising::DefineAdvertisingJSObject(env, exports);
    HILOGI("-----Advertising Init end------");
    return exports;
}
EXTERN_C_END

/*
 * Module define
 */
static napi_module nearlinkAdvertisingModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "nearlink.advertising",
    .nm_priv = ((void *)0),
    .reserved = {0}};

/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    HILOGI("Register nearlinkAdvertisingModule nm_modname:%{public}s",
        nearlinkAdvertisingModule.nm_modname);
    napi_module_register(&nearlinkAdvertisingModule);
}
}  // namespace Nearlink
}  // namespace OHOS
