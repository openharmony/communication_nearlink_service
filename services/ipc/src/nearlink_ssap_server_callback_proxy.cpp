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

#include "nearlink_ssap_server_callback_proxy.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
void NearlinkSsapServerCallbackProxy::OnMtuChanged(const NearlinkSsapDevice &device, uint16_t mtu)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapServerCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed");
    NL_CHECK_RETURN(data.WriteUint16(mtu), "write mtu failed");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_MTU_CHANGED, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
    return;
}

void NearlinkSsapServerCallbackProxy::OnAddService(const NearlinkSsapServiceParcel &service, int ret)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapServerCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&service), "write service failed");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret failed");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_ADD_SERVICE, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
    return;
}

void NearlinkSsapServerCallbackProxy::OnPropertyReadRequest(
    const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapServerCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed");
    NL_CHECK_RETURN(data.WriteParcelable(&property), "write property failed");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret failed");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_PROPERTY_READ_REQUEST, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
    return;
}

void NearlinkSsapServerCallbackProxy::OnDescriptorReadRequest(
    const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapServerCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed");
    NL_CHECK_RETURN(data.WriteParcelable(&descriptor), "write descriptor failed");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret failed");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_DESCRIPTOR_READ_REQUEST, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
    return;
}

void NearlinkSsapServerCallbackProxy::OnPropertyWriteRequest(
    const NearlinkSsapDevice &device, const NearlinkSsapPropertyParcel &property, int ret)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapServerCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed");
    NL_CHECK_RETURN(data.WriteParcelable(&property), "write property failed");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret failed");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_PROPERTY_WRITE_REQUEST, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
    return;
}

void NearlinkSsapServerCallbackProxy::OnDescriptorWriteRequest(
    const NearlinkSsapDevice &device, const NearlinkSsapDescriptorParcel &descriptor, int ret)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapServerCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed");
    NL_CHECK_RETURN(data.WriteParcelable(&descriptor), "write descriptor failed");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret failed");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_DESCRIPTOR_WRITE_REQUEST, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
    return;
}

void NearlinkSsapServerCallbackProxy::OnNotifyPropertyChanged(
    const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapServerCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed");
    NearlinkUuidParcel uid = NearlinkUuidParcel(uuid);
    NL_CHECK_RETURN(data.WriteParcelable(&uid), "write uuid failed");
    NL_CHECK_RETURN(data.WriteUint16(handle), "write handle failed");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret failed");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_NOTIFY_PROPERTY_CHANGED, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
    return;
}

void NearlinkSsapServerCallbackProxy::OnNotifyEventChanged(
    const NearlinkSsapDevice &device, const Uuid &uuid, uint16_t handle, int ret)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapServerCallbackProxy::GetDescriptor()), "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed");
    NearlinkUuidParcel uid = NearlinkUuidParcel(uuid);
    NL_CHECK_RETURN(data.WriteParcelable(&uid), "write uuid failed");
    NL_CHECK_RETURN(data.WriteUint16(handle), "write handle failed");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret failed");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_NOTIFY_EVENT_CHANGED, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
    return;
}

void NearlinkSsapServerCallbackProxy::OnConnectionStateChanged(const NearlinkSsapDevice &device,
    uint8_t state, int reason)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapServerCallbackProxy::GetDescriptor()), "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed");
    NL_CHECK_RETURN(data.WriteUint8(state), "write state failed");
    NL_CHECK_RETURN(data.WriteInt32(reason), "write reason failed");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapServerCallbackInterfaceCode::SSAP_SERVER_CALLBACK_CONNECTION_STATE_CHANGED, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
    return;
}

ErrCode NearlinkSsapServerCallbackProxy::InnerTransact(
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