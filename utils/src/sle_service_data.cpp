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

#include "sle_service_data.h"
#include "nearlink_def.h"

#include <algorithm>
#include <iostream>
#include <cstring>
#include <iomanip>
#include <sstream>

#include "array"
#include "map"
#include "vector"
#include "log.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {
constexpr uint8_t SLE_ADV_DATA_BYTE_FIELD_LEN = 3;
constexpr uint8_t SLE_ADV_DATA_ONE_BYTE_LEN = 1;
constexpr uint8_t SLE_ADV_MANUFACTURER_ID_LEN = 2;
constexpr uint8_t SLE_ONE_BYTE_LEN = 8;
constexpr uint8_t SLE_ADV_DEVICE_INFO_MIN_LEN = 2;
constexpr size_t SERVICE_UUID_MAX_NUM = 0xFFFF;
enum class SleIoCapability : uint8_t {
    SLE_IO_CAP_OUT = 0x00,    // DisplayOnly
    SLE_IO_CAP_IO = 0x01,    // DisplayYesNo
    SLE_IO_CAP_IN = 0x02,    // KeyboardOnly
    SLE_IO_CAP_NONE = 0x03,    // NoInputNoOutput
    SLE_IO_CAP_KBDISP = 0x04    // Keyboard display
};
// The type of advertising data(not adv_type)
using SLE_ADV_DATA_TYPE = enum {
    SLE_AD_TYPE_FLAG = 0x01,
    SLE_AD_TYPE_16SRV_PART = 0x02,
    SLE_AD_TYPE_16SRV_CMPL = 0x03,
    SLE_AD_TYPE_32SRV_PART = 0x04,
    SLE_AD_TYPE_32SRV_CMPL = 0x05,
    SLE_AD_TYPE_128SRV_PART = 0x06,
    SLE_AD_TYPE_128SRV_CMPL = 0x07,
    SLE_AD_TYPE_NAME_SHORT = 0x08,
    SLE_AD_TYPE_NAME_CMPL = 0x09,
    SLE_AD_TYPE_TX_PWR = 0x0A,
    SLE_AD_TYPE_DEV_CLASS = 0x0D,
    SLE_AD_TYPE_SM_TK = 0x10,
    SLE_AD_TYPE_SM_OOB_FLAG = 0x11,
    SLE_AD_TYPE_INT_RANGE = 0x12,
    SLE_AD_TYPE_SOL_SRV_UUID = 0x14,
    SLE_AD_TYPE_128SOL_SRV_UUID = 0x15,
    SLE_AD_TYPE_SERVICE_DATA = 0x16,
    SLE_AD_TYPE_PUBLIC_TARGET = 0x17,
    SLE_AD_TYPE_RANDOM_TARGET = 0x18,
    SLE_AD_TYPE_APPEARANCE = 0x19,
    SLE_AD_TYPE_ADV_INT = 0x1A,
    SLE_AD_TYPE_LE_DEV_ADDR = 0x1b,
    SLE_AD_TYPE_LE_ROLE = 0x1c,
    SLE_AD_TYPE_SPAIR_C256 = 0x1d,
    SLE_AD_TYPE_SPAIR_R256 = 0x1e,
    SLE_AD_TYPE_32SOL_SRV_UUID = 0x1f,
    SLE_AD_TYPE_32SERVICE_DATA = 0x20,
    SLE_AD_TYPE_128SERVICE_DATA = 0x21,
    SLE_AD_TYPE_LE_SECURE_CONFIRM = 0x22,
    SLE_AD_TYPE_LE_SECURE_RANDOM = 0x23,
    SLE_AD_TYPE_URI = 0x24,
    SLE_AD_TYPE_INDOOR_POSITION = 0x25,
    SLE_AD_TYPE_TRANS_DISC_DATA = 0x26,
    SLE_AD_TYPE_LE_SUPPORT_FEATURE = 0x27,
    SLE_AD_TYPE_CHAN_MAP_UPDATE = 0x28,
    SLE_AD_MANUFACTURER_SPECIFIC_TYPE = 0xFF,
};
// The type of sle advertising data(not adv_type)
using SLE_ADV_DATA_TYPE_DEFINE = enum {
    SLE_ADV_DATA_TYPE_DISCOVERY_LEVEL = 0x01,
    SLE_ADV_DATA_TYPE_ACCESS_MODE = 0x02,
    SLE_ADV_DATA_TYPE_SERVICE_DATA_16BIT_UUID = 0x03,
    SLE_ADV_DATA_TYPE_SERVICE_DATA_128BIT_UUID = 0x04,
    SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_16BIT_SERVICE_UUIDS = 0x05,
    SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_128BIT_SERVICE_UUIDS = 0x06,
    SLE_ADV_DATA_TYPE_INCOMPLETE_LIST_OF_16BIT_SERVICE_UUIDS = 0x07,
    SLE_ADV_DATA_TYPE_INCOMPLETE_LIST_OF_128BIT_SERVICE_UUIDS = 0x08,
    SLE_ADV_DATA_TYPE_SERVICE_STRUCTURE_HASH_VALUE = 0x09,
    SLE_ADV_DATA_TYPE_SHORTENED_LOCAL_NAME = 0x0A,
    SLE_ADV_DATA_TYPE_COMPLETE_LOCAL_NAME = 0x0B,
    SLE_ADV_DATA_TYPE_TX_POWER_LEVEL = 0x0C,
    SLE_ADV_DATA_TYPE_SLB_COMMUNICATION_DOMAIN = 0x0D,
    SLE_ADV_DATA_TYPE_SLB_MEDIA_ACCESS_LAYER_ID = 0x0E,
    SLE_ADV_DATA_TYPE_EXTENDED = 0xFE,
    SLE_ADV_DATA_TYPE_MANUFACTURER_SPECIFIC_DATA = 0xFF
};
// The type of sle device info data(uuid = 0x0609)
using SLE_ADV_DEVICE_INFO_TYPE_DEFINE = enum {
    SLE_DEVIE_INFO_LOCAL_NAME = 0x06,
    SLE_DEVIE_INFO_APPEARANCE = 0x07,
};

using VendorAdvService = enum {
    VENDOR_NEARBY = 0x01,
    VENDOR_VIRTUAL_CONNECT,
};

// The type of sle HiLink Nearby Extend Type(uuid = 0xFDEE)
// 继承自蓝牙HiLink定义
using SLE_HILINK_NEARBY_EXTEND_TYPE_DEFINE = enum {
    SLE_HILINK_ICONNECT_BUSINESS = 0x01,    // iConnect业务（Business）
    SLE_HILINK_RSSI,                        // RSSI
    SLE_HILINK_MODEL_ID,                    // Model ID
    SLE_HILINK_SUB_MODEL_ID,                // Sub Model ID
    SLE_HILINK_PAIRED_DEVICE_ID,            // 配对设备ID
    SLE_HILINK_CONNECTED_DEVICE_NUM,        // 连接设备数
    SLE_HILINK_PAIRED_DEVICE_NUM,           // 配对设备数
    SLE_HILINK_MAX_CONNECT_DEVICE_NUM,      // 最大设备连接数
    SLE_HILINK_MAX_PAIR_DEVICE_NUM,         // 最大设备配对数
    SLE_HILINK_DUAL_DEVICE_ID,              // 双模设备识别编码
    SLE_HILINK_MAX_BATTERY,                 // 总计电量
    SLE_HILINK_LEFT_BATTERY,                // 左耳电量
    SLE_HILINK_RIGHT_BATTERY,               // 右耳电量
    SLE_HILINK_CHARGE_BOARD_BATTERY,        // 充电盒电量
    SLE_HILINK_EXT_FEATURE,                 // 增强特性
    SLE_HILINK_ADV_INDEX,                   // 广播序号
    SLE_HILINK_ADV_POWER,                   // 微距AdvPower
    SLE_HILINK_NEW_MODEL_ID,                // New Model ID
    SLE_HILINK_ICON_ID,                     // 设备图标
    SLE_HILINK_CLASS_OF_DEVICE,             // 设备形态类别
    SLE_HILINK_BLE_ADV_ADDR,                // BLE 广播地址
    SLE_HILINK_EXT = 0xFF,                  // 自定义数据，不定长，后续所有有效数据
};

constexpr uint8_t SLE_HILINK_NEARBY_LEAST_LENGTH = 3;

using SLE_HILINK_NEARBY_EXTEND_TYPE_LENGTH = enum {
    LEN_ONE_OCTET = 0x1,
    LEN_TWO_OCTETS,
    LEN_THREE_OCTETS,
    LEN_FOUR_OCTETS,
    LEN_FIVE_OCTETS,
    LEN_SIX_OCTETS,
};

// 靠近发现业务采用TV格式，每种Type对应的Length如下：
const std::map<uint8_t, uint8_t> hiLinkDictionary = {
    {SLE_HILINK_ICONNECT_BUSINESS, LEN_ONE_OCTET},
    {SLE_HILINK_RSSI, LEN_ONE_OCTET},
    {SLE_HILINK_MODEL_ID, LEN_THREE_OCTETS},
    {SLE_HILINK_SUB_MODEL_ID, LEN_ONE_OCTET},
    {SLE_HILINK_PAIRED_DEVICE_ID, LEN_TWO_OCTETS},
    {SLE_HILINK_CONNECTED_DEVICE_NUM, LEN_ONE_OCTET},
    {SLE_HILINK_PAIRED_DEVICE_NUM, LEN_ONE_OCTET},
    {SLE_HILINK_MAX_CONNECT_DEVICE_NUM, LEN_ONE_OCTET},
    {SLE_HILINK_MAX_PAIR_DEVICE_NUM, LEN_ONE_OCTET},
    {SLE_HILINK_DUAL_DEVICE_ID, LEN_TWO_OCTETS},
    {SLE_HILINK_MAX_BATTERY, LEN_ONE_OCTET},
    {SLE_HILINK_LEFT_BATTERY, LEN_ONE_OCTET},
    {SLE_HILINK_RIGHT_BATTERY, LEN_ONE_OCTET},
    {SLE_HILINK_CHARGE_BOARD_BATTERY, LEN_ONE_OCTET},
    {SLE_HILINK_EXT_FEATURE, LEN_ONE_OCTET},
    {SLE_HILINK_ADV_INDEX, LEN_ONE_OCTET},
    {SLE_HILINK_ADV_POWER, LEN_ONE_OCTET},
    {SLE_HILINK_NEW_MODEL_ID, LEN_FOUR_OCTETS},
    {SLE_HILINK_ICON_ID, LEN_TWO_OCTETS},
    {SLE_HILINK_CLASS_OF_DEVICE, LEN_ONE_OCTET},
    {SLE_HILINK_BLE_ADV_ADDR, LEN_SIX_OCTETS},
};

/**
 * @brief Check if the device service is connectable.
 *
 * @return Returns <b>true</b> if device service is connectable;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SleAdvertiserSettingsImpl::IsConnectable() const
{
    return connectable_;
}

/**
 * @brief Set whether the device service is connectable.
 *
 * @param connectable Whether the device service is connectable.
 * @since 6
 */
void SleAdvertiserSettingsImpl::SetConnectable(bool connectable)
{
    connectable_ = connectable;
}

/**
 * @brief Check if the advertiser is in legacy mode.
 *
 * @return Returns <b>true</b> if the advertiser is in legacy mode;
 *         Returns <b>false</b> otherwisee.
 * @since 6
 */
bool SleAdvertiserSettingsImpl::IsLegacyMode() const
{
    return legacyMode_;
}

/**
 * @brief Set whether to enable the legacy mode.
 *
 * @param connectable Whether to enable the legacy mode
 * @since 6
 */
void SleAdvertiserSettingsImpl::SetLegacyMode(bool legacyMode)
{
    legacyMode_ = legacyMode;
}

/**
 * @brief Get advertise interval.
 *
 * @return Returns the advertising interval.
 * @since 6
 */
int SleAdvertiserSettingsImpl::GetInterval() const
{
    return interval_;
}

/**
 * @brief Set advertise interval.
 *
 * @param interval Advertise interval.
 * @since 6
 */
void SleAdvertiserSettingsImpl::SetInterval(int interval)
{
    interval_ = interval;
}

/**
 * @brief Get the advertiser Tx power.
 *
 * @return Returns advertiser Tx power.
 * @since 6
 */
int SleAdvertiserSettingsImpl::GetTxPower() const
{
    return txPower_;
}

/**
 * @brief Set the advertiser Tx power.
 *
 * @param txPowerthe advertiser Tx power.
 * @since 6
 */
int SleAdvertiserSettingsImpl::SetTxPower(int txPower)
{
    if (txPower >= static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_INVAILD) ||
        txPower < static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_ULTRA_LOW)) {
        return static_cast<int>(ReturnValue::RET_BAD_PARAM);
    }
    switch (txPower) {
        case static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_ULTRA_LOW):
            txPower_ = static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_ULTRA_LOW_VALUE);
            break;
        case static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_LOW):
            txPower_ = static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_LOW_VALUE);
            break;
        case static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_MEDIUM):
            txPower_ = static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_MEDIUM_VALUE);
            break;
        case static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_HIGH):
            txPower_ = static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_HIGH_VALUE);
            break;
        case static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_ULTRA_HIGH):
            txPower_ = static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_ULTRA_HIGH_VALUE);
            break;
        case static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_FIND):
            txPower_ = static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_FIND_VALUE);
            break;
        default:
            txPower_ = static_cast<int8_t>(SleAdvTxPowerValue::SLE_ADV_TX_POWER_LOW_VALUE);
            break;
    }

    return static_cast<int>(ReturnValue::RET_NO_ERROR);
}

/**
 * @brief Get the primary phy.
 *
 * @return Returns the primary phy.
 * @since 6
 */
int SleAdvertiserSettingsImpl::GetPrimaryPhy() const
{
    return primaryPhy_;
}

/**
 * @brief Set the primary phy.
 *
 * @param primaryPhy Primary phy.
 * @since 6
 */
void SleAdvertiserSettingsImpl::SetPrimaryPhy(int primaryPhy)
{
    primaryPhy_ = primaryPhy;
}

/**
 * @brief Get the secondary Phy.
 *
 * @return Returns primary phy.
 * @since 6
 */
int SleAdvertiserSettingsImpl::GetSecondaryPhy() const
{
    return secondaryPhy_;
}

/**
 * @brief Set the secondary phy.
 *
 * @param secondaryPhy Secondary Phy.
 * @since 6
 */
void SleAdvertiserSettingsImpl::SetSecondaryPhy(int secondaryPhy)
{
    secondaryPhy_ = secondaryPhy;
}

/**
 * @brief Get own address.
 *
 * @param addr Own address.
 * @since 6
 */
std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN> SleAdvertiserSettingsImpl::GetOwnAddr() const
{
    return ownAddr_;
}

/**
 * @brief Set own address.
 *
 * @param addr Own address.
 * @since 6
 */
void SleAdvertiserSettingsImpl::SetOwnAddr(const std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN>& addr)
{
    ownAddr_ = addr;
}

/**
 * @brief Get own address type.
 *
 * @return Returns own address type.
 * @since 6
 */
int8_t SleAdvertiserSettingsImpl::GetOwnAddrType() const
{
    return ownAddrType_;
}

/**
 * @brief Set own address type.
 *
 * @param addrType Own address type.
 * @since 6
 */
void SleAdvertiserSettingsImpl::SetOwnAddrType(int8_t addrType)
{
    ownAddrType_ = addrType;
}

/**
 * @brief Get local link role.
 *
 * @return Returns local link role.
 * @since 6
 */
uint8_t SleAdvertiserSettingsImpl::GetLinkRole() const
{
    return linkRole_;
}

/**
 * @brief Set local link role.
 *
 * @param role Local link role.
 * @since 6
 */
void SleAdvertiserSettingsImpl::SetLinkRole(uint8_t role)
{
    linkRole_ = role;
}

/**
 * @brief Get primary frame type.
 *
 * @return Returns primary frame type.
 * @since 6
 */
uint8_t SleAdvertiserSettingsImpl::GetPrimaryFrameType() const
{
    return primaryFrameType_;
}

/**
 * @brief Set primary frame type.
 *
 * @param primaryFrameType primary frame type.
 * @since 6
 */
void SleAdvertiserSettingsImpl::SetPrimaryFrameType(uint8_t primaryFrameType)
{
    primaryFrameType_ = primaryFrameType;
}

/**
 * @brief A constructor used to create a <b>BleAdvertiseDataInternal</b> instance.
 *
 * @since 6
 */
SleAdvertiserDataImpl::SleAdvertiserDataImpl() : payload_()
{}

/**
 * @brief Add manufacturer data.
 *
 * @param manufacturerId manufacturer Id which addad data.
 * @param data manufacturer data
 * @since 6
 */
int SleAdvertiserDataImpl::AddManufacturerData(uint16_t manufacturerId, const std::string &data)
{
    char cdata[SLE_ADV_MANUFACTURER_ID_LEN];
    cdata[0] = static_cast<char>(manufacturerId & 0xFF);
    cdata[1] = static_cast<char>((manufacturerId >> SLE_ONE_BYTE_LEN) & 0xFF);
    SetManufacturerData(std::string(cdata, SLE_ADV_MANUFACTURER_ID_LEN) + data);

    return static_cast<int>(ReturnValue::RET_NO_ERROR);
}

/**
 * @brief Add service data.
 *
 * @param uuid Uuid of service data.
 * @param data Service data.
 * @since 6
 */
void SleAdvertiserDataImpl::AddServiceData(const Uuid &uuid, const std::string &data)
{
    if ((data.length() + static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_16)) > 0xff ||
        (data.length() + static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_128)) > 0xff) {
        HILOGE("AddServiceData, the data bytes is out of range.");
        return;
    }
    char cdata[SLE_ADV_DATA_FIELD_TYPE_AND_LEN];
    switch (uuid.GetUuidType()) {
        case Uuid::UUID16_BYTES_TYPE: {
            /// [Len] [0x16] [UUID16] data
            cdata[0] = SLE_ADV_DATA_TYPE_SERVICE_DATA_16BIT_UUID;
            cdata[1] = data.length() + static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_16);
            uint16_t uuid16 = uuid.ConvertTo16Bits();
            AddData(std::string(cdata, SLE_ADV_DATA_FIELD_TYPE_AND_LEN) +
                    std::string(reinterpret_cast<char *>(&uuid16), static_cast<int>(UuidLength::SLE_UUID_LEN_16)) +
                    data);
            break;
        }

        case Uuid::UUID128_BYTES_TYPE: {
            /// [Len] [0x21] [UUID128] data
            cdata[0] = SLE_ADV_DATA_TYPE_SERVICE_DATA_128BIT_UUID;
            cdata[1] = data.length() + static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_128);
            uint8_t uuidData[static_cast<int>(UuidLength::SLE_UUID_LEN_128)];
            uuid.ConvertToBytesLE(uuidData, static_cast<int>(UuidLength::SLE_UUID_LEN_128));
            AddData(std::string(cdata, SLE_ADV_DATA_FIELD_TYPE_AND_LEN) +
                    std::string(reinterpret_cast<char *>(uuidData), static_cast<int>(UuidLength::SLE_UUID_LEN_128)) +
                    data);
            break;
        }

        default:
            return;
    }
}

/**
 * @brief Add service uuid.
 *
 * @param uuid Service uuid.
 * @since 6
 */
void SleAdvertiserDataImpl::AddServiceUuid(const Uuid &uuid)
{
    SetCompleteServices(uuid);
}

/**
 * @brief Set complete services.
 *
 * @param uuid Service uuid.
 * @since 6
 */
void SleAdvertiserDataImpl::SetCompleteServices(const Uuid &uuid)
{
    char cdata[SLE_ADV_DATA_FIELD_TYPE_AND_LEN];
    switch (uuid.GetUuidType()) {
        case Uuid::UUID16_BYTES_TYPE: {
            cdata[0] = SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_16BIT_SERVICE_UUIDS;
            cdata[1] = static_cast<int>(UuidLength::SLE_UUID_LEN_16);
            uint16_t uuid16 = uuid.ConvertTo16Bits();
            AddData(std::string(cdata, SLE_ADV_DATA_FIELD_TYPE_AND_LEN) +
                    std::string(reinterpret_cast<char *>(&uuid16), static_cast<int>(UuidLength::SLE_UUID_LEN_16)));
            break;
        }
        case Uuid::UUID128_BYTES_TYPE: {
            cdata[0] = SLE_ADV_DATA_TYPE_COMPLETE_LIST_OF_128BIT_SERVICE_UUIDS;
            cdata[1] = static_cast<int>(UuidLength::SLE_UUID_LEN_128);
            uint8_t uuidData[static_cast<int>(UuidLength::SLE_UUID_LEN_128)];
            uuid.ConvertToBytesLE(uuidData, static_cast<int>(UuidLength::SLE_UUID_LEN_128));
            AddData(std::string(cdata, SLE_ADV_DATA_FIELD_TYPE_AND_LEN) +
                    std::string(reinterpret_cast<char *>(uuidData), static_cast<int>(UuidLength::SLE_UUID_LEN_128)));
            break;
        }

        default:
            return;
    }
}

/**
 * @brief Set advertiser flag.
 *
 * @param flag Advertiser flag.
 * @since 6
 */
void SleAdvertiserDataImpl::SetFlags(uint8_t flag)
{
    char cdata[SLE_ADV_DATA_BYTE_FIELD_LEN];
    cdata[0] = SLE_ADV_DATA_TYPE_DISCOVERY_LEVEL;  /// 0x01
    cdata[1] = SLE_ADV_DATA_ONE_BYTE_LEN;
    cdata[SLE_ADV_DATA_FIELD_TYPE_AND_LEN] = flag;
    advFlag_ = flag;
    AddData(std::string(cdata, SLE_ADV_DATA_BYTE_FIELD_LEN));
}

uint8_t SleAdvertiserDataImpl::GetFlags() const
{
    return advFlag_;
}

/**
 * @brief Set manufacturer data.
 *
 * @param data manufacturer data.
 * @since 6
 */
void SleAdvertiserDataImpl::SetManufacturerData(const std::string &data)
{
    if (data.length() > 0xff) {
        HILOGE("SetManufacturerData, the data bytes is out of range.");
        return;
    }
    char cdata[SLE_ADV_DATA_FIELD_TYPE_AND_LEN];
    cdata[0] = SLE_ADV_DATA_TYPE_MANUFACTURER_SPECIFIC_DATA;  /// 0xff
    cdata[1] = data.length();
    AddData(std::string(cdata, SLE_ADV_DATA_FIELD_TYPE_AND_LEN) + data);
}

/**
 * @brief Set device name.
 *
 * @param name Device name.
 * @since 6
 */
void SleAdvertiserDataImpl::SetDeviceName(const std::string &name)
{
    if (name.length() > DEVICE_NAME_MAX_LEN) {
        SetLongName(name);
    } else {
        SetShortName(name);
    }
}

/**
 * @brief Set Tx power level.
 *
 * @param txPowerLevel Tx power level.
 * @since 6
 */
void SleAdvertiserDataImpl::SetTxPowerLevel(uint8_t txPowerLevel)
{
    char cdata[SLE_ADV_DATA_BYTE_FIELD_LEN];
    cdata[0] = SLE_ADV_DATA_TYPE_TX_POWER_LEVEL;
    cdata[1] = SLE_ADV_DATA_ONE_BYTE_LEN;
    cdata[2] = txPowerLevel; // byte 2 is txPowerLevel
    AddData(std::string(cdata, SLE_ADV_DATA_BYTE_FIELD_LEN));
}

void SleAdvertiserDataImpl::SetTxPowerFlag(bool txPowerFlag)
{
    includeTxPower_ = txPowerFlag;
}

bool SleAdvertiserDataImpl::GetTxPowerFlag()
{
    return includeTxPower_;
}

/**
 * @brief Add service data.
 *
 * @param data Service data.
 * @since 6
 */
void SleAdvertiserDataImpl::AddData(std::string data)
{
    payload_.append(data);
}

/**
 * @brief Get advertiser data packet.
 *
 * @return Returns advertiser data packet.
 * @since 6
 */
void SleAdvertiserDataImpl::SetPayload(const std::string &payload)
{
    payload_ = payload;
}

/**
 * @brief Get advertiser data packet.
 *
 * @return Returns advertiser data packet.
 * @since 6
 */
std::string SleAdvertiserDataImpl::GetPayload() const
{
    return payload_;
}

/**
 * @brief Set advertiser data long name.
 *
 * @param name Nearlink device name.
 * @since 6
 */
void SleAdvertiserDataImpl::SetLongName(const std::string &name)
{
    NL_CHECK_RETURN(name.length() <= 0xff, "SetLongName, the name bytes is out of range.");
    char cdata[SLE_ADV_DATA_FIELD_TYPE_AND_LEN];
    cdata[0] = SLE_ADV_DATA_TYPE_SHORTENED_LOCAL_NAME;
    cdata[1] = name.length();
    AddData(std::string(cdata, SLE_ADV_DATA_FIELD_TYPE_AND_LEN) + name);
}

/**
 * @brief Set advertiser data short name.
 *
 * @param name Nearlink device name.
 * @since 6
 */
void SleAdvertiserDataImpl::SetShortName(const std::string &name)
{
    NL_CHECK_RETURN(name.length() <= 0xff, "SetShortName, the name bytes is out of range.");
    char cdata[SLE_ADV_DATA_FIELD_TYPE_AND_LEN];
    cdata[0] = SLE_ADV_DATA_TYPE_COMPLETE_LOCAL_NAME;
    cdata[1] = name.length();
    AddData(std::string(cdata, SLE_ADV_DATA_FIELD_TYPE_AND_LEN) + name);
}

SlePeripheralDevice::SlePeripheralDevice()
    : manufacturerData_(),
      name_(),
      serviceUUIDs_(),
      txPower_(static_cast<int>(SLE_ADDR_TYPE::SLE_RANDOM_ADDRESS_TYPE)),
      serviceData_(),
      serviceDataUUIDs_(),
      pairState_(static_cast<int>(SlePairState::SLE_PAIR_NONE)),
      prePairState_(static_cast<int>(SlePairState::SLE_PAIR_NONE)),
      ioCapability_(static_cast<uint8_t>(SleIoCapability::SLE_IO_CAP_NONE)),
      aliasName_()
{
    manufacturerData_.clear();
    serviceDataUUIDs_.clear();
    serviceData_.clear();
    serviceUUIDs_.clear();
}

SlePeripheralDevice::~SlePeripheralDevice()
{
    manufacturerData_.clear();
    serviceUUIDs_.clear();
    serviceData_.clear();
    serviceDataUUIDs_.clear();
}

/**
 * @brief Get device address.
 *
 * @return Returns device address.
 * @since 6
 */
RawAddress SlePeripheralDevice::GetRawAddress() const
{
    return address_;
}

/**
 * @brief Get device current address.
 *
 * @return Returns device current address.
 * @since 6
 */
RawAddress SlePeripheralDevice::GetCurrentRawAddress() const
{
    return currentAddr_;
}

/**
 * @brief Set device current address.
 *
 * @return Returns device current address.
 * @since 6
 */
void SlePeripheralDevice::SetCurrentRawAddress(const RawAddress &address)
{
    currentAddr_ = address;
}

/**
 * @brief Get device appearance.
 *
 * @return Returns the device appearance.
 * @since 6
 */
int SlePeripheralDevice::GetAppearance() const
{
    return appearance_;
}

bool SlePeripheralDevice::IsAppearance() const
{
    return isAppearance_;
}

/**
 * @brief Get the manufacturer data.
 *
 * @return Returns the manufacturer data.
 * @since 6
 */
std::map<uint16_t, std::string> SlePeripheralDevice::GetManufacturerData() const
{
    return manufacturerData_;
}

/**
 * @brief Get sle manufacturer business type.
 *
 * @return Returns manufacturer business type.
 * @since 6
 */
int SlePeripheralDevice::GetManufacturerBusiness() const
{
    return manufacturerBusiness_;
}

/**
 * @brief Set sle manufacturer business type.
 *
 * @param business sle manufacturer business type.
 * @since 6
 */
void SlePeripheralDevice::SetManufacturerBusiness(int business)
{
    manufacturerBusiness_ = business;
}

/**
 * @brief the device name.
 *
 * @return Returns device Name.
 * @since 6
 */
std::string SlePeripheralDevice::GetName() const
{
    return name_;
}

/**
 * @brief Get device RSSI.
 *
 * @return Returns device RSSI.
 * @since 6
 */
int8_t SlePeripheralDevice::GetRSSI() const
{
    return rssi_;
}

/**
 * @brief Get service data.
 *
 * @return Returns service data.
 * @since 6
 */
std::vector<std::string> SlePeripheralDevice::GetServiceData() const
{
    return serviceData_;
}

/**
 * @brief Get Service Data.
 *
 * @param index Service data index.
 * @return Returns service data.
 * @since 6
 */
std::string SlePeripheralDevice::GetServiceData(int index) const
{
    return serviceData_.empty() ? "" : ((size_t)index < serviceData_.size() ? serviceData_[index] : "");
}

/**
 * @brief Get service data UUID.
 *
 * @return Returns service data UUID.
 * @since 6
 */
std::vector<Uuid> SlePeripheralDevice::GetServiceDataUUID() const
{
    return serviceDataUUIDs_;
}

/**
 * @brief Get the service UUID.
 *
 * @return Returns service UUID.
 * @since 6
 */
std::vector<Uuid> SlePeripheralDevice::GetServiceUUID() const
{
    return serviceUUIDs_;
}

/**
 * @brief Get service UUID.
 *
 * @param index Service UUID index.
 * @return Return service UUID.
 * @since 6
 */
Uuid SlePeripheralDevice::GetServiceUUID(int index) const
{
    Uuid uuid {};
    return serviceUUIDs_.empty() ? uuid : serviceUUIDs_[index];
}

/**
 * @brief Get address type.
 *
 * @return Returns address type.
 * @since 6
 */
uint8_t SlePeripheralDevice::GetAddressType() const
{
    return addressType_;
}

/**
 * @brief Set address type.
 *
 * @param type Address type.
 * @since 6
 */
void SlePeripheralDevice::SetAddressType(uint8_t type)
{
    addressType_ = type;
}

/**
 * @brief Check if manufacturer data is included.
 *
 * @return Returns <b>true</b> if manufacturer data is included;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SlePeripheralDevice::IsManufacturerData() const
{
    return isManufacturerData_;
}

/**
 * @brief Check if the device RSSI is included.
 *
 * @return Returns <b>true</b> if include device rssi;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SlePeripheralDevice::IsRSSI() const
{
    return isRSSI_;
}

/**
 * @brief Check if service data is included.
 *
 * @return Returns <b>true</b> if include service data;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SlePeripheralDevice::IsServiceData() const
{
    return isServiceData_;
}

/**
 * @brief Check if the service UUID is included.
 *
 * @return Returns <b>true</b> if the service UUID is included;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SlePeripheralDevice::IsServiceUUID() const
{
    return isServiceUUID_;
}

bool SlePeripheralDevice::IsName(void) const
{
    return isName_;
}

/**
 * @brief Set device address.
 *
 * @param address device address.
 * @since 6
 */
void SlePeripheralDevice::SetAddress(const RawAddress &address)
{
    address_ = address;
}

/**
 * @brief Set collaborate address.
 *
 * @param address collaborate device address.
 * @since 6
 */
void SlePeripheralDevice::SetCollaborateAddress(const RawAddress &address)
{
    collabAddress_ = address;
}

/**
 * @brief Get collaborate address.
 * @return collaborate device address.
 * @since 6
 */
RawAddress SlePeripheralDevice::GetCollaborateAddress() const
{
    return collabAddress_;
}

/**
 * @brief Set RSSI value.
 *
 * @param RSSI value.
 * @since 6
 */
void SlePeripheralDevice::SetRSSI(int8_t rssi)
{
    rssi_ = rssi;
    isRSSI_ = true;
}
/**
 * @brief Check whether device is connectable.
 *
 * @param [in] rssi value.
 * return Returns <b>true</b> if device is connectable.
 *        Returns <b>false</b> otherwisee.
 */
bool SlePeripheralDevice::IsConnectable() const
{
    return connectable_;
}
/**
 * @brief Sets whether the peer device is connectable.
 *
 * @param peer device's connectable.
 */
void SlePeripheralDevice::SetConnectable(bool connectable)
{
    connectable_ = connectable;
}

void SlePeripheralDevice::SetModelId(const std::string &modelId)
{
    modelId_ = modelId;
}

std::string SlePeripheralDevice::GetModelId() const
{
    return modelId_;
}

void SlePeripheralDevice::SetNewModelId(const std::string &newModelId)
{
    newModelId_ = newModelId;
}

std::string SlePeripheralDevice::GetNewModelId() const
{
    return newModelId_;
}

void SlePeripheralDevice::SetSubModelId(const std::string &subModelId)
{
    subModelId_ = subModelId;
}

std::string SlePeripheralDevice::GetSubModelId() const
{
    return subModelId_;
}

void SlePeripheralDevice::SetIconId(const std::string &iconId)
{
    iconId_ = iconId;
}

std::string SlePeripheralDevice::GetIconId() const
{
    return iconId_;
}

void SlePeripheralDevice::SetDevType(const std::string &devType)
{
    devType_ = devType;
}
 
std::string SlePeripheralDevice::GetDevType() const
{
    return devType_;
}

void SlePeripheralDevice::SetCryptoAlgo(const uint8_t cryptoAlgo)
{
    cryptoAlgo_ = cryptoAlgo;
}

uint8_t SlePeripheralDevice::GetCryptoAlgo() const
{
    return cryptoAlgo_;
}

void SlePeripheralDevice::SetKeyDerivAlgo(const uint8_t keyDerivAlgo)
{
    keyDerivAlgo_ = keyDerivAlgo;
}

uint8_t SlePeripheralDevice::GetKeyDerivAlgo() const
{
    return keyDerivAlgo_;
}

void SlePeripheralDevice::SetIntegrChkInd(const uint8_t integrChkInd)
{
    integrChkInd_ = integrChkInd;
}

uint8_t SlePeripheralDevice::GetIntegrChkInd() const
{
    return integrChkInd_;
}

void SlePeripheralDevice::SetEncryptGroupKeyStr(const std::string &encryptGroupKeyStr)
{
    encryptGroupKeyStr_ = encryptGroupKeyStr;
}

std::string SlePeripheralDevice::GetEncryptGroupKeyStr() const
{
    return encryptGroupKeyStr_;
}

void SlePeripheralDevice::SetGiv(const uint64_t giv)
{
    giv_ = giv;
}

uint64_t SlePeripheralDevice::GetGiv() const
{
    return giv_;
}

void SlePeripheralDevice::SetIsUserDisconnected(const bool isUserDisconnected)
{
    isUserDisconnected_ = isUserDisconnected;
}

bool SlePeripheralDevice::GetIsUserDisconnected() const
{
    return isUserDisconnected_;
}

void SlePeripheralDevice::ParseSleServiceData()
{
    ParseSleDisData();
    ParseSleHiLinkData();
}

void SlePeripheralDevice::ParseSleDisData()
{
    std::vector<Uuid> sleServiceUUIDArray = GetServiceDataUUID();
    std::string disServiceData = "";
    for (size_t j = 0; j < sleServiceUUIDArray.size(); j++) {
        Uuid uuid = sleServiceUUIDArray[j];
        if (uuid == Uuid::ConvertFromString(SLE_UUID_DIS)) {
            disServiceData = GetServiceData(j);
            HILOGD("has DIS data");
            break;
        }
    }
    if (disServiceData.empty()) {
        HILOGW("Dis service data is empty");
        return;
    }
    int appearance = 0;
    size_t i = 0;
    int type = -1;
    size_t length = 0;
    while ((i + SLE_ADV_DEVICE_INFO_MIN_LEN) < disServiceData.size()) {
        type = disServiceData[i++];
        length = disServiceData[i++];
        if (length <= 0 || i + length > disServiceData.size()) {
            break;
        }
        switch (type) {
            case SLE_DEVIE_INFO_LOCAL_NAME:
                SetName(disServiceData.substr(i, length));
                break;
            case SLE_DEVIE_INFO_APPEARANCE: {
                HILOGD("has APPEARANCE");
                if (length != SLE_ADV_DEVICE_APPEARANCE_LEN) {
                    break;
                }
                errno_t ret =
                    memcpy_s(&appearance, sizeof(appearance), &disServiceData[i], SLE_ADV_DEVICE_APPEARANCE_LEN);
                NL_CHECK_RETURN(EOK == ret, "memcpy_s failed!");
                SetAppearance(appearance);
                break;
            }
            default:
                break;
        }
        i = i + length;
    }
}

std::string BytesToHexString(const std::string &bytes)
{
    std::ostringstream oss;
    for (const auto c : bytes) {
        oss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

std::string FormatBtAddr(const std::string &address) {
    std::ostringstream oss;
    for (size_t i = 0; i < address.length(); i += 2) {
        if (i > 0) {
            oss << ":";
        }
        oss << address.substr(i, 2);
    }
    return oss.str();
}

void EndianReverse(std::string &data)
{
    for (size_t i = 0; i < data.size() / 2; ++i) {
        std::swap(data[i], data[data.size() - 1 - i]);
    }
}

void SlePeripheralDevice::ParseSleHiLinkData()
{
    std::vector<Uuid> sleServiceUUIDArray = GetServiceDataUUID();
    std::string hiLinkServiceData = "";
    for (size_t j = 0; j < sleServiceUUIDArray.size(); j++) {
        Uuid uuid = sleServiceUUIDArray[j];
        if (uuid == Uuid::ConvertFromString(SLE_UUID_HILINK)) {
            hiLinkServiceData = GetServiceData(j);
            HILOGD("has hilink data");
            break;
        }
    }
    if (hiLinkServiceData.length() < SLE_HILINK_NEARBY_LEAST_LENGTH) {
        // 前三个字符，分别代表version、mainBusiness、extendBusiness，最少要保证data大于3才可解析
        HILOGD("parse HiLink data length error: length=%{public}zu", hiLinkServiceData.length());
        return;
    }
    size_t i = 0;
    uint8_t version = hiLinkServiceData[i++];
    uint8_t mainBusiness = hiLinkServiceData[i++];
    uint8_t extendBusiness = hiLinkServiceData[i++];
    if (mainBusiness != VENDOR_NEARBY && mainBusiness != VENDOR_VIRTUAL_CONNECT) {
        HILOGI("business = 0x%{public}x", mainBusiness);
        return;
    }
    HILOGI("version = 0x%{public}x, business = 0x%{public}x, extendBusiness = 0x%{public}x",
        version, mainBusiness, extendBusiness);
    if (mainBusiness == VENDOR_NEARBY) {    // 靠近发现
        ParseNearbyData(i, hiLinkServiceData);
    }
}

void SlePeripheralDevice::ParseNearbyData(size_t &index, const std::string &serviceData)
{
    uint8_t length = 0;
    uint8_t extendValueType = 0;
    while (index < serviceData.size()) {
        extendValueType = serviceData[index++];
        auto iter = hiLinkDictionary.find(extendValueType);
        NL_CHECK_RETURN(iter != hiLinkDictionary.end(),
                    "invilid hilink adv type=%{public}d.", extendValueType);
        length = iter->second;
        NL_CHECK_RETURN(index + length <= serviceData.size(), "parse type(%{public}d) length error",
                    extendValueType);
        std::string value = serviceData.substr(index, length);
        ParseNearbyExtendValue(extendValueType, value);
        index = index + length;
    }
}

void SlePeripheralDevice::ParseNearbyExtendValue(uint8_t extendValueType, std::string &value)
{
    switch (extendValueType) {
        case SLE_HILINK_MODEL_ID: { // 设备产品编号，3个字节，全场景不够用，耳机只会用modelId
            std::string modelId = BytesToHexString(value);
            SetModelId(modelId);
            HILOGI("modelId = %{public}s", modelId.c_str());
            break;
        }
        case SLE_HILINK_NEW_MODEL_ID: { // 手写笔、手表，4个字节字符串
            // 为了与全场景Profile中ProdId（4字节）拉通且方便N设备接入，全场景设备统一管理，Model ID升级为New Model ID
            std::string newModelId = BytesToHexString(value);
            SetNewModelId(newModelId);
            HILOGI("newModelId = %{public}s", newModelId.c_str());
            break;
        }
        case SLE_HILINK_SUB_MODEL_ID: { // 设备子编号，用来区分颜色
            std::string subModelId = BytesToHexString(value);
            SetSubModelId(subModelId);
            HILOGI("subModelId = %{public}s", subModelId.c_str());
            break;
        }
        case SLE_HILINK_ICON_ID: {  // 图标
            // IconId广播中是小端，AT中又是大端，这里需要将小端转为大端
            EndianReverse(value);
            std::string iconId = BytesToHexString(value);
            SetIconId(iconId);
            HILOGI("iconId = %{public}s", iconId.c_str());
            break;
        }
        case SLE_HILINK_BLE_ADV_ADDR: {
            std::string btAddr = BytesToHexString(value);
            btAddr = FormatBtAddr(btAddr);
            SetBtAddr(btAddr);
            break;
        }
        default:
            HILOGW("not support hilink adv type=%{public}x.", extendValueType);
            break;
    }
}

void SlePeripheralDevice::SetServiceUUID16Bits(SlePeripheralDeviceParseAdvData &parseAdvData)
{
    NL_CHECK_RETURN(parseAdvData.length % static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_16) == 0,
               "length=%{public}zu is wrong", parseAdvData.length);
    for (size_t var = 0; var < parseAdvData.length / static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_16); ++var) {
        uint16_t uuid16Bits = *reinterpret_cast<uint16_t *>(parseAdvData.payload + var *
            static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_16));
        SetServiceUUID(Uuid::ConvertFrom16Bits(uuid16Bits));
        if (uuid16Bits == static_cast<uint16_t>(SleUuid::SLE_STANDARD_ASC_MGMT_UUID) ||
            uuid16Bits == static_cast<uint16_t>(SleUuid::SLE_STANDARD_ASC_ABLTY_UUID)) {
            SetIsAudioDeviceFlag(true);
        }
    }
}

void SlePeripheralDevice::SetServiceUUID128Bits(const SlePeripheralDeviceParseAdvData &parseAdvData)
{
    for (size_t var = 0; var < parseAdvData.length / static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_128); ++var) {
        std::array<uint8_t, static_cast<int>(UuidLength::SLE_UUID_LEN_128)> data = {};
        for (int i = 0; i < static_cast<int>(UuidLength::SLE_UUID_LEN_128); i++) {
            data[i] = *(parseAdvData.payload + var * static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_128) + i);
        }
        SetServiceUUID(Uuid::ConvertFromBytesSle(data.data()));
    }
}

void SlePeripheralDevice::SetServiceDataUUID16Bits(SlePeripheralDeviceParseAdvData &parseAdvData)
{
    NL_CHECK_RETURN(parseAdvData.length >= static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_16),
               "parseAdvData length=%{public}zu is too short", parseAdvData.length);
    uint16_t uuid = *(reinterpret_cast<uint16_t *>(parseAdvData.payload));
    std::string data = "";
    if (parseAdvData.length > static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_16)) {
        data = std::string(
            reinterpret_cast<char *>(parseAdvData.payload + static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_16)),
            parseAdvData.length - static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_16));
    }
    SetServiceDataUUID(Uuid::ConvertFrom16Bits(uuid), data);
}

void SlePeripheralDevice::SetServiceDataUUID128Bits(SlePeripheralDeviceParseAdvData &parseAdvData)
{
    NL_CHECK_RETURN(parseAdvData.length >= static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_128),
               "parseAdvData length=%{public}zu is too short", parseAdvData.length);
    std::string data = "";
    if (parseAdvData.length > static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_128)) {
        data = std::string(
            reinterpret_cast<char *>(parseAdvData.payload + static_cast<uint8_t>(UuidLength::SLE_UUID_LEN_128)),
            parseAdvData.length - static_cast<std::size_t>(UuidLength::SLE_UUID_LEN_128));
    }
    SetServiceDataUUID(Uuid::ConvertFromBytesSle(parseAdvData.payload), data);
}

/**
 * @brief Set device name.
 *
 * @param name Device name.
 * @since 6
 */
void SlePeripheralDevice::SetName(const std::string &name)
{
    name_ = name;
    isName_ = true;
}

/**
 * @brief Set device roles.
 *
 * @param roles Device roles.
 * @since 6
 */
void SlePeripheralDevice::SetRoles(uint8_t roles)
{
    roles_ = roles;
}

/**
 * @brief Set bonded from local.
 *
 * @param flag Advertiser flag.
 * @since 6
 */
void SlePeripheralDevice::SetBondedFromLocal(bool flag)
{
    bondFlag_ = flag;
}

/**
 * @brief Set acb connection state.
 *
 * @param connectState Acb connection state.
 * @since 6
 */
void SlePeripheralDevice::SetAcbConnectState(int connectState)
{
    acbConnected_ = connectState;
}

/**
 * @brief Get acb connection state.
 *
 * @param connectState Acb connection state.
 * @return Returns Acb Connect state
 * @since 6
 */
int SlePeripheralDevice::GetAcbConnectState() const
{
    return acbConnected_;
}

void SlePeripheralDevice::SetAcbDisConnReason(int reason)
{
    acbDisConnReason_ = reason;
}

int SlePeripheralDevice::GetAcbDisConnReason() const
{
    return acbDisConnReason_;
}

/**
 * @brief Set acb lcid.
 *
 * @param handle Acb lcid.
 * @since 6
 */
void SlePeripheralDevice::SetLcid(uint16_t lcid)
{
    lcid_ = lcid;
}

/**
 * @brief Set acb localIndex.
 *
 * @param handle Acb localIndex.
 * @since 6
 */
void SlePeripheralDevice::SetLocalIndex(uint16_t localIndex)
{
    localIndex_ = localIndex;
}

/**
 * @brief Set acb connect direct.
 *
 * @param direct Acb Connect Direct.
 * @since 6
 */
void SlePeripheralDevice::SetConnDirect(int direct)
{
    acbConnectDirect_ = direct;
}

/**
 * @brief Get acb connect direct.
 *
 * @return Returns Acb Connect Direct
 * @since 6
 */
int SlePeripheralDevice::GetConnDirect() const
{
    return acbConnectDirect_;
}

/**
 * @brief Check if device acb is connected.
 *
 * @return Returns <b>true</b> if device acb is connected;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SlePeripheralDevice::IsAcbConnected() const
{
    return acbConnected_ >= static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED);
}

/**
 * @brief Check if device acb is connecting.
 *
 * @return Returns <b>true</b> if device acb is connecting;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SlePeripheralDevice::IsAcbConnecting() const
{
    return acbConnected_ == static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING);
}


/**
 * @brief Check if device acb Encrypted.
 *
 * @return Returns <b>true</b> if device acb is Encrypted;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SlePeripheralDevice::IsAcbEncrypted() const
{
    return acbConnected_ > static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED);
}

/**
 * @brief Get local link role.
 *
 * @return Returns local link role.
 * @since 6
 */
uint8_t SlePeripheralDevice::GetLinkRole() const
{
    return roles_;
}

/**
 * @brief Check if device was bonded from local.
 *
 * @return Returns <b>true</b> if device was bonded from local;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SlePeripheralDevice::IsBondedFromLocal() const
{
    return bondFlag_;
}

/**
 * @brief Get acb connection handle.
 *
 * @return Returns acb connection handle;
 * @since 6
 */
uint16_t SlePeripheralDevice::GetLcid() const
{
    return lcid_;
}

/**
 * @brief Get acb connection localIndex.
 *
 * @return Returns acb connection localIndex;
 * @since 6
 */
uint16_t SlePeripheralDevice::GetLocalIndex() const
{
    return localIndex_;
}

/**
 * @brief Get advertising flag.
 *
 * @return Returns advertising flag.
 * @since 6
 */
uint8_t SlePeripheralDevice::GetAdFlag() const
{
    return adFlag_;
}

/**
 * @brief Get paired status.
 *
 * @return Returns paired status.
 * @since 6
 */
uint8_t SlePeripheralDevice::GetPairedStatus() const
{
    return pairState_;
}

/**
 * @brief Get pre paired status.
 *
 * @return Returns pre paired status.
 * @since 6
 */
uint8_t SlePeripheralDevice::GetPrePairedStatus() const
{
    return prePairState_;
}

/**
 * @brief Set paired status.
 *
 * @param status Paired status.
 * @return Returns <b>true</b> if the operation is successful;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SlePeripheralDevice::SetPairedStatus(uint8_t status)
{
    NL_CHECK_RETURN_RET(status >= static_cast<int>(SlePairState::SLE_PAIR_NONE) &&
        status <= static_cast<int>(SlePairState::SLE_PAIR_CANCELING), false, "Unavailable PairedStatus");
    pairState_ = status;
    return true;
}

/**
 * @brief Set pre paired status.
 *
 * @param status Paired status.
 * @return Returns <b>true</b> if the operation is successful;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SlePeripheralDevice::SetPrePairedStatus(uint8_t status)
{
    NL_CHECK_RETURN_RET(status >= static_cast<int>(SlePairState::SLE_PAIR_NONE) &&
        status <= static_cast<int>(SlePairState::SLE_PAIR_CANCELING), false, "Unavailable PrePairedStatus");
    prePairState_ = status;
    return true;
}

/**
* @brief Get paired direction.
*
* @return Returns paired direction.
* @since 6
*/
int SlePeripheralDevice::GetPairDirection() const
{
    return pairDirection_;
}

/**
 * @brief Set paired direction.
 *
 * @param direction Paired direction.
 * @return Returns <b>true</b> if the operation is successful;
 *         Returns <b>false</b> otherwise.
 * @since 6
 */
bool SlePeripheralDevice::SetPairDirection(int direction)
{
    NL_CHECK_RETURN_RET(direction >= static_cast<int>(SlePairDirect::SLE_PAIR_DEFAULT) &&
        direction <= static_cast<int>(SlePairDirect::SLE_PAIR_PASSIVE), false, "Unavailable PairDirection");
    pairDirection_ = direction;
    return true;
}

/**
 * @brief Set alias name.
 *
 * @param name Device alias name.
 * @since 6
 */
void SlePeripheralDevice::SetAliasName(const std::string &name)
{
    aliasName_ = name;
}

/**
 * @brief Get alias name.
 *
 * @return Returns alias name.
 * @since 6
 */
std::string SlePeripheralDevice::GetAliasName() const
{
    return aliasName_;
}

/**
 * @brief Set IO capability.
 *
 * @param io IO capability
 * @since 6
 */
void SlePeripheralDevice::SetIoCapability(uint8_t io)
{
    ioCapability_ = io;
}

/**
 * @brief Get IO capability.
 *
 * @return Returns IO capability.
 * @since 6
 */
uint8_t SlePeripheralDevice::GetIoCapability() const
{
    return ioCapability_;
}

/**
 * @brief Set advertising flag.
 *
 * @param adFlag Advertising flag.
 * @since 6
 */
void SlePeripheralDevice::SetAdFlag(uint8_t adFlag)
{
    adFlag_ = adFlag;
}

/**
 * @brief Set device appearance.
 *
 * @param device Appearance.
 * @since 6
 */
void SlePeripheralDevice::SetAppearance(int appearance)
{
    appearance_ = appearance;
    isAppearance_ = true;
}

/**
 * @brief Set manufacturer data.
 *
 * @param manufacturerData Manufacturer data.
 * @since 6
 */
void SlePeripheralDevice::SetManufacturerData(uint16_t manufacturerId, std::string manufacturerData)
{
        isManufacturerData_ = true;
        std::map<uint16_t, std::string>::const_iterator iter = manufacturerData_.find(manufacturerId);
        if (iter == manufacturerData_.cend()) {
            manufacturerData_.insert(std::make_pair(manufacturerId, manufacturerData));
        }
}

/**
 * @brief Set service data UUID.
 *
 * @param uuid Service data UUID.
 * @since 6
 */
void SlePeripheralDevice::SetServiceDataUUID(Uuid uuid, std::string data)
{
    isServiceData_ = true;
    auto iter = std::find(serviceDataUUIDs_.begin(), serviceDataUUIDs_.end(), uuid);
    if (iter == serviceDataUUIDs_.end()) {
        serviceDataUUIDs_.push_back(uuid);
        serviceData_.push_back(data);
    }
}

/**
 * @brief Set service UUID.
 *
 * @param serviceUUID Service UUID.
 * @since 6
 */
void SlePeripheralDevice::SetServiceUUID(Uuid serviceUUID)
{
    NL_CHECK_RETURN(serviceUUIDs_.size() < SERVICE_UUID_MAX_NUM,
               "the number of service uuids reaches the maximum");
    isServiceUUID_ = true;
    auto iter = std::find(serviceUUIDs_.begin(), serviceUUIDs_.end(), serviceUUID);
    if (iter == serviceUUIDs_.end()) {
        serviceUUIDs_.push_back(serviceUUID);
    }
}
/**
 * @brief Set TX power.
 *
 * @param txPower TX power.
 * @since 6
 */
void SlePeripheralDevice::SetTXPower(int8_t txPower)
{
    isTXPower_ = true;
    txPower_ = txPower;
}

void SlePeripheralDevice::SetIsDeviceDisplay(bool isDeviceDisplay)
{
    isDeviceDisplay_ = isDeviceDisplay;
}

void SlePeripheralDevice::SetManufacturerAbility(const std::array<uint8_t, SLE_MANU_ABILITY_LEN> &manufacturerAbility)
{
    manufacturerAbility_ = manufacturerAbility;
}

std::array<uint8_t, SLE_MANU_ABILITY_LEN> SlePeripheralDevice::GetManufacturerAbility() const
{
    return manufacturerAbility_;
}

bool SlePeripheralDevice::GetIsDeviceDisplay() const
{
    return isDeviceDisplay_;
}

void SlePeripheralDevice::SetIsDeviceAvailable(bool isDeviceAvailable)
{
    isDeviceAvailable_ = isDeviceAvailable;
}

bool SlePeripheralDevice::GetIsDeviceAvailable() const
{
    return isDeviceAvailable_;
}

/**
 * @brief Get peripheral device.
 *
 * @return Returns peripheral device const reference
 * @since 6
 */
const SlePeripheralDevice& SleScanResultImpl::GetPeripheralDevice() const
{
    return peripheralDevice_;
}

/**
 * @brief Set peripheral device.
 *
 * @param dev Peripheral device.
 * @since 6
 */
void SleScanResultImpl::SetPeripheralDevice(const SlePeripheralDevice &dev)
{
    peripheralDevice_ = dev;
}

/**
 * @brief Get advertiser data packet.
 *
 * @return Returns advertiser data packet.
 * @since 6
 */
std::string SlePeripheralDevice::GetPayload() const
{
    return payload_;
}

void SlePeripheralDevice::SetPayload(uint8_t *payloadData, uint16_t payloadLenth)
{
    payloadLen_ =  static_cast<size_t>(payloadLenth);
    payload_ = std::string(payloadData, payloadData + payloadLenth);
}

/**
 * @brief Get advertising packet length.
 *
 * @return Returns advertising packet length.
 * @since 6
 */
size_t SlePeripheralDevice::GetPayloadLen() const
{
    return payloadLen_;
}

/**
 * @brief Set cdsm address type.
 *
 * @param cdsmAddrType cdsm address type.
 * @since 6
 */
void SlePeripheralDevice::SetCdsmAddrType(int cdsmAddrType)
{
    cdsmAddrType_ = cdsmAddrType;
}

/**
 * @brief Get cdsm address type.
 *
 * @return Returns cdsm address type.
 * @since 6
 */
int SlePeripheralDevice::GetCdsmAddrType()
{
    return cdsmAddrType_;
}

/**
 * @brief Check is cdsm member device.
 *
 * @return Returns cdsm member flag.
 * @since 6
 */
bool SlePeripheralDevice::IsCdsmMember() const
{
    return (cdsmAddrType_ == static_cast<int>(SleCdsmAddrType::CDSM_TYPE_MEMBER));
}

/**
 * @brief Set bt address.
 *
 * @param string bt address.
 * @since 6
 */
void SlePeripheralDevice::SetBtAddr(const std::string &btAddr)
{
    btAddr_ = btAddr;
}

/**
 * @brief Get Bt Address.
 *
 * @return Returns Bt address.
 * @since 6
 */
std::string SlePeripheralDevice::GetBtAddr() const
{
    return btAddr_;
}

/**
 * @brief Set audio device flag.
 *
 * @param isAudioDevice audio device flag.
 * @since 6
 */
void SlePeripheralDevice::SetIsAudioDeviceFlag(bool isAudioDevice)
{
    HILOGD("%{public}s, isAudioDevice=%{public}d", GET_ENCRYPT_ADDR(address_), isAudioDevice);
    isAudioDevice_ = isAudioDevice;
}

/**
 * @brief Check is Audio Device.
 *
 * @return Returns audio device flag.
 * @since 6
 */
bool SlePeripheralDevice::GetIsAudioDeviceFlag() const
{
    HILOGD("%{public}s, isAudioDevice=%{public}d", GET_ENCRYPT_ADDR(address_), isAudioDevice_);
    return isAudioDevice_;
}
}  // namespace Sle
}  // namespace OHOS
