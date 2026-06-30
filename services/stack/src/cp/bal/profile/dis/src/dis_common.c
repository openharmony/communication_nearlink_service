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
#include "dis_common.h"
#include "nlstk_dis_client.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_log.h"
#include "sdf_mem.h"

static SDF_Vector_S *g_peerDevicesDisInfo = NULL;
static NLSTK_DisClientCbk_S g_disClientCbk = {0};
static uint8_t g_ssapStdBaseUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void DisRegClientCbkIn(NLSTK_DisClientCbk_S *cbk)
{
    NLSTK_CHECK_RETURN_VOID(cbk != NULL, "[DIS] cbk is null");
    (void)memcpy_s(&g_disClientCbk, sizeof(NLSTK_DisClientCbk_S), cbk, sizeof(NLSTK_DisClientCbk_S));
}

void DisUseStateChangeCallback(SLE_Addr_S *addr, NLSTK_DisConnectState_E curState,
    NLSTK_DisConnectState_E prevState, NLSTK_Errcode_E errNumb)
{
    if (g_disClientCbk.stateChangeCbk != NULL) {
        g_disClientCbk.stateChangeCbk(addr, curState, prevState, errNumb);
    }
}

static void DisVaribableLengthValueDestructor(NLSTK_VariableData_S *value)
{
    if (value->data != NULL) {
        SDF_MemFree(value->data);
    }
}

// DisDeviceInfo_S 的析构函数
void DeviceDisInfoDestructor(void *ptr)
{
    DisDeviceInfo_S *info = (DisDeviceInfo_S *)ptr;
    DisVaribableLengthValueDestructor(&info->devicesInfo.manufacturerInfo);
    DisVaribableLengthValueDestructor(&info->devicesInfo.deviceModel);
    DisVaribableLengthValueDestructor(&info->devicesInfo.deviceSerialNumber);
    DisVaribableLengthValueDestructor(&info->devicesInfo.hardwareVersion);
    DisVaribableLengthValueDestructor(&info->devicesInfo.firmwareVersion);
    DisVaribableLengthValueDestructor(&info->devicesInfo.softwareVersion);
    DisVaribableLengthValueDestructor(&info->devicesInfo.deviceLocalAlias);
    SDF_MemFree(ptr);
}

void DisClientInit(void)
{
    SDF_Traits disTraits = {.dtor = DeviceDisInfoDestructor};
    g_peerDevicesDisInfo = SDF_CreateVector(disTraits);
    NLSTK_CHECK_RETURN_VOID(g_peerDevicesDisInfo != NULL, "[DIS] create vector failed");
}

void DisClientInitIfEmpty(void)
{
    if (g_peerDevicesDisInfo == NULL) {
        NLSTK_LOG_INFO("[DIS] global variable g_peerDevicesDisInfo is null, init first");
        DisClientInit();
    }
}

NLSTK_SsapUuid_S DisConvertUuidToStru(uint16_t uuid)
{
    NLSTK_SsapUuid_S uuidStru;
    for (int i = 0; i < DIS_UUID_FIFTEENTH_BYTE; i++) {
        uuidStru.uuid[i] = g_ssapStdBaseUuid[i];
    }
    uuidStru.uuid[DIS_UUID_FIFTEENTH_BYTE] = (uint8_t)((uuid & 0xFF00) >> 0x08);
    uuidStru.uuid[DIS_UUID_SIXTEENTH_BYTE] = (uint8_t)(uuid & 0x00FF);
    return uuidStru;
}

static bool DisCompareAddr(void *ptr, void *args)
{
    DisDeviceInfo_S *info = (DisDeviceInfo_S *)ptr;
    SLE_Addr_S *addr = (SLE_Addr_S *)args;
    return (SDF_CompareSleAddr(&info->addr, addr) == 0);
}

bool DisCompAppId(void *ptr, void *args)
{
    DisDeviceInfo_S *dev = (DisDeviceInfo_S *)ptr;
    int32_t appId = *(int32_t *)args;
    return dev->appId == appId;
}

DisDeviceInfo_S *DisFindDeviceInfo(SLE_Addr_S *addr)
{
    size_t index = 0;
    DisDeviceInfo_S *info = NULL;
    if (!SDF_VectorFindFirst(g_peerDevicesDisInfo, DisCompareAddr, addr, &index)) {
        NLSTK_LOG_DEBUG("[DIS] can not find device");
        return NULL;
    } else {
        info = (DisDeviceInfo_S *)SDF_VectorElementAt(g_peerDevicesDisInfo, index);
        return info;
    }
}

DisDeviceInfo_S *DisFindDeviceInfoByAppId(int32_t appId)
{
    size_t size = 0;
    NLSTK_CHECK_RETURN(SDF_VectorFindFirst(g_peerDevicesDisInfo, DisCompAppId, &appId, &size),
        NULL, "[DIS] appId not found");
    DisDeviceInfo_S *devInfo = (DisDeviceInfo_S *)SDF_VectorElementAt(g_peerDevicesDisInfo, size);
    return devInfo;
}

void DisRemoveDeviceInfo(SLE_Addr_S *addr)
{
    size_t index = 0;
    NLSTK_CHECK_RETURN_VOID(SDF_VectorFindFirst(g_peerDevicesDisInfo, DisCompareAddr, addr, &index),
        "[DIS] addr not found");
    DisDeviceInfo_S *info = (DisDeviceInfo_S *)SDF_VectorElementAt(g_peerDevicesDisInfo, index);
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[DIS] info is null");
    int32_t appId = info->appId;
    if (appId > 0 && appId < NLSTK_SSAP_CLIENT_APP_MAX_NUM) {
        NLSTK_LOG_INFO("[DIS]NLSTK_SsapClientDeregAppAsync appId=%d, addr=%s", appId, GET_ENC_ADDR(addr));
        NLSTK_SsapClientDeregAppAsync(appId);
    }
    SDF_VectorRemove(g_peerDevicesDisInfo, index);
    if (g_peerDevicesDisInfo->size == 0) {
        SDF_DestroyVector(g_peerDevicesDisInfo);
        g_peerDevicesDisInfo = NULL;
    }
}

bool DisClientAddInfoIntoVector(DisDeviceInfo_S *info)
{
    if (!SDF_VectorEmplaceBack(g_peerDevicesDisInfo, info)) {
        NLSTK_LOG_ERROR("[DIS] SDF_VectorEmplaceBack failed");
        SDF_MemFree(info);
        return false;
    }
    return true;
}

uint8_t DisCountConnectedDevices(void)
{
    if (g_peerDevicesDisInfo == NULL) {
        // 首设备连接
        return 0;
    }
    uint8_t count = 0;
    for (size_t i = 0; i < g_peerDevicesDisInfo->size; ++i) {
        DisDeviceInfo_S *info = (DisDeviceInfo_S *)SDF_VectorElementAt(g_peerDevicesDisInfo, i);
        NLSTK_CHECK_RETURN(info != NULL, 0, "[DIS] info is null");
        if (info->state != DIS_APPID_UNREGISTERED) {
            ++count;
        }
    }
    return count;
}

void DisDisable(void)
{
    SDF_CleanVector(g_peerDevicesDisInfo);
}