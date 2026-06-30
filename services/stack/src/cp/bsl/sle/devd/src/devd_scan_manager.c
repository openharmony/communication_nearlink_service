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
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "sdf_mem.h"
#include "sdf_string.h"
#include "dli_errno.h"
#include "devd_tbl.h"
#include "devd_scan_type.h"
#include "devd_scan_util.h"
#include "devd_scan_stm.h"
#include "devd_report_parse.h"
#include "devd_scan_filter.h"
#include "nlstk_devd_api.h"
#include "devd_scan_manager.h"
#include "sdf_string.h"
#include "collab_reg_ext_func.h"
#include "collab_ext_func_wrapper.h"
#include "devd_ext_func_wrapper.h"
#include "nlstk_stm_collab_ext.h"

static NLSTK_DevdStateMachine_S *g_scanStm = NULL;

static bool AddReportScannner(SDF_Vector_S **scanners, uint32_t scannerId);
static void ScanCbk(uint8_t eventMsg, uint8_t result);
static void ReportCbk(NLSTK_DevdAdvReportInfo_S *report);

static bool CompScannerId(void *ptr, void *args);
static bool CompModuleId(void *ptr, void *args);

static void SetScanMode(NLSTK_DevdScanSettingInner_S *setting, int32_t scanMode, DevdReportType_E type);
static NLSTK_Errcode_E SetScanDuration(NLSTK_DevdScanSettingInner_S *setting, int32_t duration);
static void PrintScanSettingLog(NLSTK_DevdScanManager_S *manager);
static void DurationTimeout(void *arg);
static bool StartDurationTimer(NLSTK_DevdScanManager_S *manager, time_t expires);
static void StopDurationTimer(NLSTK_DevdScanManager_S *manager);
static uint64_t TimespecDiff(const struct timespec *t1, const struct timespec *t2);
static void UpdateExpireTimer(NLSTK_DevdScanManager_S *manager, bool timeout);
static void ClearExpiredParam(NLSTK_DevdScanManager_S *manager);

static inline void ResetScanSetting(NLSTK_DevdScanSettingInner_S *setting)
{
    if (setting == NULL) {
        return;
    }
    (void)memset_s(setting, sizeof(NLSTK_DevdScanSettingInner_S), 0, sizeof(NLSTK_DevdScanSettingInner_S));
    setting->interval = UINT16_MAX;
}

static void DevdCollabScanStmCtorAfter(void)
{
    const NLSTK_DevdCollabScanFunc_S *scanFuncs = DevdGetCollabScanFunc();
    if (scanFuncs != NULL && scanFuncs->collabScanStmCtorAfter != NULL) {
        uint32_t ret = scanFuncs->collabScanStmCtorAfter();
        NLSTK_LOG_INFO("[COLLAB] collab stm scan init after complete, ret:%u", ret);
    }
}

static NLSTK_Errcode_E DevdScanFilterRegFunc(const NLSTK_DevdCollabScanFilterFunc_S *filterFunc)
{
    NLSTK_CHECK_RETURN(filterFunc != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVD] param is null");
    return DevdScanFilterInit(filterFunc);
}

static void DevdScanFilterUnRegFunc(void)
{
    DevdScanFilterDeInit();
}

NLSTK_Errcode_E DevdScanManagerInit(void)
{
    if (g_scanStm != NULL) {
        DevdStateMachineDtor(g_scanStm);
    }

    COLLAB_StmDevdScanCbk_S cbk = {};
    cbk.createScanModule = DevdCreateScanModule;
    cbk.removeScanModule = DevdRemoveScanModule;
    cbk.createScanner = DevdCreateScanner;
    cbk.destroyScanner = DevdDestroyScanner;
    cbk.addScanParam = DevdAddScanParam;
    cbk.removeScanParam = DevdRemoveScanParam;
    cbk.setScanfilter = DevdSetScanfilter;

    COLLAB_DevdCollabStmFunc_S func = {};
    func.setStmStateName = DevdSetStmStateName;
    func.getStmStateName = DevdGetStmStateName;
    func.regCollabFunc = DevdCollabRegFunc;
    func.unRegCollabFunc = DevdCollabUnRegFunc;
    func.regScanFilterFunc = DevdScanFilterRegFunc;
    func.unRegScanFilterFunc = DevdScanFilterUnRegFunc;
    if (COLLAB_StmDevdScanInit(&cbk, &func) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DEVDS] collab stm scan register fail");
    }

    g_scanStm = DevdStateMachineCtor();
    NLSTK_CHECK_RETURN(g_scanStm != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVDS] g_scanStm create fail");
    NLSTK_DevdSleScanExterCbk_S scanEventCbk = {0};
    scanEventCbk.scanCbk = ScanCbk;
    scanEventCbk.reportCbk = ReportCbk;
    NLSTK_Errcode_E ret = NLSTK_DevdRegScanEventCbk(&scanEventCbk);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DevdStateMachineDtor(g_scanStm);
        g_scanStm = NULL;
        COLLAB_StmDevdScanDeInit();
        return ret;
    }
    COLLAB_StmDevdScanInitAfterReg_Cbk_S afterCbk = {};
    afterCbk.initAfterFunc = DevdCollabScanStmCtorAfter;
    if (COLLAB_StmDevdScanInitAfterReg(&afterCbk) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_WARN("[DEVDS] collab init after complete");
    }
    return ret;
}

void DevdScanManagerDeInit(void)
{
    NLSTK_DevdSleScanExterCbk_S scanEventCbk = {0};
    (void)NLSTK_DevdRegScanEventCbk(&scanEventCbk);
    NLSTK_DevdSleScanEnable_S scanEnable = {0};
    (void)NLSTK_DevdSleEnableScan(&scanEnable);
    DevdStateMachineDtor(g_scanStm);
    COLLAB_StmDevdScanDeInit();
    g_scanStm = NULL;
}

NLSTK_DevdStateMachine_S *DevdGetStateMachine(void)
{
    return g_scanStm;
}

uint8_t DevdCreateScanModule(NLSTK_DevdScanCbk_S *scanCbk)
{
    uint8_t moduleId = DEVD_MAX_MODULE_ID;
    NLSTK_CHECK_RETURN(scanCbk != NULL, DEVD_MAX_MODULE_ID, "[DEVDS] scanCbk is null");
    NLSTK_CHECK_RETURN(g_scanStm != NULL, DEVD_MAX_MODULE_ID, "[DEVDS] g_scanStm is null");
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(g_scanStm, scanManager);
    for (uint8_t i = 0; i < DEVD_MAX_MODULE_ID; i++) {
        if (!manager->scanModule[i].used) {
            moduleId = i;
            manager->scanModule[i].used = true;
            (void)memcpy_s(&manager->scanModule[i].scanCbk, sizeof(NLSTK_DevdScanCbk_S), scanCbk,
                sizeof(NLSTK_DevdScanCbk_S));
            NLSTK_LOG_INFO("[DEVDS] create scan moduleId %u", moduleId);
            return moduleId;
        }
    }
    return DEVD_MAX_MODULE_ID;
}

void DevdRemoveScanModule(uint8_t moduleId)
{
    NLSTK_CHECK_RETURN_VOID(moduleId < DEVD_MAX_MODULE_ID, "[DEVDS] invalid moduleId");
    NLSTK_CHECK_RETURN_VOID(g_scanStm != NULL, "[DEVDS] g_scanStm is null");
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(g_scanStm, scanManager);
    manager->scanModule[moduleId].used = false;
    (void)memset_s(&manager->scanModule[moduleId].scanCbk, sizeof(NLSTK_DevdScanCbk_S), 0, sizeof(NLSTK_DevdScanCbk_S));
    size_t index = 0;
    while (SDF_VectorFindFirst(manager->scanners, CompModuleId, &moduleId, &index)) {
        NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(manager->scanners, index);
        NLSTK_LOG_INFO("[DEVDS] remove scannerId %u", scanner->scannerId);
        SDF_VectorRemove(manager->scanners, index);
    }
    NLSTK_LOG_INFO("[DEVDS] remove scan moduleId %u, remain scanners size %u", moduleId, manager->scanners->size);
}

uint32_t DevdGetScannerMaxNum(void)
{
    return NLSTK_DEVD_MULTI_SCAN_MAX_NUM;
}

uint32_t DevdCreateScanner(uint8_t moduleId)
{
    uint32_t scannerId = DEVD_INVALID_SCANNER_ID;
    NLSTK_CHECK_RETURN(g_scanStm != NULL, DEVD_INVALID_SCANNER_ID, "[DEVDS] g_scanStm is null");
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(g_scanStm, scanManager);
    NLSTK_CHECK_RETURN(moduleId < DEVD_MAX_MODULE_ID && manager->scanModule[moduleId].used, DEVD_INVALID_SCANNER_ID,
        "[DEVDS] invalid moduleId");
    uint32_t maxNum = 0;
    const NLSTK_DevdCollabScanFunc_S *scanFuncs = DevdGetCollabScanFunc();
    if (scanFuncs != NULL && scanFuncs->collabGetScannerMaxNum != NULL) {
        maxNum = scanFuncs->collabGetScannerMaxNum();
    } else {
        maxNum = DevdGetScannerMaxNum();
    }
    size_t index = 0;
    for (uint32_t i = 1; i <= maxNum; i++) {
        uint32_t unusedId = i;
        if (!SDF_VectorFindFirst(manager->scanners, CompScannerId, &unusedId, &index)) {
            scannerId = unusedId;
            break;
        }
    }
    NLSTK_CHECK_RETURN(scannerId != DEVD_INVALID_SCANNER_ID, DEVD_INVALID_SCANNER_ID, "[DEVDS] no valid scannerId");
    NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_MemZalloc(sizeof(NLSTK_DevdScanner_S));
    NLSTK_CHECK_RETURN(scanner != NULL, DEVD_INVALID_SCANNER_ID, "[DEVDS] scanner malloc fail");
    scanner->scannerId = scannerId;
    scanner->moduleId = moduleId;
    ResetScanSetting(&scanner->scanSetting);
    ResetScanSetting(&scanner->frame4ScanSetting);

    if (!SDF_VectorEmplaceBack(manager->scanners, scanner)) {
        NLSTK_LOG_ERROR("[DEVDS] scanner emplace back fail");
        SDF_MemFree(scanner);
        return DEVD_INVALID_SCANNER_ID;
    }
    NLSTK_LOG_INFO("[DEVDS] new scannerId %u, moduleId %u, scanners size %u", scannerId, moduleId,
        manager->scanners->size);
    return scannerId;
}

void DevdDestroyScanner(uint32_t scannerId)
{
    NLSTK_CHECK_RETURN_VOID(scannerId != DEVD_INVALID_SCANNER_ID, "[DEVDS] invalid scannerId");
    NLSTK_CHECK_RETURN_VOID(g_scanStm != NULL, "[DEVDS] g_scanStm is null");
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(g_scanStm, scanManager);
    size_t index = 0;
    NLSTK_CHECK_RETURN_VOID(SDF_VectorFindFirst(manager->scanners, CompScannerId, &scannerId, &index),
        "[DEVDS] scannerId not found");
    NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(manager->scanners, index);
    DevdDeleteFilters(manager, scanner);
    SDF_VectorRemove(manager->scanners, index);
    NLSTK_LOG_INFO("[DEVDS] remove scannerId %u, remain scanners size %u", scannerId, manager->scanners->size);
}

NLSTK_Errcode_E DevdAddScanParam(uint32_t scannerId, NLSTK_DevdScanSetting_S *setting, SDF_Vector_S *filters)
{
    NLSTK_CHECK_RETURN(scannerId != DEVD_INVALID_SCANNER_ID, NLSTK_ERRCODE_PARAM_ERR, "[DEVDS] invalid scannerId");
    NLSTK_CHECK_RETURN(g_scanStm != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVDS] g_scanStm is null");
    NLSTK_CHECK_RETURN(setting != NULL && filters != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVDS] param is null");
    NLSTK_LOG_INFO("[DEVDS] AddScanParam scannerId:%u, FrameType:%u, scanMode:%d enter",
        scannerId, setting->frameType, setting->scanMode);
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(g_scanStm, scanManager);
    size_t index = 0;
    NLSTK_CHECK_RETURN(SDF_VectorFindFirst(manager->scanners, CompScannerId, &scannerId, &index),
        NLSTK_ERRCODE_PARAM_ERR, "[DEVDS] scannerId not found");
    NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(manager->scanners, index);
    SDF_Vector_S *filtersClone = DevdCloneFiltersVector(filters);
    NLSTK_CHECK_RETURN(filtersClone != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DEVDS] filters clone fail");
    DevdAddFilters(manager->scanners, scanner, setting, filtersClone);
    if (setting->scanMode == SCAN_MODE_MONITOR) {
        NLSTK_LOG_INFO("[DEVDS] monitor");
        ResetScanSetting(&scanner->scanSetting);
        ResetScanSetting(&scanner->frame4ScanSetting);
        return NLSTK_ERRCODE_SUCCESS;
    }
    DevdReportType_E type = setting->reportDelayMillis > 0 ? DEVD_TYPE_ALL_MATCHES : DEVD_TYPE_FIRST_MATCH;
    if (setting->frameType == DEVD_SCAN_FRAME_TYPE_1) {
        (void)memcpy_s(&scanner->scanSetting.setting, sizeof(NLSTK_DevdScanSetting_S),
            setting, sizeof(NLSTK_DevdScanSetting_S));
        SetScanMode(&scanner->scanSetting, setting->scanMode, type);
        if (setting->duration > 0) {
            NLSTK_CHECK_RETURN(SetScanDuration(&scanner->scanSetting, setting->duration) == NLSTK_ERRCODE_SUCCESS,
                NLSTK_ERRCODE_FAIL, "[DEVDS] set frame 1 scan duration failed");
            UpdateExpireTimer(manager, false);
        }
    } else if (setting->frameType == DEVD_SCAN_FRAME_TYPE_4) {
        (void)memcpy_s(&scanner->frame4ScanSetting.setting, sizeof(NLSTK_DevdScanSetting_S),
            setting, sizeof(NLSTK_DevdScanSetting_S));
        SetScanMode(&scanner->frame4ScanSetting, setting->scanMode, type);
        if (setting->duration > 0) {
            NLSTK_CHECK_RETURN(SetScanDuration(&scanner->frame4ScanSetting, setting->duration) == NLSTK_ERRCODE_SUCCESS,
                NLSTK_ERRCODE_FAIL, "[DEVDS] set frame 4 scan duration failed");
            UpdateExpireTimer(manager, false);
        }
    } else {
        NLSTK_LOG_ERROR("[DEVDS] invalid frameType");
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    PrintScanSettingLog(manager);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E DevdRemoveScanParam(uint32_t scannerId)
{
    NLSTK_CHECK_RETURN(scannerId != DEVD_INVALID_SCANNER_ID, NLSTK_ERRCODE_PARAM_ERR, "[DEVDS] invalid scannerId");
    NLSTK_CHECK_RETURN(g_scanStm != NULL, NLSTK_ERRCODE_POINTER_NULL, "[DEVDS] g_scanStm is null");
    NLSTK_LOG_INFO("[DEVDS] RemoveScanParam scannerId:%u enter", scannerId);
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(g_scanStm, scanManager);
    size_t index = 0;
    NLSTK_CHECK_RETURN(SDF_VectorFindFirst(manager->scanners, CompScannerId, &scannerId, &index),
        NLSTK_ERRCODE_PARAM_ERR, "[DEVDS] scannerId not found");
    NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(manager->scanners, index);
    ResetScanSetting(&scanner->scanSetting);
    ResetScanSetting(&scanner->frame4ScanSetting);
    DevdDeleteFilters(manager, scanner);
    UpdateExpireTimer(manager, false);
    PrintScanSettingLog(manager);
    return NLSTK_ERRCODE_SUCCESS;
}

void DevdRemoveAllScanParam(void)
{
    NLSTK_CHECK_RETURN_VOID(g_scanStm != NULL, "[DEVDS] g_scanStm is null");
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(g_scanStm, scanManager);
    for (size_t i = 0; i < manager->scanners->size; i++) {
        NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(manager->scanners, i);
        ResetScanSetting(&scanner->scanSetting);
        ResetScanSetting(&scanner->frame4ScanSetting);
        SDF_DestroyVector(scanner->filters);
        scanner->filters = NULL;
    }
    if (DevdIsSupportScanFilter()) {
        DevdSetScanfilterSwitch(DEVD_FILTER_DELETE_DISABLE);
    }
    UpdateExpireTimer(manager, false);
}

/*****************************************************************************************
                                    Static Functions
*****************************************************************************************/
static void SetLowPowerDuration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_LOW_POWER_INTERVAL;
        setting->window = SCAN_MODE_BATCH_LOW_POWER_WINDOW;
    } else {
        setting->interval = SCAN_MODE_LOW_POWER_INTERVAL;
        setting->window = SCAN_MODE_LOW_POWER_WINDOW;
    }
}
static void SetBalancedDuration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_BALANCED_INTERVAL;
        setting->window = SCAN_MODE_BATCH_BALANCED_WINDOW;
    } else {
        setting->interval = SCAN_MODE_BALANCED_INTERVAL;
        setting->window = SCAN_MODE_BALANCED_WINDOW;
    }
}

static void SetFullScanDuration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_FULL_SCAN_INTERVAL;
        setting->window = SCAN_MODE_BATCH_FULL_SCAN_WINDOW;
    } else {
        setting->interval = SCAN_MODE_FULL_SCAN_INTERVAL;
        setting->window = SCAN_MODE_FULL_SCAN_WINDOW;
    }
}

static void SetLowLatencyDuration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_LOW_LATENCY_INTERVAL;
        setting->window = SCAN_MODE_BATCH_LOW_LATENCY_WINDOW;
    } else {
        setting->interval = SCAN_MODE_LOW_LATENCY_INTERVAL;
        setting->window = SCAN_MODE_LOW_LATENCY_WINDOW;
    }
}

static void SetDutyCycle2Duration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_OP_P2_60_3000_INTERVAL;
        setting->window = SCAN_MODE_BATCH_OP_P2_60_3000_WINDOW;
    } else {
        setting->interval = SCAN_MODE_OP_P2_60_3000_INTERVAL;
        setting->window = SCAN_MODE_OP_P2_60_3000_WINDOW;
    }
}

static void SetDutyCycle10LowDuration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_OP_P10_30_300_INTERVAL;
        setting->window = SCAN_MODE_BATCH_OP_P10_30_300_WINDOW;
    } else {
        setting->interval = SCAN_MODE_OP_P10_30_300_INTERVAL;
        setting->window = SCAN_MODE_OP_P10_30_300_WINDOW;
    }
}

static void SetDutyCycle10Duration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_OP_P10_60_600_INTERVAL;
        setting->window = SCAN_MODE_BATCH_OP_P10_60_600_WINDOW;
    } else {
        setting->interval = SCAN_MODE_OP_P10_60_600_INTERVAL;
        setting->window = SCAN_MODE_OP_P10_60_600_WINDOW;
    }
}

static void SetDutyCycle25Duration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_OP_P25_60_240_INTERVAL;
        setting->window = SCAN_MODE_BATCH_OP_P25_60_240_WINDOW;
    } else {
        setting->interval = SCAN_MODE_OP_P25_60_240_INTERVAL;
        setting->window = SCAN_MODE_OP_P25_60_240_WINDOW;
    }
}

static void SetDutyCycle50LowDuration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_OP_P50_100_200_INTERVAL;
        setting->window = SCAN_MODE_BATCH_OP_P50_100_200_WINDOW;
    } else {
        setting->interval = SCAN_MODE_OP_P50_100_200_INTERVAL;
        setting->window = SCAN_MODE_OP_P50_100_200_WINDOW;
    }
}

static void SetDutyCycle50Duration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_OP_P50_240_480_INTERVAL;
        setting->window = SCAN_MODE_BATCH_OP_P50_240_480_WINDOW;
    } else {
        setting->interval = SCAN_MODE_OP_P50_240_480_INTERVAL;
        setting->window = SCAN_MODE_OP_P50_240_480_WINDOW;
    }
}

static void SetDutyCycle50HighDuration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_OP_P50_480_960_INTERVAL;
        setting->window = SCAN_MODE_BATCH_OP_P50_480_960_WINDOW;
    } else {
        setting->interval = SCAN_MODE_OP_P50_480_960_INTERVAL;
        setting->window = SCAN_MODE_OP_P50_480_960_WINDOW;
    }
}

static void SetDutyCycle100LowDuration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_OP_P100_240_240_INTERVAL;
        setting->window = SCAN_MODE_BATCH_OP_P100_240_240_WINDOW;
    } else {
        setting->interval = SCAN_MODE_OP_P100_240_240_INTERVAL;
        setting->window = SCAN_MODE_OP_P100_240_240_WINDOW;
    }
}

static void SetDutyCycle100Duration(NLSTK_DevdScanSettingInner_S *setting, int type)
{
    if (type == DEVD_TYPE_ALL_MATCHES) {
        setting->interval = SCAN_MODE_BATCH_OP_P100_1000_1000_INTERVAL;
        setting->window = SCAN_MODE_BATCH_OP_P100_1000_1000_WINDOW;
    } else {
        setting->interval = SCAN_MODE_OP_P100_1000_1000_INTERVAL;
        setting->window = SCAN_MODE_OP_P100_1000_1000_WINDOW;
    }
}

static void SetScanMode(NLSTK_DevdScanSettingInner_S *setting, int32_t scanMode, DevdReportType_E type)
{
    NLSTK_LOG_INFO("[DEVDS] scanMode: %d, type: %d", scanMode, type);
    switch (scanMode) {
        case SCAN_MODE_LOW_POWER:
            SetLowPowerDuration(setting, type);
            break;
        case SCAN_MODE_BALANCED:
            SetBalancedDuration(setting, type);
            break;
        case SCAN_MODE_LOW_LATENCY:
            SetLowLatencyDuration(setting, type);
            break;
        case SCAN_MODE_OP_P2_60_3000:
            SetDutyCycle2Duration(setting, type);
            break;
        case SCAN_MODE_OP_P10_30_300:
            SetDutyCycle10LowDuration(setting, type);
            break;
        case SCAN_MODE_OP_P10_60_600:
            SetDutyCycle10Duration(setting, type);
            break;
        case SCAN_MODE_OP_P25_60_240:
            SetDutyCycle25Duration(setting, type);
            break;
        case SCAN_MODE_OP_P50_100_200:
            SetDutyCycle50LowDuration(setting, type);
            break;
        case SCAN_MODE_OP_P50_240_480:
            SetDutyCycle50Duration(setting, type);
            break;
        case SCAN_MODE_OP_P50_480_960:
            SetDutyCycle50HighDuration(setting, type);
            break;
        case SCAN_MODE_OP_P100_240_240:
            SetDutyCycle100LowDuration(setting, type);
            break;
        case SCAN_MODE_OP_P100_1000_1000:
            SetDutyCycle100Duration(setting, type);
            break;
        case SCAN_MODE_FULL_SCAN:
            SetFullScanDuration(setting, type);
            break;
        default:
            NLSTK_LOG_ERROR("[DEVDS] scanMode: %d, type: %d invalid", scanMode, type);
            break;
    }
}

static void ScanCbk(uint8_t eventMsg, uint8_t result)
{
    NLSTK_LOG_INFO("[DEVDS] eventMsg %hhu, result %hhu", eventMsg, result);
    NLSTK_CHECK_RETURN_VOID(g_scanStm != NULL, "[DEVDS] g_scanStm is null");

    switch (eventMsg) {
        case DEVD_CBK_EVENT_ENABLE_SCAN:
            if (result == DLI_SUCCESS) {
                STM_MFUNC(g_scanStm, ProcessMessage, (Message) { .what = DEVD_START_OK });
            } else {
                STM_MFUNC(g_scanStm, ProcessMessage, (Message) { .what = DEVD_START_ERR });
            }
            break;
        case DEVD_CBK_EVENT_DISABLE_SCAN:
            if (result == DLI_SUCCESS) {
                STM_MFUNC(g_scanStm, ProcessMessage, (Message) { .what = DEVD_STOP_OK });
            } else {
                STM_MFUNC(g_scanStm, ProcessMessage, (Message) { .what = DEVD_STOP_ERR });
            }
            break;
        default:
            break;
    }
}

static bool AddReportScannner(SDF_Vector_S **scanners, uint32_t scannerId)
{
    SDF_Traits traits = {
        .dtor = SDF_MemFree,
    };
    if (!(*scanners)) {
        *scanners = SDF_CreateVector(traits);
        NLSTK_CHECK_RETURN(*scanners != NULL, false, "[DEVDS] create vector failed");
    }
    uint32_t *reportId = (uint32_t *)SDF_MemZalloc(sizeof(uint32_t));
    NLSTK_CHECK_RETURN(reportId != NULL, false, "[DEVDS] reportId malloc failed");
    *reportId = scannerId;
    if (!SDF_VectorEmplaceBack(*scanners, reportId)) {
        NLSTK_LOG_ERROR("[DEVDS] emplace back failed");
        SDF_MemFree(reportId);
        return false;
    }
    return true;
}

static void ReportCbk(NLSTK_DevdAdvReportInfo_S *report)
{
    NLSTK_DevdAdvResult_S *result = DevdAdvDataReport(report);
    if (result == NULL) {
        return;
    }
    NLSTK_CHECK_RETURN_VOID(g_scanStm != NULL, "[DEVDS] g_scanStm is null");

    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(g_scanStm, scanManager);
    SDF_Vector_S *scanners[DEVD_MAX_MODULE_ID] = {0};
    for (size_t i = 0; i < manager->scanners->size; i++) {
        NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(manager->scanners, i);
        if (DevdIsMatchFilters(result, scanner->filters) && scanner->moduleId < DEVD_MAX_MODULE_ID) {
            NLSTK_LOG_DEBUG("[DEVDS] report moduleId: %u, scannerId: %u", scanner->moduleId, scanner->scannerId);
            if (AddReportScannner(&scanners[scanner->moduleId], scanner->scannerId)) {
                continue;
            }
            NLSTK_LOG_ERROR("[DEVDS] add report scanner fail");
            for (uint8_t j = 0; j < DEVD_MAX_MODULE_ID; j++) {
                SDF_DestroyVector(scanners[j]);
            }
            FreeAdvResult(result);
            return;
        }
    }
    for (uint8_t i = 0; i < DEVD_MAX_MODULE_ID; i++) {
        if (scanners[i] && manager->scanModule[i].scanCbk.onScanCallback) {
            result->scannerIds = scanners[i];
            manager->scanModule[i].scanCbk.onScanCallback(result);
        }
        SDF_DestroyVector(scanners[i]);
    }

    ClearAdvDataCache(&result->addr);
    FreeAdvResult(result);
}

static bool CompScannerId(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)ptr;
    uint32_t *scannerId = (uint32_t *)args;
    return scanner->scannerId == *scannerId;
}

static bool CompModuleId(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)ptr;
    uint8_t *moduleId = (uint8_t *)args;
    return scanner->moduleId == *moduleId;
}

static NLSTK_Errcode_E SetScanDuration(NLSTK_DevdScanSettingInner_S *setting, int32_t duration)
{
    setting->hasDuration = true;
    NLSTK_CHECK_RETURN(clock_gettime(CLOCK_REALTIME, &setting->expireTime) == 0, NLSTK_ERRCODE_FAIL,
        "[DEVDS] clock_gettime failed");
    setting->expireTime.tv_sec += duration;
    return NLSTK_ERRCODE_SUCCESS;
}

static void PrintScanSettingLog(NLSTK_DevdScanManager_S *manager)
{
    for (size_t i = 0; i < manager->scanners->size; i++) {
        NLSTK_DevdScanner_S *scnr = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(manager->scanners, i);
        NLSTK_LOG_INFO("[DEVDS] ScanParam: scannerId:%d, windows:%d, interval:%d, frame type:1",
            scnr->scannerId, scnr->scanSetting.window, scnr->scanSetting.interval);
    }
    for (size_t i = 0; i < manager->scanners->size; i++) {
        NLSTK_DevdScanner_S *scnr = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(manager->scanners, i);
        NLSTK_LOG_INFO("[DEVDS] ScanParam: scannerId:%d, windows:%d, interval:%d, frame type:4",
            scnr->scannerId, scnr->frame4ScanSetting.window, scnr->frame4ScanSetting.interval);
    }
}

static void DurationTimeout(void *arg)
{
    SDF_UNUSED(arg);
    NLSTK_LOG_INFO("[DEVDS] enter DurationTimeout, ClearExpiredParam and UpdateExpireTimer");
    NLSTK_CHECK_RETURN_VOID(g_scanStm != NULL, "[DEVDS] g_scanStm is null");
    NLSTK_DevdScanManager_S *manager = DEVD_STM_M(g_scanStm, scanManager);
    ClearExpiredParam(manager);
    UpdateExpireTimer(manager, true);
}

static bool StartDurationTimer(NLSTK_DevdScanManager_S *manager, time_t expires)
{
    SDF_TimerParam param = {
        .expires = expires,
        .period = false,
        .callback = DurationTimeout,
        .args = NULL,
    };
    uint32_t ret = ScheduleTimerAdd(&manager->durationTimer, &param);
    if (ret != SDF_OK) {
        NLSTK_LOG_ERROR("create timer failed, ret: 0x%08x", ret);
        return false;
    }
    return true;
}

static void StopDurationTimer(NLSTK_DevdScanManager_S *manager)
{
    if (manager->durationTimer != NLSTK_DEVD_TIMER_NO_USED_HANDLE) {
        ScheduleTimerDel(manager->durationTimer);
        manager->durationTimer = NLSTK_DEVD_TIMER_NO_USED_HANDLE;
    }
}

static uint64_t TimespecDiff(const struct timespec *t1, const struct timespec *t2)
{
    // 计算秒差并转换为毫秒
    int64_t secDiff = (int64_t)t2->tv_sec - (int64_t)t1->tv_sec;
    int64_t msFromSec = secDiff * 1000;        // 1000 ms = 1 s
    // 计算纳秒差并转换为毫秒（使用整数除法截断）
    int64_t nsecDiff = (int64_t)t2->tv_nsec - (int64_t)t1->tv_nsec;
    int64_t msFromNsec = nsecDiff / 1000000;   // 1000000 ns = 1 ms
    // 合并毫秒值，增加1ms补偿微秒截断误差，避免计时器提前停止
    int64_t totalMs = msFromSec + msFromNsec + 1;
    // 确保非负
    return totalMs > 0 ? (uint64_t)totalMs : 0;
}

static void UpdateExpireTimer(NLSTK_DevdScanManager_S *manager, bool timeout)
{
    struct timespec *minTs = NULL;
    for (size_t i = 0; i < manager->scanners->size; i++) {
        NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(manager->scanners, i);
        if (scanner->scanSetting.hasDuration) {
            NLSTK_LOG_DEBUG("[DEVDS] scannerId %d has frame 1 duration", scanner->scannerId);
            if (minTs == NULL || minTs->tv_sec > scanner->scanSetting.expireTime.tv_sec ||
                (minTs->tv_sec == scanner->scanSetting.expireTime.tv_sec && minTs->tv_nsec >
                scanner->scanSetting.expireTime.tv_nsec)) {
                minTs = &scanner->scanSetting.expireTime;
            }
        }
        if (scanner->frame4ScanSetting.hasDuration) {
            NLSTK_LOG_DEBUG("[DEVDS] scannerId %d has frame 4 duration", scanner->scannerId);
            if (minTs == NULL || minTs->tv_sec > scanner->frame4ScanSetting.expireTime.tv_sec ||
                (minTs->tv_sec == scanner->frame4ScanSetting.expireTime.tv_sec && minTs->tv_nsec >
                scanner->frame4ScanSetting.expireTime.tv_nsec)) {
                minTs = &scanner->frame4ScanSetting.expireTime;
            }
        }
    }
    if (!timeout) {
        // 若是由定时器触发的更新持续时间，由定时器触发执行后自行删除句柄，否则会导致新申请的定时器句柄与原句柄一样被定时器逻辑误删除
        StopDurationTimer(manager);
    }
    if (minTs != NULL) {
        struct timespec now = {0};
        NLSTK_CHECK_RETURN_VOID(clock_gettime(CLOCK_REALTIME, &now) == 0, "[DEVDS] clock_gettime failed");
        uint64_t durationMs = TimespecDiff(&now, minTs);
        NLSTK_LOG_INFO("[DEVDS] start duration timer on %llu ms", durationMs);
        if (durationMs == 0) {
            DurationTimeout(NULL);
            return;
        }
        NLSTK_CHECK_RETURN_VOID(StartDurationTimer(manager, (time_t)durationMs), "[DEVDS] start duration timer failed");
    }
}

static void ClearExpiredParam(NLSTK_DevdScanManager_S *manager)
{
    NLSTK_CHECK_RETURN_VOID(g_scanStm != NULL, "[DEVDS] g_scanStm is null");
    struct timespec now = {0};
    NLSTK_CHECK_RETURN_VOID(clock_gettime(CLOCK_REALTIME, &now) == 0, "[DEVDS] clock_gettime failed");
    for (size_t i = 0; i < manager->scanners->size; i++) {
        NLSTK_DevdScanner_S *scanner = (NLSTK_DevdScanner_S *)SDF_VectorElementAt(manager->scanners, i);
        if (scanner->scanSetting.hasDuration && (now.tv_sec > scanner->scanSetting.expireTime.tv_sec ||
            (now.tv_sec == scanner->scanSetting.expireTime.tv_sec &&
            now.tv_nsec > scanner->scanSetting.expireTime.tv_nsec))) {
            NLSTK_LOG_INFO("clear scannerId %d frame 1 scan setting", scanner->scannerId);
            ResetScanSetting(&scanner->scanSetting);
        }
        if (scanner->frame4ScanSetting.hasDuration && (now.tv_sec > scanner->frame4ScanSetting.expireTime.tv_sec ||
            (now.tv_sec == scanner->frame4ScanSetting.expireTime.tv_sec &&
            now.tv_nsec > scanner->frame4ScanSetting.expireTime.tv_nsec))) {
            NLSTK_LOG_INFO("clear scannerId %d frame 4 scan setting", scanner->scannerId);
            ResetScanSetting(&scanner->frame4ScanSetting);
        }
    }
    STM_MFUNC(g_scanStm, ProcessMessage, (Message) { .what = DEVD_POTIENTIAL_UPDATE });
}