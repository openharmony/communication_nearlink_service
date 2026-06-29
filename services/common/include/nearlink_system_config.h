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

#ifndef OHOS_NEARLINK_SYSTEM_CONFIG_H
#define OHOS_NEARLINK_SYSTEM_CONFIG_H

namespace OHOS {
namespace Nearlink {

/**
 * @brief NearLink 系统配置检查器
 *
 * 集中管理所有系统配置参数的检查逻辑
 * 使用 weak 函数机制，支持扩展覆盖实现
 */
class NearlinkSystemConfig {
public:
    /**
     * @brief 检查 ICCE Profile 是否支持
     *
     * @return true 支持 ICCE Profile
     * @return false 不支持 ICCE Profile
     */
    static bool IsIcceProfileSupported();

    /**
     * @brief 检查 Audio 功能是否支持
     *
     * @return true 支持 Audio 功能
     * @return false 不支持 Audio 功能
     */
    static bool IsAudioSupported();

    /**
     * @brief 检查双耳录音功能是否支持
     *
     * @return true 支持双耳录音功能
     * @return false 不支持双耳录音功能
     */
    static bool IsDualRecordSupported();
};

} // namespace Nearlink
} // namespace OHOS

#endif // OHOS_NEARLINK_SYSTEM_CONFIG_H
