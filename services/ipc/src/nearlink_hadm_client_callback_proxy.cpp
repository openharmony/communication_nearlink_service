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

#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_hadm_client_callback_proxy.h"

namespace OHOS {
namespace Nearlink {
NearlinkHadmClientCallbackProxy::NearlinkHadmClientCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkHadmClientCallback>(impl)
{}

NearlinkHadmClientCallbackProxy::~NearlinkHadmClientCallbackProxy()
{}

void NearlinkHadmClientCallbackProxy::OnSoundingResult(
    const NearlinkRawAddress &addr, const NearlinkHadmClientSoundingResult &result)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkHadmClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&addr), "write addr failed.");
    NL_CHECK_RETURN(data.WriteParcelable(&result), "write result failed.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NL_HADM_CLIENT_CALLBACK_RESULT_EVENT, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

void NearlinkHadmClientCallbackProxy::OnSoundingStateChange(const NearlinkRawAddress &addr,
    int newState, int errorCode)
{
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkHadmClientCallbackProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&addr), "write addr failed.");
    NL_CHECK_RETURN(data.WriteInt32(newState), "write newState failed.");
    NL_CHECK_RETURN(data.WriteInt32(errorCode), "write errorcode failed.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NL_HADM_CLIENT_CALLBACK_STATE_EVENT, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

ErrCode NearlinkHadmClientCallbackProxy::InnerTransact(
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
            HILOGE("ipcErr=%{public}d code %{public}d", err, code);
            return DEAD_OBJECT;
        }
        default: {
            HILOGE("ipcErr=%{public}d code %{public}d", err, code);
            return TRANSACTION_ERR;
        }
    }
}
}  // namespace Nearlink
}  // namespace OHOS
