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

#include "nearlink_sle_central_manager_callback_proxy.h"
#include "log.h"
#include "ipc_types.h"

namespace OHOS {
namespace Nearlink {
NearlinkSleCentralManagerCallBackProxy::NearlinkSleCentralManagerCallBackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkSleCentralManagerCallback>(impl)
{}
NearlinkSleCentralManagerCallBackProxy::~NearlinkSleCentralManagerCallBackProxy()
{}

void NearlinkSleCentralManagerCallBackProxy::OnScanCallback(const NearlinkSleScanResult &result)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSleCentralManagerCallBackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&result), "write result failed");
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(
        NearlinkSleCentralManagerCallbackInterfaceCode::NL_SLE_CENTRAL_MANAGER_CALLBACK, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSleCentralManagerCallBackProxy::OnStartOrStopScanEvent(int resultCode, bool isStartScan)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkSleCentralManagerCallBackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteInt32(resultCode), "write resultCode failed");
    NL_CHECK_RETURN(data.WriteBool(isStartScan), "write isStartScan failed");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(
        NearlinkSleCentralManagerCallbackInterfaceCode::NL_SLE_CENTRAL_MANAGER_CALLBACK_SCAN_FAILED,
        option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkSleCentralManagerCallBackProxy::OnSleBatchScanResultsEvent(std::vector<NearlinkSleScanResult> &results)
{
    HILOGI("not implentment");
}

ErrCode NearlinkSleCentralManagerCallBackProxy::InnerTransact(
    uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply)
{
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, OBJECT_NULL, "[InnerTransact] fail: get Remote fail code %{public}d", code);
    ErrCode err = remote->SendRequest(code, data, reply, flags);
    switch (err) {
        case NO_ERROR: {
            return NO_ERROR;
        }
        case DEAD_OBJECT: {
            HILOGW("[InnerTransact] fail: ipcErr=%{public}d code %{public}d", err, code);
            return DEAD_OBJECT;
        }
        default: {
            HILOGW("[InnerTransact] fail: ipcErr=%{public}d code %{public}d", err, code);
            return TRANSACTION_ERR;
        }
    }
}
}  // namespace Nearlink
}  // namespace OHOS