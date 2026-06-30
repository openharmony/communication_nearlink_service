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
#ifndef VCP_DEFINES_H
#define VCP_DEFINES_H

namespace OHOS {
namespace Nearlink {
/* 协议栈返回的音量变化信息数据结构，对应 NLSTK_McpVolumeStatus_S */
class VolumeChangedInfo {
public:
    uint8_t volume;         ///< 音量
    uint8_t mute;           ///< 静音
    uint8_t additionalInfo; ///< 附加信息
};

enum VcpResult {
    VCP_SUCCESS = 0,
    VCP_ERROR = 1,
};

constexpr uint8_t VCP_MAX_VOL = 255;
constexpr uint8_t VCP_DEFAULT_VOLUME_LEVEL = 7;
constexpr uint8_t VCP_DEFAULT_VOLUME_DIVISOR = 2;

/* 音量值转换 */
#define GET_REAL_VOLUME(value) ((value) & 0xFF)

} // namespace Nearlink
} // namespace OHOS
#endif // VCP_DEFINES_H
