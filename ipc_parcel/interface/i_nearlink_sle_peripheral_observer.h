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

#ifndef I_NEARLINK_SLE_PERIPHERAL_OBSERVER_H
#define I_NEARLINK_SLE_PERIPHERAL_OBSERVER_H

#include "nearlink_raw_address.h"
#include "nearlink_service_ipc_interface_code.h"
#include "iremote_broker.h"

namespace OHOS {
namespace Nearlink {
class INearlinkSlePeripheralObserver : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkSlePeripheralObserver");

    virtual void OnReadRemoteRssiEvent(const NearlinkRawAddress &device, int rssi, int status) = 0;
    virtual void OnPairingRequest(const NearlinkRawAddress &device, const std::string &passkey, int type) = 0;
    virtual void OnPairStatusChanged(const NearlinkRawAddress &device, int preState, int state, int reason) = 0;
    virtual void OnAcbStateChanged(const NearlinkRawAddress &device, int state, int reason) = 0;
    virtual void OnConnectionStateChanged(const NearlinkRawAddress &device, int preState, int state, int reason) = 0;
    virtual void OnLinkFreqBandChanged(const NearlinkRawAddress &device, int32_t freqBand) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // I_NEARLINK_SLE_PERIPHERAL_OBSERVER_H