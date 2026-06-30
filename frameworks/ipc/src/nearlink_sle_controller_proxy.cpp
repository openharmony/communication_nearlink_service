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

#include "nearlink_sle_controller_proxy.h"
#include "nearlink_errorcode.h"
#include "log.h"

namespace OHOS::Nearlink {
NearlinkSleControllerProxy::NearlinkSleControllerProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkSleController>(impl)
{}

NearlinkSleControllerProxy::~NearlinkSleControllerProxy()
{}

NlErrCode NearlinkSleControllerProxy::SetSleCoexParam(uint16_t maxBitRate, uint8_t dutyCycle)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleControllerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteUint16(maxBitRate), NL_ERR_IPC_TRANS_FAILED, "Write maxBitRate error.");
    NL_CHECK_RETURN_RET(data.WriteUint8(dutyCycle), NL_ERR_IPC_TRANS_FAILED, "Write dutyCycle error.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_SYNC};
    ErrCode error = InnerTransact(NearlinkSleControllerInterfaceCode::NL_SET_SLE_COEX_PARAM, option, data, reply);
    NL_CHECK_RETURN_RET(error == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, ErrCode: %{public}d", error);

    return static_cast<NlErrCode>(reply.ReadInt32());
}

NlErrCode NearlinkSleControllerProxy::UpdateConnectInterval(const std::string &device, int32_t intervalType)
{
    MessageParcel data;
    NL_CHECK_RETURN_RET(data.WriteInterfaceToken(NearlinkSleControllerProxy::GetDescriptor()),
        NL_ERR_IPC_TRANS_FAILED, "Write Token error.");
    NL_CHECK_RETURN_RET(data.WriteString(device), NL_ERR_IPC_TRANS_FAILED, "Write device error.");
    NL_CHECK_RETURN_RET(data.WriteInt32(intervalType), NL_ERR_IPC_TRANS_FAILED, "Write intervalType error.");

    MessageParcel reply;
    MessageOption option{MessageOption::TF_SYNC};
    ErrCode ret = InnerTransact(NearlinkSleControllerInterfaceCode::NL_SLE_UPDATE_INTERVAL, option, data, reply);
    NL_CHECK_RETURN_RET(ret == NO_ERROR, NL_ERR_IPC_TRANS_FAILED, "done fail, error: %{public}d", ret);
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    return exception;
}

ErrCode NearlinkSleControllerProxy::InnerTransact(
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
