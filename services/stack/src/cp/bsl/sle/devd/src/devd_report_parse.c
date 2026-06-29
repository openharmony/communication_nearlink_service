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
#include "sdf_string.h"
#include "sdf_dlist.h"
#include "nlstk_scan_api.h"
#include "devd_report_parse.h"

#define NO_POS (-1)

#define MAX_ADV_DATA_LEN 510 // ADV_DATA + SCAN_RSP_DATA
#define MIN_ADV_DATA_LEN 2   // T + L no V

#define ADV_DATA_TYPE_DISCOVERY_LEVEL                   0x01
#define ADV_DATA_TYPE_STANDARD_SERVICE_DATA             0x03
#define ADV_DATA_TYPE_CUSTOM_SERVICE_DATA               0x04
#define ADV_DATA_TYPE_COMPLETE_STANDARD_SERVICE_UUIDS   0x05
#define ADV_DATA_TYPE_INCOMPLETE_STANDARD_SERVICE_UUIDS 0x07
#define ADV_DATA_TYPE_SHORTENED_LOCAL_NAME              0x0A
#define ADV_DATA_TYPE_COMPLETE_LOCAL_NAME               0x0B
#define ADV_DATA_TYPE_TX_POWER                          0x0C
#define ADV_DATA_TYPE_MESH_INFO                         0x10
#define ADV_DATA_TYPE_MANUFACTURER_DATA                 0xFF

#define ADV_OLD_DATA_TYPE_DISCOVERY_LEVEL                    0x01
#define ADV_OLD_DATA_TYPE_INCOMPLETE_STANDARD_SERVICE_UUIDS  0x02
#define ADV_OLD_DATA_TYPE_COMPLETE_STANDARD_SERVICE_UUIDS    0x03
#define ADV_OLD_DATA_TYPE_INCOMPLETE_CUSTOMIZE_SERVICE_UUIDS 0x06
#define ADV_OLD_DATA_TYPE_COMPLETE_CUSTOMIZE_SERVICE_UUIDS   0x07
#define ADV_OLD_DATA_TYPE_SHORTENED_LOCAL_NAME               0x08
#define ADV_OLD_DATA_TYPE_COMPLETE_LOCAL_NAME                0x09
#define ADV_OLD_DATA_TYPE_TX_POWER                           0x0A
#define ADV_OLD_DATA_TYPE_STANDARD_SERVICE_DATA              0x16
#define ADV_OLD_DATA_TYPE_APPEARANCE                         0x19
#define ADV_OLD_DATA_TYPE_CUSTOM_SERVICE_DATA                0x21

#define UUID_BYTE_15 14
#define UUID_BYTE_16 15

#define ADV_CONNECTABLE_IND 0x00
#define ADV_SCANABLE_IND    0X01
#define ADV_SCAN_RSP_IND    0x03

#define MAX_DEVICE_CACHE 7

#define ADV_FRAME_TYPE_1 0x00
#define ADV_FRAME_TYPE_4 0x01

#define DEVD_ADV_DEVICE_INFO_MIN_LEN    2
#define DEVD_ADV_DEVICE_APPEARANCE_LEN  3
#define DEVD_DEVICE_INFO_LOCAL_NAME      0x06
#define DEVD_DEVICE_INFO_APPEARANCE      0x07

typedef struct {
    SDF_DListEntry_S entry;
    SLE_Addr_S addr;
    uint16_t len;
    uint8_t *data;
} AdvDataCache_S;

static const uint8_t g_baseUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static uint8_t g_disUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x09};

static const uint8_t g_pencilAdvertstingData1[] = {0x03, 0x19, 0xC7, 0x03, 0x0F, 0x16, 0xEE, 0xFD};
static const uint8_t g_pencilAdvertstingData2[] = {0x12, 0x09, 0x48, 0x55, 0x41, 0x57, 0x45, 0x49,
    0x20, 0x4D, 0x2D, 0x50, 0x65, 0x6E, 0x63, 0x69, 0x6C, 0x20, 0x33};

static SDF_DListHead_S g_advDataCacheList;

static void FreeAdvCache(AdvDataCache_S *cache)
{
    if (cache == NULL) {
        return;
    }
    if (cache->data != NULL) {
        SDF_MemFree(cache->data);
    }
    SDF_MemFree(cache);
}

static AdvDataCache_S *FindAdvDataCache(SLE_Addr_S *addr)
{
    AdvDataCache_S *cache = NULL;
    AdvDataCache_S *tmpCache = NULL;
    SDF_DListElmSafeForeach(cache, tmpCache, &g_advDataCacheList, entry) {
        if (memcmp(&cache->addr, addr, sizeof(SLE_Addr_S)) == 0) {
            return cache;
        }
    }
    return NULL;
}

static AdvDataCache_S *AddAdvDataCache(SLE_Addr_S *addr, uint8_t *data, uint8_t dataLen)
{
    if (dataLen == 0) {
        NLSTK_LOG_DEBUG("[DEVDS] data len is 0, invalid");
        return NULL;
    }
    AdvDataCache_S *cache = FindAdvDataCache(addr);
    if (cache == NULL) {
        cache = (AdvDataCache_S *)SDF_MemZalloc(sizeof(AdvDataCache_S));
        NLSTK_CHECK_RETURN(cache != NULL, NULL, "[DEVDS] cache malloc failed");
        if (SDF_DListCount(&g_advDataCacheList) >= MAX_DEVICE_CACHE) {
            AdvDataCache_S *tmpCache = NULL;
            SDF_DListElmTailDel(&g_advDataCacheList, tmpCache, entry);
            FreeAdvCache(tmpCache);
        }
        SDF_DListElmHeadInsert(&g_advDataCacheList, cache, entry);
    } else {
        SDF_MemFree(cache->data);
    }
    cache->len = dataLen;
    cache->data = (uint8_t *)SDF_MemZalloc(dataLen);
    if (cache->data == NULL) {
        NLSTK_LOG_ERROR("[DEVDS] zalloc failed, data len:%hhu", dataLen);
        SDF_DListElmDel(&g_advDataCacheList, cache, entry);
        FreeAdvCache(cache);
        return NULL;
    }
    (void)memcpy_s(cache->data, dataLen, data, dataLen);
    (void)memcpy_s(&cache->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    return cache;
}

static AdvDataCache_S *AppendAdvDataCache(SLE_Addr_S *addr, uint8_t *data, uint8_t dataLen)
{
    AdvDataCache_S *cache = FindAdvDataCache(addr);
    if (cache == NULL) {
        if (dataLen == 0) {
            NLSTK_LOG_DEBUG("[DEVDS] data len is 0, invalid");
            return NULL;
        }
        return AddAdvDataCache(addr, data, dataLen);
    }
    // rsp帧数据长度存在为0的场景, 不需要重新拼接，直接返回
    if (dataLen == 0) {
        return cache;
    }
    uint16_t newLen = cache->len + dataLen;
    uint8_t *newData = (uint8_t *)SDF_MemZalloc(newLen);
    if (newData == NULL) {
        NLSTK_LOG_ERROR("[DEVDS] zalloc failed, data len:%hhu", newLen);
        SDF_DListElmDel(&g_advDataCacheList, cache, entry);
        FreeAdvCache(cache);
        return NULL;
    }
    (void)memcpy_s(newData, cache->len, cache->data, cache->len);
    (void)memcpy_s(newData + cache->len, dataLen, data, dataLen);
    SDF_MemFree(cache->data);
    cache->len = newLen;
    cache->data = newData;
    return cache;
}

void ClearAdvDataCache(SLE_Addr_S *addr)
{
    AdvDataCache_S *cache = FindAdvDataCache(addr);
    if (cache == NULL) {
        return;
    }
    SDF_DListElmDel(&g_advDataCacheList, cache, entry);
    FreeAdvCache(cache);
}

static void SetDiscoveryLevel(NLSTK_DevdAdvResult_S *result, uint8_t len, uint8_t *data)
{
    NLSTK_CHECK_RETURN_VOID(len == sizeof(uint8_t), "[DEVDS] invalid discovery level length");
    result->discoveryLevel = *data;
}

static bool AddServiceData(NLSTK_DevdAdvResult_S *result, NLSTK_DevdAdvServiceData_S *serviceData)
{
    for (size_t i = 0; i < result->serviceDataList->size; i++) {
        NLSTK_DevdAdvServiceData_S *tmpData = SDF_VectorElementAt(result->serviceDataList, i);
        if (memcmp(tmpData->uuid, serviceData->uuid, SERVICE_UUID_LEN_128) == 0) {
            return false;
        }
    }
    return SDF_VectorEmplaceBack(result->serviceDataList, serviceData);
}

static void SetServiceData(NLSTK_DevdAdvResult_S *result, uint8_t len, uint8_t *data, uint8_t uuidLen)
{
    NLSTK_CHECK_RETURN_VOID(len > uuidLen, "[DEVDS] invalid service data length");
    uint8_t *payload = data;
    uint8_t dataLen = len - uuidLen;
    NLSTK_DevdAdvServiceData_S *serviceData =
        (NLSTK_DevdAdvServiceData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvServiceData_S) + dataLen);
    NLSTK_CHECK_RETURN_VOID(serviceData != NULL, "[DEVDS] serviceData malloc failed");
    (void)memcpy_s(serviceData->uuid, SERVICE_UUID_LEN_128, g_baseUuid, SERVICE_UUID_LEN_128);
    if (uuidLen == SERVICE_UUID_LEN_16) {
        serviceData->uuid[UUID_BYTE_16] = *payload++;
        serviceData->uuid[UUID_BYTE_15] = *payload++;
    } else {
        for (uint8_t i = 0; i < SERVICE_UUID_LEN_128; i++) {
            serviceData->uuid[i] = payload[i];
        }
        payload += SERVICE_UUID_LEN_128;
    }
    serviceData->len = dataLen;
    if (dataLen != 0) {
        (void)memcpy_s(serviceData->data, dataLen, payload, dataLen);
    }
    if (!AddServiceData(result, serviceData)) {
        NLSTK_LOG_ERROR("[DEVDS] set serviceData failed");
        SDF_MemFree(serviceData);
    }
}

static bool AddServiceUuid(NLSTK_DevdAdvResult_S *result, NLSTK_DevdAdvServiceUuid_S *serviceUuid)
{
    for (size_t i = 0; i < result->serviceUuids->size; i++) {
        NLSTK_DevdAdvServiceUuid_S *tmpUuid = SDF_VectorElementAt(result->serviceUuids, i);
        if (memcmp(tmpUuid->uuid, serviceUuid->uuid, SERVICE_UUID_LEN_128) == 0) {
            return false;
        }
    }
    return SDF_VectorEmplaceBack(result->serviceUuids, serviceUuid);
}

static void SetServiceUuid(NLSTK_DevdAdvResult_S *result, uint8_t len, uint8_t *data, uint8_t uuidLen)
{
    NLSTK_CHECK_RETURN_VOID(len % uuidLen == 0, "[DEVDS] invalid service uuid length");
    uint8_t *payload = data;
    uint8_t num = len / uuidLen;
    for (uint8_t i = 0; i < num; i++) {
        NLSTK_DevdAdvServiceUuid_S *serviceUuid =
            (NLSTK_DevdAdvServiceUuid_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvServiceUuid_S));
        NLSTK_CHECK_RETURN_VOID(serviceUuid != NULL, "[DEVDS] serviceUuid malloc failed");
        (void)memcpy_s(serviceUuid->uuid, SERVICE_UUID_LEN_128, g_baseUuid, SERVICE_UUID_LEN_128);
        if (uuidLen == SERVICE_UUID_LEN_16) {
            serviceUuid->uuid[UUID_BYTE_16] = *payload++;
            serviceUuid->uuid[UUID_BYTE_15] = *payload++;
        } else {
            for (uint8_t j = 0; j < SERVICE_UUID_LEN_128; j++) {
                serviceUuid->uuid[j] = payload[j];
            }
            payload += SERVICE_UUID_LEN_128;
        }
        if (!AddServiceUuid(result, serviceUuid)) {
            NLSTK_LOG_ERROR("[DEVDS] set serviceUuid failed");
            SDF_MemFree(serviceUuid);
        }
    }
}

static void SetLocalName(NLSTK_DevdAdvResult_S *result, uint8_t len, uint8_t *data)
{
    NLSTK_CHECK_RETURN_VOID(len > 0, "[DEVDS] invalid local name length");
    if (result->localName.data != NULL) {
        SDF_MemFree(result->localName.data);
    }
    result->localName.data = (uint8_t *)SDF_MemZalloc(len);
    NLSTK_CHECK_RETURN_VOID(result->localName.data != NULL, "[DEVDS] localName malloc failed");
    (void)memcpy_s(result->localName.data, len, data, len);
    result->localName.len = len;
}

static void SetTxPower(NLSTK_DevdAdvResult_S *result, uint8_t len, uint8_t *data)
{
    NLSTK_CHECK_RETURN_VOID(len == sizeof(uint8_t), "[DEVDS] invalid discovery level length");
    result->txPower = *data;
}

static void SetMeshInfo(NLSTK_DevdAdvResult_S *result, uint8_t len, uint8_t *data)
{
    NLSTK_CHECK_RETURN_VOID(len > 0, "[DEVDS] invalid mesh info length");
    if (result->meshInfo.data != NULL) {
        SDF_MemFree(result->meshInfo.data);
    }
    result->meshInfo.data = (uint8_t *)SDF_MemZalloc(len);
    NLSTK_CHECK_RETURN_VOID(result->meshInfo.data != NULL, "[DEVDS] meshInfo malloc failed");
    (void)memcpy_s(result->meshInfo.data, len, data, len);
    result->meshInfo.len = len;
}

static bool AddManufacturerData(NLSTK_DevdAdvResult_S *result, NLSTK_DevdAdvManufacturerData_S *manufacturerData)
{
    for (size_t i = 0; i < result->manufacturerDataList->size; i++) {
        NLSTK_DevdAdvManufacturerData_S *tmpData = SDF_VectorElementAt(result->manufacturerDataList, i);
        if (tmpData->manufacturerId == manufacturerData->manufacturerId) {
            return false;
        }
    }
    return SDF_VectorEmplaceBack(result->manufacturerDataList, manufacturerData);
}

static void SetManufacturerData(NLSTK_DevdAdvResult_S *result, uint8_t len, uint8_t *data)
{
    NLSTK_CHECK_RETURN_VOID(len > sizeof(uint16_t), "[DEVDS] invalid manufacturer data length");
    uint8_t dataLen = len - sizeof(uint16_t);
    uint8_t *payload = data;
    NLSTK_DevdAdvManufacturerData_S *manufacturerData =
        (NLSTK_DevdAdvManufacturerData_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvManufacturerData_S) + dataLen);
    NLSTK_CHECK_RETURN_VOID(manufacturerData != NULL, "[DEVDS] manufacturerData malloc failed");
    (void)memcpy_s(&manufacturerData->manufacturerId, sizeof(uint16_t), payload, sizeof(uint16_t));
    payload += sizeof(uint16_t);
    manufacturerData->len = dataLen;
    if (dataLen != 0) {
        (void)memcpy_s(manufacturerData->data, dataLen, payload, dataLen);
    }
    if (!AddManufacturerData(result, manufacturerData)) {
        NLSTK_LOG_ERROR("[DEVDS] set manufacturerData failed");
        SDF_MemFree(manufacturerData);
    }
}

static void BuildAdvData(NLSTK_DevdAdvResult_S *result, uint8_t type, uint8_t len, uint8_t *data)
{
    if (len == 0) {
        return;
    }
    NLSTK_LOG_DEBUG("[DEVDS] adv data type %02X, len %hhu", type, len);
    uint8_t *payload = data;
    switch (type) {
        case ADV_DATA_TYPE_DISCOVERY_LEVEL:
            SetDiscoveryLevel(result, len, payload);
            break;
        case ADV_DATA_TYPE_STANDARD_SERVICE_DATA:
            SetServiceData(result, len, payload, SERVICE_UUID_LEN_16);
            break;
        case ADV_DATA_TYPE_CUSTOM_SERVICE_DATA:
            SetServiceData(result, len, payload, SERVICE_UUID_LEN_128);
            break;
        case ADV_DATA_TYPE_COMPLETE_STANDARD_SERVICE_UUIDS:
        case ADV_DATA_TYPE_INCOMPLETE_STANDARD_SERVICE_UUIDS:
            SetServiceUuid(result, len, payload, SERVICE_UUID_LEN_16);
            break;
        case ADV_DATA_TYPE_SHORTENED_LOCAL_NAME:
        case ADV_DATA_TYPE_COMPLETE_LOCAL_NAME:
            SetLocalName(result, len, payload);
            break;
        case ADV_DATA_TYPE_TX_POWER:
            SetTxPower(result, len, payload);
            break;
        case ADV_DATA_TYPE_MESH_INFO:
            SetMeshInfo(result, len, payload);
            break;
        case ADV_DATA_TYPE_MANUFACTURER_DATA:
            SetManufacturerData(result, len, payload);
            break;
        default:
            NLSTK_LOG_ERROR("[DEVDS] unknown adv report data type");
            break;
    }
}

static void DevdParseDisName(NLSTK_DevdAdvResult_S *result)
{
    NLSTK_DevdAdvServiceData_S *disServiceData = NULL;
    for (size_t i = 0; i < result->serviceDataList->size; i++) {
        NLSTK_DevdAdvServiceData_S *serviceData =
            (NLSTK_DevdAdvServiceData_S *)SDF_VectorElementAt(result->serviceDataList, i);
        if (memcmp(serviceData->uuid, g_disUuid, SERVICE_UUID_LEN_128) == 0) {
            disServiceData = serviceData;
            break;
        }
    }
    if (disServiceData == NULL) {
        return;
    }
    if (disServiceData->len == 0) {
        NLSTK_LOG_WARN("[DEVDS] Dis service data is empty");
        return;
    }
    size_t i = 0;
    while ((i + DEVD_ADV_DEVICE_INFO_MIN_LEN) < disServiceData->len) {
        uint8_t type = disServiceData->data[i++];
        uint8_t length = disServiceData->data[i++];
        if (length == 0 || i + length > disServiceData->len) {
            break;
        }
        if (type == DEVD_DEVICE_INFO_LOCAL_NAME) {
            HILOGD("[DEVDS] has name");
            if (result->localName.data != NULL) {
                SDF_MemFree(result->localName.data);
                result->localName.len = 0;
            }
            result->localName.data = (uint8_t *)SDF_MemAlloc(length);
            NLSTK_CHECK_RETURN_VOID(result->localName.data != NULL, "[DEVDS] localName malloc fail");
            result->localName.len = length;
            (void)memcpy_s(result->localName.data, length, &disServiceData->data[i], length);
            break;
        }
        i = i + length;
    }
}

static bool DevdParseAdvReport(NLSTK_DevdAdvResult_S *result, uint8_t *data, uint16_t dataLength)
{
    NLSTK_CHECK_RETURN(result != NULL && data != NULL, false, "[DEVDS] param is null");
    NLSTK_CHECK_RETURN(dataLength <= MAX_ADV_DATA_LEN, false, "[DEVDS] unexpected adv report length1");
    NLSTK_LOG_DEBUG("[DEVDS] parse addr %s, adv len %u, adv data %s", GET_ENC_ADDR(&result->addr), dataLength,
        SDF_GET_UINT8_STR(data, dataLength));
    uint16_t parsePos = 0;
    uint8_t *payload = data;
    while (parsePos < dataLength) {
        if (parsePos + MIN_ADV_DATA_LEN > dataLength) {
            NLSTK_LOG_ERROR("[DEVDS] unexpected adv report length2");
            break;
        }
        uint8_t type = *payload++; // T
        uint8_t len = *payload++; // L
        parsePos += MIN_ADV_DATA_LEN;
        if (parsePos + len > dataLength) {
            NLSTK_LOG_ERROR("[DEVDS] unexpected adv report length3");
            break;
        }
        BuildAdvData(result, type, len, payload);
        parsePos += len;
        payload += len;
    }
    DevdParseDisName(result);    // 由于软件过滤下移到协议栈，需要提前将Dis服务数据中的名称解析出来
    return true;
}

static void BuildAdvDataOld(NLSTK_DevdAdvResult_S *result, uint8_t type, uint8_t len, uint8_t *data)
{
    if (len == 0) {
        return;
    }
    NLSTK_LOG_DEBUG("[DEVDS] old adv data type %02X, len %hhu", type, len);
    uint8_t *payload = data;
    switch (type) {
        case ADV_OLD_DATA_TYPE_DISCOVERY_LEVEL:
            SetDiscoveryLevel(result, len, payload);
            break;
        case ADV_OLD_DATA_TYPE_STANDARD_SERVICE_DATA:
            SetServiceData(result, len, payload, SERVICE_UUID_LEN_16);
            break;
        case ADV_OLD_DATA_TYPE_CUSTOM_SERVICE_DATA:
            SetServiceData(result, len, payload, SERVICE_UUID_LEN_128);
            break;
        case ADV_OLD_DATA_TYPE_INCOMPLETE_STANDARD_SERVICE_UUIDS:
        case ADV_OLD_DATA_TYPE_COMPLETE_STANDARD_SERVICE_UUIDS:
            SetServiceUuid(result, len, payload, SERVICE_UUID_LEN_16);
            break;
        case ADV_OLD_DATA_TYPE_INCOMPLETE_CUSTOMIZE_SERVICE_UUIDS:
        case ADV_OLD_DATA_TYPE_COMPLETE_CUSTOMIZE_SERVICE_UUIDS:
            SetServiceUuid(result, len, payload, SERVICE_UUID_LEN_128);
            break;
        case ADV_OLD_DATA_TYPE_SHORTENED_LOCAL_NAME:
        case ADV_OLD_DATA_TYPE_COMPLETE_LOCAL_NAME:
            SetLocalName(result, len, payload);
            break;
        case ADV_OLD_DATA_TYPE_TX_POWER:
            SetTxPower(result, len, payload);
            break;
        case ADV_OLD_DATA_TYPE_APPEARANCE:
            break;
        case ADV_DATA_TYPE_MANUFACTURER_DATA:
            SetManufacturerData(result, len, payload);
            break;
        default:
            NLSTK_LOG_ERROR("[DEVDS] unknown adv report data type");
            break;
    }
}

static bool DevdParseAdvReportOld(NLSTK_DevdAdvResult_S *result, uint8_t *data, uint16_t dataLength)
{
    NLSTK_CHECK_RETURN(result != NULL && data != NULL, false, "[DEVDS] param is null");
    NLSTK_CHECK_RETURN(dataLength <= MAX_ADV_DATA_LEN, false, "[DEVDS] unexpected adv report length1");
    NLSTK_LOG_DEBUG("[DEVDS] old parse addr %s, adv len %u, adv data %s", GET_ENC_ADDR(&result->addr), dataLength,
        SDF_GET_UINT8_STR(data, dataLength));
    uint16_t parsePos = 0;
    uint8_t *payload = data;
    while (parsePos < dataLength) {
        if (parsePos + MIN_ADV_DATA_LEN > dataLength) {
            NLSTK_LOG_ERROR("[DEVDS] unexpected adv report length2");
            break;
        }
        uint8_t len = *payload++; // L
        NLSTK_CHECK_RETURN(len != 0, false, "[DEVDS] len is 0");
        parsePos += sizeof(uint8_t);
        if (parsePos + len > dataLength) {
            NLSTK_LOG_ERROR("[DEVDS] unexpected adv report length3");
            break;
        }
        uint8_t type = *payload++; // T
        parsePos += sizeof(uint8_t);
        if (len > sizeof(uint8_t)) {
            BuildAdvDataOld(result, type, len - sizeof(uint8_t), payload);
            parsePos += len - sizeof(uint8_t);
            payload += len - sizeof(uint8_t);
        }
    }
    return true;
}

void DevdDataCacheInit(void)
{
    SDF_DListHeadInit(&g_advDataCacheList);
}

void DevdDataCacheDeInit(void)
{
    AdvDataCache_S *node = NULL;
    AdvDataCache_S *tmpNode = NULL;
    SDF_DListElmAllFree(node, tmpNode, &g_advDataCacheList, entry, FreeAdvCache);
}

void FreeAdvResult(NLSTK_DevdAdvResult_S *result)
{
    // 向不同module回调扫描到的广播数据时，result需要复用，故NLSTK_DevdAdvResult_S中的scannerIds单独析构
    if (result == NULL) {
        return;
    }
    if (result->localName.data != NULL) {
        SDF_MemFree(result->localName.data);
        result->localName.data = NULL;
    }
    if (result->meshInfo.data != NULL) {
        SDF_MemFree(result->meshInfo.data);
        result->meshInfo.data = NULL;
    }
    SDF_DestroyVector(result->serviceDataList);
    result->serviceDataList = NULL;
    SDF_DestroyVector(result->serviceUuids);
    result->serviceUuids = NULL;
    SDF_DestroyVector(result->manufacturerDataList);
    result->manufacturerDataList = NULL;
    if (result->advData.data != NULL) {
        SDF_MemFree(result->advData.data);
        result->advData.data = NULL;
    }
    SDF_MemFree(result);
}

static NLSTK_DevdAdvResult_S *CreateAdvResult(void)
{
    NLSTK_DevdAdvResult_S *result = (NLSTK_DevdAdvResult_S *)SDF_MemZalloc(sizeof(NLSTK_DevdAdvResult_S));
    NLSTK_CHECK_RETURN(result != NULL, NULL, "[DEVDS] result malloc failed");
    SDF_Traits traits = {.dtor = SDF_MemFree};
    result->serviceDataList = SDF_CreateVector(traits);
    if (result->serviceDataList == NULL) {
        FreeAdvResult(result);
        return NULL;
    }
    result->serviceUuids = SDF_CreateVector(traits);
    if (result->serviceUuids == NULL) {
        FreeAdvResult(result);
        return NULL;
    }
    result->manufacturerDataList = SDF_CreateVector(traits);
    if (result->manufacturerDataList == NULL) {
        FreeAdvResult(result);
        return NULL;
    }
    return result;
}

static void BuildLpsTable(const uint8_t *pattern, size_t patternLen, size_t *lps)
{
    if (patternLen == 0) {
        return;
    }

    size_t len = 0;  // 当前最长相同前后缀长度
    lps[0] = 0;      // 第一个字符没有真前缀和真后缀

    size_t i = 1;
    while (i < patternLen) {
        if (pattern[i] == pattern[len]) {
            len++;
            lps[i] = len;
            i++;
        } else {
            if (len != 0) {
                // 回退到上一个匹配的前后缀长度
                len = lps[len - 1];
            } else {
                lps[i] = 0;
                i++;
            }
        }
    }
}

static int KmpSearch(const uint8_t *data, size_t dataLen, const uint8_t *pattern, size_t patternLen, size_t startPos)
{
    if (patternLen == 0) {
        return 0;
    }
    if (dataLen < patternLen) {
        return NO_POS;
    }
    if (startPos >= dataLen) {
        return NO_POS;
    }

    // 分配并构建部分匹配表
    size_t *lps = (size_t *)SDF_MemZalloc(patternLen * sizeof(size_t));
    if (lps == NULL) {
        return NO_POS;
    }

    BuildLpsTable(pattern, patternLen, lps);

    size_t i = startPos;  // data的索引
    size_t j = 0;         // pattern的索引

    while (i < dataLen) {
        if (pattern[j] == data[i]) {
            i++;
            j++;
        }

        if (j == patternLen) {
            // 找到匹配
            SDF_MemFree(lps);
            return (int)(i - j);
        } else if (i < dataLen && pattern[j] != data[i]) {
            // 不匹配时，根据部分匹配表调整j
            if (j != 0) {
                j = lps[j - 1];
            } else {
                i++;
            }
        }
    }

    SDF_MemFree(lps);
    return NO_POS;
}

static int KmpSearchSimple(const uint8_t *data, size_t dataLen, const uint8_t *pattern, size_t patternLen)
{
    return KmpSearch(data, dataLen, pattern, patternLen, 0);
}

static NLSTK_DevdAdvResult_S *DevdGetParsedAdvResult(uint8_t advType, uint8_t frameType, uint8_t rssi, SLE_Addr_S *addr,
    AdvDataCache_S *cache)
{
    NLSTK_DevdAdvResult_S *result = CreateAdvResult();
    NLSTK_CHECK_RETURN(result != NULL, NULL, "[DEVDS] result create failed");
    (void)memcpy_s(&result->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    result->rssi = rssi;
    result->frameType = frameType;
    result->isConnectable = (advType & (1 << ADV_CONNECTABLE_IND));
    if (KmpSearchSimple(cache->data, cache->len, g_pencilAdvertstingData1, sizeof(g_pencilAdvertstingData1)) != -1 ||
        KmpSearchSimple(cache->data, cache->len, g_pencilAdvertstingData2, sizeof(g_pencilAdvertstingData2)) != -1) {
        result->isPencil = true;
        if (!DevdParseAdvReportOld(result, cache->data, cache->len)) {
            NLSTK_LOG_ERROR("[DEVDS] parse adv report failed");
            FreeAdvResult(result);
            return NULL;
        }
    } else {
        if (!DevdParseAdvReport(result, cache->data, cache->len)) {
            NLSTK_LOG_ERROR("[DEVDS] parse adv report failed");
            FreeAdvResult(result);
            return NULL;
        }
    }
    if (cache->len == 0) {
        NLSTK_LOG_ERROR("[DEVDS] cache len is 0, invalid");
        FreeAdvResult(result);
        return NULL;
    }
    result->advData.data = (uint8_t *)SDF_MemZalloc(cache->len);
    if (result->advData.data == NULL) {
        FreeAdvResult(result);
        return NULL;
    }
    (void)memcpy_s(result->advData.data, cache->len, cache->data, cache->len);
    result->advData.len = cache->len;

    return result;
}

NLSTK_DevdAdvResult_S *DevdAdvDataReport(NLSTK_DevdAdvReportInfo_S *report)
{
    NLSTK_CHECK_RETURN(report != NULL, NULL, "[DEVDS] report is null");
    uint8_t advType = report->extendParams.eventType;
    uint8_t frameType = report->extendParams.primFrameType;
    uint8_t rssi = report->rssi;
    bool isScannable = (advType & (1 << ADV_SCANABLE_IND));
    bool isScanRsp = (advType & (1 << ADV_SCAN_RSP_IND));
    SLE_Addr_S addr = {0};
    addr.type = report->addrType;
    (void)memcpy_s(addr.addr, SLE_ADDR_LEN, report->addr, SLE_ADDR_LEN);
    NLSTK_LOG_INFO("[DEVDS] recv adv addr %s, rssi %d, isScannable %u, isScanRsp %u, frameType %hhu, adv len %hhu",
        GET_ENC_ADDR(&addr), (int8_t)rssi, isScannable, isScanRsp, frameType, report->dataLength);
    NLSTK_LOG_INFO("[DEVDS] recv adv data %s", SDF_GET_UINT8_STR_NO_SPACE(report->data, report->dataLength));
    AdvDataCache_S *cache = NULL;
    NLSTK_DevdAdvResult_S *result = NULL;
    // 两段的帧一广播(广播数据+扫描响应数据)，需要拼接上报
    if (frameType == ADV_FRAME_TYPE_1 && (isScannable || isScanRsp)) {
        if (!isScanRsp) {
            NLSTK_LOG_DEBUG("frame 1 waiting");
            cache = AddAdvDataCache(&addr, report->data, report->dataLength);
            return NULL;
        } else {
            NLSTK_LOG_DEBUG("frame 1 appended");
            cache = AppendAdvDataCache(&addr, report->data, report->dataLength);
        }
        NLSTK_CHECK_RETURN(cache != NULL, NULL, "[DEVDS] cache is null");
        result = DevdGetParsedAdvResult(advType, frameType, rssi, &addr, cache);
    // 一段的帧一广播（无扫描响应数据）或帧四广播，直接上报
    } else {
        if (report->dataLength == 0) {
            NLSTK_LOG_DEBUG("[DEVDS] report data len is 0, invalid");
            return NULL;
        }
        cache = (AdvDataCache_S *)SDF_MemZalloc(sizeof(AdvDataCache_S));
        NLSTK_CHECK_RETURN(cache != NULL, NULL, "[DEVDS] cache malloc fail");
        cache->data = (uint8_t *)SDF_MemZalloc(report->dataLength);
        if (cache->data == NULL) {
            NLSTK_LOG_ERROR("[DEVDS] cache data malloc fail");
            SDF_MemFree(cache);
            return NULL;
        }
        cache->len = report->dataLength;
        (void)memcpy_s(cache->data, cache->len, report->data, cache->len);
        result = DevdGetParsedAdvResult(advType, frameType, rssi, &addr, cache);
        FreeAdvCache(cache);
    }
    return result;
}