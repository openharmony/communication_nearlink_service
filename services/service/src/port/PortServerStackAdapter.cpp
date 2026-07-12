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
#include "PortServerStackAdapter.h"
#include "PortDefines.h"
#include "nlstk_port_server.h"
#include "param_wrapper.h"
#include "log_util.h"
#include <charconv>
#include <string>

namespace OHOS {
namespace Nearlink {

namespace {

constexpr unsigned int MANUFACTURE_ID_HEX_BASE = 16;
constexpr size_t MANUFACTURE_ID_HEX_CHARS_PER_BYTE = 2;
constexpr size_t MANUFACTURE_ID_MAX_HEX_LEN = sizeof(uint16_t) * MANUFACTURE_ID_HEX_CHARS_PER_BYTE;

NLSTK_SsapUuid_S ConvertToStackUuid(const Uuid &uuid)
{
    NLSTK_SsapUuid_S sleUuid;
    memset_s(&sleUuid, sizeof(NLSTK_SsapUuid_S), 0, sizeof(NLSTK_SsapUuid_S));
    uuid.ConvertToBytesLE(sleUuid.uuid);
    return sleUuid;
}

}

PortServerStackAdapter::PortServerStackAdapter()
{}

PortServerStackAdapter::~PortServerStackAdapter() = default;

void PortServerStackAdapter::AddPortByUuid(const Uuid::UUID128Bit& uuid, const uint16_t manufactureId,
    const uint16_t portId)
{
    NLSTK_SsapUuid_S stackUuid = ConvertToStackUuid(Uuid::ConvertFrom128Bits(uuid));
    uint32_t ret = NLSTK_PortAddByUuid(&stackUuid, manufactureId, portId);
    NL_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, "[PORT Adapter] cannot add port by uuid, ret(%{public}d)", ret);
    HILOGI("[PORT Adapter] successfully add port by uuid, ret(%{public}d)", ret);
}

void PortServerStackAdapter::DeletePortByUuid(const Uuid::UUID128Bit& uuid, const uint16_t portId)
{
    HILOGI("delete portId(%{public}hu, uuid:%{public}s)",
        portId, Uuid::ConvertFrom128Bits(uuid).GetEncryptUuid().c_str());
    NLSTK_SsapUuid_S stackUuid = ConvertToStackUuid(Uuid::ConvertFrom128Bits(uuid));
    uint32_t ret = NLSTK_PortDeleteByUuid(&stackUuid);
    NL_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, "[PORT Adapter] cannot delete port by uuid, ret(%{public}d)", ret);
    HILOGI("[PORT Adapter] successfully delete port by uuid, ret(%{public}d)", ret);
}

uint16_t PortServerStackAdapter::LoadManufactureInfo() const
{
    // load manufacture ID from system parameter and parse hex string to uint16_t
    // validation is required to prevent UB from untrusted input (empty, non-hex, or overflow)
    std::string manufactureInfo = "";
    int res = OHOS::system::GetStringParameter("const.nearlink.dis.manufacture_id",
        manufactureInfo, "");
    NL_CHECK_RETURN_RET((res == PORT_SUCCESS), res,
        "read manufacture id err, manufactureInfo = %{public}s, res=%{public}d", manufactureInfo.c_str(), res);
    if (manufactureInfo.empty() || manufactureInfo.size() > MANUFACTURE_ID_MAX_HEX_LEN) {
        HILOGE("[PORT Adapter] manufactureInfo invalid length: %{public}s", manufactureInfo.c_str());
        return 0;
    }
    uint16_t manufactureId_ = 0;
    const char *first = manufactureInfo.data();
    const char *last = first + manufactureInfo.size();
    std::from_chars_result parseRes = std::from_chars(first, last, manufactureId_, MANUFACTURE_ID_HEX_BASE);
    if (parseRes.ec != std::errc{} || parseRes.ptr != last) {
        HILOGE("[PORT Adapter] manufactureInfo parse failed: %{public}s", manufactureInfo.c_str());
        return 0;
    }
    HILOGI("[PORT Adapter] manufactureInfo = %{public}s, manufactureId_=%{public}hu",
        manufactureInfo.c_str(), manufactureId_);
    return manufactureId_;
}

} // Nearlink
} // OHOS