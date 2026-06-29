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
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "securec.h"
#include "sdf_mem.h"
#include "sdf_vector.h"
#include "sdf_traits.h"
#include "nlstk_port_def.h"
#include "port_type.h"
#include "ssaps_server_app.h"
#include "nlstk_port_server.h"
#include "port_server_init.h"
#include "port_server.h"

#define PORT_PROPERTY_OPERATION 521
#define NLSTK_PORT_MAX_NUM 0xFF
#define PORT_DESC_CLIENT_CONFIG_LEN 2

SDF_Vector_S *g_portStdList = NULL;
SDF_Vector_S *g_portPrivateList = NULL;
int32_t g_serverPortAppId = -1;
uint16_t g_portPropertyHandle = 0;

static void OnAddPortService(int32_t appId, SSAP_Service_S *service, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[PORT] portAppId = %d", appId);
    NLSTK_CHECK_RETURN_VOID(ret == 0, "[PORT] onAddService fail ret = %d", ret);
    NLSTK_CHECK_RETURN_VOID(service, "[PORT] onAddService service is null");
    NLSTK_CHECK_RETURN_VOID(service->properties, "[PORT] onAddService service->propertys is null");
    NLSTK_CHECK_RETURN_VOID(service->properties->size > 0, "[PORT] onAddService service->propertys size is 0");
    SSAP_Property_S *property = SDF_VectorElementAt(service->properties, 0);
    g_portPropertyHandle = property->handle;
    NLSTK_LOG_INFO("[PORT] g_portPropertyHandle = %d", g_portPropertyHandle);
}

static uint32_t PortSetDescriptor(NLSTK_SsapServicePropertyParam_S *property)
{
    // 目前只需要客户端属性配置描述符，后续可拓展
    property->descriptors =
        (NLSTK_SsapServiceDescriptorParam_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServiceDescriptorParam_S));
    NLSTK_CHECK_RETURN(property->descriptors != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[PORT] property descriptors malloc fail");
    property->descriptorNum = 1;
    // perimission暂置为0，无需授权、认证、加密
    property->descriptors[0].type = DESC_TYPE_CLIENT_CONFIG;
    property->descriptors[0].operation.operationValue = SSAP_OPERATE_INDICATION_READ |
        SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
    property->descriptors[0].val.data = (uint8_t *)SDF_MemZalloc(PORT_DESC_CLIENT_CONFIG_LEN);
    property->descriptors[0].val.len = PORT_DESC_CLIENT_CONFIG_LEN;
    NLSTK_CHECK_RETURN(property->descriptors[0].val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[PORT] property descriptor value malloc fail");
    return NLSTK_ERRCODE_SUCCESS;
}

static void FreePortProperty(NLSTK_SsapServicePropertyParam_S *property)
{
    if (property->val.data != NULL) {
        SDF_MemFree(property->val.data);
    }
    if (property->descriptors != NULL) {
        for (uint16_t i = 0; i < property->descriptorNum; i++) {
            if (property->descriptors[i].val.data != NULL) {
                SDF_MemFree(property->descriptors[i].val.data);
            }
        }
        SDF_MemFree(property->descriptors);
    }
    SDF_MemFree(property);
}

static void FreePortService(void *arg)
{
    if (arg == NULL) {
        return;
    }
    NLSTK_ServiceParam_S *service = (NLSTK_ServiceParam_S *)arg;
    if (service->property != NULL) {
        FreePortProperty(service->property);
    }
    SDF_MemFree(service);
}

NLSTK_Errcode_E PortServerEnable(void)
{
    NLSTK_LOG_DEBUG("[PORT] PortServerEnable enter");
    SsapRegServerAppParam_S param = {.appId = SSAP_SERVER_APP_INVALID_APPID};
    param.cb.onAddService = &OnAddPortService;
    SsapServerRegApp(&param);
    NLSTK_CHECK_RETURN(param.appId != SSAP_SERVER_APP_INVALID_APPID, NLSTK_ERRCODE_FAIL, "[PORT] Reg App failed");
    g_serverPortAppId = param.appId;
    NLSTK_LOG_INFO("[PORT] portAppId = %d", g_serverPortAppId);
    SDF_Traits traits = {.dtor = SDF_MemFree};
    g_portPrivateList = SDF_CreateVector(traits);
    NLSTK_CHECK_RETURN(g_portPrivateList, NLSTK_ERRCODE_MALLOC_FAIL, "[PORT] create g_portPrivateList failed");
    NLSTK_CHECK_RETURN(g_serverPortAppId >= 0, NLSTK_ERRCODE_FAIL, "[PORT] g_serverPortAppId is not init");
    NLSTK_ServiceParam_S *service = (NLSTK_ServiceParam_S *)SDF_MemZalloc(sizeof(NLSTK_ServiceParam_S));
    NLSTK_CHECK_RETURN(service, NLSTK_ERRCODE_MALLOC_FAIL, "[PORT] service malloc failed");
    (void)memcpy_s(&service->serviceStatement.uuid, sizeof(NLSTK_SsapUuid_S),
        &g_portServiceStdUuid, sizeof(NLSTK_SsapUuid_S));
    service->serviceStatement.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    service->servicePropertyNum = 1;
    service->property = (NLSTK_SsapServicePropertyParam_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServicePropertyParam_S));
    if (service->property == NULL) {
        NLSTK_LOG_ERROR("[PORT] property malloc failed");
        FreePortService(service);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    // 私有应用端口信息属性初始长度为2，指示端口数量为0
    service->property->val.len = 0x02;
    service->property->val.data = (uint8_t *)SDF_MemZalloc(service->property->val.len);
    if (service->property->val.data == NULL) {
        NLSTK_LOG_ERROR("[PORT] property val data malloc failed");
        FreePortService(service);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    service->property->type = ITEM_TYPE_STD_PROPERTY;
    service->property->operation.operationValue = SSAP_OPERATE_INDICATION_READ | SSAP_OPERATE_INDICATION_NOTIFY |
        SSAP_OPERATE_INDICATION_INDICATE | SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
    (void)memcpy_s(&service->property->uuid, sizeof(NLSTK_SsapUuid_S),
        &g_portPropertyStdUuid, sizeof(NLSTK_SsapUuid_S));
    service->property->operation.operationValue = PORT_PROPERTY_OPERATION;
    if (PortSetDescriptor(service->property) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[PORT] property set desc failed");
        FreePortService(service);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    if (NLSTK_SsapServerAddService(g_serverPortAppId, service) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[PORT] add service failed");
    }
    FreePortService(service);
    return NLSTK_ERRCODE_SUCCESS;
}

void PortServerDisable(void)
{
    NLSTK_LOG_DEBUG("[PORT] PortDeinit enter");
    NLSTK_CHECK_RETURN_VOID(g_serverPortAppId >= 0, "[PORT] g_serverPortAppId is not init");
    NLSTK_LOG_INFO("[PORT] portAppId = %d", g_serverPortAppId);
    SsapRegServerAppParam_S param = {.appId = g_serverPortAppId};
    SsapServerDeregisterApplication(&param);
    g_serverPortAppId = -1;
    g_portPropertyHandle = 0;
    SDF_DestroyVector(g_portPrivateList);
    g_portPrivateList = NULL;
}

static void PortAddNewPropertyValueByUuid(NLSTK_SsapUuid_S *uuid, uint16_t manufactureId, uint16_t portId)
{
    PortPrivateInfo_S *portInfo = (PortPrivateInfo_S *)SDF_MemZalloc(sizeof(PortPrivateInfo_S));
    NLSTK_CHECK_RETURN_VOID(portInfo != NULL, "[PORT] portInfo malloc failed");
    portInfo->portId = portId;
    portInfo->manufactureId = manufactureId;
    (void)memcpy_s(&portInfo->uuid, sizeof(NLSTK_SsapUuid_S), uuid, sizeof(NLSTK_SsapUuid_S));
    if (!SDF_VectorEmplaceBack(g_portPrivateList, portInfo)) {
        NLSTK_LOG_ERROR("[PORT] SDF_VectorEmplaceBack portInfo failed");
        SDF_MemFree(portInfo);
        return;
    }
}

static void PortSwapPropertyValueByUuid(PortPrivateInfo_S *param, size_t index)
{
    PortPrivateInfo_S *portInfo = (PortPrivateInfo_S *)SDF_VectorElementAt(g_portPrivateList, index);
    NLSTK_CHECK_RETURN_VOID(portInfo != NULL, "[PORT] portInfo is NULL");
    uint16_t manufacturePreId = portInfo->manufactureId;
    uint16_t portPreId = portInfo->portId;
    portInfo->manufactureId = param->manufactureId;
    portInfo->portId = param->portId;
    param->manufactureId = manufacturePreId;
    param->portId = portPreId;
}

static bool PortUuidCompFunc(void *ptr, void *args)
{
    NLSTK_CHECK_RETURN(ptr && args, false, "ptr or args is null");

    PortPrivateInfo_S *cache = (PortPrivateInfo_S *)ptr;
    NLSTK_SsapUuid_S *uuid = (NLSTK_SsapUuid_S *)args;
    return (memcmp(&cache->uuid, uuid, sizeof(NLSTK_SsapUuid_S)) == 0);
}

static NLSTK_VariableData_S *PortGetPropertyValue()
{
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    NLSTK_CHECK_RETURN(value != NULL, NULL, "[PORT] value malloc failed");
    value->len = (uint16_t)(PORT_NUM_LEN + g_portPrivateList->size * sizeof(PortPrivateInfo_S));
    value->data = (uint8_t *)SDF_MemZalloc(value->len);
    if (value->data == NULL) {
        SDF_MemFree(value);
        NLSTK_LOG_ERROR("[PORT] value data malloc failed");
        return NULL;
    }
    uint16_t portNum = (uint16_t)g_portPrivateList->size;
    uint8_t *buf = value->data;
    (void)memcpy_s(buf, PORT_NUM_LEN, &portNum, PORT_NUM_LEN);
    buf += PORT_NUM_LEN;
    for (size_t i = 0; i < g_portPrivateList->size; i++) {
        PortPrivateInfo_S *portInfo = (PortPrivateInfo_S *)SDF_VectorElementAt(g_portPrivateList, i);
        (void)memcpy_s(buf, sizeof(PortPrivateInfo_S), portInfo, sizeof(PortPrivateInfo_S));
        buf += sizeof(PortPrivateInfo_S);
    }
    return value;
}

void PortAddByUuidInner(void *arg)
{
    NLSTK_LOG_DEBUG("[PORT] PortAddByUuidInner enter");
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[PORT] arg is NULL");
    PortPrivateInfo_S *param = (PortPrivateInfo_S *)arg;
    size_t index = 0;
    size_t startIndex = 0;
    bool isFind = false;
    NLSTK_CHECK_RETURN_VOID(g_portPrivateList != NULL, "[PORT] g_portPrivateList is NULL");
    isFind = SDF_VectorFindFirstByStartIndex(g_portPrivateList, PortUuidCompFunc, &param->uuid, startIndex, &index);
    if (isFind) {
        PortSwapPropertyValueByUuid(param, index);
    } else {
        NLSTK_CHECK_RETURN_VOID(g_portPrivateList->size < NLSTK_PORT_MAX_NUM, "[PORT] g_portPrivateList is out range");
        PortAddNewPropertyValueByUuid(&param->uuid, param->manufactureId, param->portId);
    }
    NLSTK_VariableData_S *value = PortGetPropertyValue();
    if (value == NULL) {
        if (isFind) {
            PortSwapPropertyValueByUuid(param, index);
        } else {
            SDF_VectorRemove(g_portPrivateList, g_portPrivateList->size - 1);
        }
        NLSTK_LOG_ERROR("[PORT] PortGetPropertyValue failed !");
        return;
    }
    NLSTK_Errcode_E ret = NLSTK_SsapServerUpdatePropertyValue(g_serverPortAppId, g_portPropertyHandle, value);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        if (isFind) {
            PortSwapPropertyValueByUuid(param, index);
        } else {
            SDF_VectorRemove(g_portPrivateList, g_portPrivateList->size - 1);
        }
        NLSTK_LOG_ERROR("[PORT] NLSTK_SsapServerUpdatePropertyValue failed ! ret=%d", ret);
    }
    SDF_MemFree(value->data);
    SDF_MemFree(value);
}

void PortDeleteByUuidInner(void *arg)
{
    NLSTK_LOG_DEBUG("[PORT] PortDeleteByUuidInner enter");
    NLSTK_CHECK_RETURN_VOID(arg != NULL, "[PORT] arg is NULL");
    NLSTK_SsapUuid_S *uuid = (NLSTK_SsapUuid_S *)arg;
    size_t index = 0;
    size_t startIndex = 0;
    PortPrivateInfo_S *portInfoPre = NULL;
    if (SDF_VectorFindFirstByStartIndex(g_portPrivateList, PortUuidCompFunc, uuid, startIndex, &index)) {
        portInfoPre = (PortPrivateInfo_S *)SDF_MemZalloc(sizeof(PortPrivateInfo_S));
        NLSTK_CHECK_RETURN_VOID(portInfoPre != NULL, "[PORT] portInfoPre is NULL");
        PortPrivateInfo_S *portInfo = (PortPrivateInfo_S *)SDF_VectorElementAt(g_portPrivateList, index);
        portInfoPre->portId = portInfo->portId;
        portInfoPre->manufactureId = portInfo->manufactureId;
        (void)memcpy_s(&portInfoPre->uuid, sizeof(NLSTK_SsapUuid_S), &portInfo->uuid, sizeof(NLSTK_SsapUuid_S));
        SDF_VectorRemove(g_portPrivateList, index);
    } else {
        NLSTK_LOG_ERROR("[PORT] PortDeleteByUuidInner uuid not found");
        return;
    }
    NLSTK_VariableData_S *value = PortGetPropertyValue();
    if (value == NULL) {
        if (!SDF_VectorEmplaceBack(g_portPrivateList, portInfoPre)) {
            NLSTK_LOG_ERROR("[PORT] SDF_VectorEmplaceBack portInfoPre failed");
            SDF_MemFree(portInfoPre);
        }
        NLSTK_LOG_ERROR("[PORT] PortGetPropertyValue failed");
        return;
    }
    NLSTK_Errcode_E ret = NLSTK_SsapServerUpdatePropertyValue(g_serverPortAppId, g_portPropertyHandle, value);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        if (!SDF_VectorEmplaceBack(g_portPrivateList, portInfoPre)) {
            NLSTK_LOG_ERROR("[PORT] SDF_VectorEmplaceBack portInfoPre failed");
            SDF_MemFree(portInfoPre);
        }
        NLSTK_LOG_ERROR("[PORT] NLSTK_SsapServerUpdatePropertyValue failed ! ret=%d", ret);
    } else {
        SDF_MemFree(portInfoPre);
    }
    SDF_MemFree(value->data);
    SDF_MemFree(value);
}