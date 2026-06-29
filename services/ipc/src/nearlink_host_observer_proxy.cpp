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

#include "nearlink_host_observer_proxy.h"
#include "log.h"
#include "nearlink_errorcode.h"

namespace OHOS {
namespace Nearlink {
void NearlinkHostObserverProxy::OnStateChanged(int32_t transport, int32_t state)
{
    HILOGD("Enter");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkHostObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(transport), "write transport error");
    NL_CHECK_RETURN(data.WriteInt32(state), "write state error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};

    ErrCode result = InnerTransact(NL_HOST_OBSERVER_STATE_CHANGE, option, data, reply);
    NL_CHECK_RETURN(result == NO_ERROR, "done fail, error: %{public}d", result);
}

void NearlinkHostObserverProxy::OnFullStateChanged(int32_t transport, int32_t state)
{
    HILOGD("Enter");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkHostObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(transport), "write transport error");
    NL_CHECK_RETURN(data.WriteInt32(state), "write state error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};

    ErrCode result = InnerTransact(NL_HOST_OBSERVER_FULL_STATE_CHANGE, option, data, reply);
    NL_CHECK_RETURN(result == NO_ERROR, "done fail, error: %{public}d", result);
}

void NearlinkHostObserverProxy::OnSwitchStateChanged(int32_t state)
{
    HILOGD("Enter");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkHostObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(state), "write state error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};

    ErrCode result = InnerTransact(NL_HOST_OBSERVER_SWITCH_STATE_CHANGED, option, data, reply);
    NL_CHECK_RETURN(result == NO_ERROR, "done fail, error: %{public}d", result);
}

void NearlinkHostObserverProxy::OnDisableResponse(bool isHalfDisable)
{
    HILOGD("Enter");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkHostObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteBool(isHalfDisable), "write isHalfDisable error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};

    ErrCode result = InnerTransact(NL_HOST_OBSERVER_DISABLE_RESPONSE, option, data, reply);
    NL_CHECK_RETURN(result == NO_ERROR, "done fail, error: %{public}d", result);
}

void NearlinkHostObserverProxy::OnPairConfirmed(
    const int32_t transport, const NearlinkRawAddress &device, int reqType, int number)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkHostObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(transport), "write transport error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device error");
    NL_CHECK_RETURN(data.WriteInt32(reqType), "write reqType error");
    NL_CHECK_RETURN(data.WriteInt32(number), "write number error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};

    ErrCode result = InnerTransact(NL_HOST_OBSERVER_PAIR_CONFIRMED, option, data, reply);
    NL_CHECK_RETURN(result == NO_ERROR, "done fail, error: %{public}d", result);
}

void NearlinkHostObserverProxy::OnDeviceNameChanged(const std::string &deviceName)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkHostObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteString(deviceName), "write deviceName error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode result = InnerTransact(NL_HOST_OBSERVER_DEVICE_NAME_CHANGED, option, data, reply);
    NL_CHECK_RETURN(result == NO_ERROR, "done fail, error: %{public}d", result);
}

void NearlinkHostObserverProxy::OnDeviceAddrChanged(const std::string &address)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkHostObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteString(address), "write address error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};

    ErrCode result = InnerTransact(NL_HOST_OBSERVER_DEVICE_ADDR_CHANGED, option, data, reply);
    NL_CHECK_RETURN(result == NO_ERROR, "done fail, error: %{public}d", result);
}

ErrCode NearlinkHostObserverProxy::InnerTransact(
    uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply)
{
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, ERR_DEAD_OBJECT, "[InnerTransact] fail: get Remote fail code %{public}d", code);
    ErrCode err = remote->SendRequest(code, data, reply, flags);
    switch (err) {
        case NO_ERROR: {
            return ERR_OK;
        }
        case DEAD_OBJECT: {
            HILOGE("[InnerTransact] fail: ipcErr=%{public}d code %{public}d", err, code);
            return ERR_DEAD_OBJECT;
        }
        default: {
            HILOGE("[InnerTransact] fail: ipcErr=%{public}d code %{public}d", err, code);
            return TRANSACTION_ERR;
        }
    }
}
}  // namespace Nearlink
}  // namespace OHOS