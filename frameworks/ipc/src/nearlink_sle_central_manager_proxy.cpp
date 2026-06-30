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

#include "nearlink_sle_central_manager_proxy.h"
#include "nearlink_errorcode.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
NearlinkSleCentralManagerProxy::NearlinkSleCentralManagerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkSleCentralManager>(impl)
{}

NearlinkSleCentralManagerProxy::~NearlinkSleCentralManagerProxy()
{}

NlErrCode NearlinkSleCentralManagerProxy::RegisterSleCentralManagerCallback(uint32_t &scannerId,
    bool enableRandomAddrMode, const sptr<INearlinkSleCentralManagerCallback> &callback)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleCentralManagerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write remoteObject error");
    NL_CHECK_RETURN_RET(data.WriteBool(enableRandomAddrMode), NL_ERR_IPC_TRANS_FAILED,
        "Write enableRandomAddrMode error");
    
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_REGISTER_SLE_CENTRAL_MANAGER_CALLBACK, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        scannerId = reply.ReadUint32();
    }
    return exception;
}

NlErrCode NearlinkSleCentralManagerProxy::DeregisterSleCentralManagerCallback(uint32_t scannerId,
    const sptr<INearlinkSleCentralManagerCallback> &callback)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleCentralManagerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteUint32(scannerId), NL_ERR_IPC_TRANS_FAILED, "Write scannerId error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write remoteObject error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_DE_REGISTER_SLE_CENTRAL_MANAGER_CALLBACK, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleCentralManagerProxy::StartScanWithFilter(uint32_t scannerId,
    const NearlinkSleScanSettings &settings, const std::vector<NearlinkSleScanFilter> &filters)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleCentralManagerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteUint32(scannerId), NL_ERR_IPC_TRANS_FAILED, "Write scannerId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&settings), NL_ERR_IPC_TRANS_FAILED, "Write settings error");
    NL_CHECK_RETURN_RET(data.WriteUint32(filters.size()), NL_ERR_IPC_TRANS_FAILED, "Write filters.size error");
    for (size_t i = 0; i < filters.size(); i++) {
        NL_CHECK_RETURN_RET(data.WriteParcelable(&filters[i]), NL_ERR_IPC_TRANS_FAILED,
            "Write filters error");
    }

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_START_SCAN_WITH_FILTER, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleCentralManagerProxy::StartFullScan(uint32_t scannerId, const NearlinkSleScanSettings &settings)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleCentralManagerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteUint32(scannerId), NL_ERR_IPC_TRANS_FAILED, "Write scannerId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&settings), NL_ERR_IPC_TRANS_FAILED, "Write settings error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_START_FULL_SCAN, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleCentralManagerProxy::StopScan(uint32_t scannerId)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleCentralManagerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteUint32(scannerId), NL_ERR_IPC_TRANS_FAILED, "Write scannerId error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_STOP_SCAN, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

int32_t NearlinkSleCentralManagerProxy::InnerTransact(
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
