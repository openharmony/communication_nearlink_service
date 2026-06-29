/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "nearlink_vcp_client_proxy.h"
#include "log.h"
#include "i_nearlink_vcp_client.h"
#include "nearlink_host.h"
#include "nearlink_errorcode.h"
#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {

NlErrCode NearlinkVcpClientProxy::SetDeviceAbsoluteVolume(const NearlinkRawAddress &addr, int32_t volumeLevel,
    uint8_t streamType)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkVcpClientProxy::GetDescriptor()),
                        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&addr), NL_ERR_IPC_TRANS_FAILED, "Write device error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(volumeLevel), NL_ERR_IPC_TRANS_FAILED, "Write volumeLevel error.");
    NL_CHECK_RETURN_RET(data.WriteUint8(streamType), NL_ERR_IPC_TRANS_FAILED, "Write streamType error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error =
            InnerTransact(NearlinkVcpInterfaceCode::NL_VCP_CLIENT_SET_DEVICE_ABSOLUTE_VOLUME, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED,
                        "SetDeviceAbsoluteVolume done fail, ErrCode: %{public}d", error);
    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkVcpClientProxy::GetDeviceMediaVolume(const NearlinkRawAddress &addr, int32_t &mediaVolume)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkVcpClientProxy::GetDescriptor()),
                        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&addr), NL_ERR_IPC_TRANS_FAILED, "Write device error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error =
            InnerTransact(NearlinkVcpInterfaceCode::NL_VCP_CLIENT_GET_MEDIA_VOLUME, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED,
                        "GetDeviceMediaVolume done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        mediaVolume = reply.ReadInt32();
    }
    return exception;
}

NlErrCode NearlinkVcpClientProxy::GetDeviceCallVolume(const NearlinkRawAddress &addr, int32_t &callVolume)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkVcpClientProxy::GetDescriptor()),
                        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteParcelable(&addr), NL_ERR_IPC_TRANS_FAILED, "Write device error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error =
            InnerTransact(NearlinkVcpInterfaceCode::NL_VCP_CLIENT_GET_CALL_VOLUME, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED,
                        "GetDeviceCallVolume done fail, ErrCode: %{public}d", error);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        callVolume = reply.ReadInt32();
    }
    return exception;
}

ErrCode NearlinkVcpClientProxy::InnerTransact(
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
            HILOGW("fail: ipcErr=%{public}d code %{public}d", err, code);
            return DEAD_OBJECT;
        }
        default: {
            HILOGW("fail: ipcErr=%{public}d code %{public}d", err, code);
            return TRANSACTION_ERR;
        }
    }
}
}
}