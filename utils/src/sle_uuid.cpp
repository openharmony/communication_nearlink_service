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

#include "sle_uuid.h"

#include <sys/time.h>
#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <cstdlib>
#include <ctime>
#include "string"
#include "sys/time.h"
#include "array"
#include "log.h"
#include "securec.h"

namespace OHOS {
namespace Nearlink {

Uuid Uuid::ConvertFromString(const std::string &name)
{
    Uuid ret;
    std::string tmp = name;
    std::size_t pos = tmp.find("-");

    while (pos != std::string::npos) {
        tmp.replace(pos, 1, "");
        pos = tmp.find("-");
    }

    size_t len = tmp.length();
    NL_CHECK_RETURN_RET(len == UUID128_BYTES_TYPE * SIZE_STRING_TO_INT, ret, "len(%{public}zu) is invalid.", len);

    uint8_t tmpUuid = 0;
    for (std::size_t i = 0; (i + 1) < len; i += SIZE_STRING_TO_INT) {
        std::from_chars_result res =
            std::from_chars(tmp.data() + i, tmp.data() + i + SIZE_STRING_TO_INT, tmpUuid, 16); // 16进制
        NL_CHECK_RETURN_RET(res.ec == std::errc{} && res.ptr == tmp.data() + i + SIZE_STRING_TO_INT,
            ret, "ConvertFromString failed, string:%{public}s", tmp.c_str());
        ret.uuid_[i / SIZE_STRING_TO_INT] = tmpUuid;
    }
    return ret;
}

Uuid Uuid::ConvertFrom16Bits(uint16_t uuid)
{
    Uuid tmp;
    tmp.uuid_[UUID_NODE_FIFTH_BYTE] = static_cast<uint8_t>((uuid & 0xFF00) >> BASE_BIT_OPT_SIZE);
    tmp.uuid_[UUID_NODE_SIXTH_BYTE] = static_cast<uint8_t>(uuid & 0x00FF);
    return tmp;
}

Uuid Uuid::ConvertFromBytesSle(const uint8_t *uuid, const size_t size)
{
    Uuid leUuid;
    if (size < UUID128_BYTES_TYPE) {
        return leUuid;
    }
    UUID128Bit le;
    if (memcpy_s(le.data(), UUID128_BYTES_TYPE, uuid, UUID128_BYTES_TYPE) != EOK) {
        return leUuid;
    }
    std::copy(le.data(), le.data() + UUID128_BYTES_TYPE, leUuid.uuid_.begin());
    return leUuid;
}

Uuid Uuid::ConvertFromMostAndLeastBit(uint64_t mostSigBits, uint64_t leastSigBits)
{
    Uuid tmp;
    int division = UUID128_BYTES_TYPE / SIZE_STRING_TO_INT;

    for (int i = 0; i < division; i++) {
        tmp.uuid_[i] = (mostSigBits >> (BASE_BIT_OPT_SIZE * (division - i - 1))) & 0xFF;
    }

    for (int i = division; i < UUID128_BYTES_TYPE; i++) {
        tmp.uuid_[i] = (leastSigBits >> (BASE_BIT_OPT_SIZE * (BIT_OPT_TWO_BYTE*division - i - 1))) & 0xFF;
    }

    return tmp;
}

Uuid Uuid::ConvertFrom128Bits(const UUID128Bit &uuid)
{
    Uuid tmp;
    tmp.uuid_ = uuid;
    return tmp;
}

uint16_t Uuid::ConvertTo16Bits() const
{
    uint16_t ret = uuid_[UUID_NODE_SIXTH_BYTE] & 0xFF;
    ret += (uuid_[UUID_NODE_FIFTH_BYTE] << BASE_BIT_OPT_SIZE);
    return ret;
}

bool Uuid::ConvertToBytesLE(uint8_t *value, const size_t size) const
{
    NL_CHECK_RETURN_RET(size >= UUID128_BYTES_TYPE, false, "Value is out of size.");

    UUID128Bit le;
    std::copy(uuid_.data(), uuid_.data() + UUID128_BYTES_TYPE, le.begin());
    NL_CHECK_RETURN_RET(memcpy_s(value, UUID128_BYTES_TYPE, le.data(), UUID128_BYTES_TYPE) == EOK, false,
                        "memcpy_s failed");
    return true;
}

Uuid::UUID128Bit Uuid::ConvertTo128Bits() const
{
    return uuid_;
}

std::string Uuid::GetEncryptUuid() const
{
    char tmp[UUID_STRING_SIZE] = {0};
    (void)sprintf_s(tmp, sizeof(tmp), "%02x%02x****-****-****-****-********%02x%02x", uuid_[UUID_TIME_LOW_FIRST_BYTE],
        uuid_[UUID_TIME_LOW_SECOND_BYTE], uuid_[UUID_NODE_FIFTH_BYTE], uuid_[UUID_NODE_SIXTH_BYTE]);
    return tmp;
}

std::string Uuid::ConvertToString() const
{
    char tmp[UUID_STRING_SIZE];
    (void)sprintf_s(tmp, sizeof(tmp), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid_[UUID_TIME_LOW_FIRST_BYTE], uuid_[UUID_TIME_LOW_SECOND_BYTE], uuid_[UUID_TIME_LOW_THIRD_BYTE],
        uuid_[UUID_TIME_LOW_FOURTH_BYTE], uuid_[UUID_TIME_MID_FIRST_BYTE], uuid_[UUID_TIME_MID_SECOND_BYTE],
        uuid_[UUID_VERSION], uuid_[UUID_TIME_HIGH], uuid_[UUID_VARIANT], uuid_[UUID_CLOCK_SEQ],
        uuid_[UUID_NODE_FIRST_BYTE], uuid_[UUID_NODE_SECOND_BYTE], uuid_[UUID_NODE_THIRD_BYTE],
        uuid_[UUID_NODE_FOURTH_BYTE], uuid_[UUID_NODE_FIFTH_BYTE], uuid_[UUID_NODE_SIXTH_BYTE]);
    return tmp;
}

int Uuid::GetUuidType() const
{
    for (int i = 0; i < (UUID128_BYTES_TYPE - UUID16_BYTES_TYPE); i++) {
        if (BASE_UUID[i] != uuid_[i]) {
            return UUID128_BYTES_TYPE;
        }
    }
    return UUID16_BYTES_TYPE;
}

bool Uuid::operator==(const Uuid &rhs) const
{
    return (uuid_ == rhs.uuid_);
}

bool Uuid::operator!=(const Uuid &rhs) const
{
    return (uuid_ != rhs.uuid_);
}
}  // namespace Nearlink
}  // namespace OHOS
