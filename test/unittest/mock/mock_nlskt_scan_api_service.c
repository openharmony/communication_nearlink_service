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
#include "nai_log.h"
#include "nlstk_devd_api.h"
#include "nlstk_cfgdb_api.h"
#include "nlstk_scan_api.h"

// NLSTK_Devd* 系列 mock 相关全局变量
static uint8_t g_mock_module_id = 1;
static NLSTK_DevdScanCbk_S g_mock_scan_cbk = {0};
static uint32_t g_next_scanner_id = 1;

/* NLSTK_Devd* 系列 mock 实现 */

NLSTK_Errcode_E NLSTK_DevdRegScanModule(uint8_t *moduleId, NLSTK_DevdScanCbk_S *scanCbk)
{
    if (moduleId == NULL || scanCbk == NULL) {
        return NLSTK_ERRCODE_POINTER_NULL;
    }
    *moduleId = g_mock_module_id;
    g_mock_scan_cbk = *scanCbk;
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdAllocScannerId(uint8_t moduleId, uint32_t *scannerId)
{
    if (scannerId == NULL || moduleId != g_mock_module_id) {
        return NLSTK_ERRCODE_PARAM_ERR;
    }
    *scannerId = g_next_scanner_id++;
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdStartScan(uint32_t scannerId, NLSTK_DevdScanSetting_S *setting,
    NLSTK_DevdScanFilter_S *filters, uint16_t filtersNum)
{
    /* 触发 onStartOrStopEvent 回调 */
    if (g_mock_scan_cbk.onStartOrStopEvent != NULL) {
        g_mock_scan_cbk.onStartOrStopEvent(NLSTK_ERRCODE_SUCCESS, true);
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdStopScan(uint32_t scannerId)
{
    /* 触发 onStartOrStopEvent 回调 */
    if (g_mock_scan_cbk.onStartOrStopEvent != NULL) {
        g_mock_scan_cbk.onStartOrStopEvent(NLSTK_ERRCODE_SUCCESS, false);
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DevdStopAllScan(void)
{
    if (g_mock_scan_cbk.onStartOrStopEvent != NULL) {
        g_mock_scan_cbk.onStartOrStopEvent(NLSTK_ERRCODE_SUCCESS, false);
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_DevdDeregScanModule(uint8_t moduleId)
{
    g_mock_module_id = 0xFF;
    (void)memset_s(&g_mock_scan_cbk, sizeof(NLSTK_DevdScanCbk_S), 0x00, sizeof(NLSTK_DevdScanCbk_S));
}

void NLSTK_DevdRemoveScannerId(uint32_t scannerId)
{
    /* 空实现 */
}
