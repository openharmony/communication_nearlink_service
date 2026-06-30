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

#include "nlstk_ccp_ccs_server.h"
#include "log.h"

uint32_t NLSTK_CcpCreateCcsInstance(NLSTK_CcpCallControlInfo_S *baseInfo)
{
    HILOGI("[CcpStack Mocker] NLSTK_CcpCreateCcsInstance enter");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_CcpDeleteCcsInstance(int32_t instanceId)
{
    HILOGI("[CcpStack Mocker] NLSTK_CcpDeleteCcsInstance enter");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_CcpUpdateCcsProperty(int32_t instanceId, NLSTK_CcpCcsPropertyType_E property, void *value)
{
    HILOGI("[CcpStack Mocker] NLSTK_CcpUpdateCcsProperty enter");
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_CcpCallControlResult(uint32_t requestId, int32_t instanceId, uint8_t errorCode)
{
    HILOGI("[CcpStack Mocker] NLSTK_CcpCallControlResult enter");
}

uint32_t NLSTK_CcpCcsAuthorizeResult(uint32_t requestId, int32_t instanceId, NLSTK_CcpCcsPropertyType_E property,
    uint8_t errorCode)
{
    HILOGI("[CcpStack Mocker] NLSTK_CcpCcsAuthorizeResult enter");
    return NLSTK_ERRCODE_SUCCESS;
}