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

#include "nearlink_device_rssi_observer_proxy.h"

#include "nearlink_raw_address.h"
#include "log.h"
#include "ipc_types.h"

namespace OHOS {
namespace Nearlink {
NearlinkDeviceRssiObserverProxy::NearlinkDeviceRssiObserverProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkDeviceRssiObserver>(impl)
{
}

NearlinkDeviceRssiObserverProxy::~NearlinkDeviceRssiObserverProxy() {}

void NearlinkDeviceRssiObserverProxy::OnReadRemoteRssiEvent(const NearlinkRawAddress &device, int rssi, int status)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkDeviceRssiObserverProxy::GetDescriptor()), "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&device), "write device error");
    NL_CHECK_RETURN(data.WriteInt32(rssi), "write rssi failed.");
    NL_CHECK_RETURN(data.WriteInt32(status), "write status failed.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};

    ErrCode result = InnerTransact(NearlinkDeviceRssiObserverInterfaceCode::NL_SLE_GET_RSSI_EVENT, option, data, reply);
    NL_CHECK_RETURN(result == NO_ERROR, "done fail, error: %{public}d", result);
}

ErrCode NearlinkDeviceRssiObserverProxy::InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data,
                                                       MessageParcel &reply)
{
    auto remote = Remote();
    if (remote == nullptr) {
        HILOGW("fail: get Remote fail code %{public}d", code);
        return OBJECT_NULL;
    }
    int err = remote->SendRequest(code, data, reply, flags);
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