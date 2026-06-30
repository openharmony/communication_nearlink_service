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

#ifndef SLE_INTERFACE_PROFILE_BAS_H
#define SLE_INTERFACE_PROFILE_BAS_H

#include "SleInterfaceProfile.h"
#include "nearlink_def.h"
#include "raw_address.h"

namespace OHOS {
namespace Nearlink {
class BasObserver {
public:
    virtual ~BasObserver() = default;
    virtual void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) {};
    virtual void OnGetBatteryLevel(const RawAddress &addr, int8_t batteryLevel) {};
};

/**
 * @brief Represents Battery callback.
 *
 * @since 6
 */
class IDeviceBatteryCallback {
public:
    /**
     * @brief A destructor used to delete the <b>IDeviceBatteryCallback</b> instance.
     *
     * @since 6
     */
    virtual ~IDeviceBatteryCallback() = default;

    /**
     * @brief bas update observer.
     *
     * @param device device address.
     * @param batteryLevel device batteryLevel.
     * @since 6
     */
    virtual void OnGetBatteryLevelEvent(const RawAddress &device, int8_t batteryLevel){};

    /**
     * @brief bas update observer.
     *
     * @param device device address.
     * @param batteryLevel device batteryLevel.
     * @since 6
     */
    virtual void OnBatteryLevelChanged(const RawAddress &device, int8_t batteryLevel){};
};

class ProfileBas : public SleInterfaceProfile {
public:
    virtual ~ProfileBas() = default;
    static ProfileBas *GetInstance();
    virtual void RegisterObserver(BasObserver &observer) = 0;
    virtual void DeregisterObserver(BasObserver &observer) = 0;
    virtual void RegisterDeviceObserver(IDeviceBatteryCallback &deviceBatteryObserver) = 0;
    virtual void DeregisterDeviceObserver(IDeviceBatteryCallback &deviceBatteryObserver) = 0;
    virtual void GetDeviceBatteryLevel(const RawAddress &device) = 0;
    virtual void NotifyStateChanged(const RawAddress &device, SleConnectState curState, SleConnectState preState) = 0;
    virtual void NotifyBatteryLevelEvent(const RawAddress &device, int8_t batteryLevel) = 0;
    virtual void NotifyBatteryLevelChanged(const RawAddress &device, int8_t batteryLevel) = 0;
};

} // namespace Sle
} // namespace OHOS
#endif // SLE_INTERFACE_PROFILE_BAS_H