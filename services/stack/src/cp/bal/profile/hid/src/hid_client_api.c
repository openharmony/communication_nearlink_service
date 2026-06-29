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
#include "securec.h"
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "nlstk_schedule.h"
#include "sdf_mem.h"
#include "hid_type.h"
#include "hid_utils.h"
#include "hid_client.h"
#include "hid_client_api.h"

NLSTK_Errcode_E HidRegClientCbk(HidClientCallBack_S *clientCallback)
{
    NLSTK_CHECK_RETURN(clientCallback != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HID] cbk is null");
    HidClientCallBack_S *copyCbk = (HidClientCallBack_S *)SDF_MemZalloc(sizeof(HidClientCallBack_S));
    NLSTK_CHECK_RETURN(copyCbk != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[HID] cbk malloc fail");
    (void)memcpy_s(copyCbk, sizeof(HidClientCallBack_S), clientCallback, sizeof(HidClientCallBack_S));
    NLSTK_CHECK_RETURN(SchedulePostTask(HidRegClientCbkInner, copyCbk, SDF_MemFree) == NLSTK_OK,
        NLSTK_ERRCODE_TASK_FAIL, "[HID] SchedulePostTask failed");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E HidConnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HID] addr is null");
    SLE_Addr_S *copyAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(copyAddr != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[HID] addr malloc fail");
    (void)memcpy_s(copyAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(SchedulePostTask(HidConnectInner, copyAddr, SDF_MemFree) == NLSTK_OK,
        NLSTK_ERRCODE_TASK_FAIL, "[HID] SchedulePostTask failed");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E HidDisconnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HID] addr is null");
    SLE_Addr_S *copyAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(copyAddr != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[HID] addr malloc fail");
    (void)memcpy_s(copyAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(SchedulePostTask(HidDisconnectInner, copyAddr, SDF_MemFree) == NLSTK_OK,
        NLSTK_ERRCODE_TASK_FAIL, "[HID] SchedulePostTask failed");
    return NLSTK_ERRCODE_SUCCESS;
}

static void FreeHidGeInfoParam(void *arg)
{
    if (arg == NULL) {
        return;
    }
    HidGetInfoParam_S *param = (HidGetInfoParam_S *)arg;
    if (param->info != NULL && param->freeFunc != NULL) {
        param->freeFunc(param->info);
    }
    SDF_MemFree(param);
}

NLSTK_Errcode_E HidGetInformation(SLE_Addr_S *addr, HidInformation_S **info, HidFreeFunc *freeFunc)
{
    NLSTK_CHECK_RETURN(addr != NULL && info != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HID] addr or info is null");
    HidGetInfoParam_S *param = (HidGetInfoParam_S *)SDF_MemZalloc(sizeof(HidGetInfoParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[HID] param malloc fail");
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    uint32_t ret = SchedulePostTaskBlocked(HidGetInformationInner, param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        NLSTK_LOG_ERROR("[HID] SchedulePostTaskBlocked timeout");
        (void)SchedulePostTask(FreeHidGeInfoParam, param, NULL);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK) {
        NLSTK_LOG_ERROR("[HID] SchedulePostTaskBlocked failed");
        SDF_MemFree(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *info = param->info;
    *freeFunc = param->freeFunc;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E HidReadProperty(SLE_Addr_S *addr, HidPropertyType_E type, HidReportIdAndType_S *reportIdAndType)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HID] addr is null");
    NLSTK_CHECK_RETURN(type >= HID_TYPE_AND_FORMAT_DESC && type < HID_PROPERTY_TYPE_BUFF,
        NLSTK_ERRCODE_PARAM_ERR, "[HID] property type error");
    uint8_t reportId = 0;
    uint8_t reportType = 0;
    // 仅当属性为报告信息时，reportId和reportType字段有效
    if (type == HID_INPUT_REPORT_INFO || type == HID_OUTPUT_REPORT_INFO || type == HID_FEATURE_REPORT_INFO) {
        NLSTK_CHECK_RETURN(reportIdAndType != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HID] report id and type is null");
        reportId = reportIdAndType->reportId;
        reportType = reportIdAndType->reportType;
    }
    HidReadParam_S *param = (HidReadParam_S *)SDF_MemZalloc(sizeof(HidReadParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[HID] param malloc fail");
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    param->type = type;
    param->reportId = reportId;
    param->reportType = reportType;
    NLSTK_CHECK_RETURN(SchedulePostTask(HidReadPropertyInner, param, SDF_MemFree) == NLSTK_OK,
        NLSTK_ERRCODE_TASK_FAIL, "[HID] SchedulePostTask failed");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E HidWriteProperty(SLE_Addr_S *addr, HidPropertyType_E type, void *value)
{
    NLSTK_CHECK_RETURN(addr != NULL && value != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HID] addr or value is null");
    // 支持写入的属性：输出报告信息、特性报告信息、工作状态指示
    NLSTK_CHECK_RETURN(type == HID_OUTPUT_REPORT_INFO || type == HID_FEATURE_REPORT_INFO ||
        type == HID_WORK_STATUS_INDICATION, NLSTK_ERRCODE_UNSUPPORTED, "[HID] property type error");
    uint8_t reportId = 0;
    uint8_t reportType = 0;
    // 仅当属性为报告信息时，reportId和reportType字段有效
    if (type == HID_OUTPUT_REPORT_INFO || type == HID_FEATURE_REPORT_INFO) {
        HidReportInfo_S *info = (HidReportInfo_S *)value;
        reportId = info->reportIdAndType.reportId;
        reportType = info->reportIdAndType.reportType;
    }
    NLSTK_VariableData_S *val = HidValueConvert(type, value);
    NLSTK_CHECK_RETURN(val != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HID] value convert fail");
    HidWriteParam_S *param = (HidWriteParam_S *)SDF_MemZalloc(sizeof(HidWriteParam_S));
    if (param == NULL) {
        NLSTK_LOG_ERROR("[HID] param malloc fail");
        SDF_MemFree(val->data);
        SDF_MemFree(val);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    param->type = type;
    param->reportId = reportId;
    param->reportType = reportType;
    param->value = val;
    NLSTK_CHECK_RETURN(SchedulePostTask(HidWritePropertyInner, param, HidFreeWriteParam) == NLSTK_OK,
        NLSTK_ERRCODE_TASK_FAIL, "[HID] SchedulePostTask failed");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E HidGetConnectState(SLE_Addr_S *addr, uint8_t *state)
{
    NLSTK_CHECK_RETURN(addr != NULL && state != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HID] addr or state is null");
    HidGetConnStateParam_S *param = (HidGetConnStateParam_S *)SDF_MemZalloc(sizeof(HidGetConnStateParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[HID] param malloc fail");
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    uint32_t ret = SchedulePostTaskBlocked(HidGetConnectStateInner, param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        NLSTK_LOG_ERROR("[HID] SchedulePostTaskBlocked timeout");
        (void)SchedulePostTask(SDF_MemFree, param, NULL);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK) {
        NLSTK_LOG_ERROR("[HID] SchedulePostTaskBlocked failed");
        SDF_MemFree(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *state = param->state;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}

static void FreeHidGetConnDevParam(void *arg)
{
    if (arg == NULL) {
        return;
    }
    HidGetConnDevParam_S *param = (HidGetConnDevParam_S *)arg;
    if (param->addrs != NULL && param->freeFunc != NULL) {
        param->freeFunc(param->addrs);
    }
    SDF_MemFree(param);
}

NLSTK_Errcode_E HidGetConnectedDevice(SLE_Addr_S **addrs, size_t *num, HidFreeFunc *freeFunc)
{
    NLSTK_CHECK_RETURN(addrs != NULL && num != NULL, NLSTK_ERRCODE_POINTER_NULL, "[HID] addrs or num is null");
    HidGetConnDevParam_S *param = (HidGetConnDevParam_S *)SDF_MemZalloc(sizeof(HidGetConnDevParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[HID] param malloc fail");
    uint32_t ret = SchedulePostTaskBlocked(HidGetConnectedDeviceInner, param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        NLSTK_LOG_ERROR("[HID] SchedulePostTaskBlocked timeout");
        (void)SchedulePostTask(FreeHidGetConnDevParam, param, NULL);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK) {
        NLSTK_LOG_ERROR("[HID] SchedulePostTaskBlocked failed");
        SDF_MemFree(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *addrs = param->addrs;
    *num = param->num;
    *freeFunc = param->freeFunc;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}