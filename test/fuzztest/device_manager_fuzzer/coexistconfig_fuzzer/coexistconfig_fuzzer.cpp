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
#include "AdapterDeviceConfig.cpp"
#include "SleCoexistManager.h"
#include "SleCoexistData.h"
#include "ManufacturerAbilityLoader.h"
#include "raw_address.h"

namespace OHOS {
namespace Nearlink {

void FuzzSleCoexistManager(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleCoexistManager *mgr = SleCoexistManager::GetInstance();
    if (mgr == nullptr) return;
    CoexistConnInfo connInfo;
    connInfo.lcid = provider.ConsumeIntegral<uint16_t>();
    connInfo.interval = provider.ConsumeIntegral<uint16_t>();
    connInfo.latency = provider.ConsumeIntegral<uint16_t>();
    connInfo.timeout = provider.ConsumeIntegral<uint16_t>();
    connInfo.addr = {};
    for (int i = 0; i < 6; i++) connInfo.addr.addr[i] = provider.ConsumeIntegral<uint8_t>();
    connInfo.addr.type = provider.ConsumeIntegral<uint8_t>();
    mgr->UpdateConnectionInfo(connInfo);
    mgr->OnConnectionRemoved(connInfo.lcid);
    uint16_t timeout = 0;
    uint16_t maxLatency = 0;
    SLE_Addr_S testAddrS = {};
    for (int i = 0; i < 6; i++) testAddrS.addr[i] = provider.ConsumeIntegral<uint8_t>();
    testAddrS.type = provider.ConsumeIntegral<uint8_t>();
    mgr->GetConnectionParam(testAddrS, timeout, maxLatency);
    mgr->IterateConnInfo([](const CoexistConnInfo &info) {(void)info;});
    mgr->HasMultipleConnections();
    mgr->ClearAll();
}

void FuzzAdapterDeviceConfig(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    IAdapterDeviceConfig *config = AdapterDeviceConfig::GetInstance();
    if (config == nullptr) return;
    std::string section = BuildAddressString(provider);
    std::string subSection = BuildAddressString(provider);
    std::string property = BuildAddressString(provider);
    std::string strVal = provider.ConsumeRandomLengthString(64);
    int intVal = provider.ConsumeIntegral<int>();
    bool boolVal = provider.ConsumeBool();
    config->CreateFile();
    config->Load();
    config->Reload();
    config->Save();
    config->Fsync();
    int outInt = 0;
    std::string outStr;
    bool outBool = false;
    char charBuf[256] = {0};
    config->GetValue(section, subSection, property, outInt);
    config->GetValue(section, subSection, property, outStr);
    config->GetValue(section, subSection, property, charBuf, 255);
    config->GetValue(section, subSection, property, outBool);
    config->GetValue(section, property, outInt);
    config->GetValue(section, property, outStr);
    config->GetValue(section, property, outBool);
    config->SetValue(section, subSection, property, intVal);
    config->SetValue(section, subSection, property, strVal);
    config->SetValue(section, subSection, property, boolVal);
    config->SetValue(section, property, intVal);
    config->SetValue(section, property, strVal);
    std::vector<std::string> subSections;
    config->GetSubSections(section, subSections);
    config->RemoveSection(section, subSection);
    config->RemoveSection(section);
}

void FuzzManufacturerAbilityLoader(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    ManufacturerAbilityLoader &loader = ManufacturerAbilityLoader::GetInstance();
    loader.Load();
    loader.Unload();
    (void)loader.GetLocalAbility();
    std::vector<uint8_t> filterBytes = provider.ConsumeBytes<uint8_t>(SLE_MANU_ABILITY_LEN);
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> filterAbility;
    std::copy(filterBytes.begin(), filterBytes.end(), filterAbility.begin());
    loader.FilterAbility(filterAbility);
    uint8_t index = provider.ConsumeIntegral<uint8_t>();
    loader.CheckAbility(index);
    loader.SetLocalAbility(index, provider.ConsumeBool());
    RawAddress rawAddr = BuildRawAddress(provider);
    loader.GetAbilityValue(rawAddr, index);
    std::string abilityName = provider.ConsumeRandomLengthString(64);
    loader.GetAbilityIndex(abilityName);
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
    FuzzedDataProvider provider(data, size);
    uint32_t category = provider.ConsumeIntegral<uint32_t>();
    switch (category % 3) {
        case 0: FuzzSleCoexistManager(data, size); break;
        case 1: FuzzAdapterDeviceConfig(data, size); break;
        case 2: FuzzManufacturerAbilityLoader(data, size); break;
        default: break;
    }
    return 0;
}
