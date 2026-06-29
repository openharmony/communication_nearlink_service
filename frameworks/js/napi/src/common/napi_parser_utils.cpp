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

#include <string>
#include <vector>
#include "log.h"

#include "napi_async_callback.h"
#include "napi_parser_utils.h"
#include "napi_nearlink_error.h"
#include "nearlink_sle_datatransfer.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr size_t NAPI_ARRAY_MAX_LENGTH = 0xFFFF;
constexpr size_t UUID_128_BIT_LENGTH = 36;
constexpr size_t BASE_UUID_LENGTH = 32;
constexpr uint32_t OPERATION_READ = 1;
constexpr uint32_t OPERATION_WRITE_NO_RESPONSE = 2;
const std::string BASE_UUID = "37BEA880-FC70-11EA-B720-00000000";
}

bool CheckBaseUuid(const std::string &uuid)
{
    std::string uuidStr = uuid;
    if (uuidStr.length() != UUID_128_BIT_LENGTH) {
        return false;
    }
    ConvertUuidToUpperCase(uuidStr);
    std::string uuidPrefix = uuidStr.substr(0, BASE_UUID_LENGTH);
    if (uuidPrefix == BASE_UUID) {
        UUID printUuid = UUID::FromString(uuid);
        HILOGI("Uuid(%{public}s) is standard uuid", GET_ENCRYPT_UUID(printUuid));
        return true;
    }
    return false;
}

int32_t NapiParseSsapService(napi_env env, napi_value object, NapiSsapService &outService)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object, {
        "serviceUuid", "properties", "methods", "events"}));
    std::string uuid {};
    std::vector<NapiSsapProperty> properties {};
    std::vector<NapiSsapMethod> methods {};
    std::vector<NapiSsapEvent> events {};
    NAPI_NL_CALL_RETURN(NapiParseObjectUuid(env, object, "serviceUuid", uuid));
    NAPI_NL_CALL_RETURN(NapiParseObjectArray(env, object, "properties", properties));
    if (NapiIsObjectPropertyExist(env, object, "methods")) {
        NAPI_NL_CALL_RETURN(NapiParseObjectArray(env, object, "methods", methods));
    }
    if (NapiIsObjectPropertyExist(env, object, "events")) {
        NAPI_NL_CALL_RETURN(NapiParseObjectArray(env, object, "events", events));
    }

    if (CheckBaseUuid(uuid)) {
        HandleSyncErr(env, NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED);
        return NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED;
    }
    outService.uuid_ = UUID::FromString(uuid);
    outService.properties_ = std::move(properties);
    outService.methods_ = std::move(methods);
    outService.events_ = std::move(events);
    return NlErrCode::NL_NO_ERROR;
}

int32_t NapiParseSsapProperty(napi_env env, napi_value object, NapiSsapProperty &outProperty)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object, {"serviceUuid", "propertyUuid", "value",
    "descriptors", "operation"}));
    std::string propertyUuid {};
    std::string serviceUuid {};
    std::vector<uint8_t> propertyValue {};
    std::vector<NapiSsapDescriptor> descriptors {};
    uint32_t operationIndication = OPERATION_READ | OPERATION_WRITE_NO_RESPONSE;
    NAPI_NL_CALL_RETURN(NapiParseObjectUuid(env, object, "serviceUuid", serviceUuid));
    NAPI_NL_CALL_RETURN(NapiParseObjectUuid(env, object, "propertyUuid", propertyUuid));
    NAPI_NL_CALL_RETURN(NapiParseArrayBuffer(env, object, "value", propertyValue));
    if (NapiIsObjectPropertyExist(env, object, "descriptors")) {
        std::vector<NapiSsapDescriptor> descriptors {};
        NAPI_NL_CALL_RETURN(NapiParseObjectArray(env, object, "descriptors", descriptors));
        outProperty.descriptors_ = std::move(descriptors);
    }
    if (NapiIsObjectPropertyExist(env, object, "operation")) {
        NAPI_NL_CALL_RETURN(NapiParseObjectUint32(env, object, "operation", operationIndication));
    }

    if (CheckBaseUuid(serviceUuid) || CheckBaseUuid(propertyUuid)) {
        HandleSyncErr(env, NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED);
        return NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED;
    }
    outProperty.serviceUuid_ = UUID::FromString(serviceUuid);
    outProperty.propertyUuid_ = UUID::FromString(propertyUuid);
    outProperty.value_ = std::move(propertyValue);
    outProperty.operation_ = operationIndication;
    return NlErrCode::NL_NO_ERROR;
}

int32_t NapiParseSsapMethod(napi_env env, napi_value object, NapiSsapMethod &outMethod)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object, {"serviceUuid", "methodUuid", "parameter",
    "result"}));
    std::string serviceUuid {};
    std::string methodUuid {};
    std::vector<uint8_t> methodParameter {};
    std::vector<uint8_t> methodResult {};
    NAPI_NL_CALL_RETURN(NapiParseObjectUuid(env, object, "serviceUuid", serviceUuid));
    NAPI_NL_CALL_RETURN(NapiParseObjectUuid(env, object, "methodUuid", methodUuid));
    if (NapiIsObjectPropertyExist(env, object, "parameter")) {
        NAPI_NL_CALL_RETURN(NapiParseArrayBuffer(env, object, "parameter", methodParameter));
        HILOGI("methodParameter size : %{public}lu", methodParameter.size());
        outMethod.parameter_ = std::move(methodParameter);
    }
    if (NapiIsObjectPropertyExist(env, object, "result")) {
        NAPI_NL_CALL_RETURN(NapiParseArrayBuffer(env, object, "result", methodResult));
        outMethod.result_ = std::move(methodResult);
    }

    if (CheckBaseUuid(serviceUuid) || CheckBaseUuid(methodUuid)) {
        HandleSyncErr(env, NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED);
        return NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED;
    }
    outMethod.serviceUuid_ = UUID::FromString(serviceUuid);
    outMethod.methodUuid_ = UUID::FromString(methodUuid);
    return NlErrCode::NL_NO_ERROR;
}

int32_t NapiParseSsapDescriptor(napi_env env, napi_value object, NapiSsapDescriptor &outDescriptor)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object, {"serviceUuid", "propertyUuid",
    "descriptorType", "value", "isWriteable"}));
    std::string serviceUuid {};
    std::string propertyUuid {};
    std::vector<uint8_t> descriptorValue {};
    int32_t descriptorType = static_cast<int32_t>(NapiSsapDescriptorType::PROPERTY);
    napi_value type {};
    bool writeable = true;
    if (NapiIsObjectPropertyExist(env, object, "serviceUuid")) {
        NAPI_NL_CALL_RETURN(NapiParseObjectUuid(env, object, "serviceUuid", serviceUuid));
        outDescriptor.serviceUuid_ = UUID::FromString(serviceUuid);
    }
    if (NapiIsObjectPropertyExist(env, object, "propertyUuid")) {
        NAPI_NL_CALL_RETURN(NapiParseObjectUuid(env, object, "propertyUuid", propertyUuid));
        outDescriptor.propertyUuid_ = UUID::FromString(propertyUuid);
    }

    if (CheckBaseUuid(serviceUuid) || CheckBaseUuid(propertyUuid)) {
        HandleSyncErr(env, NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED);
        return NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED;
    }
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "descriptorType", type));
    NAPI_NL_CALL_RETURN(NapiParseSsapDescriptorType(env, type, descriptorType));
    NAPI_NL_CALL_RETURN(NapiParseArrayBuffer(env, object, "value", descriptorValue));
    if (NapiIsObjectPropertyExist(env, object, "isWriteable")) {
        NAPI_NL_CALL_RETURN(NapiParseObjectBoolean(env, object, "isWriteable", writeable));
    }
    outDescriptor.descriptorType_ = descriptorType;
    outDescriptor.descriptorValue_ = std::move(descriptorValue);
    outDescriptor.writeable_ = writeable;
    return NlErrCode::NL_NO_ERROR;
}

int32_t NapiParseSsapEvent(napi_env env, napi_value object, NapiSsapEvent &outEvent)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object, {"serviceUuid", "uuid", "handle"}));
    std::string serviceUuid {};
    std::string eventUuid {};
    uint32_t handle = 0;
    NAPI_NL_CALL_RETURN(NapiParseObjectUuid(env, object, "serviceUuid", serviceUuid));
    NAPI_NL_CALL_RETURN(NapiParseObjectUuid(env, object, "uuid", eventUuid));
    NAPI_NL_CALL_RETURN(NapiParseObjectUint32(env, object, "handle", handle));

    if (CheckBaseUuid(serviceUuid) || CheckBaseUuid(eventUuid)) {
        HandleSyncErr(env, NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED);
        return NlErrCode::NL_ERR_STANDARD_UUID_NOT_ALLOWED;
    }

    outEvent.serviceUuid_ = UUID::FromString(serviceUuid);
    outEvent.uuid_ = UUID::FromString(eventUuid);
    outEvent.handle_ = static_cast<uint16_t>(handle);
    return NlErrCode::NL_NO_ERROR;
}


napi_status NapiParseSsapServerResponse(napi_env env, napi_value object, NapiSsapServerResponse &response)
{
    napi_value property;
    std::string address;
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "address", property));
    NAPI_NL_CALL_RETURN(NapiParseString(env, property, address));
    int requestId;
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, "requestId", property));
    NAPI_NL_CALL_RETURN(NapiParseInt32(env, property, requestId));
    std::vector<uint8_t> value {};
    NAPI_NL_CALL_RETURN(NapiParseArrayBuffer(env, object, "value", value));
    response.address_ = std::move(address);
    response.requestId_ = std::move(requestId);
    response.value_ = std::move(value);
    return napi_ok;
}

napi_status NapiParseBoolean(napi_env env, napi_value value, bool &outBoolean)
{
    bool boolean = false;
    NAPI_NL_CALL_RETURN(NapiIsBoolean(env, value));
    NAPI_NL_CALL_RETURN(napi_get_value_bool(env, value, &boolean));
    outBoolean = boolean;
    return napi_ok;
}

napi_status NapiParseInt32(napi_env env, napi_value value, int32_t &outNum)
{
    int32_t num = 0;
    NAPI_NL_CALL_RETURN(NapiIsNumber(env, value));
    NAPI_NL_CALL_RETURN(napi_get_value_int32(env, value, &num));
    outNum = num;
    return napi_ok;
}

napi_status NapiParseUint32(napi_env env, napi_value value, uint32_t &outNum)
{
    uint32_t num = 0;
    NAPI_NL_CALL_RETURN(NapiIsNumber(env, value));
    NAPI_NL_CALL_RETURN(napi_get_value_uint32(env, value, &num));
    outNum = num;
    return napi_ok;
}

napi_status NapiParseString(napi_env env, napi_value value, std::string &outStr)
{
    std::string str {};
    NAPI_NL_RETURN_IF(!ParseString(env, str, value), "parse string failed", napi_invalid_arg);
    outStr = std::move(str);
    return napi_ok;
}

napi_status NapiParseUuid(napi_env env, napi_value value, std::string &outUuid)
{
    std::string uuid {};
    NAPI_NL_CALL_RETURN(NapiParseString(env, value, uuid));
    if (!IsValidUuid(uuid)) {
        HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_UUID);
        return napi_invalid_arg;
    }
    ConvertUuidToUpperCase(uuid); // UUID统一解析为大写字母形式
    outUuid = std::move(uuid);
    return napi_ok;
}

napi_status NapiParseArrayBuffer(napi_env env, napi_value value, std::vector<uint8_t> &outVec)
{
    uint8_t *data = nullptr;
    size_t size = 0;
    NAPI_NL_CALL_RETURN(NapiIsArrayBuffer(env, value));
    bool isSuccess = ParseArrayBuffer(env, &data, size, value);
    if (!isSuccess) {
        HILOGE("Parse arraybuffer failed");
        return napi_invalid_arg;
    }
    std::vector<uint8_t> vec(data, data + size);
    outVec = std::move(vec);
    return napi_ok;
}

napi_status NapiParseTransMode(napi_env env, napi_value value, uint8_t &outTransMode)
{
    int32_t transMode = 0;
    NAPI_NL_CALL_RETURN(NapiParseInt32(env, value, transMode));
    if (transMode == static_cast<int32_t>(TransferMode::BASIC)) {
        HILOGI("transMode: BASIC");
        outTransMode = static_cast<uint8_t>(ConnectionParams::PortTransMode::TRANSPORT_MODE_BASIC);
    } else if (transMode == static_cast<int32_t>(TransferMode::RELIABLE)) {
        HILOGI("transMode: RELIABLE");
        outTransMode = static_cast<uint8_t>(ConnectionParams::PortTransMode::TRANSPORT_MODE_RELIABLE);
    } else {
        HILOGE("Invalid transMode: %{public}d", transMode);
        HandleSyncErr(env, NlErrCode::NL_ERR_INVALID_INTERGER);
        return napi_invalid_arg;
    }
    return napi_ok;
}

napi_status NapiParseSsapDescriptorType(napi_env env, napi_value value, int32_t &outDescriptorType)
{
    int32_t descriptorType = -1;
    NAPI_NL_CALL_RETURN(NapiParseInt32(env, value, descriptorType));
    if (descriptorType == static_cast<int32_t>(NapiSsapDescriptorType::PROPERTY)) {
        HILOGI("descriptorType: PROPERTY");
        outDescriptorType = SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_PROPERTY;
    } else if (descriptorType == static_cast<int32_t>(NapiSsapDescriptorType::CLIENT_PROPERTY_CONFIG)) {
        HILOGI("descriptorType: CLIENT_PROPERTY_CONFIG");
        outDescriptorType = SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_CLIENT_PROPERTY_CONFIG;
    } else if (descriptorType == static_cast<int32_t>(NapiSsapDescriptorType::SERVER_PROPERTY_CONFIG)) {
        HILOGI("descriptorType: SERVER_PROPERTY_CONFIG");
        outDescriptorType = SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_SERVER_PROPERTY_CONFIG;
    } else if (descriptorType == static_cast<int32_t>(NapiSsapDescriptorType::PROPERTY_FORMAT)) {
        HILOGI("descriptorType: PROPERTY_FORMAT");
        outDescriptorType = SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_PROPERTY_FORMAT;
    } else if (descriptorType == static_cast<int32_t>(NapiSsapDescriptorType::TYPE_VENDOR)) {
        HILOGI("descriptorType: TYPE_VENDOR");
        outDescriptorType = SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_VENDOR;
    } else {
        HILOGE("Invalid descriptorType: %{public}d", descriptorType);
        return napi_invalid_arg;
    }
    return napi_ok;
}

static napi_status NapiGetObjectProperty(napi_env env, napi_value object, const char *name, napi_value &outProperty,
    bool &outExist)
{
    bool exist = false;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(napi_has_named_property(env, object, name, &exist));
    if (exist) {
        napi_value property;
        NAPI_NL_CALL_RETURN(napi_get_named_property(env, object, name, &property));
        outProperty = property;
    }
    outExist = exist;
    return napi_ok;
}
napi_status NapiGetObjectProperty(napi_env env, napi_value object, const char *name, napi_value &outProperty)
{
    bool exist = false;
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, name, outProperty, exist));
    NAPI_NL_RETURN_IF(!exist, "no needed property", napi_invalid_arg);
    return napi_ok;
}
napi_status NapiGetObjectPropertyOptional(napi_env env, napi_value object, const char *name, napi_value &outProperty,
    bool &outExist)
{
    return NapiGetObjectProperty(env, object, name, outProperty, outExist);
}

napi_status NapiParseObjectBoolean(napi_env env, napi_value object, const char *name, bool &outBoolean)
{
    napi_value property;
    bool boolean = true;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, name, property));
    NAPI_NL_CALL_RETURN(NapiParseBoolean(env, property, boolean));
    outBoolean = boolean;
    return napi_ok;
}

napi_status NapiParseObjectUuid(napi_env env, napi_value object, const char *name, std::string &outUuid)
{
    napi_value property;
    std::string uuid {};
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, name, property));
    NAPI_NL_CALL_RETURN(NapiParseUuid(env, property, uuid));
    outUuid = std::move(uuid);
    return napi_ok;
}

napi_status NapiParseArrayBuffer(napi_env env, napi_value object, const char *name, std::vector<uint8_t> &outVec)
{
    napi_value property;
    std::vector<uint8_t> vec {};
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, name, property));
    NAPI_NL_CALL_RETURN(NapiParseArrayBuffer(env, property, vec));
    outVec = std::move(vec);
    return napi_ok;
}

napi_status NapiParseObjectInt32(napi_env env, napi_value object, const char *name, int32_t &outNum)
{
    napi_value property;
    int32_t num = 0;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, name, property));
    NAPI_NL_CALL_RETURN(NapiParseInt32(env, property, num));
    outNum = num;
    return napi_ok;
}

napi_status NapiParseObjectUint32(napi_env env, napi_value object, const char *name, uint32_t &outNum)
{
    napi_value property;
    uint32_t num = 0;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, name, property));
    NAPI_NL_CALL_RETURN(NapiParseUint32(env, property, num));
    outNum = num;
    return napi_ok;
}

napi_status NapiParseObjectBooleanOptional(napi_env env, napi_value object, const char *name, bool &outBoolean,
    bool &outExist)
{
    napi_value property;
    bool exist = false;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectPropertyOptional(env, object, name, property, exist));
    if (exist) {
        bool boolean = true;
        NAPI_NL_CALL_RETURN(NapiParseBoolean(env, property, boolean));
        outBoolean = boolean;
    }
    outExist = exist;
    return napi_ok;
}
napi_status NapiParseObjectInt32Optional(napi_env env, napi_value object, const char *name, int32_t &outNum,
    bool &outExist)
{
    napi_value property;
    bool exist = false;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, name, property, exist));
    if (exist) {
        int32_t num = 0;
        NAPI_NL_CALL_RETURN(NapiParseInt32(env, property, num));
        outNum = num;
    }
    outExist = exist;
    return napi_ok;
}
napi_status NapiParseObjectUint32Optional(napi_env env, napi_value object, const char *name, uint32_t &outNum,
    bool &outExist)
{
    napi_value property;
    bool exist = false;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, name, property, exist));
    if (exist) {
        uint32_t num = 0;
        NAPI_NL_CALL_RETURN(NapiParseUint32(env, property, num));
        outNum = num;
    }
    outExist = exist;
    return napi_ok;
}

napi_status NapiParseObjectStringOptional(napi_env env, napi_value object, const char *name, std::string &outString,
    bool &outExist)
{
    napi_value property;
    bool exist = false;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectPropertyOptional(env, object, name, property, exist));
    if (exist) {
        std::string str = {};
        NAPI_NL_CALL_RETURN(NapiParseString(env, property, str));
        outString = str;
    }
    outExist = exist;
    return napi_ok;
}


napi_status NapiParseArrayBufferOptional(napi_env env, napi_value object, const char *name,
    std::vector<uint8_t> &outVec, bool &outExist)
{
    napi_value property;
    bool exist = false;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectPropertyOptional(env, object, name, property, exist));
    if (exist) {
        std::vector<uint8_t> vec = {};
        NAPI_NL_CALL_RETURN(NapiParseArrayBuffer(env, property, vec));
        outVec = std::move(vec);
    }
    outExist = exist;
    return napi_ok;
}

template <typename T>
napi_status NapiParseArrayOptional(napi_env env, napi_value object, const char *name,
    std::vector<T> &outVec, bool &outExist)
{
    napi_value property;
    bool exist = false;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectPropertyOptional(env, object, name, property, exist));
    if (exist) {
        std::vector<T> vec = {};
        NAPI_NL_CALL_RETURN(NapiParseArray(env, property, vec));
        outVec = std::move(vec);
    }
    outExist = exist;
    return napi_ok;
}

template <typename T>
napi_status NapiParseObjectOptional(napi_env env, napi_value object, const char *name,
    T &outObj, bool &outExist)
{
    napi_value property;
    bool exist = false;
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiGetObjectPropertyOptional(env, object, name, property, exist));
    if (exist) {
        NAPI_NL_CALL_RETURN(NapiParseObject(env, property, outObj));
    }
    outExist = exist;
    return napi_ok;
}

// Declaration, others will undefined symbol
template napi_status NapiParseArrayOptional<UUID>(napi_env env, napi_value object, const char *name,
    std::vector<UUID> &outVec, bool &outExist);
template napi_status NapiParseArrayOptional<NapiAdvManufactureData>(napi_env env, napi_value object, const char *name,
    std::vector<NapiAdvManufactureData> &outVec, bool &outExist);
template napi_status NapiParseArrayOptional<NapiAdvServiceData>(napi_env env, napi_value object, const char *name,
    std::vector<NapiAdvServiceData> &outVec, bool &outExist);

// Parse params template, used for NapiParseArray
template <typename T>
napi_status NapiParseObject(napi_env env, napi_value object, T &outObj)
{
    HILOGE("Unimpleted type");
    return napi_invalid_arg;
}

template <>
napi_status NapiParseObject<std::string>(napi_env env, napi_value object, std::string &outObj)
{
    return NapiParseString(env, object, outObj);
}

template <>
napi_status NapiParseObject<UUID>(napi_env env, napi_value object, UUID &outObj)
{
    std::string uuid {};
    int32_t ret = NapiParseUuid(env, object, uuid);
    if (ret != NlErrCode::NL_NO_ERROR) {
        return napi_invalid_arg;
    }
    outObj = UUID::FromString(uuid);
    return napi_ok;
}

template <>
napi_status NapiParseObject<NapiAdvManufactureData>(napi_env env, napi_value object, NapiAdvManufactureData &outObj)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object,
        {"manufacturerId", "manufacturerData"}));
    uint32_t num = 0;
    std::vector<uint8_t> vec {};
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiParseObjectUint32(env, object, "manufacturerId", num));
    NAPI_NL_RETURN_IF(num > 0xFFFF, "Invalid manufacturerId", napi_invalid_arg);
    NAPI_NL_CALL_RETURN(NapiParseArrayBuffer(env, object, "manufacturerData", vec));

    outObj.id = static_cast<uint16_t>(num);
    outObj.value = std::string(vec.begin(), vec.end());
    return napi_ok;
}

template <>
napi_status NapiParseObject<NapiAdvServiceData>(napi_env env, napi_value object, NapiAdvServiceData &outObj)
{
    NAPI_NL_CALL_RETURN(NapiCheckObjectPropertiesName(env, object, {"serviceUuid", "serviceData"}));
    std::string uuid {};
    std::vector<uint8_t> vec {};
    NAPI_NL_CALL_RETURN(NapiIsObject(env, object));
    NAPI_NL_CALL_RETURN(NapiParseObjectUuid(env, object, "serviceUuid", uuid));
    NAPI_NL_CALL_RETURN(NapiParseArrayBuffer(env, object, "serviceData", vec));

    outObj.uuid = std::move(uuid);
    outObj.value = std::move(vec);
    return napi_ok;
}

template <>
napi_status NapiParseObject<NapiSsapService>(napi_env env, napi_value object, NapiSsapService &outObj)
{
    int32_t ret = NapiParseSsapService(env, object, outObj);
    return (ret == NlErrCode::NL_NO_ERROR) ? napi_ok : napi_invalid_arg;
}

template <>
napi_status NapiParseObject<NapiSsapProperty>(napi_env env, napi_value object, NapiSsapProperty &outObj)
{
    int32_t ret = NapiParseSsapProperty(env, object, outObj);
    return (ret == NlErrCode::NL_NO_ERROR) ? napi_ok : napi_invalid_arg;
}

template <>
napi_status NapiParseObject<NapiSsapDescriptor>(napi_env env, napi_value object, NapiSsapDescriptor &outObj)
{
    int32_t ret = NapiParseSsapDescriptor(env, object, outObj);
    return (ret == NlErrCode::NL_NO_ERROR) ? napi_ok : napi_invalid_arg;
}

template <>
napi_status NapiParseObject<NapiSsapServerResponse>(napi_env env, napi_value object, NapiSsapServerResponse &outObj)
{
    return NapiParseSsapServerResponse(env, object, outObj);
}

template <>
napi_status NapiParseObject<NapiSsapMethod>(napi_env env, napi_value object, NapiSsapMethod &outObj)
{
    int32_t ret = NapiParseSsapMethod(env, object, outObj);
    return (ret == NlErrCode::NL_NO_ERROR) ? napi_ok : napi_invalid_arg;
}

template <>
napi_status NapiParseObject<NapiSsapEvent>(napi_env env, napi_value object, NapiSsapEvent &outObj)
{
    int32_t ret = NapiParseSsapEvent(env, object, outObj);
    return (ret == NlErrCode::NL_NO_ERROR) ? napi_ok : napi_invalid_arg;
}

template <typename T>
napi_status NapiParseArray(napi_env env, napi_value array, std::vector<T> &outVec)
{
    std::vector<T> vec {};

    NAPI_NL_CALL_RETURN(NapiIsArray(env, array));
    uint32_t length = 0;
    NAPI_NL_CALL_RETURN(napi_get_array_length(env, array, &length));
    NAPI_NL_RETURN_IF(length > NAPI_ARRAY_MAX_LENGTH, "Array is too long", napi_invalid_arg);
    for (uint32_t i = 0; i < length; ++i) {
        napi_value element;
        NAPI_NL_CALL_RETURN(napi_get_element(env, array, i, &element));
        T object;
        NAPI_NL_CALL_RETURN(NapiParseObject(env, element, object));
        vec.push_back(std::move(object));
    }
    outVec = std::move(vec);
    return napi_ok;
}
// // Declaration, ohters will undefined synbol
template napi_status NapiParseArray<NapiSsapDescriptor>(napi_env env, napi_value array,
    std::vector<NapiSsapDescriptor> &outVec);
template napi_status NapiParseArray<NapiSsapEvent>(napi_env env, napi_value array,
    std::vector<NapiSsapEvent> &outVec);
template napi_status NapiParseArray<NapiSsapMethod>(napi_env env, napi_value array,
    std::vector<NapiSsapMethod> &outVec);
template napi_status NapiParseArray<NapiSsapProperty>(napi_env env, napi_value array,
    std::vector<NapiSsapProperty> &outVec);
template napi_status NapiParseArray<NapiSsapService>(napi_env env, napi_value array,
    std::vector<NapiSsapService> &outVec);
template napi_status NapiParseArray<NapiAdvServiceData>(napi_env env, napi_value array,
    std::vector<NapiAdvServiceData> &outVec);
template napi_status NapiParseArray<NapiAdvManufactureData>(napi_env env, napi_value array,
    std::vector<NapiAdvManufactureData> &outVec);
template napi_status NapiParseArray<UUID>(napi_env env, napi_value array,
    std::vector<UUID> &outVec);
template napi_status NapiParseArray<std::string>(napi_env env, napi_value array,
    std::vector<std::string> &outVec);


template <typename T>
napi_status NapiParseObjectArray(napi_env env, napi_value object, const char *name, std::vector<T> &outVec)
{
    napi_value property;
    std::vector<T> vec {};
    NAPI_NL_CALL_RETURN(NapiGetObjectProperty(env, object, name, property));
    NAPI_NL_CALL_RETURN(NapiParseArray(env, property, vec));
    outVec = std::move(vec);
    return napi_ok;
}
// Declaration, ohters will undefined synbol
template napi_status NapiParseObjectArray<NapiSsapDescriptor>(napi_env env, napi_value object, const char *name,
    std::vector<NapiSsapDescriptor> &outVec);
template napi_status NapiParseObjectArray<NapiSsapEvent>(napi_env env, napi_value object, const char *name,
    std::vector<NapiSsapEvent> &outVec);
template napi_status NapiParseObjectArray<NapiSsapMethod>(napi_env env, napi_value object, const char *name,
    std::vector<NapiSsapMethod> &outVec);
template napi_status NapiParseObjectArray<NapiSsapProperty>(napi_env env, napi_value object, const char *name,
    std::vector<NapiSsapProperty> &outVec);
template napi_status NapiParseObjectArray<NapiSsapService>(napi_env env, napi_value object, const char *name,
    std::vector<NapiSsapService> &outVec);
template napi_status NapiParseObjectArray<NapiAdvServiceData>(napi_env env, napi_value object, const char *name,
    std::vector<NapiAdvServiceData> &outVec);
template napi_status NapiParseObjectArray<NapiAdvManufactureData>(napi_env env, napi_value object, const char *name,
    std::vector<NapiAdvManufactureData> &outVec);
template napi_status NapiParseObjectArray<UUID>(napi_env env, napi_value object, const char *name,
    std::vector<UUID> &outVec);
template napi_status NapiParseObjectArray<std::string>(napi_env env, napi_value object, const char *name,
    std::vector<std::string> &outVec);

template napi_status NapiParseObjectOptional<NapiAdvServiceData>(napi_env env, napi_value object, const char *name,
    NapiAdvServiceData &outObj, bool &outExist);
}  // namespace Nearlink
}  // namespace OHOS
