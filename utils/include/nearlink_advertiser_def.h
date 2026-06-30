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

#ifndef NEARLINK_ADVERTISER_DEF_H
#define NEARLINK_ADVERTISER_DEF_H

#include <map>
#include <vector>

#include "sle_service_data.h"
#include "sle_uuid.h"
#include "raw_address.h"

namespace OHOS {
namespace Nearlink {
class AdvertiserData {
public:
    /**
     * @brief A constructor used to create a NearlinkAdvertiserData instance.
     *
     * @since 6
     */
    AdvertiserData(){};

    /**
     * @brief A destructor used to delete the NearlinkAdvertiserData instance.
     *
     * @since 6
     */
    ~AdvertiserData(){};

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
     * @brief Get advertiser flag.
     *
     * @return Returns advertiser flag.
     * @since 6
     */
    uint8_t GetAdvFlag() const
    {
        return advFlag_;
    }

    /**
     * @brief Get payload.
     *
     * @return Returns payload.
     * @since 6
     */
    std::string GetPayload() const
    {
        return payload_;
    }

    /**
     * @brief Set advertiser flag.
     *
     * @param flag Advertiser flag.
     * @since 6
     */
    void SetAdvFlag(uint8_t flag)
    {
        advFlag_ = flag;
    }

    /**
     * @brief Set payload data.
     *
     * @param Payload payload.
     * @since 6
     */
    void SetPayload(const std::string &payload)
    {
        payload_ = payload;
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
     * @brief Get whether the device name will be included in the advertisement packet.
     *
     * @return Returns includeDeviceName flag.
     * @since 6
     */
    bool GetIncludeDeviceName() const
    {
        return includeDeviceName_;
    }

    /**
     * @brief Set whether the device name will be included in the advertisement packet.
     *
     * @param flag includeDeviceName flag.
     * @since 6
     */
    void SetIncludeDeviceName(bool flag)
    {
        includeDeviceName_ = flag;
    }

    /**
     * @brief Get whether the txpower will be included in the advertisement packet.
     *
     * @return Returns includeTxPower flag.
     * @since 10
     */
    bool GetIncludeTxPower() const
    {
        return includeTxPower_;
    }

    /**
     * @brief Set whether the txpower will be included in the advertisement packet.
     *
     * @param flag includeTxPower flag.
     * @since 10
     */
    void SetIncludeTxPower(bool flag)
    {
        includeTxPower_ = flag;
    }

public:
    std::vector<Uuid> serviceUuids_ {};
    std::map<uint16_t, std::string> manufacturerSpecificData_ {};
    std::map<Uuid, std::string> serviceData_ {};
    uint8_t advFlag_ {};
    std::string payload_ = "";
    bool includeDeviceName_ = false;
    bool includeTxPower_ = false;
};

/**
 * @brief Represents advertise settings.
 *
 * @since 6
 */
class AdvertiserSettings {
public:
    /**
     * @brief A constructor used to create a NearlinkAdvertiseSettings instance.
     *
     * @since 6
     */
    AdvertiserSettings(){};

    /**
     * @brief A destructor used to delete the NearlinkAdvertiseSettings instance.
     *
     * @since 6
     */
    ~AdvertiserSettings(){};

    /**
     * @brief Check if device service is connectable.
     *
     * @return Returns true if device service is connectable;
     *         returns false if device service is not connectable.
     * @since 6
     */
    bool IsConnectable() const
    {
        return connectable_;
    }

    /**
     * @brief Check if advertiser is legacy mode.
     *
     * @return Returns true if advertiser is legacy mode;
     *         returns false if advertiser is not legacy mode.
     * @since 6
     */
    bool IsLegacyMode() const
    {
        return legacyMode_;
    }

    /**
     * @brief Get advertise interval.
     *
     * @return Returns advertise interval.
     * @since 6
     */
    int GetInterval() const
    {
        return interval_;
    }

    /**
     * @brief Get Tx power.
     *
     * @return Returns Tx power.
     * @since 6
     */
    int GetTxPower() const
    {
        return txPower_;
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
     * @brief Set legacyMode.
     *
     * @param connectable Whether it is legacyMode.
     * @since 6
     */
    void SetLegacyMode(bool legacyMode)
    {
        legacyMode_ = legacyMode;
    }

    /**
     * @brief Set advertise interval.
     *
     * @param interval Advertise interval.
     * @since 6
     */
    void SetInterval(uint16_t interval)
    {
        interval_ = interval;
    }

    /**
     * @brief Set Tx power.
     *
     * @param txPower Tx power.
     * @since 6
     */
    void SetTxPower(uint8_t txPower)
    {
        txPower_ = txPower;
    }

    uint8_t GetEnableHighPower() const
    {
        return enableHighPower_;
    }

    void SetEnableHighPower(uint8_t enableHighPower)
    {
        enableHighPower_ = enableHighPower;
    }

    uint8_t GetPrimaryFrameType() const
    {
        return primaryFrameType_;
    }

    void SetPrimaryFrameType(uint8_t primaryFrameType)
    {
        primaryFrameType_ = primaryFrameType;
    }

    /**
     * @brief Get primary phy.
     *
     * @return Returns primary phy.
     * @since 6
     */
    int GetPrimaryPhy() const
    {
        return primaryPhy_;
    }

    /**
     * @brief Set primary phy.
     *
     * @param primaryPhy Primary phy.
     * @since 6
     */
    void SetPrimaryPhy(int primaryPhy)
    {
        primaryPhy_ = primaryPhy;
    }

    /**
     * @brief Get second phy.
     *
     * @return Returns primary phy.
     * @since 6
     */
    int GetSecondaryPhy() const
    {
        return secondaryPhy_;
    }

    /**
     * @brief Set second phy.
     *
     * @param secondaryPhy Second phy.
     * @since 6
     */
    void SetSecondaryPhy(int secondaryPhy)
    {
        secondaryPhy_ = secondaryPhy;
    }

    /**
     * @brief Get own address.
     *
     * @param addr Own address.
     * @since 6
     */
    std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN> GetOwnAddr() const
    {
        return ownAddr_;
    }

    /**
     * @brief Set own address.
     *
     * @param addr Own address.
     * @since 6
     */
    void SetOwnAddr(const std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN>& addr)
    {
        ownAddr_ = addr;
    }

    /**
     * @brief Get own address type.
     *
     * @return Returns own address type.
     * @since 6
     */
    int8_t GetOwnAddrType() const
    {
        return ownAddrType_;
    }

    /**
     * @brief Set own address type.
     *
     * @param addrType Own address type.
     * @since 6
     */
    void SetOwnAddrType(int8_t addrType)
    {
        ownAddrType_ = addrType;
    }

    /**
     * @brief Get local link role.
     *
     * @return Local link role.
     * @since 6
     */
    uint8_t GetLinkRole() const
    {
        return linkRole_;
    }

    /**
     * @brief Set local link role.
     *
     * @param role Local link role.
     * @since 6
     */
    void SetLinkRole(uint8_t role)
    {
        linkRole_ = role;
    }

public:
    bool connectable_ {};
    bool legacyMode_ {};
    uint16_t interval_ {};
    uint8_t txPower_ {};
    uint8_t primaryFrameType_ {};
    uint8_t enableHighPower_ {};
    int primaryPhy_ {};
    int secondaryPhy_ {};
    std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN> ownAddr_ = {};
    int8_t ownAddrType_ = -1;
    uint8_t linkRole_ = 0;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  /// NEARLINK_ADVERTISER_DEF_H