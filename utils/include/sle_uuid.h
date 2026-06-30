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

#ifndef SLE_UUID_H
#define SLE_UUID_H

#include <cstddef>
#include <cstdint>
#include <array>
#include <string>
#include "cstdint"
#include "iosfwd"

/**
 * @brief The nearlink subsystem.
 */
namespace OHOS {
namespace Nearlink {
/**
 * @brief This class provides service uuid.
 *
 * @since 6
 */

/* uuid def */
enum class SleUuid : uint16_t {
    SLE_STANDARD_SERVICE_UUID = 0x0600,
    UUID_DEVICE_INFORMATION_SERVICE = 0x0609,
    UUID_DEVICE_INFORMATION_SERVICE_PEN = 0x0906,
    UUID_BATTERY_SERVICE = 0x060A,
    UUID_BATTERY_SERVICE_PEN = 0x0A06,
    SLE_STANDARD_SERVICE_HID_UUID = 0x060B,
    SLE_STANDARD_SERVICE_HID_UUID_PEN = 0x0B06,
    SLE_STANDARD_SERVICE_ICCE_UUID = 0x060D,
    UUID_PORT_PROFILE_SERVICE = 0x0660,
    SLE_STANDARD_SERVICE_CDSM_UUID = 0x0600,
    SLE_STANDARD_SERVICE_MCP_UUID = 0x0614,     /* 通用媒体控制服务 */
    SLE_STANDARD_ASC_MGMT_UUID = 0x0605,
    SLE_STANDARD_ASC_ABLTY_UUID = 0x0606,
    SLE_STANDARD_SERVICE_VCP_UUID = 0x0616,
    SLE_STANDARD_SERVICE_CCP_UUID = 0x0612,     /* 通用通话控制服务 */
    SLE_STANDARD_SERVICE_MIC_UUID = 0x0619,     /* 音频采集开关控制服务 */
};

class Uuid {
public:
    // 128 bits uuid length type
    constexpr static int UUID128_BYTES_TYPE = 16;
    // 16 bits uuid length
    constexpr static int UUID16_BYTES_TYPE = 2;
    using UUID128Bit = std::array<uint8_t, UUID128_BYTES_TYPE>;

    constexpr static int UUID_TIME_LOW_FIRST_BYTE = 0;
    constexpr static int UUID_TIME_LOW_SECOND_BYTE = 1;
    constexpr static int UUID_TIME_LOW_THIRD_BYTE = 2;
    constexpr static int UUID_TIME_LOW_FOURTH_BYTE = 3;
    constexpr static int UUID_TIME_MID_FIRST_BYTE = 4;
    constexpr static int UUID_TIME_MID_SECOND_BYTE = 5;
    constexpr static int UUID_VERSION = 6;
    constexpr static int UUID_TIME_HIGH = 7;
    constexpr static int UUID_VARIANT = 8;
    constexpr static int UUID_CLOCK_SEQ = 9;
    constexpr static int UUID_NODE_FIRST_BYTE = 10;
    constexpr static int UUID_NODE_SECOND_BYTE = 11;
    constexpr static int UUID_NODE_THIRD_BYTE = 12;
    constexpr static int UUID_NODE_FOURTH_BYTE = 13;
    constexpr static int UUID_NODE_FIFTH_BYTE = 14;
    constexpr static int UUID_NODE_SIXTH_BYTE = 15;

    constexpr static int BASE_BIT_OPT_SIZE = 8;
    constexpr static int BIT_OPT_TWO_BYTE = 2;
    constexpr static int BIT_OPT_THREE_BYTE = 3;
    constexpr static int BIT_OPT_FOUR_BYTE = 4;
    constexpr static int BIT_OPT_FIVE_BYTE = 5;
    constexpr static int BIT_OPT_SIX_BYTE = 6;
    constexpr static int BIT_OPT_SEVEN_BYTE = 7;

    constexpr static int SIZE_STRING_TO_INT = 2;

    constexpr static int UUID_STRING_SIZE = 40;

    constexpr static int BYTE_SIZE = 8;
    constexpr static int BINARY_SIZE = 2;
    /**
     * @brief A constructor used to create an <b>UUID</b> instance.
     *
     * @since 6
     */
    Uuid() = default;

    /**
     * @brief A constructor used to create an <b>UUID</b> instance.
     *
     * @param other Other uuid to create an <b>UUID</b> instance.
     * @since 6
     */
    Uuid(const Uuid &other) = default;

    /**
     * @brief The assignment constructor.
     *
     * @param other Other uuid object.
     * @return Returns the reference of Uuid.
     * @since 6
     */
    Uuid &operator=(const Uuid &other) = default;

    /**
     * @brief A destructor used to delete the <b>UUID</b> instance.
     *
     * @since 6
     */
    ~Uuid() = default;

    /**
     * @brief Create a random uuid.
     *
     * @return @c Uuid : The function return a random uuid.
     */
    static Uuid Random();

    /**
     * @brief Constructor a new Uuid from string.
     *
     * @param name The value of string to create Uuid.
     *      for example : "00000000-0000-1000-8000-00805F9B34FB"
     * @return Returns a specified Uuid.
     * @since 6
     */
    static Uuid ConvertFromString(const std::string &name);

    /**
     * @brief Create a new uuid from uint16_t.
     *
     * @param uuid The 16 bits is a part of 128 bits.
     * @return Returns a new uuid.
     * @since 6
     */
    static Uuid ConvertFrom16Bits(uint16_t uuid);

    /**
     * @brief Create a new uuid from little endian bytes.
     *
     * @param uuid The 128 bits value for a uuid.
     * @return Returns a new uuid.
     * @since 6
     */
    static Uuid ConvertFromBytesSle(const uint8_t *uuid, const size_t size = 16);

    /**
     * @brief Create a new uuid by most significant 64 bits and least signigicant 64 bits.
     *
     * @param uuid The 128 bits value for a uuid.
     * @return Returns a new uuid.
     * @since 6
     */
    static Uuid ConvertFromMostAndLeastBit(uint64_t mostSigBits, uint64_t leastSigBits);

    /**
     * @brief Create a new uuid from uint8_t array.
     *
     * @param uuid The 128 bits value for a uuid.
     * @return Returns a uuid from uint8_t array.
     * @since 6
     */
    static Uuid ConvertFrom128Bits(const UUID128Bit &uuid);

    /**
     * @brief Convert uuid to 16 bits.
     *
     * @return Returns a uint16_t value from uuid.
     * @since 6
     */
    uint16_t ConvertTo16Bits() const;

    /**
     * @brief Convert uuid to uint8_t* with little endian.
     *
     * @param[in] value : The 128 bits value for a uuid.
     * @return Returns <b>true</b> if the operation is successful;
     *         returns <b>false</b> if the operation fails.
     * @since 6
     */
    bool ConvertToBytesLE(uint8_t *value, const size_t size = 16) const;

    /**
     * @brief Convert uuid to uint8_t array.
     *
     * @return Returns a new uuid.
     * @since 6
     */
    UUID128Bit ConvertTo128Bits() const;

    /**
     * @brief For print uuid log.
     *
     * @return Returns a encrypt uuid string.
     * @since 6
     */
    std::string GetEncryptUuid() const;

    std::string ConvertToString() const;

    /**
     * @brief Get UUID type: 16 bits or 128 bits
     *
     * @return Returns uuid type.
     *         UUID128_BYTES_TYPE : 128 bits uuid.
     *         UUID16_BYTES_TYPE : 16 bits uuid.
     * @since 6
     */
    int GetUuidType() const;

    uint64_t GetLeastSignificantBits() const
    {
        uint64_t leastSigBits = 0;
        for (int i = UUID128_BYTES_TYPE / 2; i < UUID128_BYTES_TYPE; i++) {
            leastSigBits = (leastSigBits << BYTE_SIZE) | (uuid_[i] & 0xFF);
        }
        return leastSigBits;
    }

    /**
     * @brief Returns the most significant 64 bits of this UUID's 128 bit value.
     *
     * @return Returns the most significant 64 bits of this UUID's 128 bit value.
     * @since 6
     */
    uint64_t GetMostSignificantBits() const
    {
        uint64_t mostSigBits = 0;
        for (int i = 0 / BINARY_SIZE; i < UUID128_BYTES_TYPE / BINARY_SIZE; i++) {
            mostSigBits = (mostSigBits << BYTE_SIZE) | (uuid_[i] & 0xFF);
        }
        return mostSigBits;
    }

    /**
     * @brief Compare two uuid whether are same or not.
     *
     * @param rhs Compared UUID instance.
     * @return Returns <b>true</b> if this UUID is the same as compared UUID;
     *         returns <b>false</b> if this UUID is not the same as compared UUID.
     * @since 6
     */
    bool operator==(const Uuid &rhs) const;

    /**
     * @brief Compare two uuid whether are different or not.
     *
     * @param rhs Compared UUID instance.
     * @return Returns <b>true</b> if this UUID is different as compared UUID;
     *         returns <b>false</b> if this UUID is not different as compared UUID.
     * @since 6
     */
    bool operator!=(const Uuid &rhs) const;

    /**
     * @brief In order to use the object key in the map object, overload the operator <.
     * @param[in] uuid : Uuid object.
     * @return @c bool : If the object uuid is the same, return true, otherwise return false.
     */
    bool operator<(const Uuid &uuid) const
    {
        return *this != uuid;
    }

    /**
     * @brief Convert UUID to string.
     *
     * @return Returns a String object representing this UUID.
     * @since 6
     */
    std::string ToString() const
    {
        std::string tmp = "";
        std::string ret = "";
        const std::string hex = "0123456789ABCDEF";
        const uint8_t size4 = 4;
        const uint8_t pos8 = 8;
        const uint8_t pos12 = 12;
        const uint8_t pos16 = 16;
        const uint8_t pos20 = 20;

        for (auto it = this->uuid_.begin(); it != this->uuid_.end(); ++it) {
            tmp.push_back(hex[(((*it) >> size4) & 0xF)]);
            tmp.push_back(hex[(*it) & 0xF]);
        }
        // 00000000-0000-1000-8000-00805F9B34FB
        ret = tmp.substr(0, pos8) + "-" + tmp.substr(pos8, size4) + "-" + tmp.substr(pos12, size4) + "-" +
            tmp.substr(pos16, size4) + "-" + tmp.substr(pos20);

        return ret;
    }

protected:
    /**
     * @brief Constructor.
     */
    explicit Uuid(const UUID128Bit uuid) : uuid_(uuid) {};

    // base uuid value
    std::array<uint8_t, UUID128_BYTES_TYPE> BASE_UUID = {
        0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
        0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    std::array<uint8_t, UUID128_BYTES_TYPE> uuid_ = BASE_UUID;
};
}  // namespace Nearlink
}  // namespace OHOS

#endif  // SLE_UUID_H