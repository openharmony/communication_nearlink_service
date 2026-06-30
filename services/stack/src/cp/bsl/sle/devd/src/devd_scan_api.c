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

#include "nlstk_log.h"
#include "devd_scan_type.h"
#include "devd_scan_manager.h"
#include "devd_report_parse.h"
#include "devd_scan_stm.h"
#include "devd_scan_api.h"

NLSTK_Errcode_E DevdScanInit(void)
{
    DevdDataCacheInit();
    return DevdScanManagerInit();
}

void DevdScanDeInit(void)
{
    DevdDataCacheDeInit();
    DevdScanManagerDeInit();
}

void DevdRegScanModule(void *arg)
{
    DevdScanModuleParam_S *param = (DevdScanModuleParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[DEVDS] param is null");
    param->moduleId = DevdCreateScanModule(&param->scanCbk);
}

void DevdDeregScanModule(void *arg)
{
    DevdScanModuleParam_S *param = (DevdScanModuleParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[DEVDS] param is null");
    DevdRemoveScanModule(param->moduleId);
    NLSTK_DevdStateMachine_S *scanStm = DevdGetStateMachine();
    NLSTK_CHECK_RETURN_VOID(scanStm != NULL, "[DEVDS] scanStm is null");
    STM_MFUNC(scanStm, ProcessMessage, (Message) { .what = DEVD_POTIENTIAL_UPDATE });
}

void DevdAllocScannerId(void *arg)
{
    DevdScannerParam_S *param = (DevdScannerParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[DEVDS] param is null");
    param->scannerId = DevdCreateScanner(param->moduleId);
}

void DevdRemoveScannerId(void *arg)
{
    DevdScannerParam_S *param = (DevdScannerParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[DEVDS] param is null");
    DevdDestroyScanner(param->scannerId);
    NLSTK_DevdStateMachine_S *scanStm = DevdGetStateMachine();
    NLSTK_CHECK_RETURN_VOID(scanStm != NULL, "[DEVDS] scanStm is null");
    STM_MFUNC(scanStm, ProcessMessage, (Message) { .what = DEVD_POTIENTIAL_UPDATE });
}

void DevdStartScan(void *arg)
{
    DevdStartScanParam_S *param = (DevdStartScanParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[DEVDS] param is null");
    NLSTK_Errcode_E ret = DevdAddScanParam(param->scannerId, &param->setting, param->filters);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[DEVDS] add scan param fail");
    NLSTK_DevdStateMachine_S *scanStm = DevdGetStateMachine();
    NLSTK_CHECK_RETURN_VOID(scanStm != NULL, "[DEVDS] scanStm is null");
    STM_MFUNC(scanStm, ProcessMessage, (Message) { .what = DEVD_POTIENTIAL_UPDATE });
}

void DevdStopScan(void *arg)
{
    uint32_t *param = (uint32_t *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[DEVDS] param is null");
    NLSTK_Errcode_E ret = DevdRemoveScanParam(*param);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[DEVDS] remove scan param fail");
    NLSTK_DevdStateMachine_S *scanStm = DevdGetStateMachine();
    NLSTK_CHECK_RETURN_VOID(scanStm != NULL, "[DEVDS] scanStm is null");
    STM_MFUNC(scanStm, ProcessMessage, (Message) { .what = DEVD_POTIENTIAL_UPDATE });
}

void DevdStopAllScan(void *arg)
{
    SDF_UNUSED(arg);
    DevdRemoveAllScanParam();
    NLSTK_DevdStateMachine_S *scanStm = DevdGetStateMachine();
    NLSTK_CHECK_RETURN_VOID(scanStm != NULL, "[DEVDS] scanStm is null");
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(scanStm, scanManager);
    NLSTK_CHECK_RETURN_VOID(manager != NULL, "[DEVDS] manager is null");
    const char *current = ((StateMachine *)scanStm)->GetCurrentStateName((StateMachine *)scanStm);
    const char *stopped = DevdGetStmStateName(DEVD_STATE_STOPPED);
    if (current != NULL && stopped != NULL && strncmp(current, stopped, strlen(stopped)) == 0) {
        // 已经在停止状态，向所有扫描模块回调一次扫描已停止
        NLSTK_LOG_INFO("[DEVDS] scan has stopped");
        for (uint8_t i = 0; i < DEVD_MAX_MODULE_ID; i++) {
            if (manager->scanModule[i].used && manager->scanModule[i].scanCbk.onStartOrStopEvent) {
                manager->scanModule[i].scanCbk.onStartOrStopEvent(NLSTK_ERRCODE_SUCCESS, false);
            }
        }
    }
    STM_MFUNC(scanStm, ProcessMessage, (Message) { .what = DEVD_POTIENTIAL_UPDATE });
}