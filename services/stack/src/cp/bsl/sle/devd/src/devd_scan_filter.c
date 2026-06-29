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
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "nlstk_scan_api.h"
#include "nlstk_devd_def.h"
#include "devd_report_parse.h"
#include "nlstk_devd_api.h"
#include "devd_scan_type.h"
#include "common_ext_func_wrapper.h"
#include "devd_ext_func_wrapper.h"
#include "devd_scan_filter.h"

#define UUID_BYTE_15 14
#define UUID_BYTE_16 15

static uint8_t g_baseUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

typedef bool (*MatchRuleFunc)(NLSTK_DevdAdvResult_S *result, NLSTK_DevdScanFilter_S *filter);

static bool MatchAddr(NLSTK_DevdAdvResult_S *result, NLSTK_DevdScanFilter_S *filter)
{
    if (!filter->hasAddr) {
        return true;
    }
    return memcmp(&result->addr, &filter->addr, sizeof(SLE_Addr_S)) == 0;
}

static bool MatchName(NLSTK_DevdAdvResult_S *result, NLSTK_DevdScanFilter_S *filter)
{
    if (filter->name.len == 0) {
        return true;
    }
    if (result->localName.len == 0) {
        return false;
    }
    return result->localName.len == filter->name.len &&
        memcmp(result->localName.data, filter->name.data, filter->name.len) == 0;
}

static bool MatchServiceUuidWithMask(uint8_t *uuid, uint8_t *filterUuid, uint8_t *mask, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        if ((uuid[i] & mask[i]) != (filterUuid[i] & mask[i])) {
            return false;
        }
    }
    return true;
}

static bool MatchServiceUuid(NLSTK_DevdAdvResult_S *result, NLSTK_DevdScanFilter_S *filter)
{
    if (!filter->hasServiceUuid) {
        return true;
    }
    if (result->serviceUuids->size == 0) {
        return false;
    }
    for (size_t i = 0; i < result->serviceUuids->size; i++) {
        NLSTK_DevdAdvServiceUuid_S *uuid = SDF_VectorElementAt(result->serviceUuids, i);
        if (filter->hasServiceUuidMask) {
            if (MatchServiceUuidWithMask(uuid->uuid, filter->serviceUuid, filter->serviceUuidMask,
                SERVICE_UUID_LEN_128)) {
                return true;
            }
        } else {
            if (memcmp(uuid->uuid, filter->serviceUuid, SERVICE_UUID_LEN_128) == 0) {
                return true;
            }
        }
    }
    return false;
}

static bool MatchDataWithMask(NLSTK_VariableData_S *data, NLSTK_VariableData_S *filterData, NLSTK_VariableData_S *mask)
{
    if (data->len < filterData->len) {
        return false;
    }
    if (mask->len != filterData->len) {
        for (uint16_t i = 0; i < filterData->len; i++) {
            if (data->data[i] != filterData->data[i]) {
                return false;
            }
        }
        return true;
    }
    for (uint16_t i = 0; i < filterData->len; i++) {
        if ((data->data[i] & mask->data[i]) != (filterData->data[i] & mask->data[i])) {
            return false;
        }
    }
    return true;
}

static uint8_t GetUuidLen(uint8_t uuid[SERVICE_UUID_LEN_128])
{
    for (uint8_t i = 0; i < SERVICE_UUID_LEN_128 - SERVICE_UUID_LEN_16; i++) {
        if (uuid[i] != g_baseUuid[i]) {
            return SERVICE_UUID_LEN_128;
        }
    }
    return SERVICE_UUID_LEN_16;
}

static void ParseServiceData(NLSTK_VariableData_S *data, NLSTK_DevdAdvServiceData_S *serviceData, uint8_t uuidLen)
{
    if (uuidLen == SERVICE_UUID_LEN_16) {
        data->data[0] = serviceData->uuid[UUID_BYTE_16];
        data->data[1] = serviceData->uuid[UUID_BYTE_15];
    } else {
        (void)memcpy_s(data->data, SERVICE_UUID_LEN_128, serviceData->uuid, SERVICE_UUID_LEN_128);
    }
    (void)memcpy_s(data->data + uuidLen, serviceData->len, serviceData->data, serviceData->len);
}

static bool MatchServiceData(NLSTK_DevdAdvResult_S *result, NLSTK_DevdScanFilter_S *filter)
{
    if (filter->serviceData.len == 0) {
        return true;
    }
    if (result->serviceDataList->size == 0) {
        return false;
    }
    for (size_t i = 0; i < result->serviceDataList->size; i++) {
        NLSTK_DevdAdvServiceData_S *serviceData = SDF_VectorElementAt(result->serviceDataList, i);
        uint8_t uuidLen = GetUuidLen(serviceData->uuid);
        NLSTK_VariableData_S resultData = {.len = serviceData->len + uuidLen};
        resultData.data = SDF_MemZalloc(resultData.len);
        NLSTK_CHECK_RETURN(resultData.data != NULL, false, "[DEVDS] result data malloc failed");
        ParseServiceData(&resultData, serviceData, uuidLen);
        if (MatchDataWithMask(&resultData, &filter->serviceData, &filter->serviceDataMask)) {
            SDF_MemFree(resultData.data);
            return true;
        }
        SDF_MemFree(resultData.data);
    }
    return false;
}

static bool MatchManufacturerData(NLSTK_DevdAdvResult_S *result, NLSTK_DevdScanFilter_S *filter)
{
    if (filter->manufacturerId == 0) {
        return true;
    }
    if (result->manufacturerDataList->size == 0) {
        return false;
    }
    for (size_t i = 0; i < result->manufacturerDataList->size; i++) {
        NLSTK_DevdAdvManufacturerData_S *manufacturerData = SDF_VectorElementAt(result->manufacturerDataList, i);
        if (manufacturerData->manufacturerId != filter->manufacturerId) {
            continue;
        }
        if (manufacturerData->len == 0) {
            return true;
        }
        NLSTK_VariableData_S resultData = {.len = manufacturerData->len, .data = manufacturerData->data};
        return MatchDataWithMask(&resultData, &filter->manufacturerData, &filter->manufacturerDataMask);
    }
    return false;
}

static bool MatchRssi(NLSTK_DevdAdvResult_S *result, NLSTK_DevdScanFilter_S *filter)
{
    if (!filter->hasRssiThreshold) {
        return true;
    }
    return result->rssi >= filter->rssiThreshold;
}

static bool MatchMeshInfo(NLSTK_DevdAdvResult_S *result, NLSTK_DevdScanFilter_S *filter)
{
    if (!filter->meshInfoReport) {
        return true;
    }
    return result->meshInfo.len != 0;
}

static bool IsMatchFilter(NLSTK_DevdAdvResult_S *result, NLSTK_DevdScanFilter_S *filter)
{
    static MatchRuleFunc matchRules[] = {
        MatchAddr,
        MatchName,
        MatchServiceUuid,
        MatchServiceData,
        MatchManufacturerData,
        MatchRssi,
        MatchMeshInfo
    };
    for (uint8_t i = 0; i < sizeof(matchRules) / sizeof(matchRules[0]); i++) {
        if (!matchRules[i](result, filter)) {
            return false;
        }
    }
    return true;
}

bool DevdIsMatchFilters(NLSTK_DevdAdvResult_S *result, SDF_Vector_S *filters)
{
    if (result == NULL || filters == NULL) {
        return false;
    }
    for (size_t i = 0; i < filters->size; i++) {
        NLSTK_DevdScanFilter_S *filter = SDF_VectorElementAt(filters, i);
        if (filter->isNoFilter) {
            return true;
        }
        bool flag = filter->advIndReport;
        if (flag && result->frameType != ADV_FRAME_TYPE_4) {
            return false;
        }
        if (!flag && result->frameType != ADV_FRAME_TYPE_1) {
            return false;
        }
        if (IsMatchFilter(result, filter)) {
            return true;
        }
    }
    return false;
}

void DevdAddFilters(SDF_Vector_S *scanners, NLSTK_DevdScanner_S *scanner, NLSTK_DevdScanSetting_S *setting,
    SDF_Vector_S *filters)
{
    SDF_DestroyVector(scanner->filters);
    scanner->filters = filters;
    for (size_t i = 0; i < filters->size; i++) {
        NLSTK_DevdScanFilter_S *filter = (NLSTK_DevdScanFilter_S *)SDF_VectorElementAt(filters, i);
        NLSTK_LOG_INFO("[DEVDS] add soft filters scannerId:%u, filterIndex:%hhu, ServiceUuid:0x%02x%02x, "
            "ServiceUuidMask:0x%02x%02x, isSensorHubChannel:%d", scanner->scannerId, filter->filterIndex,
            filter->serviceUuid[SERVICE_UUID_LEN_128 - 0x02], filter->serviceUuid[SERVICE_UUID_LEN_128 - 0x01],
            filter->serviceUuidMask[SERVICE_UUID_LEN_128 - 0x02], filter->serviceUuidMask[SERVICE_UUID_LEN_128 - 0x01],
            filter->isSensorHubChannel);
    }
    DevdAddChipFilters(scanners, scanner, setting, filters);
}

void DevdDeleteFilters(NLSTK_DevdScanManager_S *manager, NLSTK_DevdScanner_S *scanner)
{
    if (scanner->filters == NULL)  {
        NLSTK_LOG_DEBUG("[DEVDS] filters is null");
        return;
    }
    DevdDeleteChipFilters(manager, scanner);
    SDF_DestroyVector(scanner->filters);
    scanner->filters = NULL;
}