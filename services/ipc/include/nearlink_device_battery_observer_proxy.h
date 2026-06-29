/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_DEVICE_BATTERY_OBSERVER_PROXY_H
#define NEARLINK_DEVICE_BATTERY_OBSERVER_PROXY_H

#include "i_nearlink_device_battery_observer.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace Nearlink {
class NearlinkDeviceBatteryObserverProxy : public IRemoteProxy<INearlinkDeviceBatteryObserver> {
public:
    explicit NearlinkDeviceBatteryObserverProxy(const sptr<IRemoteObject> &impl);
    ~NearlinkDeviceBatteryObserverProxy() override;

    
    void OnGetBatteryLevelEvent(const NearlinkRawAddress &device, int32_t batteryLevel) override;
    void OnBatteryLevelChanged(const NearlinkRawAddress &device, int32_t batteryLevel) override;

private:
    static inline BrokerDelegator<NearlinkDeviceBatteryObserverProxy> delegator_;
    ErrCode InnerTransact(uint32_t code, MessageOption &flags, MessageParcel &data, MessageParcel &reply);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_SLE_PERIPHERAL_OBSERVER_PROXY_H