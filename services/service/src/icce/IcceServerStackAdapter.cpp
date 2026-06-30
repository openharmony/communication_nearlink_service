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
#include "IcceDefines.h"
#include "log.h"
#include "nlstk_icce_server.h"
#include "IcceServerStackAdapter.h"

namespace OHOS {
namespace Nearlink {

constexpr const uint16_t CAR_PORT = 0xA000;

int IcceServerStackAdapter::CreateServerInfo()
{
    NLSTK_IcceServiceInfo_S svs;
    svs.iccePort = CAR_PORT;
    svs.iccePortRight = OperationIndication::OPERATION_READ |
        OperationIndication::OPERATION_NOTIFY | OperationIndication::OPERATION_INDICATION |
        OperationIndication::OPERATION_BROADCAST | OperationIndication::OPERATION_WRITE_NO_RESPONSE |
        OperationIndication::OPERATION_WRITE_WITH_RESPONSE;
    uint32_t ret = NLSTK_IcceCreateIcceInstance(&svs);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[IcceServer] Create Server Info failed, ret = %{public}d ", ret);
        return ICCE_FAILURE;
    }
    return ICCE_SUCCESS;
}

int IcceServerStackAdapter::DestroyServerInfo()
{
    uint32_t ret = NLSTK_IcceDestroyIcceInstance();
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        HILOGE("[IcceServer] Destroy Server Info failed, ret = %{public}d ", ret);
        return ICCE_FAILURE;
    }
    return ICCE_SUCCESS;
}

} // namespace Nearlink
} // namespace OHOS