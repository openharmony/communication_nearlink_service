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

#ifndef SLE_REMOTE_DEVICE_ADAPTER_H
#define SLE_REMOTE_DEVICE_ADAPTER_H

#include <string>
#include <vector>
#include <memory>
#include <array>

#include "sle_service_data.h"
#include "nearlink_device_model.h"
#include "nearlink_safe_map.h"
#include "nearlink_safe_list.h"
#include "nearlink_def.h"
#include "sle_uuid.h"
#include "SleHuksTool.h"
#include "cm_def.h"
#include "nlstk_sm_api.h"

namespace OHOS {
namespace Nearlink {

class SleRemoteDeviceAdapter {
public:

    static SleRemoteDeviceAdapter *GetInstance(void);

    void AddPeripheralDevice(const std::string &address, std::shared_ptr<SlePeripheralDevice>& peerDevice);
    void RemovePeripheralDevice(const std::string &address);
    void RemoveAllPeripheralDevices();
    std::shared_ptr<const SlePeripheralDevice> GetRemoteDevice(const RawAddress &device);

    std::vector<RawAddress> GetPairedDevices();
    std::vector<RawAddress> GetConnectedDevices();
    std::string GetDeviceName(const RawAddress &device);
    int GetPairState(const RawAddress &device);
    std::string GetAliasName(const RawAddress &device);
    bool SetAliasName(const RawAddress &device, const std::string &name);
    bool SetName(const RawAddress &device, const std::string &name);
    bool SetAppearance(const RawAddress &device, const int appearance);
    int GetDeviceAppearance(const RawAddress &device);
    std::vector<Uuid> GetDeviceUuids(const RawAddress &device);
    bool IsVendorDevice(const RawAddress &memberAddr);
    bool IsAudioDevice(const std::string &address);
    bool IsBondedFromLocal(const RawAddress &device);
    bool IsAcbConnected(const RawAddress &device);
    bool IsAcbEncrypted(const RawAddress &device);
    uint8_t GetLinkRole(const RawAddress &device);
    uint16_t GetLcidByAddress(const RawAddress &device);
    std::string GetAddressByLcid(uint16_t lcid);
    int GetAcbState(const std::string &address);
    int GetManufacturerBusinessType(const RawAddress &device);
    bool SetAcbState(const std::string &address, int connectState);
    bool SetLcid(const std::string &address, uint16_t lcid);
    int GetPairStatus(const RawAddress &device);
    bool SetPairStatus(const RawAddress &device, int pairStatus);
    bool SetPrePairStatus(const RawAddress &device, int pairStatus);
    void SetDeviceIsAvailable(const RawAddress &device, bool isAvailable);
    bool SetCdsmAddrType(const RawAddress &device, int addrType);
    bool GetPairAlgoInfo(
        const RawAddress &addr, uint8_t &cryptoAlgo, uint8_t &keyDerivAlgo, uint8_t &integrChkInd);
    bool SetPairAlgoInfo(
        const RawAddress &addr, uint8_t cryptoAlgo, uint8_t keyDerivAlgo, uint8_t integrChkInd);
    bool GetGroupAndGiv(const RawAddress &addr, std::string &encryptGroupKeyStr, uint64_t &giv);
    bool SetGroupAndGiv(const RawAddress &addr, std::string encryptGroupKeyStr, uint64_t giv);
    bool HasConnectedDevice();
    int GetConnDirect(const RawAddress &device);
    void SetConnDirect(const RawAddress &device, int connDirect);
    void SetConnDirectActive(const RawAddress &device);
    void UpdateDeviceManufacturerAbility(
        const RawAddress &rawAddr, std::array<uint8_t, SLE_MANU_ABILITY_LEN> &deviceManuAbility);
    bool GetManufacturerAbility(const RawAddress &rawAddr, uint8_t ability);
    int GetNotPairNoneCnt(const RawAddress &device);
    int GetPairedCnt(const RawAddress &device);
    int GetConnectedCnt();
    void GetBgList(NearlinkSafeList<RawAddress> &bgList, const std::vector<std::string> &reconnectList);
    void GetDirectConnList(NearlinkSafeList<RawAddress> &bgList);

    void SetReconnDeviceParam();

    RawAddress GetRealAddress(const RawAddress &reportAddr);
    uint8_t GetPeerDeviceAddrType(const RawAddress &device);
    void FindDeviceModelInfoInCache(const RawAddress &reportAddr, DeviceModel &model, std::string &newModelId);
    void SaveDeviceModelInfo(const std::string &address, const DeviceModel &model, const std::string &newModelId);
    bool SavePeerDeviceInfoToConf();
    void SavePeerDevices2Smp();
    std::vector<NLSTK_SmRecoverKeyParam_S> CollectPairedDevicesForSmp();
    void SavePairDirect(int connDirect, const RawAddress &device);
    bool SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr);
    bool GetBtAddrBySleAddrTask(const std::string &sleAddr, std::string &btAddr);
    bool IsServiceSupportedConn(const RawAddress &device);
    void SetAudioDeviceFlag(const RawAddress &device);
    void SaveDeviceManufacturerAbility(const RawAddress &rawAddr);
    void UpdateDefaultRole(const RawAddress &device);
    bool ConnectionCompleteHelper(const RawAddress &device, uint16_t lcid, uint8_t role, uint8_t addrType);
    void CdsmSaveData(const RawAddress &memberAddr);
    bool IsCdsmMemberPair(const RawAddress &device, const RawAddress &report);

    void CdsmAddOtherRecord(const RawAddress &srcAddr, const RawAddress &otherAddr);
    void HandleCdsmMemberFirstPairing(const RawAddress &member);
    void GetDeviceTypeInfo(const RawAddress &device, SLE_Addr_S &addrInfo, int &devType);
    // Internal helper methods
    void SetPeerDeviceTypeToController(const RawAddress &device);
    void RemovePeerDeviceTypeToController(const RawAddress &device);

    void AddBgConnDevice(const std::string &address);
    bool GetCdsmOtherAddr(const RawAddress &member, RawAddress &other);
    void SendBgConnList(NearlinkSafeList<RawAddress> &bgList);
    void SendDirectConnList(NearlinkSafeList<RawAddress> &directList);
    bool DisconnectAcbAction(const RawAddress &device, uint8_t discReason);
    void CancelAllConnection();
    void RemoveAllPairsProcess(std::vector<RawAddress> &removeDevices);
    bool GetSleAddrByBtAddrTask(const std::string &btAddr, std::string &sleAddr);
    void SleAddPeerList(const RawAddress &device);
    void ClearPeerDeviceGroupId();

private:
    void UpdateSlePeripheralDeviceInfo(const RawAddress &device, std::shared_ptr<SlePeripheralDevice> value);
    void UpdateSlePeripheralDeviceHiLinkInfo(const RawAddress &reportAddr,
        std::shared_ptr<SlePeripheralDevice> value) const;
    void UpdateManufacturerAbilityFromAdvData(const RawAddress &reportAddr,
        std::shared_ptr<SlePeripheralDevice> value) const;
    void SetOtherDeviceInfo(std::shared_ptr<const SlePeripheralDevice> &srcDev,
        std::shared_ptr<SlePeripheralDevice> &otherDev) const;
    std::string GetReconnDevice() const;
    bool IsNeedToIgnore(std::string reconnDevAddr) const;
    RawAddress GetRealAddressInner(const RawAddress &reportAddr);
    uint8_t GetPeerDeviceAddrTypeInner(const RawAddress &device);
    int GetDeviceAppearanceInner(const RawAddress &device);
    void AddBgConnDeviceInner(const std::string &address);
    bool GetCdsmOtherAddrInner(const RawAddress &member, RawAddress &other);
    void SendBgConnListInner(NearlinkSafeList<RawAddress> &bgList);
    void SendDirectConnListInner(NearlinkSafeList<RawAddress> &directList);
    void SavePeerDeviceInfoToConfInner();
    void SaveDeviceManufacturerAbilityInner(const RawAddress &rawAddr);
    void SavePairDirectInner(int connDirect, const RawAddress &device);
    void CdsmAddOtherRecordTask(const RawAddress &srcAddr, const RawAddress &otherAddr);
    void CdsmSaveDataTask(const RawAddress &memberAddr);
    void HandleCdsmMemberFirstPairingTask(const RawAddress &member);
    bool SleLinkKeyDecrypt(const std::string &linkKeyStr, LinkKey &sleLinkkey);
    uint32_t GetDeviceTypeFromAppearance(const RawAddress &device, int deviceAppearance);
    uint32_t GetAudioDeviceGroupId(const RawAddress &device);
    uint32_t GetVendorAudioDeviceGroupId(const RawAddress &device);

    SleRemoteDeviceAdapter();
    ~SleRemoteDeviceAdapter();
};

} // namespace Nearlink
} // namespace OHOS

#endif // SLE_DEVICE_ADAPTER_H