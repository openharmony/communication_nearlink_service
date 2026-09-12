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

#include "ani_nearlink_ssap_utils.h"
#include "nearlink_ssap_client.h"
#include "nearlink_ssap_server.h"
#include "nearlink_ssap_service.h"
#include "nearlink_ssap_property.h"
#include "nearlink_ssap_method.h"
#include "nearlink_ssap_event.h"
#include "nearlink_ssap_descriptor.h"
#include "nearlink_utils.h"
#include "log_util.h"
#include "ani_nearlink_error.h"
#include "ani_nearlink_utils.h"

namespace OHOS {
namespace Nearlink {
const std::string BASE_UUID = "37BEA880-FC70-11EA-B720-00000000";
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

void CheckDescriptorWriteOp(const AniSsapDescriptor &descriptor, uint32_t &operationIndication,
    uint32_t &opDescriptorBitMask)
{
    if (descriptor.writeable_) {
        operationIndication = opDescriptorBitMask | operationIndication;
    }
    opDescriptorBitMask = opDescriptorBitMask << 1;
}

::taihe::array<uint8_t> ConvertValueArray(const std::unique_ptr<uint8_t[]> &value, size_t size)
{
    return ::taihe::array<uint8_t>(::taihe::copy_data_t{}, value.get(), size);
}

::ohos::nearlink::ssap::PropertyDescriptor ConvertDescriptorToTaihe(const SsapDescriptor &descriptor)
{
    size_t size = 0;
    auto& vec = descriptor.GetValue(&size);
    auto value = ConvertValueArray(vec, size);
    ::ohos::nearlink::ssap::PropertyDescriptor result = {
        .serviceUuid = static_cast<::taihe::string>(descriptor.GetServiceUuid().ToString()),
        .propertyUuid = static_cast<::taihe::string>(descriptor.GetPropertyUuid().ToString()),
        .value = value,
        .descriptorType =
            ohos::nearlink::ssap::PropertyDescriptorType::from_value(descriptor.GetDescriptorType()),
        .isWriteable = taihe::optional<bool>(std::in_place_t{}, true)
    };

    return result;
}

::ohos::nearlink::ssap::Property ConvertPropertyToTaihe(const SsapProperty &property)
{
    auto& p = const_cast<SsapProperty &>(property);
    size_t size = 0;
    auto& vec = p.GetValue(&size);
    auto value = ConvertValueArray(vec, size);
    std::vector<::ohos::nearlink::ssap::PropertyDescriptor> descriptors;
    for (const auto &descriptor : p.GetDescriptors()) {
        descriptors.emplace_back(ConvertDescriptorToTaihe(descriptor));
    }
    ::taihe::array<::ohos::nearlink::ssap::PropertyDescriptor> dspVec =
        ::taihe::array<::ohos::nearlink::ssap::PropertyDescriptor>(::taihe::copy_data_t{},
            descriptors.data(), descriptors.size());
    ::ohos::nearlink::ssap::Property result = {
        .serviceUuid = static_cast<::taihe::string>(p.GetServiceUuid().ToString()),
        .propertyUuid = static_cast<::taihe::string>(p.GetUuid().ToString()),
        .value = value,
        .descriptors =
            taihe::optional<::taihe::array<::ohos::nearlink::ssap::PropertyDescriptor>>(std::in_place_t{}, dspVec),
        .operation = taihe::optional<int32_t>(std::in_place_t{}, p.GetOperationIndication())
    };
    return result;
}

::ohos::nearlink::ssap::Method ConvertMethodToTaihe(const SsapMethod &method)
{
    size_t parameterSize = 0;
    auto& pVec = method.GetParameter(&parameterSize);
    auto parameter = ConvertValueArray(pVec, parameterSize);
    size_t resultSize = 0;
    auto& rVec = method.GetResult(&resultSize);
    auto result = ConvertValueArray(rVec, resultSize);
    ::ohos::nearlink::ssap::Method res = {
        .serviceUuid = static_cast<::taihe::string>(method.GetServiceUuid().ToString()),
        .methodUuid = static_cast<::taihe::string>(method.GetUuid().ToString()),
        .parameter = taihe::optional<::taihe::array<uint8_t>>(std::in_place_t{}, parameter),
        .result = taihe::optional<::taihe::array<uint8_t>>(std::in_place_t{}, result)
    };
    return res;
}

::ohos::nearlink::ssap::Event ConvertEventToTaihe(const SsapEvent &event)
{
    size_t parameterSize = 0;
    auto& vec = event.GetParameter(&parameterSize);
    auto parameter = ConvertValueArray(vec, parameterSize);
    ::ohos::nearlink::ssap::Event result = {
        .serviceUuid = static_cast<::taihe::string>(event.GetServiceUuid().ToString()),
        .eventUuid = static_cast<::taihe::string>(event.GetUuid().ToString()),
        .parameter = taihe::optional<::taihe::array<uint8_t>>(std::in_place_t{}, parameter)
    };
    return result;
}

::ohos::nearlink::ssap::Service ConvertServiceToTaihe(const SsapService &service)
{
    auto& s = const_cast<SsapService &>(service);
    std::vector<::ohos::nearlink::ssap::Property> properties;
    for (const auto &property : s.GetProperty()) {
        properties.emplace_back(ConvertPropertyToTaihe(property));
    }
    std::vector<::ohos::nearlink::ssap::Method> methods;
    for (const auto &method : s.GetMethod()) {
        methods.emplace_back(ConvertMethodToTaihe(method));
    }
    auto methodsVec = ::taihe::array<::ohos::nearlink::ssap::Method>(
        ::taihe::copy_data_t{}, methods.data(), methods.size());
    std::vector<::ohos::nearlink::ssap::Event> events;
    for (const auto &event : s.GetEvent()) {
        events.emplace_back(ConvertEventToTaihe(event));
    }
    auto eventsVec = ::taihe::array<::ohos::nearlink::ssap::Event>(
        ::taihe::copy_data_t{}, events.data(), events.size());
    ::ohos::nearlink::ssap::Service result = {
        .serviceUuid = static_cast<::taihe::string>(s.GetUuid().ToString()),
        .properties = ::taihe::array<::ohos::nearlink::ssap::Property>(
            ::taihe::copy_data_t{}, properties.data(), properties.size()),
        .methods = taihe::optional<::taihe::array<::ohos::nearlink::ssap::Method>>(std::in_place_t{}, methodsVec),
        .events = taihe::optional<::taihe::array<::ohos::nearlink::ssap::Event>>(std::in_place_t{}, eventsVec)
    };
    return result;
}

int32_t ConvertSsapDescriptorType(int32_t inType, int32_t &outType)
{
    if (inType == static_cast<int32_t>(AniSsapDescriptorType::PROPERTY)) {
        HILOGI("descriptorType: PROPERTY");
        outType = SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_PROPERTY;
    } else if (inType == static_cast<int32_t>(AniSsapDescriptorType::CLIENT_PROPERTY_CONFIG)) {
        HILOGI("descriptorType: CLIENT_PROPERTY_CONFIG");
        outType = SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_CLIENT_PROPERTY_CONFIG;
    } else if (inType == static_cast<int32_t>(AniSsapDescriptorType::SERVER_PROPERTY_CONFIG)) {
        HILOGI("descriptorType: SERVER_PROPERTY_CONFIG");
        outType = SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_SERVER_PROPERTY_CONFIG;
    } else if (inType == static_cast<int32_t>(AniSsapDescriptorType::PROPERTY_FORMAT)) {
        HILOGI("descriptorType: PROPERTY_FORMAT");
        outType = SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_PROPERTY_FORMAT;
    } else if (inType == static_cast<int32_t>(AniSsapDescriptorType::TYPE_VENDOR)) {
        HILOGI("descriptorType: TYPE_VENDOR");
        outType = SsapDescriptor::PropertyDescriptorType::DESCRIPTOR_TYPE_VENDOR;
    } else {
        HILOGE("Invalid descriptorType: %{public}d", inType);
        return NL_ERR_INVALID_PARAM;
    }
    return NL_NO_ERROR;
}

int32_t ConvertDescriptorToNative(const ::ohos::nearlink::ssap::PropertyDescriptor &descriptor,
    AniSsapDescriptor &aniDescriptor)
{
    std::string serviceUuid = std::string(descriptor.serviceUuid);
    std::string propertyUuid = std::string(descriptor.propertyUuid);
    if (!IsValidUuid(serviceUuid) || !IsValidUuid(propertyUuid)) {
        HILOGE("Invalid serviceUuid or propertyUuid");
        HandleSyncErr(NL_ERR_INVALID_UUID);
        return NL_ERR_INVALID_UUID;
    }
    aniDescriptor.serviceUuid_ = UUID::FromString(serviceUuid);
    aniDescriptor.propertyUuid_ = UUID::FromString(propertyUuid);
    if (CheckBaseUuid(serviceUuid) || CheckBaseUuid(propertyUuid)) {
        HandleSyncErr(NL_ERR_STANDARD_UUID_NOT_ALLOWED);
        return NL_ERR_STANDARD_UUID_NOT_ALLOWED;
    }
    aniDescriptor.descriptorValue_.assign(descriptor.value.begin(), descriptor.value.end());
    aniDescriptor.descriptorType_ = static_cast<int32_t>(AniSsapDescriptorType::PROPERTY);
    int32_t type = descriptor.descriptorType.get_value();
    if (ConvertSsapDescriptorType(type, aniDescriptor.descriptorType_) != NL_NO_ERROR) {
        HILOGE("ConvertSsapDescriptorType failed");
        return NL_ERR_INVALID_PARAM;
    }

    aniDescriptor.writeable_ = true;
    if (descriptor.isWriteable.has_value()) {
        aniDescriptor.writeable_ = descriptor.isWriteable.value();
    }
    return NL_NO_ERROR;
}

int32_t ConvertPropertyToNative(const ::ohos::nearlink::ssap::Property &property,
    AniSsapProperty &aniProperty)
{
    std::string serviceUuid = std::string(property.serviceUuid);
    std::string propertyUuid = std::string(property.propertyUuid);
    if (!IsValidUuid(serviceUuid) || !IsValidUuid(propertyUuid)) {
        HILOGE("Invalid serviceUuid or propertyUuid");
        HandleSyncErr(NL_ERR_INVALID_UUID);
        return NL_ERR_INVALID_UUID;
    }
    ConvertUuidToUpperCase(serviceUuid);
    ConvertUuidToUpperCase(propertyUuid);
    aniProperty.serviceUuid_ = UUID::FromString(serviceUuid);
    aniProperty.propertyUuid_ = UUID::FromString(propertyUuid);

    aniProperty.value_.assign(property.value.begin(), property.value.end());
    if (property.descriptors.has_value()) {
        TAIHE_NEARLINK_RETURN_IF(property.descriptors->size() > ANI_ARRAY_MAX_LENGTH,
            "Array is too long", NL_ERR_INVALID_PARAM);
        for (const auto &descriptor : property.descriptors.value()) {
            AniSsapDescriptor aniDescriptor;
            int32_t ret = ConvertDescriptorToNative(descriptor, aniDescriptor);
            if (ret != NL_NO_ERROR) {
                return ret;
            }
            aniProperty.descriptors_.emplace_back(aniDescriptor);
        }
    }
    aniProperty.operation_ = OPERATION_READ | OPERATION_WRITE_NO_RESPONSE;
    if (property.operation.has_value()) {
        aniProperty.operation_ = property.operation.value();
    }
    if (CheckBaseUuid(serviceUuid) || CheckBaseUuid(propertyUuid)) {
        HandleSyncErr(NL_ERR_STANDARD_UUID_NOT_ALLOWED);
        return NL_ERR_STANDARD_UUID_NOT_ALLOWED;
    }
    return NL_NO_ERROR;
}

int32_t ConvertMethodToNative(const ::ohos::nearlink::ssap::Method &method, AniSsapMethod &aniMethod)
{
    std::string serviceUuid = std::string(method.serviceUuid);
    std::string methodUuid = std::string(method.methodUuid);
    aniMethod.serviceUuid_ = UUID::FromString(serviceUuid);
    aniMethod.methodUuid_ = UUID::FromString(methodUuid);

    if (method.parameter.has_value()) {
        aniMethod.parameter_.assign(method.parameter.value().begin(), method.parameter.value().end());
    }
    if (method.result.has_value()) {
        aniMethod.result_.assign(method.result.value().begin(), method.result.value().end());
    }
    if (CheckBaseUuid(serviceUuid) || CheckBaseUuid(methodUuid)) {
        return NL_ERR_STANDARD_UUID_NOT_ALLOWED;
    }
    return NL_NO_ERROR;
}

int32_t ConvertEventToNative(const ::ohos::nearlink::ssap::Event &event, AniSsapEvent &aniEvent)
{
    std::string serviceUuid = std::string(event.serviceUuid);
    std::string eventUuid = std::string(event.eventUuid);
    aniEvent.serviceUuid_ = UUID::FromString(serviceUuid);
    aniEvent.uuid_ = UUID::FromString(eventUuid);

    aniEvent.handle_ = 0;
    if (event.parameter.has_value()) {
        auto& param = event.parameter.value();
        if (param.size() == 0) {
            return NL_ERR_INVALID_PARAM;
        }
        aniEvent.handle_ = static_cast<uint16_t>(param[0]);
    }

    if (CheckBaseUuid(serviceUuid) || CheckBaseUuid(eventUuid)) {
        return NL_ERR_STANDARD_UUID_NOT_ALLOWED;
    }
    return NL_NO_ERROR;
}

int32_t ConvertServiceToNative(const ::ohos::nearlink::ssap::Service &service, AniSsapService &aniService)
{
    std::string serviceUuid = std::string(service.serviceUuid);
    if (!IsValidUuid(serviceUuid)) {
        HILOGE("Invalid serviceUuid");
        HandleSyncErr(NL_ERR_INVALID_UUID);
        return NL_ERR_INVALID_UUID;
    }
    ConvertUuidToUpperCase(serviceUuid);
    aniService.uuid_ = UUID::FromString(serviceUuid);

    for (const auto &property : service.properties) {
        AniSsapProperty aniProperty;
        int32_t ret = ConvertPropertyToNative(property, aniProperty);
        if (ret != NL_NO_ERROR) {
            return ret;
        }
        aniService.properties_.emplace_back(aniProperty);
    }
    if (service.methods.has_value()) {
        for (const auto &method : service.methods.value()) {
            AniSsapMethod aniMethod;
            int32_t ret = ConvertMethodToNative(method, aniMethod);
            if (ret != NL_NO_ERROR) {
                return ret;
            }
            aniService.methods_.emplace_back(aniMethod);
        }
    }

    if (service.events.has_value()) {
        for (const auto &event : service.events.value()) {
            AniSsapEvent aniEvent;
            int32_t ret = ConvertEventToNative(event, aniEvent);
            if (ret != NL_NO_ERROR) {
                return ret;
            }
            aniService.events_.emplace_back(aniEvent);
        }
    }
    if (CheckBaseUuid(serviceUuid)) {
        HandleSyncErr(NL_ERR_STANDARD_UUID_NOT_ALLOWED);
        return NL_ERR_STANDARD_UUID_NOT_ALLOWED;
    }
    return NL_NO_ERROR;
}

int32_t AniSsapServiceToSsapService(const AniSsapService &aniService, std::unique_ptr<SsapService> &outService)
{
    const int mDes = 24;
    outService = std::make_unique<SsapService>(aniService.uuid_, SsapServiceType::VENDOR_PROMARY);
    for (auto &aniProperty : aniService.properties_) {
        uint32_t propOperationIndication = aniProperty.operation_;
        propOperationIndication |= OPERATION_WRITE_CLIENT_CONFIG_MASK; // 允许写cpcd操作
        SsapProperty property(SsapProperty::PropertyType::ENTRY_TYPE_VENDOR_PROPERTY,
            aniProperty.propertyUuid_, propOperationIndication, 0);
        property.SetValue(aniProperty.value_.data(), aniProperty.value_.size());
        if (aniProperty.descriptors_.size() > mDes) {
            HILOGE("num of descriptor out of range");
            return NL_ERR_INVALID_PARAM;
        }
        uint32_t opDescriptorBitMask = 256;
        for (auto &aniDescriptor : aniProperty.descriptors_) {
            HILOGI("check descriptor");
            CheckDescriptorWriteOp(aniDescriptor, propOperationIndication, opDescriptorBitMask);
            SsapDescriptor descriptor(aniDescriptor.descriptorType_, 0);
            descriptor.SetServiceUuid(aniService.uuid_);
            descriptor.SetValue(aniDescriptor.descriptorValue_.data(), aniDescriptor.descriptorValue_.size());
            property.AddDescriptor(descriptor);
        }
        property.SetOperationIndication(propOperationIndication);
        outService->AddProperty(property);
    }
    return NL_NO_ERROR;
}

int32_t ConvertTaiheServerResponseToNative(
    const ::ohos::nearlink::ssap::ServerResponse &response, AniSsapServerResponse &aniResponse)
{
    std::string address = std::string(response.address);
    aniResponse.address_ = address;
    aniResponse.requestId_ = static_cast<int>(response.requestId);
    aniResponse.value_.assign(response.value.begin(), response.value.end());
    return NL_NO_ERROR;
}

::ohos::nearlink::ssap::ConnectionChangeState ConvertChangeStateToTaihe(
    const NearlinkRemoteDevice &device, int state)
{
    return ::ohos::nearlink::ssap::ConnectionChangeState {
        .address = static_cast<::taihe::string>(device.GetDeviceAddr()),
        .state = ohos::nearlink::constant::ConnectionState::from_value(state)
    };
}

::ohos::nearlink::ssap::PropertyReadRequest ConvertReadRequestToTaihe(
    const NearlinkRemoteDevice &device, const SsapProperty &property, int requestId)
{
    ::ohos::nearlink::ssap::PropertyReadRequest result = {
        .address = static_cast<::taihe::string>(device.GetDeviceAddr()),
        .serviceUuid = static_cast<::taihe::string>(property.GetServiceUuid().ToString()),
        .propertyUuid = static_cast<::taihe::string>(property.GetUuid().ToString()),
        .requestId = requestId
    };
    return result;
}

::ohos::nearlink::ssap::PropertyWriteRequest ConvertWriteRequestToTaihe(
    const NearlinkRemoteDevice &device, const SsapProperty &property, int requestId)
{
    size_t size = 0;
    auto& vec = property.GetValue(&size);
    auto value = ConvertValueArray(vec, size);
    ::ohos::nearlink::ssap::PropertyWriteRequest result = {
        .address = static_cast<::taihe::string>(device.GetDeviceAddr()),
        .serviceUuid = static_cast<::taihe::string>(property.GetServiceUuid().ToString()),
        .propertyUuid = static_cast<::taihe::string>(property.GetUuid().ToString()),
        .value = value,
        .requestId = requestId,
        .writeType = ohos::nearlink::ssap::PropertyWriteType::from_value(property.GetWriteType())
    };
    return result;
}
}  // namespace Nearlink
}  // namespace OHOS