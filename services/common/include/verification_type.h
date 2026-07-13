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

#ifndef OHOS_NEARLINK_VERIFICATION_TYPE_H
#define OHOS_NEARLINK_VERIFICATION_TYPE_H
#include "cstdint"

namespace OHOS {
namespace Nearlink {

enum class VerificationType : uint8_t {
    CLOUD_PAIR_CHECK = 0,
    SCAN_RSSI_FILTER,
    SCAN_RESULT_FILTER,
    SCAN_MATCH_FILTER,
    SWITCH_CONTROL,
    CONTROLLER_5G,
    CONTROLLER_HIGH_POWER,
    CONTROLLER_COEX,
    CONTROLLER_CHIP_LOG,
    CONTROLLER_BT_ADDR,
    CONTROLLER_TX_POWER,
    HADM_FULL_SCENARIO_CHECK,
    DATATRANSFER_PROXY,
};

} // namespace Nearlink
} // namespace OHOS

#endif // OHOS_NEARLINK_VERIFICATION_TYPE_H