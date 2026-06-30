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
#include "nlstk_ssap_app_server.h"
#include "ssaps_service.h"
#include "ssap_link.h"
#include "ssap_common.h"
#include "nlstk_log.h"
#include "sdf_vector.h"
#include "sdf_mem.h"
#include "securec.h"
#include "ssap_link_state.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_public_define.h"
#include "ssap_utils.h"
#include "ssaps_server_app.h"

/**
 * @brief  服务注册结果
 */
typedef struct {
    uint16_t beginHdl;
    uint16_t endHdl;
    NLSTK_SsapUuid_S uuid;
    uint8_t charactersCount;
} NLSTK_SsapStartServerResult_S;

typedef struct {
    uint8_t usedFlag;
    int32_t appId;
    SDF_Vector_S *serviceVector; /* 单个AppId下创建的所有服务的集合,这里面的元素是NLSTK_SsapStartServerResult_S */
    NLSTK_SsapAppServerCb_S cb;
} SsapServerApp_S;

#define SSAP_SERVER_APP_USED 1
#define SSAP_SERVER_APP_UNUSED 0

SsapServerApp_S g_ssapServerApp[NLSTK_SSAP_SERVER_APP_MAX_NUM];
NLSTK_SsapServerCleanAppResultCb g_ssapServerCleanAppResultCb = NULL;

void SsapServerRegApp(void *param)
{
    int32_t index = 0;
    SsapServerApp_S *serverApp = NULL;
    SsapRegServerAppParam_S *input = (SsapRegServerAppParam_S *)param;
    input->appId = SSAP_SERVER_APP_INVALID_APPID;

    for (index = 0; index < NLSTK_SSAP_SERVER_APP_MAX_NUM; index++) {
        if (g_ssapServerApp[index].usedFlag == SSAP_SERVER_APP_USED) {
            continue;
        }
        serverApp = &(g_ssapServerApp[index]);
        if (memcpy_s(&(serverApp->cb), sizeof(NLSTK_SsapAppServerCb_S), (void *)&(input->cb),
                     sizeof(NLSTK_SsapAppServerCb_S)) != EOK) {
            return;
        }
        SDF_Traits serviceTraits = { .dtor = SDF_MemFree };
        serverApp->serviceVector = SDF_CreateVector(serviceTraits);
        if (serverApp->serviceVector == NULL) {
            memset_s(&(serverApp->cb), sizeof(NLSTK_SsapAppServerCb_S), 0, sizeof(NLSTK_SsapAppServerCb_S));
            return;
        }
        serverApp->appId = index;
        serverApp->usedFlag = SSAP_SERVER_APP_USED;
        input->appId = index;
        NLSTK_LOG_INFO("serverApp->appId=%d", serverApp->appId);
        return;
    }
}

void SsapServerRegAppAsyn(void *param)
{
    int32_t index = 0;
    SsapServerApp_S *serverApp = NULL;
    SsapRegServerAppParam_S *input = (SsapRegServerAppParam_S *)param;
    input->appId = SSAP_SERVER_APP_INVALID_APPID;

    for (index = 0; index < NLSTK_SSAP_SERVER_APP_MAX_NUM; index++) {
        if (g_ssapServerApp[index].usedFlag == SSAP_SERVER_APP_USED) {
            continue;
        }
        serverApp = &(g_ssapServerApp[index]);
        if (memcpy_s(&(serverApp->cb), sizeof(NLSTK_SsapAppServerCb_S), (void *)&(input->cb),
                     sizeof(NLSTK_SsapAppServerCb_S)) != EOK) {
            return;
        }
        SDF_Traits serviceTraits = { .dtor = SDF_MemFree };
        serverApp->serviceVector = SDF_CreateVector(serviceTraits);
        if (serverApp->serviceVector == NULL) {
            memset_s(&(serverApp->cb), sizeof(NLSTK_SsapAppServerCb_S), 0, sizeof(NLSTK_SsapAppServerCb_S));
            return;
        }
        serverApp->appId = index;
        serverApp->usedFlag = SSAP_SERVER_APP_USED;
        input->appId = index;
        NLSTK_LOG_INFO("serverApp->appId=%d", serverApp->appId);
        if (input->cb.onRegisterApp) {
            input->cb.onRegisterApp(input->appId, NLSTK_ERRCODE_SUCCESS);
        }
        break;
    }
}

void SsapServerDeregisterApplication(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "param is null when de-register App");
    SsapRegServerAppParam_S *input = (SsapRegServerAppParam_S *)param;
    NLSTK_LOG_INFO("Deregister server appid %d", input->appId);
    int32_t index = input->appId;
    NLSTK_CHECK_RETURN_VOID(index >= 0 && index < NLSTK_SSAP_SERVER_APP_MAX_NUM, "index is invalid");
    if (g_ssapServerApp[index].usedFlag == SSAP_SERVER_APP_USED) {
        SsapServerApp_S *serverApp = &(g_ssapServerApp[index]);
        memset_s(&(serverApp->cb), sizeof(NLSTK_SsapAppServerCb_S), 0, sizeof(NLSTK_SsapAppServerCb_S));
        SsapServerCleanAppService((void *)&index);  // 清理掉所有的service
        serverApp->appId = SSAP_SERVER_APP_INVALID_APPID;
        serverApp->usedFlag = SSAP_SERVER_APP_UNUSED;
        SDF_DestroyVector(serverApp->serviceVector);
        serverApp->serviceVector = NULL;
    }
    return;
}

void SsapServerCheckServiceExistByUuid(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "param is null when check the service exist or not");
    SsapServerCheckServiceExistParam_S *input = (SsapServerCheckServiceExistParam_S *)param;
    NLSTK_LOG_INFO("Check service exist appid %d, uuid %s", input->appId, SSAP_GET_ENC_UUID(&input->uuid));
    input->isServiceExist = false;
    SsapServerApp_S *ssapServerApp = &(g_ssapServerApp[input->appId]);
    if (ssapServerApp->usedFlag != SSAP_SERVER_APP_USED || ssapServerApp->serviceVector == NULL) {
        return;
    }
    for (uint32_t i = 0; i < ssapServerApp->serviceVector->size; i++) {
        NLSTK_SsapStartServerResult_S *service =
            (NLSTK_SsapStartServerResult_S *)SDF_VectorElementAt(ssapServerApp->serviceVector, i);
        if (memcmp(&(service->uuid), &(input->uuid), sizeof(NLSTK_SsapUuid_S)) == 0) {
            input->isServiceExist = true;
            return;
        }
    }
    return;
}

void SsapServerRemoveAppService(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[SSAP] remove service param is null");
    SsapServerRemoveAppServiceParam_S *removeService = (SsapServerRemoveAppServiceParam_S *)param;
    NLSTK_LOG_INFO("Remove server service appid %d, handle %d", removeService->appId, removeService->handle);
    SsapServerApp_S *serverApp = &(g_ssapServerApp[removeService->appId]);
    NLSTK_CHECK_RETURN_VOID(serverApp->usedFlag == SSAP_SERVER_APP_USED, "[SSAP] remove service app id unused");
    NLSTK_CHECK_RETURN_VOID(serverApp->serviceVector != NULL, "[SSAP] remove service vector is NULL");
    NLSTK_SsapStartServerResult_S *startResult = NULL;
    for (uint32_t i = 0; i < serverApp->serviceVector->size; i++) {
        startResult = (NLSTK_SsapStartServerResult_S *)SDF_VectorElementAt(serverApp->serviceVector, i);
        if (removeService->handle >= startResult->beginHdl && removeService->handle <= startResult->endHdl) {
            SSAP_ParamRemoveService_S removeServiceParam = { 0 };
            removeServiceParam.startHandle = startResult->beginHdl;
            removeServiceParam.endHandle = startResult->endHdl;
            SSAPS_RemoveService(&removeServiceParam);
            SDF_VectorRemove(serverApp->serviceVector, i);  // 这里会释放startResult的内存，因此不需要额外释放
            return;
        }
    }
}

void SsapServerCleanAppService(void *param)
{
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[SSAP] clean service param is null");
    int32_t appId = *(int32_t *)param;
    NLSTK_LOG_INFO("Clean server service appid %d", appId);
    SsapServerApp_S *serverApp = &(g_ssapServerApp[appId]);
    NLSTK_CHECK_RETURN_VOID(serverApp->usedFlag == SSAP_SERVER_APP_USED, "[SSAP] clear service app id unused");
    NLSTK_CHECK_RETURN_VOID(serverApp->serviceVector != NULL, "[SSAP] clear service vector is NULL");
    NLSTK_SsapStartServerResult_S *startResult = NULL;
    while (serverApp->serviceVector->size > 0) {
        startResult = (NLSTK_SsapStartServerResult_S *)SDF_VectorElementAt(serverApp->serviceVector, 0);
        SSAP_ParamRemoveService_S removeServiceParam = { 0 };
        removeServiceParam.startHandle = startResult->beginHdl;
        removeServiceParam.endHandle = startResult->endHdl;
        SSAPS_RemoveService(&removeServiceParam);
        SDF_VectorRemove(serverApp->serviceVector, 0);  // 这里会释放startResult的内存，因此不需要额外释放
    }
}

static SsapServerApp_S *GetServerAppByHandle(uint16_t handle)
{
    SsapServerApp_S *serverApp = NULL;
    SDF_Vector_S *services = SSAPS_GetServices();
    NLSTK_CHECK_RETURN(services != NULL, NULL, "[SSAP] service is null");
    for (size_t i = 0; i < services->size; i++) {
        SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, i);
        if (handle >= service->handle && handle <= service->endHandle) {
            serverApp = &(g_ssapServerApp[service->appId]);
            return serverApp;
        }
    }
    return NULL;
}

// <----下面的函数均是注册给SSAP通道层的回调函数>
void SsapServerLinkStateNofity(SLE_Addr_S *addr, NLSTK_SsapConnectLinkState_E state, NLSTK_Errcode_E errCode,
                               int32_t reason)
{
    for (uint32_t index = 0; index < NLSTK_SSAP_SERVER_APP_MAX_NUM; index++) {
        if (g_ssapServerApp[index].usedFlag == SSAP_SERVER_APP_UNUSED) {
            continue;
        }
        SsapServerApp_S *serverApp = &(g_ssapServerApp[index]);
        if (serverApp->cb.onConnectionStateChanged != NULL) {
            // 此处均是CM通知上报的回调，因此ret和reason都是0,现在CM模块不上报reason
            serverApp->cb.onConnectionStateChanged(serverApp->appId, addr, state, errCode, reason);
        }
    }
}

static void SsapServerCleanAllApp(void)
{
    for (int32_t index = 0; index < NLSTK_SSAP_SERVER_APP_MAX_NUM; index++) {
        if (g_ssapServerApp[index].usedFlag == SSAP_SERVER_APP_UNUSED) {
            continue;
        }
        SsapRegServerAppParam_S deRegParam = { 0 };
        deRegParam.appId = index;
        SsapServerDeregisterApplication((void *)&deRegParam);
    }
}

void SsapServerAppCleanUpNotify(void)
{
    if (SsapIsAllLinkCleanUp() == true) {
        SsapServerCleanAllApp();
        NLSTK_LOG_INFO("clean up all server appId and notify service");
        if (g_ssapServerCleanAppResultCb != NULL) {
            g_ssapServerCleanAppResultCb();
        }
        // 表示本次appId已经完成了清理，复位清理标记
        SsapResetServerCleanUp();
    }
}

// 此函数注册给startServiceCbk，当服务启动成功后，会回调此函数
void SsapServerAppMtuExchangeCallBack(SSAP_MtuInfo_S *mtuInfo)
{
    NLSTK_CHECK_RETURN_VOID(mtuInfo != NULL, "mtuInfo param is null");
    SSAP_Link_S *link = SSAP_FindSsapLinkByLcid(mtuInfo->connId);
    NLSTK_CHECK_RETURN_VOID(link != NULL, "[SSAP] cant find link");
    SLE_Addr_S addr = { 0 };
    addr.type = link->addr.type;
    (void)memcpy_s(&(addr.addr), SLE_ADDR_LEN, &(link->addr.addr), SLE_ADDR_LEN);

    // 通知所有注册的应用
    for (int32_t i = 0; i < NLSTK_SSAP_SERVER_APP_MAX_NUM; i++) {
        SsapServerApp_S *ssapServerApp = &(g_ssapServerApp[i]);
        if (ssapServerApp->usedFlag == SSAP_SERVER_APP_USED) {
            if (ssapServerApp->cb.onMtuChanged == NULL) {
                NLSTK_LOG_ERROR("[SSAP] onMtuChanged is null, appid is %d", ssapServerApp->appId);
                continue;
            }
            ssapServerApp->cb.onMtuChanged(i, &addr, mtuInfo->mtuSize);
        }
    }
}

void SsapServerAppAddServiceExceptionCallBack(SsapAsyncProcessResult_S *result)
{
    NLSTK_CHECK_RETURN_VOID(result != NULL, "[SSAP] add service exception param is null");
    SsapServerApp_S *ssapServerApp = &(g_ssapServerApp[result->appId]);
    NLSTK_CHECK_RETURN_VOID(ssapServerApp->cb.onAddService != NULL, "[SSAP] onAddService is null");
    ssapServerApp->cb.onAddService(result->appId, NULL, result->errorCode);
}

void SsapServerAppStartServiceCallBack(SSAP_Service_S *service)
{
    NLSTK_CHECK_RETURN_VOID(service != NULL, "SsapServerAppStartServiceCallBack service null");
    int32_t appId = service->appId;
    SsapServerApp_S *ssapServerApp = &(g_ssapServerApp[appId]);

    NLSTK_SsapStartServerResult_S *startResult =
        (NLSTK_SsapStartServerResult_S *)SDF_MemZalloc(sizeof(NLSTK_SsapStartServerResult_S));
    NLSTK_CHECK_RETURN_VOID(startResult != NULL, "malloc mem failed in SsapServerAppStartServiceCallBack");
    startResult->beginHdl = service->handle;
    startResult->endHdl = service->endHandle;
    startResult->charactersCount = (uint8_t)service->properties->size;
    (void)memcpy_s(&(startResult->uuid), sizeof(NLSTK_SsapUuid_S), &(service->uuid), sizeof(NLSTK_SsapUuid_S));
    // 将startResult添加到serverApp的serviceVector中之后，内存在全局变量中统一管理,不需要在此处额外释放
    if (!SDF_VectorEmplaceBack(ssapServerApp->serviceVector, startResult)) {
        NLSTK_LOG_ERROR("[SSAP] cache property add vector failed");
        SDF_MemFree(startResult);
        return;
    }
    NLSTK_CHECK_RETURN_VOID(ssapServerApp->cb.onAddService != NULL, "[SSAP] onAddService is null");
    ssapServerApp->cb.onAddService(appId, service, NLSTK_ERRCODE_SUCCESS);
}

static void SsapServerAppUpdateAndNotifyPropertyCallBack(SSAP_BufferedOperation_S *operation)
{
    SsapServerApp_S *serverApp = GetServerAppByHandle(operation->propertyHandle);
    NLSTK_CHECK_RETURN_VOID(serverApp != NULL, "can not find the service in SsapServerUpdateValueResultCallback");
    NLSTK_SsapServerOnNotifyPropertyParam_S *property =
        SDF_MemZalloc(sizeof(NLSTK_SsapServerOnNotifyPropertyParam_S));
    NLSTK_CHECK_RETURN_VOID(property != NULL, "SsapServerUpdateValueResultCallback malloc mem failed");
    property->handle = operation->propertyHandle;
    (void)memcpy_s(&(property->addr), sizeof(SLE_Addr_S), &(operation->addr), sizeof(SLE_Addr_S));
    (void)memcpy_s(&(property->uuid), sizeof(NLSTK_SsapUuid_S), &(operation->propertyUuid), sizeof(NLSTK_SsapUuid_S));
    property->value.len = operation->value.len;
    property->value.data = (uint8_t *)SDF_MemZalloc(operation->value.len);
    if (property->value.data == NULL) {
        SDF_MemFree(property);
        return;
    }
    if (memcpy_s(property->value.data, operation->value.len, operation->value.value, operation->value.len) != 0) {
        SDF_MemFree(property->value.data);
        SDF_MemFree(property);
        return;
    }
    if (serverApp->cb.onNotifyProperty != NULL) {
        serverApp->cb.onNotifyProperty(serverApp->appId, property, operation->errCode);
    }
    SDF_MemFree(property->value.data);
    SDF_MemFree(property);
    return;
}

static void SsapServerAppUpdatePropertyCallBack(SSAP_BufferedOperation_S *operation)
{
    SsapServerApp_S *serverApp = GetServerAppByHandle(operation->propertyHandle);
    NLSTK_CHECK_RETURN_VOID(serverApp != NULL, "can not find the service in SsapServerUpdateValueResultCallback");
    NLSTK_SsapServerOnSetPropertyParam_S *propertyUpdate =
        SDF_MemZalloc(sizeof(NLSTK_SsapServerOnSetPropertyParam_S));
    NLSTK_CHECK_RETURN_VOID(propertyUpdate != NULL, "SsapServerUpdateValueResultCallback malloc mem failed");
    propertyUpdate->handle = operation->propertyHandle;
    propertyUpdate->value.len = operation->value.len;
    (void)memcpy_s(&(propertyUpdate->uuid), sizeof(NLSTK_SsapUuid_S),
        &(operation->propertyUuid), sizeof(NLSTK_SsapUuid_S));
    propertyUpdate->value.data = (uint8_t *)SDF_MemZalloc(operation->value.len);
    if (propertyUpdate->value.data == NULL) {
        SDF_MemFree(propertyUpdate);
        NLSTK_LOG_ERROR("mem alloc failed when alloc data for property");
        return;
    }
    (void)memcpy_s(propertyUpdate->value.data, operation->value.len, operation->value.value, operation->value.len);
    if (serverApp->cb.onSetPropertyValue != NULL) {
        serverApp->cb.onSetPropertyValue(serverApp->appId, propertyUpdate, operation->errCode);
    }
    SDF_MemFree(propertyUpdate->value.data);
    SDF_MemFree(propertyUpdate);
    return;
}

static void SsapServerAppUpdateDescriptorCallBack(SSAP_BufferedOperation_S *operation)
{
    SsapServerApp_S *serverApp = GetServerAppByHandle(operation->propertyHandle);
    NLSTK_CHECK_RETURN_VOID(serverApp != NULL, "can not find the service in SsapServerUpdateValueResultCallback");
    NLSTK_SsapServerOnSetDescriptorParam_S *descriptor =
        SDF_MemZalloc(sizeof(NLSTK_SsapServerOnSetDescriptorParam_S));
    NLSTK_CHECK_RETURN_VOID(descriptor != NULL, "SsapServerUpdateValueResultCallback malloc mem failed");
    descriptor->handle = operation->propertyHandle;
    descriptor->value.len = operation->value.len;
    (void)memcpy_s(&(descriptor->uuid), sizeof(NLSTK_SsapUuid_S), &(operation->propertyUuid), sizeof(NLSTK_SsapUuid_S));
    descriptor->type = operation->dataType;
    descriptor->value.data = (uint8_t *)SDF_MemZalloc(operation->value.len);
    if (descriptor->value.data == NULL) {
        SDF_MemFree(descriptor);
        NLSTK_LOG_ERROR("mem alloc failed when alloc data for descriptor");
        return;
    }
    (void)memcpy_s(descriptor->value.data, operation->value.len, operation->value.value, operation->value.len);
    if (serverApp->cb.onSetDescriptorValue != NULL) {
        serverApp->cb.onSetDescriptorValue(serverApp->appId, descriptor, operation->errCode);
    }
    SDF_MemFree(descriptor->value.data);
    SDF_MemFree(descriptor);
    return;
}

void SsapServerUpdateValueResultCallback(SSAP_BufferedOperation_S *operation)
{
    NLSTK_CHECK_RETURN_VOID(operation != NULL, "SsapServerUpdateValueResultCallback service null");

    uint8_t zeroAddr[SLE_ADDR_LEN] = { 0 };
    // 当operation->addr.addr中的地址不是0时，表示用户是通过 NLSTK_SsapServerUpdateAndNotifyProperty 接口发起的
    // 此时需要通过onNotifyProperty的回调通知给service
    if (memcmp(operation->addr.addr, zeroAddr, SLE_ADDR_LEN) != 0) {
        SsapServerAppUpdateAndNotifyPropertyCallBack(operation);
    } else {
        if (operation->dataType == 0) {  // 表示更新的是属性
            SsapServerAppUpdatePropertyCallBack(operation);
        } else {
            SsapServerAppUpdateDescriptorCallBack(operation);
        }
    }
    return;
}

static void SsapServerAppWritePropertyCallback(SSAP_BufferedOperation_S *operation)
{
    SsapServerApp_S *serverApp = GetServerAppByHandle(operation->propertyHandle);
    NLSTK_CHECK_RETURN_VOID(serverApp != NULL, "can not find the service, prop handle is %d",
        operation->propertyHandle);

    NLSTK_SsapServerWritePropertyInfo_S *writeInfo = (NLSTK_SsapServerWritePropertyInfo_S *)SDF_MemZalloc(
        sizeof(NLSTK_SsapServerWritePropertyInfo_S));
    NLSTK_CHECK_RETURN_VOID(writeInfo != NULL, "mem alloc failed in SsapServerAppWritePropertyCallback");
    writeInfo->handle = operation->propertyHandle;
    (void)memcpy_s(&(writeInfo->uuid), sizeof(NLSTK_SsapUuid_S), &(operation->propertyUuid), sizeof(NLSTK_SsapUuid_S));
    (void)memcpy_s(&(writeInfo->addr), sizeof(SLE_Addr_S), &(operation->addr), sizeof(SLE_Addr_S));
    writeInfo->value.len = operation->value.len;
    writeInfo->value.data = (uint8_t *)SDF_MemZalloc(operation->value.len);
    if (writeInfo->value.data == NULL) {
        SDF_MemFree(writeInfo);
        return;
    }
    if (memcpy_s(writeInfo->value.data, operation->value.len, operation->value.value, operation->value.len) != 0) {
        SDF_MemFree(writeInfo->value.data);
        SDF_MemFree(writeInfo);
        return;
    }
    if (operation->needAuth) {
        // 需要授权
        if (serverApp->cb.onWritePropertyAuthorizeRequest != NULL) {
            serverApp->cb.onWritePropertyAuthorizeRequest(serverApp->appId, operation->requestId, writeInfo);
        }
    } else {
        // 当前只有写成功才会上报回调
        if (serverApp->cb.onWriteProperty != NULL) {
            serverApp->cb.onWriteProperty(serverApp->appId, writeInfo);
        }
    }
    SDF_MemFree(writeInfo->value.data);
    SDF_MemFree(writeInfo);
    return;
}

static void SsapServerAppWriteDescriptorCallback(SSAP_BufferedOperation_S *operation)
{
    SsapServerApp_S *serverApp = GetServerAppByHandle(operation->propertyHandle);
    NLSTK_CHECK_RETURN_VOID(serverApp != NULL, "can not find the service, prop handle is %d",
        operation->propertyHandle);

    NLSTK_SsapServerWriteDescriptorInfo_S *writeInfo = (NLSTK_SsapServerWriteDescriptorInfo_S *)SDF_MemZalloc(
        sizeof(NLSTK_SsapServerWriteDescriptorInfo_S));    // 防御式编程，防止operation->value.len
    NLSTK_CHECK_RETURN_VOID(writeInfo != NULL, "mem alloc failed in SsapServerAppWriteDescriptorCallback");
    writeInfo->handle = operation->propertyHandle;
    writeInfo->type = operation->dataType;
    (void)memcpy_s(&(writeInfo->uuid), sizeof(NLSTK_SsapUuid_S), &(operation->propertyUuid), sizeof(NLSTK_SsapUuid_S));
    (void)memcpy_s(&(writeInfo->addr), sizeof(SLE_Addr_S), &(operation->addr), sizeof(SLE_Addr_S));
    writeInfo->value.len = operation->value.len;
    writeInfo->value.data = (uint8_t *)SDF_MemZalloc(operation->value.len);
    if (writeInfo->value.data == NULL) {
        SDF_MemFree(writeInfo);
        return;
    }
    if (memcpy_s(writeInfo->value.data, operation->value.len, operation->value.value, operation->value.len) != 0) {
        SDF_MemFree(writeInfo->value.data);
        SDF_MemFree(writeInfo);
        return;
    }
    if (operation->needAuth) {
        // 需要授权
        if (serverApp->cb.onWritePropertyAuthorizeRequest != NULL) {
            serverApp->cb.onWriteDescriptorAuthorizeRequest(serverApp->appId, operation->requestId, writeInfo);
        }
    } else {
        // 当前只有写成功才会上报回调
        if (serverApp->cb.onWriteDescriptor != NULL) {
            serverApp->cb.onWriteDescriptor(serverApp->appId, writeInfo);
        }
    }
    SDF_MemFree(writeInfo->value.data);
    SDF_MemFree(writeInfo);
    return;
}

void SsapServerAppWriteValueCallback(SSAP_BufferedOperation_S *operation)
{
    NLSTK_CHECK_RETURN_VOID(operation != NULL, "operation is null");
    if (operation->errCode != NLSTK_ERRCODE_SUCCESS) { // 写入失败，由SSAP回复异常响应，不需要上报
        return;
    }
    if (operation->dataType == 0) {  // type 0 表示属性，type是其它值表示描述符
        SsapServerAppWritePropertyCallback(operation);
    } else {
        SsapServerAppWriteDescriptorCallback(operation);
    }
    return;
}

static void SsapServerAppReadPropertyCallback(SSAP_BufferedOperation_S *operation)
{
    SsapServerApp_S *serverApp = GetServerAppByHandle(operation->propertyHandle);
    NLSTK_CHECK_RETURN_VOID(serverApp != NULL, "can not find the service, prop handle is %d",
        operation->propertyHandle);

    NLSTK_SsapServerReadPropertyInfo_S readInfo = { 0 };
    (void)memcpy_s(&(readInfo.uuid), sizeof(NLSTK_SsapUuid_S), &(operation->propertyUuid), sizeof(NLSTK_SsapUuid_S));
    readInfo.handle = operation->propertyHandle;
    (void)memcpy_s(&(readInfo.addr), sizeof(SLE_Addr_S), &(operation->addr), sizeof(SLE_Addr_S));
    if (operation->needAuth) {
        // 需要授权
        if (serverApp->cb.onReadPropertyAuthorizeRequest != NULL) {
            serverApp->cb.onReadPropertyAuthorizeRequest(serverApp->appId, operation->requestId, &readInfo);
        }
    } else {
        if (serverApp->cb.onReadProperty != NULL) {
            serverApp->cb.onReadProperty(serverApp->appId, &readInfo);
        }
    }
    return;
}

static void SsapServerAppReadDescriptorCallback(SSAP_BufferedOperation_S *operation)
{
    SsapServerApp_S *serverApp = GetServerAppByHandle(operation->propertyHandle);
    NLSTK_CHECK_RETURN_VOID(serverApp != NULL, "can not find the service, prop handle is %d",
        operation->propertyHandle);

    NLSTK_SsapServerReadDescriptorInfo_S readInfo = { 0 };
    (void)memcpy_s(&(readInfo.uuid), sizeof(NLSTK_SsapUuid_S), &(operation->propertyUuid), sizeof(NLSTK_SsapUuid_S));
    readInfo.handle = operation->propertyHandle;
    readInfo.type = operation->dataType;
    (void)memcpy_s(&(readInfo.addr), sizeof(SLE_Addr_S), &(operation->addr), sizeof(SLE_Addr_S));
    if (operation->needAuth) {
        // 需要授权
        if (serverApp->cb.onReadDescriptorAuthorizeRequest != NULL) {
            serverApp->cb.onReadDescriptorAuthorizeRequest(serverApp->appId, operation->requestId, &readInfo);
        }
    } else {
        if (serverApp->cb.onReadDescriptor != NULL) {
            serverApp->cb.onReadDescriptor(serverApp->appId, &readInfo);
        }
    }
}

void SsapServerAppReadValueCallback(SSAP_BufferedOperation_S *operation)
{
    NLSTK_CHECK_RETURN_VOID(operation != NULL, "operation is null");
    if (operation->errCode != NLSTK_ERRCODE_SUCCESS) { // 客户端读取消息失败，不需要上报
        return;
    }
    if (operation->dataType == 0) {
        SsapServerAppReadPropertyCallback(operation);
    } else {
        SsapServerAppReadDescriptorCallback(operation);
    }
    return;
}

void SsapServerAppIndicateCfmValueCallback(SSAP_ValuePkt_S *valuePkt)
{
    NLSTK_CHECK_RETURN_VOID(valuePkt != NULL, "valuePkt is null");
    NLSTK_LOG_INFO("cfm value handle is %d", valuePkt->handle);
}

void SsapServerAppCallMethodCallback(SSAP_BufferedOperation_S *methodCall)
{
    NLSTK_CHECK_RETURN_VOID(methodCall != NULL, "operation is null");
    SsapServerApp_S *serverApp = GetServerAppByHandle(methodCall->propertyHandle);
    NLSTK_CHECK_RETURN_VOID(serverApp != NULL, "can not find the service");
    NLSTK_SsapServerCallMethodRequestInfo_S method = { 0 };
    method.handle = methodCall->propertyHandle;
    (void)memcpy_s(&(method.addr), sizeof(SLE_Addr_S), &(methodCall->addr), sizeof(SLE_Addr_S));
    (void)memcpy_s(&(method.uuid), sizeof(NLSTK_SsapUuid_S), &(methodCall->propertyUuid), sizeof(NLSTK_SsapUuid_S));
    method.param.len = methodCall->value.len;
    method.param.data = SDF_MemZalloc(method.param.len);
    NLSTK_CHECK_RETURN_VOID(method.param.data != NULL, "memery alloc fail, no resouce");
    (void)memcpy_s(method.param.data, method.param.len, methodCall->value.value, method.param.len);
    serverApp->cb.onCallMethod(serverApp->appId, methodCall->requestId, &method, methodCall->needRsp,
                               methodCall->needAuth);
    SDF_MemFree(method.param.data);
    return;
}

void SsapServerCleanApp(void *param)
{
    NLSTK_LOG_INFO("start clean all ssap server app");
    g_ssapServerCleanAppResultCb = param;
    if (SsapIsAllLinkCleanUp() == true) {
        SsapServerCleanAllApp();
        NLSTK_LOG_INFO("clean up all server appId and notify service");
        if (g_ssapServerCleanAppResultCb != NULL) {
            g_ssapServerCleanAppResultCb();
        }
        // 表示本次appId已经完成了清理，复位清理标记
        SsapResetServerCleanUp();
    } else {
        SsapRemoveAllLink();
    }
}

void SsapServerAppDeinit(void)
{
    for (int32_t index = 0; index < NLSTK_SSAP_SERVER_APP_MAX_NUM; index++) {
        if (g_ssapServerApp[index].usedFlag == SSAP_SERVER_APP_UNUSED) {
            continue;
        }
        SsapServerApp_S *serverApp = &(g_ssapServerApp[index]);
        memset_s(&(serverApp->cb), sizeof(NLSTK_SsapAppServerCb_S), 0, sizeof(NLSTK_SsapAppServerCb_S));
        serverApp->appId = SSAP_SERVER_APP_INVALID_APPID;
        serverApp->usedFlag = SSAP_SERVER_APP_UNUSED;
        SDF_DestroyVector(serverApp->serviceVector);
        serverApp->serviceVector = NULL;
    }
    g_ssapServerCleanAppResultCb = NULL;
}