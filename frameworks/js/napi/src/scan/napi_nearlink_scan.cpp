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
#include "napi_nearlink_scan.h"
#include <memory>
#include "log_util.h"
#include "napi_nearlink_scan_callback.h"
#include "napi_nearlink_error.h"
#include "napi_nearlink_utils.h"
#include "nearlink_sle_scanner.h"
#include "nearlink_errorcode.h"
#include "napi_parser_utils.h"
#include "nearlink_host.h"
#include "napi_ha_manager.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr size_t NAPI_ARRAY_MAX_LENGTH = 0xFFFF;

std::shared_ptr<SleCentralManager> SleCentralManagerGetInstance(void)
{
    static std::shared_ptr<SleCentralManager> instance =
        SleCentralManager::CreateSleCentralManager(NapiNearlinkCentralManagerCallback::GetInstance());
    return instance;
}
}  // namespace {}
// Scan duration
const int SCAN_DURATION_MIN = 10;
const int SCAN_DURATION_MAX = 60;
constexpr int32_t SCAN_RSSI_MIN = -128;
constexpr int32_t SCAN_RSSI_MAX = 127;

napi_value NapiNearlinkScan::OnDeviceFound(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = NapiNearlinkCentralManagerCallback::GetInstance()->eventSubscribe.RegisterWithName(
        env, info, REGISTER_SCAN_DEVICE_NAME);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

napi_value NapiNearlinkScan::OffDeviceFound(napi_env env, napi_callback_info info)
{
    HILOGD("enter");
    NAPI_NL_ASSERT_RETURN_UNDEF(env, NearlinkHost::GetInstance().IsNearlinkSupport(), NL_ERR_API_NOT_SUPPORT);
    auto status = NapiNearlinkCentralManagerCallback::GetInstance()->eventSubscribe.DeregisterWithName(
        env, info, REGISTER_SCAN_DEVICE_NAME);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);
    return NapiGetUndefinedRet(env);
}

void NapiNearlinkScan::DefineCentralManagerJSObject(napi_env env, napi_value exports)
{
    HILOGD("enter");
    PropertyInit(env, exports);
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("startScan", StartScanWithFilter),
        DECLARE_NAPI_FUNCTION("stopScan", StopScan),
        DECLARE_NAPI_FUNCTION("onDeviceFound", OnDeviceFound),
        DECLARE_NAPI_FUNCTION("offDeviceFound", OffDeviceFound),
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}

static ScanDuty ConvertDutyMode(int32_t dutyMode)
{
    ScanDuty scanDutyMode = ScanDuty::SCAN_MODE_LOW_POWER;
    switch (dutyMode) {
        case static_cast<int32_t>(ScanDuty::SCAN_MODE_LOW_POWER):
            scanDutyMode = ScanDuty::SCAN_MODE_LOW_POWER;
            break;
        case static_cast<int32_t>(ScanDuty::SCAN_MODE_BALANCED):
            scanDutyMode = ScanDuty::SCAN_MODE_BALANCED;
            break;
        case static_cast<int32_t>(ScanDuty::SCAN_MODE_LOW_LATENCY):
            scanDutyMode = ScanDuty::SCAN_MODE_LOW_LATENCY;
            break;
        default:
            break;
    }
    return scanDutyMode;
}

static napi_status ParseScanParameters(
    const napi_env &env, const napi_callback_info &info, const napi_value &scanArg, ScanOptions &params)
{
    (void)info;
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, scanArg, {"scanMode", "duration", "frameType"}));
    bool exist;
    int32_t dutyMode;
    NAPI_NL_CALL_RETURN(NapiParseObjectInt32Optional(env, scanArg, "scanMode", dutyMode, exist));
    if (exist) {
        HILOGI("Scan dutyMode is %{public}d", dutyMode);
        if (dutyMode < static_cast<int32_t>(ScanDuty::SCAN_MODE_LOW_POWER) ||
            dutyMode > static_cast<int32_t>(ScanDuty::SCAN_MODE_LOW_LATENCY)) {
            HILOGE("Invalid scan dutyMode: %{public}d", dutyMode);
            return napi_invalid_arg;
        }
        params.dutyMode = ConvertDutyMode(dutyMode);
    }
    int32_t duration;
    NAPI_NL_CALL_RETURN(NapiParseObjectInt32Optional(env, scanArg, "duration", duration, exist));
    if (exist) {
        HILOGI("Scan duration is %{public}d", duration);
        NAPI_NL_RETURN_IF(!(SCAN_DURATION_MIN <= duration && duration <= SCAN_DURATION_MAX),
            "duration should in range of 10 to 60 (sec), inclusive.", napi_invalid_arg);
        params.duration = duration;
    }
    int32_t frameTypeValue;
    NAPI_NL_CALL_RETURN(NapiParseObjectInt32Optional(env, scanArg, "frameType", frameTypeValue, exist));
    if (exist) {
        HILOGI("Scan frameType is %{public}d", frameTypeValue);
        if (frameTypeValue < static_cast<int32_t>(FrameType::FRAME_TYPE_1) ||
            frameTypeValue > static_cast<int32_t>(FrameType::FRAME_TYPE_4)) {
            HILOGE("Invalid scan frameType: %{public}d", frameTypeValue);
            return napi_invalid_arg;
        }
        params.frameType = static_cast<FrameType>(frameTypeValue);
        params.hasFrameType = true;
    }
    return napi_ok;
}

static napi_status ParseScanFilterDeviceIdParameters(
    const napi_env &env, napi_value &scanFilter, SleScanFilter &sleScanFilter)
{
    bool exist = false;
    std::string deviceId {};
    NAPI_NL_CALL_RETURN(NapiParseObjectStringOptional(env, scanFilter, "address", deviceId, exist));
    if (exist) {
        if (!IsValidAddress(deviceId)) {
            HILOGE("Invalid deviceId");
            HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_ADDRESS);
            return napi_invalid_arg;
        }

        HILOGI("Scan filter device id is %{public}s", GetEncryptAddr(deviceId).c_str());
        sleScanFilter.SetDeviceId(deviceId);
    }
    return napi_ok;
}

static napi_status ParseScanFilterLocalNameParameters(
    const napi_env &env, napi_value &scanFilter, SleScanFilter &sleScanFilter)
{
    bool exist = false;
    std::string name {};
    NAPI_NL_CALL_RETURN(NapiParseObjectStringOptional(env, scanFilter, "deviceName", name, exist));
    if (exist) {
        if (name.empty()) {
            HILOGE("name is empty");
            return napi_invalid_arg;
        }
        sleScanFilter.SetName(name);
    }
    return napi_ok;
}

static napi_status ParseScanFilterManufactureDataParameters(
    const napi_env &env, napi_value &scanFilter, SleScanFilter &sleScanFilter)
{
    bool existManufacturerId = false;
    bool existManufacturerData = false;
    bool existManufacturerDataMask = false;
    uint32_t manufacturerId = 0;
    std::vector<uint8_t> data {};
    std::vector<uint8_t> dataMask {};
    NAPI_NL_CALL_RETURN(NapiParseObjectUint32Optional(
        env, scanFilter, "manufacturerId", manufacturerId, existManufacturerId));
    NAPI_NL_CALL_RETURN(NapiParseArrayBufferOptional(
        env, scanFilter, "manufacturerData", data, existManufacturerData));
    NAPI_NL_CALL_RETURN(NapiParseArrayBufferOptional(
        env, scanFilter, "manufacturerDataMask", dataMask, existManufacturerDataMask));

    if (!data.empty() && !existManufacturerId) {
        HILOGE("invalid manufacturerId");
        return napi_invalid_arg;
    }
    if (!dataMask.empty()) {
        if (data.empty()) {
            HILOGE("manufacturerData is empty while manufacturerDataMask is not empty");
            return napi_invalid_arg;
        }
        if (data.size() != dataMask.size()) {
            HILOGE("size mismatch for manufacturerData and manufacturerDataMask");
            return napi_invalid_arg;
        }
    }
    sleScanFilter.SetManufacturerId(manufacturerId);
    HILOGI("Scan filter manufacturerId is %{public}#x", manufacturerId);
    sleScanFilter.SetManufactureData(std::move(data));
    sleScanFilter.SetManufactureDataMask(std::move(dataMask));
    return napi_ok;
}

static napi_status ParseScanFilterRssiParameters(
        const napi_env &env, napi_value &scanFilter, SleScanFilter &sleScanFilter)
{
    bool exist = false;
    int32_t rssi = 0;
    NAPI_NL_CALL_RETURN(NapiParseObjectInt32Optional(env, scanFilter, "rssi", rssi, exist));
    if (exist) {
        HILOGI("Scan rssi is %{public}d", rssi);
        NAPI_NL_RETURN_IF((rssi < SCAN_RSSI_MIN || rssi > SCAN_RSSI_MAX),
                          "rssi should in range of -128 to 127, inclusive.", napi_invalid_arg);
        sleScanFilter.SetRssiThreshold(rssi);
    }
    return napi_ok;
}

static napi_status ParseScanFilterServiceDataParameters(
    const napi_env &env, napi_value &scanFilter, SleScanFilter &sleScanFilter)
{
    bool existServiceData = false;
    bool existServiceDataMask = false;
    NapiAdvServiceData serviceData {};
    NapiAdvServiceData serviceDataMask {};

    NAPI_NL_CALL_RETURN(NapiParseObjectOptional(
        env, scanFilter, "serviceData", serviceData, existServiceData));
    NAPI_NL_CALL_RETURN(NapiParseObjectOptional(
        env, scanFilter, "serviceDataMask", serviceDataMask, existServiceDataMask));

    if (existServiceDataMask && !existServiceData) {
        HILOGE("serviceData is empty while serviceDataMask is not empty");
        return napi_invalid_arg;
    }

    if (existServiceData) {
        std::vector<uint8_t> concatenatedData;
        UUID uuid = UUID::FromString(serviceData.uuid);
        auto uuidBytes = uuid.ConvertTo128Bits();
        concatenatedData.insert(concatenatedData.end(), uuidBytes.begin(), uuidBytes.end());
        concatenatedData.insert(concatenatedData.end(),
                               serviceData.value.begin(),
                               serviceData.value.end());

        HILOGI("Scan filter serviceData: uuid=%{public}s, dataLen=%{public}zu",
               GET_ENCRYPT_UUID(uuid), serviceData.value.size());
        sleScanFilter.SetServiceData(concatenatedData);
    }

    if (existServiceDataMask) {
        std::vector<uint8_t> concatenatedMask;
        UUID uuid = UUID::FromString(serviceDataMask.uuid);
        auto uuidBytes = uuid.ConvertTo128Bits();
        concatenatedMask.insert(concatenatedMask.end(), uuidBytes.begin(), uuidBytes.end());
        concatenatedMask.insert(concatenatedMask.end(),
                               serviceDataMask.value.begin(),
                               serviceDataMask.value.end());

        HILOGI("Scan filter serviceDataMask: uuid=%{public}s, maskLen=%{public}zu",
               GET_ENCRYPT_UUID(uuid), serviceDataMask.value.size());
        sleScanFilter.SetServiceDataMask(concatenatedMask);
    }

    return napi_ok;
}

static napi_status ParseScanFilter(const napi_env &env, napi_value &scanFilter, SleScanFilter &sleScanFilter)
{
    HILOGD("enter");
    std::vector<std::string> filterList {"address", "deviceName", "manufacturerId",
        "manufacturerData", "manufacturerDataMask", "rssi", "serviceData", "serviceDataMask"};
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, scanFilter, filterList));
    NAPI_NL_CALL_RETURN(ParseScanFilterDeviceIdParameters(env, scanFilter, sleScanFilter));
    NAPI_NL_CALL_RETURN(ParseScanFilterLocalNameParameters(env, scanFilter, sleScanFilter));
    NAPI_NL_CALL_RETURN(ParseScanFilterManufactureDataParameters(env, scanFilter, sleScanFilter));
    NAPI_NL_CALL_RETURN(ParseScanFilterRssiParameters(env, scanFilter, sleScanFilter));
    NAPI_NL_CALL_RETURN(ParseScanFilterServiceDataParameters(env, scanFilter, sleScanFilter));
    return napi_ok;
}

static bool isFilterEmpty(const SleScanFilter &filter)
{
    if (!filter.GetDeviceId().empty()) {
        return false;
    } else if (!filter.GetName().empty()) {
        return false;
    } else if (filter.GetManufacturerId() > 0) {
        return false;
    } else if (!filter.GetManufactureData().empty()) {
        return false;
    } else if (!filter.GetManufactureDataMask().empty()) {
        return false;
    } else if (filter.HasRssiThreshold()) {
        return false;
    } else if (filter.HasServiceData()) {
        return false;
    } else if (filter.HasServiceDataMask()) {
        return false;
    } else {
        return true;
    }
}

static napi_status ParseScanFilterParameters(const napi_env &env, napi_value &args, std::vector<SleScanFilter> &params)
{
    HILOGD("enter");
    NAPI_NL_CALL_RETURN(NapiIsArray(env, args));

    uint32_t length = 0;
    NAPI_NL_CALL_RETURN(napi_get_array_length(env, args, &length));
    if (length == 0) {
        HILOGE("Requires array length > 0");
        HandleSyncErr(env, NlErrCode::NL_ERR_EMPTY_ARRAY);
        return napi_invalid_arg;
    }
    NAPI_NL_RETURN_IF(length > NAPI_ARRAY_MAX_LENGTH, "Array is too long", napi_invalid_arg);
    for (uint32_t i = 0; i < length; i++) {
        napi_value scanFilter;
        NAPI_NL_CALL_RETURN(napi_get_element(env, args, i, &scanFilter));
        NAPI_NL_CALL_RETURN(NapiIsObject(env, scanFilter));
        SleScanFilter sleScanFilter;
        NAPI_NL_CALL_RETURN(ParseScanFilter(env, scanFilter, sleScanFilter));
        if (!isFilterEmpty(sleScanFilter)) {
            params.push_back(sleScanFilter);
        }
    }
    if (params.empty()) {
        HILOGE("Filters are all empty");
        HandleSyncErr(env, NlErrCode::NL_ERR_EMPTY_ARRAY);
        return napi_invalid_arg;
    }
    return napi_ok;
}

static napi_status CheckFullScanParams(napi_env env, napi_callback_info info, SleScanSettings &outSettings)
{
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    NAPI_NL_RETURN_IF((argc > ARGS_SIZE_ONE), "Requires 0 or 1 arguments", napi_invalid_arg);

    if (argc == ARGS_SIZE_ONE) {
        ScanOptions scanOptions;
        NAPI_NL_CALL_RETURN(ParseScanParameters(env, info, argv[PARAM0], scanOptions));
        outSettings.SetReportDelay(scanOptions.interval);
        outSettings.SetScanMode(static_cast<int>(scanOptions.dutyMode));
        outSettings.SetDuration(static_cast<int>(scanOptions.duration));
        if (scanOptions.hasFrameType) {
            uint8_t frameTypeValue = (scanOptions.frameType == FrameType::FRAME_TYPE_4) ?
                static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4) :
                static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1);
            outSettings.SetFrameType(frameTypeValue);
        }
    }
    return napi_ok;
}

static napi_status CheckFilterScanParams(napi_env env, napi_callback_info info,
    std::vector<SleScanFilter> &outScanFilters, SleScanSettings &outSettings)
{
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    napi_value thisVar = nullptr;
    NAPI_NL_CALL_RETURN(napi_get_cb_info(env, info, &argc, argv, &thisVar, NULL));
    NAPI_NL_RETURN_IF((argc != ARGS_SIZE_ONE && argc != ARGS_SIZE_TWO),
        "Requires 1 or 2 arguments", napi_invalid_arg);

    std::vector<SleScanFilter> scanFilters;
    napi_valuetype type = napi_undefined;
    NAPI_NL_CALL_RETURN(napi_typeof(env, argv[PARAM0], &type));
    if (type == napi_null) {
        SleScanFilter emptyFilter;
        scanFilters.push_back(emptyFilter);
    } else {
        NAPI_NL_CALL_RETURN(ParseScanFilterParameters(env, argv[PARAM0], scanFilters));
    }
    if (argc == ARGS_SIZE_TWO) {
        ScanOptions scanOptions;
        NAPI_NL_CALL_RETURN(ParseScanParameters(env, info, argv[PARAM1], scanOptions));
        outSettings.SetReportDelay(scanOptions.interval);
        outSettings.SetScanMode(static_cast<int>(scanOptions.dutyMode));
        outSettings.SetDuration(static_cast<int>(scanOptions.duration));
        if (scanOptions.hasFrameType) {
            uint8_t frameTypeValue = (scanOptions.frameType == FrameType::FRAME_TYPE_4) ?
                static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_4) :
                static_cast<uint8_t>(SleScanFrameType::SLE_SCAN_FRAME_TYPE_1);
            outSettings.SetFrameType(frameTypeValue);
        }
    }

    outScanFilters = std::move(scanFilters);
    return napi_ok;
}

napi_value NapiNearlinkScan::StartScanWithFilter(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("startScan", beginTime, true);
    // user permission, have one default filter at least
    std::vector<SleScanFilter> scanFilters{};
    SleScanSettings settings;
    auto status = CheckFilterScanParams(env, info, scanFilters, settings);
    NAPI_HA_EXCEP_REPORT(status == napi_ok, context->apiName, beginTime, NL_ERR_INVALID_PARAM);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, status == napi_ok, NL_ERR_INVALID_PARAM);

    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    NAPI_HA_EXCEP_REPORT(checkResult == NL_NO_ERROR, context->apiName, beginTime, checkResult);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, checkResult == NL_NO_ERROR, checkResult);
    NAPI_HA_EXCEP_REPORT(isGranted, context->apiName, beginTime, NL_ERR_PERMISSION_FAILED);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, isGranted, NL_ERR_PERMISSION_FAILED);

    auto func = [settings, scanFilters]() {
        int ret = SleCentralManagerGetInstance()->StartScanWithFilter(settings, scanFilters);
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkScan::StopScan(napi_env env, napi_callback_info info)
{
    int64_t beginTime = NapiHaManager::GetCurrentTimestamp();
    HILOGI("enter");
    std::shared_ptr<AsyncWorkContext> context = std::make_shared<AsyncWorkContext>("stopScan", beginTime, true);
    // 提前校验ACCESS权限
    bool isGranted = false;
    NlErrCode checkResult = NearlinkHost::GetInstance().CheckPermissionForNapi(ACCESS_NEARLINK_PERMISSION, isGranted);
    NAPI_HA_EXCEP_REPORT(checkResult == NL_NO_ERROR, context->apiName, beginTime, checkResult);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, checkResult == NL_NO_ERROR, checkResult);
    NAPI_HA_EXCEP_REPORT(isGranted, context->apiName, beginTime, NL_ERR_PERMISSION_FAILED);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, isGranted, NL_ERR_PERMISSION_FAILED);

    auto func = []() {
        NlErrCode ret = SleCentralManagerGetInstance()->StopScan();
        return NapiAsyncWorkRet(ret);
    };
    auto asyncWork = NapiAsyncWorkFactory::CreateAsyncWork(env, info, func, context, ASYNC_WORK_NO_NEED_CALLBACK);
    NAPI_HA_EXCEP_REPORT(asyncWork, context->apiName, beginTime, NL_ERR_INTERNAL_ERROR);
    NAPI_NL_ASSERT_RETURN_UNDEF(env, asyncWork, NL_ERR_INTERNAL_ERROR);
    asyncWork->Run();
    return asyncWork->GetRet();
}

napi_value NapiNearlinkScan::PropertyInit(napi_env env, napi_value exports)
{
    HILOGD("enter");

    napi_value scanDutyObj = nullptr;
    napi_create_object(env, &scanDutyObj);
    SetNamedPropertyByInteger(
        env, scanDutyObj, static_cast<int32_t>(ScanDuty::SCAN_MODE_BALANCED), "SCAN_MODE_BALANCED");
    SetNamedPropertyByInteger(
        env, scanDutyObj, static_cast<int32_t>(ScanDuty::SCAN_MODE_LOW_LATENCY), "SCAN_MODE_LOW_LATENCY");
    SetNamedPropertyByInteger(
        env, scanDutyObj, static_cast<int32_t>(ScanDuty::SCAN_MODE_LOW_POWER), "SCAN_MODE_LOW_POWER");

    napi_value frameTypeObj = nullptr;
    napi_create_object(env, &frameTypeObj);
    SetNamedPropertyByInteger(env, frameTypeObj,
        static_cast<int32_t>(FrameType::FRAME_TYPE_1), "FRAME_TYPE_1");
    SetNamedPropertyByInteger(env, frameTypeObj,
        static_cast<int32_t>(FrameType::FRAME_TYPE_4), "FRAME_TYPE_4");

    napi_property_descriptor exportFuncs[] = {
        DECLARE_NAPI_PROPERTY("ScanMode", scanDutyObj),
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
    HILOGI("-----Nearlink Central Manager Init start------");

    NapiNearlinkScan::DefineCentralManagerJSObject(env, exports);
    NapiHaManager::AddProcessor();
    HILOGI("-----Nearlink Central Manager Init end------");
    return exports;
}
EXTERN_C_END

/*
 * Module define
 */
static napi_module nearlinkScanModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "nearlink.scan",
    .nm_priv = ((void *)0),
    .reserved = {0}};

/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    HILOGI("Register nearlinkScanModule nm_modname:%{public}s", nearlinkScanModule.nm_modname);
    napi_module_register(&nearlinkScanModule);
}
}  // namespace Nearlink
}  // namespace OHOS
