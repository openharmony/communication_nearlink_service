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
#include "nlstk_dis_client.h"
#include "nlstk_dis_def.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "dis_stm.h"
#include "nlstk_schedule.h"
#include "ssap_app_link.h"
#include "ssapc_client_api.h"
#include "dis_client.h"
#include "dis_type.h"

NLSTK_Errcode_E NLSTK_DisRegisterCallbBack(NLSTK_DisClientCbk_S *clientCallback)
{
    NLSTK_LOG_INFO("[DIS] NLSTK_DisRegisterCallbBack start!");
    NLSTK_CHECK_RETURN(clientCallback != NULL, NLSTK_ERR, "[DIS] clientCallback is null");
    NLSTK_DisClientCbk_S *cbk = (NLSTK_DisClientCbk_S *)SDF_MemZalloc(sizeof(NLSTK_DisClientCbk_S));
    NLSTK_CHECK_RETURN(cbk != NULL, NLSTK_ERR, "[DIS] cbk is null");
    (void)memcpy_s(cbk, sizeof(NLSTK_DisClientCbk_S), clientCallback, sizeof(NLSTK_DisClientCbk_S));
    if (SchedulePostTaskBlocked(DisRegClientCbk, cbk, SDF_MemFree, NLSTK_API_TIME_OUT) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DisProfileConnect(SLE_Addr_S *addr)
{
    NLSTK_LOG_DEBUG("[DIS] NLSTK_DisProfileConnect start!");
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERR, "[DIS] NLSTK_DisProfileConnect: addr is null");
    SLE_Addr_S *addrIn = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERR, "[DIS] addr is null");
    (void)memcpy_s(addrIn, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(DisConnectTask, addrIn, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_DisProfileDisconnect(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERR, "[DIS] NLSTK_DisProfileDisconnect: addr is null");
    SLE_Addr_S *addrIn = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN(addrIn != NULL, NLSTK_ERR, "[DIS] addr is null");
    (void)memcpy_s(addrIn, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (SchedulePostTask(DisDisconnectTask, addrIn, SDF_MemFree) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] SchedulePostTask failed");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_GetConnectedDeviceNum(uint8_t *num)
{
    NLSTK_LOG_DEBUG("[DIS] NLSTK_GetConnectedDeviceNum start!");
    NLSTK_CHECK_RETURN(num != NULL, NLSTK_ERR, "[DIS] NLSTK_GetConnectedDeviceNum: addr is null");
    uint8_t *param = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DIS] mem alloc failed");
    uint32_t ret = SchedulePostTaskBlocked(DisCountConnectedDevicesTask, param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        SchedulePostTask(SDF_MemFree, (void *)param, NULL);
        NLSTK_LOG_ERROR("[DIS] SchedulePostTask timeout");
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] SchedulePostTask failed");
        SDF_MemFree(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *num = *param;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}

static void DisCopyVariableLengthValue(NLSTK_VariableData_S *src, NLSTK_VariableData_S *dst)
{
    NLSTK_CHECK_RETURN_VOID(dst != NULL && src->len <= DIS_MAX_VAR_LEN, "[DIS] dst is invalid");
    if (memcpy_s(dst->data, DIS_MAX_VAR_LEN, src->data, src->len) != EOK) {
        NLSTK_LOG_ERROR("[DIS] memcpy failed!");
        return;
    }
    dst->len = src->len;
}

static void DisCopyDisPropData(uint16_t handle, NLSTK_VariableData_S *src, NLSTK_DisPropData_S *dst)
{
    NLSTK_CHECK_RETURN_VOID(handle != 0, "[DIS] src handle is invalid");
    NLSTK_CHECK_RETURN_VOID(dst != NULL && src->len <= DIS_MAX_VAR_LEN, "[DIS] dst is invalid");
    if (memcpy_s(dst->var, DIS_MAX_VAR_LEN, src->data, src->len) != EOK) {
        NLSTK_LOG_ERROR("[DIS] memcpy failed!");
        return;
    }
    dst->len = src->len;
}

void DisReadAllInfoByTypeTask(void *arg)
{
    NLSTK_DisAllPropInfo_S *readInfo = (NLSTK_DisAllPropInfo_S *)arg;
    DisDeviceInfo_S *info = NULL;
    info = DisFindDeviceInfo(&readInfo->addr);
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[DIS] info is null");
    readInfo->find = true;

    DisCopyDisPropData(info->deviceHandleInfo.manufacturerInfoHandle, &info->devicesInfo.manufacturerInfo,
        &readInfo->manufacturerInfo);
    DisCopyDisPropData(info->deviceHandleInfo.modelInfoHandle, &info->devicesInfo.deviceModel,
        &readInfo->deviceModel);
    DisCopyDisPropData(info->deviceHandleInfo.serialInfoHandle, &info->devicesInfo.deviceSerialNumber,
        &readInfo->deviceSerialNumber);
    DisCopyDisPropData(info->deviceHandleInfo.hardwareInfoHandle, &info->devicesInfo.hardwareVersion,
        &readInfo->hardwareVersion);
    DisCopyDisPropData(info->deviceHandleInfo.firmwareInfoHandle, &info->devicesInfo.firmwareVersion,
        &readInfo->firmwareVersion);
    DisCopyDisPropData(info->deviceHandleInfo.softwareInfoHandle, &info->devicesInfo.softwareVersion,
        &readInfo->softwareVersion);
    DisCopyDisPropData(info->deviceHandleInfo.deviceLocalAliasHandle, &info->devicesInfo.deviceLocalAlias,
        &readInfo->deviceLocalAlias);
    readInfo->deviceAppearance = info->devicesInfo.deviceAppearance;
}

void DisReadInfoByTypeTask(void *arg)
{
    DisReadInfoByType_S *readInfo = (DisReadInfoByType_S *)arg;
    DisDeviceInfo_S *info = NULL;
    info = DisFindDeviceInfo(&readInfo->addr);
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[DIS] info is null");
    switch (readInfo->type) {
        case DIS_MANUFACTURER_INFO:
            DisCopyVariableLengthValue(&info->devicesInfo.manufacturerInfo, &readInfo->value);
            break;
        case DIS_MODEL_INFO:
            DisCopyVariableLengthValue(&info->devicesInfo.deviceModel, &readInfo->value);
            break;
        case DIS_SERIAL_INFO:
            DisCopyVariableLengthValue(&info->devicesInfo.deviceSerialNumber, &readInfo->value);
            break;
        case DIS_HARDWARE_VERSION_INFO:
            DisCopyVariableLengthValue(&info->devicesInfo.hardwareVersion, &readInfo->value);
            break;
        case DIS_FIRMWARE_VERSION_INFO:
            DisCopyVariableLengthValue(&info->devicesInfo.firmwareVersion, &readInfo->value);
            break;
        case DIS_SOFTWARE_VERSION_INFO:
            DisCopyVariableLengthValue(&info->devicesInfo.softwareVersion, &readInfo->value);
            break;
        case DIS_LOCAL_ALIAS_INFO:
            DisCopyVariableLengthValue(&info->devicesInfo.deviceLocalAlias, &readInfo->value);
            break;
        default:
            break;
    }
}

static void FreeDisReadInfoByTypeParam(void *arg)
{
    if (arg == NULL) {
        return;
    }
    DisReadInfoByType_S *param = (DisReadInfoByType_S *)arg;
    if (param->value.data != NULL) {
        SDF_MemFree(param->value.data);
    }
    SDF_MemFree(param);
}

// 从client端本地缓存读取对应设备的DIS信息
NLSTK_Errcode_E NLSTK_DisReadInfo(SLE_Addr_S *addr, NLSTK_DisInfoType_E type, NLSTK_VariableData_S *outData)
{
    NLSTK_CHECK_RETURN(addr != NULL && outData != NULL && outData->data != NULL,
        NLSTK_ERRCODE_FAIL, "[DIS] invalid param");
    DisReadInfoByType_S *param = (DisReadInfoByType_S *)SDF_MemZalloc(sizeof(DisReadInfoByType_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_FAIL, "[DIS] mem alloc failed");
    param->value.data = (uint8_t *)SDF_MemZalloc(DIS_MAX_VAR_LEN);
    if (param->value.data == NULL) {
        NLSTK_LOG_ERROR("[DIS] mem alloc failed");
        SDF_MemFree(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    param->value.len = DIS_MAX_VAR_LEN;
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    param->type = type;
    NLSTK_Errcode_E ret = SchedulePostTaskBlocked(DisReadInfoByTypeTask, param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        NLSTK_LOG_ERROR("[DIS] SchedulePostTask timeout");
        // 在异常场景下，无法再处理，因此不判断返回值，此时param的内存会由SDF_MemFree释放
        (void)SchedulePostTask(FreeDisReadInfoByTypeParam, (void *)param, NULL);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] SchedulePostTask failed");
        FreeDisReadInfoByTypeParam(param);
        return ret;
    }
    if (memcpy_s(outData->data, DIS_MAX_VAR_LEN, param->value.data, param->value.len) != EOK) {
        NLSTK_LOG_ERROR("[DIS] memcpy failed");
        FreeDisReadInfoByTypeParam(param);
        return NLSTK_ERRCODE_MEMCPY_FAIL;
    }
    outData->len = param->value.len;
    FreeDisReadInfoByTypeParam(param);
    return ret;
}

NLSTK_Errcode_E NLSTK_DisReadAllInfo(NLSTK_DisAllPropInfo_S *propInfo)
{
    NLSTK_CHECK_RETURN(propInfo != NULL, NLSTK_ERRCODE_FAIL, "[DIS] invalid param");

    NLSTK_DisAllPropInfo_S *param = (NLSTK_DisAllPropInfo_S *)SDF_MemZalloc(sizeof(NLSTK_DisAllPropInfo_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_FAIL, "[DIS] mem alloc failed");
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), &propInfo->addr, sizeof(SLE_Addr_S));

    NLSTK_Errcode_E ret = SchedulePostTaskBlocked(DisReadAllInfoByTypeTask, param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        NLSTK_LOG_ERROR("[DIS] SchedulePostTask timeout");
        // 在异常场景下，无法再处理，因此不判断返回值，此时param的内存会由SDF_MemFree释放
        (void)SchedulePostTask(SDF_MemFree, (void *)param, NULL);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] SchedulePostTask failed");
        SDF_MemFree(param);
        return ret;
    }
    (void)memcpy_s(propInfo, sizeof(NLSTK_DisAllPropInfo_S), param, sizeof(NLSTK_DisAllPropInfo_S));
    SDF_MemFree(param);
    return ret;
}

void DisReadAppearanceTask(void *arg)
{
    DisReadAppearance_S *param = (DisReadAppearance_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[DIS] param is null");
    DisDeviceInfo_S *info = NULL;
    info = DisFindDeviceInfo(&param->addr);
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[DIS] info is null");
    param->appearance = info->devicesInfo.deviceAppearance;
}

NLSTK_Errcode_E NLSTK_DisReadAppearanceInfo(SLE_Addr_S *addr, uint32_t *appearance)
{
    NLSTK_CHECK_RETURN(addr != NULL && appearance != NULL, NLSTK_ERRCODE_FAIL, "[DIS] invalid param");
    DisReadAppearance_S *param = (DisReadAppearance_S *)SDF_MemZalloc(sizeof(DisReadAppearance_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_FAIL, "[DIS] mem alloc failed");
    (void)memcpy_s(&param->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    uint32_t ret = SchedulePostTaskBlocked(DisReadAppearanceTask, param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        NLSTK_LOG_ERROR("[DIS] SchedulePostTask timeout");
        // 在异常场景下，无法再处理，因此不判断返回值，此时param的内存会由SDF_MemFree释放
        (void)SchedulePostTask(SDF_MemFree, (void *)param, NULL);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] SchedulePostTask failed");
        SDF_MemFree(param);
        return ret;
    }
    *appearance = param->appearance;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}