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
 * @file nearlink_sle_controller_def.h
 *
 * @brief Defines nearlink SLE controller related types.
 */

#ifndef NEARLINK_SLE_CONTROLLER_DEF_H
#define NEARLINK_SLE_CONTROLLER_DEF_H

#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief SLE 码率类型
 */
enum class SLEBitRate : uint16_t {
    BITRATE_4600  = 4600,  ///< 4.6 Mbps
    BITRATE_2300  = 2300,  ///< 2.3 Mbps
    BITRATE_1500  = 1500,  ///< 1.5 Mbps
    BITRATE_640   = 640,   ///< 640 Kbps
    BITRATE_320   = 320,   ///< 320 Kbps
    BITRATE_256   = 256,   ///< 256 Kbps
    BITRATE_192   = 192,   ///< 192 Kbps
    BITRATE_64    = 64,    ///< 64 Kbps
    BITRATE_32    = 32,    ///< 32 Kbps
};

/**
 * @brief SLE 占空比类型
 */
enum class SledutyCycle : uint8_t {
    DUTY_CYCLE_100P = 0,  ///< 100% 占空比
    DUTY_CYCLE_50P = 1,   ///< 50% 占空比
    DUTY_CYCLE_20P = 2,   ///< 20% 占空比
};

/**
 * @brief Sle coexist mode
 */
enum SleCoexMode : int {
    SLE_HID_COEX_MODE_ENABLE = 0,
    SLE_HID_COEX_MODE_DISABLE,
    SLE_COEX_MODE_BUTT,
};
 
enum SleCoexModeStatus : int {
    STARTING = 0,
    STARTED,
    STOPPING,
    STOPPED,
};
 
struct SleHidCoexDevice {
    SleHidCoexDevice(const std::string &addr, uint16_t coexInterval, uint16_t pendingInterval) :
        addr(addr), coexInterval(coexInterval), pendingInterval(pendingInterval) {}
    ~SleHidCoexDevice() {}
 
    std::string addr = {};
    uint16_t coexInterval = 0;
    uint16_t pendingInterval = 0;
};
 
struct SleHidCoexModeParam {
    SleCoexModeStatus state = SleCoexModeStatus::STOPPED;
    std::vector<SleHidCoexDevice> deviceList = {};
};

} // namespace Nearlink
} // namespace OHOS

#endif // NEARLINK_SLE_CONTROLLER_DEF_H
