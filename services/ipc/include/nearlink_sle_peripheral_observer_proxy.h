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

#ifndef NEARLINK_SLE_PERIPHERAL_OBSERVER_PROXY_H
#define NEARLINK_SLE_PERIPHERAL_OBSERVER_PROXY_H

#include "i_nearlink_sle_peripheral_observer.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace Nearlink {
class NearlinkSlePeripheralObserverProxy : public IRemoteProxy<INearlinkSlePeripheralObserver> {
public:
    explicit NearlinkSlePeripheralObserverProxy(const sptr<IRemoteObject> &impl);
    ~NearlinkSlePeripheralObserverProxy() override;

    void OnReadRemoteRssiEvent(const NearlinkRawAddress &device, int rssi, int status) override;
    void OnPairingRequest(const NearlinkRawAddress &device, const std::string &passkey, int type) override;
    void OnPairStatusChanged(const NearlinkRawAddress &device, int preState, int state, int reason) override;
    void OnAcbStateChanged(const NearlinkRawAddress &device, int state, int reason) override;
    void OnConnectionStateChanged(const NearlinkRawAddress &device, int preState, int state, int reason) override;
    void OnLinkFreqBandChanged(const NearlinkRawAddress &device, int32_t freqBand) override;

private:
    static inline BrokerDelegator<NearlinkSlePeripheralObserverProxy> delegator_;
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_SLE_PERIPHERAL_OBSERVER_PROXY_H