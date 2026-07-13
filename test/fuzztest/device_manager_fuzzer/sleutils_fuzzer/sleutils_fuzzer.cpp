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
#include "device_manager_fuzzer_common.h"
#include "fuzztest_utils.h"
#include "fuzzer/FuzzedDataProvider.h"
#include <filesystem>
#include "SleUtils.h"

namespace OHOS {
namespace Nearlink {

void FuzzSleUtilsConvertHexStringToInt(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    std::string hexStr = BuildHexChars(data, size, 32);
    std::string mixedStr = BuildMixedHexChars(data, size, 32);
    std::string oddStr = BuildHexChars(data, size, 17);
    std::vector<uint8_t> outVec;
    SleUtils::ConvertHexStringToInt(hexStr, outVec);
    SleUtils::ConvertHexStringToInt(mixedStr, outVec);
    SleUtils::ConvertHexStringToInt(oddStr, outVec);
    std::array<uint8_t, 32> outBuf;
    SleUtils::ConvertHexStringToInt(hexStr, outBuf.data(), 32);
    SleUtils::ConvertHexStringToInt(mixedStr, outBuf.data(), 32);
    SleUtils::ConvertHexStringToInt(oddStr, outBuf.data(), 32);
}

void FuzzSleUtilsConvertHexCharToInt(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    std::string hexStr = BuildHexChars(data, size, 32);
    std::string mixedStr = BuildMixedHexChars(data, size, 32);
    std::array<uint8_t, 32> outBuf;
    SleUtils::ConvertHexCharToInt(hexStr.c_str(), outBuf.data(), 32);
    SleUtils::ConvertHexCharToInt(mixedStr.c_str(), outBuf.data(), 32);
}

void FuzzSleUtilsIntToHexString(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    uint8_t val = provider.ConsumeIntegral<uint8_t>();
    SleUtils::IntToHexString(val);
}

void FuzzSleUtilsConvertIntToHexString(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    std::vector<uint8_t> vecData = provider.ConsumeBytes<uint8_t>(32);
    std::array<uint8_t, 32> arrData;
    for (size_t i = 0; i < arrData.size(); i++)
        arrData[i] = provider.ConsumeIntegral<uint8_t>();
    SleUtils::ConvertIntToHexString(vecData);
    SleUtils::ConvertIntToHexString(arrData.data(), arrData.size());
}

void FuzzSleUtilsStringDataToHexString(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    std::string strData = provider.ConsumeRandomLengthString(256);
    SleUtils::StringDataToHexString(strData);
}

void FuzzSleUtilsRand16hex(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    std::vector<uint8_t> randData;
    SleUtils::Rand16hex(randData);
}

void FuzzSleUtilsGetRandomAddress(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    bool isNonResPriAddr = provider.ConsumeBool();
    std::vector<uint8_t> addrData;
    SleUtils::GetRandomAddress(addrData, isNonResPriAddr);
}

}  // namespace Nearlink
}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    std::filesystem::create_directories(OHOS::Nearlink::SLE_CONFIG_DIR);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    using namespace OHOS::Nearlink;
    if (data == nullptr || size == 0) return 0;
    FuzzedDataProvider provider(data, size);
    uint32_t category = provider.ConsumeIntegral<uint32_t>();
    switch (category % 7) {
        case 0: FuzzSleUtilsConvertHexStringToInt(data, size); break;
        case 1: FuzzSleUtilsConvertHexCharToInt(data, size); break;
        case 2: FuzzSleUtilsIntToHexString(data, size); break;
        case 3: FuzzSleUtilsConvertIntToHexString(data, size); break;
        case 4: FuzzSleUtilsStringDataToHexString(data, size); break;
        case 5: FuzzSleUtilsRand16hex(data, size); break;
        case 6: FuzzSleUtilsGetRandomAddress(data, size); break;
        default: break;
    }
    return 0;
}
