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
#include "nlstk_dis_server.h"
#include "nlstk_log.h"
#include "nlstk_dis_def.h"
#include "nlstk_ssap_app_server.h"
#include "dis_server.h"
#include "nlstk_schedule.h"

static void DisFreeVariableLengthValue(void *arg)
{
    NLSTK_VariableData_S *info = (NLSTK_VariableData_S *)arg;
    if (info->data != NULL) {
        SDF_MemFree(info->data);
        info->data = NULL;
    }
    SDF_MemFree(info);
}

NLSTK_Errcode_E NLSTK_DisCreateInstance(NLSTK_DeviceInfo_S* devInfo)
{
    NLSTK_CHECK_RETURN(devInfo != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DIS] Invalid device info");
    NLSTK_DeviceInfo_S *inDevInfo = (NLSTK_DeviceInfo_S *)SDF_MemZalloc(sizeof(NLSTK_DeviceInfo_S));
    NLSTK_CHECK_RETURN(inDevInfo != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DIS] Mem alloc failed");
    DisDeepCopyDevInfo(inDevInfo, devInfo);
    if (SchedulePostTask(DisSaveDevInfoTask, inDevInfo, DisFreeDeviceInfoTask) != 0) {
        NLSTK_LOG_ERROR("[DIS] SchedulPostTask failed in NLSTK_DisCreateInstance");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DisUpdateLocalDeviceName(NLSTK_VariableData_S* name)
{
    NLSTK_CHECK_RETURN(name != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DIS] Invalid device info");
    NLSTK_VariableData_S *inName = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(inName != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DIS] Mem alloc failed");
    (void)memcpy_s(inName, sizeof(NLSTK_VariableData_S), name, sizeof(NLSTK_VariableData_S));
    inName->data = (uint8_t *)SDF_MemZalloc(name->len);
    if (inName->data == NULL) {
        NLSTK_LOG_ERROR("[DIS] Mem alloc failed");
        SDF_MemFree(inName);
        return NLSTK_ERRCODE_FAIL;
    }
    (void)memcpy_s(inName->data, name->len, name->data, name->len);
    if (SchedulePostTask(DisUpdateDevNameTask, inName, DisFreeVariableLengthValue) != 0) {
        NLSTK_LOG_ERROR("[DIS] SchedulPostTask failed in NLSTK_DisUpdateLocalDeviceName");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DisDestroyInstance(void)
{
    if (SchedulePostTask(DisDestroyInstTask, NULL, NULL) != 0) {
        NLSTK_LOG_ERROR("[DIS] SchedulPostTask failed in NLSTK_DisDestroyInstance");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}