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

#include "nearlink_ccp_client_proxy.h"
#include "ipc_types.h"
#include "log.h"
#include "nearlink_ccp_export.h"

namespace OHOS {
namespace Nearlink {

int32_t NearlinkCcpClientProxy::PhoneStateChanged(int32_t callId, int32_t numActive, int32_t numHeld,
    int32_t callState, const std::string &number)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkCcpClientProxy::GetDescriptor()),
                        NL_CCP_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(callId), NL_CCP_ERR_IPC_TRANS_FAILED, "Write callId error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(numActive), NL_CCP_ERR_IPC_TRANS_FAILED, "Write numActive error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(numHeld), NL_CCP_ERR_IPC_TRANS_FAILED, "Write numHeld error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(callState), NL_CCP_ERR_IPC_TRANS_FAILED, "Write callState error.");
    NL_CHECK_RETURN_RET(data.WriteString(number), NL_CCP_ERR_IPC_TRANS_FAILED, "Write number error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error =
            InnerTransact(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_PHONE_STATE_CHANGED, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_CCP_ERR_IPC_TRANS_FAILED,
                        "PhoneStateChanged done fail, ErrCode: %{public}d", error);
    return reply.ReadInt32();
}

int32_t NearlinkCcpClientProxy::CallDetailsChanged(int32_t callId, int32_t callState, int32_t callType)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkCcpClientProxy::GetDescriptor()),
                        NL_CCP_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(callId), NL_CCP_ERR_IPC_TRANS_FAILED, "Write callId error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(callState), NL_CCP_ERR_IPC_TRANS_FAILED, "Write callState error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(callType), NL_CCP_ERR_IPC_TRANS_FAILED, "Write callType error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error =
            InnerTransact(NearlinkCcpInterfaceCode::NL_CCP_CLIENT_CALL_DETAILS_CHANGED, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_CCP_ERR_IPC_TRANS_FAILED,
                        "CallDetailsChanged done fail, ErrCode: %{public}d", error);
    return reply.ReadInt32();
}

ErrCode NearlinkCcpClientProxy::InnerTransact(
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
}  // namespace Nearlink
}  // namespace OHOS
