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

#include "nearlink_tws_client_observer_stub.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
NearlinkTwsClientObserverStub::NearlinkTwsClientObserverStub()
{
    HILOGD("start.");
    memberFuncMap_[static_cast<uint32_t>(
        NearlinkTwsClientObserverInterfaceCode::NL_TWS_OBSERVER_REMOTE_INFO)] =
        &NearlinkTwsClientObserverStub::OnTwsRemoteInfoInner;
}

NearlinkTwsClientObserverStub::~NearlinkTwsClientObserverStub()
{
    HILOGD("start.");
    memberFuncMap_.clear();
}

int NearlinkTwsClientObserverStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    HILOGD("NearlinkTwsClientObserverStub::OnRemoteRequest, cmd = %{public}d, flags= %{public}d",
        code, option.GetFlags());
    if (NearlinkTwsClientObserverStub::GetDescriptor() != data.ReadInterfaceToken()) {
        HILOGI("local descriptor is not equal to remote");
        return ERR_INVALID_STATE;
    }
    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return (this->*memberFunc)(data, reply);
        }
    }
    HILOGW("NearlinkTwsClientObserverStub::OnRemoteRequest, default case, need check.");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode NearlinkTwsClientObserverStub::OnTwsRemoteInfoInner(MessageParcel &data, MessageParcel &reply)
{
    HILOGD("NearlinkTwsClientObserverStub::OnTwsRemoteInfoInner Triggered!");
    std::string address;
    NL_CHECK_RETURN_RET(data.ReadString(address), TRANSACTION_ERR, "ReadString failed.");
    std::vector<uint8_t> dataValue;
    NL_CHECK_RETURN_RET(data.ReadUInt8Vector(&dataValue), TRANSACTION_ERR, "ReadUInt8Vector dataValue failed.");
    OnTwsRemoteInfo(address, dataValue);
    return NO_ERROR;
}

}  // namespace Nearlink
}  // namespace OHOS