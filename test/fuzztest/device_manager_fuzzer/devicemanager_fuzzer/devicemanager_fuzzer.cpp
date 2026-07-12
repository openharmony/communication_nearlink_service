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
#include "AdapterDeviceConfig.h"
#include "nearlink_device_manager.h"
#include "raw_address.h"
#include "AdapterDeviceConfig.cpp"

namespace OHOS {
namespace Nearlink {

void FuzzNearlinkDeviceManager(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    NearlinkDeviceManager *dm = NearlinkDeviceManager::GetInstance();
    if (dm == nullptr) return;
    RawAddress realAddr = BuildRawAddress(provider);
    RawAddress randomAddr = BuildRawAddress(provider);
    std::string realAddrStr = BuildAddressString(provider);
    bool isRetention = provider.ConsumeBool();
    bool isUseRealAddr = provider.ConsumeBool();
    int status = provider.ConsumeIntegral<int>();
    dm->AddDeviceInfo(realAddr, randomAddr, isRetention);
    RawAddress randomOut;
    dm->GetDeviceRandomAddr(realAddr, randomOut);
    RawAddress realOut;
    dm->GetDeviceRealAddr(realAddr, realOut);
    std::string realStr;
    dm->GetDeviceRealAddr(realAddrStr, realStr);
    dm->GetDeviceRealAddr(realAddrStr, realStr, provider.ConsumeIntegral<uint32_t>());
    dm->ConvertToRandomAddress(realAddr, randomOut, isRetention);
    dm->ConvertToRandomAddress(isUseRealAddr, realAddr, randomOut, isRetention);
    dm->UpdateRandomAddressMap(realAddr, status);
    dm->RecoverRetainedDeviceInfo();
    dm->ClearDevicesInfo();
}

}  // namespace Nearlink
}  // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    std::filesystem::create_directories(OHOS::Nearlink::SLE_CONFIG_DIR);
    OHOS::Nearlink::AdapterDeviceConfig *cfg = static_cast<OHOS::Nearlink::AdapterDeviceConfig *>(OHOS::Nearlink::AdapterDeviceConfig::GetInstance());
    cfg->pimpl->filePath_ = OHOS::Nearlink::SLE_CONFIG_DIR + cfg->pimpl->fileName_;
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    using namespace OHOS::Nearlink;
    if (data == nullptr || size == 0) return 0;
    FuzzNearlinkDeviceManager(data, size);
    return 0;
}
