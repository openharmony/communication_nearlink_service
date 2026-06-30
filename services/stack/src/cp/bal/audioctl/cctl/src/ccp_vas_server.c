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
#include "securec.h"
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "nlstk_ssap_app_server.h"
#include "nlstk_ccp.h"
#include "ccp_utils.h"
#include "nlstk_ccp_vas_server.h"
#include "ccp_vas_server.h"

static CcpVasService_S g_vasService = { .appId = CCP_INVALID_APP_ID };

static uint32_t CcpAddVoiceAssistantService(CcpVasService_S *serviceInfo);

static void CcpResetVoiceAssistantService(CcpVasService_S *serviceInfo)
{
    if (serviceInfo->appId >= 0) {
        NLSTK_SsapServerDeregisterApplicationAsync(serviceInfo->appId);
    }
    (void)memset_s(serviceInfo, sizeof(CcpVasService_S), 0, sizeof(CcpVasService_S));
    serviceInfo->appId = CCP_INVALID_APP_ID;
}

void CcpVasEnable(void)
{
    NLSTK_LOG_INFO("[CCP] enter CcpVasEnable");
    CcpResetVoiceAssistantService(&g_vasService);
}

void CcpVasDisable(void)
{
    NLSTK_LOG_INFO("[CCP] enter CcpVasDisable");
    CcpResetVoiceAssistantService(&g_vasService);
}

void CcpVasRegisterAppCallback(int32_t appId, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[CCP] enter CcpVasRegisterAppCallback");
    if (appId < 0 || ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] reg app invalid");
        if (g_vasService.startServiceCbk != NULL) {
            g_vasService.startServiceCbk(NLSTK_ERRCODE_FAIL);
        }
        CcpResetVoiceAssistantService(&g_vasService);
        return;
    }
    g_vasService.appId = appId;
    NLSTK_LOG_INFO("[CCP] vas reg ssap appId = %d", appId);
    if (CcpAddVoiceAssistantService(&g_vasService) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] add voice assistant service fail");
        if (g_vasService.startServiceCbk != NULL) {
            g_vasService.startServiceCbk(NLSTK_ERRCODE_FAIL);
        }
        CcpResetVoiceAssistantService(&g_vasService);
    }
}

static uint32_t CcpBuildVasCache(CcpVasService_S *service, SSAP_Service_S *result)
{
    SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(result->properties, 0);
    NLSTK_CHECK_RETURN(property != NULL && property->val != NULL,
        NLSTK_ERRCODE_POINTER_NULL, "[CCP] property is null");
    NLSTK_CHECK_RETURN(CcpConvertUuidTo16Bits(property->uuid) == CCP_VOICE_ASSISTANT_STATE_UUID,
        NLSTK_ERRCODE_PARAM_ERR, "[CCP] property uuid error");
    NLSTK_CHECK_RETURN(property->val->len == sizeof(uint8_t),
        NLSTK_ERRCODE_PARAM_ERR, "[CCP] property value len error");
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(result->methods, 0);
    NLSTK_CHECK_RETURN(method != NULL, NLSTK_ERRCODE_POINTER_NULL, "[CCP] method is null");
    NLSTK_CHECK_RETURN(CcpConvertUuidTo16Bits(method->uuid) == CCP_VOICE_ASSISTANT_CONTROL_UUID,
        NLSTK_ERRCODE_PARAM_ERR, "[CCP] method uuid error");
    service->srvHandle = result->handle;
    service->stateHandle = property->handle;
    service->state = *property->val->value;
    service->controlHandle = method->handle;
    return NLSTK_ERRCODE_SUCCESS;
}

static void CcpVasStartServiceCallback(int32_t appId, SSAP_Service_S *result, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[CCP] enter CcpVasStartServiceCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_vasService.appId, "[CCP] appid error");
    if (result == NULL || ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] add result fail");
        if (g_vasService.startServiceCbk != NULL) {
            g_vasService.startServiceCbk(NLSTK_ERRCODE_FAIL);
        }
        CcpResetVoiceAssistantService(&g_vasService);
        return;
    }
    if (CcpBuildVasCache(&g_vasService, result) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] vas cache build error");
        if (g_vasService.startServiceCbk != NULL) {
            g_vasService.startServiceCbk(NLSTK_ERRCODE_FAIL);
        }
        CcpResetVoiceAssistantService(&g_vasService);
        return;
    }
    if (g_vasService.startServiceCbk != NULL) {
        g_vasService.startServiceCbk(NLSTK_ERRCODE_SUCCESS);
    }
}

void CcpVasControlResult(void *arg)
{
    NLSTK_LOG_INFO("[CCP] enter CcpVasControlResult");
    CcpVasControlResultParam_S *param = (CcpVasControlResultParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[CCP] param is null");
    NLSTK_VariableData_S value = {0};
    value.len = CCP_VAS_METHOD_RES_LEN;
    value.data = (uint8_t *)SDF_MemZalloc(value.len);
    NLSTK_CHECK_RETURN_VOID(value.data != NULL, "[CCP] value data malloc fail");
    size_t index = 0;
    value.data[index++] = param->opCode;
    value.data[index++] = param->errorCode;
    if (NLSTK_SsapServerSendMethodCallRes(g_vasService.appId, param->requestId, &value) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] send method call result fail");
    }
    SDF_MemFree(value.data);
}

static void CcpVasCallConfirmCallback(int32_t appId, uint16_t requestId,
    NLSTK_SsapServerCallMethodRequestInfo_S *method, bool needReturn, bool needAuth)
{
    NLSTK_LOG_INFO("[CCP] enter CcpVasCallConfirmCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_vasService.appId, "[CCP] appid error");
    NLSTK_CHECK_RETURN_VOID(method != NULL && method->param.len == CCP_VAS_METHOD_REQ_LEN &&
        method->param.data != NULL, "[CCP] method call value error");
    NLSTK_CHECK_RETURN_VOID(method->handle == g_vasService.controlHandle, "[CCP] method handle error");
    uint8_t opCode = *method->param.data;
    switch (opCode) {
        case NLSTK_CCP_VAS_ACTIVATE: {
            if (g_vasService.state == NLSTK_VAS_STATE_IDLE && g_vasService.controlPoint.activate != NULL) {
                g_vasService.controlPoint.activate(&method->addr, requestId);
            } else {
                CcpVasControlResultParam_S param = { requestId, opCode, NLSTK_VAS_CONTROL_FAIL };
                CcpVasControlResult(&param);
            }
            break;
        }
        case NLSTK_CCP_VAS_TERMINATE: {
            if (g_vasService.state == NLSTK_VAS_STATE_ACTIVATED && g_vasService.controlPoint.terminate != NULL) {
                g_vasService.controlPoint.terminate(&method->addr, requestId);
            } else {
                CcpVasControlResultParam_S param = { requestId, opCode, NLSTK_VAS_CONTROL_FAIL };
                CcpVasControlResult(&param);
            }
            break;
        }
        default: {
            NLSTK_LOG_ERROR("[CCP] method call opCode error");
            CcpVasControlResultParam_S param = { requestId, opCode, NLSTK_VAS_CONTROL_FAIL };
            CcpVasControlResult(&param);
            break;
        }
    }
}

static void CcpVasAuthorizeCallback(int32_t appId, uint16_t requestId, NLSTK_SsapServerReadPropertyInfo_S *property)
{
    NLSTK_LOG_INFO("[CCP] enter CcpVasAuthorizeCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_vasService.appId, "[CCP] appid error");
    if (g_vasService.authorizeCbk != NULL) {
        g_vasService.authorizeCbk(&property->addr, requestId);
    } else {
        // 上层没有注册语音助手授权回调的情况下，直接认为授权失败
        NLSTK_SsapServerAuthorizeResult(g_vasService.appId, requestId, false);
    }
}

static void CcpVasUpdateStateCallBack(int32_t appId, NLSTK_SsapServerOnSetPropertyParam_S *property,
    NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[CCP] enter CcpVasUpdateStateCallBack");
    NLSTK_CHECK_RETURN_VOID(appId == g_vasService.appId && property != NULL && ret == NLSTK_ERRCODE_SUCCESS,
        "[CCP] param error or is null");
    NLSTK_CHECK_RETURN_VOID(property->handle == g_vasService.stateHandle && property->value.data != NULL,
        "[CCP] state handle error or value is null");
    g_vasService.state = *property->value.data;
    NLSTK_LOG_INFO("[CCP] vas state = %u", g_vasService.state);
}

static void CcpVasWriteDescriptorCallback(int32_t appId, NLSTK_SsapServerWriteDescriptorInfo_S *descriptor)
{
    NLSTK_LOG_INFO("[CCP] enter CcpVasWriteDescriptorCallback");
    NLSTK_CHECK_RETURN_VOID(appId == g_vasService.appId, "[CCP] appid error");
    NLSTK_CHECK_RETURN_VOID(descriptor != NULL, "[CCP] descriptor is null");
}

static uint32_t CcpVasSetStateProperty(NLSTK_SsapServicePropertyParam_S *property, CcpVasService_S *info)
{
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru(CCP_VOICE_ASSISTANT_STATE_UUID);
    (void)memcpy_s(&property->uuid, sizeof(NLSTK_SsapUuid_S), &uuidStru, sizeof(NLSTK_SsapUuid_S));
    property->val.len = sizeof(uint8_t);
    property->val.data = (uint8_t *)SDF_MemZalloc(property->val.len);
    NLSTK_CHECK_RETURN(property->val.data != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
        "[CCP] vas state property value malloc fail");
    *property->val.data = info->state;

    // 根据right添加权限
    property->permission.permissionValue = info->stateRight;

    // 添加操作指示，应支持读、通知、指示、写入客户端属性配置描述符
    uint32_t operation = SSAP_OPERATE_INDICATION_READ + SSAP_OPERATE_INDICATION_NOTIFY +
        SSAP_OPERATE_INDICATION_INDICATE + SSAP_OPERATE_INDICATION_DESCRIPTOR_CLIENT_CONFIGURATION_WRITE;
    property->operation.operationValue = operation;

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

uint32_t CcpVasBuildServiceParam(NLSTK_ServiceParam_S *service, CcpVasService_S *info)
{
    // 若失败，统一由调用处析构内存
    NLSTK_SsapServiceStatementParam_S *statement = &service->serviceStatement;
    NLSTK_SsapUuid_S srvcUuidStru = CcpConvertUuidToStru(CCP_VOICE_ASSISTANT_SERVICE_UUID);
    (void)memcpy_s(&statement->uuid, sizeof(NLSTK_SsapUuid_S), &srvcUuidStru, sizeof(NLSTK_SsapUuid_S));
    statement->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    service->servicePropertyNum = CCP_VAS_PROPERTY_NUM;
    service->property = (NLSTK_SsapServicePropertyParam_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServicePropertyParam_S));
    NLSTK_CHECK_RETURN(service->property != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] vas state property malloc fail");
    NLSTK_CHECK_RETURN(CcpVasSetStateProperty(service->property, info) == NLSTK_ERRCODE_SUCCESS,
        NLSTK_ERRCODE_FAIL, "[CCP] vas state property set error");
    service->serviceMethodNum = CCP_VAS_METHOD_NUM;
    service->method = (NLSTK_SsapServiceMethodParam_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServiceMethodParam_S));
    NLSTK_CHECK_RETURN(service->method != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] vas control method malloc fail");
    service->method->type = ITEM_TYPE_STD_METHOD;
    service->method->permission.permissionValue =
        SSAP_PERMISSION_AUTHENTICATION_NEED | SSAP_PERMISSION_ENCRYPTION_NEED;
    NLSTK_SsapUuid_S crtlUuidStru = CcpConvertUuidToStru(CCP_VOICE_ASSISTANT_CONTROL_UUID);
    (void)memcpy_s(&service->method->uuid, sizeof(NLSTK_SsapUuid_S), &crtlUuidStru, sizeof(NLSTK_SsapUuid_S));
    return NLSTK_ERRCODE_SUCCESS;
}

static uint32_t CcpAddVoiceAssistantService(CcpVasService_S *serviceInfo)
{
    NLSTK_ServiceParam_S *service = (NLSTK_ServiceParam_S *)SDF_MemZalloc(sizeof(NLSTK_ServiceParam_S));
    NLSTK_CHECK_RETURN(service != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CCP] service malloc fail");
    if (CcpVasBuildServiceParam(service, serviceInfo) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] vas build service fail");
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

void CcpCreateVoiceAssistantService(void *arg)
{
    NLSTK_LOG_INFO("[CCP] enter CcpCreateVoiceAssistantService");
    NLSTK_CcpVasInfo_S *info = (NLSTK_CcpVasInfo_S *)arg;
    NLSTK_CHECK_RETURN_VOID(info != NULL, "[CCP] info is null");
    if (g_vasService.used == true) {
        NLSTK_LOG_ERROR("[CCP] has added voice assistant service");
        if (info->startService != NULL) {
            info->startService(NLSTK_ERRCODE_FAIL);
        }
        return;
    }
    NLSTK_SsapAppServerCb_S cb = {0};
    cb.onRegisterApp = CcpVasRegisterAppCallback;
    cb.onAddService = CcpVasStartServiceCallback;
    cb.onSetPropertyValue = CcpVasUpdateStateCallBack;
    cb.onCallMethod = CcpVasCallConfirmCallback;
    cb.onReadPropertyAuthorizeRequest = CcpVasAuthorizeCallback;
    cb.onWriteDescriptor = CcpVasWriteDescriptorCallback;
    if (NLSTK_SsapServerRegAppAsyn(&cb) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] reg app fail");
        if (info->startService != NULL) {
            info->startService(NLSTK_ERRCODE_FAIL);
        }
        return;
    }
    g_vasService.used = true;
    g_vasService.state = info->state;
    g_vasService.stateRight = info->stateRight;
    g_vasService.startServiceCbk = info->startService;
    g_vasService.authorizeCbk = info->authorize;
    g_vasService.controlPoint = info->vasControlPoint;
}

void CcpVasStateAuthorizeResult(void *arg)
{
    NLSTK_LOG_INFO("[CCP] enter CcpVasStateAuthorizeResult");
    CcpVasAuthorizeResultParam_S *param = (CcpVasAuthorizeResultParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[CCP] param is null");
    uint16_t requestId = param->requestId;
    bool allow = param->errorCode == NLSTK_ERRCODE_SUCCESS ? true : false;
    if (NLSTK_SsapServerAuthorizeResult(g_vasService.appId, requestId, allow) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] authorize result fail");
    }
}

void CcpUpdateVasState(void *arg)
{
    NLSTK_LOG_INFO("[CCP] enter CcpUpdateVasState");
    uint8_t *param = (uint8_t *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[CCP] param is null");
    NLSTK_VariableData_S value = {0};
    value.len = sizeof(uint8_t);
    value.data = (uint8_t *)SDF_MemZalloc(value.len);
    NLSTK_CHECK_RETURN_VOID(value.data != NULL, "[CCP] value data malloc fail");
    *value.data = *param;
    if (NLSTK_SsapServerUpdatePropertyValue(g_vasService.appId, g_vasService.stateHandle, &value) !=
        NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[CCP] update property fail");
    }
    SDF_MemFree(value.data);
}

void CcpDeleteVoiceAssistantService(void *arg)
{
    NLSTK_LOG_INFO("[CCP] enter CcpDeleteVoiceAssistantService");
    CcpResetVoiceAssistantService(&g_vasService);
}