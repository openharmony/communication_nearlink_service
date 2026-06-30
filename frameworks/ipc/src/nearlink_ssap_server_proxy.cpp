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

#include "nearlink_ssap_server_proxy.h"
#include "log.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {
NlErrCode NearlinkSsapServerProxy::AddService(int32_t appId, NearlinkSsapServiceParcel *services)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_IPC_TRANS_FAILED, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(services), NL_ERR_IPC_TRANS_FAILED, "Write services error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };

    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_ADD_SERVICE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapServerProxy::ClearServices(int appId)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_IPC_TRANS_FAILED, "Write appId error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };

    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_CLEAR_SERVICES, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapServerProxy::CancelConnection(int appId, const NearlinkSsapDevice &device)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_IPC_TRANS_FAILED, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&device), NL_ERR_IPC_TRANS_FAILED, "Write device error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_CANCEL_CONNECTION, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapServerProxy::RegisterApplication(const sptr<INearlinkSsapServerCallback> &callback,
    int32_t &appId)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write remoteObject error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_REGISTER, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        appId = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkSsapServerProxy::DeregisterApplication(int appId)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_IPC_TRANS_FAILED, "Write appId error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };

    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_DEREGISTER, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapServerProxy::NotifyClient(int appId,
    NearlinkSsapPropertyParcel *property, const NearlinkSsapDevice &device, bool needConfirm)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_IPC_TRANS_FAILED, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(property), NL_ERR_IPC_TRANS_FAILED, "Write property error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&device), NL_ERR_IPC_TRANS_FAILED, "Write device error");
    NL_CHECK_RETURN_RET(data.WriteBool(needConfirm), NL_ERR_IPC_TRANS_FAILED, "Write needConfirm error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };

    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_NOTIFY_CLIENT, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapServerProxy::NotifyEvent(int appId, NearlinkSsapEventParcel *event, std::vector<uint8_t> &value,
    const NearlinkSsapDevice &device, bool needConfirm)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_IPC_TRANS_FAILED, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(event), NL_ERR_IPC_TRANS_FAILED, "Write event error");
    NL_CHECK_RETURN_RET(data.WriteUInt8Vector(value), NL_ERR_IPC_TRANS_FAILED, "Write value error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&device), NL_ERR_IPC_TRANS_FAILED, "Write device error");
    NL_CHECK_RETURN_RET(data.WriteBool(needConfirm), NL_ERR_IPC_TRANS_FAILED, "Write needConfirm error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };

    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_NOTIFY_EVENT, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapServerProxy::SetPropertyValue(int32_t appId, NearlinkSsapPropertyParcel *property)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_IPC_TRANS_FAILED, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(property), NL_ERR_IPC_TRANS_FAILED, "Write property error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };

    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_SET_PROPERTY, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapServerProxy::SetDescriptorValue(int32_t appId, NearlinkSsapDescriptorParcel *descriptor)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_IPC_TRANS_FAILED, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(descriptor), NL_ERR_IPC_TRANS_FAILED, "Write descriptor error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };

    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_SET_DESCRIPTOR, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapServerProxy::Connect(int32_t appId, const NearlinkSsapDevice &device, uint8_t secureReq,
    bool autoConnect)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_IPC_TRANS_FAILED, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&device), NL_ERR_IPC_TRANS_FAILED, "Write device error");
    NL_CHECK_RETURN_RET(data.WriteUint8(secureReq), NL_ERR_IPC_TRANS_FAILED, "Write secureReq error");
    NL_CHECK_RETURN_RET(data.WriteBool(autoConnect), NL_ERR_IPC_TRANS_FAILED, "Write autoConnect error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };

    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_CONNECT, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapServerProxy::RemoveService(int32_t appId, const NearlinkSsapServiceParcel &services)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_IPC_TRANS_FAILED, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&services), NL_ERR_IPC_TRANS_FAILED, "Write services error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };

    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_REMOVE_SERVICE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapServerProxy::AuthorizeResponse(int appId, uint16_t requestId, bool allow)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapServerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_IPC_TRANS_FAILED, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteUint16(requestId), NL_ERR_IPC_TRANS_FAILED, "Write requestId error");
    NL_CHECK_RETURN_RET(data.WriteBool(allow), NL_ERR_IPC_TRANS_FAILED, "Write allow error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };

    ErrCode error = InnerTransact(NearlinkSsapServerInterfaceCode::SSAP_SERVER_AUTHORIZE_RESPONSE, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

ErrCode NearlinkSsapServerProxy::InnerTransact(
    uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply)
{
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, OBJECT_NULL, "[InnerTransact] fail: get Remote fail code %{public}d", code);
    ErrCode err = remote->SendRequest(code, data, reply, flags);
    if (err != NO_ERROR) {
        HILOGW("[InnerTransact] fail: ipcErr=%{public}d code %{public}d", err, code);
    }
    return err;
}

}  // namespace Nearlink
}  // namespace OHOS