/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "nlstk_mcp_media_server.h"
#include "log.h"

uint32_t NLSTK_McpCreateMediaInstance(NLSTK_McpMediaInfo_S *basicInfo)
{
    HILOGI("[McpStack Mocker] NLSTK_McpCreateMediaInstance enter");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpPlayControlResult(uint16_t requestId, int32_t instanceId, uint8_t errorCode)
{
    HILOGI("[McpStack Mocker] NLSTK_McpPlayControlResult enter");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpMediaAuthorizeResult(uint16_t requestId, int32_t instanceId, NLSTK_McpPropertyType_E property,
    uint8_t errorCode)
{
    HILOGI("[McpStack Mocker] NLSTK_McpMediaAuthorizeResult enter");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpUpdateMediaProperty(int32_t instanceId, NLSTK_McpPropertyType_E property, void *value)
{
    HILOGI("[McpStack Mocker] NLSTK_McpUpdateMediaProperty enter");
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpDeleteMediaInstance(int32_t instanceId)
{
    HILOGI("[McpStack Mocker] NLSTK_McpDeleteMediaInstance enter");
    return NLSTK_ERRCODE_SUCCESS;
}