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
#include "bas_common.h"
#include "nlstk_bas_client.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "bas_def.h"
#include "bas_stm.h"
#include "nlstk_public_define.h"

static SDF_Vector_S *g_basDeviceInfo = NULL;
static BasClientCallBack_S g_basClientCbk = {0};
static uint8_t g_ssapStdBaseUuid[] = {
    0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

BasClientCallBack_S *BasGetUserCbk(void)
{
    return &g_basClientCbk;
}

void BasRegClientCbkIn(BasClientCallBack_S *cbk)
{
    NLSTK_CHECK_RETURN_VOID(cbk != NULL, "[BAS] cbk is null");
    (void)memcpy_s(&g_basClientCbk, sizeof(BasClientCallBack_S), cbk, sizeof(BasClientCallBack_S));
}

void BasStateChangeCbk(
    SLE_Addr_S *addr, NLSTK_BasConnectState_E curState, NLSTK_BasConnectState_E prevState, NLSTK_Errcode_E errNumb)
{
    if (g_basClientCbk.connectStateChangeCbk != NULL) {
        g_basClientCbk.connectStateChangeCbk(addr, curState, prevState, errNumb);
    }
}

void BasReadCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value, NLSTK_Errcode_E ret)
{
    if (g_basClientCbk.readPropertyCbk != NULL) {
        g_basClientCbk.readPropertyCbk(addr, type, value, ret);
    }
}

void BasNotifyCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value)
{
    if (g_basClientCbk.propertyChangedCbk != NULL) {
        g_basClientCbk.propertyChangedCbk(addr, type, value);
    }
}

static void BasClearDeviceInfoData(BasDeviceInfo_S *deviceInfo)
{
    if (deviceInfo == NULL) {
        return;
    }
    if (deviceInfo->devDeviceInfo.remainBatPctInfo.data != NULL) {
        SDF_MemFree(deviceInfo->devDeviceInfo.remainBatPctInfo.data);
        deviceInfo->devDeviceInfo.remainBatPctInfo.data = NULL;
    }
    if (deviceInfo->devDeviceInfo.remainBatInfo.data != NULL) {
        SDF_MemFree(deviceInfo->devDeviceInfo.remainBatInfo.data);
        deviceInfo->devDeviceInfo.remainBatInfo.data = NULL;
    }
    if (deviceInfo->devDeviceInfo.batCapacityInfo.data != NULL) {
        SDF_MemFree(deviceInfo->devDeviceInfo.batCapacityInfo.data);
        deviceInfo->devDeviceInfo.batCapacityInfo.data = NULL;
    }
    if (deviceInfo->devDeviceInfo.batRateCapacityInfo.data != NULL) {
        SDF_MemFree(deviceInfo->devDeviceInfo.batRateCapacityInfo.data);
        deviceInfo->devDeviceInfo.batRateCapacityInfo.data = NULL;
    }
    if (deviceInfo->devDeviceInfo.remainWorkingTime.data != NULL) {
        SDF_MemFree(deviceInfo->devDeviceInfo.remainWorkingTime.data);
        deviceInfo->devDeviceInfo.remainWorkingTime.data = NULL;
    }
    (void)memset_s(&deviceInfo->devDeviceInfo, sizeof(BasPropertyInfo_S), 0, sizeof(BasPropertyInfo_S));
}

// BasDeviceInfo_S 的析构函数
void BasClearDeviceData(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    BasDeviceInfo_S *deviceInfo = (BasDeviceInfo_S *)ptr;
    BasClearDeviceInfoData(deviceInfo);
    SDF_DestroyVector(deviceInfo->indexHandle);
    SDF_MemFree(ptr);
}

bool BasClientInit(void)
{
    if (g_basDeviceInfo != NULL) {
        SDF_DestroyVector(g_basDeviceInfo);
        g_basDeviceInfo = NULL;
    }
    SDF_Traits basTraits = {.dtor = BasClearDeviceData};
    g_basDeviceInfo = SDF_CreateVector(basTraits);
    NLSTK_CHECK_RETURN(g_basDeviceInfo != NULL, false, "[BAS] create vector failed");
    return true;
}

void BasClientInitIfEmpty(void)
{
    if (g_basDeviceInfo == NULL) {
        NLSTK_LOG_INFO("[Bas] global variable g_basDeviceInfo is null, init first");
        BasClientInit();
    }
}

void BasClientDeinit(void)
{
    SDF_DestroyVector(g_basDeviceInfo);
    g_basDeviceInfo = NULL;
}

NLSTK_SsapUuid_S BasConvertUuidToStru(uint16_t uuid)
{
    NLSTK_SsapUuid_S uuidStru;
    for (int i = 0; i < BAS_UUID_FIFTEENTH_BYTE; i++) {
        uuidStru.uuid[i] = g_ssapStdBaseUuid[i];
    }
    uuidStru.uuid[BAS_UUID_FIFTEENTH_BYTE] = (uint8_t)((uuid & 0xFF00) >> 0x08);
    uuidStru.uuid[BAS_UUID_SIXTEENTH_BYTE] = (uint8_t)(uuid & 0x00FF);
    return uuidStru;
}

bool BasCompareAddr(void *ptr, void *args)
{
    BasDeviceInfo_S *info = (BasDeviceInfo_S *)ptr;
    SLE_Addr_S *addr = (SLE_Addr_S *)args;
    return (SDF_CompareSleAddr(&info->addr, addr) == 0);
}

bool BasCompAppId(void *ptr, void *args)
{
    BasDeviceInfo_S *devInfo = (BasDeviceInfo_S *)ptr;
    int32_t appId = *(int32_t *)args;
    return devInfo->appId == appId;
}

BasDeviceInfo_S *BasFindDeviceInfo(SLE_Addr_S *addr)
{
    size_t index = 0;
    BasDeviceInfo_S *info = NULL;
    if (!SDF_VectorFindFirst(g_basDeviceInfo, BasCompareAddr, addr, &index)) {
        NLSTK_LOG_ERROR("[BAS] can not find device");
        return NULL;
    } else {
        info = (BasDeviceInfo_S *)SDF_VectorElementAt(g_basDeviceInfo, index);
        return info;
    }
}

BasDeviceInfo_S *BasFindDeviceInfoByAppId(int32_t appId)
{
    size_t size = 0;
    NLSTK_CHECK_RETURN(
        SDF_VectorFindFirst(g_basDeviceInfo, BasCompAppId, &appId, &size), NULL, "[BAS] appId not found");
    BasDeviceInfo_S *devInfo = (BasDeviceInfo_S *)SDF_VectorElementAt(g_basDeviceInfo, size);
    return devInfo;
}

void BasRemoveDeviceInfo(SLE_Addr_S *addr)
{
    size_t index = 0;
    NLSTK_CHECK_RETURN_VOID(
        SDF_VectorFindFirst(g_basDeviceInfo, BasCompareAddr, addr, &index), "[BAS] addr not found");
    BasDeviceInfo_S *info = (BasDeviceInfo_S *)SDF_VectorElementAt(g_basDeviceInfo, index);
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[BAS] info is null");
    int32_t appId = info->appId;
    if (appId > 0 && appId < NLSTK_SSAP_CLIENT_APP_MAX_NUM) {
        NLSTK_LOG_INFO("[BAS]NLSTK_SsapClientDeregAppAsync appId=%d, addr=%s", appId, GET_ENC_ADDR(addr));
        NLSTK_SsapClientDeregAppAsync(appId);
    }
    SDF_VectorRemove(g_basDeviceInfo, index);
    if (g_basDeviceInfo->size == 0) {
        SDF_DestroyVector(g_basDeviceInfo);
        g_basDeviceInfo = NULL;
    }
}

bool BasClientAddInfoIntoVector(BasDeviceInfo_S *info)
{
    if (!SDF_VectorEmplaceBack(g_basDeviceInfo, info)) {
        NLSTK_LOG_ERROR("[BAS] SDF_VectorEmplaceBack failed");
        BasClearDeviceData(info);
        return false;
    }
    return true;
}

uint8_t BasCountConnectedDevices(void)
{
    if (g_basDeviceInfo == NULL) {
        // 首设备连接
        return 0;
    }
    uint8_t count = 0;
    for (size_t i = 0; i < g_basDeviceInfo->size; ++i) {
        BasDeviceInfo_S *info = (BasDeviceInfo_S *)SDF_VectorElementAt(g_basDeviceInfo, i);
        NLSTK_CHECK_RETURN(info != NULL, 0, "[BAS] info is null");
        if (info->state != BAS_APPID_UNREGISTERED) {
            ++count;
        }
    }
    return count;
}

uint16_t BasConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru)
{
    uint16_t uuid = 0;
    uuid |= (uint16_t)(uuidStru.uuid[BAS_UUID_FIFTEENTH_BYTE]) << 0x08;
    uuid |= (uint16_t)(uuidStru.uuid[BAS_UUID_SIXTEENTH_BYTE]);
    return uuid;
}

static uint32_t BasBuildProperty(BasDeviceInfo_S *devInfo, NLSTK_SsapServ_S service, uint16_t index)
{
    for (size_t i = 0; i < service.propertyNum; i++) {
        NLSTK_SsapPrty_S *property = &service.properties[i];
        if (property == NULL) {
            NLSTK_LOG_ERROR("[BAS] property is null");
            SDF_CleanVector(devInfo->indexHandle);
            return NLSTK_ERRCODE_MALLOC_FAIL;
        }
        uint16_t propertyUuid = BasConvertUuidTo16Bits(property->uuid);
        NLSTK_LOG_INFO("[BAS] property handle is: %d, property uuid is: %x", property->handle, propertyUuid);
        switch (propertyUuid) {
            case BAS_REMAIN_BATTERY_PERCENTAGE_UUID:
            case BAS_REMAIN_BATTERY_PERCENTAGE_UUID_PEN:
                devInfo->devHandleInfo.remainBatPctHdl = property->handle;
                HILOGI("[BAS] remainBatPctHdl value: %{public}d", devInfo->devHandleInfo.remainBatPctHdl);
                break;
            case BAS_REMAIN_BATTERY_UUID:
                devInfo->devHandleInfo.remainBatHdl = property->handle;
                break;
            case BAS_BATTERY_CAPACITY_UUID:
                devInfo->devHandleInfo.batCapacityHdl = property->handle;
                break;
            case BAS_BATTERY_RATED_CAPACITY_UUID:
                devInfo->devHandleInfo.batRateCapacityHdl = property->handle;
                break;
            case BAS_REMAINING_WORKING_TIME_UUID:
                devInfo->devHandleInfo.remainWorkingTimeHdl = property->handle;
                break;
            default:
                break;
        }
        uint16_t *handle = (uint16_t *)SDF_MemZalloc(sizeof(uint16_t));
        if (handle == NULL) {
            NLSTK_LOG_ERROR("[BAS] handle malloc fail");
            SDF_CleanVector(devInfo->indexHandle);
            return NLSTK_ERRCODE_MALLOC_FAIL;
        }
        *handle = property->handle;
        if (!SDF_VectorEmplaceBack(devInfo->indexHandle, handle)) {
            NLSTK_LOG_ERROR("[BAS] emplace back fail");
            SDF_CleanVector(devInfo->indexHandle);
            SDF_MemFree(handle);
            return NLSTK_ERRCODE_FAIL;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E BasBuildService(BasDeviceInfo_S *devInfo, NLSTK_SsapServ_S *service, uint16_t serviceNum)
{
    NLSTK_LOG_INFO("[BAS] find service num = %u", serviceNum);
    if (serviceNum > BAS_MAX_SERVICE_NUM) {
        NLSTK_LOG_ERROR("[BAS] serviceNum is invalid");
        return NLSTK_ERRCODE_FAIL;
    }

    for (uint16_t i = 0; i < serviceNum; i++) {
        uint16_t srvcUuid = BasConvertUuidTo16Bits(service[i].uuid);
        if (srvcUuid != BAS_SERVICE_UUID && srvcUuid != BAS_SERVICE_UUID_PEN) {
            continue;
        }
        if (service[i].propertyNum > BAS_MAX_SERVICE_NUM) {
            NLSTK_LOG_ERROR("[BAS] propertyNum is invalid");
            return NLSTK_ERRCODE_FAIL;
        }
        BasBuildProperty(devInfo, service[i], i);
        return NLSTK_ERRCODE_SUCCESS;
    }
    return NLSTK_ERRCODE_FAIL;
}