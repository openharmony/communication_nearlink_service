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
#ifndef LIS_DEFINES_H
#define LIS_DEFINES_H

namespace OHOS {
namespace Nearlink {
// default invalid
constexpr int LIS_INVALID_EVT = 0;
// Service start/stop
constexpr int LIS_SERVICE_STARTUP_EVT = 1;
constexpr int LIS_SERVICE_SHUTDOWN_EVT = 2;
// 设备类型指示
enum DeviceTypeInd : uint8_t {
    LOCAL_INFO_IND_MANAGER = 1,
    LOCAL_INFO_IND_ANCHOR = 1 << 1,
    LOCAL_INFO_IND_FLAG = 1 << 2,
};
// 5 means LOCAL_INFO_IND_MANAGER and LOCAL_INFO_IND_FLAG
constexpr char LIS_DEVICE_TYPE_IND[] = "5";
// 位置管理服务中设备类型指示属性
constexpr uint16_t LIS_UUID_SSAP_DEVICE_TYPE_IND = 0x1040;
} // namespace Sle
} // namespace OHOS
#endif // LIS_DEFINES_H