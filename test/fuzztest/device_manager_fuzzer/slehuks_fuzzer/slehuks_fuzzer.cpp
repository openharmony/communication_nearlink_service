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
#include "SleHuksTool.h"

namespace OHOS {
namespace Nearlink {

void FuzzSleHksToolEncrypt(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleHksTool &hksTool = SleHksTool::GetInstance();
    LinkKey linkKey;
    for (int i = 0; i < 16; i++) linkKey[i] = provider.ConsumeIntegral<uint8_t>();
    EncryptedLinkKey encryptedLinkKey;
    hksTool.SleLinkKeyEncrypt(linkKey, encryptedLinkKey);
    CloudDeviceToken token;
    for (int i = 0; i < 32; i++) token[i] = provider.ConsumeIntegral<uint8_t>();
    EncryptedCloudDeviceToken encryptedToken;
    hksTool.SleCloudDeviceTokenEncrypt(token, encryptedToken);
}

void FuzzSleHksToolDecrypt(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleHksTool &hksTool = SleHksTool::GetInstance();
    EncryptedLinkKey encryptedLinkKey;
    for (int i = 0; i < 128; i++) encryptedLinkKey[i] = provider.ConsumeIntegral<uint8_t>();
    LinkKey linkKey;
    hksTool.SleLinkKeyDecrypt(encryptedLinkKey, linkKey);
    EncryptedCloudDeviceToken encryptedToken;
    for (int i = 0; i < 160; i++) encryptedToken[i] = provider.ConsumeIntegral<uint8_t>();
    CloudDeviceToken token;
    hksTool.SleCloudDeviceTokenDecrypt(encryptedToken, token);
}

void FuzzSleHksToolDeleteKey(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    SleHksTool &hksTool = SleHksTool::GetInstance();
    hksTool.SleDeleteHksKey();
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
    switch (category % 3) {
        case 0: FuzzSleHksToolEncrypt(data, size); break;
        case 1: FuzzSleHksToolDecrypt(data, size); break;
        case 2: FuzzSleHksToolDeleteKey(data, size); break;
        default: break;
    }
    return 0;
}
