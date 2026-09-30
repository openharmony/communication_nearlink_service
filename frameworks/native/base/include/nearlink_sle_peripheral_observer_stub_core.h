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
#ifndef NEARLINK_SLE_PERIPHERAL_OBSERVER_STUB_CORE_H
#define NEARLINK_SLE_PERIPHERAL_OBSERVER_STUB_CORE_H

#include "nearlink_sle_peripheral_observer_stub.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief base.so implementation class of framework SLE peripheral observer stub.
 *
 * Kept in sync with NearlinkSlePeripheralObserverStub declared in
 * nearlink_sle_peripheral_observer_stub.h.
 */
class NearlinkSlePeripheralObserverStubCore : public IRemoteStub<INearlinkSlePeripheralObserver> {
public:
    NearlinkSlePeripheralObserverStubCore();
    ~NearlinkSlePeripheralObserverStubCore();

    int32_t OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    ErrCode OnReadRemoteRssiEventInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnPairRequestInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnPairStatusChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnAcbStateChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnConnectionStateChangedInner(MessageParcel &data, MessageParcel &reply);
    ErrCode OnLinkFreqBandChangedInner(MessageParcel &data, MessageParcel &reply);

    std::map<uint32_t,
        ErrCode (NearlinkSlePeripheralObserverStubCore::*)(MessageParcel &data, MessageParcel &reply)>
        memberFuncMap_;
    DISALLOW_COPY_AND_MOVE(NearlinkSlePeripheralObserverStubCore);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_SLE_PERIPHERAL_OBSERVER_STUB_CORE_H
