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

#include "nearlink_ssap_client_callback_stub.h"
#include "log.h"
#include "raw_address.h"
#include "nearlink_ssap_service_parcel.h"

namespace OHOS {
namespace Nearlink {
constexpr int PROPERTY_READ_BY_UUID_MAX_SIZE = 255;
constexpr int SERVICE_MAX_NUMBER = 255;
NearlinkSsapClientCallbackStub::NearlinkSsapClientCallbackStub()
{
    InitConnectionFuncMap();
    InitPropertyFuncMap();
}

void NearlinkSsapClientCallbackStub::InitConnectionFuncMap()
{
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_CONNECT_STATE_CHANGE)] =
        &NearlinkSsapClientCallbackStub::OnConnectionStateChangedInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_MTU_UPDATE)] =
        &NearlinkSsapClientCallbackStub::OnMtuChangedInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_SERVICES_DISCOVER)] =
        &NearlinkSsapClientCallbackStub::OnServicesDiscoveredInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_SERVICES_DISCOVER_BY_UUID)] =
        &NearlinkSsapClientCallbackStub::OnServicesDiscoveredByUuidInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_SERVICES_REDISCOVERED)] =
        &NearlinkSsapClientCallbackStub::OnServicesRediscoveredInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_SERVICE_CHANGED)] =
        &NearlinkSsapClientCallbackStub::OnServiceChangedInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_CONNECTION_PARA_CHANGE)] =
        &NearlinkSsapClientCallbackStub::OnConnectionParameterChangedInner;
}

void NearlinkSsapClientCallbackStub::InitPropertyFuncMap()
{
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_CHANGE)] =
        &NearlinkSsapClientCallbackStub::OnPropertyChangedInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_READ)] =
        &NearlinkSsapClientCallbackStub::OnPropertyReadInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_READ_BY_UUID)] =
        &NearlinkSsapClientCallbackStub::OnPropertiesReadByUuidInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_WRITE)] =
        &NearlinkSsapClientCallbackStub::OnPropertyWriteInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_DESCRIPTOR_READ)] =
        &NearlinkSsapClientCallbackStub::OnDescriptorReadInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_DESCRIPTOR_WRITE)] =
        &NearlinkSsapClientCallbackStub::OnDescriptorWriteInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_SET_NOTIFY)] =
        &NearlinkSsapClientCallbackStub::OnSetPropertyNotifyInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_METHOD_CALL)] =
        &NearlinkSsapClientCallbackStub::OnMethodCallInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_EVENT_NOTIFY)] =
        &NearlinkSsapClientCallbackStub::OnEventNotifyInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_SET_INDICATE)] =
        &NearlinkSsapClientCallbackStub::OnSetPropertyIndicateInner;
}

NearlinkSsapClientCallbackStub::~NearlinkSsapClientCallbackStub()
{
    HILOGD("start.");
    memberFuncMap_.clear();
}

int NearlinkSsapClientCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    HILOGD("NearlinkSsapClientCallbackStub::OnRemoteRequest, cmd = %{public}d, flags= %{public}d",
        code, option.GetFlags());
    if (NearlinkSsapClientCallbackStub::GetDescriptor() != data.ReadInterfaceToken()) {
        HILOGI("local descriptor is not equal to remote");
        return ERR_INVALID_STATE;
    }
    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }
    HILOGW("NearlinkSsapClientCallbackStub::OnRemoteRequest, default case, need check.");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode NearlinkSsapClientCallbackStub::OnConnectionStateChangedInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkSsapClientCallbackStub::OnConnectionStateChangedInner Triggered!");
    int32_t state = data.ReadInt32();
    int32_t newState = data.ReadInt32();
    OnConnectionStateChanged(state, newState);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnPropertyChangedInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkSsapPropertyParcel> property(data.ReadParcelable<NearlinkSsapPropertyParcel>());
    if (!property) {
        return TRANSACTION_ERR;
    }
    OnPropertyChanged(*property);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnEventNotifyInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkSsapEventParcel> event(data.ReadParcelable<NearlinkSsapEventParcel>());
    if (!event) {
        return TRANSACTION_ERR;
    }
    OnEventNotified(*event);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnPropertyReadInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnPropertyReadInner Triggered!");
    int32_t ret = data.ReadInt32();
    std::shared_ptr<NearlinkSsapPropertyParcel> property(data.ReadParcelable<NearlinkSsapPropertyParcel>());
    if (!property) {
        return TRANSACTION_ERR;
    }
    OnReadProperty(ret, *property);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnMethodCallInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnMethodCallInner Triggered!");
    int32_t ret = data.ReadInt32();
    std::shared_ptr<NearlinkSsapMethodParcel> method(data.ReadParcelable<NearlinkSsapMethodParcel>());
    if (!method) {
        return TRANSACTION_ERR;
    }
    HILOGI("%{public}s", method->uuid_.GetEncryptUuid().c_str());
    method->result_.swap(method->parameter_);
    OnCallMethod(ret, *method);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnPropertiesReadByUuidInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnPropertiesReadByUuidInner Triggered!");
    int32_t ret = data.ReadInt32();
    int32_t ssapPropertiesNumber = data.ReadInt32();
    if (ssapPropertiesNumber > PROPERTY_READ_BY_UUID_MAX_SIZE || ssapPropertiesNumber < 0) {
        HILOGE("OnPropertiesReadByUuidInner properties number is invalid: %{public}d", ssapPropertiesNumber);
        return TRANSACTION_ERR;
    }
    std::vector<NearlinkSsapPropertyParcel> properties;
    for (int i = 0; i < ssapPropertiesNumber; i++) {
        std::shared_ptr<NearlinkSsapPropertyParcel> property(data.ReadParcelable<NearlinkSsapPropertyParcel>());
        if (!property) {
            return TRANSACTION_ERR;
        }
        properties.push_back(*property);
    }
    OnReadPropertiesByUuid(ret, properties);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnPropertyWriteInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnPropertyWriteInner Triggered!");
    int32_t ret = data.ReadInt32();
    std::shared_ptr<NearlinkSsapPropertyParcel> proeprty(data.ReadParcelable<NearlinkSsapPropertyParcel>());
    if (!proeprty) {
        return TRANSACTION_ERR;
    }
    OnWriteProperty(ret, *proeprty);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnSetPropertyNotifyInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnSetPropertyNotifyInner Triggered!");
    int32_t ret = data.ReadInt32();
    bool enable = data.ReadBool();
    std::shared_ptr<NearlinkSsapPropertyParcel> property(data.ReadParcelable<NearlinkSsapPropertyParcel>());
    if (!property) {
        return TRANSACTION_ERR;
    }
    OnSetPropertyNotification(ret, enable, *property);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnSetPropertyIndicateInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnSetPropertyIndicateInner Triggered!");
    int32_t ret = data.ReadInt32();
    bool enable = data.ReadBool();
    std::shared_ptr<NearlinkSsapPropertyParcel> property(data.ReadParcelable<NearlinkSsapPropertyParcel>());
    if (!property) {
        return TRANSACTION_ERR;
    }
    OnSetPropertyIndication(ret, enable, *property);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnDescriptorReadInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnDescriptorReadInner Triggered!");
    int32_t ret = data.ReadInt32();
    std::shared_ptr<NearlinkSsapDescriptorParcel> descriptor(data.ReadParcelable<NearlinkSsapDescriptorParcel>());
    if (!descriptor) {
        return TRANSACTION_ERR;
    }
    OnReadDescriptor(ret, *descriptor);

    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnDescriptorWriteInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnDescriptorWriteInner Triggered!");
    int32_t ret = data.ReadInt32();
    std::shared_ptr<NearlinkSsapDescriptorParcel> descriptor(data.ReadParcelable<NearlinkSsapDescriptorParcel>());
    if (!descriptor) {
        return TRANSACTION_ERR;
    }
    OnWriteDescriptor(ret, *descriptor);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnMtuChangedInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnMtuChangedInner Triggered!");
    int32_t state = data.ReadInt32();
    uint16_t mtu = data.ReadUint16();
    OnMtuChanged(state, mtu);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnServicesDiscoveredInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnServicesDiscoveredInner Triggered!");
    int32_t status = data.ReadInt32();
    OnServicesDiscovered(status);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnServicesDiscoveredByUuidInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnServicesDiscoveredInner Triggered!");
    int32_t status = data.ReadInt32();
    std::shared_ptr<NearlinkUuidParcel> uuid(data.ReadParcelable<NearlinkUuidParcel>());
    if (uuid == nullptr) {
        HILOGE("[RemoveLpDeviceParamInner] fail: read uuid failed");
        return ERR_INVALID_VALUE;
    }
    Uuid sleUuid = Uuid(*uuid);
    OnServicesDiscoveredByUuid(status, sleUuid);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnConnectionParameterChangedInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnConnectionParameterChangedInner Triggered!");
    int32_t interval = data.ReadInt32();
    int32_t latency = data.ReadInt32();
    int32_t timeout = data.ReadInt32();
    int32_t status = data.ReadInt32();
    OnConnectionParameterChanged(interval, latency, timeout, status);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnServicesRediscoveredInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnServicesRediscoveredInner Triggered!");
    int32_t serviceNumber = data.ReadInt32();
    if (serviceNumber <= 0 || serviceNumber > SERVICE_MAX_NUMBER) {
        HILOGE("OnServicesRediscoveredInner services number is invalid: %{public}d", serviceNumber);
        return TRANSACTION_ERR;
    }
    std::vector<NearlinkSsapServiceParcel> services;
    for (int i = 0; i < serviceNumber; i++) {
        std::shared_ptr<NearlinkSsapServiceParcel> service(data.ReadParcelable<NearlinkSsapServiceParcel>());
        if (!service) {
            return TRANSACTION_ERR;
        }
        services.push_back(*service);
    }
    OnServicesRediscovered(services);
    return NO_ERROR;
}

ErrCode NearlinkSsapClientCallbackStub::OnServiceChangedInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientCallbackStub::OnServiceChangedInner Triggered!");
    uint16_t handle = data.ReadUint16();
    std::shared_ptr<NearlinkUuidParcel> uuidParcel(data.ReadParcelable<NearlinkUuidParcel>());
    if (!uuidParcel) {
        HILOGE("OnServiceChangedInner read uuid failed");
        return TRANSACTION_ERR;
    }
    OnServiceChanged(handle, *uuidParcel);
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS
