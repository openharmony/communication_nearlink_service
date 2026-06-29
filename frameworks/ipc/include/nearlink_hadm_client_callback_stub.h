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

#ifndef NEARLINK_HADM_CLIENT_CALLBACK_STUB_H
#define NEARLINK_HADM_CLIENT_CALLBACK_STUB_H

#include <map>
#include "i_nearlink_hadm_client_callback.h"
#include "iremote_stub.h"

namespace OHOS {
namespace Nearlink {
class NearlinkHadmClientCallbackStub : public IRemoteStub<INearlinkHadmClientCallback> {
public:
    NearlinkHadmClientCallbackStub();
    ~NearlinkHadmClientCallbackStub() override;

    int OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    ErrCode OnSoundingResultInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnSoundingStateChangeResultInner(MessageParcel &data, MessageParcel &reply);
    static const std::map<uint32_t,
        std::function<ErrCode(NearlinkHadmClientCallbackStub *, MessageParcel &, MessageParcel &)>>
        memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkHadmClientCallbackStub);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif // NEARLINK_HADM_CLIENT_CALLBACK_STUB_H
