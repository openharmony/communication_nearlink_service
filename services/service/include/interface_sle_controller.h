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

#ifndef INTERFACE_SLE_CONTROLLER_H
#define INTERFACE_SLE_CONTROLLER_H

#include "nearlink_def.h"

#include <cstdint>
#include <string>

namespace OHOS {
namespace Nearlink {

/**
 * @brief Represents interface SLE controller.
 *        Service 层接口定义，供 Server 层调用
 */
class InterfaceSleController {
public:
    virtual ~InterfaceSleController() = default;

    /**
     * @brief Get singleton instance
     */
    static InterfaceSleController &GetInstance();

    /**
     * @brief Set SLE coexistence parameters
     * @param maxBitRate Maximum bit rate (e.g., 4600, 2300, 1500, etc.)
     * @param dutyCycle Duty cycle (0: 100%, 1: 50%, 2: 20%)
     * @return true if success, false otherwise
     */
    virtual bool SetSleCoexParam(uint16_t maxBitRate, uint8_t dutyCycle) = 0;

    /**
     * @brief Update SLE connection interval
     * @param device Remote device address
     * @param intervalType Connection interval type (0-9)
     * @return true if success, false otherwise
     */
    virtual bool UpdateConnectInterval(const std::string &device, int32_t intervalType) = 0;

    /**
     * @brief The function to set sle coexist mode.
     * @param mode coexist mode.
     *         SLE_HID_COEX_MODE_ENABLE, // enable HID coexist mode, restrain HID interval to low speed
     *         SLE_HID_COEX_MODE_DISABLE, // disable HID coexist mode
     * @param deviceList device list in which sle param should be adjust during coexist mode
     * @param paramList params of device list
     * @return Returns the status code for this function called.
     */
    virtual bool SetSleCoexMode(int32_t mode, const std::vector<std::string> &deviceList,
        const std::vector<ConnectionInterval> &paramList) = 0;
};

} // namespace Nearlink
} // namespace OHOS

#endif // INTERFACE_SLE_CONTROLLER_H
