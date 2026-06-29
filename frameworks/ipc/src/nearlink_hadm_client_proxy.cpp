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

#include "log.h"
#include "nearlink_service_ipc_interface_code.h"
#include "nearlink_hadm_client_proxy.h"

namespace OHOS {
namespace Nearlink {
NearlinkHadmClientProxy::NearlinkHadmClientProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkHadmClient>(impl)
{}

NearlinkHadmClientProxy::~NearlinkHadmClientProxy()
{}

NlErrCode NearlinkHadmClientProxy::RegisterNearlinkHadmClientCallback(uint32_t &hadmId,
    const sptr<INearlinkHadmClientCallback> &callback)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHadmClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write callback error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(
        NearlinkHadmClientInterfaceCode::NL_REGISTER_HADM_CLIENT_CALLBACK, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", result);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        hadmId = reply.ReadUint32();
    }
    return exception;
}

NlErrCode NearlinkHadmClientProxy::DeregisterNearlinkHadmClientCallback(uint32_t hadmId,
    const sptr<INearlinkHadmClientCallback> &callback)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHadmClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteUint32(hadmId), NL_ERR_IPC_TRANS_FAILED, "Write hadmId error");
    NL_CHECK_RETURN_RET(data.WriteRemoteObject(callback->AsObject()), NL_ERR_IPC_TRANS_FAILED,
        "Write callback error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(
        NearlinkHadmClientInterfaceCode::NL_DE_REGISTER_HADM_CLIENT_CALLBACK, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHadmClientProxy::StartSounding(uint32_t hadmId, const NearlinkRawAddress &addr)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHadmClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteUint32(hadmId), NL_ERR_IPC_TRANS_FAILED, "Write hadmId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&addr), NL_ERR_INTERNAL_ERROR, "Write addr error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode result = InnerTransact(
        NearlinkHadmClientInterfaceCode::NL_HADM_CLIENT_START_SOUNDING, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHadmClientProxy::StopSounding(uint32_t hadmId, const NearlinkRawAddress &addr)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHadmClientProxy::GetDescriptor()),
        NL_ERR_INTERNAL_ERROR, "Write Token error");
    NL_CHECK_RETURN_RET(data.WriteUint32(hadmId), NL_ERR_IPC_TRANS_FAILED, "Write hadmId error");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&addr), NL_ERR_INTERNAL_ERROR, "Write addr error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_SYNC
    };
    ErrCode result = InnerTransact(
        NearlinkHadmClientInterfaceCode::NL_HADM_CLIENT_STOP_SOUNDING, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkHadmClientProxy::GetHadmFeature(uint8_t &capability)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkHadmClientProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(
        NearlinkHadmClientInterfaceCode::NL_GET_HADM_FEATURE, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", result);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        capability = reply.ReadUint8();
    }
    return exception;
}

ErrCode NearlinkHadmClientProxy::InnerTransact(
    uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply)
{
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, OBJECT_NULL, "fail: get Remote fail code %{public}d", code);
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