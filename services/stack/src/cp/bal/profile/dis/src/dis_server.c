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
#include "dis_server.h"
#include "sdf_addr.h"
#include "sdf_mem.h"
#include "sdf_vector.h"
#include "nlstk_log.h"
#include "nlstk_dis_server.h"
#include "nlstk_dis_def.h"
#include "nlstk_ssap_app_server.h"
#include "nlstk_dis_server.h"
#include "dis_common.h"
#include "ssap_type.h"

#define DIS_APPEARANCE_LEN 3

static NLSTK_DeviceInfo_S g_devInfo = {0};
static int32_t g_disAppId = -1;
static uint16_t g_devNameHandle = 0;


// 创建描述符，若失败由调用者统一释放内存
static uint32_t DisPropertySetDescriptor(NLSTK_SsapServicePropertyParam_S *property)
{
    // 目前只需要客户端属性配置描述符，后续可拓展；
    property->descriptors = (NLSTK_SsapServiceDescriptorParam_S *)SDF_MemZalloc(
        sizeof(NLSTK_SsapServiceDescriptorParam_S));
    NLSTK_CHECK_RETURN(property->descriptors != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[DIS] property descriptors malloc fail");
    property->descriptorNum = 1;
    // perimission暂置为0，无需授权、认证、加密
    property->descriptors[0].type = DESC_TYPE_CLIENT_CONFIG;
    property->descriptors[0].operation.operationValue =
        SSAP_OPERATE_INDICATION_READ + SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
    property->descriptors[0].val.len = DIS_DESC_CLIENT_CONFIG_LEN;
    property->descriptors[0].val.data = (uint8_t *)SDF_MemZalloc(DIS_DESC_CLIENT_CONFIG_LEN);
    NLSTK_CHECK_RETURN(property->descriptors[0].val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[DIS] property descriptor value malloc fail");
    return NLSTK_ERRCODE_SUCCESS;
}

static void DisAddServiceStatement(NLSTK_ServiceParam_S *service)
{
    NLSTK_SsapServiceStatementParam_S *statement = &service->serviceStatement;
    NLSTK_SsapUuid_S uuid = DisConvertUuidToStru(DIS_SERVICE_UUID);
    (void)memcpy_s(&statement->uuid, sizeof(NLSTK_SsapUuid_S), &uuid, sizeof(NLSTK_SsapUuid_S));
    statement->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
}

static uint32_t DisAddPropertyInfo(NLSTK_SsapServicePropertyParam_S *property, NLSTK_VariableData_S *basicInfo,
    uint16_t uuid, uint8_t type)
{
    NLSTK_SsapUuid_S uuidStru = DisConvertUuidToStru(uuid);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    property->permission.permissionValue = type;
    property->operation.operationValue = SSAP_OPERATE_INDICATION_READ;
    if (property->val.len) {
        return DisPropertySetDescriptor(property);
    }
    property->val.len = basicInfo->len;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DIS] property value malloc fail");
    (void)memcpy_s(property->val.data, property->val.len, basicInfo->data, property->val.len);
    return DisPropertySetDescriptor(property);
}
static uint32_t DisAddDeviceAppearance(NLSTK_SsapServicePropertyParam_S *property, NLSTK_DeviceInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = DisConvertUuidToStru(DIS_APPEARANCE_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    property->permission.permissionValue = basicInfo->propertyRights[DIS_APPEARANCE_INFO];
    property->operation.operationValue = SSAP_OPERATE_INDICATION_READ;
    property->val.len = DIS_APPEARANCE_LEN;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DIS] property value malloc fail");
    (void)memcpy_s(property->val.data, property->val.len, &basicInfo->deviceAppearance, property->val.len);
    return DisPropertySetDescriptor(property);
}

// 组装服务属性
uint32_t DisAddServiceProperty(NLSTK_ServiceParam_S *service, NLSTK_DeviceInfo_S *basicInfo)
{
    service->property = (NLSTK_SsapServicePropertyParam_S *)SDF_MemZalloc(
        sizeof(NLSTK_SsapServicePropertyParam_S) * DIS_MAX_PROPERTY_SIZE);
    NLSTK_CHECK_RETURN(service->property != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DIS] serviceProperty malloc fail");
    service->servicePropertyNum = DIS_MAX_PROPERTY_SIZE;
    size_t index = 0;
    uint32_t ret = DisAddPropertyInfo(&service->property[index++], &basicInfo->manufacturerInfo, DIS_MANUFACTURER_UUID,
        basicInfo->propertyRights[DIS_MANUFACTURER_INFO]);
    ret += DisAddPropertyInfo(&service->property[index++], &basicInfo->deviceModel, DIS_MODEL_UUID,
        basicInfo->propertyRights[DIS_MODEL_INFO]);
    ret += DisAddPropertyInfo(&service->property[index++], &basicInfo->deviceSerialNumber, DIS_SERIAL_NUMBER_UUID,
        basicInfo->propertyRights[DIS_SERIAL_INFO]);
    ret += DisAddPropertyInfo(&service->property[index++], &basicInfo->hardwareVersion, DIS_HARDWARE_VERSION_UUID,
        basicInfo->propertyRights[DIS_HARDWARE_VERSION_INFO]);
    ret += DisAddPropertyInfo(&service->property[index++], &basicInfo->firmwareVersion, DIS_FIRMWARE_VERSION_UUID,
        basicInfo->propertyRights[DIS_FIRMWARE_VERSION_INFO]);
    ret += DisAddPropertyInfo(&service->property[index++], &basicInfo->softwareVersion, DIS_SOFTWARE_VERSION_UUID,
        basicInfo->propertyRights[DIS_SOFTWARE_VERSION_INFO]);
    ret += DisAddPropertyInfo(&service->property[index++], &basicInfo->deviceLocalAlias, DIS_LOCAL_ALIAS_UUID,
        basicInfo->propertyRights[DIS_LOCAL_ALIAS_INFO]);
    ret = DisAddDeviceAppearance(&service->property[index++], basicInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[DIS] add device appearance fail");
    return NLSTK_ERRCODE_SUCCESS; // 所有属性添加成功
}

static void DisAddService(void)
{
    NLSTK_ServiceParam_S *service = (NLSTK_ServiceParam_S *)SDF_MemZalloc(sizeof(NLSTK_ServiceParam_S));
    NLSTK_CHECK_RETURN_VOID(service != NULL, "[DIS] service malloc fail");
    NLSTK_LOG_INFO("[DIS] start add service");
    DisAddServiceStatement(service);
    uint32_t ret = DisAddServiceProperty(service, &g_devInfo);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] add service property fail");
        NLSTK_SsapFreeServiceParam(service);
        return;
    }
    ret = NLSTK_SsapServerAddService(g_disAppId, service);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] reg service fail");
    }
    NLSTK_SsapFreeServiceParam(service);
}

static void DisSaveOneProperty(NLSTK_VariableData_S *dest, NLSTK_VariableData_S *source)
{
    if (source->data == NULL || source->len == 0) {
        return;
    }
    dest->data = (uint8_t*)SDF_MemZalloc(source->len);
    NLSTK_CHECK_RETURN_VOID(dest->data != NULL, "[DIS] save property malloc fail");
    dest->len = source->len;
    (void)memcpy_s(dest->data, dest->len, source->data, dest->len);
}

static bool DisCompUuid(void *ptr, void *args)
{
    SSAP_Property_S *property = (SSAP_Property_S *)ptr;
    NLSTK_SsapUuid_S *uuid = (NLSTK_SsapUuid_S *)args;
    return memcmp(&property->uuid, uuid, sizeof(NLSTK_SsapUuid_S)) == 0;
}

static void DisOnAddService(int32_t appId, SSAP_Service_S *service, NLSTK_Errcode_E ret)
{
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] on add service failed");
        return;
    }
    NLSTK_CHECK_RETURN_VOID(service != NULL && service->properties,
        "[DIS] DisOnAddService service or properties is null");
    NLSTK_SsapUuid_S uuid = DisConvertUuidToStru(DIS_LOCAL_ALIAS_UUID);
    size_t index = 0;
    SSAP_Property_S *property = NULL;
    if (!SDF_VectorFindFirst(service->properties, DisCompUuid, &uuid, &index)) {
        NLSTK_LOG_ERROR("[DIS] DisOnAddService not found local alias");
        return;
    }
    property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, index);
    NLSTK_CHECK_RETURN_VOID(property != NULL, "[DIS] DisOnAddService property is null");
    g_devNameHandle = property->handle;
}
static void DisOnRegisterApp(int32_t appId, NLSTK_Errcode_E ret)
{
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[DIS] on register app failed DisOnRegisterApp");
        return;
    }
    NLSTK_LOG_INFO("[DIS] on register app success DisOnRegisterApp");
    g_disAppId = appId;
    DisAddService();
}

void DisSaveDevInfoTask(void *arg)
{
    NLSTK_DeviceInfo_S *info = (NLSTK_DeviceInfo_S *)arg;
    DisSaveOneProperty(&g_devInfo.manufacturerInfo, &info->manufacturerInfo);
    DisSaveOneProperty(&g_devInfo.deviceModel, &info->deviceModel);
    DisSaveOneProperty(&g_devInfo.deviceSerialNumber, &info->deviceSerialNumber);
    DisSaveOneProperty(&g_devInfo.hardwareVersion, &info->hardwareVersion);
    DisSaveOneProperty(&g_devInfo.firmwareVersion, &info->firmwareVersion);
    DisSaveOneProperty(&g_devInfo.softwareVersion, &info->softwareVersion);
    DisSaveOneProperty(&g_devInfo.deviceLocalAlias, &info->deviceLocalAlias);
    g_devInfo.deviceAppearance = info->deviceAppearance;
    (void)memcpy_s(g_devInfo.propertyRights, sizeof(info->propertyRights),
        info->propertyRights, sizeof(info->propertyRights));

    uint32_t ret = 0;
    NLSTK_SsapAppServerCb_S cb = {0};
    cb.onAddService = DisOnAddService;
    cb.onRegisterApp = DisOnRegisterApp;
    ret = NLSTK_SsapServerRegAppAsyn(&cb);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[DIS] Reg App failed");
}

static void DisFreeDeviceInfo(void *arg)
{
    NLSTK_DeviceInfo_S *info = (NLSTK_DeviceInfo_S *)arg;
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[DIS] info is null");
    if (info->manufacturerInfo.data != NULL) {
        SDF_MemFree(info->manufacturerInfo.data);
        info->manufacturerInfo.data = NULL;
    }
    if (info->deviceModel.data != NULL) {
        SDF_MemFree(info->deviceModel.data);
        info->deviceModel.data = NULL;
    }
    if (info->deviceSerialNumber.data != NULL) {
        SDF_MemFree(info->deviceSerialNumber.data);
        info->deviceSerialNumber.data = NULL;
    }
    if (info->hardwareVersion.data != NULL) {
        SDF_MemFree(info->hardwareVersion.data);
        info->hardwareVersion.data = NULL;
    }
    if (info->firmwareVersion.data != NULL) {
        SDF_MemFree(info->firmwareVersion.data);
        info->firmwareVersion.data = NULL;
    }
    if (info->softwareVersion.data != NULL) {
        SDF_MemFree(info->softwareVersion.data);
        info->softwareVersion.data = NULL;
    }
    if (info->deviceLocalAlias.data != NULL) {
        SDF_MemFree(info->deviceLocalAlias.data);
        info->deviceLocalAlias.data = NULL;
    }
}

void DisDestroyInstTask(void *arg)
{
    NLSTK_SsapServerDeregisterApplicationAsync(g_disAppId);
    DisFreeDeviceInfo(&g_devInfo);
}

void DisUpdateDevNameTask(void *arg)
{
    NLSTK_VariableData_S *param = (NLSTK_VariableData_S *)arg;
    if (g_devInfo.deviceLocalAlias.data != NULL) {
        SDF_MemFree(g_devInfo.deviceLocalAlias.data);
    }
    g_devInfo.deviceLocalAlias.data = NULL;
    g_devInfo.deviceLocalAlias.len = 0;
    DisSaveOneProperty(&g_devInfo.deviceLocalAlias, param);
    NLSTK_SsapServerUpdatePropertyValue(g_disAppId, g_devNameHandle, param);
}

void DisDeepCopyDevInfo(NLSTK_DeviceInfo_S* destDevInfo, NLSTK_DeviceInfo_S* srcDevInfo)
{
    destDevInfo->deviceAppearance = srcDevInfo->deviceAppearance;
    DisSaveOneProperty(&destDevInfo->manufacturerInfo, &srcDevInfo->manufacturerInfo);
    DisSaveOneProperty(&destDevInfo->deviceModel, &srcDevInfo->deviceModel);
    DisSaveOneProperty(&destDevInfo->deviceSerialNumber, &srcDevInfo->deviceSerialNumber);
    DisSaveOneProperty(&destDevInfo->hardwareVersion, &srcDevInfo->hardwareVersion);
    DisSaveOneProperty(&destDevInfo->firmwareVersion, &srcDevInfo->firmwareVersion);
    DisSaveOneProperty(&destDevInfo->softwareVersion, &srcDevInfo->softwareVersion);
    DisSaveOneProperty(&destDevInfo->deviceLocalAlias, &srcDevInfo->deviceLocalAlias);
    (void)memcpy_s(destDevInfo->propertyRights, sizeof(srcDevInfo->propertyRights),
        srcDevInfo->propertyRights, sizeof(srcDevInfo->propertyRights));
}

void DisFreeDeviceInfoTask(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[DIS] arg is null");
    DisFreeDeviceInfo(arg);
    SDF_MemFree(arg);
}