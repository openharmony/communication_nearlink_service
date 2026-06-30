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

#ifndef OHOS_NEARLINK_STANDARD_HOST_OBSERVER_PROXY_H
#define OHOS_NEARLINK_STANDARD_HOST_OBSERVER_PROXY_H

#include "i_nearlink_host_observer.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace Nearlink {
class NearlinkHostObserverProxy : public IRemoteProxy<INearlinkHostObserver> {
public:
    explicit NearlinkHostObserverProxy(const sptr<IRemoteObject> &impl)
        : IRemoteProxy<INearlinkHostObserver>(impl) {}
    ~NearlinkHostObserverProxy() {}

    void OnStateChanged(int32_t transport, int32_t status) override;
    void OnFullStateChanged(int32_t transport, int32_t status) override;
    void OnSwitchStateChanged(int32_t status) override;
    void OnDisableResponse(bool isHalfDisable) override;

    void OnPairConfirmed(const int32_t transport, const NearlinkRawAddress &device, int reqType, int number) override;
    virtual void OnDeviceNameChanged(const std::string &deviceName) override;
    virtual void OnDeviceAddrChanged(const std::string &address) override;

private:
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
    static inline BrokerDelegator<NearlinkHostObserverProxy> delegator_;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_HOST_OBSERVER_PROXY_H
