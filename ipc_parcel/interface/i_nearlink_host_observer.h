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

#ifndef OHOS_NEARLINK_STANDARD_HOST_OBSERVER_INTERFACE_H
#define OHOS_NEARLINK_STANDARD_HOST_OBSERVER_INTERFACE_H

#include "nearlink_service_ipc_interface_code.h"
#include "iremote_broker.h"
#include "nearlink_raw_address.h"

namespace OHOS {
namespace Nearlink {
class INearlinkHostObserver : public OHOS::IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.ipc.INearlinkHostObserver");

    virtual void OnStateChanged(int32_t transport, int32_t status) = 0;
    virtual void OnFullStateChanged(int32_t transport, int32_t status) = 0;
    virtual void OnSwitchStateChanged(int32_t status) = 0;
    virtual void OnDisableResponse(bool isHalfDisable) = 0;
    virtual void OnPairConfirmed(
        const int32_t transport, const NearlinkRawAddress &device, int reqType, int number) = 0;
    virtual void OnDeviceNameChanged(const std::string &deviceName) = 0;
    virtual void OnDeviceAddrChanged(const std::string &address) = 0;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // OHOS_NEARLINK_STANDARD_HOST_OBSERVER_INTERFACE_H
