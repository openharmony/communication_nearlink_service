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

#include "nearlink_sle_datatransfer_proxy.h"
#include "nearlink_errorcode.h"
#include "log.h"
#include "nearlink_uuid_parcel.h"

namespace OHOS::Nearlink {
NearlinkSleDataTransferProxy::NearlinkSleDataTransferProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkSleDataTransfer>(impl)
{}

NearlinkSleDataTransferProxy::~NearlinkSleDataTransferProxy()
{}

NlErrCode NearlinkSleDataTransferProxy::RegisterSleDataTransferCallback(
    const sptr<INearlinkSleDataTransferCallback> &callback)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleDataTransferProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(
        data.WriteRemoteObject(callback->AsObject()), NL_ERR_IPC_TRANS_FAILED, "Write remoteObject error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_REGISTER_SLE_DATATRANSFER_CALLBACK, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleDataTransferProxy::DeregisterSleDataTransferCallback(
    const sptr<INearlinkSleDataTransferCallback> &callback)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleDataTransferProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(
        data.WriteRemoteObject(callback->AsObject()), NL_ERR_IPC_TRANS_FAILED, "Write remoteObject error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_DE_REGISTER_SLE_DATATRANSFER_CALLBACK, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleDataTransferProxy::CreatePort(const std::string &uuid, uint16_t &port)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleDataTransferProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(uuid), NL_ERR_IPC_TRANS_FAILED, "Write uuid error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_CREATE_PORT, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        port = reply.ReadUint16();
    }
    return exception;
}

NlErrCode NearlinkSleDataTransferProxy::DestroyPort(const std::string &uuid, uint16_t port)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleDataTransferProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(uuid), NL_ERR_IPC_TRANS_FAILED, "Write uuid error.");
    NL_CHECK_RETURN_RET(data.WriteUint16(port), NL_ERR_IPC_TRANS_FAILED, "Write port error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_DESTROY_PORT, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleDataTransferProxy::SocketEmptyMsg(uint16_t port, std::string address)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleDataTransferProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteUint16(port), NL_ERR_IPC_TRANS_FAILED, "Write port error.");
    NL_CHECK_RETURN_RET(data.WriteString(address), NL_ERR_IPC_TRANS_FAILED, "Write port error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_SOCKET_EMPTY_PORT, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleDataTransferProxy::Connect(NearlinkSleDataTransferConnectionParams &params)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleDataTransferProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&params), NL_ERR_IPC_TRANS_FAILED, "Write settings error.");
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_CONNECT, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    NlErrCode errorCode = static_cast<NlErrCode>(reply.ReadInt32());
    return errorCode;
}

NlErrCode NearlinkSleDataTransferProxy::Disconnect(NearlinkSleDataTransferConnectionParams &params)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleDataTransferProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&params), NL_ERR_IPC_TRANS_FAILED, "Write settings error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_DISCONNECT, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleDataTransferProxy::GetConnectionState(
    NearlinkSleDataTransferConnectionParams &params, int32_t &connState)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleDataTransferProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED,
        "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&params), NL_ERR_IPC_TRANS_FAILED, "Write con param error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode result = InnerTransact(SLE_GET_CONNECTION_STATE, option, data, reply);
    NL_CHECK_RETURN_RET(result == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", result);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        connState = reply.ReadInt32();
    }
    return exception;
}

#ifdef WATCH_STANDARD
NlErrCode NearlinkSleDataTransferProxy::UpdateConnectInterval(std::string device, int32_t intervalType, bool &result)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleDataTransferProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(device), NL_ERR_IPC_TRANS_FAILED, "Write device error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(intervalType), NL_ERR_IPC_TRANS_FAILED, "Write intervalType error.");

    MessageParcel reply;
    MessageOption option{MessageOption::TF_SYNC};
    ErrCode ret = InnerTransact(SLE_UPDATE_INTERVAL, option, data, reply);
    NL_CHECK_RETURN_RET(ret == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", ret);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        result = reply.ReadBool();
    }
    HILOGI("UpdateConnectInterval result= %{public}d, intervalType = %{public}d", result, intervalType);
    return exception;
}
#endif

ErrCode NearlinkSleDataTransferProxy::InnerTransact(
    uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply)
{
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, OBJECT_NULL, "get Remote fail");
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
}  // namespace OHOS::Nearlink