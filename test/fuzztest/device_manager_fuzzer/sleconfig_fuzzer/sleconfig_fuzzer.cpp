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
#include "SleConfig.h"
#include "AdapterDeviceConfig.cpp"

namespace OHOS {
namespace Nearlink {

void FuzzSleConfigValidate(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    (void)BuildHexChars(data, size, 32);
    (void)BuildMixedHexChars(data, size, 32);
}

void FuzzSleConfigLocalInfo(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleConfig &config = SleConfig::GetInstance();
    std::string addr = BuildAddressString(provider);
    std::string name = provider.ConsumeRandomLengthString(64);
    int addrType = provider.ConsumeIntegral<int>();
    config.SetLocalAddress(addr);
    config.GetLocalAddress();
    config.SetLocalName(name);
    config.GetLocalName();
    config.GetIoCapability();
    config.SetSleLocalAddrType(addrType);
    config.GetFileFlag();
}

void FuzzSleConfigPeerInfo(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleConfig &config = SleConfig::GetInstance();
    std::string addr = BuildAddressString(provider);
    std::string name = provider.ConsumeRandomLengthString(64);
    std::string alias = provider.ConsumeRandomLengthString(64);
    std::string randomAddr = BuildAddressString(provider);
    int appearance = provider.ConsumeIntegral<int>();
    uint8_t addrType = provider.ConsumeIntegral<uint8_t>();
    int ioCap = provider.ConsumeIntegral<int>();
    bool isAudioDevice = provider.ConsumeBool();
    bool isUserDisconnected = provider.ConsumeBool();
    config.SetPeerName(addr, name);
    config.GetPeerName(addr);
    config.SetPeerAlias(addr, alias);
    config.GetPeerAlias(addr);
    config.SetPeerRandomAddress(addr, randomAddr);
    config.GetPeerRandomAddress(addr);
    config.SetPeerAppearance(addr, appearance);
    config.GetPeerAppearance(addr);
    config.SetPeerAddressType(addr, addrType);
    config.GetPeerAddressType(addr);
    config.SetPeerDeviceIoCapability(addr, ioCap);
    config.GetPeerDeviceIoCapability(addr);
    config.SetIsAudioDeviceFlag(addr, isAudioDevice);
    config.GetIsAudioDeviceFlag(addr);
    config.SetUserDisconnectedFlag(addr, isUserDisconnected);
    config.GetUserDisconnectedFlag(addr);
    config.RemovePairedDevice(addr);
    config.RemoveAllPairedDevices();
    config.GetPairedAddrList();
}

void FuzzSleConfigCryptoInfo(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleConfig &config = SleConfig::GetInstance();
    std::string addr = BuildAddressString(provider);
    std::string linkKey = BuildHexChars(data, size, 32);
    int cryptoAlgo = provider.ConsumeIntegral<int>();
    int keyDerivAlgo = provider.ConsumeIntegral<int>();
    int integrChk = provider.ConsumeIntegral<int>();
    std::string groupKey = provider.ConsumeRandomLengthString(64);
    uint64_t giv = provider.ConsumeIntegral<uint64_t>();
    int pairDirect = provider.ConsumeIntegral<int>();
    config.SetLinkKey(addr, linkKey);
    config.GetLinkKey(addr);
    char charBuf[64] = {0};
    config.GetLinkKeyChar(addr, charBuf, 64);
    config.SetCryptoAlgo(addr, cryptoAlgo);
    config.GetCryptoAlgo(addr);
    config.SetKeyDerivAlgo(addr, keyDerivAlgo);
    config.GetKeyDerivAlgo(addr);
    config.SetIntegrChk(addr, integrChk);
    config.GetIntegrChk(addr);
    config.SetGroupkey(addr, groupKey);
    config.GetGroupKey(addr);
    config.SetGiv(addr, giv);
    config.GetGiv(addr);
    config.SetPairDirect(addr, pairDirect);
    config.GetPairDirect(addr);
}

void FuzzSleConfigCdsmInfo(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleConfig &config = SleConfig::GetInstance();
    std::string addr = BuildAddressString(provider);
    int cdsmAddrType = provider.ConsumeIntegral<int>();
    int isPrivate = provider.ConsumeIntegral<int>();
    std::vector<std::string> addrList;
    uint32_t addrCount = provider.ConsumeIntegralInRange<uint32_t>(0, 5);
    for (uint32_t i = 0; i < addrCount; i++)
        addrList.push_back(BuildAddressString(provider));
    config.SetCdsmAddrType(addr, cdsmAddrType);
    config.GetCdsmAddrType(addr);
    config.SetCdsmMemberList(addr, addrList);
    std::vector<std::string> resultAddrList;
    config.GetCdsmMemberList(addr, resultAddrList);
    config.SetCdsmIsPrivateDevice(addr, isPrivate);
    config.GetCdsmIsPrivateDevice(addr);
    config.GetAllCdsmReportList();
    config.RemoveCdsmGroup(addr);
}

void FuzzSleConfigCloudDevice(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleConfig &config = SleConfig::GetInstance();
    std::string addr = BuildAddressString(provider);
    std::string btAddr = BuildAddressString(provider);
    std::string deviceName = provider.ConsumeRandomLengthString(64);
    std::string reportAddr = BuildAddressString(provider);
    std::string model = provider.ConsumeRandomLengthString(64);
    int32_t cloudState = provider.ConsumeIntegral<int32_t>();
    config.SetCloudDeviceBtAddr(addr, btAddr);
    config.GetCloudDeviceBtAddr(addr);
    config.SetCloudDeviceName(addr, deviceName);
    config.GetCloudDeviceName(addr);
    config.SetCloudDeviceReportAddr(addr, reportAddr);
    config.GetCloudDeviceReportAddr(addr);
    config.SetCloudDeviceModel(addr, model);
    config.GetCloudDeviceModel(addr);
    config.SetCloudDeviceSubModelId(addr, model);
    config.GetCloudDeviceSubModelId(addr);
    config.SetCloudDeviceIconId(addr, model);
    config.GetCloudDeviceIconId(addr);
    config.SetCloudDeviceState(addr, cloudState);
    config.GetCloudDeviceState(addr);
    std::vector<uint8_t> token = provider.ConsumeBytes<uint8_t>(32);
    config.SetCloudDeviceToken(addr, token);
    std::vector<uint8_t> outToken(32);
    config.GetCloudDeviceToken(addr, outToken);
    std::vector<std::string> membersList;
    uint32_t mc = provider.ConsumeIntegralInRange<uint32_t>(0, 3);
    for (uint32_t i = 0; i < mc; i++) membersList.push_back(BuildAddressString(provider));
    config.SetCloudDeviceMembersAddrList(addr, membersList);
    std::vector<std::string> resultMembers;
    config.GetCloudDeviceMembersAddrList(addr, resultMembers);
    config.GetCloudDeviceAddrList();
    config.RemoveSpecificCloudDevice(addr);
    config.RemoveAllCloudDevice();
}

void FuzzSleConfigDeviceModel(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleConfig &config = SleConfig::GetInstance();
    std::string addr = BuildAddressString(provider);
    std::string modelId = provider.ConsumeRandomLengthString(64);
    std::string newModelId = provider.ConsumeRandomLengthString(64);
    std::string subModelId = provider.ConsumeRandomLengthString(64);
    std::string iconId = provider.ConsumeRandomLengthString(64);
    std::string devType = provider.ConsumeRandomLengthString(64);
    std::string manuAbility = BuildHexChars(data, size, 32);
    bool isAudioDevice = provider.ConsumeBool();
    bool availableControl = provider.ConsumeBool();
    int sleBusiness = provider.ConsumeIntegral<int>();
    config.SetDeviceModelId(addr, modelId);
    config.GetDeviceModelId(addr);
    config.SetDeviceNewModelId(addr, newModelId);
    config.GetDeviceNewModelId(addr);
    config.SetDeviceSubModelId(addr, subModelId);
    config.GetDeviceSubModelId(addr);
    config.SetDeviceIconId(addr, iconId);
    config.GetDeviceIconId(addr);
    config.SetDeviceDevType(addr, devType);
    config.GetDeviceDevType(addr);
    config.SetManufacturerAbility(addr, manuAbility);
    config.GetManufacturerAbility(addr);
    config.SetIsAudioDeviceFlag(addr, isAudioDevice);
    config.GetIsAudioDeviceFlag(addr);
    config.SetAvailableControl(addr, availableControl);
    config.GetAvailableControl(addr);
    config.SetSleBusiness(addr, sleBusiness);
    config.GetSleBusiness(addr);
}

void FuzzSleConfigVolume(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleConfig &config = SleConfig::GetInstance();
    std::string addr = BuildAddressString(provider);
    int volume = provider.ConsumeIntegral<int>();
    int defaultVolume = provider.ConsumeIntegral<int>();
    int wearState = provider.ConsumeIntegral<int>();
    int autoConnect = provider.ConsumeIntegral<int>();
    std::string btAddr = BuildAddressString(provider);
    config.SetDeviceMediaVolume(addr, volume);
    config.GetDeviceMediaVolume(addr, defaultVolume);
    config.SetDeviceCallVolume(addr, volume);
    config.GetDeviceCallVolume(addr, defaultVolume);
    config.SetConfigWearDetectionState(addr, wearState);
    config.GetConfigWearDetectionState(addr);
    config.SetAutoConnectSwitch(addr, autoConnect);
    config.GetAutoConnectSwitch(addr);
    config.SetBtAddrBySleAddr(addr, btAddr);
    config.GetBtAddrBySleAddr(addr);
}

void FuzzSleConfigAscDevice(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleConfig &config = SleConfig::GetInstance();
    std::string activeDevice = BuildAddressString(provider);
    std::string connectedDevice = BuildAddressString(provider);
    std::string addressList = BuildAddressString(provider) + ";" + BuildAddressString(provider);
    config.SetLastASCActiveDevice(activeDevice);
    config.GetLastASCActiveDevice();
    config.RemoveLastASCActiveDevice();
    config.SetLastASCConnectedDevice(connectedDevice);
    config.GetLastASCConnectedDevice();
    config.RemoveLastASCConnectedDevice();
    config.SetReconnectDeviceAddressList(addressList);
    config.GetReconnectDeviceAddressList();
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
    switch (category % 9) {
        case 0: FuzzSleConfigValidate(data, size); break;
        case 1: FuzzSleConfigLocalInfo(data, size); break;
        case 2: FuzzSleConfigPeerInfo(data, size); break;
        case 3: FuzzSleConfigCryptoInfo(data, size); break;
        case 4: FuzzSleConfigCdsmInfo(data, size); break;
        case 5: FuzzSleConfigCloudDevice(data, size); break;
        case 6: FuzzSleConfigDeviceModel(data, size); break;
        case 7: FuzzSleConfigVolume(data, size); break;
        case 8: FuzzSleConfigAscDevice(data, size); break;
        default: break;
    }
    return 0;
}
