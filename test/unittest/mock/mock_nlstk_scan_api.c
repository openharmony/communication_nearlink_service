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

#include "nlstk_scan_api.h"
#include "nlstk_devd_api.h"
#include "nlstk_public_define.h"
#include "log.h"

/* 全局变量存储回调和状态 */
static NLSTK_DevdScanCbk_S g_scan_callbacks = {0};
static uint8_t g_next_scanner_id = 1;
static bool g_stop_all_scan_triggered = false;
static uint32_t g_last_start_scan_id = 0;
static uint32_t g_last_stop_scan_id = 0;
static uint32_t g_last_remove_scan_id = 0;
static bool g_scan_started = false;
static bool g_scan_stopped = false;
static uint32_t g_start_scan_call_count = 0;
static uint32_t g_stop_scan_call_count = 0;

/* Mock NLSTK 扫描 API 实现 */

NLSTK_Errcode_E NLSTK_DevdRegScanModule(uint8_t *moduleId, NLSTK_DevdScanCbk_S *scanCbk)
{
    HILOGI("[Mock NLSTK] NLSTK_DevdRegScanModule enter");
    if (moduleId == NULL || scanCbk == NULL) {
        HILOGE("[Mock NLSTK] Invalid parameters");
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    
    *moduleId = 1;
    g_scan_callbacks = *scanCbk;
    HILOGI("[Mock NLSTK] Module registered with ID: %{public}d", *moduleId);
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_DevdDeregScanModule(uint8_t moduleId)
{
    HILOGI("[Mock NLSTK] NLSTK_DevdDeregScanModule enter, moduleId: %{public}d", moduleId);
    g_scan_callbacks.onStartOrStopEvent = NULL;
    g_scan_callbacks.onScanCallback = NULL;
}

NLSTK_Errcode_E NLSTK_DevdAllocScannerId(uint8_t moduleId, uint32_t *scannerId)
{
    HILOGI("[Mock NLSTK] NLSTK_DevdAllocScannerId enter, moduleId: %{public}d", moduleId);
    if (scannerId == NULL) {
        HILOGE("[Mock NLSTK] Invalid scannerId pointer");
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    
    *scannerId = g_next_scanner_id++;
    HILOGI("[Mock NLSTK] Allocated scannerId: %{public}u", *scannerId);
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_DevdRemoveScannerId(uint32_t scannerId)
{
    HILOGI("[Mock NLSTK] NLSTK_DevdRemoveScannerId enter, scannerId: %{public}u", scannerId);
    g_last_remove_scan_id = scannerId;
}

NLSTK_Errcode_E NLSTK_DevdStartScan(uint32_t scannerId, NLSTK_DevdScanSetting_S *setting,
    NLSTK_DevdScanFilter_S *filters, uint16_t filtersNum)
{
    HILOGI("[Mock NLSTK] NLSTK_DevdStartScan enter, scannerId: %{public}u, filtersNum: %{public}u", 
        scannerId, filtersNum);
    HILOGI("[Mock NLSTK] Scan mode: %{public}d, duration: %{public}d", 
        setting ? setting->scanMode : 0, setting ? setting->duration : 0);
    g_last_start_scan_id = scannerId;
    g_scan_started = true;
    g_start_scan_call_count++;
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdStopScan(uint32_t scannerId)
{
    HILOGI("[Mock NLSTK] NLSTK_DevdStopScan enter, scannerId: %{public}u", scannerId);
    g_last_stop_scan_id = scannerId;
    g_scan_stopped = true;
    g_stop_scan_call_count++;
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdStopAllScan(void)
{
    HILOGI("[Mock NLSTK] NLSTK_DevdStopAllScan enter");
    g_stop_all_scan_triggered = true;
    return NLSTK_ERRCODE_SUCCESS;
}

/* 测试辅助函数：手动触发回调 */

void MockTriggerScanStartEvent(NLSTK_Errcode_E resultCode)
{
    HILOGI("[Mock NLSTK] MockTriggerScanStartEvent, resultCode: %{public}d", resultCode);
    if (g_scan_callbacks.onStartOrStopEvent != NULL) {
        g_scan_callbacks.onStartOrStopEvent(resultCode, true);
    }
}

void MockTriggerScanStopEvent(NLSTK_Errcode_E resultCode)
{
    HILOGI("[Mock NLSTK] MockTriggerScanStopEvent, resultCode: %{public}d", resultCode);
    if (g_scan_callbacks.onStartOrStopEvent != NULL) {
        g_scan_callbacks.onStartOrStopEvent(resultCode, false);
    }
}

void MockTriggerScanResult(NLSTK_DevdAdvResult_S *result)
{
    HILOGI("[Mock NLSTK] MockTriggerScanResult, addr type: %{public}d", result ? result->addr.type : 0);
    if (g_scan_callbacks.onScanCallback != NULL && result != NULL) {
        g_scan_callbacks.onScanCallback(result);
    }
}

void MockResetStopAllScanFlag(void)
{
    g_stop_all_scan_triggered = false;
}

bool MockIsStopAllScanTriggered(void)
{
    return g_stop_all_scan_triggered;
}

/* 新增查询接口供测试使用 */
uint32_t MockGetLastStartScanId(void)
{
    return g_last_start_scan_id;
}

uint32_t MockGetLastStopScanId(void)
{
    return g_last_stop_scan_id;
}

uint32_t MockGetLastRemoveScanId(void)
{
    return g_last_remove_scan_id;
}

bool MockIsScanStarted(void)
{
    return g_scan_started;
}

bool MockIsScanStopped(void)
{
    return g_scan_stopped;
}

uint32_t MockGetStartScanCallCount(void)
{
    return g_start_scan_call_count;
}

uint32_t MockGetStopScanCallCount(void)
{
    return g_stop_scan_call_count;
}

void MockResetScanFlags(void)
{
    g_scan_started = false;
    g_scan_stopped = false;
    g_start_scan_call_count = 0;
    g_stop_scan_call_count = 0;
    g_last_start_scan_id = 0;
    g_last_stop_scan_id = 0;
    g_last_remove_scan_id = 0;
}
