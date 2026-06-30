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

#include "nearlink_ssap_client_callback_proxy.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
constexpr int PROPERTY_READ_BY_UUID_SIZE_MAX = 255;
void NearlinkSsapClientCallbackProxy::OnConnectionStateChanged(int32_t state, int32_t newState)
{
    HILOGD("Enter");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(state), "Write state error");
    NL_CHECK_RETURN(data.WriteInt32(newState), "Write newState error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_CONNECT_STATE_CHANGE, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnPropertyChanged(const NearlinkSsapPropertyParcel &property)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&property), "write property error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_CHANGE, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnEventNotified(const NearlinkSsapEventParcel &event)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&event), "write event error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_EVENT_NOTIFY, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnReadProperty(
    int32_t ret, const NearlinkSsapPropertyParcel &property)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret error");
    NL_CHECK_RETURN(data.WriteParcelable(&property), "write property error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_READ, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnCallMethod(
    int32_t ret, const NearlinkSsapMethodParcel &method)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret error");
    NL_CHECK_RETURN(data.WriteParcelable(&method), "write method error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_METHOD_CALL, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnReadPropertiesByUuid(
    int32_t ret, const std::vector<NearlinkSsapPropertyParcel> &properties)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret error");
    NL_CHECK_RETURN(data.WriteInt32(properties.size()), "write size error");
    NL_CHECK_RETURN(properties.size() <= PROPERTY_READ_BY_UUID_SIZE_MAX, "properties size error");
    for (size_t i = 0; i < properties.size(); i++) {
        NL_CHECK_RETURN(data.WriteParcelable(&properties[i]), "write property error");
    }

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_READ_BY_UUID, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnWriteProperty(
    int32_t ret, const NearlinkSsapPropertyParcel &property)
{
    HILOGI("Enter");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret error");
    NL_CHECK_RETURN(data.WriteParcelable(&property), "write property error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_WRITE, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnSetPropertyNotification(
    int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret error");
    NL_CHECK_RETURN(data.WriteBool(enable), "write enable error");
    NL_CHECK_RETURN(data.WriteParcelable(&property), "write property error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_SET_NOTIFY, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnSetPropertyIndication(
    int32_t ret, bool enable, const NearlinkSsapPropertyParcel &property)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret error");
    NL_CHECK_RETURN(data.WriteBool(enable), "write enable error");
    NL_CHECK_RETURN(data.WriteParcelable(&property), "write property error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_PROPERTY_SET_INDICATE, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnReadDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret error");
    NL_CHECK_RETURN(data.WriteParcelable(&descriptor), "write descriptor error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_DESCRIPTOR_READ, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnWriteDescriptor(int32_t ret, const NearlinkSsapDescriptorParcel &descriptor)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(ret), "write ret error");
    NL_CHECK_RETURN(data.WriteParcelable(&descriptor), "write descriptor error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_DESCRIPTOR_WRITE, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnMtuChanged(int32_t state, uint16_t mtu)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(state), "write state error");
    NL_CHECK_RETURN(data.WriteUint16(mtu), "write mtu error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_MTU_UPDATE, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnServicesDiscovered(int32_t status)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(status), "write status error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_SERVICES_DISCOVER, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnServicesDiscoveredByUuid(int32_t status, const Uuid &uuid)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(status), "write status error");
    NearlinkUuidParcel uid = NearlinkUuidParcel(uuid);
    NL_CHECK_RETURN(data.WriteParcelable(&uid), "write uuid error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_SERVICES_DISCOVER_BY_UUID,
        option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnConnectionParameterChanged(
    int32_t interval, int32_t latency, int32_t timeout, int32_t status)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(interval), "write interval error");
    NL_CHECK_RETURN(data.WriteInt32(latency), "write latency error");
    NL_CHECK_RETURN(data.WriteInt32(timeout), "write timeout error");
    NL_CHECK_RETURN(data.WriteInt32(status), "write status error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_CONNECTION_PARA_CHANGE, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnServicesRediscovered(const std::vector<NearlinkSsapServiceParcel> &services)
{
    HILOGI("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(services.size()), "write serviceNum error");
    for (const auto &service : services) {
        NL_CHECK_RETURN(data.WriteParcelable(&service), "write service error");
    }

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_SERVICES_REDISCOVERED, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSsapClientCallbackProxy::OnServiceChanged(uint16_t handle, const NearlinkUuidParcel &uuid)
{
    HILOGI("handle: %{public}u", handle);
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSsapClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteUint16(handle), "write handle error");
    NL_CHECK_RETURN(data.WriteParcelable(&uuid), "write uuid error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkSsapClientCallbackInterfaceCode::NL_SSAP_CLIENT_CALLBACK_SERVICE_CHANGED, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

ErrCode NearlinkSsapClientCallbackProxy::InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data,
    MessageParcel &reply)
{
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, OBJECT_NULL, "[InnerTransact] fail: get Remote fail code %{public}d", code);
    ErrCode err = remote->SendRequest(code, data, reply, flags);
    switch (err) {
        case NO_ERROR: {
            return NO_ERROR;
        }
        case DEAD_OBJECT: {
            HILOGW("fail: ipcErr=%{public}d code %{public}d", err, code);
            return DEAD_OBJECT;
        }
        default: {
            HILOGW("fail: ipcErr=%{public}d code %{public}d", err, code);
            return TRANSACTION_ERR;
        }
    }
}
}  // namespace Nearlink
}  // namespace OHOS