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

#include "fuzztest_utils.h"

namespace OHOS {
namespace Nearlink {
constexpr static uint8_t BYTES_LEN = 8;
uint32_t GetU32Data(const uint8_t* data)
{
    /*
     * Move the 0th digit to the left by 24 bits, the 1st digit to the left by 16 bits,
     * the 2nd digit to the left by 8 bits, and the 3rd digit not to the left
     */
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3]);
}

uint16_t GetU16IntData(const uint8_t* data)
{
    return (data[0] << BYTES_LEN) | (data[1]);
}

uint64_t GetU64Data(const uint8_t* data)
{
    uint64_t u64Value = 0;
 
    for (int8_t i = 7; i >= 0; i--) {
        u64Value <<= BYTES_LEN;
        u64Value |= data[i];
    }
 
    return u64Value;
}

std::string BuildAddressString(FuzzedDataProvider &provider)
{
    std::string addr("00:00:00:00:00:00");
    char temp[18] = {0};
    int ret = sprintf_s(temp, sizeof(temp), "%02X:%02X:%02X:%02X:%02X:%02X",
        provider.ConsumeIntegral<uint8_t>(), provider.ConsumeIntegral<uint8_t>(), provider.ConsumeIntegral<uint8_t>(),
        provider.ConsumeIntegral<uint8_t>(), provider.ConsumeIntegral<uint8_t>(), provider.ConsumeIntegral<uint8_t>());
    if (ret != -1) {
        addr = std::string(temp);
    }
    return addr;
}

RawAddress BuildRawAddress(FuzzedDataProvider &provider)
{
    std::string addrStr("00:00:00:00:00:00");
    char temp[18] = {0};
    int ret = sprintf_s(temp, sizeof(temp), "%02X:%02X:%02X:%02X:%02X:%02X",
        provider.ConsumeIntegral<uint8_t>(), provider.ConsumeIntegral<uint8_t>(), provider.ConsumeIntegral<uint8_t>(),
        provider.ConsumeIntegral<uint8_t>(), provider.ConsumeIntegral<uint8_t>(), provider.ConsumeIntegral<uint8_t>());
    if (ret != -1) {
        addrStr = std::string(temp);
    }
    RawAddress addr(addrStr);
    return addr;
}

Uuid BuildUuid(FuzzedDataProvider &provider)
{
    std::vector<uint8_t> dataVec(Uuid::UUID128_BYTES_TYPE);
    dataVec = provider.ConsumeBytes<uint8_t>(Uuid::UUID128_BYTES_TYPE);
    uint8_t *ptr = dataVec.data();
    Uuid uuid = Uuid::ConvertFromBytesSle(ptr, dataVec.size());
    return uuid;
}
std::string BuildHexChars(const uint8_t *data, size_t size, uint32_t len)
{
    const char *hexChars = "0123456789ABCDEF";
    std::string result;
    for (uint32_t i = 0; i < len && i < size; i++)
        result += hexChars[data[i] % 16];
    return result;
}

std::string BuildMixedHexChars(const uint8_t *data, size_t size, uint32_t len)
{
    const char *mixedChars = "0123456789ABCDEFabcdef!@%XYZ";
    std::string result;
    for (uint32_t i = 0; i < len && i < size; i++)
        result += mixedChars[data[i] % 26];
    return result;
}

}  // namespace Nearlink
}  // namespace OHOS
