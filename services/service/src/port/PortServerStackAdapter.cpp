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
#include <string>
#include <sstream>

namespace OHOS {
namespace Nearlink {

namespace {

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
    std::string manufactureInfo = "";
    int res = OHOS::system::GetStringParameter("const.nearlink.dis.manufacture_id",
        manufactureInfo, "");
    NL_CHECK_RETURN_RET((res == PORT_SUCCESS), res,
        "read manufacture id err, manufactureInfo = %{public}s, res=%{public}d", manufactureInfo.c_str(), res);
    std::stringstream ss;
    uint16_t manufactureId_;
    ss << std::hex << manufactureInfo;
    ss >> manufactureId_;
    HILOGI("[PORT Adapter] manufactureInfo = %{public}s, manufactureId_=%{public}hu",
        manufactureInfo.c_str(), manufactureId_);
    return manufactureId_;
}

} // Nearlink
} // OHOS