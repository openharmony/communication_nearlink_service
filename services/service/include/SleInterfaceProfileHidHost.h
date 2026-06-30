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
#ifndef SLE_INTERFACE_PROFILE_HID_HOST_H
#define SLE_INTERFACE_PROFILE_HID_HOST_H

#include "SleInterfaceProfile.h"

namespace OHOS {
namespace Nearlink {
/* hid host observer for framework api */
class HidHostObserver {
public:
    /* Destroy the IHidHostObserver object */
    virtual ~HidHostObserver() = default;
    /**
     * ConnectionState Changed
     * @param  deviceAddress  sle address
     * @param  state          changed status
     */
    virtual void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) {};
};

class ProfileHidHost : public SleInterfaceProfile {
public:
    /**
     * @brief  register observer
     *
     * @param  observer         function pointer
     */
    virtual void RegisterObserver(HidHostObserver &observer) = 0;
    /**
     * @brief  deregister observer
     *
     * @param  observer         function pointer
     */
    virtual void DeregisterObserver(HidHostObserver &observer) = 0;
    /**
     * @brief Get remote AG device list which are in the specified states.
     *
     * @param states List of remote device states.
     * @return Returns the list of devices.
     */
    virtual std::list<RawAddress> GetDevicesByStates(std::vector<int> states) = 0;

    /**
     * @brief Get the connection state of the specified remote device.
     *
     * @param device Remote device object.
     * @return Returns the connection state of the remote device.
     */
    virtual int GetDeviceState(const RawAddress &device) = 0;

    /**
     * @brief Get Hid Host VCUnplug.
     *
     */
    virtual int HidHostVCUnplug(std::string device, uint8_t id, uint16_t size, uint8_t type) = 0;

    /**
     * @brief Hid Host Send Report.
     *
     */
    virtual int HidHostSendReport(std::string device, uint8_t type, uint16_t size, const uint8_t* report) = 0;

    /**
     * @brief Hid Host Send Report.
     *
     */
    virtual int HidHostSendReport(std::string device, uint8_t type, uint16_t size, std::string &report) = 0;

    /**
     * @brief Hid Host Set Report.
     *
     */
    virtual int HidHostSetReport(std::string device, uint8_t type, uint16_t size, const uint8_t* report) = 0;

    /**
     * @brief Hid Host Get Report.
     *
     */
    virtual int HidHostGetReport(std::string device, uint8_t id, uint16_t size, const uint8_t type) = 0;

    /**
     * @brief Hid Host Get Divece Info.
     *
     */
    virtual int GetHidDeviceInfo(const RawAddress &device, int infoType) = 0;
};

} // namespace Sle
} // namespace OHOS
#endif // SLE_INTERFACE_PROFILE_HID_HOST_H