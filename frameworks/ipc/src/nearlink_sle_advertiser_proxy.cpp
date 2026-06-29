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

#include "nearlink_sle_advertiser_proxy.h"
#include "nearlink_errorcode.h"
#include "log.h"
#include "nearlink_uuid_parcel.h"

namespace OHOS {
namespace Nearlink {
NearlinkSleAdvertiserProxy::NearlinkSleAdvertiserProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkSleAdvertiser>(impl)
{}

NearlinkSleAdvertiserProxy::~NearlinkSleAdvertiserProxy()
{}

NlErrCode NearlinkSleAdvertiserProxy::RegisterSleAdvertiserCallback(const sptr<INearlinkSleAdvertiseCallback> &callback)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleAdvertiserProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write remoteObject error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_REGISTER_SLE_ADVERTISER_CALLBACK, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleAdvertiserProxy::DeregisterSleAdvertiserCallback(
    const sptr<INearlinkSleAdvertiseCallback> &callback)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleAdvertiserProxy::GetDescriptor()), NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write remoteObject error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_DE_REGISTER_SLE_ADVERTISER_CALLBACK, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleAdvertiserProxy::StartAdvertising(const NearlinkSleAdvertiserSettings &settings,
    const NearlinkSleAdvertiserData &advData, const NearlinkSleAdvertiserData &scanResponse, int32_t advHandle)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleAdvertiserProxy::GetDescriptor()), NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&settings), NL_ERR_IPC_TRANS_FAILED, "Write settings error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&advData), NL_ERR_IPC_TRANS_FAILED, "Write advData error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&scanResponse), NL_ERR_IPC_TRANS_FAILED, "Write scanResponse error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(advHandle), NL_ERR_IPC_TRANS_FAILED, "Write advHandle error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_START_ADVERTISING, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleAdvertiserProxy::StopAdvertising(int32_t advHandle)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleAdvertiserProxy::GetDescriptor()), NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(advHandle), NL_ERR_IPC_TRANS_FAILED, "Write advHandle error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_STOP_ADVERTISING, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleAdvertiserProxy::GetAdvertiserHandle(int32_t &advHandle)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleAdvertiserProxy::GetDescriptor()), NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_GET_ADVERTISER_HANDLE, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);

    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        advHandle = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkSleAdvertiserProxy::SetAdvertisingData(const NearlinkSleAdvertiserData &advData,
    const NearlinkSleAdvertiserData &scanResponse, int32_t advHandle)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleAdvertiserProxy::GetDescriptor()), NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&advData), NL_ERR_IPC_TRANS_FAILED, "Write advData error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&scanResponse), NL_ERR_IPC_TRANS_FAILED, "Write scanResponse error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(advHandle), NL_ERR_IPC_TRANS_FAILED, "Write advHandle error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_SET_ADVERTISING_DATA, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleAdvertiserProxy::EnableAdvertising(int32_t advHandle)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleAdvertiserProxy::GetDescriptor()), NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(advHandle), NL_ERR_IPC_TRANS_FAILED, "Write advHandle error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_ENABLE_ADVERTISING, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleAdvertiserProxy::DisableAdvertising(int32_t advHandle)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleAdvertiserProxy::GetDescriptor()), NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(advHandle), NL_ERR_IPC_TRANS_FAILED, "Write advHandle error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_DISABLE_ADVERTISING, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

ErrCode NearlinkSleAdvertiserProxy::InnerTransact(
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