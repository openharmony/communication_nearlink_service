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
#include "nlstk_ssap_app_server.h"
#include "nlstk_schedule.h"
#include "ssap_common.h"
#include "ssaps_service_param.h"
#include "ssaps_server_app.h"
#include "ssaps_service.h"
#include "ssaps_server.h"
#include "ssap_pkt.h"
#include "nlstk_log.h"
#include "sdf_vector.h"
#include "securec.h"

NLSTK_Errcode_E NLSTK_SsapServerRegApp(NLSTK_SsapAppServerCb_S *cb, int32_t *appId)
{
    NLSTK_CHECK_RETURN(cb != NULL && appId != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "NLSTK_SsapServerRegApp input param is NULL");
    *appId = -1;  // 初始化appId为-1

    SsapRegServerAppParam_S *param = (SsapRegServerAppParam_S *)SDF_MemZalloc(sizeof(SsapRegServerAppParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DEVD] param is null");
    param->appId = -1;
    if (memcpy_s(&(param->cb), sizeof(NLSTK_SsapAppServerCb_S), cb, sizeof(NLSTK_SsapAppServerCb_S)) != EOK) {
        return NLSTK_ERRCODE_MEMCPY_FAIL;
    }

    // 同步接口的内存，由调用点释放
    uint32_t ret = SchedulePostTaskBlocked(SsapServerRegApp, (void *)param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        // 在异常场景下，无法再处理，因此不判断返回值，此时param的内存会由SDF_MemFree释放
        (void)SchedulePostTask(SsapServerDeregisterApplication, (void *)param, SDF_MemFree);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK || param->appId == -1) {
        SDF_MemFree(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *appId = param->appId;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerRegAppAsyn(NLSTK_SsapAppServerCb_S *cb)
{
    NLSTK_CHECK_RETURN(cb != NULL, NLSTK_ERRCODE_POINTER_NULL, "NLSTK_SsapServerRegApp input param is NULL");

    SsapRegServerAppParam_S *param = (SsapRegServerAppParam_S *)SDF_MemZalloc(sizeof(SsapRegServerAppParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[DEVD] param is null");
    param->appId = -1;
    if (memcpy_s(&(param->cb), sizeof(NLSTK_SsapAppServerCb_S), cb, sizeof(NLSTK_SsapAppServerCb_S)) != EOK) {
        return NLSTK_ERRCODE_MEMCPY_FAIL;
    }
    NLSTK_LOG_INFO("[NLSTK_SSAP] NLSTK_SsapServerRegAppAsyn enter");
    // 同步接口的内存，由调用点释放
    uint32_t ret = SchedulePostTask(SsapServerRegAppAsyn, (void *)param, SDF_MemFree);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[NLSTK_SSAPS] post task fail in NLSTK_SsapServerRegAppAsyn");
    return NLSTK_ERRCODE_SUCCESS;
}

void NLSTK_SsapServerDeregisterApplicationAsync(int32_t appId)
{
    NLSTK_CHECK_RETURN_VOID(appId >= 0 && appId < NLSTK_SSAP_SERVER_APP_MAX_NUM, "appId(%d) is invalid", appId);
    SsapRegServerAppParam_S *param = (SsapRegServerAppParam_S *)SDF_MemZalloc(sizeof(SsapRegServerAppParam_S));
    NLSTK_CHECK_RETURN_VOID(param != NULL, "memory alloc error");
    param->appId = appId;
    uint32_t ret = SchedulePostTask(SsapServerDeregisterApplication, (void *)param, SDF_MemFree);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_OK,
        "[NLSTK_SSAPS] post task fail in NLSTK_SsapServerDeregisterApplicationAsync");
    return;
}

void NLSTK_SsapServerDeregisterApplication(int32_t appId)
{
    NLSTK_CHECK_RETURN_VOID(appId >= 0 && appId < NLSTK_SSAP_SERVER_APP_MAX_NUM, "appId(%d) is invalid", appId);
    SsapRegServerAppParam_S *param = (SsapRegServerAppParam_S *)SDF_MemZalloc(sizeof(SsapRegServerAppParam_S));
    NLSTK_CHECK_RETURN_VOID(param != NULL, "memory alloc error");
    param->appId = appId;
    // 同步接口的内存，由调用点释放
    uint32_t ret =
        SchedulePostTaskBlocked(SsapServerDeregisterApplication, (void *)param, SDF_MemFree, NLSTK_API_TIME_OUT);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_OK, "CP_PostTask failed in NLSTK_SsapServerDeregisterApplication function");

    return;
}

NLSTK_Errcode_E NLSTK_SsapServerCheckServiceExistByUuid(int32_t appId, NLSTK_SsapUuid_S *uuid, bool *isExist)
{
    NLSTK_CHECK_RETURN(uuid != NULL && isExist != NULL, NLSTK_ERRCODE_POINTER_NULL, "input param is NULL");
    NLSTK_CHECK_RETURN(appId >= 0 && appId < NLSTK_SSAP_SERVER_APP_MAX_NUM, NLSTK_ERRCODE_PARAM_ERR,
                         "app Id(%d) is invalid", appId);
    *isExist = false;
    SsapServerCheckServiceExistParam_S *param =
        (SsapServerCheckServiceExistParam_S *)SDF_MemZalloc(sizeof(SsapServerCheckServiceExistParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
                         "memory alloc error in NLSTK_SsapServerCheckServiceExistByUuid");
    param->appId = appId;
    (void)memcpy_s(&(param->uuid), sizeof(NLSTK_SsapUuid_S), uuid, sizeof(NLSTK_SsapUuid_S));
    param->isServiceExist = false;
    uint32_t ret =
        SchedulePostTaskBlocked(SsapServerCheckServiceExistByUuid, (void *)param, NULL, NLSTK_API_TIME_OUT);
    if (ret == NLSTK_ERRCODE_TASK_TIMEOUT) {
        // 在异常场景下，无法再处理，因此不判断返回值，推送一个内存释放的任务；
        (void)SchedulePostTask(NULL, (void *)param, SDF_MemFree);
        return NLSTK_ERRCODE_TASK_TIMEOUT;
    } else if (ret != NLSTK_OK) {
        SDF_MemFree(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *isExist = param->isServiceExist;
    SDF_MemFree(param);
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerSetMtu(uint16_t mtu)
{
    NLSTK_LOG_INFO("SSAP SERVER ENTER NLSTK_SsapServerSetMtu");
    NLSTK_CHECK_RETURN(mtu >= SSAP_STACK_MTU_DEFAULT && mtu <= SSAP_STACK_MTU_MAX, NLSTK_ERRCODE_PARAM_ERR,
                         "ssap set mtu info param error");
    SSAP_ServerExchangeInfo_S *serverExchangeInfo =
        (SSAP_ServerExchangeInfo_S *)SDF_MemZalloc(sizeof(SSAP_ServerExchangeInfo_S));
    NLSTK_CHECK_RETURN(serverExchangeInfo != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "memory alloc error");
    serverExchangeInfo->mtuSize = mtu;
    serverExchangeInfo->version = 0;  // 版本号暂时不用
    if (SchedulePostTask(SSAP_SetServerExchangeInfo, serverExchangeInfo, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("CP_PostTask failed in NLSTK_SsapServerSetMtu function");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerAddService(int32_t appId, const NLSTK_ServiceParam_S *service)
{
    NLSTK_CHECK_RETURN(appId >= 0 && appId < NLSTK_SSAP_SERVER_APP_MAX_NUM, NLSTK_ERRCODE_PARAM_ERR,
        "appId(%d) is invalid", appId);
    NLSTK_CHECK_RETURN(service != NULL, NLSTK_ERRCODE_POINTER_NULL, "input param is NULL");
    SSAP_Service_S *ssapService = SsapAllocServiceParam(service);
    NLSTK_CHECK_RETURN(ssapService != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "memory alloc error");
    ssapService->appId = appId;
    // 这块内存，当服务添加成功后，会将其放入g_services全局变量中，所以这里不需要释放，如果添加过程中失败，则由添加处释放
    // 避免出现多次内存申请和拷贝
    if (SchedulePostTask(SsapServerAddService, ssapService, NULL) != NLSTK_OK) {
        FreeService(ssapService);  // 当添加任务队列失败的时候，通过此处释放内存
        ssapService = NULL;
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerRemoveService(int32_t appId, uint16_t handle)
{
    NLSTK_CHECK_RETURN(appId >= 0 && appId < NLSTK_SSAP_SERVER_APP_MAX_NUM, NLSTK_ERRCODE_PARAM_ERR,
                         "appId(%d) is invalid", appId);
    SsapServerRemoveAppServiceParam_S *param =
        (SsapServerRemoveAppServiceParam_S *)SDF_MemZalloc(sizeof(SsapServerRemoveAppServiceParam_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "memory alloc error");
    param->appId = appId;
    param->handle = handle;
    if (SchedulePostTask(SsapServerRemoveAppService, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("CP_PostTask failed in NLSTK_SsapServerRemoveService function");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerClearServices(int32_t appId)
{
    NLSTK_CHECK_RETURN(appId >= 0 && appId < NLSTK_SSAP_SERVER_APP_MAX_NUM, NLSTK_ERRCODE_PARAM_ERR,
                         "appId(%d) is invalid", appId);
    int32_t *param = (int32_t *)SDF_MemZalloc(sizeof(int32_t));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "memory alloc error");
    *param = appId;
    if (SchedulePostTask(SsapServerCleanAppService, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("CP_PostTask failed in NLSTK_SsapServerClearServices function");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerUpdatePropertyValue(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value)
{
    NLSTK_CHECK_RETURN((appId >= 0 && appId < NLSTK_SSAP_SERVER_APP_MAX_NUM) && (value != NULL),
                         NLSTK_ERRCODE_PARAM_ERR, "appId(%d) or value is invalid", appId);
    NLSTK_CHECK_RETURN(value->len <= SSAP_MAX_VALUE_LENTH, NLSTK_ERRCODE_PARAM_ERR, "value length is invalid");
    SSAP_BufferedOperation_S *param =
        (SSAP_BufferedOperation_S *)SDF_MemZalloc(sizeof(SSAP_BufferedOperation_S) + value->len);
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "memory alloc error");
    param->propertyHandle = handle;
    param->dataType = SSAP_TYPE_DATA;
    param->value.len = value->len;
    param->appId = appId;
    (void)memcpy_s(param->value.value, value->len, value->data, value->len);
    if (SchedulePostTask(SSAPS_UpdateItemValueByHandle, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("SchedulePostTask failed in NLSTK_SsapServerUpdatePropertyValue function");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}
 
NLSTK_Errcode_E NLSTK_SsapServerUpdateDescriptorValue(int32_t appId, uint16_t handle, uint8_t type,
                                                  NLSTK_VariableData_S *value)
{
    NLSTK_CHECK_RETURN((appId >= 0 && appId < NLSTK_SSAP_SERVER_APP_MAX_NUM) && (value != NULL),
                         NLSTK_ERRCODE_PARAM_ERR, "appId(%d) or value is invalid", appId);
    NLSTK_CHECK_RETURN(value->len <= SSAP_MAX_VALUE_LENTH, NLSTK_ERRCODE_PARAM_ERR, "value length is invalid");

    SSAP_BufferedOperation_S *param =
        (SSAP_BufferedOperation_S *)SDF_MemZalloc(sizeof(SSAP_BufferedOperation_S) + value->len);
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "memory alloc error");
    param->propertyHandle = handle;
    param->dataType = type;
    param->value.len = value->len;
    param->appId = appId;
    (void)memcpy_s(param->value.value, value->len, value->data, value->len);
    if (SchedulePostTask(SSAPS_UpdateItemValueByHandle, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("SchedulePostTask failed in NLSTK_SsapServerUpdateDescriptorValue function");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerAuthorizeResult(int32_t appId, uint16_t requestId, bool allow)
{
    NLSTK_CHECK_RETURN((appId >= 0 && appId < NLSTK_SSAP_SERVER_APP_MAX_NUM),
        NLSTK_ERRCODE_PARAM_ERR, "appId(%d) is invalid", appId);
    SSAP_SendResponseValue_S *param = (SSAP_SendResponseValue_S *)SDF_MemZalloc(sizeof(SSAP_SendResponseValue_S));
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "param malloc fail");
    param->requestId = requestId;
    param->status = allow;
    if (SchedulePostTask(SSAPS_SendUserResponse, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("SchedulePostTask failed in NLSTK_SsapServerAuthorizeResult function");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerUpdateAndNotifyProperty(int32_t appId, uint16_t handle, SLE_Addr_S *addr,
                                                    NLSTK_VariableData_S *value)
{
    NLSTK_CHECK_RETURN((appId >= 0 && appId < NLSTK_SSAP_SERVER_APP_MAX_NUM) && (value != NULL) && (addr != NULL),
                         NLSTK_ERRCODE_PARAM_ERR, "appId(%d) or value is invalid", appId);
    NLSTK_CHECK_RETURN(value->len <= SSAP_MAX_VALUE_LENTH, NLSTK_ERRCODE_PARAM_ERR, "value length is invalid");
    uint8_t zeroAddr[SLE_ADDR_LEN] = { 0 };
    NLSTK_CHECK_RETURN(memcmp(addr->addr, zeroAddr, SLE_ADDR_LEN) != 0, NLSTK_ERRCODE_PARAM_ERR,
                         "addr is invalid in NLSTK_SsapServerUpdateAndNotifyProperty");

    SSAP_BufferedOperation_S *param =
        (SSAP_BufferedOperation_S *)SDF_MemZalloc(sizeof(SSAP_BufferedOperation_S) + value->len);
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "memory alloc error");
    param->propertyHandle = handle;
    param->addr.type = addr->type;
    (void)memcpy_s(param->addr.addr, SLE_ADDR_LEN, addr->addr, SLE_ADDR_LEN);
    param->dataType = 0; // 0表示需要更新的内容是属性值
    param->value.len = value->len;
    param->appId = appId;
    (void)memcpy_s(param->value.value, value->len, value->data, value->len);

    if (SchedulePostTask(SSAPS_UpdateItemValueByHandle, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("SchedulePostTask failed in NLSTK_SsapServerUpdateAndNotifyProperty function");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapServerSendMethodCallRes(int32_t appId, uint16_t requestId, NLSTK_VariableData_S *value)
{
    NLSTK_CHECK_RETURN((appId >= 0 && appId < NLSTK_SSAP_SERVER_APP_MAX_NUM) && (value != NULL),
                         NLSTK_ERRCODE_PARAM_ERR, "appId or value is invalid");
    NLSTK_CHECK_RETURN(value->len <= SSAP_MAX_VALUE_LENTH, NLSTK_ERRCODE_PARAM_ERR, "value length is invalid");

    SSAP_MethodCallResValue_S *param =
        (SSAP_MethodCallResValue_S *)SDF_MemZalloc(sizeof(SSAP_MethodCallResValue_S) + value->len);
    NLSTK_CHECK_RETURN(param != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "memory alloc error");
    param->requestId = requestId;
    param->len = value->len;
    (void)memcpy_s(param->data, param->len, value->data, value->len);

    if (SchedulePostTask(SSAPS_SendMethodCallRes, param, SDF_MemFree) != NLSTK_OK) {
        NLSTK_LOG_ERROR("SchedulePostTask failed in NLSTK_SsapServerSendMethodCallRes function");
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapCleanServerApp(NLSTK_SsapServerCleanAppResultCb serverCleanAppResultCb)
{
    // 这里不需要释放内存，因为传入的是一个函数指针，该地址在整个系统生命周期内都是固定的，因此不需要进行释放
    uint32_t ret = SchedulePostTask(SsapServerCleanApp, (void *)serverCleanAppResultCb, NULL);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
                         "[NLSTK_SSAPC] post task fail in NLSTK_SsapCleanServerApp");
    return NLSTK_ERRCODE_SUCCESS;
}

static void SsapFreeDescriptorParam(NLSTK_SsapServiceDescriptorParam_S *descriptors, uint16_t descriptorNum)
{
    if (descriptors == NULL) {
        return;
    }
    for (int i = 0; i < descriptorNum; i++) {
        SDF_MemFree(descriptors[i].val.data);
    }
    SDF_MemFree(descriptors);
}

static void SsapFreePropertyParam(NLSTK_SsapServicePropertyParam_S *property, uint16_t propertyNum)
{
    if (property == NULL) {
        return;
    }
    for (int i = 0; i < propertyNum; i++) {
        SsapFreeDescriptorParam(property[i].descriptors, property[i].descriptorNum);
        SDF_MemFree(property[i].val.data);
    }
    SDF_MemFree(property);
}

void NLSTK_SsapFreeServiceParam(void *ptr)
{
    NLSTK_ServiceParam_S *param = (NLSTK_ServiceParam_S *)ptr;
    if (param == NULL) {
        return;
    }
    SsapFreeDescriptorParam(param->serviceStatement.descriptors, param->serviceStatement.descriptorNum);
    if (param->serviceReference != NULL) {
        SDF_MemFree(param->serviceReference);
    }
    SsapFreePropertyParam(param->property, param->servicePropertyNum);
    if (param->method != NULL) {
        SDF_MemFree(param->method);
    }
    if (param->event != NULL) {
        SDF_MemFree(param->event);
    }
    SDF_MemFree(param);
}