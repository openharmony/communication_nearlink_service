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

#include "log.h"
#include "nearlink_hadm_client_callback_stub.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
namespace OHOS {
namespace Nearlink {
const std::map<uint32_t, std::function<ErrCode(NearlinkHadmClientCallbackStub *, MessageParcel &, MessageParcel &)>>
    NearlinkHadmClientCallbackStub::memberFuncMap_ = {
        {NearlinkHadmClientCallbackInterfaceCode::NL_HADM_CLIENT_CALLBACK_RESULT_EVENT,
            std::bind(&NearlinkHadmClientCallbackStub::OnSoundingResultInner, _1, _2, _3)},
        {NearlinkHadmClientCallbackInterfaceCode::NL_HADM_CLIENT_CALLBACK_STATE_EVENT,
            std::bind(&NearlinkHadmClientCallbackStub::OnSoundingStateChangeResultInner, _1, _2, _3)},
};

NearlinkHadmClientCallbackStub::NearlinkHadmClientCallbackStub()
{
}

NearlinkHadmClientCallbackStub::~NearlinkHadmClientCallbackStub()
{
}

int NearlinkHadmClientCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    HILOGD("cmd = %{public}d, flags= %{public}d", code, option.GetFlags());
    if (NearlinkHadmClientCallbackStub::GetDescriptor() != data.ReadInterfaceToken()) {
        HILOGI("local descriptor is not equal to remote");
        return ERR_INVALID_STATE;
    }

    auto itFunc = memberFuncMap_.find(code);
    if (itFunc != memberFuncMap_.end()) {
        auto memberFunc = itFunc->second;
        if (memberFunc != nullptr) {
            return memberFunc(this, data, reply);
        }
    }

    HILOGW("default case, need check.");
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}

ErrCode NearlinkHadmClientCallbackStub::OnSoundingResultInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> addr(data.ReadParcelable<NearlinkRawAddress>());
    if (!addr) {
        return TRANSACTION_ERR;
    }
    std::shared_ptr<NearlinkHadmClientSoundingResult> result(data.ReadParcelable<NearlinkHadmClientSoundingResult>());
    if (!result) {
        return TRANSACTION_ERR;
    }
    OnSoundingResult(*addr, *result);
    return NO_ERROR;
}

ErrCode NearlinkHadmClientCallbackStub::OnSoundingStateChangeResultInner(MessageParcel &data, MessageParcel &reply)
{
    std::shared_ptr<NearlinkRawAddress> addr(data.ReadParcelable<NearlinkRawAddress>());
    NL_CHECK_RETURN_RET(addr, TRANSACTION_ERR, "addr is null");
    int32_t newState = data.ReadInt32();
    int32_t errorCode = data.ReadInt32();
    OnSoundingStateChange(*addr, newState, errorCode);
    return NO_ERROR;
}
}  // namespace Nearlink
}  // namespace OHOS