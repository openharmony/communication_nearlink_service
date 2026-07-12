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
#include "SleRemoteDeviceManager.h"
#include "raw_address.h"
#include "sle_service_data.h"
#include "nearlink_device_model.h"
#include "AdapterDeviceConfig.cpp"

namespace OHOS {
namespace Nearlink {

void FuzzSleRemoteDeviceManager(const uint8_t *data, size_t size)
{
    if (size < 2) return;
    FuzzedDataProvider provider(data, size);
    SleRemoteDeviceManager *mgr = SleRemoteDeviceManager::GetInstance();
    if (mgr == nullptr) return;
    std::string addrStr = BuildAddressString(provider);
    RawAddress rawAddr = BuildRawAddress(provider);
    std::string name = provider.ConsumeRandomLengthString(64);
    std::string alias = provider.ConsumeRandomLengthString(64);
    int appearance = provider.ConsumeIntegral<int>();
    uint16_t lcid = provider.ConsumeIntegral<uint16_t>();
    int acbState = provider.ConsumeIntegral<int>();
    uint8_t role = provider.ConsumeIntegral<uint8_t>();
    uint8_t addrType = provider.ConsumeIntegral<uint8_t>();
    int pairStatus = provider.ConsumeIntegral<int>();
    int connDirect = provider.ConsumeIntegral<int>();
    bool isAvailable = provider.ConsumeBool();
    std::string btAddr = BuildAddressString(provider);
    int pairDirection = provider.ConsumeIntegral<int>();
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    mgr->AddPeripheralDevice(addrStr, peerDevice);
    mgr->GetRemoteDevice(rawAddr);
    mgr->RemovePeripheralDevice(addrStr);
    mgr->GetPairedDevices();
    mgr->GetConnectedDevices();
    mgr->GetConnectingDevices();
    mgr->GetAcbConnectedDevices();
    mgr->GetDeviceName(rawAddr);
    mgr->GetAliasName(rawAddr);
    mgr->SetAliasName(rawAddr, alias);
    mgr->SetName(rawAddr, name);
    mgr->SetAppearance(rawAddr, appearance);
    mgr->GetDeviceAppearance(rawAddr);
    mgr->GetDeviceUuids(rawAddr);
    mgr->IsBondedFromLocal(rawAddr);
    mgr->IsAcbConnected(rawAddr);
    mgr->IsAcbEncrypted(rawAddr);
    mgr->GetLinkRole(rawAddr);
    mgr->GetLcidByAddress(rawAddr);
    mgr->GetAddressByLcid(lcid);
    mgr->GetAcbState(addrStr);
    mgr->SetAcbState(addrStr, acbState);
    mgr->SetLcid(addrStr, lcid);
    uint8_t cryptoAlgo = provider.ConsumeIntegral<uint8_t>();
    uint8_t keyDerivAlgo = provider.ConsumeIntegral<uint8_t>();
    uint8_t integrChk = provider.ConsumeIntegral<uint8_t>();
    mgr->GetPairAlgoInfo(rawAddr, cryptoAlgo, keyDerivAlgo, integrChk);
    mgr->SetPairAlgoInfo(rawAddr, cryptoAlgo, keyDerivAlgo, integrChk);
    std::string groupKey = provider.ConsumeRandomLengthString(64);
    uint64_t giv = provider.ConsumeIntegral<uint64_t>();
    mgr->GetGroupAndGiv(rawAddr, groupKey, giv);
    mgr->SetGroupAndGiv(rawAddr, groupKey, giv);
    mgr->HasConnectedDevice();
    mgr->GetConnDirect(rawAddr);
    mgr->SetConnDirect(rawAddr, connDirect);
    mgr->SetConnDirectActive(rawAddr);
    mgr->GetPairState(rawAddr);
    mgr->SetPairStatus(rawAddr, pairStatus);
    mgr->SetPrePairStatus(rawAddr, pairStatus);
    mgr->SetDeviceIsAvailable(rawAddr, isAvailable);
    mgr->GetConnectedCnt();
    mgr->GetPeerDeviceAddrType(rawAddr);
    mgr->SetBtAddrBySleAddr(addrStr, btAddr);
    std::string outBtAddr;
    mgr->GetBtAddrBySleAddr(addrStr, outBtAddr);
    std::string outSleAddr;
    mgr->GetSleAddrByBtAddr(btAddr, outSleAddr);
    mgr->IsAudioDevice(addrStr);
    mgr->IsServiceSupportedConn(rawAddr);
    DeviceModel model;
    std::string newModelId = provider.ConsumeRandomLengthString(64);
    mgr->SaveDeviceModelInfo(addrStr, model, newModelId);
    DeviceModel outModel;
    std::string outNewModelId;
    mgr->GetDeviceModelInfo(rawAddr, outModel, outNewModelId);
    std::vector<uint8_t> manuBytes = provider.ConsumeBytes<uint8_t>(SLE_MANU_ABILITY_LEN);
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> manuAbility;
    std::copy(manuBytes.begin(), manuBytes.end(), manuAbility.begin());
    mgr->SetManufacturerAbility(rawAddr, manuAbility);
    (void)mgr->GetManufacturerAbility(rawAddr);
    mgr->SetPairDirection(rawAddr, pairDirection);
    mgr->GetPairDirection(rawAddr);
    mgr->GetIsDeviceAvailable(rawAddr);
    mgr->GetIsAudioDeviceFlag(rawAddr);
    mgr->SetIsAudioDeviceFlag(rawAddr);
    mgr->GetIsUserDisconnected(rawAddr);
    mgr->GetCdsmAddrType(rawAddr);
    mgr->GetManufacturerBusinessType(rawAddr);
    mgr->GetManufacturerBusinessTypeExt(rawAddr);
    mgr->GetDirectConnDevices();
    std::vector<RawAddress> deviceList;
    deviceList.push_back(RawAddress());
    mgr->GetEncryptedDevicesCount(deviceList);
    mgr->SetConnectionInfo(rawAddr, lcid, role, addrType);
    std::vector<std::string> cdsmDevList;
    cdsmDevList.push_back(BuildAddressString(provider));
    mgr->SaveCdsmInfo(rawAddr, provider.ConsumeBool(), cdsmDevList);
    std::shared_ptr<SlePeripheralDevice> devPtr = std::make_shared<SlePeripheralDevice>();
    mgr->SaveDeviceModelInfoToConf(rawAddr, devPtr);
    mgr->SavePeerDeviceInfoToConf();
    mgr->RemoveAllPeripheralDevices();
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
    FuzzSleRemoteDeviceManager(data, size);
    return 0;
}
