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

#include "nearlink_sle_datatransfer_callback_proxy.h"
#include "log.h"

namespace OHOS::Nearlink {
NearlinkSleDataTransferCallbackProxy::NearlinkSleDataTransferCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkSleDataTransferCallback>(impl)
{}

NearlinkSleDataTransferCallbackProxy::~NearlinkSleDataTransferCallbackProxy()
{}

void NearlinkSleDataTransferCallbackProxy::OnConnectionStateChanged(
    const NearlinkSleDataTransferConnectionParams &connectionParams, int fd)
{
    MessageParcel data;
    NL_CHECK_RETURN(
        data.WriteInterfaceToken(NearlinkSleDataTransferCallbackProxy::GetDescriptor()), "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&connectionParams), "write connect failed");
    if (fd != -1) {
        if (!data.WriteFileDescriptor(fd)) {
            HILOGE("write fd failed");
            close(fd);
            return;
        }
        close(fd);
    }
    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NL_SLE_DATATRANSFER_CALLBACK_CONNECION_STATE_CHANGE_EVENT, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

ErrCode NearlinkSleDataTransferCallbackProxy::InnerTransact(
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
