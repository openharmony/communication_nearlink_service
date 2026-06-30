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
#include <stdint.h>
#include "nlstk_log.h"
#include "dis_stm.h"
#include "nlstk_dis_client.h"
#include "sdf_mem.h"
#include "ssap_type.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_cfgdb.h"

#define DIS_APPEARANCE_INFO_LEN 3
#define DIS_READ_PROPERTY_NUM 4

typedef void (*DisStmDispatch)(DisDeviceInfo_S *devInfo, DisStmParam param);

static void UpdatePropertyInfo(NLSTK_SsapClientReadPropertyInfo_S *property, DisDeviceInfo_S *devInfo);
static void DisHandleEvcInUnregState(DisDeviceInfo_S *devInfo, DisStmParam param);
static void DisHandleEvcInRegisteringState(DisDeviceInfo_S *devInfo, DisStmParam param);
static void DisHandleEvcInLinkDisconnectedState(DisDeviceInfo_S *devInfo, DisStmParam param);
static void DisHandleEvcInLinkConnectedState(DisDeviceInfo_S *devInfo, DisStmParam param);
static void DisHandleEvcInServiceFoundState(DisDeviceInfo_S *devInfo, DisStmParam param);
static void DisHandleEvcInConnectedState(DisDeviceInfo_S *devInfo, DisStmParam param);

uint16_t DisConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru)
{
    uint16_t uuid = 0;
    uuid |= (uint16_t)(uuidStru.uuid[DIS_UUID_FIFTEENTH_BYTE]) << 0x08;
    uuid |= (uint16_t)(uuidStru.uuid[DIS_UUID_SIXTEENTH_BYTE]);
    return uuid;
}

void FreeDeviceInfoInDisconnected(DisDeviceInfo_S *info)
{
    SDF_MemFree(info->devicesInfo.manufacturerInfo.data);
    SDF_MemFree(info->devicesInfo.deviceModel.data);
    SDF_MemFree(info->devicesInfo.deviceSerialNumber.data);
    SDF_MemFree(info->devicesInfo.hardwareVersion.data);
    SDF_MemFree(info->devicesInfo.firmwareVersion.data);
    SDF_MemFree(info->devicesInfo.softwareVersion.data);
    SDF_MemFree(info->devicesInfo.deviceLocalAlias.data);
    SDF_MemFree(info);
}

static void UpdateDeviceHandleInfo(NLSTK_SsapServ_S *service, DisDeviceInfo_S *devInfo)
{
    devInfo->deviceHandleInfo.disServiceHandle = service->handle;
    for (size_t i = 0; i < service->propertyNum; i++) {
        NLSTK_SsapPrty_S *property = &service->properties[i];
        uint16_t propertyUuid = DisConvertUuidTo16Bits(property->uuid);
        NLSTK_LOG_INFO("[DIS] property handle is: %d, property uuid is: %x", property->handle, propertyUuid);
        switch (propertyUuid) {
            case DIS_MANUFACTURER_UUID_PEN:
            case DIS_MANUFACTURER_UUID:
                devInfo->deviceHandleInfo.manufacturerInfoHandle = property->handle;
                break;
            case DIS_MODEL_UUID:
                devInfo->deviceHandleInfo.modelInfoHandle = property->handle;
                break;
            case DIS_SERIAL_NUMBER_UUID:
                devInfo->deviceHandleInfo.serialInfoHandle = property->handle;
                break;
            case DIS_HARDWARE_VERSION_UUID:
                devInfo->deviceHandleInfo.hardwareInfoHandle = property->handle;
                break;
            case DIS_FIRMWARE_VERSION_UUID:
                devInfo->deviceHandleInfo.firmwareInfoHandle = property->handle;
                break;
            case DIS_SOFTWARE_VERSION_UUID:
                devInfo->deviceHandleInfo.softwareInfoHandle = property->handle;
                break;
            case DIS_LOCAL_ALIAS_UUID_PEN:
            case DIS_LOCAL_ALIAS_UUID:
                devInfo->deviceHandleInfo.deviceLocalAliasHandle = property->handle;
                break;
            case DIS_APPEARANCE_UUID_PEN:
            case DIS_APPEARANCE_UUID:
                devInfo->deviceHandleInfo.deviceAppearanceHandle = property->handle;
                break;
            default:
                break;
        }
    }
}

static void DisGetAllServicesCb(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_SsapServ_S *service,
    uint16_t serviceNum, NLSTK_SsapClientFreeFunc func)
{
    DisDeviceInfo_S *devInfo = DisFindDeviceInfoByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(devInfo != NULL, "[DIS] get dev info is null");
    DisStmParam param = { .what = DIS_ON_GET_SERVICE_CBK, .extData = NULL };
    if (serviceNum > 0) {
        for (size_t i = 0; i < serviceNum; i++) {
            uint16_t uuid = DisConvertUuidTo16Bits(service[i].uuid);
            NLSTK_LOG_INFO("[DIS] service uuid is: %x", uuid);
            if (uuid == DIS_SERVICE_UUID || uuid == DIS_SERVICE_UUID_PEN) {
                UpdateDeviceHandleInfo(&service[i], devInfo);
            }
        }
    } else {
        NLSTK_LOG_ERROR("[DIS] DisGetServicesByUuidCb get service num is 0");
    }
    NLSTK_LOG_DEBUG("[DIS] DisGetServicesByUuidCb cur state is: %d", devInfo->state);
    func(service, serviceNum);
    DisClientStmCall(devInfo, param);
}

static void DisConnectStateChangeCb(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason)
{
    DisDeviceInfo_S *devInfo = DisFindDeviceInfoByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(devInfo != NULL, "[DIS] get dev info is null");
    DisStmParam param = {.what = DIS_ON_LINK_STATE_CHANGE, .extData = (void *)&state, .reason = reason};
    NLSTK_LOG_DEBUG("[DIS] DisConnectStateChangeCb cur state is: %d", devInfo->state);
    DisClientStmCall(devInfo, param);
}

static void DisRegisterAppCb(int32_t appId, SLE_Addr_S *addr, NLSTK_Errcode_E ret)
{
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[DIS] addr is null");
    DisDeviceInfo_S *devInfo = DisFindDeviceInfo(addr);
    NLSTK_CHECK_RETURN_VOID(devInfo != NULL, "[DIS] get dev info is null");
    DisStmParam param = { .what = DIS_ON_REGISTERED_CBK, .extData = (void *)&appId};
    NLSTK_LOG_DEBUG("[DIS] cur state is: %d", devInfo->state);
    DisClientStmCall(devInfo, param);
}

static uint32_t ConvertAppearanceId(NLSTK_VariableData_S *value)
{
    if (value->len != DIS_APPEARANCE_INFO_LEN) {
        return DIS_INVALID_APPEARANCE;
    }

    uint32_t result = 0;
    for (int i = 0; i < value->len; i++) {
        result |= (value->data[i] << (DIS_CONVERT_EIGHT * i));
    }

    return result;
}

static void DisReadPropertyCb(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret)
{
    NLSTK_CHECK_RETURN_VOID(property != NULL, "[DIS] property is null");
    DisDeviceInfo_S *devInfo = DisFindDeviceInfoByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(devInfo != NULL, "[DIS] get dev info is null");
    uint16_t propertyHandle = property->handle;

    if (propertyHandle == devInfo->deviceHandleInfo.deviceAppearanceHandle) {
        uint32_t appearance = ConvertAppearanceId(&property->value);
        NLSTK_CHECK_RETURN_VOID(appearance != DIS_INVALID_APPEARANCE, "[DIS] appearance convert failed");
        devInfo->devicesInfo.deviceAppearance = appearance;
    } else {
        UpdatePropertyInfo(property, devInfo);
    }
    if (propertyHandle == devInfo->deviceHandleInfo.deviceLocalAliasHandle) {
        NLSTK_LOG_DEBUG("[DIS] cur state is: %d", devInfo->state);
        DisStmParam param = { .what = DIS_ON_READ_PROPERTY_CBK, .extData = (void *)property };
        DisClientStmCall(devInfo, param);
    }
}

static void DisReadPropertiesCb(int32_t appId, uint8_t num, NLSTK_SsapClientReadPropertyInfo_S *properties,
    NLSTK_Errcode_E ret)
{
    NLSTK_CHECK_RETURN_VOID(properties != NULL, "[DIS] properties is null");
    for (uint8_t i = 0; i < num; i++) {
        DisReadPropertyCb(appId, &properties[i], ret);
    }
}

void DisClientStmCall(DisDeviceInfo_S *devInfo, DisStmParam event)
{
    NLSTK_CHECK_RETURN_VOID((devInfo != NULL), "[DIS] stm input is null");
    NLSTK_CHECK_RETURN_VOID(devInfo->state < DIS_STM_STATE_BUTT, "[DIS] stm invalid input");
    NLSTK_LOG_DEBUG("[DIS] app id(%d) start the stm, state is %d, event is %d",
        devInfo->appId, devInfo->state, event.what);
    static DisStmDispatch stateMachine[DIS_STM_STATE_BUTT] = {
        [DIS_APPID_UNREGISTERED] = DisHandleEvcInUnregState,
        [DIS_APPID_REGISTERING] = DisHandleEvcInRegisteringState,
        [DIS_LINK_DISCONNECTED] = DisHandleEvcInLinkDisconnectedState,
        [DIS_LINK_CONNECTED] = DisHandleEvcInLinkConnectedState,
        [DIS_SERVICE_FOUND] = DisHandleEvcInServiceFoundState,
        [DIS_DEV_CONNECTED] = DisHandleEvcInConnectedState,
    };
    DisStmDispatch dispatch = stateMachine[devInfo->state];
    if (dispatch == NULL) {
        NLSTK_LOG_INFO("[DIS] stm dispatch is null");
        return;
    }
    dispatch(devInfo, event);
}

static void DisHandleEvcInUnregState(DisDeviceInfo_S *devInfo, DisStmParam param)
{
    switch (param.what) {
        case DIS_ON_USER_CONNECT: {
            NLSTK_SsapAppClientCb_S cbk = {0};
            cbk.onGetServices = DisGetAllServicesCb;
            cbk.onConnectionStateChanged = DisConnectStateChangeCb;
            cbk.onReadProperty = DisReadPropertyCb;
            cbk.onReadProperties = DisReadPropertiesCb;
            cbk.onRegisterApp = DisRegisterAppCb;
            NLSTK_ConnParam_S connParam = { 0 };
            if (NLSTK_SsapClientRegAppAsyn(&devInfo->addr, &connParam, &cbk) == NLSTK_ERRCODE_SUCCESS) {
                DisUseStateChangeCallback(&devInfo->addr, DIS_CONNECTING, DIS_DISCONNECTED, NLSTK_ERRCODE_SUCCESS);
            }
            NLSTK_LOG_DEBUG("[DIS] cur state is DIS_APPID_UNREGISTERED next state is DIS_APPID_REGISTERING");
            devInfo->state = DIS_APPID_REGISTERING;
            break;
        }
        default:
            break;
    }
}

static void DisHandleEvcInRegisteringState(DisDeviceInfo_S *devInfo, DisStmParam param)
{
    switch (param.what) {
        case DIS_ON_REGISTERED_CBK: {
            int32_t appId = *(int32_t *)param.extData;
            devInfo->appId = appId;
            if (NLSTK_SsapClientConnect(appId) != NLSTK_ERRCODE_SUCCESS) {
                DisUseStateChangeCallback(&devInfo->addr, DIS_DISCONNECTED, DIS_CONNECTING, NLSTK_ERRCODE_TASK_FAIL);
            }
            NLSTK_LOG_DEBUG("[DIS] cur state is DIS_APPID_REGISTERING next state is DIS_LINK_DISCONNECTED");
            devInfo->state = DIS_LINK_DISCONNECTED;
            break;
        }
        default:
            break;
    }
}

static void DisHandleEvcInLinkDisconnectedState(DisDeviceInfo_S *devInfo, DisStmParam param)
{
    switch (param.what) {
        case DIS_ON_USER_CONNECT: {
            if (NLSTK_SsapClientConnect(devInfo->appId) != NLSTK_ERRCODE_SUCCESS) {
                DisUseStateChangeCallback(&devInfo->addr, DIS_DISCONNECTED, DIS_CONNECTING, NLSTK_ERRCODE_TASK_FAIL);
            }
            break;
        }
        // CONNECTING状态，收到连接状态改变回调，状态变为CONNECTED,调用NLSTK_SsapClientGetServicesByUuid
        case DIS_ON_LINK_STATE_CHANGE: {
            uint8_t state = *(uint8_t *)param.extData;
            if (state == SSAP_CONNECT_STATE_CONNECTED &&
                NLSTK_SsapClientGetServicesAsyn(devInfo->appId) == NLSTK_ERRCODE_SUCCESS) {
                    NLSTK_LOG_DEBUG("[DIS] cur state is DIS_LINK_DISCONNECTED next state is DIS_LINK_CONNECTED");
                    devInfo->state = DIS_LINK_CONNECTED;
            } else if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
                DisUseStateChangeCallback(&devInfo->addr, DIS_DISCONNECTED,
                    DIS_CONNECTING, (NLSTK_Errcode_E)param.reason);
                DisRemoveDeviceInfo(&devInfo->addr);
            }
            break;
        }
        default:
            break;
    }
}

static void CheckDisPropHandleAndNum(uint16_t *handles, uint16_t propHandle, uint8_t *num)
{
    if (propHandle != 0 && *num < DIS_READ_PROPERTY_NUM) {
        handles[(*num)++] = propHandle;
    }
}

static uint32_t DisReadProperties(DisDeviceInfo_S *devInfo)
{
    uint16_t handles[DIS_READ_PROPERTY_NUM] = {0};
    uint8_t num = 0;
    CheckDisPropHandleAndNum(handles, devInfo->deviceHandleInfo.deviceAppearanceHandle, &num);
    CheckDisPropHandleAndNum(handles, devInfo->deviceHandleInfo.manufacturerInfoHandle, &num);
    CheckDisPropHandleAndNum(handles, devInfo->deviceHandleInfo.modelInfoHandle, &num);
    CheckDisPropHandleAndNum(handles, devInfo->deviceHandleInfo.deviceLocalAliasHandle, &num);
    return NLSTK_SsapClientReadProperties(devInfo->appId, handles, num);
}

static uint32_t DisReadSsapClientProperty(DisDeviceInfo_S *devInfo)
{
    uint32_t ret = NLSTK_ERRCODE_SUCCESS;
    if (devInfo->deviceHandleInfo.deviceAppearanceHandle != 0) {
        ret = NLSTK_SsapClientReadProperty(devInfo->appId, devInfo->deviceHandleInfo.deviceAppearanceHandle);
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[DIS] read device appearance handle failed, ret=%u", ret);
            return ret;
        }
    }
    if (devInfo->deviceHandleInfo.manufacturerInfoHandle != 0) {
        ret = NLSTK_SsapClientReadProperty(devInfo->appId, devInfo->deviceHandleInfo.manufacturerInfoHandle);
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[DIS] read device manufacturer info handle failed, ret=%u", ret);
            return ret;
        }
    }
    if (devInfo->deviceHandleInfo.modelInfoHandle != 0) {
        ret = NLSTK_SsapClientReadProperty(devInfo->appId, devInfo->deviceHandleInfo.modelInfoHandle);
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[DIS] read device model info handle failed, ret=%u", ret);
            return ret;
        }
    }
    ret = NLSTK_SsapClientReadProperty(devInfo->appId, devInfo->deviceHandleInfo.deviceLocalAliasHandle);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] read device local alias handle failed, ret=%u", ret);
        return ret;
    }
    return ret;
}

static void DisHandleEvcInLinkConnectedState(DisDeviceInfo_S *devInfo, DisStmParam param)
{
    switch (param.what) {
        case DIS_ON_GET_SERVICE_CBK: {
            NLSTK_LOG_DEBUG("[DIS] cur state is DIS_LINK_CONNECTED next state is DIS_SERVICE_FOUND");
            devInfo->state = DIS_SERVICE_FOUND;
            uint32_t ret = NLSTK_ERRCODE_SUCCESS;
            if (CfgdbGetManufacturerSupport(&devInfo->addr, CFGDB_READ_MULTI_HANDLES)) {
                ret = DisReadProperties(devInfo);
            } else {
                ret = DisReadSsapClientProperty(devInfo);
            }
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                DisUseStateChangeCallback(&devInfo->addr, DIS_DISCONNECTED, DIS_CONNECTING, NLSTK_ERRCODE_TASK_FAIL);
            }
            break;
        }
        case DIS_ON_LINK_STATE_CHANGE: {
            uint8_t state = *(uint8_t *)param.extData;
            NLSTK_LOG_DEBUG("[DIS] cur state is %d, ", devInfo->state);
            if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
                DisUseStateChangeCallback(&devInfo->addr, DIS_DISCONNECTED, DIS_CONNECTED, NLSTK_ERRCODE_SUCCESS);
                DisRemoveDeviceInfo(&devInfo->addr);
            }
            break;
        }
        default:
            break;
    }
}

static void DisHandleEvcInServiceFoundState(DisDeviceInfo_S *devInfo, DisStmParam param)
{
    switch (param.what) {
        case DIS_ON_READ_PROPERTY_CBK: {
            DisUseStateChangeCallback(&devInfo->addr, DIS_CONNECTED, DIS_CONNECTING, NLSTK_ERRCODE_SUCCESS);
            NLSTK_LOG_DEBUG("[DIS] cur state is DIS_SERVICE_FOUND next state is DIS_DEV_CONNECTED");
            devInfo->state = DIS_DEV_CONNECTED;
            break;
        }
        case DIS_ON_LINK_STATE_CHANGE: {
            uint8_t state = *(uint8_t *)param.extData;
            if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
                DisUseStateChangeCallback(&devInfo->addr, DIS_DISCONNECTED, DIS_CONNECTED, NLSTK_ERRCODE_SUCCESS);
                DisRemoveDeviceInfo(&devInfo->addr);
            }
            break;
        }
        default:
            break;
    }
}

typedef struct {
    uint16_t handle;
    NLSTK_VariableData_S *info;
} HandleMapEntry;

typedef void (*HandleFunc)(NLSTK_VariableData_S *, DisDeviceInfo_S *, NLSTK_SsapClientReadPropertyInfo_S *);

void UpdateInfo(NLSTK_VariableData_S *info, DisDeviceInfo_S *devInfo, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    info->data = (uint8_t*)SDF_MemZalloc(property->value.len);
    NLSTK_CHECK_RETURN_VOID(info->data != NULL, "[DIS] mem alloc failed");
    info->len = property->value.len;
    (void)memcpy_s(info->data, property->value.len, property->value.data, property->value.len);
}

static void UpdatePropertyInfo(NLSTK_SsapClientReadPropertyInfo_S *property, DisDeviceInfo_S *devInfo)
{
    HandleMapEntry handleMap[] = {
        {devInfo->deviceHandleInfo.manufacturerInfoHandle, &devInfo->devicesInfo.manufacturerInfo},
        {devInfo->deviceHandleInfo.modelInfoHandle, &devInfo->devicesInfo.deviceModel},
        {devInfo->deviceHandleInfo.serialInfoHandle, &devInfo->devicesInfo.deviceSerialNumber},
        {devInfo->deviceHandleInfo.hardwareInfoHandle, &devInfo->devicesInfo.hardwareVersion},
        {devInfo->deviceHandleInfo.firmwareInfoHandle, &devInfo->devicesInfo.firmwareVersion},
        {devInfo->deviceHandleInfo.softwareInfoHandle, &devInfo->devicesInfo.softwareVersion},
        {devInfo->deviceHandleInfo.deviceLocalAliasHandle, &devInfo->devicesInfo.deviceLocalAlias},
    };

    size_t handleMapSize = sizeof(handleMap) / sizeof(handleMap[0]);
    uint16_t propertyHandle = property->handle;

    for (size_t i = 0; i < handleMapSize; i++) {
        if (propertyHandle == handleMap[i].handle) {
            UpdateInfo(handleMap[i].info, devInfo, property);
            break;
        }
    }
}

static void DisHandleEvcInConnectedState(DisDeviceInfo_S *devInfo, DisStmParam param)
{
    switch (param.what) {
        case DIS_ON_LINK_STATE_CHANGE: {
            uint8_t state = *(uint8_t *)param.extData;
            if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
                DisUseStateChangeCallback(&devInfo->addr, DIS_DISCONNECTED, DIS_CONNECTED, NLSTK_ERRCODE_SUCCESS);
                DisRemoveDeviceInfo(&devInfo->addr);
            }
            break;
        }
        case DIS_ON_USER_DISCONNECT: {
            DisUseStateChangeCallback(&devInfo->addr, DIS_DISCONNECTED, DIS_CONNECTED, NLSTK_ERRCODE_SUCCESS);
            DisRemoveDeviceInfo(&devInfo->addr);
            break;
        }
        default:
            break;
    }
}