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

#include "nearlink_ssap_client_stub.h"
#include "log.h"
#include "nearlink_permission_manager.h"
#include "ipc_types.h"
#include "string_ex.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm) NearlinkSsapClientInterfaceCode::code, {NearlinkSsapClientStub::func, perm}

namespace OHOS {
namespace Nearlink {

NearlinkSsapClientStub::NearlinkSsapClientStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(NL_SSAP_CLIENT_REGISTER_APP, RegisterApplicationInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_DEREGISTER_APP, DeregisterApplicationInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_CONNECT, ConnectInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_DIS_CONNECT, DisconnectInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_DISCOVERY_SERVICES, DiscoveryServicesInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_DISCOVERY_SERVICES_BY_UUID, DiscoverServiceByUuidInner,
            CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_READ_PROPERTY, ReadPropertyInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_CALL_METHOD, CallMethodInner, CHECK_PERM(true, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_WRITE_PROPERTY, WritePropertyInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_READ_DESCRIPTOR, ReadDescriptorInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_WRITE_DESCRIPTOR, WriteDescriptorInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_REQUEST_EXCHANGE_MTU, RequestExchangeMtuInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_REQUEST_CONNECTION_PRIORITY, RequestConnectionPriorityInner,
            CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_GET_SERVICES, GetServicesInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_GET_SERVICES_BY_UUID, GetServicesByUuidInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_REQUEST_NOTIFICATION, RequestNotificationInner,
            CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(NL_SSAP_CLIENT_REQUEST_INDICATION, RequestIndicationInner,
            CHECK_PERM(true, MULTI_PERM(ACCESS_NEARLINK, MANAGE_NEARLINK)))},
    };
}

NearlinkSsapClientStub::~NearlinkSsapClientStub()
{}

int32_t NearlinkSsapClientStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkSsapClient", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkSsapClientStub);
}

int32_t NearlinkSsapClientStub::RegisterApplicationInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::RegisterApplicationInner starts");
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkSsapClientCallback> callback = OHOS::iface_cast<INearlinkSsapClientCallback>(remote);
    NL_CHECK_RETURN_RET(callback, TRANSACTION_ERR, "callback is nullptr");
    uint8_t secureReq = data.ReadUint8();
    std::shared_ptr<NearlinkRawAddress> addr(data.ReadParcelable<NearlinkRawAddress>());
    if (!addr) {
        return TRANSACTION_ERR;
    }
    int32_t transport = data.ReadInt32();
    int appId = 0;
    NlErrCode result = stub->RegisterApplication(callback, secureReq, *addr, transport, appId);
    bool resultRet = reply.WriteInt32(result);
    bool appIdRet = reply.WriteInt32(appId);
    if (!(resultRet && appIdRet)) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::DeregisterApplicationInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::DeregisterApplicationInner starts");
    int32_t appId = data.ReadInt32();
    NlErrCode result = stub->DeregisterApplication(appId);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::ConnectInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::ConnectInner starts");
    int32_t appId = data.ReadInt32();
    bool isAutoConnect = data.ReadBool();
    NlErrCode result = stub->Connect(appId, isAutoConnect);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::DisconnectInner(NearlinkSsapClientStub *stub, MessageParcel &data, MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::DisconnectInner starts");
    int32_t appId = data.ReadInt32();
    NlErrCode result = stub->Disconnect(appId);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::DiscoveryServicesInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::DiscoveryServicesInner starts");
    int32_t appId = data.ReadInt32();
    NlErrCode result = stub->DiscoveryServices(appId);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::DiscoverServiceByUuidInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::DiscoveryServicesInner starts");
    int32_t appId = data.ReadInt32();
    std::shared_ptr<Uuid> uuid(data.ReadParcelable<NearlinkUuidParcel>());
    if (!uuid) {
        return TRANSACTION_ERR;
    }
    NlErrCode result = stub->DiscoverServiceByUuid(appId, *uuid);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::ReadPropertyInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::ReadPropertyInner starts");
    int32_t appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapPropertyParcel> property(data.ReadParcelable<NearlinkSsapPropertyParcel>());
    if (!property) {
        return TRANSACTION_ERR;
    }
    NlErrCode result = stub->ReadProperty(appId, *property);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::CallMethodInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int32_t appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapMethodParcel> method(data.ReadParcelable<NearlinkSsapMethodParcel>());
    HILOGI("appId: %{public}d", appId);
    if (!method) {
        return TRANSACTION_ERR;
    }
    bool withoutRespond = data.ReadBool();

    NlErrCode result = stub->CallMethod(appId, method.get(), withoutRespond);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::WritePropertyInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::WritePropertyInner starts");
    int32_t appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapPropertyParcel> property(data.ReadParcelable<NearlinkSsapPropertyParcel>());
    if (!property) {
        return TRANSACTION_ERR;
    }
    bool withoutRespond = data.ReadBool();
    NlErrCode result = stub->WriteProperty(appId, property.get(), withoutRespond);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::ReadDescriptorInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::ReadDescriptorInner starts");
    int32_t appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapDescriptorParcel> descriptor(data.ReadParcelable<NearlinkSsapDescriptorParcel>());
    if (!descriptor) {
        return TRANSACTION_ERR;
    }
    NlErrCode result = stub->ReadDescriptor(appId, *descriptor);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::WriteDescriptorInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::WriteDescriptorInner starts");
    int32_t appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapDescriptorParcel> descriptor(data.ReadParcelable<NearlinkSsapDescriptorParcel>());
    if (!descriptor) {
        return TRANSACTION_ERR;
    }
    bool withoutRespond = data.ReadBool();
    NlErrCode result = stub->WriteDescriptor(appId, descriptor.get(), withoutRespond);
    HILOGI("appId=%{public}d, result=%{public}d", appId, result);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::RequestExchangeMtuInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::RequestExchangeMtuInner starts");
    int32_t appId = data.ReadInt32();
    int32_t mtu = data.ReadInt32();
    NlErrCode result = stub->RequestExchangeMtu(appId, mtu);
    HILOGI("appId=%{public}d, mtu=%{public}d, result=%{public}d", appId, mtu, result);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::RequestConnectionPriorityInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::RequestConnectionPriorityInner starts");
    int32_t appId = data.ReadInt32();
    int32_t connPriority = data.ReadInt32();
    NlErrCode result = stub->RequestConnectionPriority(appId, connPriority);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::GetServicesInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::GetServicesInner starts");
    int32_t appId = data.ReadInt32();
    std::vector<NearlinkSsapServiceParcel> service;
    NlErrCode result = stub->GetServices(appId, service);
    bool resultRet = reply.WriteInt32(result);
    bool sizeRet = reply.WriteInt32(service.size());
    if (!(resultRet && sizeRet)) {
        HILOGE("Write data failed");
        return ERR_INVALID_VALUE;
    }
    size_t num = service.size();
    for (size_t i = 0; i < num; i++) {
        bool ret = reply.WriteParcelable(&service[i]);
        if (!ret) {
            HILOGE("WriteParcelable<GetServicesInner> failed");
            return ERR_INVALID_VALUE;
        }
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::GetServicesByUuidInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::GetServicesByUuidInner starts");
    int32_t appId = data.ReadInt32();
    std::vector<NearlinkSsapServiceParcel> service;
    std::shared_ptr<Uuid> uuid(data.ReadParcelable<NearlinkUuidParcel>());
    if (!uuid) {
        HILOGE("GetServicesByUuidInner: uuid transport failed.");
        return TRANSACTION_ERR;
    }
    NlErrCode result = stub->GetServicesByUuid(appId, *uuid, service);
    bool resultRet = reply.WriteInt32(result);
    if (!resultRet) {
        HILOGE("Write data failed");
        return ERR_INVALID_VALUE;
    }
    size_t num = service.size();
    for (size_t i = 0; i < num; i++) {
        bool ret = reply.WriteParcelable(&service[i]);
        if (!ret) {
            HILOGE("WriteParcelable<GetServicesInner> failed");
            return ERR_INVALID_VALUE;
        }
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::RequestNotificationInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::RequestNotificationInner starts");
    int32_t appId = data.ReadInt32();
    uint16_t propertyHandle = data.ReadUint16();
    bool enable = data.ReadBool();
    NlErrCode result = stub->RequestPropertyNotification(appId, propertyHandle, enable, 
        static_cast<uint8_t>(NotifyOption::SSAP_SET_NOTIFY));
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapClientStub::RequestIndicationInner(NearlinkSsapClientStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    HILOGI("NearlinkSsapClientStub::RequestIndicationInner starts");
    int32_t appId = data.ReadInt32();
    uint16_t propertyHandle = data.ReadUint16();
    bool enable = data.ReadBool();
    NlErrCode result = stub->RequestPropertyNotification(appId, propertyHandle, enable,
        static_cast<uint8_t>(NotifyOption::SSAP_SET_INDICATE));
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapClientStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS