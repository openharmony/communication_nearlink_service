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
#include "SleUtils.h"

#include <charconv>
#include <ctime>
#include <random>
#include <sstream>
#include <cctype>
#include "log.h"
#include "SleDefs.h"
#include "securec.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr uint8_t SIZE_STRING_TO_UINT8 = 2;
}
std::string SleUtils::IntToHexString(uint8_t value)
{
    std::stringstream strStream;
    char token[hexFormatSize] = {0};
    (void)sprintf_s(token, hexFormatSize, "%02X", value);
    strStream << token;
    return strStream.str();
}

std::string SleUtils::ConvertIntToHexString(const std::vector<uint8_t> &key)
{
    std::string strs;
    for (auto iter = key.begin(); iter != key.end(); iter++) {
        uint8_t temp = *iter;
        strs += IntToHexString(temp);
    }
    return strs;
}

std::string SleUtils::ConvertIntToHexString(uint8_t* value, int length)
{
    std::string strs;
    for (int i = 0; i < length; i++) {
        uint8_t temp = value[i];
        strs += IntToHexString(temp);
    }
    return strs;
}

void SleUtils::ConvertHexStringToInt(const std::string &str, std::vector<uint8_t> &key)
{
    uint8_t k = 0;
    for (size_t i = 0; i < str.size(); i = i + SIZE_STRING_TO_UINT8) {
        std::from_chars_result res = std::from_chars(str.data() + i, str.data() + i + SIZE_STRING_TO_UINT8, k, hexName);
        if (res.ec != std::errc{} || res.ptr != str.data() + i + SIZE_STRING_TO_UINT8) {
            HILOGE("ConvertHexStringToInt failed, string:%{public}s", str.c_str());
            return;
        }
        key.push_back(k);
    }
}

bool SleUtils::ConvertHexStringToInt(const std::string &str, uint8_t* value, int length)
{
    NL_CHECK_RETURN_RET(length >= 0 && str.size() / SIZE_STRING_TO_UINT8 == static_cast<uint32_t>(length) && value,
        false, "ConvertHexStringToInt failed, invalid params.");
    uint8_t k = 0;
    int idx = 0;
    for (size_t i = 0; i < str.size(); i = i + SIZE_STRING_TO_UINT8) {
        std::from_chars_result res = std::from_chars(str.data() + i, str.data() + i + SIZE_STRING_TO_UINT8, k, hexName);
        if (res.ec != std::errc{} || res.ptr != str.data() + i + SIZE_STRING_TO_UINT8) {
            HILOGE("ConvertHexStringToInt failed, string:%{public}s", str.c_str());
            return false;
        }
        value[idx] = k;
        idx++;
    }
    return true;
}

bool SleUtils::ConvertHexCharToInt(const char *str, uint8_t* value, int valueLength)
{
    NL_CHECK_RETURN_RET(
        str && valueLength >= 0 && strlen(str) / SIZE_STRING_TO_UINT8 == static_cast<uint32_t>(valueLength) &&
        value, false, "ConvertHexStringToInt failed, invalid params.");
    uint8_t k = 0;
    int idx = 0;
    for (size_t i = 0; i < strlen(str); i += SIZE_STRING_TO_UINT8) {
        std::from_chars_result res = std::from_chars(str + i, str + i + SIZE_STRING_TO_UINT8, k, hexName);
        if (res.ec != std::errc{} || res.ptr != str + i + SIZE_STRING_TO_UINT8) {
            HILOGE("ConvertHexStringToInt failed");
            return false;
        }
        value[idx] = k;
        idx++;
    }
    return true;
}

void SleUtils::Rand16hex(std::vector<uint8_t> &key)
{
    uint8_t result = 0;
    uint8_t n3 = 0;

    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<int> random_value(0, (((unsigned int)(-1)) >> 1));

    for (int i = 0; i < SLE_IRK_HEX_ELN; i++) {
        if (n3 == 0) {
            result = random_value(re);
            n3 = SLE_IRK_RAND_HEX_LEN;
        }
        key.push_back(result & SLE_IRK_RAND_ELN);
        result >>= SLE_IRK_RAND_LEFT_SHIFT;
        --n3;
    }
}

void SleUtils::GetRandomAddress(std::vector<uint8_t> &addr, bool isNonResPriAddr)
{
    std::random_device rd;
    std::default_random_engine re(rd());
    std::uniform_int_distribution<uint8_t> random_value(0, SLE_STATIC_PRI_ADDR);

    for (int i = 0; i < SLE_ADDR_LEN - 1; ++i) {
        addr.push_back(random_value(re) & 0xff);
    }

    if (isNonResPriAddr) {
        addr.push_back(random_value(re) & SLE_NON_RES_PRI_ADDR);
    } else {
        addr.push_back((random_value(re) & 0xff) | SLE_STATIC_PRI_ADDR);
    }

    return;
}

std::string SleUtils::StringDataToHexString(const std::string &str)
{
    const std::string hex = "0123456789ABCDEF";
    const uint8_t sizeFour = 4;
    std::stringstream ss;

    for (size_t i = 0; i < str.size(); ++i) {
        uint8_t n = static_cast<uint8_t>(str[i]);
        ss << hex[n >> sizeFour] << hex[n & 0xF];
    }
    return ss.str();
}
}  // namespace Nearlink
}  // namespace OHOS