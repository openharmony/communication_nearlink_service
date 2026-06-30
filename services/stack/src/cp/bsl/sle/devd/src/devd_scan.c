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
#include <stdint.h>
#include "securec.h"
#include "sdf_mem.h"
#include "dli_cmd.h"
#include "dli_errno.h"
#include "dli_event_struct.h"
#include "nlstk_log.h"

#include "devd_cbk.h"
#include "devd_local.h"
#include "devd_scan.h"
#include "sdf_string.h"

// index:mcs
static const NLSTK_DevdMcsToModAndPolar_S g_mcsToModAndPolar[ADV_MAX_MCS] = {
    {ADV_MOD_GFSK, ADV_POLAR_1_4},
    {ADV_MOD_GFSK, ADV_POLAR_3_8},
    {ADV_MOD_QPSK, ADV_POLAR_1_4},
    {ADV_MOD_QPSK, ADV_POLAR_3_8},
    {ADV_MOD_QPSK, ADV_POLAR_1_2},
    {ADV_MOD_QPSK, ADV_POLAR_5_8},
    {ADV_MOD_QPSK, ADV_POLAR_3_4},
    {ADV_MOD_QPSK, ADV_POLAR_7_8},
    {ADV_MOD_QPSK, ADV_POLAR_NO},
    {ADV_MOD_8PSK, ADV_POLAR_5_8},
    {ADV_MOD_8PSK, ADV_POLAR_3_4},
    {ADV_MOD_8PSK, ADV_POLAR_7_8},
    {ADV_MOD_8PSK, ADV_POLAR_NO},
    {ADV_MOD_GFSK, ADV_POLAR_NO},
    {ADV_MOD_GFSK, ADV_POLAR_NO},
    {ADV_MOD_GFSK, ADV_POLAR_NO},
};

static void SleSetScanParamToDLI(NLSTK_DevdSleScanParams_S *scanParams)
{
    DLI_ScanParam *cmd = NULL;
    NLSTK_CHECK_RETURN_VOID(scanParams != NULL, "[DEVD]scanParams is null");

    uint8_t phyCount = DLI_GetPhyCountByFrameType(scanParams->frameType);
    cmd = (DLI_ScanParam *)SDF_MemZalloc(sizeof(DLI_ScanParam) + phyCount * sizeof(DLI_ScanParamCoreNoPhy));
    NLSTK_CHECK_RETURN_VOID(cmd != NULL, "[DEVD]setScamParams cmd malloc failed");
    cmd->ownAddrType = scanParams->localAddrType;
    cmd->scanFilterPolicy = scanParams->scanFilterPolicy;
    cmd->frameFormatInd = scanParams->frameType;
    for (uint8_t i = 0; i < phyCount; i++) {
        cmd->param[i].scanType = scanParams->params[i].scanType;
        cmd->param[i].scanInterval = scanParams->params[i].scanInterval;
        cmd->param[i].scanWindow = scanParams->params[i].scanWindow;
    }
    if (DEVD_SCAN_MGR.tmpScanParams != NULL) {
        SDF_MemFree(DEVD_SCAN_MGR.tmpScanParams);
        DEVD_SCAN_MGR.tmpScanParams = NULL;
    }
    DEVD_SCAN_MGR.tmpScanParams = (NLSTK_DevdSleScanParams_S *)SDF_MemZalloc(
        sizeof(NLSTK_DevdSleScanParams_S) + phyCount * sizeof(NLSTK_DevdSleScanParamsNoPhy_S));
    if (DEVD_SCAN_MGR.tmpScanParams == NULL) {
        NLSTK_LOG_ERROR("[DEVD]tmpScanParams malloc failed");
        SDF_MemFree(cmd);
        return;
    }
    DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_SET_SCAN_PARAMS;
    DEVD_SCAN_MGR.tmpScanParams->localAddrType = scanParams->localAddrType;
    DEVD_SCAN_MGR.tmpScanParams->scanFilterPolicy = scanParams->scanFilterPolicy;
    DEVD_SCAN_MGR.tmpScanParams->frameType = scanParams->frameType;
    for (uint8_t i = 0; i < phyCount; i++) {
        DEVD_SCAN_MGR.tmpScanParams->params[i].scanType = scanParams->params[i].scanType;
        DEVD_SCAN_MGR.tmpScanParams->params[i].scanInterval = scanParams->params[i].scanInterval;
        DEVD_SCAN_MGR.tmpScanParams->params[i].scanWindow = scanParams->params[i].scanWindow;
    }

    DLI_SetScanParam(cmd);

    SDF_MemFree(cmd);
    return;
}

void DevdSleSetScanParam(void *param)
{
    NLSTK_DevdSleScanParams_S *scanParams = (NLSTK_DevdSleScanParams_S *)param;
    NLSTK_CHECK_RETURN_VOID(scanParams != NULL, "[DEVD]scanParams is null");
    NLSTK_CHECK_RETURN_VOID(DEVD_SCAN_MGR.status == DEVD_SLE_STATUS_IDLE, "[DEVD]scan mgr is busy");
    SleSetScanParamToDLI(scanParams);
}

static void SleUpdateEnableScan(DLI_ScanEnable *cmd)
{
    NLSTK_CHECK_RETURN_VOID(cmd != NULL, "[DEVD]scanEnable cmd is null");
    NLSTK_LOG_INFO("[DEVD]cmd->enable=%d", cmd->enable);
    if (cmd->enable == DEVD_SLE_SCAN_STATUS_ENABLED) {
        DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_SET_SCAN_DISABLE2ENABLE;
        DLI_EnableScan(cmd);
    } else if (cmd->enable == DEVD_SLE_SCAN_STATUS_DISABLED) {
        DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_SET_SCAN_ENABLE2DISABLE;
        DLI_EnableScan(cmd);
    } else {
        NLSTK_LOG_ERROR("[DEVD] parameter error！");
    }
}

void DevdSleEnableScan(void *param)
{
    NLSTK_DevdSleScanEnable_S *scanEnable = (NLSTK_DevdSleScanEnable_S *)param;
    NLSTK_CHECK_RETURN_VOID(scanEnable != NULL, "[DEVD]scanEnable is null");
    NLSTK_CHECK_RETURN_VOID(DEVD_SCAN_MGR.status == DEVD_SLE_STATUS_IDLE, "[DEVD]scan mgr is busy");
    struct DLI_ScanEnable cmd;
    cmd.enable = scanEnable->scanEnable;
    cmd.filterDuplicates = scanEnable->scanFilterDuplicates;
    NLSTK_LOG_INFO("[DEVD]scanEnable = %d, filter_duplicates = %d", cmd.enable, cmd.filterDuplicates);
    SleUpdateEnableScan(&cmd);
}

void DevdRegScanEventCbk(void *param)
{
    NLSTK_DevdSleScanExterCbk_S *scanCbk = (NLSTK_DevdSleScanExterCbk_S *)param;
    NLSTK_CHECK_RETURN_VOID(scanCbk != NULL, "[DEVD]scanCbk is null");
    SCAN_EVENT_CBK = scanCbk->scanCbk;
    SCAN_REPORT_CBK = scanCbk->reportCbk;
    SCAN_FILTER_CBK = scanCbk->scanFilterCbk;
}

void DevdSleSetScanParamCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    if (status == DLI_SUCCESS) {
        NLSTK_LOG_INFO("[DEVD]Scan parameters set successfully");
        if (DEVD_SCAN_MGR.scanParams != NULL) {
            SDF_MemFree(DEVD_SCAN_MGR.scanParams);
        }
        DEVD_SCAN_MGR.scanParams = DEVD_SCAN_MGR.tmpScanParams;
        DEVD_SCAN_MGR.tmpScanParams = NULL;
        DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_IDLE;
        // 设置扫描参数事件结束并且成功后进行开启扫描操作
        NLSTK_DevdSleScanEnable_S scanEnable = {0};
        scanEnable.scanEnable = true;
        scanEnable.scanFilterDuplicates = false;    // 是否过滤重复广播包，暂未支持用户自定义
        DevdSleEnableScan(&scanEnable);
    } else {
        NLSTK_LOG_ERROR("[DEVD]Failed to set scan parameters, status: %d", status);
        if (DEVD_SCAN_MGR.tmpScanParams != NULL) {
            SDF_MemFree(DEVD_SCAN_MGR.tmpScanParams);
        }
        DEVD_SCAN_MGR.tmpScanParams = NULL;
        // 设置扫描参数失败，回报service开启扫描失败
        if (SCAN_EVENT_CBK != NULL) {
            NLSTK_CHECK_RETURN_VOID(status <= UINT8_MAX, "[DEVD]adv param too long");
            SCAN_EVENT_CBK(DEVD_CBK_EVENT_ENABLE_SCAN, (uint8_t)status);
        }
        DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_IDLE;
    }

    SDF_UNUSED(context);
    SDF_UNUSED(cmdRes);
}

void DevdSleEnableScanCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    uint8_t msg = DEVD_CBK_EVENT_DISABLE_SCAN;
    if (status == DLI_SUCCESS) {
        if (DEVD_SCAN_MGR.status == DEVD_SLE_STATUS_SET_SCAN_DISABLE2ENABLE) {
            msg = DEVD_CBK_EVENT_ENABLE_SCAN;
            NLSTK_LOG_INFO("[DEVD]Scan status set enabled");
        } else if (DEVD_SCAN_MGR.status == DEVD_SLE_STATUS_SET_SCAN_ENABLE2DISABLE) {
            msg = DEVD_CBK_EVENT_DISABLE_SCAN;
            NLSTK_LOG_INFO("[DEVD]Scan status set disabled");
        }
    } else {
        NLSTK_LOG_ERROR("[DEVD]Failed to set scan status, status: %d", status);
    }
    if (SCAN_EVENT_CBK != NULL) {
        SCAN_EVENT_CBK(msg, (uint8_t)status);
    }
    DEVD_SCAN_MGR.status = DEVD_SLE_STATUS_IDLE;
    SDF_UNUSED(context);
    SDF_UNUSED(cmdRes);
}

bool DevdSleScanReportCbkCheck(const struct DLI_ExecuteCmdRetParam *cmdRes)
{
    if (cmdRes == NULL) {
        NLSTK_LOG_ERROR("[DEVD]scan report cbk par null error");
        return false;
    }
    if (cmdRes->eventParameter == NULL) {
        NLSTK_LOG_ERROR("[DEVD]scan report cbk event_parameter null error");
        return false;
    }
    if (cmdRes->size < sizeof(struct DLI_AdvReportEvt)) {
        NLSTK_LOG_ERROR("[DEVD]scan report cmdRes size=%d error", cmdRes->size);
        return false;
    }
    return true;
}

void DevdSleScanReportCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    struct DLI_AdvReportEvt *par = NULL;

    SDF_UNUSED(context);
    SDF_UNUSED(status);
    if (!DevdSleScanReportCbkCheck(cmdRes)) {
        return;
    }

    par = (struct DLI_AdvReportEvt *)cmdRes->eventParameter;
    NLSTK_CHECK_RETURN_VOID(par->dataLength == cmdRes->size - sizeof(struct DLI_AdvReportEvt),
        "[DEVD]scan report data length error");
    NLSTK_DevdAdvReportInfo_S *report = SDF_MemZalloc(sizeof(NLSTK_DevdAdvReportInfo_S) +
        par->dataLength);
    NLSTK_CHECK_RETURN_VOID(report != NULL, "[DEVD]scan report malloc failed");

    (void)memcpy_s(report->addr, SLE_ADDR_LEN, par->addr, SLE_ADDR_LEN);
    (void)memcpy_s(report->extendParams.directAddr, SLE_ADDR_LEN, par->directAddr, SLE_ADDR_LEN);
    if (par->dataLength != 0) {
        (void)memcpy_s(report->data, par->dataLength, par->data, par->dataLength);
    }

    report->addrType = par->addrType;
    report->rssi = par->rssi;
    report->dataLength = par->dataLength;
    /* see definition in struct gle_hci_adv_report */
    report->extendParams.eventType = par->eventType & LOW_BITS_5;
    /* see definition in struct gle_hci_adv_report */
    report->extendParams.dataStatus = (par->eventType >> SHIFT_BITS_5) & LOW_BITS_2;
    report->extendParams.directAddrType = par->directAddrType;
    report->extendParams.primFrameType = par->primFrameType;
    report->extendParams.primPhy = 0;
    report->extendParams.secondPhy = par->secondPhy;
    report->extendParams.secondFrameType = par->secondFrameType;
    if (par->secondMcs < ADV_MAX_MCS) {
        report->extendParams.secondMod = g_mcsToModAndPolar[par->secondMcs].mod;
        report->extendParams.secondPolar = g_mcsToModAndPolar[par->secondMcs].polar;
    }
    report->extendParams.secondPilotRatio = par->secondPilotRatio;

    if (SCAN_REPORT_CBK != NULL) {
        SCAN_REPORT_CBK(report);
    }
    SDF_MemFree(report);
}

void DevdSleSetScanFilterCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes)
{
    DLI_SetScanFilterEvt *par = (DLI_SetScanFilterEvt *)cmdRes->eventParameter;
    NLSTK_CHECK_RETURN_VOID(par != NULL, "[DEVD]scan filter eventParameter null");
    NLSTK_LOG_INFO("[DEVD]sle set scan filter cbk ret=%hu, subCode=%hhu, numAvailable=%hhu",
        status, par->subCode, par->numAvailable);
    NLSTK_DevdScanFilterInfo_S param = {0};
    param.status = status;
    param.subCode = par->subCode;
    param.numAvailable = par->numAvailable;
    if (SCAN_FILTER_CBK != NULL) {
        SCAN_FILTER_CBK(&param);
    }
}