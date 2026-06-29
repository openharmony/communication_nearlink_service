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
#include "sdf_mem.h"
#include "sdf_string.h"
#include "nlstk_log.h"
#include "nlstk_ssap_app_server.h"
#include "nlstk_mcp_media.h"
#include "ssap_pkt.h"
#include "mcp_type.h"
#include "mcp_utils.h"
#include "mcp_media_server.h"

#define MCP_CONTROL_CBK(cbk, addr, requestId, instanceId, opCode)                       \
do {                                                                                    \
    if ((cbk) != NULL) {                                                                \
        McpPushMethodCallVector((requestId), (opCode));                                 \
        (cbk)((addr), (requestId), (instanceId));                                       \
    } else {                                                                            \
        NLSTK_LOG_ERROR("[MCP] cbk is null, opcode = %u", (opCode));                      \
        McpPlayControlErrorHandle((requestId), (opCode), NLSTK_MCP_CONTROL_UNEXECUTABLE); \
    }                                                                                   \
} while (0)

static McpMediaService_S g_mcpCommonService = { .appId = MCP_INVALID_APP_ID };

static uint32_t McpAddMediaService(McpMediaService_S *serviceInfo);

static void McpResetMediaService(McpMediaService_S *serviceInfo)
{
    McpFreeMediaInfo(serviceInfo->basicInfo);
    if (serviceInfo->appId >= 0) {
        NLSTK_SsapServerDeregisterApplicationAsync(serviceInfo->appId);
    }
    SDF_DestroyVector(serviceInfo->playReqQue);
    SDF_DestroyVector(serviceInfo->serviceCache.properties);
    (void)memset_s(serviceInfo, sizeof(McpMediaService_S), 0, sizeof(McpMediaService_S));
    serviceInfo->appId = MCP_INVALID_APP_ID;
}

void McpMediaEnable(void)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpMediaEnable");
    McpResetMediaService(&g_mcpCommonService);
}

void McpMediaDisable(void)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpMediaDisable");
    McpResetMediaService(&g_mcpCommonService);
}

static void McpRegisterAppCallback(int32_t appId, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpRegisterAppCallback");
    if (appId < 0 || ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] reg app invalid");
        if (g_mcpCommonService.startMediaInst != NULL) {
            g_mcpCommonService.startMediaInst(g_mcpCommonService.instanceId, NLSTK_ERRCODE_FAIL);
        }
        McpResetMediaService(&g_mcpCommonService);
        return;
    }
    g_mcpCommonService.appId = appId;
    NLSTK_LOG_INFO("[CCP] media reg ssap appId = %d", appId);
    if (McpAddMediaService(&g_mcpCommonService) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] add media service fail");
        if (g_mcpCommonService.startMediaInst != NULL) {
            g_mcpCommonService.startMediaInst(g_mcpCommonService.instanceId, NLSTK_ERRCODE_FAIL);
        }
        McpResetMediaService(&g_mcpCommonService);
    }
}

static void McpFreePropertyCache(void *val)
{
    if (val == NULL) {
        return;
    }
    McpMediaPropertyCache_S *property = (McpMediaPropertyCache_S *)val;
    if (property->value.data != NULL) {
        SDF_MemFree(property->value.data);
    }
    SDF_MemFree(property);
}

static uint32_t McpBuildServiceCache(McpMediaService_S *service, SSAP_Service_S *result)
{
    NLSTK_CHECK_RETURN(result->properties != NULL && result->methods != NULL,
        NLSTK_ERRCODE_POINTER_NULL, "[MCP] service is null");
    service->serviceCache.srvHandle = result->handle;
    service->serviceCache.endHandle = result->endHandle;
    service->serviceCache.uuid = McpConvertUuidTo16Bits(result->uuid);
    SDF_Traits propertyTraits = {.dtor = McpFreePropertyCache};
    service->serviceCache.properties = SDF_CreateVector(propertyTraits);
    NLSTK_CHECK_RETURN(service->serviceCache.properties != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] prty create fail");
    for (size_t i = 0; i < result->properties->size; i++) {
        SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(result->properties, i);
        uint16_t uuid = McpConvertUuidTo16Bits(property->uuid);
        McpMediaPropertyCache_S *propertyCache =
            (McpMediaPropertyCache_S *)SDF_MemZalloc(sizeof(McpMediaPropertyCache_S));
        if (propertyCache == NULL) {
            NLSTK_LOG_ERROR("[MCP] property cache malloc fail");
            SDF_DestroyVector(service->serviceCache.properties);
            service->serviceCache.properties = NULL;
            return NLSTK_ERRCODE_MALLOC_FAIL;
        }
        propertyCache->handle = property->handle;
        propertyCache->uuid = uuid;
        propertyCache->type = McpGetPropertyTypeByUuid(uuid);
        if (propertyCache->type == NLSTK_MCP_PLAYBACK_STATE && property->val != NULL && property->val->len > 0) {
            service->playState = *property->val->value;
        }
        NLSTK_LOG_INFO("[MCP] property handle: %u, uuid: 0x%04x", property->handle, uuid);
        if (!SDF_VectorEmplaceBack(service->serviceCache.properties, propertyCache)) {
            NLSTK_LOG_ERROR("[MCP] property cache emplace back fail");
            SDF_MemFree(propertyCache);
            SDF_DestroyVector(service->serviceCache.properties);
            service->serviceCache.properties = NULL;
            return NLSTK_ERRCODE_FAIL;
        }
    }
    for (size_t i = 0; i < result->methods->size; i++) {
        SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(result->methods, i);
        uint16_t uuid = McpConvertUuidTo16Bits(method->uuid);
        if (uuid == MCP_MEDIA_PLAYBACK_CONTROL_POINT_UUID) {
            service->serviceCache.playControlHandle = method->handle;
        } else if (uuid == MCP_MEDIA_BROWSER_CONTROL_POINT_UUID) {
            service->serviceCache.browseControlHandle = method->handle;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static void McpAddServiceCallback(int32_t appId, SSAP_Service_S *result, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpAddServiceCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_mcpCommonService.appId, "[MCP] appid error");
    if (result == NULL || ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] add result fail");
        if (g_mcpCommonService.startMediaInst != NULL) {
            g_mcpCommonService.startMediaInst(g_mcpCommonService.instanceId, NLSTK_ERRCODE_FAIL);
        }
        McpResetMediaService(&g_mcpCommonService);
        return;
    }
    if (McpBuildServiceCache(&g_mcpCommonService, result) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] service cache build fail");
        if (g_mcpCommonService.startMediaInst != NULL) {
            g_mcpCommonService.startMediaInst(g_mcpCommonService.instanceId, NLSTK_ERRCODE_FAIL);
        }
        McpResetMediaService(&g_mcpCommonService);
        return;
    }
    if (g_mcpCommonService.startMediaInst != NULL) {
        g_mcpCommonService.startMediaInst(g_mcpCommonService.instanceId, NLSTK_ERRCODE_SUCCESS);
    }
}

static void McpPushMethodCallVector(uint16_t requestId, uint8_t opCode)
{
    McpMethodCallRequest_S *request = (McpMethodCallRequest_S *)SDF_MemZalloc(sizeof(McpMethodCallRequest_S));
    NLSTK_CHECK_RETURN_VOID(request != NULL, "[MCP] request malloc fail");
    request->requestId = requestId;
    request->opCode = opCode;
    if (g_mcpCommonService.playReqQue == NULL) {
        SDF_Traits traits = {.dtor = SDF_MemFree};
        g_mcpCommonService.playReqQue = SDF_CreateVector(traits);
        if (g_mcpCommonService.playReqQue == NULL) {
            NLSTK_LOG_ERROR("[MCP] playReqQue create fail");
            SDF_MemFree(request);
            return;
        }
    }
    if (!SDF_VectorEmplaceBack(g_mcpCommonService.playReqQue, request)) {
        NLSTK_LOG_ERROR("[MCP] push back failed");
        SDF_MemFree(request);
    }
}

static McpMethodCallRequest_S *McpPopMethodCallVector(uint16_t requestId, size_t *index)
{
    NLSTK_CHECK_RETURN(g_mcpCommonService.playReqQue != NULL, NULL, "[MCP] pop method vector is null");
    McpMethodCallRequest_S *request = NULL;
    for (size_t i = 0; i < g_mcpCommonService.playReqQue->size; i++) {
        request = SDF_VectorElementAt(g_mcpCommonService.playReqQue, i);
        if (request->requestId == requestId) {
            *index = i;
            return request;
        }
    }
    return NULL;
}

static void McpPlayControlErrorHandle(uint16_t requestId, uint8_t opCode, uint8_t errorCode)
{
    NLSTK_VariableData_S value = {0};
    value.len = MCP_MEDIA_METHOD_RES_LEN;
    value.data = (uint8_t *)SDF_MemZalloc(value.len);
    NLSTK_CHECK_RETURN_VOID(value.data != NULL, "[MCP] value data malloc fail");
    size_t index = 0;
    value.data[index++] = opCode;
    value.data[index++] = errorCode;
    if (NLSTK_SsapServerSendMethodCallRes(g_mcpCommonService.appId, requestId, &value) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] send method call result fail");
    }
    SDF_MemFree(value.data);
}

static NLSTK_McpPlayControlRes_E McpCheckPlayState(uint8_t opCode)
{
    if (g_mcpCommonService.playState == NLSTK_MCP_STATE_UNINITIALIZED) {
        NLSTK_LOG_ERROR("[MCP] play state uninitialized");
        return NLSTK_MCP_MEDIA_NOT_READY;
    }
    switch (opCode) {
        case MCP_STOP_OPCODE:
        case MCP_PAUSE_OPCODE:
            NLSTK_CHECK_RETURN(g_mcpCommonService.playState != NLSTK_MCP_STATE_READY, NLSTK_MCP_CONTROL_UNEXECUTABLE,
                "[MCP] play state error, state = %u", g_mcpCommonService.playState);
            break;
        case MCP_FAST_FORWARD_OPCODE:
            NLSTK_CHECK_RETURN(g_mcpCommonService.playState != NLSTK_MCP_STATE_SEEKING, NLSTK_MCP_CONTROL_UNEXECUTABLE,
                "[MCP] play state error, state = %u", g_mcpCommonService.playState);
            break;
        default:
            break;
    }
    return NLSTK_MCP_CONTROL_SUCCESS;
}

static void McpMethodCallConfirmCallback(int32_t appId, uint16_t requestId,
    NLSTK_SsapServerCallMethodRequestInfo_S *method, bool needReturn, bool needAuth)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpMethodCallConfirmCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_mcpCommonService.appId, "[MCP] appid error");
    if (method == NULL || method->param.len == 0 || method->param.data == NULL) {
        NLSTK_LOG_ERROR("[MCP] method call value error");
        McpPlayControlErrorHandle(requestId, MCP_MEDIA_INVALID_OPCODE, NLSTK_MCP_CONTROL_FAILED);
        return;
    }
    // 当前方法调用参数只有操作码
    uint8_t opCode = *(uint8_t *)method->param.data;
    int32_t instanceId = g_mcpCommonService.instanceId;
    NLSTK_McpPlayControl_S *control = &g_mcpCommonService.playControl;
    SLE_Addr_S copyAddr = {0};
    (void)memcpy_s(&copyAddr, sizeof(SLE_Addr_S), &method->addr, sizeof(SLE_Addr_S));
    NLSTK_LOG_INFO("[MCP] recv media control req, opcode = 0x%02x", opCode);
    uint8_t errorCode = McpCheckPlayState(opCode);
    if (method->handle != g_mcpCommonService.serviceCache.playControlHandle) {
        NLSTK_LOG_ERROR("[MCP] method handle error");
        McpPlayControlErrorHandle(requestId, opCode, NLSTK_MCP_CONTROL_FAILED);
        return;
    }
    if (errorCode != NLSTK_MCP_CONTROL_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] play state error");
        McpPlayControlErrorHandle(requestId, opCode, errorCode);
        return;
    }
    switch (opCode) {
        case MCP_PLAY_OPCODE:
            MCP_CONTROL_CBK(control->play, &copyAddr, requestId, instanceId, opCode);
            break;
        case MCP_STOP_OPCODE:
            MCP_CONTROL_CBK(control->stop, &copyAddr, requestId, instanceId, opCode);
            break;
        case MCP_PAUSE_OPCODE:
            MCP_CONTROL_CBK(control->pause, &copyAddr, requestId, instanceId, opCode);
            break;
        case MCP_FAST_FORWARD_OPCODE:
            MCP_CONTROL_CBK(control->fastForward, &copyAddr, requestId, instanceId, opCode);
            break;
        case MCP_PREVIOUS_MEDIA_OPCODE:
            MCP_CONTROL_CBK(control->previousMedia, &copyAddr, requestId, instanceId, opCode);
            break;
        case MCP_NEXT_MEDIA_OPCODE:
            MCP_CONTROL_CBK(control->nextMedia, &copyAddr, requestId, instanceId, opCode);
            break;
        default:
            NLSTK_LOG_ERROR("[MCP] control opcode unsupport");
            McpPlayControlErrorHandle(requestId, opCode, NLSTK_MCP_CONTROL_UNSUPPORTED);
            break;
    }
}

static bool McpCompPropertyHandle(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    McpMediaPropertyCache_S *property = (McpMediaPropertyCache_S *)ptr;
    uint16_t *handle = (uint16_t *)args;
    return property->handle == *handle;
}

static void McpAuthorizeCallback(int32_t appId,  uint16_t requestId, NLSTK_SsapServerReadPropertyInfo_S *property)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpAuthorizeCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_mcpCommonService.appId, "[MCP] appid error");
    if (property == NULL) {
        NLSTK_LOG_ERROR("[MCP] property is null");
        NLSTK_SsapServerAuthorizeResult(g_mcpCommonService.appId, requestId, false);
        return;
    }
    NLSTK_ServicePropertyOpType_E operation = NLSTK_SSAP_PROPERTY_READ;
    size_t index = 0;
    uint16_t handle = property->handle;
    if (handle > g_mcpCommonService.serviceCache.srvHandle && handle <= g_mcpCommonService.serviceCache.endHandle) {
        NLSTK_CHECK_RETURN_VOID(
            SDF_VectorFindFirst(g_mcpCommonService.serviceCache.properties, McpCompPropertyHandle, &handle, &index),
            "[MCP] property handle not found");
        McpMediaPropertyCache_S *propertyCache = SDF_VectorElementAt(g_mcpCommonService.serviceCache.properties, index);
        NLSTK_McpPropertyType_E type = propertyCache->type;
        if (g_mcpCommonService.authorize != NULL) {
            g_mcpCommonService.authorize(requestId, g_mcpCommonService.instanceId, type, operation);
        } else {
            NLSTK_LOG_ERROR("[MCP] authorize cbk is null");
            NLSTK_SsapServerAuthorizeResult(g_mcpCommonService.appId, requestId, false);
        }
        return;
    }
    NLSTK_LOG_ERROR("[MCP] property handle not found");
    NLSTK_SsapServerAuthorizeResult(g_mcpCommonService.appId, requestId, false);
}

static void McpUpdatePropertyCallBack(int32_t appId, NLSTK_SsapServerOnSetPropertyParam_S *property,
    NLSTK_Errcode_E ret)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpUpdatePropertyCallBack");
    NLSTK_CHECK_RETURN_VOID(appId == g_mcpCommonService.appId, "[MCP] appid error");
    NLSTK_CHECK_RETURN_VOID(property != NULL, "[MCP] property is null");
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[MCP] update property fail");
    uint16_t uuid = McpConvertUuidTo16Bits(property->uuid);
    if (McpGetPropertyTypeByUuid(uuid) == NLSTK_MCP_PLAYBACK_STATE && property->value.data != NULL) {
        g_mcpCommonService.playState = *property->value.data;
        NLSTK_LOG_INFO("[MCP] play state = %u", g_mcpCommonService.playState);
    }
}

static void McpWriteDescriptorCallback(int32_t appId, NLSTK_SsapServerWriteDescriptorInfo_S *descriptor)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpWriteDescriptorCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_mcpCommonService.appId, "[MCP] appid error");
    NLSTK_CHECK_RETURN_VOID(descriptor != NULL, "[MCP] descriptor is null");
}

static void McpAddMediaServiceStatement(NLSTK_ServiceParam_S *service, NLSTK_McpServiceType_E type)
{
    NLSTK_SsapServiceStatementParam_S *statement = &service->serviceStatement;
    uint16_t uuid = type == NLSTK_MCP_SERVICE ? MCP_MEDIA_SERVICE_UUID : MCP_COMMON_MEDIA_SERVICE_UUID;
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(uuid);
    (void)memcpy_s(&statement->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    statement->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
}

static void McpSetPersimissonAndOperation(NLSTK_SsapServicePropertyParam_S *property, uint8_t right, bool readOnly)
{
    // 根据right添加权限
    property->permission.permissionValue = right;

    uint32_t operation = 0;
    if (readOnly == true) {
        // 添加操作指示，只支持读
        operation = SSAP_OPERATE_INDICATION_READ;
    } else {
        // 添加操作指示，应支持读、通知、指示、写入客户端属性配置描述符
        operation = SSAP_OPERATE_INDICATION_READ | SSAP_OPERATE_INDICATION_NOTIFY |
            SSAP_OPERATE_INDICATION_INDICATE | SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
    }
    property->operation.operationValue = operation;
}

// 创建描述符，若失败由调用者统一释放内存
static uint32_t McpPropertySetDescriptor(NLSTK_SsapServicePropertyParam_S *property)
{
    // 目前只需要客户端属性配置描述符，后续可拓展；
    property->descriptors = (NLSTK_SsapServiceDescriptorParam_S *)SDF_MemZalloc(
        sizeof(NLSTK_SsapServiceDescriptorParam_S));
    NLSTK_CHECK_RETURN(property->descriptors != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property descriptors malloc fail");
    property->descriptorNum = 1;
    // perimission暂置为0，无需授权、认证、加密
    property->descriptors[0].type = DESC_TYPE_CLIENT_CONFIG;
    property->descriptors[0].operation.operationValue = SSAP_OPERATE_INDICATION_READ |
        SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
    property->descriptors[0].val.len = MCP_DESC_CLIENT_CONFIG_LEN;
    property->descriptors[0].val.data = (uint8_t *)SDF_MemZalloc(MCP_DESC_CLIENT_CONFIG_LEN);
    NLSTK_CHECK_RETURN(property->descriptors[0].val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property descriptor value malloc fail");
    return NLSTK_ERRCODE_SUCCESS;
}

static uint32_t McpAddMediaInstanceName(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_MEDIA_INSTANCE_NAME_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_MEDIA_INSTANCE_NAME];
    McpSetPersimissonAndOperation(property, right, true);
    if (basicInfo->instanceName.len == 0 || basicInfo->instanceName.data == NULL) {
        return McpPropertySetDescriptor(property);
    }
    property->val.len = basicInfo->instanceName.len;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for instance name malloc fail");
    (void)memcpy_s(property->val.data, property->val.len, basicInfo->instanceName.data, basicInfo->instanceName.len);
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaInstanceIcon(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_MEDIA_INSTANCE_ICON_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_MEDIA_INSTANCE_ICON];
    McpSetPersimissonAndOperation(property, right, true);
    property->val.len = basicInfo->optionalItem.instanceIcon.iconLen + MCP_MEDIA_ICON_TYPE_LEN;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for instance icon malloc fail");
    property->val.data[0] = basicInfo->optionalItem.instanceIcon.iconType;
    if (basicInfo->optionalItem.instanceIcon.iconLen != 0) {
        (void)memcpy_s(property->val.data + MCP_MEDIA_ICON_TYPE_LEN, basicInfo->optionalItem.instanceIcon.iconLen,
            basicInfo->optionalItem.instanceIcon.iconValue, basicInfo->optionalItem.instanceIcon.iconLen);
    }
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaBaseInfo(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_MEDIA_BASIC_INFO_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_MEDIA_BASIC_INFO];
    McpSetPersimissonAndOperation(property, right, false);
    property->val.len = MCP_MEDIA_NAME_OFFSET + basicInfo->mediaBaseInfo.mediaNameLen;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for basic info malloc fail");
    property->val.data[0] = basicInfo->mediaBaseInfo.mediaType;
    (void)memcpy_s(property->val.data + MCP_MEDIA_TYPE_LEN, MCP_MEDIA_DURATION_LEN,
        &basicInfo->mediaBaseInfo.duration, MCP_MEDIA_DURATION_LEN);
    property->val.data[MCP_MEDIA_NAME_LEN_OFFSET] = basicInfo->mediaBaseInfo.mediaNameLen;
    if (basicInfo->mediaBaseInfo.mediaNameLen != 0) {
        (void)memcpy_s(property->val.data + MCP_MEDIA_NAME_OFFSET, basicInfo->mediaBaseInfo.mediaNameLen,
            basicInfo->mediaBaseInfo.mediaName, basicInfo->mediaBaseInfo.mediaNameLen);
    }
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaExtendedInfo(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_MEDIA_EXTENDED_INFO_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_MEDIA_EXTENDED_INFO];
    McpSetPersimissonAndOperation(property, right, false);
    if (basicInfo->optionalItem.mediaExtendedInfo.len == 0 || basicInfo->optionalItem.mediaExtendedInfo.data == NULL) {
        return McpPropertySetDescriptor(property);
    }
    property->val.len = basicInfo->optionalItem.mediaExtendedInfo.len;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for extended info malloc fail");
    (void)memcpy_s(property->val.data, property->val.len, basicInfo->optionalItem.mediaExtendedInfo.data,
        basicInfo->optionalItem.mediaExtendedInfo.len);
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaIdInfo(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_MEDIA_IDENTIFIER_INFO_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_MEDIA_IDENTIFIER_INFO];
    McpSetPersimissonAndOperation(property, right, false);
    // 若当前媒体的媒体类型为未指定媒体，媒体标识信息的属性值为空
    if (basicInfo->mediaBaseInfo.mediaType == NLSTK_MEDIA_UNSPECIFIED) {
        return McpPropertySetDescriptor(property);
    }
    property->val.len = sizeof(basicInfo->optionalItem.mediaIdInfo);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for identifier info malloc fail");
    (void)memcpy_s(property->val.data, property->val.len, &basicInfo->optionalItem.mediaIdInfo, property->val.len);
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaSegmentInfo(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_MEDIA_SEGMENT_INFO_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_MEDIA_SEGMENT_INFO];
    McpSetPersimissonAndOperation(property, right, false);
    if (basicInfo->optionalItem.segmentInfo.len == 0 || basicInfo->optionalItem.segmentInfo.data == NULL) {
        return McpPropertySetDescriptor(property);
    }
    property->val.len = basicInfo->optionalItem.segmentInfo.len;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for segement info malloc fail");
    (void)memcpy_s(property->val.data, property->val.len,
        basicInfo->optionalItem.segmentInfo.data, basicInfo->optionalItem.segmentInfo.len);
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaPlaybackLocation(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_MEDIA_PLAYBACK_POSITION_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_MEDIA_PLAYBACK_POSITION];
    McpSetPersimissonAndOperation(property, right, false);
    // 若当前媒体类型为未指定媒体时，媒体播放位置属性的属性值为空
    if (basicInfo->mediaBaseInfo.mediaType == NLSTK_MEDIA_UNSPECIFIED) {
        return McpPropertySetDescriptor(property);
    }
    property->val.len = sizeof(basicInfo->playbackLocation);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for playback location malloc fail");
    (void)memcpy_s(property->val.data, property->val.len, &basicInfo->playbackLocation, property->val.len);
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaPlaybackSpeed(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_PLAYBACK_SPEED_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_PLAYBACK_SPEED];
    McpSetPersimissonAndOperation(property, right, false);
    // 若当前媒体类型为未指定媒体，播放速度属性的属性值为空
    if (basicInfo->mediaBaseInfo.mediaType == NLSTK_MEDIA_UNSPECIFIED) {
        return McpPropertySetDescriptor(property);
    }
    property->val.len = sizeof(basicInfo->optionalItem.playbackSpeed);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for playback speed malloc fail");
    (void)memcpy_s(property->val.data, property->val.len, &basicInfo->optionalItem.playbackSpeed, property->val.len);
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaSeekSpeed(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_SEEK_SPEED_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_SEEK_SPEED];
    McpSetPersimissonAndOperation(property, right, false);
    property->val.len = sizeof(basicInfo->optionalItem.seekSpeed);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for seek speed malloc fail");
    // 若当前媒体类型为未指定媒体时，寻道速度为0
    if (basicInfo->mediaBaseInfo.mediaType == NLSTK_MEDIA_UNSPECIFIED) {
        return McpPropertySetDescriptor(property);
    }
    (void)memcpy_s(property->val.data, property->val.len, &basicInfo->optionalItem.seekSpeed, property->val.len);
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaFeaturesSupported(NLSTK_SsapServicePropertyParam_S *property,
    NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_FEATURE_SUPPORT_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_FEATURE_SUPPORT];
    McpSetPersimissonAndOperation(property, right, false);
    property->val.len = MCP_FEATURE_SUPPORT_LEN;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for features supported malloc fail");
    property->val.data[0] = basicInfo->featuresSupported.FeatureType;
    (void)memcpy_s(property->val.data + MCP_CTRL_FEATURES_OFFSET, sizeof(basicInfo->featuresSupported.playCtl),
        &basicInfo->featuresSupported.playCtl, sizeof(basicInfo->featuresSupported.playCtl));
    (void)memcpy_s(property->val.data + MCP_MODEL_FEATURES_OFFSET, sizeof(basicInfo->featuresSupported.playMode),
        &basicInfo->featuresSupported.playMode, sizeof(basicInfo->featuresSupported.playMode));
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaPlaybackOrder(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_PLAYBACK_ORDER_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_PLAYBACK_ORDER];
    McpSetPersimissonAndOperation(property, right, false);
    property->val.len = sizeof(basicInfo->optionalItem.playbackOrder);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for playback order malloc fail");
    *property->val.data = basicInfo->optionalItem.playbackOrder;
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaPlaybackState(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_PLAYBACK_STATE_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_PLAYBACK_STATE];
    McpSetPersimissonAndOperation(property, right, false);
    property->val.len = sizeof(basicInfo->playbackState);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for playback state malloc fail");
    *property->val.data = basicInfo->playbackState;
    return McpPropertySetDescriptor(property);
}

static uint32_t McpAddMediaInstanceId(NLSTK_SsapServicePropertyParam_S *property, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_MEDIA_INSTANCE_ID_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = basicInfo->propertyRights[NLSTK_MCP_MEDIA_INSTANCE_ID];
    McpSetPersimissonAndOperation(property, right, true);
    property->val.len = sizeof(basicInfo->mediaInstanceId);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] property value for instance id malloc fail");
    *property->val.data = basicInfo->mediaInstanceId;
    return McpPropertySetDescriptor(property);
}

uint16_t McpCountSetFlagPropertyNum(NLSTK_McpOptionalFlag_S flags) {
    uint16_t count = 0;
    count += flags.flagBit.instanceIconFlag;
    count += flags.flagBit.mediaExtendedInfoFlag;
    count += flags.flagBit.mediaIdInfoFlag;
    count += flags.flagBit.segmentInfoFlag;
    count += flags.flagBit.playbackSpeedFlag;
    count += flags.flagBit.seekSpeedFlag;
    count += flags.flagBit.playbackOrderFlag;
    return count;
}

// 组装服务属性，若失败由调用者统一释放内存
static uint32_t McpAddMediaServiceProperty(NLSTK_ServiceParam_S *service, NLSTK_McpMediaInfo_S *basicInfo)
{
    NLSTK_McpOptionalFlag_S flags = basicInfo->optionalItem.flags;
    service->servicePropertyNum = MCP_MEDIA_REQUIRED_PROPERTY_NUM + McpCountSetFlagPropertyNum(flags);
    service->property = (NLSTK_SsapServicePropertyParam_S *)SDF_MemZalloc(
        sizeof(NLSTK_SsapServicePropertyParam_S) * service->servicePropertyNum);
    NLSTK_CHECK_RETURN(service->property != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] property malloc fail");
    NLSTK_SsapServicePropertyParam_S *property = service->property;
    size_t index = 0;
    uint32_t ret = McpAddMediaInstanceName(&property[index++], basicInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media instance name fail");
    ret = McpAddMediaBaseInfo(&property[index++], basicInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media base info fail");
    ret = McpAddMediaPlaybackLocation(&property[index++], basicInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media playback location fail");
    ret = McpAddMediaFeaturesSupported(&property[index++], basicInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media features supported fail");
    ret = McpAddMediaPlaybackState(&property[index++], basicInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media playback state fail");
    ret = McpAddMediaInstanceId(&property[index++], basicInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media instance id fail");
    if (flags.flagBit.instanceIconFlag != 0) {
        ret = McpAddMediaInstanceIcon(&property[index++], basicInfo);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media instance icon fail");
    }
    if (flags.flagBit.mediaExtendedInfoFlag != 0) {
        ret = McpAddMediaExtendedInfo(&property[index++], basicInfo);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media extended info fail");
    }
    if (flags.flagBit.mediaIdInfoFlag != 0) {
        ret = McpAddMediaIdInfo(&property[index++], basicInfo);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media id info fail");
    }
    if (flags.flagBit.segmentInfoFlag != 0) {
        ret = McpAddMediaSegmentInfo(&property[index++], basicInfo);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media segment info fail");
    }
    if (flags.flagBit.playbackSpeedFlag != 0) {
        ret = McpAddMediaPlaybackSpeed(&property[index++], basicInfo);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media playback speed fail");
    }
    if (flags.flagBit.seekSpeedFlag != 0) {
        ret = McpAddMediaSeekSpeed(&property[index++], basicInfo);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media seek speed fail");
    }
    if (flags.flagBit.playbackOrderFlag != 0) {
        ret = McpAddMediaPlaybackOrder(&property[index++], basicInfo);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[MCP] add media playback order fail");
    }
    return NLSTK_ERRCODE_SUCCESS;
}

// 组装服务方法，若失败由调用者统一释放内存
static uint32_t McpAddMediaServiceMethod(NLSTK_ServiceParam_S *service)
{
    // 目前只支持方法：媒体播放控制点
    service->method = (NLSTK_SsapServiceMethodParam_S *)SDF_MemZalloc(
        MCP_MEDIA_METHOD_NUM * sizeof(NLSTK_SsapServiceMethodParam_S));
    NLSTK_CHECK_RETURN(service->method != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[MCP] service method malloc fail");
    service->serviceMethodNum = MCP_MEDIA_METHOD_NUM;
    service->method[0].type = ITEM_TYPE_STD_METHOD;
    service->method[0].permission.permissionValue =
        SSAP_PERMISSION_AUTHENTICATION_NEED | SSAP_PERMISSION_ENCRYPTION_NEED;
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru(MCP_MEDIA_PLAYBACK_CONTROL_POINT_UUID);
    (void)memcpy_s(&service->method[0].uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    return NLSTK_ERRCODE_SUCCESS;
}

static uint32_t McpAddMediaService(McpMediaService_S *serviceInfo)
{
    NLSTK_CHECK_RETURN(serviceInfo->basicInfo != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] basicInfo is null");
    NLSTK_ServiceParam_S *service = (NLSTK_ServiceParam_S *)SDF_MemZalloc(sizeof(NLSTK_ServiceParam_S));
    NLSTK_CHECK_RETURN(service != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] service malloc fail");
    McpAddMediaServiceStatement(service, serviceInfo->basicInfo->type);
    if (McpAddMediaServiceProperty(service, serviceInfo->basicInfo) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] add service property fail");
        NLSTK_SsapFreeServiceParam(service);
        return NLSTK_ERRCODE_FAIL;
    }
    if (McpAddMediaServiceMethod(service) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] add service method fail");
        NLSTK_SsapFreeServiceParam(service);
        return NLSTK_ERRCODE_FAIL;
    }
    if (NLSTK_SsapServerAddService(serviceInfo->appId, service) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] add service fail");
        NLSTK_SsapFreeServiceParam(service);
        return NLSTK_ERRCODE_FAIL;
    }
    NLSTK_SsapFreeServiceParam(service);
    return NLSTK_ERRCODE_SUCCESS;
}

void McpCreateMediaInstance(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpCreateMediaInstance");
    NLSTK_McpMediaInfo_S *param = (NLSTK_McpMediaInfo_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[MCP] param is null");
    NLSTK_McpMediaInfo_S *basicInfo = (NLSTK_McpMediaInfo_S *)SDF_MemZalloc(sizeof(NLSTK_McpMediaInfo_S));
    NLSTK_CHECK_RETURN_VOID(basicInfo != NULL, "[MCP] basicInfo malloc fail");
    if (McpCopyMediaInfo(basicInfo, param) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] copy media info failed");
        McpFreeMediaInfo(basicInfo);
        return;
    }
    if (basicInfo->type == NLSTK_MCP_COMMON_SERVICE && g_mcpCommonService.used == true) {
        NLSTK_LOG_ERROR("[MCP] has added common media service");
        if (basicInfo->startMediaInst != NULL) {
            basicInfo->startMediaInst(basicInfo->mediaInstanceId, NLSTK_ERRCODE_FAIL);
        }
        McpFreeMediaInfo(basicInfo);
        return;
    } else if (basicInfo->type == NLSTK_MCP_SERVICE) {
        NLSTK_LOG_ERROR("[MCP] media service unsupport");
        if (basicInfo->startMediaInst != NULL) {
            basicInfo->startMediaInst(basicInfo->mediaInstanceId, NLSTK_ERRCODE_FAIL);
        }
        McpFreeMediaInfo(basicInfo);
        return;
    }
    NLSTK_SsapAppServerCb_S cb = {0};
    cb.onRegisterApp = McpRegisterAppCallback;
    cb.onAddService = McpAddServiceCallback;
    cb.onSetPropertyValue = McpUpdatePropertyCallBack;
    cb.onCallMethod = McpMethodCallConfirmCallback;
    cb.onReadPropertyAuthorizeRequest = McpAuthorizeCallback;
    cb.onWriteDescriptor = McpWriteDescriptorCallback;
    if (NLSTK_SsapServerRegAppAsyn(&cb) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] reg app fail");
        if (basicInfo->startMediaInst != NULL) {
            basicInfo->startMediaInst(basicInfo->mediaInstanceId, NLSTK_ERRCODE_FAIL);
        }
        McpFreeMediaInfo(basicInfo);
        return;
    }
    g_mcpCommonService.used = true;
    g_mcpCommonService.instanceId = basicInfo->mediaInstanceId;
    g_mcpCommonService.playControl = basicInfo->playbackControlPoint;
    g_mcpCommonService.startMediaInst = basicInfo->startMediaInst;
    g_mcpCommonService.authorize = basicInfo->authorize;
    g_mcpCommonService.basicInfo = basicInfo;
}

void McpPlayControlResult(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpPlayControlResult");
    McpPlayControlResultParam_S *param = (McpPlayControlResultParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[MCP] conrol result param is null");
    NLSTK_CHECK_RETURN_VOID(param->instanceId == g_mcpCommonService.instanceId, "[MCP] instance id invalid");
    size_t index = 0;
    McpMethodCallRequest_S *request = McpPopMethodCallVector(param->requestId, &index);
    NLSTK_CHECK_RETURN_VOID(request != NULL, "[MCP] call request is null");
    uint8_t opCode = request->opCode;
    SDF_VectorRemove(g_mcpCommonService.playReqQue, index);
    NLSTK_VariableData_S value = {0};
    value.len = MCP_MEDIA_METHOD_RES_LEN;
    value.data = (uint8_t *)SDF_MemZalloc(value.len);
    NLSTK_CHECK_RETURN_VOID(value.data != NULL, "[MCP] value data malloc fail");
    index = 0;
    value.data[index++] = opCode;
    value.data[index++] = param->errorCode;
    if (NLSTK_SsapServerSendMethodCallRes(g_mcpCommonService.appId, param->requestId, &value) !=
        NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] send method call result fail");
    }
    SDF_MemFree(value.data);
}

void McpMediaAuthorizeResult(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpMediaAuthorizeResult");
    McpMediaAuthorizeResultParam_S *param = (McpMediaAuthorizeResultParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[MCP] param is null");
    NLSTK_CHECK_RETURN_VOID(param->instanceId == g_mcpCommonService.instanceId, "[MCP] instance id invalid");
    uint16_t requestId = param->requestId;
    bool allow = param->errorCode == NLSTK_ERRCODE_SUCCESS ? true : false;
    if (NLSTK_SsapServerAuthorizeResult(g_mcpCommonService.appId, requestId, allow) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] authorize result fail");
    }
}

static bool McpCompPropertyType(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    McpMediaPropertyCache_S *property = (McpMediaPropertyCache_S *)ptr;
    uint16_t *type = (uint16_t *)args;
    return property->type == *type;
}

void McpUpdateMediaProperty(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpUpdateMediaProperty");
    McpUpdateMediaPropertyParam_S *param = (McpUpdateMediaPropertyParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[MCP] param is null");
    NLSTK_CHECK_RETURN_VOID(param->value != NULL, "[MCP] param value is null");
    NLSTK_CHECK_RETURN_VOID(param->instanceId == g_mcpCommonService.instanceId, "[MCP] instance id invalid");
    if (param->value->len != 0) {
        NLSTK_CHECK_RETURN_VOID(param->value->data != NULL, "[MCP] param value data is null");
    }
    McpMediaServiceCache_S *serviceCache = &g_mcpCommonService.serviceCache;
    size_t index = 0;
    NLSTK_CHECK_RETURN_VOID(
        SDF_VectorFindFirst(serviceCache->properties, McpCompPropertyType, &param->type, &index),
        "[MCP] property type not found");
    McpMediaPropertyCache_S *propertyCache = SDF_VectorElementAt(serviceCache->properties, index);
    NLSTK_CHECK_RETURN_VOID(propertyCache != NULL, "[MCP] property cache is null");
    uint16_t handle = propertyCache->handle;
    NLSTK_VariableData_S value = {0};
    value.len = param->value->len;
    if (value.len != 0) {
        value.data = (uint8_t *)SDF_MemZalloc(value.len);
        NLSTK_CHECK_RETURN_VOID(value.data != NULL, "[MCP] value data malloc fail");
        (void)memcpy_s(value.data, value.len, param->value->data, param->value->len);
    }
    if (NLSTK_SsapServerUpdatePropertyValue(g_mcpCommonService.appId, handle, &value) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] update property fail");
    }
    SDF_MemFree(value.data);
}

void McpDeleteMediaInstance(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] enter McpDeleteMediaInstance");
    int32_t *instanceId = (int32_t *)arg;
    NLSTK_CHECK_RETURN_VOID(instanceId != NULL, "[MCP] instanceId is null");
    NLSTK_CHECK_RETURN_VOID(*instanceId == g_mcpCommonService.instanceId, "[MCP] invalid instanceId");
    McpResetMediaService(&g_mcpCommonService);
}