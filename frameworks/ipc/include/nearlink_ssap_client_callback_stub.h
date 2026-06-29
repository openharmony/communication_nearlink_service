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

#ifndef OHOS_NEARLINK_STANDARD_SSAP_CLIENT_CALLBACK_STUB_H
#define OHOS_NEARLINK_STANDARD_SSAP_CLIENT_CALLBACK_STUB_H

#include <map>

#include "iremote_stub.h"
#include "i_nearlink_ssap_client_callback.h"
namespace OHOS {
namespace Nearlink {
class NearlinkSsapClientCallbackStub : public IRemoteStub<INearlinkSsapClientCallback> {
public:
    NearlinkSsapClientCallbackStub();
    ~NearlinkSsapClientCallbackStub() override;

    int32_t OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    ErrCode OnConnectionStateChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnPropertyChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnEventNotifyInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnPropertyReadInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnMethodCallInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnPropertiesReadByUuidInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnPropertyWriteInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnSetPropertyNotifyInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnSetPropertyIndicateInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnDescriptorReadInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnDescriptorWriteInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnMtuChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnServicesDiscoveredInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnServicesDiscoveredByUuidInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnServicesRediscoveredInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnServiceChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnConnectionParameterChangedInner(MessageParcel &data, MessageParcel &reply);
    void InitConnectionFuncMap();
    void InitPropertyFuncMap();
    using NearlinkHostFunc = ErrCode (NearlinkSsapClientCallbackStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, NearlinkHostFunc> memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkSsapClientCallbackStub);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_SSAP_CLIENT_CALLBACK_STUB_H