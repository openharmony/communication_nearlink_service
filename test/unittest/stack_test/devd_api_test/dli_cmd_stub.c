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
#include "dli_cmd.h"

#include "securec.h"

#include "sdf_mem.h"
#include "dli_log.h"
#include "dli_opcode.h"
#include "dli_errno.h"
#include "dli_def.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "dli_layer.h"
#include "dli_layer_stru.h"
#include "dli_event.h"
#include "dli_dev_discovery_event.h"
#include "dli_secu_event.h"
#include "dli_connect_event.h"
#include "dli_layer_utils.h"
#include "dli_layer_config.h"
#include "nlstk_schedule.h"
#include "nlstk_public_define.h"
#include "devd_cbk.h"
#include "nlstk_devd_def.h"

#define DLI_DATA_OFFSET 4
#define SLE_SHIFT_BITS_8 8
#define SLE_SHIFT_BITS_16 16
#define DLI_DEFAULT_OPCODE 0
#define CONN_BIT_FRAME_FORMAT_1_IND 0x01
#define CONN_BIT_FRAME_FORMAT_4_IND 0x08
#define DEFAULT_CONN_PILOT 0x0F
#define DEFAULT_FRAME_FORMAT_M_SEQ_IND 0x3F
#define DEFAULT_FRAME_TYPE_1_ADV_TIMEOUT 200
#define DEFAULT_FRAME_TYPE_4_ADV_TIMEOUT 2000
#define DEFAULT_FRAME_TYPE_4_CONN_SCAN_WINDOW 480
#define DEFAULT_FRAME_TYPE_4_CONN_SCAN_INTERVAL 480
#define DEFAULT_FRAME_4_CONN_INTERVAL 240
#define SCAN_PHY_COUNTT_MAX 2

#define SCAN_FRAME_TYPE_1 0x01
#define SCAN_FRAME_TYPE_4 0x02

// 新增Bit72定义, 增加白名单列表设备V2特性是否支持
#define DLI_LOCAL_FEATURE_CONN_BYPASS_ADV_INDEX 9
#define DLI_LOCAL_FEATURE_IS_SUPPORT_CONN_BYPASS_ADV 0x01

// 系统初始化阶段由NBC模块读取本端特性在回调后进行设置才能读取，否则无效
static DLI_LocalFeatures_S g_dliLocalFeatures = { 0 };

static uint32_t DLI_ExecuteCommand(uint16_t cmd, uint16_t event, void *inParam, uint16_t paramLen,
    DLI_ExecuteCmdCbk cbk, void *cbkContext, uint16_t cbkContextLen)
{
    DLI_LOGI("[test stub]dli execute cmd: 0x%04X, event: 0x%04X, paramLen: %hu", cmd, event, paramLen);
    return DLI_SUCCESS;
}

/* ---------------------------------------------设备发现命令-------------------------------------------------------*/
/* DEVD模块UT测试用例专用桩文件，在对应的dli设置 */

static void StubDevdSleSetScanParamCbk(void *param)
{
    DLI_AdvCbkContext cbkContext = {0};
    struct DLI_ExecuteCmdRetParam cmdRes = {0};
    DLI_LOGI("[DEVD_API_TEST] enter StubDevdSleSetScanParamCbk");
    DevdSleSetScanParamCbk(&cbkContext, DLI_SUCCESS, &cmdRes);
}

uint32_t DLI_SetScanParam(DLI_ScanParam *param)
{
    DLI_ScanParam *scanParam = SDF_MemZalloc(sizeof(DLI_ScanParam));
    (void)memcpy_s(scanParam, sizeof(DLI_ScanParam), param, sizeof(DLI_ScanParam));
    DLI_LOGI("[DEVD_API_TEST] enter stub DLI_SetScanParam");
    if (SchedulePostTask(StubDevdSleSetScanParamCbk, scanParam, SDF_MemFree) != NLSTK_OK) {
        DLI_LOGE("[DEVD_API_TEST] stub SchedulePostTask error");
        return DLI_UNKNOWN_COMMAND;
    }
    return DLI_SUCCESS;
}

static void StubDevdSleEnableScanCbk(void *param)
{
    DLI_AdvCbkContext cbkContext = {0};
    struct DLI_ExecuteCmdRetParam cmdRes = {0};
    DLI_LOGI("[DEVD_API_TEST] enter StubDevdSleSetScanParamCbk");
    DevdSleEnableScanCbk(&cbkContext, DLI_SUCCESS, &cmdRes);
}

uint32_t DLI_EnableScan(DLI_ScanEnable *param)
{
    if (param == NULL) {
        DLI_LOGE("[DEVD_API_TEST] scanEnable is null");
        return DLI_UNKNOWN_COMMAND;
    }
    DLI_LOGI("[DEVD_API_TEST] enter stub DLI_EnableScan, enable:%d", param->enable);
    DLI_ScanEnable *scanEnable = (DLI_ScanEnable *)SDF_MemZalloc(sizeof(DLI_ScanEnable));
    (void)memcpy_s(scanEnable, sizeof(DLI_ScanEnable), param, sizeof(DLI_ScanEnable));
    if (SchedulePostTask(StubDevdSleEnableScanCbk, scanEnable, SDF_MemFree) != NLSTK_OK) {
        DLI_LOGE("[DEVD_API_TEST] stub SchedulePostTask error");
        return DLI_UNKNOWN_COMMAND;
    }
    return DLI_SUCCESS;
}

static void StubDevdSetAdvParamCbk(void *param)
{
    DLI_AdvParam *advParam = (DLI_AdvParam *)param;
    uint16_t ret = DLI_UNKNOWN_COMMAND;
    if (advParam->advMode == ADV_MODE_CONNECTABLE_SCANABLE || advParam->advMode == ADV_MODE_NONCONN_SCANABLE) {
        ret = DLI_SUCCESS;
    }
    DLI_AdvCbkContext cbkContext = {.advHandle = advParam->advHandle};
    struct DLI_ExecuteCmdRetParam cmdRes = {0};
    DLI_LOGI("[DEVD_API_TEST] enter StubDevdSetAdvParamCbk, advHandle = %u, ret = %u", cbkContext.advHandle, ret);
    DevdSetAdvParamCbk(&cbkContext, DLI_SUCCESS, &cmdRes);
}

// 用于devd打桩验证
// advParam.advMode = ADV_MODE_CONNECTABLE_SCANABLE/ADV_MODE_NONCONN_SCANABLE返回成功，其他返回失败（可用于打桩控制返回值）
uint32_t DLI_SetAdvParam(DLI_AdvParam *param)
{
    if (param == NULL) {
        DLI_LOGE("[DEVD_API_TEST] advParam is null");
        return DLI_UNKNOWN_COMMAND;
    }
    DLI_LOGI("[DEVD_API_TEST] enter stub DLI_SetAdvParam");
    DLI_AdvParam *advParam = (DLI_AdvParam *)SDF_MemZalloc(sizeof(DLI_AdvParam));
    (void)memcpy_s(advParam, sizeof(DLI_AdvParam), param, sizeof(DLI_AdvParam));
    if (SchedulePostTask(StubDevdSetAdvParamCbk, advParam, SDF_MemFree) != NLSTK_OK) {
        DLI_LOGE("[DEVD_API_TEST] stub SchedulePostTask error");
        return DLI_UNKNOWN_COMMAND;
    }
    return DLI_SUCCESS;
}

static void StubDevdSetAdvDataCbk(void *param)
{
    DLI_AdvData *advData = (DLI_AdvData *)param;
    DLI_AdvCbkContext cbkContext = {.advHandle = advData->advHandle};
    struct DLI_ExecuteCmdRetParam cmdRes = {0};
    DLI_LOGI("[DEVD_API_TEST] enter StubDevdSetAdvDataCbk, advhandle = %u", cbkContext.advHandle);
    DevdSetAdvDataCbk(&cbkContext, DLI_SUCCESS, &cmdRes);
}

uint32_t DLI_SetAdvData(DLI_AdvData *advData)
{
    DLI_LOGI("[DEVD_API_TEST] enter stub DLI_SetAdvData");
    if (advData == NULL) {
        DLI_LOGE("[DEVD_API_TEST] advData is null");
        return DLI_UNKNOWN_COMMAND;
    }
    DLI_AdvData *advDataCbk = (DLI_AdvData *)SDF_MemZalloc(sizeof(DLI_AdvData));
    (void)memcpy_s(advDataCbk, sizeof(DLI_AdvData), advData, sizeof(DLI_AdvData));
    if (SchedulePostTask(StubDevdSetAdvDataCbk, advDataCbk, SDF_MemFree) != NLSTK_OK) {
        DLI_LOGE("[DEVD_API_TEST] stub SchedulePostTask error");
        return DLI_UNKNOWN_COMMAND;
    }
    return DLI_SUCCESS;
}

static void StubDevdEnableAdvCbk(void *param)
{
    DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)param;
    struct DLI_ExecuteCmdRetParam cmdRes = {0};
    DLI_LOGI("[DEVD_API_TEST] enter StubDevdEnableAdvCbk, advHandle = %u", cbkContext->advHandle);
    DevdEnableAdvCbk(cbkContext, DLI_SUCCESS, &cmdRes);
}

uint32_t DLI_EnableAdv(uint8_t advHandle, DLI_AdvEnable *advEnable)
{
    DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)SDF_MemZalloc(sizeof(DLI_AdvCbkContext));
    cbkContext->advHandle = advHandle;

    DLI_LOGI("[DEVD_API_TEST] enter stub DLI_EnableAdv");
    if (SchedulePostTask(StubDevdEnableAdvCbk, cbkContext, SDF_MemFree) != NLSTK_OK) {
        DLI_LOGE("[DEVD_API_TEST] stub SchedulePostTask error");
        return DLI_UNKNOWN_COMMAND;
    }
    return DLI_SUCCESS;
}

static void StubDevdRemoveAdvCbk(void *param)
{
    DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)param;
    struct DLI_ExecuteCmdRetParam cmdRes = {0};
    DLI_LOGI("[DEVD_API_TEST] enter StubDevdRemoveAdvCbk");
    DevdRemoveAdvCbk(cbkContext, DLI_SUCCESS, &cmdRes);
}

uint32_t DLI_RemoveAdvSet(uint8_t advHandle)
{
    DLI_AdvCbkContext *cbkContext = (DLI_AdvCbkContext *)SDF_MemZalloc(sizeof(DLI_AdvCbkContext));
    cbkContext->advHandle = advHandle;
    DLI_LOGI("advHandle = 0x%02X", advHandle);
    if (SchedulePostTask(StubDevdRemoveAdvCbk, cbkContext, SDF_MemFree) != NLSTK_OK) {
        DLI_LOGE("[DEVD_API_TEST] stub SchedulePostTask error");
        return DLI_UNKNOWN_COMMAND;
    }
    return DLI_SUCCESS;
}

bool DLI_IsSupportNewDisMeasure(void)
{
    return false;
}

uint32_t DLI_SetScanRspData(DLI_ScanRspData *scanRspData)
{
    DLI_LOGI("[DEVD_API_TEST] enter stub DLI_SetScanRspData");
    if (scanRspData == NULL) {
        DLI_LOGE("[DEVD_API_TEST] scanRspData is null");
        return DLI_UNKNOWN_COMMAND;
    }
    DLI_ScanRspData *advDataCbk = (DLI_ScanRspData *)SDF_MemZalloc(sizeof(DLI_ScanRspData));
    (void)memcpy_s(advDataCbk, sizeof(DLI_ScanRspData), scanRspData, sizeof(DLI_ScanRspData));
    if (SchedulePostTask(StubDevdSetAdvDataCbk, advDataCbk, SDF_MemFree) != NLSTK_OK) {
        DLI_LOGE("[DEVD_API_TEST] stub SchedulePostTask error");
        return DLI_UNKNOWN_COMMAND;
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