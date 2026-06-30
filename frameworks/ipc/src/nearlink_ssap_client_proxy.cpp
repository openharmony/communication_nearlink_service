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

#include "nearlink_errorcode.h"
#include "log.h"
#include "nearlink_uuid_parcel.h"
#include "nearlink_ssap_client_proxy.h"

namespace OHOS {
namespace Nearlink {
const int SSAP_CLIENT_READ_DATA_SIZE_MAX_LEN = 0xFF;
NlErrCode NearlinkSsapClientProxy::RegisterApplication(const sptr<INearlinkSsapClientCallback> &callback,
    uint8_t secureReq, const NearlinkRawAddress &addr, int32_t transport, int &appId)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_INTERNAL_ERROR,
        "Write RemoteObject error");
    NL_CHECK_RETURN_RET(data.WriteUint8(secureReq), NL_ERR_INTERNAL_ERROR, "Write secureReq error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&addr), NL_ERR_INTERNAL_ERROR, "Write addr error");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_INTERNAL_ERROR, "Write transport error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_REGISTER_APP, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        appId = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkSsapClientProxy::RegisterApplication(const sptr<INearlinkSsapClientCallback> &callback,
    const NearlinkRawAddress &addr, int32_t transport, int32_t &appId)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_INTERNAL_ERROR,
        "Write RemoteObject error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&addr), NL_ERR_INTERNAL_ERROR, "Write addr error");
    NL_CHECK_RETURN_RET(data.WriteInt32(transport), NL_ERR_INTERNAL_ERROR, "Write transport error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_REGISTER_APP, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::DeregisterApplication(int32_t appId)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    HILOGE("appId : %{public}d", appId);
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_DEREGISTER_APP, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::Connect(int32_t appId, bool isAutoConnect)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteBool(isAutoConnect), NL_ERR_INTERNAL_ERROR, "Write isAutoConnect error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_CONNECT, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::Disconnect(int32_t appId)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_DIS_CONNECT, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::DiscoveryServices(int32_t appId)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_DISCOVERY_SERVICES, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::DiscoverServiceByUuid(int32_t appId, const Uuid &uuid)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");
    NearlinkUuidParcel uid(Uuid::ConvertFrom128Bits(uuid.ConvertTo128Bits()));
    NL_CHECK_RETURN_RET(data.WriteParcelable(&uid), NL_ERR_INTERNAL_ERROR, "Write uid error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_DISCOVERY_SERVICES_BY_UUID, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::ReadProperty(int32_t appId, const NearlinkSsapPropertyParcel &property)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&property), NL_ERR_INTERNAL_ERROR, "Write property error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_READ_PROPERTY, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::CallMethod(int32_t appId, NearlinkSsapMethodParcel *method, bool withoutRespond)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(method), NL_ERR_INTERNAL_ERROR, "Write method error");
    NL_CHECK_RETURN_RET(data.WriteBool(withoutRespond), NL_ERR_INTERNAL_ERROR, "Write withoutRespond error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_CALL_METHOD, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::WriteProperty(
    int32_t appId, NearlinkSsapPropertyParcel *property, bool withoutRespond)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(property), NL_ERR_INTERNAL_ERROR, "Write property error");
    NL_CHECK_RETURN_RET(data.WriteBool(withoutRespond), NL_ERR_INTERNAL_ERROR, "Write withoutRespond error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_WRITE_PROPERTY, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::ReadDescriptor(int32_t appId, const NearlinkSsapDescriptorParcel &descriptor)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&descriptor), NL_ERR_INTERNAL_ERROR, "Write descriptor error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_READ_DESCRIPTOR, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::WriteDescriptor(int32_t appId, NearlinkSsapDescriptorParcel *descriptor,
    bool withoutRespond)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(descriptor), NL_ERR_INTERNAL_ERROR, "Write descriptor error");
    NL_CHECK_RETURN_RET(data.WriteBool(withoutRespond), NL_ERR_INTERNAL_ERROR, "Write withoutRespond error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_WRITE_DESCRIPTOR, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::RequestExchangeMtu(int32_t appId, int32_t mtu)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteInt32(mtu), NL_ERR_INTERNAL_ERROR, "Write mtu error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_REQUEST_EXCHANGE_MTU, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::RequestConnectionPriority(int32_t appId, int32_t connPriority)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteInt32(connPriority), NL_ERR_INTERNAL_ERROR, "Write connPriority error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_REQUEST_CONNECTION_PRIORITY, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSsapClientProxy::GetServices(int32_t appId, std::vector<NearlinkSsapServiceParcel> &service)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_GET_SERVICES, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception != NL_NO_ERROR) {
        return exception;
    }
    int ssapServiceNumber = reply.ReadInt32();
    NL_CHECK_RETURN_RET(ssapServiceNumber <= SSAP_CLIENT_READ_DATA_SIZE_MAX_LEN, NL_ERR_INVALID_STATE,
        "read Parcelable size failed.");
    for (int i = ssapServiceNumber; i > 0; i--) {
        std::shared_ptr<NearlinkSsapServiceParcel> dev(reply.ReadParcelable<NearlinkSsapServiceParcel>());
        NL_CHECK_RETURN_RET(dev, NL_ERR_INTERNAL_ERROR, "read Parcelable dev failed.");
        service.push_back(*dev);
    }
    return exception;
}

NlErrCode NearlinkSsapClientProxy::GetServicesByUuid(int32_t appId, const Uuid &uuid,
    std::vector<NearlinkSsapServiceParcel> &service)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");
    NearlinkUuidParcel uid(Uuid::ConvertFrom128Bits(uuid.ConvertTo128Bits()));
    NL_CHECK_RETURN_RET(data.WriteParcelable(&uid), NL_ERR_INTERNAL_ERROR, "Write uid error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientInterfaceCode::NL_SSAP_CLIENT_GET_SERVICES_BY_UUID, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception != NL_NO_ERROR) {
        return exception;
    }
    const std::shared_ptr<NearlinkSsapServiceParcel> dev(reply.ReadParcelable<NearlinkSsapServiceParcel>());
    NL_CHECK_RETURN_RET(dev, NL_ERR_INTERNAL_ERROR, "read Parcelable dev failed.");
    service.push_back(*dev);
    return exception;
}

NlErrCode NearlinkSsapClientProxy::RequestPropertyNotification(int32_t appId, uint16_t propertyHandle, bool enable,
    uint8_t notifyOption)
{
    HILOGI("start");
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSsapClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteInt32(appId), NL_ERR_INTERNAL_ERROR, "Write appId error");
    NL_CHECK_RETURN_RET(data.WriteUint16(propertyHandle), NL_ERR_INTERNAL_ERROR, "Write propertyHandle error");
    NL_CHECK_RETURN_RET(data.WriteBool(enable), NL_ERR_INTERNAL_ERROR, "Write enable error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    NearlinkSsapClientInterfaceCode code = notifyOption == static_cast<uint8_t>(NotifyOption::SSAP_SET_NOTIFY) ? 
        NL_SSAP_CLIENT_REQUEST_NOTIFICATION : NL_SSAP_CLIENT_REQUEST_INDICATION;
    ErrCode error = InnerTransact(code, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

ErrCode NearlinkSsapClientProxy::InnerTransact(
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
