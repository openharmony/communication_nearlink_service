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

#include "SleRemoteDeviceAdapter.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

SleRemoteDeviceAdapter::SleRemoteDeviceAdapter()
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] enter");
}

SleRemoteDeviceAdapter::~SleRemoteDeviceAdapter()
{}

SleRemoteDeviceAdapter* SleRemoteDeviceAdapter::GetInstance(void)
{
    static SleRemoteDeviceAdapter instance;
    return &instance;
}

void SleRemoteDeviceAdapter::AddPeripheralDevice(const std::string &address, std::shared_ptr<SlePeripheralDevice>& peerDevice)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] AddPeripheralDevice addr:%{public}s", address.c_str());
}

void SleRemoteDeviceAdapter::RemovePeripheralDevice(const std::string &address)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] RemovePeripheralDevice addr:%{public}s", address.c_str());
}

void SleRemoteDeviceAdapter::RemoveAllPeripheralDevices()
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] RemoveAllPeripheralDevices");
}

std::shared_ptr<const SlePeripheralDevice> SleRemoteDeviceAdapter::GetRemoteDevice(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetRemoteDevice addr:%{public}s", device.GetAddress().c_str());
    return nullptr;
}

std::vector<RawAddress> SleRemoteDeviceAdapter::GetPairedDevices()
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetPairedDevices");
    return {};
}

std::vector<RawAddress> SleRemoteDeviceAdapter::GetConnectedDevices()
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetConnectedDevices");
    return {};
}

std::string SleRemoteDeviceAdapter::GetDeviceName(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetDeviceName addr:%{public}s", device.GetAddress().c_str());
    return "";
}

int SleRemoteDeviceAdapter::GetPairState(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetPairState addr:%{public}s", device.GetAddress().c_str());
    return 0;
}

std::string SleRemoteDeviceAdapter::GetAliasName(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetAliasName addr:%{public}s", device.GetAddress().c_str());
    return "";
}

bool SleRemoteDeviceAdapter::SetAliasName(const RawAddress &device, const std::string &name)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetAliasName addr:%{public}s", device.GetAddress().c_str());
    return true;
}

bool SleRemoteDeviceAdapter::SetName(const RawAddress &device, const std::string &name)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetName addr:%{public}s", device.GetAddress().c_str());
    return true;
}

bool SleRemoteDeviceAdapter::SetAppearance(const RawAddress &device, const int appearance)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetAppearance addr:%{public}s", device.GetAddress().c_str());
    return true;
}

int SleRemoteDeviceAdapter::GetDeviceAppearance(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetDeviceAppearance addr:%{public}s", device.GetAddress().c_str());
    return 0;
}

std::vector<Uuid> SleRemoteDeviceAdapter::GetDeviceUuids(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetDeviceUuids addr:%{public}s", device.GetAddress().c_str());
    return {};
}

bool SleRemoteDeviceAdapter::IsVendorDevice(const RawAddress &memberAddr)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] IsVendorDevice addr:%{public}s", memberAddr.GetAddress().c_str());
    return false;
}

bool SleRemoteDeviceAdapter::IsAudioDevice(const std::string &address)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] IsAudioDevice addr:%{public}s", address.c_str());
    return false;
}

bool SleRemoteDeviceAdapter::IsBondedFromLocal(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] IsBondedFromLocal addr:%{public}s", device.GetAddress().c_str());
    return false;
}

bool SleRemoteDeviceAdapter::IsAcbConnected(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] IsAcbConnected addr:%{public}s", device.GetAddress().c_str());
    return false;
}

bool SleRemoteDeviceAdapter::IsAcbEncrypted(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] IsAcbEncrypted addr:%{public}s", device.GetAddress().c_str());
    return false;
}

uint8_t SleRemoteDeviceAdapter::GetLinkRole(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetLinkRole addr:%{public}s", device.GetAddress().c_str());
    return 0;
}

uint16_t SleRemoteDeviceAdapter::GetLcidByAddress(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetLcidByAddress addr:%{public}s", device.GetAddress().c_str());
    return 0;
}

std::string SleRemoteDeviceAdapter::GetAddressByLcid(uint16_t lcid)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetAddressByLcid lcid:%{public}d", lcid);
    return "";
}

int SleRemoteDeviceAdapter::GetAcbState(const std::string &address)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetAcbState addr:%{public}s", address.c_str());
    return 0;
}

int SleRemoteDeviceAdapter::GetManufacturerBusinessType(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetManufacturerBusinessType addr:%{public}s", device.GetAddress().c_str());
    return 0;
}

bool SleRemoteDeviceAdapter::SetAcbState(const std::string &address, int connectState)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetAcbState addr:%{public}s state:%{public}d", address.c_str(), connectState);
    return true;
}

bool SleRemoteDeviceAdapter::SetLcid(const std::string &address, uint16_t lcid)
{
    return true;
}

int SleRemoteDeviceAdapter::GetPairStatus(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetPairStatus addr:%{public}s", device.GetAddress().c_str());
    return 0;
}

bool SleRemoteDeviceAdapter::SetPairStatus(const RawAddress &device, int pairStatus)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetPairStatus addr:%{public}s status:%{public}d",
        device.GetAddress().c_str(), pairStatus);
    return true;
}

bool SleRemoteDeviceAdapter::SetPrePairStatus(const RawAddress &device, int pairStatus)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetPrePairStatus addr:%{public}s status:%{public}d",
        device.GetAddress().c_str(), pairStatus);
    return true;
}

void SleRemoteDeviceAdapter::SetDeviceIsAvailable(const RawAddress &device, bool isAvailable)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetDeviceIsAvailable addr:%{public}s available:%{public}d",
        device.GetAddress().c_str(), isAvailable);
}

bool SleRemoteDeviceAdapter::SetCdsmAddrType(const RawAddress &device, int addrType)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetCdsmAddrType addr:%{public}s type:%{public}d",
        device.GetAddress().c_str(), addrType);
    return true;
}

bool SleRemoteDeviceAdapter::GetPairAlgoInfo(
    const RawAddress &addr, uint8_t &cryptoAlgo, uint8_t &keyDerivAlgo, uint8_t &integrChkInd)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetPairAlgoInfo addr:%{public}s", addr.GetAddress().c_str());
    cryptoAlgo = 0;
    keyDerivAlgo = 0;
    integrChkInd = 0;
    return true;
}

bool SleRemoteDeviceAdapter::SetPairAlgoInfo(
    const RawAddress &addr, uint8_t cryptoAlgo, uint8_t keyDerivAlgo, uint8_t integrChkInd)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetPairAlgoInfo addr:%{public}s", addr.GetAddress().c_str());
    return true;
}

bool SleRemoteDeviceAdapter::GetGroupAndGiv(const RawAddress &addr, std::string &encryptGroupKeyStr, uint64_t &giv)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetGroupAndGiv addr:%{public}s", addr.GetAddress().c_str());
    encryptGroupKeyStr = "";
    giv = 0;
    return true;
}

bool SleRemoteDeviceAdapter::SetGroupAndGiv(const RawAddress &addr, std::string encryptGroupKeyStr, uint64_t giv)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetGroupAndGiv addr:%{public}s", addr.GetAddress().c_str());
    return true;
}

bool SleRemoteDeviceAdapter::HasConnectedDevice()
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] HasConnectedDevice");
    return false;
}

int SleRemoteDeviceAdapter::GetConnDirect(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetConnDirect addr:%{public}s", device.GetAddress().c_str());
    return 0;
}

void SleRemoteDeviceAdapter::SetConnDirect(const RawAddress &device, int connDirect)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetConnDirect addr:%{public}s direct:%{public}d",
        device.GetAddress().c_str(), connDirect);
}

void SleRemoteDeviceAdapter::SetConnDirectActive(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetConnDirectActive addr:%{public}s", device.GetAddress().c_str());
}

void SleRemoteDeviceAdapter::UpdateDeviceManufacturerAbility(
    const RawAddress &rawAddr, std::array<uint8_t, SLE_MANU_ABILITY_LEN> &deviceManuAbility)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] UpdateDeviceManufacturerAbility addr:%{public}s", rawAddr.GetAddress().c_str());
}

bool SleRemoteDeviceAdapter::GetManufacturerAbility(const RawAddress &rawAddr, uint8_t ability)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetManufacturerAbility addr:%{public}s", rawAddr.GetAddress().c_str());
    return false;
}

int SleRemoteDeviceAdapter::GetNotPairNoneCnt(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetNotPairNoneCnt addr:%{public}s", device.GetAddress().c_str());
    return 0;
}

int SleRemoteDeviceAdapter::GetPairedCnt(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetPairedCnt addr:%{public}s", device.GetAddress().c_str());
    return 0;
}

int SleRemoteDeviceAdapter::GetConnectedCnt()
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetConnectedCnt");
    return 0;
}

void SleRemoteDeviceAdapter::GetBgList(NearlinkSafeList<RawAddress> &bgList,
                                       const std::vector<std::string> &reconnectList)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetBgList");
}

void SleRemoteDeviceAdapter::GetDirectConnList(NearlinkSafeList<RawAddress> &bgList)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetDirectConnList");
}

void SleRemoteDeviceAdapter::SetReconnDeviceParam()
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetReconnDeviceParam");
}

RawAddress SleRemoteDeviceAdapter::GetRealAddress(const RawAddress &reportAddr)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetRealAddress addr:%{public}s", reportAddr.GetAddress().c_str());
    return reportAddr;
}

uint8_t SleRemoteDeviceAdapter::GetPeerDeviceAddrType(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetPeerDeviceAddrType addr:%{public}s", device.GetAddress().c_str());
    return 0;
}

void SleRemoteDeviceAdapter::FindDeviceModelInfoInCache(const RawAddress &reportAddr,
    DeviceModel &model, std::string &newModelId)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] FindDeviceModelInfoInCache addr:%{public}s",
        reportAddr.GetAddress().c_str());
}

void SleRemoteDeviceAdapter::SaveDeviceModelInfo(const std::string &address, const DeviceModel &model,
    const std::string &newModelId)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SaveDeviceModelInfo addr:%{public}s", address.c_str());
}

bool SleRemoteDeviceAdapter::SavePeerDeviceInfoToConf()
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SavePeerDeviceInfoToConf");
    return true;
}

void SleRemoteDeviceAdapter::SavePeerDevices2Smp()
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SavePeerDevices2Smp");
}

std::vector<NLSTK_SmRecoverKeyParam_S> SleRemoteDeviceAdapter::CollectPairedDevicesForSmp()
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] CollectPairedDevicesForSmp");
    return std::vector<NLSTK_SmRecoverKeyParam_S>();
}

void SleRemoteDeviceAdapter::SavePairDirect(int connDirect, const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SavePairDirect direct:%{public}d addr:%{public}s",
        connDirect, device.GetAddress().c_str());
}

bool SleRemoteDeviceAdapter::SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetBtAddrBySleAddr sleAddr:%{public}s btAddr:%{public}s",
        sleAddr.c_str(), btAddr.c_str());
    return true;
}

bool SleRemoteDeviceAdapter::GetBtAddrBySleAddrTask(const std::string &sleAddr, std::string &btAddr)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetBtAddrBySleAddr sleAddr:%{public}s", sleAddr.c_str());
    btAddr = "";
    return true;
}

bool SleRemoteDeviceAdapter::IsServiceSupportedConn(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] IsServiceSupportedConn addr:%{public}s", device.GetAddress().c_str());
    return false;
}

void SleRemoteDeviceAdapter::SetAudioDeviceFlag(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetAudioDeviceFlag addr:%{public}s", device.GetAddress().c_str());
}

void SleRemoteDeviceAdapter::SaveDeviceManufacturerAbility(const RawAddress &rawAddr)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SaveDeviceManufacturerAbility addr:%{public}s", rawAddr.GetAddress().c_str());
}

void SleRemoteDeviceAdapter::UpdateDefaultRole(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] UpdateDefaultRole addr:%{public}s", device.GetAddress().c_str());
}

bool SleRemoteDeviceAdapter::ConnectionCompleteHelper(const RawAddress &device, uint16_t lcid, uint8_t role, uint8_t addrType)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] ConnectionCompleteHelper addr:%{public}s lcid:%{public}u",
        device.GetAddress().c_str(), lcid);
    return true;
}

void SleRemoteDeviceAdapter::CdsmSaveData(const RawAddress &memberAddr)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] CdsmSaveData addr:%{public}s", memberAddr.GetAddress().c_str());
}

bool SleRemoteDeviceAdapter::IsCdsmMemberPair(const RawAddress &device, const RawAddress &report)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] IsCdsmMemberPair device:%{public}s report:%{public}s",
        device.GetAddress().c_str(), report.GetAddress().c_str());
    return false;
}

void SleRemoteDeviceAdapter::CdsmAddOtherRecord(const RawAddress &srcAddr, const RawAddress &otherAddr)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] CdsmAddOtherRecord src:%{public}s other:%{public}s",
        srcAddr.GetAddress().c_str(), otherAddr.GetAddress().c_str());
}

void SleRemoteDeviceAdapter::HandleCdsmMemberFirstPairing(const RawAddress &member)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] HandleCdsmMemberFirstPairing addr:%{public}s", member.GetAddress().c_str());
}

void SleRemoteDeviceAdapter::GetDeviceTypeInfo(const RawAddress &device, SLE_Addr_S &addrInfo, int &devType)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetDeviceTypeInfo addr:%{public}s", device.GetAddress().c_str());
    devType = 0;
}

void SleRemoteDeviceAdapter::SetPeerDeviceTypeToController(const RawAddress &device)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] SetPeerDeviceTypeToController addr:%{public}s", device.GetAddress().c_str());
}

void SleRemoteDeviceAdapter::AddBgConnDevice(const std::string &address)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] AddBgConnDevice addr:%{public}s", address.c_str());
}

bool SleRemoteDeviceAdapter::DisconnectAcbAction(const RawAddress &device, uint8_t discReason)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] DisconnectAcbAction addr:%{public}s reason:%{public}u",
        device.GetAddress().c_str(), discReason);
    return true;
}

void SleRemoteDeviceAdapter::CancelAllConnection()
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] CancelAllConnection");
}

void SleRemoteDeviceAdapter::RemoveAllPairsProcess(std::vector<RawAddress> &removeDevices)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] RemoveAllPairsProcess count:%{public}zu", removeDevices.size());
}

bool SleRemoteDeviceAdapter::GetSleAddrByBtAddrTask(const std::string &btAddr, std::string &sleAddr)
{
    HILOGI("[SleRemoteDeviceAdapter Mocker] GetSleAddrByBtAddrTask btAddr:%{public}s", btAddr.c_str());
    sleAddr = "";
    return true;
}

} // namespace Nearlink
} // namespace OHOS
