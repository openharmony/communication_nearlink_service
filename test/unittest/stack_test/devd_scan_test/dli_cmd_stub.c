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
#include "nlstk_schedule.h"
#include "dli_cmd.h"
#include "dli_errno.h"
#include "devd_cbk.h"
#include "devd_scan_test_ctrl.h"

// 链接闭包依赖编译器裁剪不可达节（cfgdb/cm 等保留源引用本桩未覆盖的 DLI 读命令）；链接报缺符号时优先在本桩补齐

uint8_t g_dliStubSetAdvParamReply;
uint16_t g_dliStubSetAdvParamStatus;
uint8_t g_dliStubSetAdvDataReply;
uint16_t g_dliStubSetAdvDataStatus;
uint8_t g_dliStubSetScanRspReply;
uint16_t g_dliStubSetScanRspStatus;
uint8_t g_dliStubEnableAdvReply;
uint16_t g_dliStubEnableAdvStatus;
uint8_t g_dliStubRemoveAdvReply;
uint16_t g_dliStubRemoveAdvStatus;
uint8_t g_dliStubSetScanParamReply;
uint16_t g_dliStubSetScanParamStatus;
uint8_t g_dliStubEnableScanReply;
uint16_t g_dliStubEnableScanStatus;
uint8_t g_dliStubEnableAdvFail;

uint8_t g_dliStubAdvParamSecondFormat;
uint16_t g_dliStubAdvParamTimeout;
uint16_t g_dliStubAdvParamConnMin;
uint16_t g_dliStubAdvParamConnMax;
uint8_t g_dliStubAdvDataHandle;
uint8_t g_dliStubAdvDataOp;
uint16_t g_dliStubAdvDataLen;
uint8_t g_dliStubAdvDataPayload[TEST_DLI_STUB_MAX_PAYLOAD];
uint32_t g_dliStubSetAdvDataCnt;
uint8_t g_dliStubScanRspHandle;
uint8_t g_dliStubScanRspOp;
uint8_t g_dliStubEnableAdvHandle;
uint8_t g_dliStubEnableAdvEnable;
uint32_t g_dliStubEnableAdvCnt;
uint8_t g_dliStubRemoveAdvHandle;
uint8_t g_dliStubScanParamFrameType;
uint16_t g_dliStubScanParamWindow[2];
uint16_t g_dliStubScanParamInterval[2];
uint8_t g_dliStubEnableScanEnable;
uint32_t g_dliStubSetScanParamCnt;
uint32_t g_dliStubEnableScanCnt;

void TEST_DevdDliStubReset(void)
{
    g_dliStubSetAdvParamReply = 0;
    g_dliStubSetAdvDataReply = 0;
    g_dliStubSetScanRspReply = 0;
    g_dliStubEnableAdvReply = 0;
    g_dliStubRemoveAdvReply = 0;
    g_dliStubSetScanParamReply = 0;
    g_dliStubEnableScanReply = 0;
    g_dliStubEnableAdvFail = 0;
    g_dliStubSetAdvDataCnt = 0;
    g_dliStubEnableAdvCnt = 0;
    g_dliStubSetScanParamCnt = 0;
    g_dliStubEnableScanCnt = 0;
    g_dliStubAdvParamSecondFormat = 0;
    g_dliStubAdvParamTimeout = 0;
    g_dliStubAdvParamConnMin = 0;
    g_dliStubAdvParamConnMax = 0;
    g_dliStubAdvDataHandle = 0;
    g_dliStubAdvDataOp = 0;
    g_dliStubAdvDataLen = 0;
    (void)memset_s(g_dliStubAdvDataPayload, TEST_DLI_STUB_MAX_PAYLOAD, 0, TEST_DLI_STUB_MAX_PAYLOAD);
    g_dliStubScanRspHandle = 0;
    g_dliStubScanRspOp = 0;
    g_dliStubEnableAdvHandle = 0;
    g_dliStubEnableAdvEnable = 0;
    g_dliStubRemoveAdvHandle = 0;
    g_dliStubScanParamFrameType = 0;
    (void)memset_s(g_dliStubScanParamWindow, sizeof(g_dliStubScanParamWindow), 0, sizeof(g_dliStubScanParamWindow));
    (void)memset_s(g_dliStubScanParamInterval, sizeof(g_dliStubScanParamInterval), 0,
        sizeof(g_dliStubScanParamInterval));
    g_dliStubEnableScanEnable = 0;
}

typedef struct {
    uint8_t advHandle;
    uint16_t status;
} StubReplyArg_S;

static uint32_t StubPostReply(void (*cbk)(void *, uint16_t, struct DLI_ExecuteCmdRetParam *),
    uint8_t advHandle, uint16_t status)
{
    StubReplyArg_S *arg = (StubReplyArg_S *)SDF_MemZalloc(sizeof(StubReplyArg_S));
    if (arg == NULL) {
        return DLI_UNKNOWN_COMMAND;
    }
    arg->advHandle = advHandle;
    arg->status = status;
    if (SchedulePostTask(cbk, arg, SDF_MemFree) != 0) {
        SDF_MemFree(arg);
        return DLI_UNKNOWN_COMMAND;
    }
    return DLI_SUCCESS;
}

static void StubAdvParamCbk(void *param)
{
    StubReplyArg_S *arg = (StubReplyArg_S *)param;
    DLI_AdvCbkContext ctx = {.advHandle = arg->advHandle};
    DevdSetAdvParamCbk(&ctx, arg->status, NULL);
}

static void StubAdvDataCbk(void *param)
{
    StubReplyArg_S *arg = (StubReplyArg_S *)param;
    DLI_AdvCbkContext ctx = {.advHandle = arg->advHandle};
    DevdSetAdvDataCbk(&ctx, arg->status, NULL);
}

static void StubEnableAdvCbk(void *param)
{
    StubReplyArg_S *arg = (StubReplyArg_S *)param;
    DLI_AdvCbkContext ctx = {.advHandle = arg->advHandle};
    DevdEnableAdvCbk(&ctx, arg->status, NULL);
}

static void StubRemoveAdvCbk(void *param)
{
    StubReplyArg_S *arg = (StubReplyArg_S *)param;
    DLI_AdvCbkContext ctx = {.advHandle = arg->advHandle};
    DevdRemoveAdvCbk(&ctx, arg->status, NULL);
}

static void StubScanParamCbk(void *param)
{
    StubReplyArg_S *arg = (StubReplyArg_S *)param;
    DevdSleSetScanParamCbk(NULL, arg->status, NULL);
}

static void StubEnableScanCbk(void *param)
{
    StubReplyArg_S *arg = (StubReplyArg_S *)param;
    DevdSleEnableScanCbk(NULL, arg->status, NULL);
}

uint32_t DLI_SetAdvParam(DLI_AdvParam *param)
{
    if (param == NULL) {
        return DLI_UNKNOWN_COMMAND;
    }
    g_dliStubAdvParamSecondFormat = param->secondAdvFrameFormat;
    g_dliStubAdvParamTimeout = param->supervisionTimeout;
    g_dliStubAdvParamConnMin = param->connIntervalMin;
    g_dliStubAdvParamConnMax = param->connIntervalMax;
    if (g_dliStubSetAdvParamReply != 0) {
        return StubPostReply(StubAdvParamCbk, param->advHandle, g_dliStubSetAdvParamStatus);
    }
    return DLI_SUCCESS;
}

uint32_t DLI_SetAdvData(DLI_AdvData *advData)
{
    if (advData == NULL) {
        return DLI_UNKNOWN_COMMAND;
    }
    g_dliStubSetAdvDataCnt++;
    g_dliStubAdvDataHandle = advData->advHandle;
    g_dliStubAdvDataOp = advData->operation;
    g_dliStubAdvDataLen = advData->advDataLen;
    uint16_t copyLen = advData->advDataLen;
    if (copyLen > TEST_DLI_STUB_MAX_PAYLOAD) {
        copyLen = TEST_DLI_STUB_MAX_PAYLOAD;
    }
    (void)memset_s(g_dliStubAdvDataPayload, TEST_DLI_STUB_MAX_PAYLOAD, 0, TEST_DLI_STUB_MAX_PAYLOAD);
    if (copyLen != 0) {
        (void)memcpy_s(g_dliStubAdvDataPayload, TEST_DLI_STUB_MAX_PAYLOAD, advData->advData, copyLen);
    }
    if (g_dliStubSetAdvDataReply != 0) {
        return StubPostReply(StubAdvDataCbk, advData->advHandle, g_dliStubSetAdvDataStatus);
    }
    return DLI_SUCCESS;
}

uint32_t DLI_SetScanRspData(DLI_ScanRspData *scanRspData)
{
    if (scanRspData == NULL) {
        return DLI_UNKNOWN_COMMAND;
    }
    g_dliStubScanRspHandle = scanRspData->advHandle;
    g_dliStubScanRspOp = scanRspData->operation;
    if (g_dliStubSetScanRspReply != 0) {
        return StubPostReply(StubAdvDataCbk, scanRspData->advHandle, g_dliStubSetScanRspStatus);
    }
    return DLI_SUCCESS;
}

uint32_t DLI_EnableAdv(uint8_t advHandle, DLI_AdvEnable *advEnable)
{
    if (g_dliStubEnableAdvFail != 0) {
        return DLI_UNKNOWN_COMMAND;
    }
    g_dliStubEnableAdvCnt++;
    g_dliStubEnableAdvHandle = advHandle;
    g_dliStubEnableAdvEnable = (advEnable != NULL) ? advEnable->enable : 0;
    if (g_dliStubEnableAdvReply != 0) {
        return StubPostReply(StubEnableAdvCbk, advHandle, g_dliStubEnableAdvStatus);
    }
    return DLI_SUCCESS;
}

uint32_t DLI_RemoveAdvSet(uint8_t advHandle)
{
    g_dliStubRemoveAdvHandle = advHandle;
    if (g_dliStubRemoveAdvReply != 0) {
        return StubPostReply(StubRemoveAdvCbk, advHandle, g_dliStubRemoveAdvStatus);
    }
    return DLI_SUCCESS;
}

uint32_t DLI_SetScanParam(DLI_ScanParam *scanParam)
{
    if (scanParam == NULL) {
        return DLI_UNKNOWN_COMMAND;
    }
    g_dliStubSetScanParamCnt++;
    g_dliStubScanParamFrameType = scanParam->frameFormatInd;
    uint8_t phyCount = DLI_GetPhyCountByFrameType(scanParam->frameFormatInd);
    if (phyCount > 2) {
        phyCount = 2;
    }
    for (uint8_t i = 0; i < phyCount; i++) {
        g_dliStubScanParamWindow[i] = scanParam->param[i].scanWindow;
        g_dliStubScanParamInterval[i] = scanParam->param[i].scanInterval;
    }
    if (g_dliStubSetScanParamReply != 0) {
        return StubPostReply(StubScanParamCbk, 0, g_dliStubSetScanParamStatus);
    }
    return DLI_SUCCESS;
}

uint32_t DLI_EnableScan(DLI_ScanEnable *scanEnable)
{
    if (scanEnable == NULL) {
        return DLI_UNKNOWN_COMMAND;
    }
    g_dliStubEnableScanCnt++;
    g_dliStubEnableScanEnable = scanEnable->enable;
    if (g_dliStubEnableScanReply != 0) {
        return StubPostReply(StubEnableScanCbk, 0, g_dliStubEnableScanStatus);
    }
    return DLI_SUCCESS;
}

uint8_t DLI_GetPhyCountByFrameType(uint8_t frameType)
{
    uint8_t phyCount = 0;
    if ((frameType & SCAN_FRAME_TYPE_1) != 0) {
        phyCount++;
    }
    if ((frameType & SCAN_FRAME_TYPE_4) != 0) {
        phyCount++;
    }
    return phyCount;
}
