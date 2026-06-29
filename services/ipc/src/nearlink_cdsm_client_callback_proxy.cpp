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

#include "log.h"
#include "nearlink_errorcode.h"
#include "nearlink_cdsm_client_callback_proxy.h"

namespace OHOS {
namespace Nearlink {

NearlinkCdsmClientCallbackProxy::NearlinkCdsmClientCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<INearlinkCdsmClientCallback>(impl)
{
    HILOGD("[Cdsm]Enter");
}

NearlinkCdsmClientCallbackProxy::~NearlinkCdsmClientCallbackProxy()
{
    HILOGD("[Cdsm]Enter");
}

void NearlinkCdsmClientCallbackProxy::OnCdsInfoChanged(const NearlinkCdsInfoParcel &cdsInfo)
{
    HILOGD("[Cdsm]Enter");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkCdsmClientCallbackProxy::GetDescriptor()),
        "[Cdsm]Write Token error");
    NL_CHECK_RETURN(data.WriteParcelable(&cdsInfo), "[Cdsm]write result failed.");

    MessageParcel reply;
    MessageOption option = {MessageOption::TF_ASYNC};
    ErrCode error = InnerTransact(NL_CDSM_CLIENT_CALLBACK_RESULT_EVENT, option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "[Cdsm]done fail, error: %{public}d", error);
}

ErrCode NearlinkCdsmClientCallbackProxy::InnerTransact(
    uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply)
{
    HILOGD("[Cdsm]Enter");
    auto remote = Remote();
    NL_CHECK_RETURN_RET(remote, OBJECT_NULL, "[Cdsm] fail: get Remote fail code %{public}d", code);
    ErrCode err = remote->SendRequest(code, data, reply, flags);
    switch (err) {
        case NO_ERROR: {
            return NO_ERROR;
        }
        case DEAD_OBJECT: {
            HILOGE("[Cdsm]ipcErr=%{public}d code %{public}d", err, code);
            return DEAD_OBJECT;
        }
        default: {
            HILOGE("[Cdsm]ipcErr=%{public}d code %{public}d", err, code);
            return TRANSACTION_ERR;
        }
    }
}
}  // namespace Nearlink
}  // namespace OHOS