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

#ifndef LOG_TAG
#define LOG_TAG "nearlink_taihe_utils"
#endif

#include <algorithm>
#include <regex>

#include "ani_nearlink_utils.h"

namespace OHOS {
namespace Nearlink {
bool IsValidAddress(std::string addr)
{
    const std::regex deviceIdRegex("^[0-9a-fA-F]{2}(:[0-9a-fA-F]{2}){5}$");
    return regex_match(addr, deviceIdRegex);
}

bool CheckDeviceIdParam(std::string &addr)
{
    TAIHE_NEARLINK_RETURN_IF(!IsValidAddress(addr), "Invalid addr", false);
    return true;
}
} // namespace Nearlink
} // namespace OHOS