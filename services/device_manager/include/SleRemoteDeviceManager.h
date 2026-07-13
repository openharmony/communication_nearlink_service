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

#ifndef SLE_REMOTE_DEVICE_MANAGER_H
#define SLE_REMOTE_DEVICE_MANAGER_H

#include <string>
#include <vector>
#include <array>
#include <memory>
#include "sle_service_data.h"
#include "nearlink_device_model.h"
#include "nearlink_safe_map.h"
#include "nearlink_def.h"
#include "SleConfig.h"

namespace OHOS {
namespace Nearlink {
class SleRemoteDeviceManager {
public:
    static SleRemoteDeviceManager *GetInstance(void);

    void AddPeripheralDevice(const std::string &address, std::shared_ptr<SlePeripheralDevice>& peerDevice);
    void RemovePeripheralDevice(const std::string &address);
    void RemoveAllPeripheralDevices();
    std::shared_ptr<SlePeripheralDevice> GetRemoteDevice(const RawAddress &device);

    std::vector<RawAddress> GetPairedDevices();
    std::vector<RawAddress> GetConnectedDevices();
    std::vector<RawAddress> GetConnectingDevices();
    std::vector<RawAddress> GetAcbConnectedDevices();
    std::string GetDeviceName(const RawAddress &device);
    std::string GetAliasName(const RawAddress &device);
    bool SetAliasName(const RawAddress &device, const std::string &name);
    bool SetName(const RawAddress &device, const std::string &name);
    bool SetAppearance(const RawAddress &device, const int appearance);
    int GetDeviceAppearance(const RawAddress &device);
    std::vector<Uuid> GetDeviceUuids(const RawAddress &device);
    bool IsBondedFromLocal(const RawAddress &device);
    bool IsAcbConnected(const RawAddress &device);
    bool IsAcbEncrypted(const RawAddress &device);
    uint8_t GetLinkRole(const RawAddress &device);
    uint16_t GetLcidByAddress(const RawAddress &device);
    std::string GetAddressByLcid(uint16_t lcid);
    int GetAcbState(const std::string &address);
    int GetManufacturerBusinessType(const RawAddress &device);
    bool SetAcbState(const std::string &address, int connectState);
    bool SetAcbDisConnReason(const std::string &address, int reason);
    int GetAcbDisConnReason(const std::string &address);
    bool SetLcid(const std::string &address, uint16_t lcid);
    int GetPairState(const RawAddress &device);
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
    int GetConnectedCnt();
    uint8_t GetPeerDeviceAddrType(const RawAddress &device);
    bool SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr);
    bool GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr);
    bool GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr);
    bool IsAudioDevice(const std::string &address);
    bool IsServiceSupportedConn(const RawAddress &device);
    void SaveDeviceModelInfo(const std::string &address, const DeviceModel &model, const std::string &newModelId);
    bool GetDeviceModelInfo(const RawAddress &device, DeviceModel &model, std::string &newModelId);
    void SetManufacturerAbility(const RawAddress &device,
        const std::array<uint8_t, SLE_MANU_ABILITY_LEN> &manufacturerAbility);
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> GetManufacturerAbility(const RawAddress &device);
    void SetPairDirection(const RawAddress &device, int pairDirection);
    int GetPairDirection(const RawAddress &device);
    bool GetIsDeviceAvailable(const RawAddress &device);
    bool GetIsAudioDeviceFlag(const RawAddress &device);
    void SetIsAudioDeviceFlag(const RawAddress &device);
    bool GetIsUserDisconnected(const RawAddress &device);
    int GetCdsmAddrType(const RawAddress &device);
    int GetManufacturerBusinessTypeExt(const RawAddress &device);
    std::vector<RawAddress> GetDirectConnDevices();
    int GetEncryptedDevicesCount(const std::vector<RawAddress> &devices);
    bool SetConnectionInfo(const RawAddress &device, uint16_t lcid, uint8_t role, uint8_t addrType);
    void SaveCdsmInfo(const RawAddress &reportAddr, bool isPrivate, const std::vector<std::string> &cdsmDevList);
    bool SaveDeviceModelInfoToConf(const RawAddress &device, const std::shared_ptr<SlePeripheralDevice> &value);
    bool SavePeerDeviceInfoToConf();

private:
    SleRemoteDeviceManager();
    ~SleRemoteDeviceManager();

    NearlinkSafeMap<std::string, std::shared_ptr<SlePeripheralDevice>> peerConnDeviceSafeList_ {};
};

} // namespace Nearlink
} // namespace OHOS

#endif // SLE_DEVICE_MANAGER_H