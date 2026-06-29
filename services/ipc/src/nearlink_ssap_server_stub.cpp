/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "nearlink_ssap_server_stub.h"
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
#define STUB_FUNC(code, func, perm) NearlinkSsapServerInterfaceCode::code, {NearlinkSsapServerStub::func, perm}

namespace OHOS {
namespace Nearlink {

NearlinkSsapServerStub::NearlinkSsapServerStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(SSAP_SERVER_ADD_SERVICE, AddServiceInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SSAP_SERVER_CLEAR_SERVICES, ClearServicesInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(SSAP_SERVER_CANCEL_CONNECTION, CancelConnectionInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SSAP_SERVER_REGISTER, RegisterApplicationInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SSAP_SERVER_DEREGISTER, DeregisterApplicationInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SSAP_SERVER_NOTIFY_CLIENT, NotifyClientInner, CHECK_PERM(false,
            {ACCESS_NEARLINK}))},
        {STUB_FUNC(SSAP_SERVER_NOTIFY_EVENT, NotifyEventInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(SSAP_SERVER_SET_PROPERTY, SetPropertyValueInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(SSAP_SERVER_SET_DESCRIPTOR, SetDescriptorValueInner, CHECK_PERM(false, {MANAGE_NEARLINK}))},
        {STUB_FUNC(SSAP_SERVER_CONNECT, ConnectInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SSAP_SERVER_REMOVE_SERVICE, RemoveServiceInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SSAP_SERVER_AUTHORIZE_RESPONSE, AuthorizeResponseInner, CHECK_PERM(false,
            {ACCESS_NEARLINK}))},
    };
}

NearlinkSsapServerStub::~NearlinkSsapServerStub()
{}

int32_t NearlinkSsapServerStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkSsapServer", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkSsapServerStub);
}

int32_t NearlinkSsapServerStub::AddServiceInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply)
{
    int32_t appID = data.ReadInt32();
    std::shared_ptr<NearlinkSsapServiceParcel> service(data.ReadParcelable<NearlinkSsapServiceParcel>());
    if (!service) {
        return TRANSACTION_ERR;
    }
    NlErrCode result = stub->AddService(appID, service.get());
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapServerStub::ClearServicesInner(NearlinkSsapServerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int appId = data.ReadInt32();
    NlErrCode result = stub->ClearServices(appId);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapServerStub::CancelConnectionInner(NearlinkSsapServerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    NlErrCode result = stub->CancelConnection(appId, *device);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapServerStub::RegisterApplicationInner(NearlinkSsapServerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkSsapServerCallback> callback = OHOS::iface_cast<INearlinkSsapServerCallback>(remote);
    NL_CHECK_RETURN_RET(callback, NL_ERR_IPC_TRANS_FAILED, "callback is nullptr.");
    int appId = 0;
    NlErrCode result = stub->RegisterApplication(callback, appId);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply result failed.");
        return ERR_INVALID_VALUE;
    }
    ret = reply.WriteInt32(appId);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply appId failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapServerStub::DeregisterApplicationInner(NearlinkSsapServerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int appId = data.ReadInt32();
    NlErrCode result = stub->DeregisterApplication(appId);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapServerStub::NotifyClientInner(NearlinkSsapServerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapPropertyParcel> property(data.ReadParcelable<NearlinkSsapPropertyParcel>());
    if (!property) {
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    bool needConfirm = data.ReadBool();
    NlErrCode result = stub->NotifyClient(appId, property.get(), *device, needConfirm);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapServerStub::NotifyEventInner(NearlinkSsapServerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapEventParcel> event(data.ReadParcelable<NearlinkSsapEventParcel>());
    if (!event) {
        return TRANSACTION_ERR;
    }
    std::vector<uint8_t> value;
    if (!data.ReadUInt8Vector(&value)) {
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    bool needConfirm = data.ReadBool();
    NlErrCode result = stub->NotifyEvent(appId, event.get(), value, *device, needConfirm);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapServerStub::SetPropertyValueInner(NearlinkSsapServerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapPropertyParcel> property(data.ReadParcelable<NearlinkSsapPropertyParcel>());
    if (!property) {
        return TRANSACTION_ERR;
    }
    NlErrCode result = stub->SetPropertyValue(appId, property.get());
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapServerStub::SetDescriptorValueInner(NearlinkSsapServerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapDescriptorParcel> descriptor(data.ReadParcelable<NearlinkSsapDescriptorParcel>());
    if (!descriptor) {
        return TRANSACTION_ERR;
    }
    NlErrCode result = stub->SetDescriptorValue(appId, descriptor.get());
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapServerStub::ConnectInner(NearlinkSsapServerStub *stub, MessageParcel &data, MessageParcel &reply)
{
    int appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapDevice> device(data.ReadParcelable<NearlinkSsapDevice>());
    if (!device) {
        return TRANSACTION_ERR;
    }
    uint8_t secureReq = data.ReadUint8();
    bool autoConnect = data.ReadBool();
    NlErrCode result = stub->Connect(appId, *device, secureReq, autoConnect);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapServerStub::RemoveServiceInner(NearlinkSsapServerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int appId = data.ReadInt32();
    std::shared_ptr<NearlinkSsapServiceParcel> service(data.ReadParcelable<NearlinkSsapServiceParcel>());
    if (!service) {
        return TRANSACTION_ERR;
    }
    NlErrCode result = stub->RemoveService(appId, *service);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}

int32_t NearlinkSsapServerStub::AuthorizeResponseInner(NearlinkSsapServerStub *stub, MessageParcel &data,
    MessageParcel &reply)
{
    int appId = data.ReadInt32();
    uint16_t requestId = data.ReadUint16();
    bool allow = data.ReadBool();
    NlErrCode result = stub->AuthorizeResponse(appId, requestId, allow);
    bool ret = reply.WriteInt32(result);
    if (!ret) {
        HILOGE("NearlinkSsapServerStub: reply writing failed.");
        return ERR_INVALID_VALUE;
    }
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS