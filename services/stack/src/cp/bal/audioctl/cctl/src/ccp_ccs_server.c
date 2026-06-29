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
#include <stdbool.h>
#include "securec.h"
#include "sdf_mem.h"
#include "sdf_string.h"
#include "nlstk_log.h"
#include "ssap_type.h"
#include "nlstk_ssap_app_server.h"
#include "nlstk_ccp.h"
#include "ccp_utils.h"
#include "ccp_ccs_server.h"

static CcpCallControlService_S g_ccpCommonService = { .appId = CCP_INVALID_APP_ID };

static uint32_t CcpAddCallControlService(CcpCallControlService_S *serviceInfo);

static void CcpResetCallControlService(CcpCallControlService_S *serviceInfo)
{
    CcpFreeCallControlInfo(serviceInfo->baseInfo);
    if (serviceInfo->appId >= 0) {
        NLSTK_SsapServerDeregisterApplicationAsync(serviceInfo->appId);
    }
    SDF_DestroyVector(serviceInfo->serviceCache.properties);
    SDF_DestroyVector(serviceInfo->callReqQue);
    SDF_DestroyVector(serviceInfo->callStatusVec);
    (void)memset_s(serviceInfo, sizeof(CcpCallControlService_S), 0, sizeof(CcpCallControlService_S));
    serviceInfo->appId = CCP_INVALID_APP_ID;
}

void CcpCcsEnable(void)
{
    NLSTK_LOG_INFO("[CCP] enter CcpCcsEnable");
    CcpResetCallControlService(&g_ccpCommonService);
}

void CcpCcsDisable(void)
{
    NLSTK_LOG_INFO("[CCP] enter CcpCcsDisable");
    CcpResetCallControlService(&g_ccpCommonService);
}

void CcpCcsRegisterAppCallback(int32_t appId, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[CCP] enter CcpCcsRegisterAppCallback");
    if (appId < 0 || ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] reg app invalid");
        if (g_ccpCommonService.startCcsInst != NULL) {
            g_ccpCommonService.startCcsInst(g_ccpCommonService.instanceId, NLSTK_ERRCODE_FAIL);
        }
        CcpResetCallControlService(&g_ccpCommonService);
        return;
    }
    g_ccpCommonService.appId = appId;
    NLSTK_LOG_INFO("[CCP] ccs reg ssap appId = %d", appId);
    if (CcpAddCallControlService(&g_ccpCommonService) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] add call control service fail");
        if (g_ccpCommonService.startCcsInst != NULL) {
            g_ccpCommonService.startCcsInst(g_ccpCommonService.instanceId, NLSTK_ERRCODE_FAIL);
        }
        CcpResetCallControlService(&g_ccpCommonService);
    }
}

static void CcpCcsFreePropertyCache(void *val)
{
    if (val == NULL) {
        return;
    }
    CcpCcsPropertyCache_S *property = (CcpCcsPropertyCache_S *)val;
    if (property->value.data != NULL) {
        SDF_MemFree(property->value.data);
    }
    SDF_MemFree(property);
}

static uint32_t CcpBuildServiceCache(CcpCallControlService_S *service, SSAP_Service_S *result)
{
    NLSTK_CHECK_RETURN(result->properties != NULL && result->methods != NULL,
        NLSTK_ERRCODE_POINTER_NULL, "[CCP] service is null");
    NLSTK_CHECK_RETURN(result->properties->size < CCP_CCS_MAX_DATA_LEN,
        NLSTK_ERRCODE_PARAM_ERR, "[CCP] service properties size is invalid");
    NLSTK_CHECK_RETURN(result->methods->size < CCP_CCS_MAX_DATA_LEN,
        NLSTK_ERRCODE_PARAM_ERR, "[CCP] service methods size is invalid");
    service->serviceCache.srvHandle = result->handle;
    service->serviceCache.endHandle = result->endHandle;
    service->serviceCache.uuid = CcpConvertUuidTo16Bits(result->uuid);
    SDF_Traits propertyTraits = {.dtor = CcpCcsFreePropertyCache};
    service->serviceCache.properties = SDF_CreateVector(propertyTraits);
    NLSTK_CHECK_RETURN(service->serviceCache.properties != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] prty create fail");
    for (size_t i = 0; i < result->properties->size; i++) {
        SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(result->properties, i);
        uint16_t uuid = CcpConvertUuidTo16Bits(property->uuid);
        CcpCcsPropertyCache_S *propertyCache = (CcpCcsPropertyCache_S *)SDF_MemZalloc(sizeof(CcpCcsPropertyCache_S));
        if (propertyCache == NULL) {
            NLSTK_LOG_ERROR("[CCP] property cache malloc fail");
            SDF_DestroyVector(service->serviceCache.properties);
            service->serviceCache.properties = NULL;
            return NLSTK_ERRCODE_MALLOC_FAIL;
        }
        propertyCache->handle = property->handle;
        propertyCache->uuid = uuid;
        propertyCache->type = CcpGetPropertyTypeByUuid(uuid);
        NLSTK_LOG_INFO("[CCP] property handle: %u, uuid: 0x%04x", property->handle, uuid);
        if (!SDF_VectorEmplaceBack(service->serviceCache.properties, propertyCache)) {
            NLSTK_LOG_ERROR("[CCP] property cache emplace back fail");
            SDF_MemFree(propertyCache);
            SDF_DestroyVector(service->serviceCache.properties);
            service->serviceCache.properties = NULL;
            return NLSTK_ERRCODE_FAIL;
        }
    }
    for (size_t i = 0; i < result->methods->size; i++) {
        SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(result->methods, i);
        uint16_t uuid = CcpConvertUuidTo16Bits(method->uuid);
        if (uuid == CCP_CCS_CALL_CONTROL_POINT_UUID) {
            service->serviceCache.callControlHandle = method->handle;
        } else {
            NLSTK_LOG_ERROR("[CCP] invalid method uuid");
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void CcpCcsStartServiceCallback(int32_t appId, SSAP_Service_S *result, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[CCP] enter CcpCcsStartServiceCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_ccpCommonService.appId, "[CCP] appid error");
    if (result == NULL || ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] add result fail");
        if (g_ccpCommonService.startCcsInst != NULL) {
            g_ccpCommonService.startCcsInst(g_ccpCommonService.instanceId, NLSTK_ERRCODE_FAIL);
        }
        CcpResetCallControlService(&g_ccpCommonService);
        return;
    }
    if (CcpBuildServiceCache(&g_ccpCommonService, result) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] service cache build error");
        if (g_ccpCommonService.startCcsInst != NULL) {
            g_ccpCommonService.startCcsInst(g_ccpCommonService.instanceId, NLSTK_ERRCODE_FAIL);
        }
        CcpResetCallControlService(&g_ccpCommonService);
        return;
    }
    if (g_ccpCommonService.startCcsInst != NULL) {
        g_ccpCommonService.startCcsInst(g_ccpCommonService.instanceId, NLSTK_ERRCODE_SUCCESS);
    }
}

static void CcpPushMethodCallVector(uint16_t requestId, uint8_t opCode, uint8_t callId)
{
    CcpCcsMethodCallRequest_S *request = (CcpCcsMethodCallRequest_S *)SDF_MemZalloc(sizeof(CcpCcsMethodCallRequest_S));
    NLSTK_CHECK_RETURN_VOID(request != NULL, "[CCP] request malloc fail");
    request->requestId = requestId;
    request->opCode = opCode;
    request->callId = callId;
    if (g_ccpCommonService.callReqQue == NULL) {
        SDF_Traits traits = {.dtor = SDF_MemFree};
        g_ccpCommonService.callReqQue = SDF_CreateVector(traits);
        if (g_ccpCommonService.callReqQue == NULL) {
            NLSTK_LOG_ERROR("[CCP] create callReqQue failed");
            SDF_MemFree(request);
            return;
        }
    }
    if (!SDF_VectorEmplaceBack(g_ccpCommonService.callReqQue, request)) {
        NLSTK_LOG_ERROR("[CCP] push back failed");
        SDF_MemFree(request);
    }
}

static CcpCcsMethodCallRequest_S *CcpPopMethodCallVector(uint16_t requestId, size_t *index)
{
    NLSTK_CHECK_RETURN(g_ccpCommonService.callReqQue != NULL, NULL, "[CCP] pop method vector is null");
    CcpCcsMethodCallRequest_S *request = NULL;
    for (size_t i = 0; i < g_ccpCommonService.callReqQue->size; i++) {
        request = SDF_VectorElementAt(g_ccpCommonService.callReqQue, i);
        if (request->requestId == requestId) {
            *index = i;
            return request;
        }
    }
    return NULL;
}

static void CcpCallControlErrorHandle(uint16_t requestId, uint8_t opCode, uint8_t errorcode)
{
    NLSTK_VariableData_S value = {0};
    value.len = CCP_CCS_METHOD_RES_ERROR_LEN;
    value.data = (uint8_t *)SDF_MemZalloc(value.len);
    NLSTK_CHECK_RETURN_VOID(value.data != NULL, "[CCP] value data malloc fail");
    size_t index = 0;
    value.data[index++] = opCode;
    value.data[index++] = errorcode;
    if (NLSTK_SsapServerSendMethodCallRes(g_ccpCommonService.appId, requestId, &value) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] send method call result fail");
    }
    SDF_MemFree(value.data);
}

static void CcpCallControlStmErrorHandle(uint16_t requestId, uint8_t opCode, uint8_t errorcode,
                                         CcpCcsCallStatus_S *callStatus)
{
    NLSTK_VariableData_S value = {0};
    value.len = CCP_CCS_METHOD_RES_STM_ERROR_LEN;
    value.data = (uint8_t *)SDF_MemZalloc(value.len);
    NLSTK_CHECK_RETURN_VOID(value.data != NULL, "[CCP] value data malloc fail");
    size_t index = 0;
    value.data[index++] = opCode;
    value.data[index++] = errorcode;
    value.data[index++] = callStatus->callId;
    value.data[index++] = callStatus->callStatus;
    if (NLSTK_SsapServerSendMethodCallRes(g_ccpCommonService.appId, requestId, &value) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] send method call result fail");
    }
    SDF_MemFree(value.data);
}

static bool CcpCompcallId(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    CcpCcsCallStatus_S *callStatus = (CcpCcsCallStatus_S *)ptr;
    uint8_t *callId = (uint8_t *)args;
    return callStatus->callId == *callId;
}

static void CcpAnswer(SLE_Addr_S *addr, uint16_t requestId, uint8_t opCode, CcpCcsCallStatus_S *callStatus)
{
    if (callStatus->callStatus != NLSTK_CCP_CCS_INCOMING_CALL) {
        NLSTK_LOG_ERROR("[CCP] state machine error when answer");
        CcpCallControlStmErrorHandle(requestId, opCode, NLSTK_CCP_CCS_STATE_MACHINE_ERROR, callStatus);
        return;
    }
    NLSTK_CcpCallControlPoint_S *control = &g_ccpCommonService.callControl;
    if (control->answer != NULL) {
        CcpPushMethodCallVector(requestId, opCode, callStatus->callId);
        control->answer(addr, g_ccpCommonService.instanceId, requestId, callStatus->callId);
    } else {
        NLSTK_LOG_ERROR("[CCP] answer cbk is null");
        CcpCallControlErrorHandle(requestId, opCode, NLSTK_CCP_CCS_OPERATION_FAILED);
    }
}

static void CcpHangUp(SLE_Addr_S *addr, uint16_t requestId, uint8_t opCode, CcpCcsCallStatus_S *callStatus)
{
    NLSTK_CcpCallControlPoint_S *control = &g_ccpCommonService.callControl;
    if (control->hangUp != NULL) {
        CcpPushMethodCallVector(requestId, opCode, callStatus->callId);
        control->hangUp(addr, g_ccpCommonService.instanceId, requestId, callStatus->callId);
    } else {
        NLSTK_LOG_ERROR("[CCP] hangUp cbk is null");
        CcpCallControlErrorHandle(requestId, opCode, NLSTK_CCP_CCS_OPERATION_FAILED);
    }
}

void CcpCcsCallConfirmCallback(int32_t appId, uint16_t requestId,
                               NLSTK_SsapServerCallMethodRequestInfo_S *method, bool needReturn, bool needAuth)
{
    NLSTK_LOG_INFO("[CCP] enter CcpCcsCallConfirmCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_ccpCommonService.appId, "[CCP] appid error");
    if (method == NULL || method->param.len < CCP_CCS_METHOD_REQ_MIN_LEN || method->param.data == NULL) {
        NLSTK_LOG_ERROR("[CCP] method call value error");
        CcpCallControlErrorHandle(requestId, CCP_CCS_INVALID_OPCODE, NLSTK_CCP_CCS_OPERATION_FAILED);
        return;
    }
    // 当前方法调用参数只有操作码和通话标识
    size_t index = 0;
    uint8_t opCode = method->param.data[index++];
    uint8_t callId = method->param.data[index++];
    SLE_Addr_S copyAddr = {0};
    (void)memcpy_s(&copyAddr, sizeof(SLE_Addr_S), &method->addr, sizeof(SLE_Addr_S));
    NLSTK_LOG_INFO("[CCP] recv call control req, opcode = 0x%02x", opCode);
    if (method->handle != g_ccpCommonService.serviceCache.callControlHandle) {
        NLSTK_LOG_ERROR("[CCP] method handle error");
        CcpCallControlErrorHandle(requestId, opCode, NLSTK_CCP_CCS_OPERATION_FAILED);
        return;
    }
    if (!SDF_VectorFindFirst(g_ccpCommonService.callStatusVec, CcpCompcallId, &callId, &index)) {
        NLSTK_LOG_ERROR("[CCP] call id not found");
        CcpCallControlErrorHandle(requestId, opCode, NLSTK_CCP_CCS_CALL_ID_NOT_FOUND);
        return;
    }
    CcpCcsCallStatus_S *callStatus = SDF_VectorElementAt(g_ccpCommonService.callStatusVec, index);

    switch (opCode) {
        case CCP_ANSWER_OPCODE:
            CcpAnswer(&copyAddr, requestId, opCode, callStatus);
            break;
        case CCP_HANGUP_OPCODE:
            CcpHangUp(&copyAddr, requestId, opCode, callStatus);
            break;
        default:
            NLSTK_LOG_ERROR("[CCP] call control opcode not support");
            CcpCallControlErrorHandle(requestId, opCode, NLSTK_CCP_CCS_REQUEST_TYPE_NOT_SUPPORTED);
            break;
    }
}

static bool CcpCompPropertyHandle(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    CcpCcsPropertyCache_S *property = (CcpCcsPropertyCache_S *)ptr;
    uint16_t *handle = (uint16_t *)args;
    return property->handle == *handle;
}

void CcpCcsAuthorizeCallback(int32_t appId,  uint16_t requestId, NLSTK_SsapServerReadPropertyInfo_S *property)
{
    NLSTK_LOG_INFO("[CCP] enter CcpCcsAuthorizeCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_ccpCommonService.appId, "[CCP] appid error");
    if (property == NULL) {
        NLSTK_LOG_ERROR("[CCP] property is null");
        if (NLSTK_SsapServerAuthorizeResult(g_ccpCommonService.appId, requestId, false) != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[CCP] send reject authorize result fail");
        }
        return;
    }
    NLSTK_ServicePropertyOpType_E operation = NLSTK_SSAP_PROPERTY_READ;
    size_t index = 0;
    uint16_t handle = property->handle;
    if (handle > g_ccpCommonService.serviceCache.srvHandle && handle <= g_ccpCommonService.serviceCache.endHandle) {
        NLSTK_CHECK_RETURN_VOID(
            SDF_VectorFindFirst(g_ccpCommonService.serviceCache.properties, CcpCompPropertyHandle, &handle, &index),
            "[CCP] property handle not found");
        CcpCcsPropertyCache_S *propertyCache = SDF_VectorElementAt(g_ccpCommonService.serviceCache.properties, index);
        NLSTK_CcpCcsPropertyType_E type = propertyCache->type;
        if (g_ccpCommonService.authorize != NULL) {
            g_ccpCommonService.authorize(requestId, g_ccpCommonService.instanceId, type, operation);
        } else {
            NLSTK_LOG_ERROR("[CCP] authorize cbk is null");
            if (NLSTK_SsapServerAuthorizeResult(g_ccpCommonService.appId, requestId, false) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[CCP] send reject authorize result fail");
            }
        }
        return;
    }
    NLSTK_LOG_ERROR("[CCP] property handle not found");
    if (NLSTK_SsapServerAuthorizeResult(g_ccpCommonService.appId, requestId, false) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] send reject authorize result fail");
    }
}

static void CcpCcsUpdatePropertyCallBack(int32_t appId, NLSTK_SsapServerOnSetPropertyParam_S *property,
    NLSTK_Errcode_E ret)
{
    NLSTK_LOG_DEBUG("[CCP] enter CcpCcsUpdatePropertyCallBack");
    NLSTK_CHECK_RETURN_VOID(appId == g_ccpCommonService.appId, "[CCP] appid error");
    NLSTK_CHECK_RETURN_VOID(property != NULL, "[CCP] property is null");
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[CCP] update property fail");
    uint16_t uuid16Bits = CcpConvertUuidTo16Bits(property->uuid);
    if (uuid16Bits != CCP_CCS_CALL_STATUS_UUID) {
        return;
    }
    size_t index = 0;
    uint8_t callCount = property->value.data[index++];
    NLSTK_LOG_DEBUG("[CCP] call count is %u", callCount);
    if (property->value.len == callCount * CCP_CALL_STATUS_LEN + CCP_CALL_COUNT_LEN) {
        if (g_ccpCommonService.callStatusVec == NULL) {
            SDF_Traits traits = {.dtor = SDF_MemFree};
            g_ccpCommonService.callStatusVec = SDF_CreateVector(traits);
            NLSTK_CHECK_RETURN_VOID(g_ccpCommonService.callStatusVec != NULL, "[CCP] create callStatusVec failed");
        } else {
            SDF_CleanVector(g_ccpCommonService.callStatusVec);
        }
        for (uint8_t i = 0; i < callCount; i++) {
            CcpCcsCallStatus_S *callStatus = (CcpCcsCallStatus_S *)SDF_MemZalloc(sizeof(CcpCcsCallStatus_S));
            NLSTK_CHECK_RETURN_VOID(callStatus != NULL, "[CCP] call status malloc fail");
            callStatus->callId = property->value.data[index++];
            callStatus->networkId = property->value.data[index++];
            callStatus->callStatus = property->value.data[index++];
            callStatus->callFlag = property->value.data[index++];
            NLSTK_LOG_INFO("[CCP] callId = %u, callStatus = %u", callStatus->callId, callStatus->callStatus);
            if (!SDF_VectorEmplaceBack(g_ccpCommonService.callStatusVec, callStatus)) {
                NLSTK_LOG_ERROR("[CCP] emplace back failed");
                SDF_MemFree(callStatus);
                return;
            }
        }
    }
}

static void CcpCcsWriteDescriptorCallback(int32_t appId, NLSTK_SsapServerWriteDescriptorInfo_S *descriptor)
{
    NLSTK_LOG_DEBUG("[CCP] enter CcpCcsWriteDescriptorCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_ccpCommonService.appId, "[CCP] appid error");
    NLSTK_CHECK_RETURN_VOID(descriptor != NULL, "[CCP] descriptor is null");
}

static void CcpAddCcsStatement(NLSTK_ServiceParam_S *service, NLSTK_CcpCcsType_E type)
{
    NLSTK_SsapServiceStatementParam_S *statement = &service->serviceStatement;
    uint16_t uuid = type == NLSTK_CCP_CALL_CONTROL_SERVICE ?
        CCP_CALL_CONTROL_SERVICE_UUID : CCP_CALL_CONTROL_COMMON_SERVICE_UUID;
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(uuid);
    (void)memcpy_s(&statement->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    statement->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
}

static void CcpSetPermissionAndOperation(NLSTK_SsapServicePropertyParam_S *property, uint8_t right, bool readOnly)
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

static uint32_t CcpPropertySetDescriptor(NLSTK_SsapServicePropertyParam_S *property)
{
    // 目前只需要客户端属性配置描述符，后续可拓展
    property->descriptors = (NLSTK_SsapServiceDescriptorParam_S *)SDF_MemZalloc(
            sizeof(NLSTK_SsapServiceDescriptorParam_S));
    NLSTK_CHECK_RETURN(property->descriptors != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property descriptors malloc fail");
    property->descriptorNum = 1;
    // perimission暂置为0，无需授权、认证、加密
    property->descriptors[0].type = DESC_TYPE_CLIENT_CONFIG;
    property->descriptors[0].operation.operationValue = SSAP_OPERATE_INDICATION_READ |
        SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
    property->descriptors[0].val.data = (uint8_t *)SDF_MemZalloc(CCP_DESC_CLIENT_CONFIG_LEN);
    property->descriptors[0].val.len = CCP_DESC_CLIENT_CONFIG_LEN;
    NLSTK_CHECK_RETURN(property->descriptors[0].val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property descriptor value malloc fail");
    return NLSTK_ERRCODE_SUCCESS;
}

static uint32_t CcpAddCcsInstanceName(NLSTK_SsapServicePropertyParam_S *property, NLSTK_CcpCallControlInfo_S *baseInfo)
{
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_CCS_INSTANCE_NAME_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = baseInfo->propertyRights[NLSTK_CCP_CCS_INSTANCE_NAME];
    CcpSetPermissionAndOperation(property, right, true);
    if (baseInfo->instanceName.len == 0) {
        return CcpPropertySetDescriptor(property);
    }
    property->val.len = baseInfo->instanceName.len;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property value for instance name malloc fail");
    (void)memcpy_s(property->val.data, property->val.len, baseInfo->instanceName.data, baseInfo->instanceName.len);
    return CcpPropertySetDescriptor(property);
}

static uint32_t CcpAddCcsInstanceIcon(NLSTK_SsapServicePropertyParam_S *property, NLSTK_CcpCallControlInfo_S *baseInfo)
{
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_CCS_INSTANCE_ICON_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = baseInfo->propertyRights[NLSTK_CCP_CCS_INSTANCE_ICON];
    CcpSetPermissionAndOperation(property, right, true);
    property->val.len = baseInfo->instanceIcon.len + CCP_ICON_TYPE_LEN;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property value for instance icon malloc fail");
    property->val.data[0] = baseInfo->instanceIcon.type;
    if (baseInfo->instanceIcon.len != 0) {
        (void)memcpy_s(property->val.data + CCP_ICON_TYPE_LEN, baseInfo->instanceIcon.len,
            baseInfo->instanceIcon.icon, baseInfo->instanceIcon.len);
    }
    return CcpPropertySetDescriptor(property);
}

static uint32_t CcpAddCcsFeatureStatus(NLSTK_SsapServicePropertyParam_S *property, NLSTK_CcpCallControlInfo_S *baseInfo)
{
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_CCS_FEATURE_STATUS_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = baseInfo->propertyRights[NLSTK_CCP_CCS_FEATURE_STATUS];
    CcpSetPermissionAndOperation(property, right, false);
    property->val.len = sizeof(baseInfo->featureStatus);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property value for feature status malloc fail");
    *property->val.data = baseInfo->featureStatus;
    return CcpPropertySetDescriptor(property);
}

static uint32_t CcpAddCcsProtocolSupport(NLSTK_SsapServicePropertyParam_S *property,
    NLSTK_CcpCallControlInfo_S *baseInfo)
{
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_CCS_PROTOCOL_SUPPORT_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = baseInfo->propertyRights[NLSTK_CCP_CCS_PROTOCOL_SUPPORT];
    CcpSetPermissionAndOperation(property, right, false);
    if (baseInfo->protocolSupport.len == 0) {
        return CcpPropertySetDescriptor(property);
    }
    property->val.len = baseInfo->protocolSupport.len;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property value for protocol support malloc fail");
    (void)memcpy_s(property->val.data, property->val.len,
        baseInfo->protocolSupport.data, baseInfo->protocolSupport.len);
    return CcpPropertySetDescriptor(property);
}

static uint32_t CcpAddCcsCallInOutInfo(NLSTK_SsapServicePropertyParam_S *property, NLSTK_CcpCallControlInfo_S *baseInfo)
{
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_CCS_CALLIN_OUT_INFO_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = baseInfo->propertyRights[NLSTK_CCP_CCS_CALLIN_OUT_INFO];
    CcpSetPermissionAndOperation(property, right, false);
    property->val.len =
        CCP_CALLIN_OUT_LEN + baseInfo->callInOutInfo.userInfo.len + baseInfo->callInOutInfo.userAlias.len;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property value for callin/out info malloc fail");
    size_t index = 0;
    property->val.data[index++] = baseInfo->callInOutInfo.callId;
    property->val.data[index++] = baseInfo->callInOutInfo.networkId;
    property->val.data[index++] = baseInfo->callInOutInfo.callFlag;
    if (baseInfo->callInOutInfo.userInfo.len != 0) {
        (void)memcpy_s(property->val.data + CCP_CALLIN_OUT_LEN, baseInfo->callInOutInfo.userInfo.len,
            baseInfo->callInOutInfo.userInfo.data, baseInfo->callInOutInfo.userInfo.len);
    }
    if (baseInfo->callInOutInfo.userAlias.len != 0) {
        (void)memcpy_s(property->val.data + CCP_CALLIN_OUT_LEN + baseInfo->callInOutInfo.userInfo.len,
            baseInfo->callInOutInfo.userAlias.len, baseInfo->callInOutInfo.userAlias.data,
            baseInfo->callInOutInfo.userAlias.len);
    }
    return CcpPropertySetDescriptor(property);
}

static uint32_t CcpAddCcsCallStatus(NLSTK_SsapServicePropertyParam_S *property, NLSTK_CcpCallControlInfo_S *baseInfo)
{
    // 这里不需要对内部参数判空，在post_task前结构体拷贝时已经做了校验
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_CCS_CALL_STATUS_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S),
        &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = baseInfo->propertyRights[NLSTK_CCP_CCS_CALL_STATUS];
    CcpSetPermissionAndOperation(property, right, false);
    property->val.len = baseInfo->callStatus.callCount * CCP_CALL_STATUS_LEN + CCP_CALL_COUNT_LEN;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property value for call status malloc fail");
    property->val.data[0] = baseInfo->callStatus.callCount;
    for (int i = 0; i < baseInfo->callStatus.callCount; i++) {
        size_t offset = CCP_CALL_COUNT_LEN;
        property->val.data[i * CCP_CALL_STATUS_LEN + offset++] = baseInfo->callStatus.callId[i];
        property->val.data[i * CCP_CALL_STATUS_LEN + offset++] = baseInfo->callStatus.networkId[i];
        property->val.data[i * CCP_CALL_STATUS_LEN + offset++] = baseInfo->callStatus.callStatus[i];
        property->val.data[i * CCP_CALL_STATUS_LEN + offset++] = baseInfo->callStatus.callFlag[i];
    }
    return CcpPropertySetDescriptor(property);
}

static uint32_t CcpAddCcsCallTermination(NLSTK_SsapServicePropertyParam_S *property,
    NLSTK_CcpCallControlInfo_S *baseInfo)
{
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_CCS_CALL_TERMINATION_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = baseInfo->propertyRights[NLSTK_CCP_CCS_CALL_TERMINATION];
    CcpSetPermissionAndOperation(property, right, false);
    property->val.len = sizeof(baseInfo->callTermination);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property value for call termination malloc fail");
    (void)memcpy_s(property->val.data, property->val.len, &baseInfo->callTermination, property->val.len);
    return CcpPropertySetDescriptor(property);
}

static uint32_t CcpAddCcsMediaInstanceId(NLSTK_SsapServicePropertyParam_S *property,
    NLSTK_CcpCallControlInfo_S *baseInfo)
{
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_CCS_MEDIA_INSTANCE_ID_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = baseInfo->propertyRights[NLSTK_CCP_CCS_MEDIA_INSTANCE_ID];
    CcpSetPermissionAndOperation(property, right, true);
    property->val.len = sizeof(baseInfo->mediaInstanceId);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property value for media instanceId malloc fail");
    *property->val.data = baseInfo->mediaInstanceId;
    return CcpPropertySetDescriptor(property);
}

static uint32_t CcpAddCcsNetworkSelection(NLSTK_SsapServicePropertyParam_S *property,
    NLSTK_CcpCallControlInfo_S *baseInfo)
{
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_CCS_NETWORK_SELECTION_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = baseInfo->propertyRights[NLSTK_CCP_CCS_NETWORK_SELECTION];
    CcpSetPermissionAndOperation(property, right, false);
    if (baseInfo->networkSelection.len == 0) {
        return CcpPropertySetDescriptor(property);
    }
    property->val.len = baseInfo->networkSelection.len;
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property value for network selection malloc fail");
    (void)memcpy_s(property->val.data, property->val.len,
        baseInfo->networkSelection.data, baseInfo->networkSelection.len);
    return CcpPropertySetDescriptor(property);
}

static uint32_t CcpAddCcsCallReqSupport(NLSTK_SsapServicePropertyParam_S *property,
    NLSTK_CcpCallControlInfo_S *baseInfo)
{
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_CCS_CALL_REQ_SUPPORT_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    uint8_t right = baseInfo->propertyRights[NLSTK_CCP_CCS_CALL_REQ_SUPPORT];
    CcpSetPermissionAndOperation(property, right, true);
    property->val.len = sizeof(baseInfo->callRequestSupport);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] property value for callReq support malloc fail");
    (void)memcpy_s(property->val.data, property->val.len, &baseInfo->callRequestSupport, property->val.len);
    return CcpPropertySetDescriptor(property);
}

uint32_t CcpAddCcsProperty(NLSTK_ServiceParam_S *service, NLSTK_CcpCallControlInfo_S *baseInfo)
{
    // 若申请失败，统一由外部析构
    uint16_t num = CCP_CCS_REQUIRED_PROPERTY_NUM;
    if (baseInfo->instanceIconFlag) {
        num++;
    }
    if (baseInfo->networkSelectionFlag) {
        num++;
    }
    service->property = (NLSTK_SsapServicePropertyParam_S *)SDF_MemZalloc(
        sizeof(NLSTK_SsapServicePropertyParam_S) * num);
    NLSTK_CHECK_RETURN(service->property != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] property malloc fail");
    service->servicePropertyNum = num;
    NLSTK_SsapServicePropertyParam_S *property = service->property;
    size_t index = 0;
    uint32_t ret = NLSTK_ERRCODE_SUCCESS;
    ret = CcpAddCcsInstanceName(&property[index++], baseInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] add ccs instance name fail");
    ret = CcpAddCcsFeatureStatus(&property[index++], baseInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] add ccs feature status fail");
    ret = CcpAddCcsProtocolSupport(&property[index++], baseInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] add ccs protocol support fail");
    ret = CcpAddCcsCallInOutInfo(&property[index++], baseInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] add ccs callInOut info fail");
    ret = CcpAddCcsCallStatus(&property[index++], baseInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] add ccs call status fail");
    ret = CcpAddCcsCallTermination(&property[index++], baseInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] add ccs call termination fail");
    ret = CcpAddCcsMediaInstanceId(&property[index++], baseInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] add ccs media instanceId fail");
    ret = CcpAddCcsCallReqSupport(&property[index++], baseInfo);
    NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] add ccs callReq support fail");
    if (baseInfo->instanceIconFlag) {
        ret = CcpAddCcsInstanceIcon(&property[index++], baseInfo);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] add ccs instance icon fail");
    }
    if (baseInfo->networkSelectionFlag) {
        ret = CcpAddCcsNetworkSelection(&property[index++], baseInfo);
        NLSTK_CHECK_RETURN(ret == NLSTK_ERRCODE_SUCCESS, ret, "[CCP] add ccs network selection fail");
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static uint32_t CcpAddCcsMethod(NLSTK_ServiceParam_S *service)
{
    service->method = (NLSTK_SsapServiceMethodParam_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServiceMethodParam_S));
    NLSTK_CHECK_RETURN(service->method != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] method malloc fail");
    service->serviceMethodNum = CCP_CCS_METHOD_NUM;
    service->method->type = ITEM_TYPE_STD_METHOD;
    service->method->permission.permissionValue =
         SSAP_PERMISSION_AUTHENTICATION_NEED | SSAP_PERMISSION_ENCRYPTION_NEED;
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_CCS_CALL_CONTROL_POINT_UUID);
    (void)memcpy_s(&service->method->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    return NLSTK_ERRCODE_SUCCESS;
}

static uint32_t CcpAddCallControlService(CcpCallControlService_S *serviceInfo)
{
    NLSTK_ServiceParam_S *service = (NLSTK_ServiceParam_S *)SDF_MemZalloc(sizeof(NLSTK_ServiceParam_S));
    NLSTK_CHECK_RETURN(service != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] service malloc fail");
    CcpAddCcsStatement(service, serviceInfo->baseInfo->type);
    if (CcpAddCcsProperty(service, serviceInfo->baseInfo) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] add service property fail");
        NLSTK_SsapFreeServiceParam(service);
        return NLSTK_ERRCODE_FAIL;
    }
    if (CcpAddCcsMethod(service) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] add service method fail");
        NLSTK_SsapFreeServiceParam(service);
        return NLSTK_ERRCODE_FAIL;
    }
    if (NLSTK_SsapServerAddService(serviceInfo->appId, service) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] add service fail");
        NLSTK_SsapFreeServiceParam(service);
        return NLSTK_ERRCODE_FAIL;
    }
    NLSTK_SsapFreeServiceParam(service);
    return NLSTK_ERRCODE_SUCCESS;
}

void CcpCreateCcsInstance(void *arg)
{
    NLSTK_LOG_INFO("[CCP] enter CcpCreateCcsInstance");
    NLSTK_CcpCallControlInfo_S *param = (NLSTK_CcpCallControlInfo_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[CCP] param is null");
    NLSTK_CcpCallControlInfo_S *baseInfo = (NLSTK_CcpCallControlInfo_S *)SDF_MemZalloc(
        sizeof(NLSTK_CcpCallControlInfo_S));
    NLSTK_CHECK_RETURN_VOID(baseInfo != NULL, "[CCP] baseInfo malloc fail");
    if (CcpCopyCallControlInfo(baseInfo, param) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] copy call control info failed");
        CcpFreeCallControlInfo(baseInfo);
        return;
    }
    if (baseInfo->type == NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE && g_ccpCommonService.used == true) {
        NLSTK_LOG_ERROR("[CCP] has added common call control service");
        if (baseInfo->startCcsInst != NULL) {
            baseInfo->startCcsInst(baseInfo->mediaInstanceId, NLSTK_ERRCODE_FAIL);
        }
        CcpFreeCallControlInfo(baseInfo);
        return;
    } else if (baseInfo->type == NLSTK_CCP_CALL_CONTROL_SERVICE) {
        NLSTK_LOG_ERROR("[CCP] call control service unsupport");
        if (baseInfo->startCcsInst != NULL) {
            baseInfo->startCcsInst(baseInfo->mediaInstanceId, NLSTK_ERRCODE_FAIL);
        }
        CcpFreeCallControlInfo(baseInfo);
        return;
    }
    NLSTK_SsapAppServerCb_S cb = {0};
    cb.onRegisterApp = CcpCcsRegisterAppCallback;
    cb.onAddService = CcpCcsStartServiceCallback;
    cb.onSetPropertyValue = CcpCcsUpdatePropertyCallBack;
    cb.onCallMethod = CcpCcsCallConfirmCallback;
    cb.onReadPropertyAuthorizeRequest = CcpCcsAuthorizeCallback;
    cb.onWriteDescriptor = CcpCcsWriteDescriptorCallback;
    if (NLSTK_SsapServerRegAppAsyn(&cb) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] reg app fail");
        if (baseInfo->startCcsInst != NULL) {
            baseInfo->startCcsInst(baseInfo->mediaInstanceId, NLSTK_ERRCODE_FAIL);
        }
        CcpFreeCallControlInfo(baseInfo);
        return;
    }
    g_ccpCommonService.used = true;
    g_ccpCommonService.instanceId = baseInfo->mediaInstanceId;
    g_ccpCommonService.callControl = baseInfo->callControlPoint;
    g_ccpCommonService.startCcsInst = baseInfo->startCcsInst;
    g_ccpCommonService.authorize = baseInfo->authorize;
    g_ccpCommonService.baseInfo = baseInfo;
}

void CcpCallControlResult(void *arg)
{
    NLSTK_LOG_INFO("[CCP] enter CcpCallControlResult");
    CcpCallControlResultParam_S *param = (CcpCallControlResultParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[CCP] conrol result param is null");
    NLSTK_CHECK_RETURN_VOID(param->instanceId == g_ccpCommonService.instanceId, "[CCP] instance id invalid");
    size_t index = 0;
    CcpCcsMethodCallRequest_S *request = CcpPopMethodCallVector(param->requestId, &index);
    NLSTK_CHECK_RETURN_VOID(request != NULL, "[CCP] call request is null");
    uint8_t opCode = request->opCode;
    uint8_t callId = request->callId;
    SDF_VectorRemove(g_ccpCommonService.callReqQue, index);
    NLSTK_VariableData_S value = {0};
    index = 0;
    if (param->errorCode == NLSTK_CCP_CCS_OPERATION_SUCCESS) {
        value.len = CCP_CCS_METHOD_RES_LEN;
        value.data = (uint8_t *)SDF_MemZalloc(value.len);
        NLSTK_CHECK_RETURN_VOID(value.data != NULL, "[CCP] value data malloc fail");
        value.data[index++] = opCode;
        value.data[index++] = param->errorCode;
        value.data[index++] = callId;
        if (NLSTK_SsapServerSendMethodCallRes(g_ccpCommonService.appId, param->requestId, &value) !=
            NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[CCP] send method call result fail");
        }
    } else {
        value.len = CCP_CCS_METHOD_RES_ERROR_LEN;
        value.data = (uint8_t *)SDF_MemZalloc(value.len);
        NLSTK_CHECK_RETURN_VOID(value.data != NULL, "[CCP] value data malloc fail");
        value.data[index++] = opCode;
        value.data[index++] = param->errorCode;
        if (NLSTK_SsapServerSendMethodCallRes(g_ccpCommonService.appId, param->requestId, &value) !=
            NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[CCP] send method call result fail");
        }
    }
    SDF_MemFree(value.data);
}

void CcpCcsAuthorizeResult(void *arg)
{
    NLSTK_LOG_INFO("[CCP] enter CcpCcsAuthorizeResult");
    CcpCcsAuthorizeResultParam_S *param = (CcpCcsAuthorizeResultParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[CCP] param is null");
    uint16_t requestId = param->requestId;
    bool allow = param->errorCode == NLSTK_ERRCODE_SUCCESS ? true : false;
    if (NLSTK_SsapServerAuthorizeResult(g_ccpCommonService.appId, requestId, allow) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] authorize result fail");
    }
}

static bool CcpCcsCompPropertyType(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    CcpCcsPropertyCache_S *property = (CcpCcsPropertyCache_S *)ptr;
    uint16_t *type = (uint16_t *)args;
    return property->type == *type;
}

void CcpUpdateCcsProperty(void *arg)
{
    NLSTK_LOG_DEBUG("[CCP] enter CcpUpdateCcsProperty");
    CcpUpdateCcsPropertyParam_S *param = (CcpUpdateCcsPropertyParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[CCP] param is null");
    NLSTK_CHECK_RETURN_VOID(param->value != NULL, "[CCP] param value is null");
    NLSTK_CHECK_RETURN_VOID(param->instanceId == g_ccpCommonService.instanceId, "[CCP] instance id invalid");
    if (param->value->len != 0) {
        NLSTK_CHECK_RETURN_VOID(param->value->data != NULL, "[CCP] param value data is null");
    }
    CcpCallControlServiceCache_S *serviceCache = &g_ccpCommonService.serviceCache;
    size_t index = 0;
    NLSTK_CHECK_RETURN_VOID(
        SDF_VectorFindFirst(serviceCache->properties, CcpCcsCompPropertyType, &param->type, &index),
        "[CCP] property type not found");
    CcpCcsPropertyCache_S *propertyCache = SDF_VectorElementAt(serviceCache->properties, index);
    NLSTK_CHECK_RETURN_VOID(propertyCache != NULL, "[CCP] property cache is null");
    uint16_t handle = propertyCache->handle;
    NLSTK_VariableData_S value = {0};
    value.len = param->value->len;
    if (value.len != 0) {
        value.data = (uint8_t *)SDF_MemZalloc(value.len);
        NLSTK_CHECK_RETURN_VOID(value.data != NULL, "[CCP] value data malloc fail");
        (void)memcpy_s(value.data, value.len, param->value->data, param->value->len);
    }
    if (NLSTK_SsapServerUpdatePropertyValue(g_ccpCommonService.appId, handle, &value) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] update property fail");
    }
    SDF_MemFree(value.data);
}

void CcpDeleteCcsInstance(void *arg)
{
    NLSTK_LOG_INFO("[CCP] enter CcpDeleteCcsInstance");
    int32_t *instanceId = (int32_t *)arg;
    NLSTK_CHECK_RETURN_VOID(instanceId != NULL, "[CCP] instanceId is null");
    NLSTK_CHECK_RETURN_VOID(*instanceId == g_ccpCommonService.instanceId, "[CCP] invalid instanceId");
    CcpResetCallControlService(&g_ccpCommonService);
}