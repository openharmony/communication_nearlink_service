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

namespace OHOS {
namespace Nearlink {

/**
 * @brief NearlinkSystemConfig 的 mock 实现
 */

// 覆盖 IsIcceProfileSupported 方法
bool NearlinkSystemConfig::IsIcceProfileSupported()
{
    return true;
}

// 覆盖 IsAudioSupported 方法
bool NearlinkSystemConfig::IsAudioSupported()
{
    return true;
}

// 覆盖 IsDualRecordSupported 方法
bool NearlinkSystemConfig::IsDualRecordSupported()
{
    return false;
}

} // namespace Nearlink
} // namespace OHOS
