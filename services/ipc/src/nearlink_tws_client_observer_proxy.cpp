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

#include "nearlink_tws_client_observer_proxy.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
void NearlinkTwsClientObserverProxy::OnTwsRemoteInfo(const std::string &address,
    const std::vector<uint8_t> &value)
{
    HILOGD("Triggered!");
    MessageParcel data;
    NL_CHECK_RETURN(data.WriteInterfaceToken(NearlinkTwsClientObserverProxy::GetDescriptor()),
        "Write Token error");
    NL_CHECK_RETURN(data.WriteString(address), "Write address error");
    NL_CHECK_RETURN(data.WriteUInt8Vector(value), "Write value error");

    MessageParcel reply;
    MessageOption option {
        MessageOption::TF_ASYNC
    };
    ErrCode error = InnerTransact(
        NearlinkTwsClientObserverInterfaceCode::NL_TWS_OBSERVER_REMOTE_INFO,
        option, data, reply);
    NL_CHECK_RETURN(error == NO_ERROR, "done fail, error: %{public}d", error);
}

ErrCode NearlinkTwsClientObserverProxy::InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data,
    MessageParcel &reply)
{
    auto remote = Remote();
    if (remote == nullptr) {
        HILOGW("fail: get Remote fail code %{public}d", code);
        return OBJECT_NULL;
    }
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
