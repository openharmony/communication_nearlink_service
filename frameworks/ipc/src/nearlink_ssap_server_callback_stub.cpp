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

#include "nearlink_ssap_server_callback_stub.h"
#include "log.h"
#include "ipc_types.h"
#include "string_ex.h"

namespace OHOS {
namespace Nearlink {
NearlinkSsapServerCallbackStub::NearlinkSsapServerCallbackStub()
{
    HILOGD("start.");
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_MTU_CHANGED)] =
        &NearlinkSsapServerCallbackStub::OnMtuChangedInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_ADD_SERVICE)] =
        &NearlinkSsapServerCallbackStub::OnAddServiceInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_PROPERTY_READ_REQUEST)] =
        &NearlinkSsapServerCallbackStub::OnPropertyReadRequestInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_DESCRIPTOR_READ_REQUEST)] =
        &NearlinkSsapServerCallbackStub::OnDescriptorReadRequestInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_PROPERTY_WRITE_REQUEST)] =
        &NearlinkSsapServerCallbackStub::OnPropertyWriteRequestInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_DESCRIPTOR_WRITE_REQUEST)] =
        &NearlinkSsapServerCallbackStub::OnDescriptorWriteRequestInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_NOTIFY_PROPERTY_CHANGED)] =
        &NearlinkSsapServerCallbackStub::OnNotifyPropertyChangedInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_NOTIFY_EVENT_CHANGED)] =
        &NearlinkSsapServerCallbackStub::OnNotifyEventChangedInner;
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_CONNECTION_STATE_CHANGED)] =
        &NearlinkSsapServerCallbackStub::OnConnectionStateChangedInner;
    HILOGD("ends.");
}

NearlinkSsapServerCallbackStub::~NearlinkSsapServerCallbackStub()
{
    HILOGD("start.");
    memberFuncMap_.clear();
}
int NearlinkSsapServerCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    HILOGD("NearlinkSsapServerCallbackStub::OnRemoteRequest, cmd = %{public}u, flags= %{public}d",
        code, option.GetFlags());
    if (NearlinkSsapServerCallbackStub::GetDescriptor() != data.ReadInterfaceToken()) {
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
    HILOGW("NearlinkSsapServerCallbackStub::OnRemoteRequest, default case, need check.");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
};

ErrCode NearlinkSsapServerCallbackStub::OnMtuChangedInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkSsapServerCallbackStub::OnMtuChangedInner Triggered!");
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        HILOGE("NearlinkSsapServerCallbackStub::OnMtuChangedInner device is null");
        return TRANSACTION_ERR;
    }
    uint16_t mtu = data.ReadUint16();
    OnMtuChanged(*device, mtu);
    return NO_ERROR;
}

ErrCode NearlinkSsapServerCallbackStub::OnAddServiceInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkSsapServerCallbackStub::OnAddServiceInner Triggered!");
    std::shared_ptr<NearlinkSsapServiceParcel> service(data.ReadParcelable<NearlinkSsapServiceParcel>());
    if (!service) {
        HILOGE("NearlinkSsapServerCallbackStub::OnAddServiceInner service is null");
        return TRANSACTION_ERR;
    }
    int ret = data.ReadInt32();
    OnAddService(*service, ret);
    return NO_ERROR;
}

ErrCode NearlinkSsapServerCallbackStub::OnPropertyReadRequestInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkSsapServerCallbackStub::OnPropertyReadRequestInner Triggered!");
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        HILOGE("NearlinkSsapServerCallbackStub::OnPropertyReadRequestInner device is null");
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkSsapPropertyParcel> property(data.ReadParcelable<NearlinkSsapPropertyParcel>());
    if (!property) {
        HILOGE("NearlinkSsapServerCallbackStub::OnPropertyReadRequestInner property is null");
        return TRANSACTION_ERR;
    }
    int ret = data.ReadInt32();
    OnPropertyReadRequest(*device, *property, ret);
    return NO_ERROR;
}

ErrCode NearlinkSsapServerCallbackStub::OnDescriptorReadRequestInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkSsapServerCallbackStub::OnDescriptorReadRequestInner Triggered!");
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        HILOGE("NearlinkSsapServerCallbackStub::OnDescriptorReadRequestInner device is null");
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkSsapDescriptorParcel> descriptor(data.ReadParcelable<NearlinkSsapDescriptorParcel>());
    if (!descriptor) {
        HILOGE("NearlinkSsapServerCallbackStub::OnDescriptorReadRequestInner descriptor is null");
        return TRANSACTION_ERR;
    }
    int ret = data.ReadInt32();
    OnDescriptorReadRequest(*device, *descriptor, ret);
    return NO_ERROR;
}

ErrCode NearlinkSsapServerCallbackStub::OnPropertyWriteRequestInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkSsapServerCallbackStub::OnPropertyWriteRequestInner Triggered!");
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        HILOGE("NearlinkSsapServerCallbackStub::OnPropertyWriteRequestInner device is null");
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkSsapPropertyParcel> property(data.ReadParcelable<NearlinkSsapPropertyParcel>());
    if (!property) {
        HILOGE("NearlinkSsapServerCallbackStub::OnPropertyWriteRequestInner property is null");
        return TRANSACTION_ERR;
    }
    int ret = data.ReadInt32();
    OnPropertyWriteRequest(*device, *property, ret);
    return NO_ERROR;
}

ErrCode NearlinkSsapServerCallbackStub::OnDescriptorWriteRequestInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkSsapServerCallbackStub::OnDescriptorWriteRequestInner Triggered!");
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        HILOGE("NearlinkSsapServerCallbackStub::OnDescriptorWriteRequestInner device is null");
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkSsapDescriptorParcel> descriptor(data.ReadParcelable<NearlinkSsapDescriptorParcel>());
    if (!descriptor) {
        HILOGE("NearlinkSsapServerCallbackStub::OnDescriptorWriteRequestInner property is null");
        return TRANSACTION_ERR;
    }
    int ret = data.ReadInt32();
    OnDescriptorWriteRequest(*device, *descriptor, ret);
    return NO_ERROR;
}

ErrCode NearlinkSsapServerCallbackStub::OnNotifyPropertyChangedInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkSsapServerCallbackStub::OnNotifyPropertyChangedInner Triggered!");
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        HILOGE("NearlinkSsapServerCallbackStub::OnNotifyPropertyChangedInner device is null");
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkUuidParcel> uuid(data.ReadParcelable<NearlinkUuidParcel>());
    if (!uuid) {
        HILOGE("NearlinkSsapServerCallbackStub::OnNotifyPropertyChangedInner uuid is null");
        return TRANSACTION_ERR;
    }
    uint16_t handle = data.ReadUint16();
    int ret = data.ReadInt32();
    OnNotifyPropertyChanged(*device, *uuid, handle, ret);
    return NO_ERROR;
}

ErrCode NearlinkSsapServerCallbackStub::OnNotifyEventChangedInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkSsapServerCallbackStub::OnNotifyEventChangedInner Triggered!");
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        HILOGE("NearlinkSsapServerCallbackStub::OnNotifyEventChangedInner device is null");
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkUuidParcel> uuid(data.ReadParcelable<NearlinkUuidParcel>());
    if (!uuid) {
        HILOGE("NearlinkSsapServerCallbackStub::OnNotifyEventChangedInner uuid is null");
        return TRANSACTION_ERR;
    }
    uint16_t handle = data.ReadUint16();
    int ret = data.ReadInt32();
    OnNotifyEventChanged(*device, *uuid, handle, ret);
    return NO_ERROR;
}

ErrCode NearlinkSsapServerCallbackStub::OnConnectionStateChangedInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkSsapServerCallbackStub::OnConnectionStateChangedInner Triggered!");
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        HILOGE("NearlinkSsapServerCallbackStub::OnConnectionStateChangedInner device is null");
        return TRANSACTION_ERR;
    }
    uint8_t state = data.ReadUint8();
    int reason = data.ReadInt32();
    OnConnectionStateChanged(*device, state, reason);
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS