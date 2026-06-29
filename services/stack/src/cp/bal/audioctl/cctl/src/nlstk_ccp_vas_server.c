/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <securec.h>
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "ccp_type.h"
#include "ccp_vas_server.h"
#include "nlstk_ccp_vas_server.h"

uint32_t NLSTK_CcpCreateVoiceAssistantService(NLSTK_CcpVasInfo_S *vasInfo)
{
    NLSTK_CHECK_RETURN(vasInfo != NULL, NLSTK_ERRCODE_POINTER_NULL, "[CCP] vasInfo is null");
    NLSTK_CcpVasInfo_S *tmpInfo = (NLSTK_CcpVasInfo_S *)SDF_MemZalloc(sizeof(NLSTK_CcpVasInfo_S));
    NLSTK_CHECK_RETURN(tmpInfo != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] tmpInfo malloc fail");
    (void)memcpy_s(tmpInfo, sizeof(NLSTK_CcpVasInfo_S), vasInfo, sizeof(NLSTK_CcpVasInfo_S));
    if (SchedulePostTask(CcpCreateVoiceAssistantService, tmpInfo, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[CCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_CcpVasControlResult(uint32_t requestId, uint8_t opCode, uint8_t errorCode)
{
    CcpVasControlResultParam_S *param = (CcpVasControlResultParam_S *)SDF_MemZalloc(sizeof(CcpVasControlResultParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] param malloc fail");
    param->requestId = requestId;
    param->opCode = opCode;
    param->errorCode = errorCode;
    if (SchedulePostTask(CcpVasControlResult, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[CCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_CcpVasStateAuthorizeResult(uint32_t requestId, uint8_t errorCode)
{
    CcpVasAuthorizeResultParam_S *param =
        (CcpVasAuthorizeResultParam_S *)SDF_MemZalloc(sizeof(CcpVasAuthorizeResultParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] param malloc fail");
    param->requestId = requestId;
    param->errorCode = errorCode;
    if (SchedulePostTask(CcpVasStateAuthorizeResult, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[CCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_CcpUpdateVasState(uint8_t state)
{
    uint8_t *tmpState = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    NLSTK_CHECK_RETURN(tmpState != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] tmpState malloc fail");
    *tmpState = state;
    if (SchedulePostTask(CcpUpdateVasState, tmpState, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[CCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_CcpDeleteVoiceAssistantService(void)
{
    if (SchedulePostTask(CcpDeleteVoiceAssistantService, NULL, NULL) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[CCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}