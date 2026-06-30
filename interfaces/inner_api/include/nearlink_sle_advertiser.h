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

/**
 * @addtogroup Nearlink
 * @{
 *
 * @brief Defines advertiser, including avertise data and callbacks, and advertiser functions.
 *
 * @since 6
 */

/**
 * @file nearlink_sle_advertiser.h
 *
 * @brief Advertiser common functions.
 *
 * @since 6
 */

#ifndef NEARLINK_SLE_ADVERTISER_H
#define NEARLINK_SLE_ADVERTISER_H

#include "nearlink_def.h"
#include "nearlink_types.h"
#include "nearlink_errorcode.h"
#include "nearlink_uuid.h"

namespace OHOS {
namespace Nearlink {

const int NEARLINK_SLE_ADDR_LEN = 6;

/**
 * @brief Represents advertise data.
 *
 * @since 6
 */
class NEARLINK_API SleAdvertiserData {
public:
    /**
     * @brief A constructor used to create a <b>SleAdvertiserData</b> instance.
     *
     * @since 6
     */
    SleAdvertiserData();

    /**
     * @brief A destructor used to delete the <b>SleAdvertiserData</b> instance.
     *
     * @since 6
     */
    ~SleAdvertiserData();

    /**
     * @brief Add manufacture data.
     *
     * @param manufacturerId Manufacture Id which addad data.
     * @param data Manufacture data
     * @since 6
     */
    void AddManufacturerData(uint16_t manufacturerId, const std::string &data);

    /**
     * @brief Add service data.
     *
     * @param uuid Uuid of service data, uuid is big endian order.
     * @param serviceData Service data.
     * @since 6
     */
    void AddServiceData(const UUID &uuid, const std::string &serviceData);

    /**
     * @brief Add service uuid.
     *
     * @param serviceUuid Service uuid, uuid is big endian order.
     * @since 6
     */
    void AddServiceUuid(const UUID &serviceUuid);

    /**
     * @brief Get manufacture data.
     *
     * @return Returns manufacture data.
     * @since 6
     */
    std::map<uint16_t, std::string> GetManufacturerData() const;

    /**
     * @brief Get service data.
     *
     * @return Returns service data, uuid is big endian order.
     * @since 6
     */
    std::map<UUID, std::string> GetServiceData() const;

    /**
     * @brief Get service uuids.
     *
     * @return Returns service uuids, uuid is big endian order.
     * @since 6
     */
    std::vector<UUID> GetServiceUuids() const;

    /**
     * @brief Get advertiser flag.
     *
     * @return Returns advertiser flag.
     * @since 6
     */
    uint8_t GetAdvFlag() const;

    /**
     * @brief Set advertiser flag.
     *
     * @param flag Advertiser flag.
     * @return Returns error flag.
     * @since 6
     */
    void SetAdvFlag(uint8_t flag);

    /**
     * @brief Get whether the device name will be included in the advertisement packet.
     *
     * @return Returns includeDeviceName flag.
     * @since 6
     */
    bool GetIncludeDeviceName() const;

    /**
     * @brief Set whether the device name will be included in the advertisement packet.
     *
     * @param flag includeDeviceName flag.
     * @since 6
     */
    void SetIncludeDeviceName(bool flag);

    /**
     * @brief Get whether the txpower will be included in the advertisement packet.
     *
     * @return Returns includeTxPower flag.
     * @since 10
     */
    bool GetIncludeTxPower() const;

    /**
     * @brief Set whether the txpower will be included in the advertisement packet.
     *
     * @param flag includeTxPower flag.
     * @since 10
     */
    void SetIncludeTxPower(bool flag);

private:
    std::vector<UUID> serviceUuids_{}; // uuid is big endian order.
    std::map<uint16_t, std::string> manufacturerSpecificData_{};
    std::map<UUID, std::string> serviceData_{}; // uuid is big endian order.
    uint8_t advFlag_ = static_cast<uint8_t>(SleAdvertiserFlag::SLE_ADV_FLAG_GENERAL_DISC);
    bool includeDeviceName_ = false;
    bool includeTxPower_ = false;
};

/**
 * @brief Represents advertise settings.
 *
 * @since 6
 */
class NEARLINK_API SleAdvertiserSettings {
public:
    /**
     * @brief A constructor used to create a <b>SleAdvertiseSettings</b> instance.
     *
     * @since 6
     */
    SleAdvertiserSettings();

    /**
     * @brief A destructor used to delete the <b>SleAdvertiseSettings</b> instance.
     *
     * @since 6
     */
    ~SleAdvertiserSettings();

    /**
     * @brief Check if device service is connctable.
     *
     * @return Returns <b>true</b> if device service is connctable;
     *         returns <b>false</b> if device service is not connctable.
     * @since 6
     */
    bool IsConnectable() const;

    /**
     * @brief Check if advertiser is legacy mode.
     *
     * @return Returns <b>true</b> if advertiser is legacy mode;
     *         returns <b>false</b> if advertiser is not legacy mode.
     * @since 6
     */
    bool IsLegacyMode() const;

    /**
     * @brief Set connectable.
     *
     * @param connectable Whether it is connectable.
     * @since 6
     */
    void SetConnectable(bool connectable);

    /**
     * @brief Set legacyMode.
     *
     * @param legacyMode Whether it is legacyMode.
     * @since 6
     */
    void SetLegacyMode(bool legacyMode);

    /**
     * @brief Get advertise interval.
     *
     * @return Returns advertise interval.
     * @since 6
     */
    uint32_t GetInterval() const;

    /**
     * @brief Get Tx power.
     *
     * @return Returns Tx power.
     * @since 6
     */
    uint8_t GetTxPower() const;

    /**
     * @brief Set advertise interval.
     *
     * @param interval Advertise interval.
     * @since 6
     */
    void SetInterval(uint32_t interval);

    /**
     * @brief Set Tx power.
     *
     * @param txPower Tx power.
     * @since 6
     */
    void SetTxPower(uint8_t txPower);

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
     * @param addr Own address, address is little endian order.
     * @since 6
     */
    std::array<uint8_t, NEARLINK_SLE_ADDR_LEN> GetOwnAddr() const;

    /**
     * @brief Set own address.
     *
     * @param addr Own address, address is little endian order.
     * @since 6
     */
    void SetOwnAddr(const std::array<uint8_t, NEARLINK_SLE_ADDR_LEN>& addr);

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

private:
    bool connectable_ = true;
    bool legacyMode_ = true;
    uint32_t interval_ = static_cast<int>(AdvInterval::ADV_INTERVAL_DEFAULT);
    uint8_t txPower_ = static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_LOW);
    uint8_t primaryFrameType_ = static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_1);
    int primaryPhy_ = static_cast<uint8_t>(SleAdvPhy::SLE_ADVERTISEMENT_PHY_1M);
    int secondaryPhy_ = static_cast<uint8_t>(SleAdvPhy::SLE_ADVERTISEMENT_PHY_1M);
    std::array<uint8_t, NEARLINK_SLE_ADDR_LEN> ownAddr_ = {}; // address is little endian order.
    int8_t ownAddrType_ = -1;
    uint8_t linkRole_ = static_cast<uint8_t>(SleLinkRole::T_CAN_NEGO);
};

/**
 * @brief Represents advertise callback.
 *
 * @since 6
 */
class SleAdvertiseCallback {
public:
    /**
     * @brief A destructor used to delete the <b>SleAdvertiseCallback</b> instance.
     *
     * @since 6
     */
    virtual ~SleAdvertiseCallback() = default;

    /**
     * @brief Start advertising result event callback.
     *
     * @param result Start advertising result.
     * @param advHandle advertising handle.
     * @since 6
     */
    virtual void OnStartResultEvent(int result, int advHandle) = 0;

    /**
     * @brief Enable advertising result event callback.
     *
     * @param result Enable advertising result.
     * @param advHandle advertising handle.
     * @since 11
     */
    virtual void OnEnableResultEvent(int result, int advHandle){};

    /**
     * @brief Disable advertising result event callback.
     *
     * @param result Disable advertising result.
     * @param advHandle advertising handle.
     * @since 11
     */
    virtual void OnDisableResultEvent(int result, int advHandle){};

    /**
     * @brief Stop advertising result event callback.
     *
     * @param result Stop advertising result.
     * @param advHandle advertising handle.
     * @since 11
     */
    virtual void OnStopResultEvent(int result, int advHandle){};

    /**
     * @brief Set advertising data result event callback.
     *
     * @param result Set advertising data result
     * @since 6
     */
    virtual void OnSetAdvDataEvent(int result) = 0;

    /**
     * @brief Get advertising handle callback.
     *
     * @param result get advertising handle result.
     * @param advHandle advertising handle.
     * @since 11
     */
    virtual void OnGetAdvHandleEvent(int result, int advHandle){};
};

/**
 * @brief Represents advertiser.
 *
 * @since 6
 */
class NEARLINK_API SleAdvertiser {
public:
    /**
     * @brief A constructor of SleAdvertiser.
     */
    static std::shared_ptr<SleAdvertiser> CreateSleAdvertiser(void);

    /**
     * @brief Start advertising.
     *
     * @param settings Advertise settings.
     * @param advData Advertise data.
     * @param scanResponse Scan response.
     * @param callback Advertise callback.
     * @return Returns the status code for this function called.
     */
    NlErrCode StartAdvertising(const SleAdvertiserSettings &settings, const SleAdvertiserData &advData,
        const SleAdvertiserData &scanResponse, uint16_t duration, std::shared_ptr<SleAdvertiseCallback> callback);

    /**
     * @brief Enable advertising.
     *
     * @param advHandle Advertise handle.
     * @return Returns the status code for this function called.
     */
    NlErrCode EnableAdvertising(uint8_t advHandle);

    /**
     * @brief Disable advertising.
     *
     * @param advHandle Advertise handle.
     * @return Returns the status code for this function called.
     */
    NlErrCode DisableAdvertising(uint8_t advHandle);

    NlErrCode SetAdvertisingData(const SleAdvertiserData &advData, const SleAdvertiserData &scanResponse,
        std::shared_ptr<SleAdvertiseCallback> callback);

    NlErrCode StopAdvertising(std::shared_ptr<SleAdvertiseCallback> callback);

    NlErrCode StopAdvertising(uint8_t advHandle);

    /**
     * @brief Get Advertise handle.
     *
     * @param callback Advertise callback.
     * @return Returns the status code for this function called.
     */
    NlErrCode GetAdvHandle(std::shared_ptr<SleAdvertiseCallback> callback, uint8_t &advHandle);

    ~SleAdvertiser();

private:
    SleAdvertiser();

    NEARLINK_DISALLOW_COPY_AND_ASSIGN(SleAdvertiser);

    NEARLINK_DECLARE_IMPL();

    // It has no specific meaning, only used to hide the constructor.
    struct Pattern {
        Pattern() {};
    };

public:
    // This constructor is not available, use CreateSleAdvertiser interface to create objects.
    explicit SleAdvertiser(Pattern) : SleAdvertiser() {};
};
}  // namespace Nearlink
}  // namespace OHOS
#endif