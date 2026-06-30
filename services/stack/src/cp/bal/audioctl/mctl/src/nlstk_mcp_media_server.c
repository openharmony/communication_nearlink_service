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
#include "sdf_mem.h"
#include "sdf_string.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "mcp_type.h"
#include "mcp_utils.h"
#include "mcp_media_server.h"
#include "nlstk_mcp_media_server.h"

#define MCP_WAIT_TIME_OUT 1000

uint32_t NLSTK_McpCreateMediaInstance(NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_CHECK_RETURN(basicInfo != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] basicInfo is null");
    NLSTK_CHECK_RETURN(basicInfo->type == NLSTK_MCP_COMMON_SERVICE, NLSTK_ERRCODE_UNSUPPORTED, "[MCP] type unsupport");
    NLSTK_CHECK_RETURN(basicInfo->mediaInstanceId != 0, NLSTK_ERRCODE_PARAM_ERR, "[MCP] instance id illegal");
    NLSTK_McpMediaInfo_S *param = (NLSTK_McpMediaInfo_S *)SDF_MemZalloc(sizeof(NLSTK_McpMediaInfo_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] param malloc fail");
    if (McpCopyMediaInfo(param, basicInfo) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] copy media info failed");
        McpFreeMediaInfo(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    if (SchedulePostTask(McpCreateMediaInstance, param, McpFreeMediaInfo) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpPlayControlResult(uint16_t requestId, int32_t instanceId, uint8_t errorCode)
{
    McpPlayControlResultParam_S *param =
        (McpPlayControlResultParam_S *)SDF_MemZalloc(sizeof(McpPlayControlResultParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] param malloc fail");
    param->requestId = requestId;
    param->instanceId = instanceId;
    param->errorCode = errorCode;
    if (SchedulePostTask(McpPlayControlResult, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpMediaAuthorizeResult(uint16_t requestId, int32_t instanceId, NLSTK_McpPropertyType_E property,
                                     uint8_t errorCode)
{
    McpMediaAuthorizeResultParam_S *param =
        (McpMediaAuthorizeResultParam_S *)SDF_MemZalloc(sizeof(McpMediaAuthorizeResultParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] param malloc fail");
    param->requestId = requestId;
    param->instanceId = instanceId;
    param->type = property;
    param->errorCode = errorCode;
    if (SchedulePostTask(McpMediaAuthorizeResult, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpUpdateMediaProperty(int32_t instanceId, NLSTK_McpPropertyType_E property, void *value)
{
    NLSTK_CHECK_RETURN(value != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] value is null");
    McpUpdateMediaPropertyParam_S *param =
        (McpUpdateMediaPropertyParam_S *)SDF_MemZalloc(sizeof(McpUpdateMediaPropertyParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] param malloc fail");
    param->instanceId = instanceId;
    param->type = property;
    param->value = McpMediaValueConvert(value, property);
    if (param->value == NULL) {
        NLSTK_LOG_ERROR("[MCP] param value malloc fail");
        SDF_MemFree(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    NLSTK_LOG_INFO("[MCP] update property value : %s", SDF_GET_UINT8_STR(param->value->data, param->value->len));
    if (SchedulePostTask(McpUpdateMediaProperty, param, McpFreeUpdatePropertyParam) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_McpDeleteMediaInstance(int32_t instanceId)
{
    int32_t *param = (int32_t *)SDF_MemZalloc(sizeof(int32_t));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] param malloc fail");
    *param = instanceId;
    if (SchedulePostTask(McpDeleteMediaInstance, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[MCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}