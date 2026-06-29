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
#include "sdf_mem.h"
#include "sdf_string.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "ccp_type.h"
#include "ccp_utils.h"
#include "ccp_ccs_server.h"
#include "nlstk_ccp_ccs_server.h"

#define CCP_WAIT_TIME_OUT 1000

uint32_t NLSTK_CcpCreateCcsInstance(NLSTK_CcpCallControlInfo_S *baseInfo)
{
    NLSTK_CHECK_RETURN(baseInfo != NULL, NLSTK_ERRCODE_POINTER_NULL, "[CCP] baseInfo is null");
    NLSTK_CHECK_RETURN(baseInfo->type == NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE,
        NLSTK_ERRCODE_UNSUPPORTED, "[CCP] service type unsupport");
    NLSTK_CHECK_RETURN(baseInfo->mediaInstanceId != 0, NLSTK_ERRCODE_PARAM_ERR, "[CCP] instance id illegal");
    NLSTK_CcpCallControlInfo_S *param = (NLSTK_CcpCallControlInfo_S *)SDF_MemZalloc(sizeof(NLSTK_CcpCallControlInfo_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] param malloc fail");
    // CcpCopyCallControlInfo 此函数在拷贝副本时做了原始数据len!=0 => data!=NULL的判断，故post_task后不需要再对data做非空判断
    if (CcpCopyCallControlInfo(param, baseInfo) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] copy call control info failed");
        CcpFreeCallControlInfo(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    if (SchedulePostTask(CcpCreateCcsInstance, param, CcpFreeCallControlInfo) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[CCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_CcpCallControlResult(uint32_t requestId, int32_t instanceId, uint8_t errorCode)
{
    CcpCallControlResultParam_S *param =
        (CcpCallControlResultParam_S *)SDF_MemZalloc(sizeof(CcpCallControlResultParam_S));
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[CCP] param malloc fail");
    param->requestId = requestId;
    param->instanceId = instanceId;
    param->errorCode = errorCode;
    if (SchedulePostTask(CcpCallControlResult, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[CCP] SchedulePostTask failed");
    }
}

uint32_t NLSTK_CcpCcsAuthorizeResult(uint32_t requestId, int32_t instanceId, NLSTK_CcpCcsPropertyType_E property,
    uint8_t errorCode)
{
    CcpCcsAuthorizeResultParam_S *param =
        (CcpCcsAuthorizeResultParam_S *)SDF_MemZalloc(sizeof(CcpCcsAuthorizeResultParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] param malloc fail");
    param->requestId = requestId;
    param->instanceId = instanceId;
    param->type = property;
    param->errorCode = errorCode;
    if (SchedulePostTask(CcpCcsAuthorizeResult, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[CCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_CcpUpdateCcsProperty(int32_t instanceId, NLSTK_CcpCcsPropertyType_E property, void *value)
{
    NLSTK_CHECK_RETURN(value != NULL, NLSTK_ERRCODE_POINTER_NULL, "[CCP] value is null");
    CcpUpdateCcsPropertyParam_S *param =
        (CcpUpdateCcsPropertyParam_S *)SDF_MemZalloc(sizeof(CcpUpdateCcsPropertyParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] param malloc fail");
    param->instanceId = instanceId;
    param->type = property;
    param->value = CcpCcsPropertyValueConvert(value, property);
    if (param->value == NULL) {
        NLSTK_LOG_ERROR("[CCP] param value malloc fail");
        SDF_MemFree(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    NLSTK_LOG_INFO("[CCP] update property value : %s", SDF_GET_UINT8_STR(param->value->data, param->value->len));
    if (SchedulePostTask(CcpUpdateCcsProperty, param, CcpFreeUpdateCcsPropertyParam) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[CCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t NLSTK_CcpDeleteCcsInstance(int32_t instanceId)
{
    int32_t *param = (int32_t *)SDF_MemZalloc(sizeof(int32_t));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] param malloc fail");
    *param = instanceId;
    if (SchedulePostTask(CcpDeleteCcsInstance, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("[CCP] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}