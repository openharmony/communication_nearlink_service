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

#include "securec.h"
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "devd_scan_type.h"
#include "devd_scan_util.h"
#include "devd_scan_api.h"
#include "nlstk_scan_api.h"

NLSTK_Errcode_E NLSTK_DevdRegScanModule(uint8_t *moduleId, NLSTK_DevdScanCbk_S *scanCbk)
{
    NLSTK_CHECK_RETURN(moduleId && scanCbk, NLSTK_ERRCODE_POINTER_NULL, "[DEVDS] moduleId or scanCbk is null");
    *moduleId = DEVD_MAX_MODULE_ID;
    DevdScanModuleParam_S *param = (DevdScanModuleParam_S *)SDF_MemZalloc(sizeof(DevdScanModuleParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "param malloc fail");
    (void)memcpy_s(&param->scanCbk, sizeof(NLSTK_DevdScanCbk_S), scanCbk, sizeof(NLSTK_DevdScanCbk_S));
    uint32_t ret = SchedulePostTaskBlocked(DevdRegScanModule, param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        (void)SchedulePostTask(DevdDeregScanModule, param, SDF_MemFree);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK || param->moduleId == DEVD_MAX_MODULE_ID) {
        SDF_MemFree(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *moduleId = param->moduleId;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_DevdDeregScanModule(uint8_t moduleId)
{
    NLSTK_CHECK_RETURN_VOID(moduleId < DEVD_MAX_MODULE_ID, "[DEVDS] moduleId is invalid");
    DevdScanModuleParam_S *param = (DevdScanModuleParam_S *)SDF_MemZalloc(sizeof(DevdScanModuleParam_S));
    NLSTK_CHECK_RETURN_VOID(param != NULL, "param malloc fail");
    param->moduleId = moduleId;
    uint32_t ret = SchedulePostTaskBlocked(DevdDeregScanModule, param, SDF_MemFree, NLSTK_API_TIME_OUT);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_OK, "[DEVDS] post task fail");
}

NLSTK_Errcode_E NLSTK_DevdAllocScannerId(uint8_t moduleId, uint32_t *scannerId)
{
    NLSTK_CHECK_RETURN(scannerId, NLSTK_ERRCODE_POINTER_NULL, "[DEVDS] scannerId is null");
    NLSTK_CHECK_RETURN(moduleId < DEVD_MAX_MODULE_ID, NLSTK_ERRCODE_PARAM_ERR, "[DEVDS] invalid moduleId");
    *scannerId = DEVD_INVALID_SCANNER_ID;
    DevdScannerParam_S *param = (DevdScannerParam_S *)SDF_MemZalloc(sizeof(DevdScannerParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "param malloc fail");
    param->moduleId = moduleId;
    uint32_t ret = SchedulePostTaskBlocked(DevdAllocScannerId, param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        (void)SchedulePostTask(DevdRemoveScannerId, param, SDF_MemFree);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK || param->scannerId == DEVD_INVALID_SCANNER_ID) {
        SDF_MemFree(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *scannerId = param->scannerId;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_DevdRemoveScannerId(uint32_t scannerId)
{
    NLSTK_CHECK_RETURN_VOID(scannerId != DEVD_INVALID_SCANNER_ID, "[DEVDS] scannerId is invalid");
    DevdScannerParam_S *param = (DevdScannerParam_S *)SDF_MemZalloc(sizeof(DevdScannerParam_S));
    NLSTK_CHECK_RETURN_VOID(param != NULL, "param malloc fail");
    param->scannerId = scannerId;
    uint32_t ret = SchedulePostTaskBlocked(DevdRemoveScannerId, param, SDF_MemFree, NLSTK_API_TIME_OUT);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_OK, "[DEVDS] post task fail");
}

NLSTK_Errcode_E NLSTK_DevdStartScan(uint32_t scannerId, NLSTK_DevdScanSetting_S *setting,
    NLSTK_DevdScanFilter_S *filters, uint16_t filtersNum)
{
    NLSTK_LOG_INFO("[DEVDS] scannerId %d enter NLSTK_DevdStartScan", scannerId);
    NLSTK_CHECK_RETURN(scannerId != DEVD_INVALID_SCANNER_ID, NLSTK_ERRCODE_PARAM_ERR, "[DEVDS] scannerId is invalid");
    NLSTK_CHECK_RETURN(setting != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVDS] setting is null");
    NLSTK_CHECK_RETURN(CheckFiltersLegal(filters, filtersNum), NLSTK_ERRCODE_PARAM_ERR, "[DEVDS] filters illgal");
    DevdStartScanParam_S *param = (DevdStartScanParam_S *)SDF_MemZalloc(sizeof(DevdStartScanParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "param malloc fail");
    SDF_Vector_S *filtersVector = DevdConvertFiltersToVector(filters, filtersNum);
    param->scannerId = scannerId;
    (void)memcpy_s(&param->setting, sizeof(NLSTK_DevdScanSetting_S), setting, sizeof(NLSTK_DevdScanSetting_S));
    param->filters = filtersVector;
    uint32_t ret = SchedulePostTask(DevdStartScan, param, DevdFreeStartScanParam);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL, "[DEVDS] post task fail");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdStopScan(uint32_t scannerId)
{
    NLSTK_LOG_INFO("[DEVDS] scannerId %d enter NLSTK_DevdStopScan", scannerId);
    NLSTK_CHECK_RETURN(scannerId != DEVD_INVALID_SCANNER_ID, NLSTK_ERRCODE_PARAM_ERR, "[DEVDS] scannerId is invalid");
    uint32_t *param = (uint32_t *)SDF_MemZalloc(sizeof(uint32_t));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "param malloc fail");
    *param = scannerId;
    uint32_t ret = SchedulePostTask(DevdStopScan, param, SDF_MemFree);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL, "[DEVDS] post task fail");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdStopAllScan(void)
{
    NLSTK_LOG_INFO("[DEVDS] enter NLSTK_DevdStopAllScan");
    uint32_t ret = SchedulePostTask(DevdStopAllScan, NULL, NULL);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL, "[DEVDS] post task fail");
    return NLSTK_ERRCODE_SUCCESS;
}