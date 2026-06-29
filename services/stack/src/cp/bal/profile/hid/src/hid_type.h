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
#ifndef HID_TYPE_H
#define HID_TYPE_H

#include <stdbool.h>
#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_ssap_app_client.h"
#include "hid_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HID_UUID_FIFTEENTH_BYTE 14
#define HID_UUID_SIXTEENTH_BYTE 15
#define HID_REPORT_INDEX_INFO_LEN 8
#define HID_MAX_SERVICE_NUM 0xFF
#define HID_MAX_DATA_LEN 0xFFFF
#define HID_INVALID_APPID (-1)

// For HID Service
typedef enum {
    HID_SERVICE_UUID = 0x060B,
    HID_REPORT_MAP_UUID = 0x1039,
    HID_WORK_STATE_UUID = 0x103A,
    HID_REPORT_INDEX_UUID = 0x103B,
    HID_INPUT_REPORT_UUID = 0x103C,
    HID_OUTPUT_REPORT_UUID = 0x103D,
    HID_FEATURE_REPORT_UUID = 0x103E,
} HidStdUuid_E;

// big-endian to compatible with old devices
typedef enum {
    HID_SERVICE_UUID_PEN = 0x0B06,
    HID_REPORT_MAP_UUID_PEN = 0x3910,
    HID_WORK_STATE_UUID_PEN = 0x3A10,
    HID_REPORT_INDEX_UUID_PEN = 0x3B10,
    HID_INPUT_REPORT_UUID_PEN = 0x3C10,
    HID_OUTPUT_REPORT_UUID_PEN = 0x3D10,
    HID_FEATURE_REPORT_UUID_PEN = 0x3E10,
} HidStdUuidPen_E;

typedef struct {
    SLE_Addr_S addr;
    HidPropertyType_E type;
    uint8_t reportId;
    uint8_t reportType;
} HidReadParam_S;

typedef struct {
    SLE_Addr_S addr;
    HidPropertyType_E type;
    uint8_t reportId;
    uint8_t reportType;
    NLSTK_VariableData_S *value;
} HidWriteParam_S;

typedef struct {
    SLE_Addr_S addr;
    HidInformation_S *info;
    HidFreeFunc freeFunc;
} HidGetInfoParam_S;

typedef struct {
    SLE_Addr_S addr;
    uint8_t state;
} HidGetConnStateParam_S;

typedef struct {
    SLE_Addr_S *addrs;
    size_t num;
    HidFreeFunc freeFunc;
} HidGetConnDevParam_S;

typedef struct {
    uint16_t serviceNum;
    NLSTK_SsapServ_S *service;
    NLSTK_SsapClientFreeFunc func;
} HidGetServiceMsg_S;

typedef struct {
    NLSTK_SsapClientReadPropertyInfo_S *property;
    NLSTK_Errcode_E ret;
} HidReadPropertyMsg_S;

typedef struct {
    NLSTK_SsapClientWritePropertyInfo_S *property;
    NLSTK_Errcode_E ret;
} HidWritePropertyMsg_S;

typedef struct {
    NLSTK_SsapUuid_S *uuid;
    uint16_t handle;
    bool enable;
    NLSTK_Errcode_E ret;
} HidSetPropertyNtfMsg_S;

typedef struct {
    uint8_t reportId;
    uint8_t reportType;
    uint16_t reportHandle;
    uint16_t reportSrcPort;
    uint16_t reportDestPort;
    NLSTK_VariableData_S reportInfoValue;
} HidReport_S;

typedef struct {
    uint16_t handle;
    uint16_t endHandle;
    uint16_t descHandle;
    uint16_t workStateHandle;
    SDF_Vector_S *indexHandle;
} HidService_S;

typedef struct {
    int32_t appId;
    SLE_Addr_S addr;
    HidTypeAndFormatDesc_S desc;
    HidService_S service;
    SDF_Vector_S *report;
    uint16_t lastReadHandle;
    uint16_t lastSetNtfHandle;
    uint8_t state;
} HidDevice_S;

#ifdef __cplusplus
}
#endif

#endif