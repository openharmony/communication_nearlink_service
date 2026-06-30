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
#include <charconv>
#include "nearlink_utils.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

bool IsValidAddress(const std::string &addr)
{
    if (addr.empty() || addr.length() != ADDRESS_LENGTH) {
        return false;
    }
    for (size_t i = 0; i < ADDRESS_LENGTH; i++) {
        char c = addr[i];
        switch (i % ADDRESS_SEPARATOR_UNIT) {
            case 0:
            case 1:
                if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                    break;
                }
                return false;
            case ADDRESS_COLON_INDEX:
            default:
                if (c == ':') {
                    break;
                }
                return false;
        }
    }
    return true;
}

bool IsValidUuid(const std::string &uuid)
{
    // 检查长度是否为36个字符（32个十六进制字符 + 4个连字符）
    if (uuid.length() != UUID_LENGTH) {
        return false;
    }

    if (uuid[8] != '-' || uuid[13] != '-' || uuid[18] != '-' || uuid[23] != '-') { // 第8、13、18、23个是连字符的位置
        return false;
    }
    // 验证所有非连字符的字符都是有效的十六进制字符
    for (size_t i = 0; i < UUID_LENGTH; ++i) {
        // 跳过已知的连字符位置
        if (i == 8 || i == 13 || i == 18 || i == 23) { // 第8、13、18、23个是连字符的位置
            continue;
        }
        // 检查是否为有效十六进制字符（0-9, a-f, A-F）
        if (!std::isxdigit(static_cast<unsigned char>(uuid[i]))) {
            return false;
        }
    }
    return true;
}

bool ConvertStrToInt(const std::string &strParam, int &intParam)
{
    auto res = std::from_chars(strParam.data(), strParam.data() + strParam.size(), intParam);
    if (res.ec != std::errc{} || res.ptr != (strParam.data() + strParam.size())) {
        HILOGE("param Converting failed");
        return false;
    }
    return true;
}

void ConvertUuidToUpperCase(std::string &uuidStr)
{
    for (char &c : uuidStr) {
        if (c >= 'a' && c <= 'f') {
            // 将UUID中的字母统一转换为大写
            c = c -'a' + 'A';
        }
    }
    return;
}

}  // namespace Nearlink
}  // namespace OHOS