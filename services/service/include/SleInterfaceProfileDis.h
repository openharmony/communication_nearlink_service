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

#ifndef SLE_INTERFACE_PROFILE_DIS_H
#define SLE_INTERFACE_PROFILE_DIS_H

#include "SleInterfaceProfile.h"
#include "nearlink_device_information.h"

namespace OHOS {
namespace Nearlink {
class DisObserver {
public:
    virtual ~DisObserver() = default;
    virtual void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) {};
};

class ProfileDis : public SleInterfaceProfile {
public:
    virtual void RegisterObserver(DisObserver &observer) = 0;
    virtual void DeregisterObserver(DisObserver &observer) = 0;

    virtual int GetDeviceVendorId(const RawAddress &device) = 0;
    virtual int GetDeviceProductId(const RawAddress &device) = 0;
    virtual int GetDeviceVersion(const RawAddress &device) = 0;
    virtual void NotifyStateChanged(const RawAddress &device, SleConnectState curState, SleConnectState preState) = 0;

    /**
     * @brief Get remote device information.
     *
     * @param device Remote device address.
     * @return Returns remote device information.
     * @since 24
     */
    virtual DeviceInformation GetDeviceInformation(const RawAddress &device) const = 0;
};

} // namespace Sle
} // namespace OHOS
#endif // SLE_INTERFACE_PROFILE_DIS_H