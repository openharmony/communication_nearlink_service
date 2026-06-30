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

#include <map>

#include "log.h"
#include "ipc_types.h"
#include "nearlink_uuid_parcel.h"
#include "nearlink_def.h"
#include "nearlink_sle_datatransfer_stub.h"
#include "nearlink_permission_manager.h"
#ifdef HICOLLIE_ENABLE
#include "nearlink_hicollie_adapter.h"
#endif

#ifdef STUB_FUNC
#undef STUB_FUNC
#endif
#define STUB_FUNC(code, func, perm)             \
    NearlinkSleDataTransferInterfaceCode::code, \
    {                                           \
        NearlinkSleDataTransferStub::func, perm \
    }

namespace OHOS::Nearlink {

NearlinkSleDataTransferStub::NearlinkSleDataTransferStub()
{
    HILOGI("enter");
    // Note: Permissions need to be configured when the itf to be used. "nullptr" means no permission needed.
    memberFuncMap_ = {
        {STUB_FUNC(SLE_REGISTER_SLE_DATATRANSFER_CALLBACK,
            RegisterSleDataTransferCallbackInner,
            CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_DE_REGISTER_SLE_DATATRANSFER_CALLBACK,
            DeregisterSleDataTransferCallbackInner,
            CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_CREATE_PORT, CreatePortInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_DESTROY_PORT, DestroyPortInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_SOCKET_EMPTY_PORT, SocketEmptyMsgInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_CONNECT, ConnectInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_DISCONNECT, DisconnectInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_GET_CONNECTION_STATE, GetConnectionStateInner, CHECK_PERM(false, {ACCESS_NEARLINK}))},
        {STUB_FUNC(SLE_UPDATE_INTERVAL, UpdateConnectIntervalInner,
            CHECK_PERM(false, {MANAGE_NEARLINK}))},
    };
}

NearlinkSleDataTransferStub::~NearlinkSleDataTransferStub()
{}

int32_t NearlinkSleDataTransferStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
#ifdef HICOLLIE_ENABLE
    NearlinkHicollieAdapter hicollieAdapter("NearlinkSleDataTransfer", code);
#endif
    CHECK_PERMISSION_AND_EXECUTE(NearlinkSleDataTransferStub);
}

int32_t NearlinkSleDataTransferStub::RegisterSleDataTransferCallbackInner(
    NearlinkSleDataTransferStub *stub, MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkSleDataTransferCallback> callBack = OHOS::iface_cast<INearlinkSleDataTransferCallback>(remote);
    NL_CHECK_RETURN_RET(callBack, TRANSACTION_ERR, "callBack is nullptr");
    NlErrCode status = stub->RegisterSleDataTransferCallback(callBack);
    bool ret = reply.WriteInt32(status);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply register DT cb failed");
    return NO_ERROR;
}

int32_t NearlinkSleDataTransferStub::DeregisterSleDataTransferCallbackInner(
    NearlinkSleDataTransferStub *stub, MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remote = data.ReadRemoteObject();
    const sptr<INearlinkSleDataTransferCallback> callBack = OHOS::iface_cast<INearlinkSleDataTransferCallback>(remote);
    NL_CHECK_RETURN_RET(callBack, TRANSACTION_ERR, "callBack is nullptr");
    NlErrCode status = stub->DeregisterSleDataTransferCallback(callBack);
    bool ret = reply.WriteInt32(status);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply Deregister DT cb failed");
    return NO_ERROR;
}

int32_t NearlinkSleDataTransferStub::CreatePortInner(
    NearlinkSleDataTransferStub *stub, MessageParcel &data, MessageParcel &reply)
{
    std::string uuid = data.ReadString();
    uint16_t port = 0;
    NlErrCode status = stub->CreatePort(uuid, port);
    bool ret = reply.WriteInt32(status);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply CreatePort failed");
    ret = reply.WriteUint16(port);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply port failed");
    return NO_ERROR;
}

int32_t NearlinkSleDataTransferStub::DestroyPortInner(
    NearlinkSleDataTransferStub *stub, MessageParcel &data, MessageParcel &reply)
{
    std::string uuid = data.ReadString();
    uint16_t port = data.ReadUint16();
    NlErrCode status = stub->DestroyPort(uuid, port);
    bool ret = reply.WriteInt32(status);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply destroy failed");
    return NO_ERROR;
}

int32_t NearlinkSleDataTransferStub::SocketEmptyMsgInner(
    NearlinkSleDataTransferStub *stub, MessageParcel &data, MessageParcel &reply)
{
    uint16_t port = data.ReadUint16();
    std::string address = data.ReadString();
    NlErrCode status = stub->SocketEmptyMsg(port, address);
    bool ret = reply.WriteInt32(status);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply empty failed");
    return NO_ERROR;
}


int32_t NearlinkSleDataTransferStub::ConnectInner(
    NearlinkSleDataTransferStub *stub, MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkSleDataTransferConnectionParams> params(
        data.ReadParcelable<NearlinkSleDataTransferConnectionParams>());
    NL_CHECK_RETURN_RET(params, TRANSACTION_ERR, "read connection params failed");
    NlErrCode status = stub->Connect(*params);
    bool ret = reply.WriteInt32(status);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply connect failed");
    return NO_ERROR;
}

int32_t NearlinkSleDataTransferStub::DisconnectInner(
    NearlinkSleDataTransferStub *stub, MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkSleDataTransferConnectionParams> params(
        data.ReadParcelable<NearlinkSleDataTransferConnectionParams>());
    NL_CHECK_RETURN_RET(params, TRANSACTION_ERR, "read connection params failed");

    NlErrCode status = stub->Disconnect(*params);
    bool ret = reply.WriteInt32(status);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply disconnect failed");
    return NO_ERROR;
}

int32_t NearlinkSleDataTransferStub::GetConnectionStateInner(
    NearlinkSleDataTransferStub *stub, MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkSleDataTransferConnectionParams> params(
        data.ReadParcelable<NearlinkSleDataTransferConnectionParams>());
    NL_CHECK_RETURN_RET(params, TRANSACTION_ERR, "read connection state params failed");

    int32_t connState = 0;
    NlErrCode status = stub->GetConnectionState(*params, connState);
    bool ret = reply.WriteInt32(status);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply GetConnectionState failed");
    ret = reply.WriteInt32(connState);
    NL_CHECK_RETURN_RET(ret, TRANSACTION_ERR, "reply connState failed");
    return NO_ERROR;
}

int32_t NearlinkSleDataTransferStub::UpdateConnectIntervalInner(
    NearlinkSleDataTransferStub *stub, MessageParcel &data, MessageParcel &reply)
{
#ifdef WATCH_STANDARD
    std::string device;
    NL_CHECK_RETURN_RET(data.ReadString(device), TRANSACTION_ERR, "Read address failed.");
    int32_t intervalType;
    NL_CHECK_RETURN_RET(data.ReadInt32(intervalType), TRANSACTION_ERR, "Read type failed.");
    bool result = false;
    NlErrCode status = stub->UpdateConnectInterval(device, intervalType, result);
    HILOGI("status: %{public}d, result: %{public}d, intervalType: %{public}d", status, result, intervalType);
    NL_CHECK_RETURN_RET(reply.WriteInt32(status), TRANSACTION_ERR, "WriteInt32 failed.");
    NL_CHECK_RETURN_RET(reply.WriteBool(result), TRANSACTION_ERR, "WriteBool failed.");
#endif
    return NO_ERROR;
}
}  // namespace OHOS::Nearlink
