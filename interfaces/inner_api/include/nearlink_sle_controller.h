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

/**
 * @addtogroup Nearlink
 *
 * @brief Defines nearlink SLE controller, including standard control functions.
 */

/**
 * @file nearlink_sle_controller.h
 *
 * @brief Framework nearlink SLE controller interface.
 */

#ifndef NEARLINK_SLE_CONTROLLER_H
#define NEARLINK_SLE_CONTROLLER_H

#include "nearlink_def.h"
#include "nearlink_types.h"
#include "nearlink_errorcode.h"
#include "nearlink_sle_controller_def.h"

namespace OHOS {
namespace Nearlink {
/**
 * @brief Represents framework SLE controller.
 */
class NEARLINK_API NearlinkSleController {
public:
    /**
     * @brief Get default SLE controller instance.
     *
     * @return Returns the singleton instance.
     */
    static NearlinkSleController &GetInstance();

    /**
     * @brief Setting the maximum bit rate and duty cycle of nearlink.
     *
     * @param maxBitRate Maximum output bit rate of the audio encoder
     * @param dutyCycle Audio service duty cycle
     *
     * @return Returns the status code for this function called.
     */
    NlErrCode SetSleCoexParam(SLEBitRate maxBitRate, SledutyCycle dutyCycle);

    /**
     * @brief The function to update connect interval.
     * @param[in] device, nearlink data intervalType of acb link.
     *         HIGH_SPEED_INTERVAL_4_5---0, //  4.5ms
     *         HIGH_SPEED_INTERVAL_4_875---0x1, // 4.875ms
     *         MID_SPEED_INTERVAL_11_25---0x2, //  11.25ms
     *         MID_SPEED_INTERVAL_15---0x3, //  15ms
     *         MID_SPEED_INTERVAL_50---0x4, // 50ms
     *         LOW_SPEED_INTERVAL_100---0x5, // 100ms
     *         LOW_SPEED_INTERVAL_150---0x6, // 150ms
     *         LOW_SPEED_INTERVAL_200---0x7, // 200ms
     *         LOW_SPEED_INTERVAL_300---0x8, // 300ms
     *         LOW_SPEED_INTERVAL_500---0x9 // 500ms
     * @return Returns the status code for this function called.
     */
    NlErrCode UpdateConnectInterval(const std::string &device, ConnectionInterval intervalType) const;

    /**
     * @brief The function to set sle coexist mode.
     * @param mode coexist mode.
     *         SLE_HID_COEX_MODE_ENABLE, // enable HID coexist mode, restrain HID interval to low speed
     *         SLE_HID_COEX_MODE_DISABLE, // disable HID coexist mode
     * @param deviceList device list in which sle param should be adjust during coexist mode
     * @param paramList params of device list
     * @return Returns the status code for this function called.
     */
    NlErrCode SetSleCoexMode(SleCoexMode mode, const std::vector<std::string> &deviceList,
        const std::vector<ConnectionInterval> &paramList);

private:
    NearlinkSleController();
    ~NearlinkSleController();
    NEARLINK_DISALLOW_COPY_AND_ASSIGN(NearlinkSleController);
    NEARLINK_DECLARE_IMPL();
};

} // namespace Nearlink
} // namespace OHOS

#endif  // NEARLINK_SLE_CONTROLLER_H
