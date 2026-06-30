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

#include "nearlink_sle_peripheral_observer_proxy.h"

#include "nearlink_raw_address.h"
#include "log.h"
#include "ipc_types.h"

namespace OHOS {
namespace Nearlink {
NearlinkSlePeripheralObserverProxy::NearlinkSlePeripheralObserverProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkSlePeripheralObserver>(impl)
{}

NearlinkSlePeripheralObserverProxy::~NearlinkSlePeripheralObserverProxy()
{}

void NearlinkSlePeripheralObserverProxy::OnReadRemoteRssiEvent(const NearlinkRawAddress &device, int rssi, int status)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSlePeripheralObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed.");
    NL_CHECK_RETURN(data.WriteInt32(rssi), "write rssi failed.");
    NL_CHECK_RETURN(data.WriteInt32(status), "write status failed.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_ON_READ_REMOTE_RSSI_EVENT, option,
        data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSlePeripheralObserverProxy::OnPairingRequest(const NearlinkRawAddress &device, const std::string &passkey,
    int type)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSlePeripheralObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed.");
    NL_CHECK_RETURN(data.WriteString(passkey), "write passkey error");
    NL_CHECK_RETURN(data.WriteInt32(type), "write type failed.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_PAIRING_REQUEST, option,
    data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSlePeripheralObserverProxy::OnPairStatusChanged(const NearlinkRawAddress &device, int preState, int state,
    int reason)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSlePeripheralObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed.");
    NL_CHECK_RETURN(data.WriteInt32(preState), "write preState failed.");
    NL_CHECK_RETURN(data.WriteInt32(state), "write state failed.");
    NL_CHECK_RETURN(data.WriteInt32(reason), "write reason failed.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_PAIR_STATUS_CHANGED, option,
        data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSlePeripheralObserverProxy::OnAcbStateChanged(const NearlinkRawAddress &device, int state, int reason)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSlePeripheralObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed.");
    NL_CHECK_RETURN(data.WriteInt32(state), "write state failed.");
    NL_CHECK_RETURN(data.WriteUint32(reason), "write reason failed.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_ACB_STATE_CHANGED, option,
        data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSlePeripheralObserverProxy::OnConnectionStateChanged(const NearlinkRawAddress &device, int preState,
    int state, int reason)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSlePeripheralObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device failed.");
    NL_CHECK_RETURN(data.WriteInt32(preState), "write preState failed.");
    NL_CHECK_RETURN(data.WriteInt32(state), "write state failed.");
    NL_CHECK_RETURN(data.WriteInt32(reason), "write reason failed.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_CONNECT_STATE_CHANGED, option,
        data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSlePeripheralObserverProxy::OnLinkFreqBandChanged(const NearlinkRawAddress &device, int32_t freqBand)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSlePeripheralObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device error");
    NL_CHECK_RETURN(data.WriteInt32(freqBand), "write freqBand error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};

    ErrCode result = InnerTransact(NearlinkSlePeripheralObserverInterfaceCode::NL_SLE_LINK_FREQ_BAND_CHANGED,
        option, data, reply);
    NL_CHECK_RETURN(result == NO_ERROR, "done fail, error: %{public}d", result);
}

ErrCode NearlinkSlePeripheralObserverProxy::InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data,
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