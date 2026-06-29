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
#include "sdf_mem.h"
#include "hid_def.h"
#include "hid_type.h"
#include "hid_utils.h"

static uint8_t g_ssapStdBaseUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

typedef struct {
    HidPropertyType_E type;
    uint16_t uuid;
} HidPropertyTypeUuid_S;

static HidPropertyTypeUuid_S g_hidPropertyTypeUuid[HID_PROPERTY_TYPE_BUFF] = {
    { HID_TYPE_AND_FORMAT_DESC,     HID_REPORT_MAP_UUID },
    { HID_WORK_STATUS_INDICATION,   HID_WORK_STATE_UUID },
    { HID_REPORT_INDEX_INFO,        HID_REPORT_INDEX_UUID },
    { HID_INPUT_REPORT_INFO,        HID_INPUT_REPORT_UUID },
    { HID_OUTPUT_REPORT_INFO,       HID_OUTPUT_REPORT_UUID },
    { HID_FEATURE_REPORT_INFO,      HID_FEATURE_REPORT_UUID },
};

static HidPropertyTypeUuid_S g_hidPropertyTypeUuidPen[HID_PROPERTY_TYPE_BUFF] = {
    { HID_TYPE_AND_FORMAT_DESC,     HID_REPORT_MAP_UUID_PEN },
    { HID_WORK_STATUS_INDICATION,   HID_WORK_STATE_UUID_PEN },
    { HID_REPORT_INDEX_INFO,        HID_REPORT_INDEX_UUID_PEN },
    { HID_INPUT_REPORT_INFO,        HID_INPUT_REPORT_UUID_PEN },
    { HID_OUTPUT_REPORT_INFO,       HID_OUTPUT_REPORT_UUID_PEN },
    { HID_FEATURE_REPORT_INFO,      HID_FEATURE_REPORT_UUID_PEN },
};

HidPropertyType_E HidGetPropertyTypeByUuid(uint16_t uuid)
{
    for (int i = 0; i < HID_PROPERTY_TYPE_BUFF; i++) {
        if (g_hidPropertyTypeUuid[i].uuid == uuid) {
            return g_hidPropertyTypeUuid[i].type;
        }
    }
    for (int i = 0; i < HID_PROPERTY_TYPE_BUFF; i++) {
        if (g_hidPropertyTypeUuidPen[i].uuid == uuid) {
            return g_hidPropertyTypeUuidPen[i].type;
        }
    }
    return HID_PROPERTY_TYPE_BUFF;
}

NLSTK_SsapUuid_S HidConvertUuidToStru(uint16_t uuid)
{
    NLSTK_SsapUuid_S uuidStru;
    for (int i = 0; i < HID_UUID_FIFTEENTH_BYTE; i++) {
        uuidStru.uuid[i] = g_ssapStdBaseUuid[i];
    }
    uuidStru.uuid[HID_UUID_FIFTEENTH_BYTE] = (uint8_t)((uuid & 0xFF00) >> 0x08);
    uuidStru.uuid[HID_UUID_SIXTEENTH_BYTE] = (uint8_t)(uuid & 0x00FF);
    return uuidStru;
}

uint16_t HidConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru)
{
    uint16_t uuid = 0;
    uuid |= (uint16_t)(uuidStru.uuid[HID_UUID_FIFTEENTH_BYTE]) << 0x08;
    uuid |= (uint16_t)(uuidStru.uuid[HID_UUID_SIXTEENTH_BYTE]);
    return uuid;
}

bool HidCompAppId(void *ptr, void *args)
{
    HidDevice_S *dev = (HidDevice_S *)ptr;
    int32_t appId = *(int32_t *)args;
    return dev->appId == appId;
}

bool HidCompAddr(void *ptr, void *args)
{
    HidDevice_S *dev = (HidDevice_S *)ptr;
    SLE_Addr_S *addr = (SLE_Addr_S *)args;
    return (SDF_CompareSleAddr(&dev->addr, addr) == 0);
}

bool HidCompHandle(void *ptr, void *args)
{
    HidReport_S *report = (HidReport_S *)ptr;
    uint16_t handle = *(uint16_t *)args;
    return report->reportHandle == handle;
}

bool HidCompReportIdAndType(void *ptr, void *args)
{
    HidReport_S *report = (HidReport_S *)ptr;
    HidReportIdAndType_S reportIdAndType = *(HidReportIdAndType_S *)args;
    return report->reportId == reportIdAndType.reportId && report->reportType == reportIdAndType.reportType;
}

void HidFreeWriteParam(void *ptr)
{
    HidWriteParam_S *param = (HidWriteParam_S *)ptr;
    if (param == NULL) {
        return;
    }
    if (param->value != NULL) {
        if (param->value->data != NULL) {
            SDF_MemFree(param->value->data);
        }
        SDF_MemFree(param->value);
    }
    SDF_MemFree(param);
}

void HidFreeReport(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    HidReport_S *report = (HidReport_S *)ptr;
    if (report->reportInfoValue.data != NULL) {
        SDF_MemFree(report->reportInfoValue.data);
    }
    SDF_MemFree(report);
}

void HidFreeDevice(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    HidDevice_S *dev = (HidDevice_S *)ptr;
    if (dev->desc.desc != NULL) {
        SDF_MemFree(dev->desc.desc);
    }
    SDF_DestroyVector(dev->service.indexHandle);
    SDF_DestroyVector(dev->report);
    SDF_MemFree(dev);
}

NLSTK_VariableData_S *HidValueConvert(HidPropertyType_E type, void *value)
{
    NLSTK_VariableData_S *val = SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(val != NULL, NULL, "[HID] val malloc fail");
    switch (type) {
        case HID_WORK_STATUS_INDICATION: {
            uint8_t *workStatus = (uint8_t *)value;
            val->len = sizeof(uint8_t);
            val->data = (uint8_t *)SDF_MemZalloc(val->len);
            if (val->data == NULL) {
                NLSTK_LOG_ERROR("[HID] val data malloc fail");
                SDF_MemFree(val);
                return NULL;
            }
            *val->data = *workStatus;
            break;
        }
        case HID_OUTPUT_REPORT_INFO:
        case HID_FEATURE_REPORT_INFO: {
            HidReportInfo_S *info = (HidReportInfo_S *)value;
            if (info->reportInfoValue.len == 0 || info->reportInfoValue.data == NULL) {
                NLSTK_LOG_ERROR("[HID] value data is null");
                SDF_MemFree(val);
                return NULL;
            }
            val->len = info->reportInfoValue.len;
            val->data = (uint8_t *)SDF_MemZalloc(val->len);
            if (val->data == NULL) {
                NLSTK_LOG_ERROR("[HID] val data malloc fail");
                SDF_MemFree(val);
                return NULL;
            }
            (void)memcpy_s(val->data, val->len, info->reportInfoValue.data, info->reportInfoValue.len);
            break;
        }
        default: {
            NLSTK_LOG_ERROR("[HID] property type error");
            SDF_MemFree(val);
            return NULL;
        }
    }
    return val;
}