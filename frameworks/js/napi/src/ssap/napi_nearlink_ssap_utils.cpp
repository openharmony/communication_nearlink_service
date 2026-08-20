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

#include <algorithm>
#include <functional>
#include <optional>
#include "nearlink_errorcode.h"
#include "log_util.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_nearlink_error.h"
#include "securec.h"
#include "napi_nearlink_utils.h"
#include "napi_nearlink_ssap_utils.h"


namespace OHOS {
namespace Nearlink {
void ConvertSsapServiceToJS(napi_env env, napi_value result, SsapService& service)
{
    napi_value serviceUuid;
    napi_create_string_utf8(env, service.GetUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &serviceUuid);
    napi_set_named_property(env, result, "serviceUuid", serviceUuid);

    napi_value properties;
    napi_create_array(env, &properties);
    ConvertSsapPropertyVectorToJS(env, properties, service.GetProperty());
    napi_set_named_property(env, result, "properties", properties);

    napi_value methods;
    napi_create_array(env, &methods);
    ConvertSsapMethodVectorToJS(env, methods, service.GetMethod());
    napi_set_named_property(env, result, "methods", methods);

    napi_value events;
    napi_create_array(env, &events);
    ConvertSsapEventVectorToJS(env, events, service.GetEvent());
    napi_set_named_property(env, result, "events", events);
}

void ConvertSsapServiceVectorToJS(napi_env env, napi_value result, std::vector<SsapService>& services)
{
    HILOGI("enter");
    size_t idx = 0;

    if (services.empty()) {
        return;
    }
    HILOGI("size: %{public}zu", services.size());
    for (auto& service : services) {
        napi_value obj = nullptr;
        napi_create_object(env, &obj);
        ConvertSsapServiceToJS(env, obj, service);
        napi_set_element(env, result, idx, obj);
        idx++;
    }
}

void ConvertSsapPropertyVectorToJS(napi_env env, napi_value result,
    std::vector<SsapProperty>& properties)
{
    HILOGI("size: %{public}zu", properties.size());
    size_t idx = 0;
    if (properties.empty()) {
        return;
    }

    for (auto &property : properties) {
        napi_value obj = nullptr;
        napi_create_object(env, &obj);
        ConvertSsapPropertyToJS(env, obj, property);
        napi_set_element(env, result, idx, obj);
        idx++;
    }
}

void ConvertSsapPropertyToJS(napi_env env, napi_value result, SsapProperty& property)
{
    napi_value propertyUuid;
    HILOGI("uuid: %{public}s", GET_ENCRYPT_UUID(property.GetUuid()));
    napi_create_string_utf8(env, property.GetUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &propertyUuid);
    napi_set_named_property(env, result, "propertyUuid", propertyUuid);

    napi_value serviceUuid;
    HILOGI("serviceUuid: %{public}s", GET_ENCRYPT_UUID(property.GetServiceUuid()));
    napi_create_string_utf8(env, property.GetServiceUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &serviceUuid);
    napi_set_named_property(env, result, "serviceUuid", serviceUuid);

    size_t valueSize = 0;
    uint8_t* valueData = property.GetValue(&valueSize).get();
    napi_value value = nullptr;
    uint8_t* bufferData = nullptr;
    napi_create_arraybuffer(env, valueSize, reinterpret_cast<void **>(&bufferData), &value);
    if (valueSize > 0 && valueData != nullptr && memcpy_s(bufferData, valueSize, valueData, valueSize) != EOK) {
        HILOGE("memcpy_s failed");
        return;
    }
    napi_set_named_property(env, result, "value", value);

    napi_value descriptors;
    napi_create_array(env, &descriptors);
    ConvertSsapDescriptorVectorToJS(env, descriptors, property.GetDescriptors(), property);
    napi_set_named_property(env, result, "descriptors", descriptors);

    napi_value operation;
    /*
     * the last byte of operationindication indicates the property's operation,
     * other bytes indicates writeable for every descriptor in property
     */
    napi_create_uint32(env, static_cast<unsigned int>(property.GetOperationIndication() & 0xFF), &operation);
    napi_set_named_property(env, result, "operation", operation);
    HILOGI("handle: %{public}d", property.GetOperationIndication());
}

void ConvertSsapMethodVectorToJS(napi_env env, napi_value result,
    std::vector<SsapMethod>& methods)
{
    HILOGI("methods size: %{public}zu", methods.size());
    size_t idx = 0;
    if (methods.empty()) {
        return;
    }

    for (auto &method : methods) {
        napi_value obj = nullptr;
        napi_create_object(env, &obj);
        ConvertSsapMethodToJS(env, obj, method);
        napi_set_element(env, result, idx, obj);
        idx++;
    }
}

void ConvertSsapMethodToJS(napi_env env, napi_value result, SsapMethod& method)
{
    napi_value methodUuid;
    HILOGI("uuid: %{public}s", GET_ENCRYPT_UUID(method.GetUuid()));
    napi_create_string_utf8(env, method.GetUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &methodUuid);
    napi_set_named_property(env, result, "methodUuid", methodUuid);

    napi_value serviceUuid;
    HILOGI("serviceUuid: %{public}s", GET_ENCRYPT_UUID(method.GetServiceUuid()));
    napi_create_string_utf8(env, method.GetServiceUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &serviceUuid);
    napi_set_named_property(env, result, "serviceUuid", serviceUuid);

    size_t parameterSize = 0;
    uint8_t* parameterData = method.GetParameter(&parameterSize).get();
    napi_value parameter = nullptr;
    uint8_t* bufferData = nullptr;
    napi_create_arraybuffer(env, parameterSize, reinterpret_cast<void **>(&bufferData), &parameter);
    if (parameterSize > 0 && memcpy_s(bufferData, parameterSize, parameterData, parameterSize) != EOK) {
        HILOGE("memcpy_s failed");
        return;
    }
    napi_set_named_property(env, result, "parameter", parameter);

    size_t resultSize = 0;
    uint8_t* resultData = method.GetResult(&resultSize).get();
    napi_value ret = nullptr;
    uint8_t* retbufferData = nullptr;
    napi_create_arraybuffer(env, resultSize, reinterpret_cast<void **>(&retbufferData), &ret);
    if (resultSize > 0 && memcpy_s(retbufferData, resultSize, resultData, resultSize) != EOK) {
        HILOGE("memcpy_s failed");
        return;
    }
    napi_set_named_property(env, result, "result", ret);
}

void ConvertSsapEventVectorToJS(napi_env env, napi_value result,
    std::vector<SsapEvent>& events)
{
    HILOGI("events' size: %{public}zu", events.size());
    size_t idx = 0;
    if (events.empty()) {
        return;
    }

    for (auto &event : events) {
        napi_value obj = nullptr;
        napi_create_object(env, &obj);
        ConvertSsapEventToJS(env, obj, event);
        napi_set_element(env, result, idx, obj);
        idx++;
    }
}

void ConvertSsapEventToJS(napi_env env, napi_value result, SsapEvent& event)
{
    napi_value eventUuid;
    HILOGI("uuid: %{public}s", GET_ENCRYPT_UUID(event.GetUuid()));
    napi_create_string_utf8(env, event.GetUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &eventUuid);
    napi_set_named_property(env, result, "eventUuid", eventUuid);

    napi_value serviceUuid;
    HILOGI("serviceUuid: %{public}s", GET_ENCRYPT_UUID(event.GetServiceUuid()));
    napi_create_string_utf8(env, event.GetServiceUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &serviceUuid);
    napi_set_named_property(env, result, "serviceUuid", serviceUuid);

    size_t parameterSize = 0;
    uint8_t* parameterData = event.GetParameter(&parameterSize).get();
    napi_value parameter = nullptr;
    uint8_t* bufferData = nullptr;
    napi_create_arraybuffer(env, parameterSize, reinterpret_cast<void **>(&bufferData), &parameter);
    if (parameterSize > 0 && memcpy_s(bufferData, parameterSize, parameterData, parameterSize) != EOK) {
        HILOGE("memcpy_s failed");
        return;
    }
    napi_set_named_property(env, result, "parameter", parameter);
}

void ConvertSsapDescriptorVectorToJS(napi_env env, napi_value result, std::vector<SsapDescriptor>& descriptors,
                                     SsapProperty& property)
{
    HILOGI("size: %{public}zu", descriptors.size());
    size_t idx = 0;

    if (descriptors.empty()) {
        return;
    }

    for (auto& descriptor : descriptors) {
        napi_value obj = nullptr;
        napi_create_object(env, &obj);
        ConvertSsapDescriptorToJS(env, obj, descriptor, idx, property.GetOperationIndication());
        napi_set_element(env, result, idx, obj);
        idx++;
    }
}

void ConvertSsapDescriptorCommonToJS(napi_env env, napi_value result, SsapDescriptor& descriptor)
{
    napi_value propertyUuid;
    HILOGI("uuid: %{public}s", GET_ENCRYPT_UUID(descriptor.GetPropertyUuid()));
    napi_create_string_utf8(env, descriptor.GetPropertyUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &propertyUuid);
    napi_set_named_property(env, result, "propertyUuid", propertyUuid);

    napi_value serviceUuid;
    HILOGI("serviceUuid: %{public}s", GET_ENCRYPT_UUID(descriptor.GetServiceUuid()));
    napi_create_string_utf8(env, descriptor.GetServiceUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &serviceUuid);
    napi_set_named_property(env, result, "serviceUuid", serviceUuid);

    napi_value descriptorType;
    napi_create_int32(env, descriptor.GetDescriptorType(), &descriptorType);
    napi_set_named_property(env, result, "descriptorType", descriptorType);
    HILOGI("descriptorType: %{public}d", descriptor.GetDescriptorType());

    napi_value value = nullptr;
    size_t valueSize = 0;
    uint8_t* valueData = descriptor.GetValue(&valueSize).get();
    uint8_t* bufferData = nullptr;
    napi_create_arraybuffer(env, valueSize, reinterpret_cast<void **>(&bufferData), &value);
    if (valueSize > 0 && valueData != nullptr && memcpy_s(bufferData, valueSize, valueData, valueSize) != EOK) {
        HILOGE("memcpy_s error");
    }
    napi_set_named_property(env, result, "value", value);
}

void ConvertSsapDescriptorToJS(napi_env env, napi_value result, SsapDescriptor& descriptor, size_t idx,
                               uint32_t operationIndication)
{
    ConvertSsapDescriptorCommonToJS(env, result, descriptor);

    napi_value writable;
    bool isWritable = (operationIndication & (0x0100U << idx)) != 0;
    napi_get_boolean(env, isWritable, &writable);
    napi_set_named_property(env, result, "isWriteable", writable);
}

void ConvertSsapPropertyReadRequestToJS(napi_env env, napi_value result, std::string deviceId,
    SsapProperty& property, int requestId)
{
    napi_value value = nullptr;
    napi_create_string_utf8(env, deviceId.c_str(), NAPI_AUTO_LENGTH, &value);
    std::string propertyName = "address";
    napi_set_named_property(env, result, propertyName.c_str(), value);

    napi_value propertyUuid;
    HILOGI("uuid: %{public}s", GET_ENCRYPT_UUID(property.GetUuid()));
    napi_create_string_utf8(env, property.GetUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &propertyUuid);
    napi_set_named_property(env, result, "propertyUuid", propertyUuid);

    napi_value serviceUuid;
    HILOGI("serviceUuid: %{public}s", GET_ENCRYPT_UUID(property.GetServiceUuid()));
    napi_create_string_utf8(env, property.GetServiceUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &serviceUuid);
    napi_set_named_property(env, result, "serviceUuid", serviceUuid);

    napi_value requestIdValue;
    napi_create_uint32(env, static_cast<unsigned int>(requestId), &requestIdValue);
    napi_set_named_property(env, result, "requestId", requestIdValue);
    HILOGI("requestId: %{public}d", requestId);
}

void ConvertSsapPropertyWriteRequestToJS(napi_env env, napi_value result, std::string deviceId,
    SsapProperty& property, int requestId)
{
    napi_value addressValue = nullptr;
    napi_create_string_utf8(env, deviceId.c_str(), NAPI_AUTO_LENGTH, &addressValue);
    std::string propertyName = "address";
    napi_set_named_property(env, result, propertyName.c_str(), addressValue);

    napi_value propertyUuid;
    HILOGI("uuid: %{public}s", GET_ENCRYPT_UUID(property.GetUuid()));
    napi_create_string_utf8(env, property.GetUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &propertyUuid);
    napi_set_named_property(env, result, "propertyUuid", propertyUuid);

    napi_value serviceUuid;
    HILOGI("serviceUuid: %{public}s", GET_ENCRYPT_UUID(property.GetServiceUuid()));
    napi_create_string_utf8(env, property.GetServiceUuid().ToString().c_str(), NAPI_AUTO_LENGTH, &serviceUuid);
    napi_set_named_property(env, result, "serviceUuid", serviceUuid);

    size_t valueSize = 0;
    uint8_t* valueData = property.GetValue(&valueSize).get();
    napi_value value = nullptr;
    uint8_t* bufferData = nullptr;
    napi_create_arraybuffer(env, valueSize, reinterpret_cast<void **>(&bufferData), &value);
    if (valueSize > 0 && valueData != nullptr && memcpy_s(bufferData, valueSize, valueData, valueSize) != EOK) {
        HILOGE("memcpy_s failed");
        return;
    }
    napi_set_named_property(env, result, "value", value);

    napi_value requestIdValue;
    napi_create_uint32(env, static_cast<unsigned int>(requestId), &requestIdValue);
    napi_set_named_property(env, result, "requestId", requestIdValue);
    HILOGI("requestId: %{public}d", requestId);

    napi_value writeTypeValue;
    napi_create_int32(env, static_cast<int32_t>(property.GetWriteType()), &writeTypeValue);
    napi_set_named_property(env, result, "writeType", writeTypeValue);
    HILOGI("writeType: %{public}d", property.GetWriteType());
}

napi_value NapiNativeSsapServiceArray::ToNapiValue(napi_env env) const
{
    napi_value object;
    napi_create_array(env, &object);
    ConvertSsapServiceVectorToJS(env, object, const_cast<std::vector<SsapService> &>(ssapServices_));
    return object;
}

napi_value NapiNativeSsapConnectionState::ToNapiValue(napi_env env) const
{
    napi_value object;
    napi_create_object(env, &object);
    napi_value value = nullptr;
    napi_create_string_utf8(env, deviceId_.c_str(), NAPI_AUTO_LENGTH, &value);
    std::string propertyName = "address";
    napi_set_named_property(env, object, propertyName.c_str(), value);
    napi_create_int32(env, state_, &value);
    napi_set_named_property(env, object, "state", value);
    return object;
}

napi_value NapiNativeSsapProperty::ToNapiValue(napi_env env) const
{
    napi_value object = nullptr;
    napi_create_object(env, &object);
    SsapProperty innerProperty(property_);
    ConvertSsapPropertyToJS(env, object, innerProperty);
    return object;
}

napi_value NapiNativeSsapMethod::ToNapiValue(napi_env env) const
{
    napi_value object = nullptr;
    napi_create_object(env, &object);
    SsapMethod innerMethod(method_);
    ConvertSsapMethodToJS(env, object, innerMethod);
    return object;
}

napi_value NapiNativeSsapEvent::ToNapiValue(napi_env env) const
{
    napi_value object = nullptr;
    napi_create_object(env, &object);
    SsapEvent innerEvent(event_);
    ConvertSsapEventToJS(env, object, innerEvent);
    return object;
}

napi_value NapiNativeSsapPropertyArray::ToNapiValue(napi_env env) const
{
    napi_value object;
    napi_create_array(env, &object);
    ConvertSsapPropertyVectorToJS(env, object, const_cast<std::vector<SsapProperty> &>(ssapProperties_));
    return object;
}

napi_value NapiNativeSsapDescriptor::ToNapiValue(napi_env env) const
{
    napi_value object = nullptr;
    napi_create_object(env, &object);
    SsapDescriptor innerDescriptor(descriptor_);
    ConvertSsapDescriptorCommonToJS(env, object, innerDescriptor);
    return object;
}

napi_value NapiNativeSsapPropertyReadRequest::ToNapiValue(napi_env env) const
{
    napi_value object = nullptr;
    napi_create_object(env, &object);
    SsapProperty innerProperty(property_);
    ConvertSsapPropertyReadRequestToJS(env, object, deviceId_, innerProperty, requestId_);
    return object;
}

napi_value NapiNativeSsapPropertyWriteRequest::ToNapiValue(napi_env env) const
{
    napi_value object = nullptr;
    napi_create_object(env, &object);
    SsapProperty innerProperty(property_);
    ConvertSsapPropertyWriteRequestToJS(env, object, deviceId_, innerProperty, requestId_);
    return object;
}
}  // namespace Nearlink
}  // namespace OHOS
