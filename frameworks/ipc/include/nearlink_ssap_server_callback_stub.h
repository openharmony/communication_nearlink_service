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

#ifndef OHOS_NEARLINK_STANDARD_SSAP_SERVER_CALLBACK_STUB_H
#define OHOS_NEARLINK_STANDARD_SSAP_SERVER_CALLBACK_STUB_H

#include <map>

#include "iremote_stub.h"
#include "i_nearlink_host.h"
#include "i_nearlink_ssap_server_callback.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSsapServerCallbackStub : public IRemoteStub<INearlinkSsapServerCallback> {
public:
    NearlinkSsapServerCallbackStub();
    virtual ~NearlinkSsapServerCallbackStub();

    int OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    ErrCode OnMtuChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnAddServiceInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnPropertyReadRequestInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnDescriptorReadRequestInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnPropertyWriteRequestInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnDescriptorWriteRequestInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnNotifyPropertyChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnNotifyEventChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnConnectionStateChangedInner(MessageParcel &data, MessageParcel &reply);

    using NearlinkSsapServerCallbackFunc = ErrCode (NearlinkSsapServerCallbackStub::*)(
        MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, NearlinkSsapServerCallbackFunc> memberFuncMap_;

    DISALLOW_COPY_AND_MOVE(NearlinkSsapServerCallbackStub);
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // OHOS_NEARLINK_STANDARD_SSAP_SERVER_CALLBACK_STUB_H