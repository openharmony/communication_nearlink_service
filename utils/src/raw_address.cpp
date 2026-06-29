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

#include "raw_address.h"
#include <vector>
#include <cstdlib>
#include "securec.h"
#include "log.h"
#include "string"

namespace OHOS {
namespace Nearlink {
void RawAddress::ConvertToUint8(uint8_t *dst, const size_t size) const
{
    if (dst != nullptr && address_.length() == SLE_ADDRESS_STR_LEN && size >= SLE_ADDRESS_BYTE_LEN) {
        std::vector<std::string> token;
        std::size_t startPostion = 0;
        std::size_t colonPosition = address_.find(':', startPostion);
        while (colonPosition != std::string::npos) {
            token.push_back(address_.substr(startPostion, colonPosition - startPostion));
            startPostion = colonPosition + SLE_COLON_BYTE_SIZE;
            colonPosition = address_.find(':', startPostion);
        }
        if (startPostion != SLE_ADDRESS_STR_LEN) {
            token.push_back(address_.substr(startPostion));
        }
        NL_CHECK_RETURN(token.size() == SLE_ADDRESS_BYTE_LEN, "token length uncorrect");
        const int baseHex = 16; // hex format to transfom.
        for (int i = 0; i < SLE_ADDRESS_BYTE_LEN; ++i) {
            char *end = nullptr;
            const char *ptr = token[i].c_str();
            errno = 0;
            int64_t num = strtol(ptr, &end, baseHex);
            // check the exception of strtol
            NL_CHECK_RETURN(errno != ERANGE && end != ptr && *end == '\0', "strtol failed.");
            dst[i] = static_cast<uint8_t>(num);
        }
    }
}


RawAddress RawAddress::ConvertToString(const uint8_t *src, const size_t size)
{
    char token[SLE_ADDRESS_STR_LEN + 1] = {0};
    if (size >= SLE_ADDRESS_BYTE_LEN) {
        (void)sprintf_s(token,
            SLE_ADDRESS_STR_LEN + 1,
            "%02X:%02X:%02X:%02X:%02X:%02X",
            src[SLE_NAP_LOW_BYTE],
            src[SLE_NAP_HIGH_BYTE],
            src[SLE_UAP_BYTE],
            src[SLE_LAP_LOW_BYTE],
            src[SLE_LAP_MIDDLE_BYTE],
            src[SLE_LAP_HIGH_BYTE]);
    }
    return RawAddress(token);
}

bool RawAddress::operator<(const RawAddress &rhs) const
{
    return (address_.compare(rhs.address_) < 0);
}

bool RawAddress::operator==(const RawAddress &rhs) const
{
    return (address_.compare(rhs.address_) == 0);
}

bool RawAddress::operator!=(const RawAddress &rhs) const
{
    return (address_.compare(rhs.address_) != 0);
}

}  // namespace Sle
}  // namespace OHOS