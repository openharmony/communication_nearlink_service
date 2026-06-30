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

#ifndef SLE_SERVICE_DATA_H
#define SLE_SERVICE_DATA_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>
#include "nearlink_def_types.h"
#include "sle_uuid.h"
#include "cstdint"
#include "iosfwd"
#include "raw_address.h"
#include "string"
#include "utility"

namespace OHOS {
namespace Nearlink {

enum class SleAdvTxPowerValue : int8_t {
    SLE_ADV_TX_POWER_ULTRA_LOW_VALUE = -21,
    SLE_ADV_TX_POWER_LOW_VALUE = -15,
    SLE_ADV_TX_POWER_MEDIUM_VALUE = -7,
    SLE_ADV_TX_POWER_HIGH_VALUE = 1,
    SLE_ADV_TX_POWER_ULTRA_HIGH_VALUE = 21, // 广播7档功率 21dBm
    SLE_ADV_TX_POWER_FIND_VALUE = 14, // 星闪查找广播功率
};

// SLE connection txPower Level, level_7: 21dBm, level_8: 23dBm
// currently only support level 7,8
enum class SleConnTxPowerValue : int8_t {
    SLE_CONN_TX_POWER_LEVEL_7_VALUE = 21,   // 7档功率，21dBm
    SLE_CONN_TX_POWER_LEVEL_8_VALUE = 23,   // 8档功率，23dBm
};

enum class SleCdsmAddrType : int8_t {
    CDSM_TYPE_NONE = 0,   /* 非合作集 */
    CDSM_TYPE_REPORT = 1, /* 合作集report */
    CDSM_TYPE_MEMBER = 2, /* 合作集成员 */
};

constexpr int SLE_INVALID_APPEARANCE = -1;

/**
 * @brief Represents advertise settings.
 *
 * @since 6
 */
class SleAdvertiserSettingsImpl {
public:
    /**
     * @brief A constructor used to create a <b>BleAdvertiseSettingsInternal</b> instance.
     *
     * @since 6
     */
    SleAdvertiserSettingsImpl(){};

    /**
     * @brief A destructor used to delete the <b>BleAdvertiseSettingsInternal</b> instance.
     *
     * @since 6
     */
    ~SleAdvertiserSettingsImpl(){};

    /**
     * @brief Check if device service is connectable.
     *
     * @return Returns <b>true</b> if device service is connectable;
     *         returns <b>false</b> if device service is not connectable.
     * @since 6
     */
    bool IsConnectable() const;

    /**
     * @brief Set connectable.
     *
     * @param connectable Whether it is connectable.
     * @since 6
     */
    void SetConnectable(bool connectable);
    /**
     * @brief Check if advertiser is legacy mode.
     *
     * @return Returns <b>true</b> if advertiser is legacy mode;
     *         returns <b>false</b> if advertiser is not legacy mode.
     * @since 6
     */
    bool IsLegacyMode() const;

    /**
     * @brief Set legacyMode.
     *
     * @param connectable Whether it is legacyMode.
     * @since 6
     */
    void SetLegacyMode(bool legacyMode);

    /**
     * @brief Get advertise interval.
     *
     * @return Returns advertise interval.
     * @since 6
     */
    int GetInterval() const;

    /**
     * @brief Set advertise interval.
     *
     * @param interval Advertise interval.
     * @since 6
     */
    void SetInterval(int interval = static_cast<int>(AdvInterval::ADV_INTERVAL_DEFAULT));

    /**
     * @brief Get advertiser Tx power.
     *
     * @return Returns advertiser Tx power.
     * @since 6
     */
    int GetTxPower() const;

    /**
     * @brief Set advertiser Tx power.
     *
     * @param txPower Advertiser Tx power.
     * @since 6
     */
    int SetTxPower(int txPower);

    /**
     * @brief Get primary phy.
     *
     * @return Returns primary phy.
     * @since 6
     */
    int GetPrimaryPhy() const;

    /**
     * @brief Set primary phy.
     *
     * @param primaryPhy Primary phy.
     * @since 6
     */
    void SetPrimaryPhy(int primaryPhy);

    /**
     * @brief Get second phy.
     *
     * @return Returns primary phy.
     * @since 6
     */
    int GetSecondaryPhy() const;

    /**
     * @brief Set second phy.
     *
     * @param secondaryPhy Second phy.
     * @since 6
     */
    void SetSecondaryPhy(int secondaryPhy);

    /**
     * @brief Get own address.
     *
     * @return Returns own address.
     * @since 6
     */
    std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN> GetOwnAddr() const;

    /**
     * @brief Set own address.
     *
     * @param addr Own address.
     * @since 6
     */
    void SetOwnAddr(const std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN>& addr);

    /**
     * @brief Get own address type.
     *
     * @return Returns own address type.
     * @since 6
     */
    int8_t GetOwnAddrType() const;

    /**
     * @brief Set own address type.
     *
     * @param addrType Own address type.
     * @since 6
     */
    void SetOwnAddrType(int8_t addrType);

    /**
     * @brief Get local link role.
     *
     * @return Returns local link role.
     * @since 6
     */
    uint8_t GetLinkRole() const;

    /**
     * @brief Set local link role.
     *
     * @param role Local link role.
     * @since 6
     */
    void SetLinkRole(uint8_t role);

    /**
     * @brief Get primary frame type.
     *
     * @return Returns primary frame type.
     * @since 6
     */
    uint8_t GetPrimaryFrameType() const;

    /**
     * @brief Set primary frame type.
     *
     * @param primaryFrameType primary frame type.
     * @since 6
     */
    void SetPrimaryFrameType(uint8_t primaryFrameType);

private:
    /// Advertising interval.
    int interval_ = static_cast<int>(AdvInterval::ADV_INTERVAL_DEFAULT);
    /// Advertising connectable.
    bool connectable_ = true;
    /// Advertising txPower.
    int txPower_ = static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_HIGH_VALUE);
    /// Advertising primary frame type.
    uint8_t primaryFrameType_ = static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_1);
    /// Advertising legacyMode.
    bool legacyMode_ = true;
    /// Advertising primaryPhy.
    int primaryPhy_ = static_cast<uint8_t>(SleAdvPhy::SLE_ADVERTISEMENT_PHY_1M);
    /// Advertising secondaryPhy.
    int secondaryPhy_ = static_cast<uint8_t>(SleAdvPhy::SLE_ADVERTISEMENT_PHY_1M);
    /// Own address.
    std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN> ownAddr_ = {};
    /// Own address type.
    int8_t ownAddrType_ = -1;
    uint8_t linkRole_ = static_cast<uint8_t>(SleLinkRole::T_CAN_NEGO);
};

/**
 * @brief Represents advertise data.
 *
 * @since 6
 */
class SleAdvertiserDataImpl {
public:
    /**
     * @brief A constructor used to create a <b>BleAdvertiseDataInternal</b> instance.
     *
     * @since 6
     */
    SleAdvertiserDataImpl();

    /**
     * @brief A destructor used to delete the <b>BleAdvertiseDataInternal</b> instance.
     *
     * @since 6
     */
    ~SleAdvertiserDataImpl()
    {}

    /**
     * @brief Add manufacture data.
     *
     * @param manufacturerId Manufacture Id which addad data.
     * @param data Manufacture data
     * @since 6
     */
    int AddManufacturerData(uint16_t manufacturerId, const std::string &data);

    /**
     * @brief Add service data.
     *
     * @param uuid Uuid of service data.
     * @param data Service data.
     * @since 6
     */
    void AddServiceData(const Uuid &uuid, const std::string &data);

    /**
     * @brief Add service uuid.
     *
     * @param uuid Service uuid.
     * @since 6
     */
    void AddServiceUuid(const Uuid &uuid);

    /**
     * @brief Set complete services.
     *
     * @param uuid Service uuid.
     * @since 6
     */
    void SetCompleteServices(const Uuid &uuid);

    /**
     * @brief Set advertiser flag.
     *
     * @param flag Advertiser flag.
     * @since 6
     */
    void SetFlags(uint8_t flag);

    /**
     * @brief Get advertiser flag.
     *
     * @return flag Advertiser flag.
     * @since 6
     */
    uint8_t GetFlags() const;

    /**
     * @brief Set manufacture data.
     *
     * @param data Manufacture data.
     * @since 6
     */
    void SetManufacturerData(const std::string &data);

    /**
     * @brief Set device name.
     *
     * @param name Device name.
     * @since 6
     */
    void SetDeviceName(const std::string &name);

    /**
     * @brief Set Tx power level.
     *
     * @param txPowerLevel Tx power level.
     * @since 6
     */
    void SetTxPowerLevel(uint8_t txPowerLevel);

    void SetTxPowerFlag(bool txPowerFlag);
    bool GetTxPowerFlag();

    /**
     * @brief Add service data.
     *
     * @param data Service data.
     * @since 6
     */
    void AddData(std::string data);

    /**
     * @brief Set advertiser data packet.
     *
     * @return Returns advertiser data packet.
     * @since 1.0
     * @version 1.0
     */
    void SetPayload(const std::string &payload);
    /**
     * @brief Get advertiser data packet.
     *
     * @return Returns advertiser data packet.
     * @since 6
     */
    std::string GetPayload() const;

private:
    /// Advertiser data packet
    std::string payload_ {};
    uint8_t advFlag_ = static_cast<uint8_t>(SleAdvertiserFlag::SLE_ADV_FLAG_GENERAL_DISC);
    bool includeTxPower_ = false;

    /**
     * @brief Set advertiser data long name.
     *
     * @param name Nearlink device name.
     * @since 6
     */
    void SetLongName(const std::string &name);

    /**
     * @brief Set advertiser data short name
     *
     * @param name Nearlink device name.
     * @since 6
     */
    void SetShortName(const std::string &name);
};

/**
 * @brief Parse advertisement parameter .
 *
 * @since 6
 */
struct SlePeripheralDeviceParseAdvData {
    uint8_t *payload = nullptr;
    size_t length = 0;
};
/**
 * @brief Represents peripheral device.
 *
 * @since 6
 */
class SlePeripheralDevice {
public:
    /**
     * @brief A constructor used to create a <b>SlePeripheralDevice</b> instance.
     *
     * @since 6
     */
    SlePeripheralDevice();

    /**
     * @brief A destructor used to delete the <b>SlePeripheralDevice</b> instance.
     *
     * @since 6
     */
    ~SlePeripheralDevice();

    /**
     * @brief Get device address.
     *
     * @return Returns device address.
     * @since 6
     */
    RawAddress GetRawAddress() const;

    /**
     * @brief Get collaborate address.
     * @return collaborate device address.
     * @since 6
     */
    RawAddress GetCollaborateAddress() const;

    /**
     * @brief Get current device address.
     *
     * @return Returns current device address.
     * @since 6
     */
    RawAddress GetCurrentRawAddress() const;

    /**
     * @brief Get device Appearance.
     *
     * @return Returns device Appearance.
     * @since 6
     */
    int GetAppearance() const;

    /**
     * @brief Get Manufacturer Data.
     *
     * @return Returns Manufacturer Data.
     * @since 6
     */
    std::map<uint16_t, std::string> GetManufacturerData() const;

    /**
     * @brief Get device Name.
     *
     * @return Returns device Name.
     * @since 6
     */
    std::string GetName() const;

    /**
     * @brief Get device RSSI.
     *
     * @return Returns device RSSI.
     * @since 6
     */
    int8_t GetRSSI() const;

    /**
     * @brief Get service Data.
     *
     * @return Returns service data.
     * @since 6
     */
    std::vector<std::string> GetServiceData() const;

    /**
     * @brief Get Service Data.
     *
     * @param index Service data index.
     * @return Returns service data.
     * @since 6
     */
    std::string GetServiceData(int index) const;

    /**
     * @brief Get service data UUID.
     *
     * @return Returns service data UUID.
     * @since 6
     */
    std::vector<Uuid> GetServiceDataUUID() const;

    /**
     * @brief Get serviceU UUID.
     *
     * @return Returns service UUID.
     * @since 6
     */
    std::vector<Uuid> GetServiceUUID() const;

    /**
     * @brief Get service UUID.
     *
     * @param index Service UUID index.
     * @return Return service UUID.
     * @since 6
     */
    Uuid GetServiceUUID(int index) const;

    /**
     * @brief Get advertiser data packet.
     *
     * @return Returns advertiser data packet.
     * @since 6
     */
    std::string GetPayload() const;

    /**
     * @brief Get advertising packet length.
     *
     * @return Returns advertising packet length.
     * @since 6
     */
    size_t GetPayloadLen() const;

    void SetPayload(uint8_t *payloadData, uint16_t payloadLenth);

    /**
     * @brief Get address type.
     *
     * @return Returns address type.
     * @since 6
     */
    uint8_t GetAddressType() const;

    /**
     * @brief Set address type.
     *
     * @param type Address type.
     * @since 6
     */
    void SetAddressType(uint8_t type);
    /**
     * @brief Check if include manufacture data.
     *
     * @return Returns <b>true</b> if include manufacture data;
     *         returns <b>false</b> if do not include manufacture data.
     * @since 6
     */
    bool IsManufacturerData() const;

    /**
     * @brief Check if include device rssi.
     *
     * @return Returns <b>true</b> if include device rssi;
     *         returns <b>false</b> if do not include device rssi.
     * @since 6
     */
    bool IsRSSI() const;

    /**
     * @brief Check if include service data.
     *
     * @return Returns <b>true</b> if include service data;
     *         returns <b>false</b> if do not include service data.
     * @since 6
     */
    bool IsServiceData() const;

    /**
     * @brief Check if include service UUID.
     *
     * @return Returns <b>true</b> if include service UUID;
     *         returns <b>false</b> if do not include service UUID.
     * @since 6
     */
    bool IsServiceUUID() const;

    bool IsName(void) const;

    bool IsAppearance() const;

    /**
     * @brief set device address.
     *
     * @param address device address.
     * @since 6
     */
    void SetAddress(const RawAddress &address);

    /**
     * @brief Set collaborate address.
     *
     * @param address collaborate device address.
     * @since 6
     */
    void SetCollaborateAddress(const RawAddress &address);

    /**
     * @brief set current device address.
     *
     * @param address device address.
     * @since 6
     */
    void SetCurrentRawAddress(const RawAddress &address);

    /**
     * @brief set rssi value.
     *
     * @param rssi rssi value.
     * @since 6
     */
    void SetRSSI(int8_t rssi);

    /**
     * @brief set rssi value.
     *
     * @param [in] rssi value.
     */
    bool IsConnectable() const;

    /**
     * @brief set rssi value.
     *
     * @param [in] rssi value.
     */
    void SetConnectable(bool connectable);

    /**
     * @brief Parse sle advertisement device info data.
     *
     */
    void ParseSleServiceData();

    /**
     * @brief Set service uuid 16 bit data.
     *
     * @param payload Advertisement packet.
     * @param total_len Advertisement packet len.
     * @since 6
     */
    void SetServiceUUID16Bits(SlePeripheralDeviceParseAdvData &parseAdvData);

    /**
     * @brief Set service uuid 128 bit data.
     *
     * @param payload Advertisement packet.
     * @param total_len Advertisement packet len.
     * @since 6
     */
    void SetServiceUUID128Bits(const SlePeripheralDeviceParseAdvData &parseAdvData);

    /**
     * @brief Set service data uuid 16 bit data.
     *
     * @param payload Advertisement packet.
     * @param total_len Advertisement packet len.
     * @since 6
     */
    void SetServiceDataUUID16Bits(SlePeripheralDeviceParseAdvData &parseAdvData);

    /**
     * @brief Set service data uuid 128 bit data.
     *
     * @param payload Advertisement packet.
     * @param total_len Advertisement packet len.
     * @since 6
     */
    void SetServiceDataUUID128Bits(SlePeripheralDeviceParseAdvData &parseAdvData);

    /**
     * @brief Set device name.
     *
     * @param name Device name.
     * @since 6
     */
    void SetName(const std::string &name);

    /**
     * @brief Set appearance.
     *
     * @param appearance Appearance.
     * @since 6
     */
    void SetAppearance(int appearance);

    /**
     * @brief Set device roles.
     *
     * @param roles Device roles.
     * @since 6
     */
    void SetRoles(uint8_t roles);

    /**
     * @brief Set bonded from local.
     *
     * @param flag Advertiser flag.
     * @since 6
     */
    void SetBondedFromLocal(bool flag);

    /**
     * @brief Set acb connect state.
     *
     * @param connectState Acb connect state.
     * @since 6
     */
    void SetAcbConnectState(int connectState);

    /**
     * @brief Get acb connect state.
     *
     * @return Returns Acb connect state.
     * @since 6
     */
    int GetAcbConnectState() const;

    /**
     * @brief Set lcid.
     *
     * @param handle Acb lcid.
     * @since 6
     */
    void SetLcid(uint16_t lcid);

    /**
     * @brief Set acb localIndex.
     *
     * @param handle Acb localIndex.
     * @since 6
     */
    void SetLocalIndex(uint16_t localIndex);

    /**
    * @brief Set acb connect direct.
    *
    * @param direct Acb Connect Direct.
    * @since 6
    */
    void SetConnDirect(int direct);

    /**
    * @brief Get acb connect direct.
    *
    * @return Returns Acb Connect Direct
    * @since 6
    */
    int GetConnDirect() const;

    /**
     * @brief Check if device acb connected.
     *
     * @return Returns <b>true</b> if device acb connected;
     *         returns <b>false</b> if device does not acb connect.
     * @since 6
     */
    bool IsAcbConnected() const;

    /**
    * @brief Check if device acb is connecting.
    *
    * @return Returns <b>true</b> if device acb is connecting;
    *         Returns <b>false</b> otherwise.
    * @since 6
    */
    bool IsAcbConnecting() const;

    /**
     * @brief Check if device acb Encrypted.
     *
     * @return Returns <b>true</b> if device acb Encrypted;
     *         returns <b>false</b> if device does not acb Encrypt.
     * @since 6
     */
    bool IsAcbEncrypted() const;

    /**
     * @brief Get local link role.
     *
     * @return Returns local link role.
     * @since 6
     */
    uint8_t GetLinkRole() const;

    /**
     * @brief Check if device was bonded from local.
     *
     * @return Returns <b>true</b> if device was bonded from local;
     *         returns <b>false</b> if device was not bonded from local.
     * @since 6
     */
    bool IsBondedFromLocal() const;

    /**
     * @brief Get lcid.
     *
     * @return Returns  lcid;
     * @since 6
     */
    uint16_t GetLcid() const;

    /**
     * @brief Get acb localIndex.
     *
     * @return Returns acb localIndex;
     * @since 6
     */
    uint16_t GetLocalIndex() const;

    /**
     * @brief Get advertising flag.
     *
     * @return Returns advertising flag.
     * @since 6
     */
    uint8_t GetAdFlag() const;

    /**
     * @brief Get paired status.
     *
     * @return Returns paired status.
     * @since 6
     */
    uint8_t GetPairedStatus() const;

    /**
     * @brief Get pre paired status.
     *
     * @return Returns pre paired status.
     * @since 6
     */
    uint8_t GetPrePairedStatus() const;

    /**
     * @brief Set paired status.
     *
     * @param status Paired status.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool SetPairedStatus(uint8_t status);

    /**
     * @brief Set Pre paired status.
     *
     * @param status Pre Paired status.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool SetPrePairedStatus(uint8_t status);

    /**
    * @brief Set paired direction.
    *
    * @param direction Paired direction.
    * @return Returns <b>true</b> if the operation is successful;
    *         Returns <b>false</b> otherwise.
    * @since 6
    */
    bool SetPairDirection(int direction);

    /**
    * @brief Get paired direction.
    *
    * @return Returns paired direction.
    * @since 6
    */
    int GetPairDirection() const;

    /**
     * @brief Set alias name.
     *
     * @param name Device alias name.
     * @since 6
     */
    void SetAliasName(const std::string &name);

    /**
     * @brief Get alias name.
     *
     * @return Returns alias name.
     * @since 6
     */
    std::string GetAliasName() const;

    /**
     * @brief Set IO capability.
     *
     * @param io IO capability
     * @since 6
     */
    void SetIoCapability(uint8_t io);

    /**
     * @brief Get IO capability.
     *
     * @return Returns IO capability.
     * @since 6
     */
    uint8_t GetIoCapability() const;

    /**
     * @brief Set service  UUID.
     *
     * @param serviceUUID Service  UUID.
     * @since 6
     */
    void SetServiceUUID(Uuid serviceUUID);

    /**
     * @brief Set cdsm address type.
     *
     * @param cdsmAddrType cdsm address type.
     * @since 6
     */
    void SetCdsmAddrType(int cdsmAddrType);

    /**
     * @brief Get cdsm address type.
     *
     * @return Returns cdsm address type.
     * @since 6
     */
    int GetCdsmAddrType();

    /**
     * @brief Check is cdsm member device.
     *
     * @return Returns cdsm flag.
     * @since 6
     */
    bool IsCdsmMember() const;

    /**
     * @brief Save cdsm device address list.
     *
     * @param cdsmDevList cdsm address list.
     * @since 6
     */
    void SaveCdsmDeviceList(std::vector<std::string> &cdsmDevList);

    /**
     * @brief Get cdsm device address list.
     *
     * @param cdsmDevList cdsm address list.
     * @return Returns cdsm address list.
     * @since 6
     */
    void GetCdsmDeviceList(std::vector<std::string> &cdsmDevList) const;

    /**
     * @brief Set bt address.
     *
     * @param string bt address.
     * @since 6
     */
    void SetBtAddr(const std::string &btAddr);

    /**
     * @brief Get Bt Address.
     *
     * @return Returns Bt address.
     * @since 6
     */
    std::string GetBtAddr() const;

    /**
    * @brief Set audio device flag.
    *
    * @param isAudioDevice audio device flag.
    * @since 6
    */
    void SetIsAudioDeviceFlag(bool isAudioDevice);

    /**
    * @brief Check is Audio Device.
    *
    * @return Returns audio device flag.
    * @since 6
    */
    bool GetIsAudioDeviceFlag() const;

    /**
     * @brief Set sle manufacturer business type.
     *
     * @param business sle manufacturer business type.
     * @since 6
     */
    void SetManufacturerBusiness(int business);

    /**
     * @brief Get sle manufacturer business type.
     *
     * @return Returns manufacturer business type.
     * @since 6
     */
    int GetManufacturerBusiness() const;

    /**
     * @brief Set sle device model id.
     *
     * @param modelId device model id.
     * @since 6
     */
    void SetModelId(const std::string &modelId);

    /**
     * @brief Get sle device model id.
     *
     * @return Returns device model id.
     * @since 6
     */
    std::string GetModelId() const;

    /**
     * @brief Set sle device new model id.
     *
     * @param newModelId device new model id.
     * @since 6
     */
    void SetNewModelId(const std::string &newModelId);

    /**
     * @brief Get sle device new model id.
     *
     * @return Returns device new model id.
     * @since 6
     */
    std::string GetNewModelId() const;

    /**
     * @brief Set sle device sub model id.
     *
     * @param subModelId device sub model id.
     * @since 6
     */
    void SetSubModelId(const std::string &subModelId);

    /**
     * @brief Get sle device sub model id.
     *
     * @return Returns device sub model id.
     * @since 6
     */
    std::string GetSubModelId() const;

    /**
     * @brief Set sle device icon id.
     *
     * @param iconId device icon id.
     * @since 6
     */
    void SetIconId(const std::string &iconId);

    /**
     * @brief Get sle device icon id.
     *
     * @return Returns device icon id.
     * @since 6
     */
    std::string GetIconId() const;

    /**
     * @brief 设置私有星闪设备类型
     *
     * @param 私有设备类型
     * @since 6
     */
    void SetDevType(const std::string &devType);

    /**
     * @brief 获取私有星闪设备类型
     *
     * @return 私有设备类型
     * @since 6
     */
    std::string GetDevType() const;

    void SetIsDeviceDisplay(bool isDeviceDisplay);

    void SetManufacturerAbility(const std::array<uint8_t, SLE_MANU_ABILITY_LEN> &manufacturerAbility);

    std::array<uint8_t, SLE_MANU_ABILITY_LEN> GetManufacturerAbility() const;

    bool GetIsDeviceDisplay() const;
    void SetCryptoAlgo(const uint8_t cryptoAlgo);
    uint8_t GetCryptoAlgo() const;
    void SetKeyDerivAlgo(const uint8_t keyDerivAlgo);
    uint8_t GetKeyDerivAlgo() const;
    void SetIntegrChkInd(const uint8_t integrChkInd);
    uint8_t GetIntegrChkInd() const;
    void SetEncryptGroupKeyStr(const std::string &encryptGroupKeyStr);
    std::string GetEncryptGroupKeyStr() const;
    void SetGiv(const uint64_t giv);
    uint64_t GetGiv() const;
    void SetIsUserDisconnected(const bool isUserDisconnected);
    bool GetIsUserDisconnected() const;

    void SetIsDeviceAvailable(bool isDeviceAvailable);
    bool GetIsDeviceAvailable() const;
    void SetServiceDataUUID(Uuid uuid, std::string data);
    void SetTXPower(int8_t txPower);
    void SetAdFlag(uint8_t adFlag);
    void SetManufacturerData(uint16_t manufacturerDataId, std::string manufacturerData);
private:

    /**
     * @brief Parse sle dis info data.
     *
     * @param serviceData DIS service data
     * @since 6
     */
    void ParseSleDisData();

    /**
     * @brief Parse sle HiLink info data.
     *
     * @param serviceData HiLink service data
     * @since 6
     */
    void ParseSleHiLinkData();

    /**
     * @brief Parse HiLink Nearby info data.
     * @param index current index in total service data
     * @param serviceData HiLink Nearby data.
     * @since 6
     */
    void ParseNearbyData(size_t &index, const std::string &serviceData);

    /**
     * @brief Parse Nearby ExtendValue.
     * @param extendValueType extendValueType
     * @param value adv string currently to be parsed
     * @since 6
     */
    void ParseNearbyExtendValue(uint8_t extendValueType, std::string &value);

    /// include appearance?
    bool isAppearance_ = false;
    /// include Manufacturer Data?
    bool isManufacturerData_ = false;
    /// include device name?
    bool isName_ = false;
    /// include rssi value?
    bool isRSSI_ = false;
    /// include service data?
    bool isServiceData_ = false;
    /// include service uuid?
    bool isServiceUUID_ = false;
    /// include tx power?
    bool isTXPower_ = false;
    /// is device display?
    bool isDeviceDisplay_ = true; // 默认设备可显示
    /// is device connect?
    bool isDeviceAvailable_ = true; // 默认设备可用
    // sle business type
    int manufacturerBusiness_ = 0;
    /// manfacturer ability
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> manufacturerAbility_ = {0};
    /// peer roles
    uint8_t roles_ = 0;
    /// device address
    RawAddress address_ = RawAddress("00:00:00:00:00:00");
    /// real device address
    RawAddress currentAddr_ = RawAddress("00:00:00:00:00:00");
    /// collaborate device address
    RawAddress collabAddress_ = RawAddress("00:00:00:00:00:00");
    /// advertising flag
    uint8_t adFlag_ = 0;
    /// appearance
    int appearance_ = SLE_INVALID_APPEARANCE;
    /// manufacturer Data
    std::map<uint16_t, std::string> manufacturerData_ {};
    /// device name
    std::string name_ {};
    /// rssi value
    int8_t rssi_ = 0;
    /// service uuid
    std::vector<Uuid> serviceUUIDs_ {};
    /// tx power
    int8_t txPower_ {};
    /// service data
    std::vector<std::string> serviceData_ {};
    /// service data uuid
    std::vector<Uuid> serviceDataUUIDs_ {};
    /// address type
    uint8_t addressType_ = static_cast<uint8_t>(SLE_ADDR_TYPE::SLE_PUBLIC_ADDRESS_TYPE);
    int acbConnected_ = 0;
    uint16_t lcid_ = 0;
    uint16_t localIndex_ = 0;
    bool bondFlag_ = false;
    int pairDirection_ {};
    uint8_t pairState_ {};
    uint8_t prePairState_ {};
    uint8_t ioCapability_ {};
    std::string aliasName_ {};
    bool connectable_ = true;
    std::string payload_;
    size_t payloadLen_ = 0;
    int acbConnectDirect_ = static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE);
    /* 合作集地址类型，仅类型为report时,cdsmDeviceList_ 不为空 */
    int cdsmAddrType_ = static_cast<int>(SleCdsmAddrType::CDSM_TYPE_NONE);
    std::vector<std::string> cdsmDeviceList_ {};
    std::string btAddr_ {};
    bool isAudioDevice_ = false;
    std::string modelId_ {};
    std::string newModelId_ {};
    std::string subModelId_ {};
    std::string iconId_ {};
    std::string devType_ {};
    uint8_t cryptoAlgo_ = 0;
    uint8_t keyDerivAlgo_ = 0;
    uint8_t integrChkInd_ = 0;
    std::string encryptGroupKeyStr_{};
    uint64_t giv_ = 0;
    bool isUserDisconnected_ = false; // 是否被用户主动断连
};

/**
 * @brief Represents scan result.
 *
 * @since 6
 */
class SleScanResultImpl {
public:
    /**
     * @brief A constructor used to create a <b>BleScanResultInternal</b> instance.
     *
     * @since 6
     */
    SleScanResultImpl() : peripheralDevice_()
    {}

    /**
     * @brief A destructor used to delete the <b>BleScanResultInternal</b> instance.
     *
     * @since 6
     */
    ~SleScanResultImpl()
    {}

    /**
     * @brief Get peripheral device.
     *
     * @return Returns peripheral device const reference
     * @since 6
     */
    const SlePeripheralDevice& GetPeripheralDevice() const;

    /**
     * @brief Set peripheral device.
     *
     * @param dev Peripheral device.
     * @since 6
     */
    void SetPeripheralDevice(const SlePeripheralDevice &dev);

    /**
     * @brief Get service uuids.
     *
     * @return Returns service uuids.
     * @since 6
     */
    std::vector<Uuid> GetServiceUuids() const
    {
        return serviceUuids_;
    }

    /**
     * @brief Get manufacture data.
     *
     * @return Returns manufacture data.
     * @since 6
     */
    std::map<uint16_t, std::string> GetManufacturerData() const
    {
        return manufacturerSpecificData_;
    }

    /**
     * @brief Get service data.
     *
     * @return Returns service data.
     * @since 6
     */
    std::map<Uuid, std::string> GetServiceData() const
    {
        return serviceData_;
    }

    /**
     * @brief Get peer device rssi.
     *
     * @return Returns peer device rssi.
     * @since 6
     */
    int8_t GetRssi() const
    {
        return rssi_;
    }

    /**
     * @brief Check if device is connectable.
     *
     * @return Returns <b>true</b> if device is connectable;
     *         returns <b>false</b> if device is not connectable.
     * @since 6
     */
    bool IsConnectable() const
    {
        return connectable_;
    }

    /**
     * @brief Get advertiser flag.
     *
     * @return Returns advertiser flag.
     * @since 6
     */
    uint8_t GetAdvertiseFlag() const
    {
        return advertiseFlag_;
    }

    /**
     * @brief Add manufacture data.
     *
     * @param manufacturerId Manufacture Id which addad data.
     * @since 6
     */
    void AddManufacturerData(uint16_t manufacturerId, std::string data)
    {
        manufacturerSpecificData_.insert(std::make_pair(manufacturerId, data));
    }

    /**
     * @brief Add service data.
     *
     * @param uuid Uuid of service data.
     * @param serviceData Service data.
     * @since 6
     */
    void AddServiceData(Uuid uuid, std::string serviceData)
    {
        serviceData_.insert(std::make_pair(uuid, serviceData));
    }

    /**
     * @brief Add service uuid.
     *
     * @param serviceUuid Service uuid.
     * @since 6
     */
    void AddServiceUuid(const Uuid &serviceUuid)
    {
        serviceUuids_.push_back(serviceUuid);
    }

    /**
     * @brief Set peripheral device.
     *
     * @param device Remote device.
     * @since 6
     */
    void SetPeripheralDevice(const RawAddress &device)
    {
        addr_ = device;
    }

    /**
     * @brief Set peer device rssi.
     *
     * @param rssi Peer device rssi.
     * @since 6
     */
    void SetRssi(int8_t rssi)
    {
        rssi_ = rssi;
    }

    /**
     * @brief Set connectable.
     *
     * @param connectable Whether it is connectable.
     * @since 6
     */
    void SetConnectable(bool connectable)
    {
        connectable_ = connectable;
    }

    /**
     * @brief Set advertiser flag.
     *
     * @param flag Advertiser flag.
     * @since 6
     */
    void SetAdvertiseFlag(uint8_t flag)
    {
        advertiseFlag_ = flag;
    }

    void SetPrimFrameType(uint8_t primFrameType)
    {
        primFrameType_ = primFrameType;
    }

    uint8_t GetPrimFrameType() const
    {
        return primFrameType_;
    }

private:
    /// scan device results
    SlePeripheralDevice peripheralDevice_;
    std::vector<Uuid> serviceUuids_ {};
    std::map<uint16_t, std::string> manufacturerSpecificData_ {};
    std::map<Uuid, std::string> serviceData_ {};
    RawAddress addr_ {};
    int8_t rssi_ {};
    bool connectable_ {};
    uint8_t advertiseFlag_ {};
    uint8_t primFrameType_ = static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_1);
};
}  // namespace Sle
}  // namespace OHOS
#endif  /// SLE_SERVICE_DATA_H
