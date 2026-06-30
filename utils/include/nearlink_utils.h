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

#ifndef NEARLINK_UTILS_H
#define NEARLINK_UTILS_H

#include <string>
#include "log_util.h"

namespace OHOS {
namespace Nearlink {

constexpr size_t ADDRESS_LENGTH = 17;
constexpr size_t ADDRESS_COLON_INDEX = 2;
constexpr size_t ADDRESS_SEPARATOR_UNIT = 3;
constexpr size_t UUID_LENGTH = 36;  // UUID的总长度

bool IsValidAddress(const std::string &addr);
bool IsValidUuid(const std::string &uuid);
bool ConvertStrToInt(const std::string &strParam, int &intParam);
void ConvertUuidToUpperCase(std::string &uuidStr);
}  // namespace Nearlink
}  // namespace OHOS

#endif // NEARLINK_UTILS_SERVER_H