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
#include "nlstk_devd.h"
#include "devd_cbk.h"
#include "devd_local.h"
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "devd_adv.h"
#include "dli.h"
#include "dli_errno.h"

static uint32_t RegDliCbk(void)
{
    const DLI_CbkLineStru cbkTable[] = {
        {DLI_CBK_SET_ADV_PARAMS, (void *)DevdSetAdvParamCbk},
        {DLI_CBK_SET_ADV_DATA, (void *)DevdSetAdvDataCbk},
        {DLI_CBK_ENABLE_ADV, (void *)DevdEnableAdvCbk},
        {DLI_CBK_REMOVE_ADV, (void *)DevdRemoveAdvCbk},
        {DLI_CBK_ADV_TERMINATED, (void *)DevdAdvTerminatedCbk},
        {DLI_CBK_SET_SCAN_PARAMS, (void *)DevdSleSetScanParamCbk},
        {DLI_CBK_ENABLE_SCAN, (void *)DevdSleEnableScanCbk},
        {DLI_CBK_ADV_REPORT, (void *)DevdSleScanReportCbk},
        {DLI_CBK_SET_SCAN_FILTER, (void *)DevdSleSetScanFilterCbk},
        {DLI_CBK_SET_TX_POWER, (void *)DevdSetTxPowerCbk},
    };
    uint32_t ret = DLI_CmdCbkReg(DEVD, NULL, 0, cbkTable, sizeof(cbkTable) / sizeof(DLI_CbkLineStru));

    return ret;
}

uint32_t DevdInit(void)
{
    if (RegDliCbk() != DLI_SUCCESS) {
        NLSTK_LOG_ERROR("[DEVD] reg dli cbk failed.");
        return NLSTK_ERR;
    }
    DevdLocalDeviceInit();
    return NLSTK_OK;
}

void DevdDeInit(void)
{
    DevdLocalDeviceDeInit();
}

void DevdEnable(void)
{
    DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_IDLE;
}