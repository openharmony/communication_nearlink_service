/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "nearlink_uuid.h"
#include <charconv>
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int UUID_LOW_FIRST_BYTE = 0;
constexpr int UUID_LOW_SECOND_BYTE = 1;
constexpr int UUID_LOW_THIRD_BYTE = 2;
constexpr int UUID_LOW_FOURTH_BYTE = 3;
constexpr int UUID_MID_FIRST_BYTE = 4;
constexpr int UUID_MID_SECOND_BYTE = 5;
constexpr int UUID_VERSION = 6;
constexpr int UUID_HIGH = 7;
constexpr int UUID_VARIANT = 8;
constexpr int UUID_CLOCK_SEQ = 9;
constexpr int UUID_NODE_FIRST_BYTE = 10;
constexpr int UUID_NODE_SECOND_BYTE = 11;
constexpr int UUID_NODE_THIRD_BYTE = 12;
constexpr int UUID_NODE_FOURTH_BYTE = 13;
constexpr int UUID_NODE_FIFTH_BYTE = 14;
constexpr int UUID_NODE_SIXTH_BYTE = 15;
constexpr int BASE_BIT_OPT_SIZE = 8;
constexpr int BIT_OPT_TWO_BYTE = 2;
constexpr int BIT_OPT_THREE_BYTE = 3;
constexpr int BIT_OPT_FOUR_BYTE = 4;
constexpr int BIT_OPT_FIVE_BYTE = 5;
constexpr int BIT_OPT_SIX_BYTE = 6;
constexpr int BIT_OPT_SEVEN_BYTE = 7;

constexpr int SIZE_STRING_TO_INT = 2;
constexpr int DIVISION_STEP_LEN = 2;
constexpr int UUID_SUB_STRING_TYPE = 20;
constexpr int UUID_STRING_SIZE = 40;
}

UUID::UUID(const long mostSigBits, const long leastSigBits)
{
    unsigned long unsignedMostSigBits = static_cast<unsigned long>(mostSigBits);
    unsigned long unsignedLeastSigBits = static_cast<unsigned long>(leastSigBits);
    this->uuid_[UUID_NODE_SIXTH_BYTE] = static_cast<uint8_t>(unsignedLeastSigBits & 0x00000000000000FF);
    this->uuid_[UUID_NODE_FIFTH_BYTE] =
        static_cast<uint8_t>((unsignedLeastSigBits & 0x000000000000FF00) >> BASE_BIT_OPT_SIZE);
    this->uuid_[UUID_NODE_FOURTH_BYTE] =
        static_cast<uint8_t>((unsignedLeastSigBits & 0x0000000000FF0000) >> (BIT_OPT_TWO_BYTE * BASE_BIT_OPT_SIZE));
    this->uuid_[UUID_NODE_THIRD_BYTE] =
        static_cast<uint8_t>((unsignedLeastSigBits & 0x00000000FF000000) >> (BIT_OPT_THREE_BYTE * BASE_BIT_OPT_SIZE));
    this->uuid_[UUID_NODE_SECOND_BYTE] =
        static_cast<uint8_t>((unsignedLeastSigBits & 0x000000FF00000000) >> (BIT_OPT_FOUR_BYTE * BASE_BIT_OPT_SIZE));
    this->uuid_[UUID_NODE_FIRST_BYTE] =
        static_cast<uint8_t>((unsignedLeastSigBits & 0x0000FF0000000000) >> (BIT_OPT_FIVE_BYTE * BASE_BIT_OPT_SIZE));
    this->uuid_[UUID_CLOCK_SEQ] =
        static_cast<uint8_t>((unsignedLeastSigBits & 0x00FF000000000000) >> (BIT_OPT_SIX_BYTE * BASE_BIT_OPT_SIZE));
    this->uuid_[UUID_VARIANT] =
        static_cast<uint8_t>((unsignedLeastSigBits & 0xFF00000000000000) >> (BIT_OPT_SEVEN_BYTE * BASE_BIT_OPT_SIZE));
    this->uuid_[UUID_HIGH] =
        static_cast<uint8_t>(unsignedMostSigBits & 0x00000000000000FF);
    this->uuid_[UUID_VERSION] =
        static_cast<uint8_t>((unsignedMostSigBits & 0x000000000000FF00) >> BASE_BIT_OPT_SIZE);
    this->uuid_[UUID_MID_SECOND_BYTE] =
        static_cast<uint8_t>((unsignedMostSigBits & 0x0000000000FF0000) >> (BIT_OPT_TWO_BYTE * BASE_BIT_OPT_SIZE));
    this->uuid_[UUID_MID_FIRST_BYTE] =
        static_cast<uint8_t>((unsignedMostSigBits & 0x00000000FF000000) >> (BIT_OPT_THREE_BYTE * BASE_BIT_OPT_SIZE));
    this->uuid_[UUID_LOW_FOURTH_BYTE] =
        static_cast<uint8_t>((unsignedMostSigBits & 0x000000FF00000000) >> (BIT_OPT_FOUR_BYTE * BASE_BIT_OPT_SIZE));
    this->uuid_[UUID_LOW_THIRD_BYTE] =
        static_cast<uint8_t>((unsignedMostSigBits & 0x0000FF0000000000) >> (BIT_OPT_FIVE_BYTE * BASE_BIT_OPT_SIZE));
    this->uuid_[UUID_LOW_SECOND_BYTE] =
        static_cast<uint8_t>((unsignedMostSigBits & 0x00FF000000000000) >> (BIT_OPT_SIX_BYTE * BASE_BIT_OPT_SIZE));
    this->uuid_[UUID_LOW_FIRST_BYTE] =
        static_cast<uint8_t>((unsignedMostSigBits & 0xFF00000000000000) >> (BIT_OPT_SEVEN_BYTE * BASE_BIT_OPT_SIZE));
}

UUID::UUID(const uint16_t leastSig16Bits)
{
    this->uuid_[UUID_NODE_SIXTH_BYTE] = static_cast<uint8_t>(leastSig16Bits & 0x00000000000000FF);
    this->uuid_[UUID_NODE_FIFTH_BYTE] =
        static_cast<uint8_t>((leastSig16Bits & 0x000000000000FF00) >> BASE_BIT_OPT_SIZE);
    for (size_t i = 0; i < UUID_NODE_FIFTH_BYTE; i++) {
        this->uuid_[i] = BASE_UUID[i];
    }
}

UUID UUID::FromString(const std::string &name)
{
    UUID ret;
    std::string tmp = name;
    std::size_t pos = tmp.find("-");

    while (pos != std::string::npos) {
        tmp.replace(pos, 1, "");
        pos = tmp.find("-");
    }

    size_t len = tmp.length();
    NL_CHECK_RETURN_RET(len == UUID_16_BYTES_LEN * SIZE_STRING_TO_INT, ret, "len(%{public}zu) is invalid.", len);

    uint8_t tmpUuid = 0;
    for (std::size_t i = 0; (i + 1) < len; i += SIZE_STRING_TO_INT) {
        std::from_chars_result res =
            std::from_chars(tmp.data() + i, tmp.data() + i + SIZE_STRING_TO_INT, tmpUuid, 16); // 16进制
        if (res.ec != std::errc{} || res.ptr != tmp.data() + i + SIZE_STRING_TO_INT) {
            HILOGE("FromString failed, string:%{public}s", tmp.c_str());
            return ret;
        }
        ret.uuid_[i / SIZE_STRING_TO_INT] = tmpUuid;
    }
    return ret;
}

uint16_t UUID::GetLeastSig16Bits() const
{
    return (static_cast<uint16_t>(this->uuid_[UUID_NODE_SIXTH_BYTE]) +
        (static_cast<uint16_t>(uuid_[UUID_NODE_FIFTH_BYTE]) << BASE_BIT_OPT_SIZE));
}

std::string UUID::GetEncryptUuid() const
{
    char tmp[UUID_STRING_SIZE] = {0};
    (void)sprintf_s(tmp, sizeof(tmp), "%02x%02x****-****-****-****-********%02x%02x", uuid_[UUID_LOW_FIRST_BYTE],
        uuid_[UUID_LOW_SECOND_BYTE], uuid_[UUID_NODE_FIFTH_BYTE], uuid_[UUID_NODE_SIXTH_BYTE]);
    return tmp;
}

std::string UUID::ToString() const
{
    std::string tmp = "";
    std::string ret = "";
    static const char *hex = "0123456789ABCDEF";

    for (auto it = this->uuid_.begin(); it != this->uuid_.end(); it++) {
        tmp.push_back(hex[(((*it) >> BIT_OPT_FOUR_BYTE) & 0xF)]);
        tmp.push_back(hex[(*it) & 0xF]);
    }

    ret = tmp.substr(0, UUID_VARIANT) + "-" + tmp.substr(UUID_VARIANT, UUID_MID_FIRST_BYTE) +
        "-" + tmp.substr(UUID_NODE_THIRD_BYTE, UUID_MID_FIRST_BYTE) + "-" +
        tmp.substr(UUID_16_BYTES_LEN, UUID_MID_FIRST_BYTE) + "-" + tmp.substr(UUID_SUB_STRING_TYPE);

    return ret;
}

int UUID::CompareTo(const UUID &val) const
{
    UUID tmp = val;
    return this->ToString().compare(tmp.ToString());
}

bool UUID::Equals(const UUID &val) const
{
    for (int i = 0; i < UUID_16_BYTES_LEN; i++) {
        if (this->uuid_[i] != val.uuid_[i]) {
            return false;
        }
    }
    return true;
}

uint64_t UUID::GetLeastSignificantBits() const
{
    uint64_t leastSigBits = 0;
    for (int i = UUID_16_BYTES_LEN / 2; i < UUID_16_BYTES_LEN; i++) {
        leastSigBits = (leastSigBits << BASE_BIT_OPT_SIZE) | (uuid_[i] & 0xFF);
    }
    return leastSigBits;
}

uint64_t UUID::GetMostSignificantBits() const
{
    uint64_t mostSigBits = 0;
    for (int i = 0 / DIVISION_STEP_LEN; i < UUID_16_BYTES_LEN / DIVISION_STEP_LEN; i++) {
        mostSigBits = (mostSigBits << BASE_BIT_OPT_SIZE) | (uuid_[i] & 0xFF);
    }
    return mostSigBits;
}

UUID UUID::ConvertFrom128Bits(const std::array<uint8_t, UUID_16_BYTES_LEN> &name)
{
    UUID tmp;
    for (int i = 0; i < UUID_16_BYTES_LEN; i++) {
        tmp.uuid_[i] = name[i];
    }
    return tmp;
}

std::array<uint8_t, UUID_16_BYTES_LEN> UUID::ConvertTo128Bits() const
{
    std::array<uint8_t, UUID_16_BYTES_LEN> uuid;
    for (int i = 0; i < UUID_16_BYTES_LEN; i++) {
        uuid[i] = uuid_[i];
    }
    return uuid;
}
}  // namespace Nearlink
}  // namespace OHOS