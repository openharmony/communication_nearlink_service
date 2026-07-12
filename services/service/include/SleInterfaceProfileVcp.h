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

#ifndef SLE_INTERFACE_PROFILE_VCP_CLIENT_H
#define SLE_INTERFACE_PROFILE_VCP_CLIENT_H

#include "SleInterfaceProfile.h"

namespace OHOS {
namespace Nearlink {
class VcpClientObserver {
public:
    virtual ~VcpClientObserver() = default;
    /**
     * ConnectionState Changed
     * @param  deviceAddress  sle address
     * @param  state          new state
     * @param  oldState       old state
     */
    virtual void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) {};
};

class ProfileVcp : public SleInterfaceProfile {
public:
    /**
     * @brief  register observer
     *
     * @param  observer         function pointer
     */
    virtual void RegisterObserver(VcpClientObserver &observer) = 0;

    /**
     * @brief  deregister observer
     *
     * @param  observer         function pointer
     */
    virtual void DeregisterObserver(VcpClientObserver &observer) = 0;

    /**
     * @brief system volume changed, audio notify volume changed.
     * @param[in] addr The device.
     * @param[in] volumeLevel The device volume.
     * @param[in] streamType The stream Type.
     */
    virtual void SetDeviceAbsoluteVolume(const RawAddress &device, int32_t volumeLevel, uint8_t streamType) = 0;

    /**
     * @brief audio get device media volume.
     *
     * @param[in] device The remote device.
     * @return error code
     * @since 6
     */
    virtual int GetDeviceMediaVolume(const RawAddress &device) = 0;

    /**
     * @brief audio get device call volume.
     *
     * @param[in] device The remote device.
     * @return error code
     * @since 6
     */
    virtual int GetDeviceCallVolume(const RawAddress &device) = 0;
};
} // namespace Nearlink
} // namespace OHOS
#endif // SLE_INTERFACE_PROFILE_VCP_CLIENT_H