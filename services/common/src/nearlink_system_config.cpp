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

#include "nearlink_system_config.h"
#include "log.h"
#include "parameters.h"

namespace OHOS {
namespace Nearlink {

// 使用 __attribute__((weak)) 标记，允许第三方提供自定义实现
bool __attribute__((weak)) NearlinkSystemConfig::IsIcceProfileSupported()
{
    bool isSupported = OHOS::system::GetBoolParameter("const.nearlink.support.icce", false);
    HILOGI("[ICCE_CONFIG] Check ICCE profile support: %{public}d", isSupported);
    return isSupported;
}

bool __attribute__((weak)) NearlinkSystemConfig::IsAudioSupported()
{
    bool isSupported = OHOS::system::GetIntParameter("const.nearlink.audio", 0) != 0;
    HILOGI("[AUDIO_CONFIG] Check audio support: %{public}d", isSupported);
    return isSupported;
}

bool __attribute__((weak)) NearlinkSystemConfig::IsDualRecordSupported()
{
    bool isSupported = OHOS::system::GetBoolParameter("const.nearlink.audio.binauralrecord", false);
    HILOGI("[DUAL_RECORD_CONFIG] Check dual record support: %{public}d", isSupported);
    return isSupported;
}

} // namespace Nearlink
} // namespace OHOS
