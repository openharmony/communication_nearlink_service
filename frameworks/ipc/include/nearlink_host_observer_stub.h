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
#ifndef OHOS_NEARLINK_STANDARD_HOST_OBSERVER_STUB_H
#define OHOS_NEARLINK_STANDARD_HOST_OBSERVER_STUB_H

#include <map>

#include "i_nearlink_host_observer.h"
#include "iremote_stub.h"

namespace OHOS {
namespace Nearlink {
class NearlinkHostObserverStub : public IRemoteStub<INearlinkHostObserver> {
public:
    NearlinkHostObserverStub();
    ~NearlinkHostObserverStub();

    int32_t OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    ErrCode OnSystemStateChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnStateChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnFullStateChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnSwitchStateChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnDisableResponseInner(MessageParcel &data, MessageParcel &reply);

    ErrCode OnPairConfirmedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnDeviceNameChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnDeviceAddrChangedInner(MessageParcel &data, MessageParcel &reply);

    using NearlinkObserverHostFunc = ErrCode (NearlinkHostObserverStub::*)(MessageParcel &data, MessageParcel &reply);
    std::map<uint32_t, NearlinkObserverHostFunc> memberFuncMap_;

    DISALLOW_COPY_AND_MOVE(NearlinkHostObserverStub);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_HOST_OBSERVER_STUB_H
