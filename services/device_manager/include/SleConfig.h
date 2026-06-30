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
#ifndef SLE_CONFIG_H
#define SLE_CONFIG_H

#include <cstring>
#include <vector>

#include "AdapterDeviceConfig.h"

/*
 * @brief The Nearlink subsystem.
 */
namespace OHOS {
namespace Nearlink {
/**
 * @brief SLE config.
 */
class SleConfig {
public:
    static SleConfig &GetInstance();

    bool LoadConfigInfo() const;
    bool Save() const;

    void EncryptLinkKey() const;
    void EncryptCloudDeviceToken() const;
    int GetFileFlag() const;
    void SetFileFlag() const;
    std::string GetLocalAddress() const;
    std::string GetLocalName() const;
    int GetIoCapability() const;

    bool SetLocalAddress(const std::string &addr) const;
    bool SetLocalName(const std::string &name) const;
    bool SetBleSecurity(bool security) const;
    bool SetSleLocalAddrType(int localAddrType) const;
    bool SetPairDirect(const std::string &section, const int activePair) const;

    /// peer
    std::string GetLinkKey(const std::string &section) const;
    bool GetLinkKeyChar(const std::string &section, char* lk, uint8_t valueLen) const;
    int GetCryptoAlgo(const std::string &subSection) const;
    int GetKeyDerivAlgo(const std::string &subSection) const;
    int GetIntegrChk(const std::string &subSection) const;
    std::string GetGroupKey(const std::string &section) const;
    uint64_t GetGiv(const std::string &section) const;

    /// Api for get paired device info.
    std::string GetPeerName(const std::string &subSection) const;
    std::string GetPeerAlias(const std::string &subSection) const;
    int GetPeerDeviceIoCapability(const std::string &subSection) const;
    uint8_t GetPeerAddressType(const std::string &subSection) const;
    std::string GetPeerRandomAddress(const std::string &subSection) const;
    int GetPeerAppearance(const std::string &subSection) const;
    std::string GetManufacturerAbility(const std::string &subSection) const;
    int GetPairDirect(const std::string &section) const;
    bool GetUserDisconnectedFlag(const std::string &subSection) const;

    bool SetPeerName(const std::string &subSection, const std::string &name) const;
    bool SetPeerDeviceIoCapability(const std::string &subSection, const int io) const;
    bool SetPeerAddressType(const std::string &subSection, const uint8_t type) const;
    bool SetPeerRandomAddress(const std::string &subSection, const std::string &addr) const;
    bool SetIsAudioDeviceFlag(const std::string &subSection, const bool isAudioDevice) const;
    bool GetIsAudioDeviceFlag(const std::string &subSection) const;
    bool RemovePairedDevice(const std::string &subSection) const;
    bool RemoveAllPairedDevices() const;
    bool SetPeerAlias(const std::string &subSection, const std::string &name) const;
    bool SetPeerAppearance(const std::string &subSection, const int appearance) const;
    bool SetUserDisconnectedFlag(const std::string &subSection, bool isUserDisconnected) const;
    bool SetManufacturerAbility(const std::string &subSection, const std::string &manuAbility) const;

    bool SetCdsmAddrType(const std::string &subSection, const int cdsmAddrType) const;
    int GetCdsmAddrType(const std::string &subSection) const;
    bool SetCdsmMemberList(const std::string &subSection, const std::vector<std::string> &cdsmAddrList) const;
    bool GetCdsmMemberList(const std::string &subSection, std::vector<std::string> &cdsmAddrList) const;
    bool SetCdsmIsPrivateDevice(const std::string &subSection, const int isPrivate) const;
    bool GetCdsmIsPrivateDevice(const std::string &subSection) const;
    bool RemoveCdsmGroup(const std::string &subSection) const;
    std::vector<std::string> GetAllCdsmReportList() const;

    bool SetLinkKey(const std::string &section, const std::string &lk) const;
    bool SetCryptoAlgo(const std::string &section, const int algo) const;
    bool SetKeyDerivAlgo(const std::string &section, const int algo) const;
    bool SetIntegrChk(const std::string &section, const int check) const;
    bool SetGroupkey(const std::string &section, const std::string &groupkey) const;
    bool SetGiv(const std::string &section, const uint64_t giv) const;

    std::vector<std::string> GetPairedAddrList() const;

    bool SetDeviceMediaVolume(const std::string &address, const int volume) const;
    int GetDeviceMediaVolume(const std::string &address, const int defaultVolume) const;

    bool SetDeviceCallVolume(const std::string &address, const int volume) const;
    int GetDeviceCallVolume(const std::string &address, const int defaultVolume) const;

    std::string GetBtAddrBySleAddr(const std::string &subSection) const;
    bool SetBtAddrBySleAddr(const std::string &subSection, const std::string &name) const;

    bool SetConfigWearDetectionState(const std::string &address, const int state) const;
    int GetConfigWearDetectionState(const std::string &address) const;
    bool SetAutoConnectSwitch(const std::string &address, const int result) const;
    int GetAutoConnectSwitch(const std::string &address) const;
    bool SetDeviceModelId(const std::string &address, const std::string &modelId) const;
    std::string GetDeviceModelId(const std::string &address) const;

    bool SetDeviceNewModelId(const std::string &address, const std::string &newModelId) const;
    std::string GetDeviceNewModelId(const std::string &address) const;

    bool SetDeviceSubModelId(const std::string &address, const std::string &subModelId) const;
    std::string GetDeviceSubModelId(const std::string &address) const;

    bool SetDeviceIconId(const std::string &address, const std::string &iconId) const;
    std::string GetDeviceIconId(const std::string &address) const;

    bool SetDeviceDevType(const std::string &address, const std::string &devType) const;
    std::string GetDeviceDevType(const std::string &address) const;

    bool SetSleBusiness(const std::string &subSection, const int sleBusinessType) const;
    int GetSleBusiness(const std::string &subSection) const;

    bool SetCloudDeviceBtAddr(const std::string &subSection, const std::string &btAddr) const;
    std::string GetCloudDeviceBtAddr(const std::string &subSection);
    bool SetCloudDeviceName(const std::string &subSection, const std::string &deviceName) const;
    std::string GetCloudDeviceName(const std::string &subSection);
    bool SetCloudDeviceToken(const std::string &subSection, const std::vector<uint8_t> &token) const;
    bool GetCloudDeviceToken(const std::string &subSection, std::vector<uint8_t> &token) const;
    bool GetCloudDeviceTokenChar(const std::string &address, char* token, uint8_t valueLen) const;

    bool SetCloudDeviceReportAddr(const std::string &subSection, const std::string &reportAddr) const;
    std::string GetCloudDeviceReportAddr(const std::string &subSection);
    bool SetCloudDeviceMembersAddrList(const std::string &subSection,
                                      const std::vector<std::string> &membersAddrList) const;
    bool GetCloudDeviceMembersAddrList(const std::string &subSection, std::vector<std::string> &membersAddrList);
    bool SetCloudDeviceModel(const std::string &subSection, const std::string &model) const;
    std::string GetCloudDeviceModel(const std::string &subSection);
    bool SetCloudDeviceSubModelId(const std::string &subSection, const std::string &subModelId) const;
    std::string GetCloudDeviceSubModelId(const std::string &subSection);
    bool SetCloudDeviceIconId(const std::string &subSection, const std::string &deviceIconId) const;
    std::string GetCloudDeviceIconId(const std::string &subSection);
    bool SetCloudDeviceState(const std::string &subSection, const int32_t cloudPairState) const;
    int32_t GetCloudDeviceState(const std::string &subSection);

    std::vector<std::string> GetCloudDeviceAddrList() const;

    bool RemoveAllCloudDevice() const;
    bool RemoveSpecificCloudDevice(const std::string &subSection) const;

    bool SetAvailableControl(const std::string &address, bool availableControl) const;
    bool GetAvailableControl(const std::string &address) const;

    bool SetLastASCActiveDevice(const std::string &device) const;
    std::string GetLastASCActiveDevice() const;
    bool RemoveLastASCActiveDevice() const;
    bool SetLastASCConnectedDevice(const std::string &device) const;
    std::string GetLastASCConnectedDevice() const;
    bool RemoveLastASCConnectedDevice() const;

    bool SetReconnectDeviceAddressList(const std::string &addressList) const;
    std::string GetReconnectDeviceAddressList() const;

private:
    /**
     * @brief Constructor.
     */
    SleConfig();

    /**
     * @brief Destructor.
     */
    ~SleConfig();
    /**
     * @brief Constructor.
     */
    SleConfig(SleConfig &) = delete;

    /**
     * @brief Constructor.
     */
    SleConfig &operator=(const SleConfig &) = delete;
    /**
     * @brief Device config single instance.
     */
    IAdapterDeviceConfig *config_ = nullptr;

    bool SetDeviceVolume(const std::string &address, const std::string &property, const int volume) const;
    int GetDeviceVolume(const std::string &address, const std::string &property, const int defaultVolume) const;
    bool IsRawLinkKeyCharValid(const char* linkkeyChar, const uint8_t linkKeyLen) const;
    bool IsRawCloudDeviceTokenCharValid(const char* tokenChar, const uint8_t tokenLen) const;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // SLE_CONFIG_H
