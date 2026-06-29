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
#include "ssapc_app.h"
#include "securec.h"
#include "nlstk_schedule.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_ssap_app_client.h"

static void FreeAppRegParam(void *arg)
{
    if (arg == NULL) {
        return;
    }
    SsapcAppRegParam_S *param = (SsapcAppRegParam_S *)arg;
    if (param->cb != NULL) {
        SDF_MemFree(param->cb);
    }
    SDF_MemFree(param);
}

static void FreeServObtainAsyn(void *arg)
{
    if (arg == NULL) {
        return;
    }
    NLSTK_SsapServObtainAsyn_S *param = (NLSTK_SsapServObtainAsyn_S *)arg;
    if (param->uuid != NULL) {
        SDF_MemFree(param->uuid);
    }
    SDF_MemFree(param);
}

/**
 * @brief 注册应用客户端
 * @details 该函数用于注册一个应用客户端，将回调函数传递给系统，以便在后续处理中使用。
 * @param [in] cb 指向回调函数结构体的指针
 * @param [in] addr 指向对端服务端设备地址的指针
 * @param [in] secReq 指示连接后要不要做鉴权加密
 * @param [out] appId 返回注册的appId，返回<0 注册失败，具体错误信息可通过错误码查看
 * @return 错误码信息
 */
NLSTK_Errcode_E NLSTK_SsapClientRegApp(int32_t *appId, NLSTK_SsapAppClientCb_S *cb, SLE_Addr_S *addr)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientRegApp");
    NLSTK_CHECK_RETURN(appId && cb && addr, NLSTK_ERRCODE_POINTER_NULL, "[NLSTK_SSAPC] input param is null");
    SsapcAppRegParam_S *param = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc param fails");
    param->cb = (NLSTK_SsapAppClientCb_S *)SDF_MemZalloc(sizeof(NLSTK_SsapAppClientCb_S));
    if (param->cb == NULL) {
        NLSTK_LOG_ERROR("[NLSTK_SSAPC] malloc param fails");
        SDF_MemFree(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    (void)memcpy_s(&(param->addr), sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(param->cb, sizeof(NLSTK_SsapAppClientCb_S), cb, sizeof(NLSTK_SsapAppClientCb_S));
    param->linkState = SSAP_CONNECT_STATE_IDLE;
    param->nextOperator = SSAP_LINK_CHANGE_EVENT_BUTT;
    param->timeout = SSAP_INTERACTION_DEFAULT_TIMEOUT;
    uint32_t taskRet = SchedulePostTaskBlocked(SsapcAppRegister, (void *)param, NULL, NLSTK_API_TIME_OUT);
    if (taskRet == NLSTK_ERRCODE_TASK_TIMEOUT) {
        (void)SchedulePostTask(SsapcAppDeregister, (void *)param, FreeAppRegParam);
        return taskRet;
    } else if (taskRet != NLSTK_OK || param->appId == SSAP_APP_INVALID_ID) {
        FreeAppRegParam(param);
        return NLSTK_ERRCODE_TASK_FAIL;
    }
    *appId = param->appId;
    FreeAppRegParam(param);
    return NLSTK_ERRCODE_SUCCESS;
}

/**
 * @brief 注册应用客户端异步接口
 * @details 该函数用于注册一个应用客户端，将回调函数传递给系统，AppId通过回调返回，请确保注册的接收AppId的回调函数不为空。
 * @param [in] addr 指向对端服务端设备地址的指针
 * @param [in] cb 指向回调函数结构体的指针
 * @return 错误码信息
 */
NLSTK_Errcode_E NLSTK_SsapClientRegAppAsyn(SLE_Addr_S *addr, NLSTK_ConnParam_S *connParam, NLSTK_SsapAppClientCb_S *cb)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientRegAppAsyn");
    NLSTK_CHECK_RETURN(addr && connParam && cb, NLSTK_ERRCODE_POINTER_NULL, "[NLSTK_SSAPC] input param is null");
    SsapcAppRegParam_S *param = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc param fail");
    param->cb = (NLSTK_SsapAppClientCb_S *)SDF_MemZalloc(sizeof(NLSTK_SsapAppClientCb_S));
    if (param->cb == NULL) {
        NLSTK_LOG_ERROR("[NLSTK_SSAPC] malloc param cb fail");
        SDF_MemFree(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    (void)memcpy_s(&(param->addr), sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(param->cb, sizeof(NLSTK_SsapAppClientCb_S), cb, sizeof(NLSTK_SsapAppClientCb_S));
    (void)memcpy_s(&param->connParam, sizeof(NLSTK_ConnParam_S), connParam, sizeof(NLSTK_ConnParam_S));
    param->linkState = SSAP_CONNECT_STATE_IDLE;
    param->nextOperator = SSAP_LINK_CHANGE_EVENT_BUTT;
    param->timeout = SSAP_INTERACTION_DEFAULT_TIMEOUT;
    uint32_t taskRet = SchedulePostTask(SsapcAppRegisterAsync, (void *)param, FreeAppRegParam);
    NLSTK_CHECK_RETURN(taskRet == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[NLSTK_SSAPC] post task fail in NLSTK_SsapClientRegAppAsyn");
    return NLSTK_ERRCODE_SUCCESS;
}

/**
 * @brief 注销应用，异步接口
 * @details 该函数用于注销一个已注册的应用客户端，释放相关资源并停止与该应用相关的客户端服务。
 * @param [in] appId 应用的唯一标识符
 * @note 请确保appId有效，否则可能导致意外行为。注销后，该应用客户端将无法再使用
 */
void NLSTK_SsapClientDeregAppAsync(int32_t appId)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientDeregApp");
    SsapcAppRegParam_S *param = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));
    NLSTK_CHECK_RETURN_VOID(param, "[NLSTK_SSAPC] malloc appId param failed");
    param->appId = appId;
    uint32_t ret = SchedulePostTask(SsapcAppDeregister, (void *)param, SDF_MemFree);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_OK, "[NLSTK_SSAPC] post task fail in NLSTK_SsapClientDeregAppAsync");
    return;
}

/**
 * @brief 注销应用
 * @details 该函数用于注销一个已注册的应用客户端，释放相关资源并停止与该应用相关的客户端服务。
 * @param [in] appId 应用的唯一标识符
 * @note 请确保appId有效，否则可能导致意外行为。注销后，该应用客户端将无法再使用
 */
void NLSTK_SsapClientDeregApp(int32_t appId)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientDeregApp");
    SsapcAppRegParam_S *param = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));
    NLSTK_CHECK_RETURN_VOID(param, "[NLSTK_SSAPC] malloc appId param failed");
    param->appId = appId;
    uint32_t ret = SchedulePostTaskBlocked(SsapcAppDeregister, (void *)param, SDF_MemFree, NLSTK_API_TIME_OUT);
    NLSTK_CHECK_RETURN_VOID(ret == NLSTK_OK, "[NLSTK_SSAPC] post task fail in NLSTK_SsapClientDeregApp");
    return;
}

NLSTK_Errcode_E NLSTK_SsapClientSetInteractionTimeout(int32_t appId, int64_t timeout)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientSetInteractionTimeout");
    NLSTK_CHECK_RETURN(appId >= 0 && appId < NLSTK_SSAP_CLIENT_APP_MAX_NUM &&
        timeout >= SSAP_INTERACTION_MIN_TIMEOUT && timeout <= SSAP_INTERACTION_MAX_TIMEOUT,
        NLSTK_ERRCODE_PARAM_ERR, "[NLSTK_SSAPC] param invalid");

    SsapcAppTimeoutParam_S *param = (SsapcAppTimeoutParam_S *)SDF_MemZalloc(sizeof(SsapcAppTimeoutParam_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc param fail");
    param->appId = appId;
    param->timeout = timeout;
    uint32_t taskRet = SchedulePostTask(SsapcAppSetInteractionTimeout, (void *)param, SDF_MemFree);
    NLSTK_CHECK_RETURN(taskRet == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[NLSTK_SSAPC] post task fail in NLSTK_SsapClientSetInteractionTimeout");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientExchangeMtu(int32_t appId, uint16_t mtu)
{
    NLSTK_LOG_INFO("[NLSTK_SSAPC] enter func NLSTK_SsapClientExchangeMtu");
    SSAP_ExchangeInfoReqInfo_S *exchangeInfo = (SSAP_ExchangeInfoReqInfo_S*)SDF_MemZalloc(
        sizeof(SSAP_ExchangeInfoReqInfo_S));
    NLSTK_CHECK_RETURN(exchangeInfo, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] memory alloc fail");
    exchangeInfo->appId = appId;
    exchangeInfo->mtu = mtu;
    uint32_t ret = SchedulePostTask(SsapcAppExchangeMtu, (void *)exchangeInfo, SDF_MemFree);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[NLSTK_SSAPC] post task fail in NLSTK_SsapClientExchangeMtu");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientDiscoverServices(int32_t appId, uint16_t handleStart, uint16_t handleEnd, uint8_t type)
{
    NLSTK_LOG_INFO("[NLSTK_SSAPC] enter func NLSTK_SsapClientDiscoverServices");

    /* 申请内存 */
    NLSTK_ParamFind_S *param = (NLSTK_ParamFind_S *)SDF_MemZalloc(sizeof(NLSTK_ParamFind_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc appId param failed");

    /* 初始化param结构体 */
    param->appId = appId;
    /* 当前业务需要发现从0x0001-0xFFFF的所有服务，因此这里暂不支持自定义服务发现的句柄范围 */
    param->startHandle = SSAP_START_HANDLE;
    param->endHandle = SSAP_END_HANDLE;
    param->type = type;

    /* 调用schedule函数生成异步任务 */
    uint32_t ret = SchedulePostTask(SsapcAppDiscServ, (void *)param, SDF_MemFree);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[NLSTK_SSAPC] post task fail in NLSTK_SsapClientDiscoverServices");

    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientDiscoverServicesByUuid
(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handleStart, uint16_t handleEnd, uint8_t type)
{
    NLSTK_LOG_INFO("[NLSTK_SSAPC] enter func NLSTK_SsapClientDiscoverServicesByUuid");
    NLSTK_CHECK_RETURN(uuid, NLSTK_ERRCODE_POINTER_NULL, "[NLSTK_SSAPC] paramFind is null");

    /* 申请内存 */
    NLSTK_ParamFindByUuid_S *param = (NLSTK_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(NLSTK_ParamFindByUuid_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc appId param failed");

    /* 初始化param结构体 */
    param->appId = appId;
    param->uuid = *uuid;
    /* 当前业务需要发现从0x0001-0xFFFF的所有服务，因此这里暂不支持自定义服务发现的句柄范围 */
    param->startHandle = SSAP_START_HANDLE;
    param->endHandle = SSAP_END_HANDLE;
    if (type == FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE) {
        param->type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
    } else {
        param->type = type;
    }

    /* 调用schedule函数生成异步任务 */
    uint32_t ret = SchedulePostTask(SsapcAppDiscServByUuid, (void *)param, SDF_MemFree);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[NLSTK_SSAPC] post task fail in NLSTK_SsapClientDiscoverServicesByUuid");

    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientGetServices
(int32_t appId, NLSTK_SsapServ_S **service, uint16_t *serviceNum, NLSTK_SsapClientFreeFunc *func)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientGetServices");
    NLSTK_CHECK_RETURN(service && serviceNum && func, NLSTK_ERRCODE_POINTER_NULL, "[NLSTK_SSAPC] input param is null");

    /* 申请内存 */
    NLSTK_SsapServObtain_S *param = (NLSTK_SsapServObtain_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServObtain_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc param fail");

    /* 初始化param结构体 */
    param->appId = appId;
    param->serv = service;
    param->num = serviceNum;
    param->func = func;
    param->uuid = NULL;

    uint32_t ret = SchedulePostTaskBlocked(SsapcAppGetServ, (void *)param, SDF_MemFree, NLSTK_API_TIME_OUT);
    // 改成阻塞调用后异常逻辑未考虑
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[NLSTK_SSAPC] post task fail for NLSTK_SsapClientGetServices");

    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientGetServicesAsyn(int32_t appId)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientGetServicesAsyn");
    NLSTK_SsapServObtainAsyn_S *param = (NLSTK_SsapServObtainAsyn_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServObtainAsyn_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc param fail");
    param->appId = appId;

    uint32_t ret = SchedulePostTask(SsapcAppGetServAsyn, (void *)param, FreeServObtainAsyn);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[NLSTK_SSAPC] post task fail for NLSTK_SsapClientGetServicesAsyn");

    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientGetServicesByUuid(int32_t appId, NLSTK_SsapUuid_S *uuid,
    NLSTK_SsapServ_S **service, uint16_t *serviceNum, NLSTK_SsapClientFreeFunc *func)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientGetServicesByUuid");
    NLSTK_CHECK_RETURN(service && serviceNum && func, NLSTK_ERRCODE_POINTER_NULL, "[NLSTK_SSAPC] input param is null");

    /* 申请内存 */
    NLSTK_SsapServObtain_S *param = (NLSTK_SsapServObtain_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServObtain_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc param fail");

    /* 初始化param结构体 */
    param->appId = appId;
    param->serv = service;
    param->num = serviceNum;
    param->func = func;
    param->uuid = uuid;

    /* 调用schedule函数生成异步任务 */
    uint32_t ret = SchedulePostTaskBlocked(SsapcAppGetServ, (void *)param, SDF_MemFree, NLSTK_API_TIME_OUT);
    // 改成阻塞调用后异常逻辑未考虑
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[NLSTK_SSAPC] post task fail for NLSTK_SsapClientGetServicesByUuid");

    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientGetServicesByUuidAsyn(int32_t appId, NLSTK_SsapUuid_S *uuid)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientGetServicesByUuidAsyn");

    /* 申请内存 */
    NLSTK_SsapServObtainAsyn_S *param = (NLSTK_SsapServObtainAsyn_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServObtainAsyn_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc param fail");
    param->uuid = (NLSTK_SsapUuid_S *)SDF_MemZalloc(sizeof(NLSTK_SsapUuid_S));
    if (param->uuid == NULL) {
        NLSTK_LOG_ERROR("[NLSTK_SSAPC] param uuid malloc fail");
        SDF_MemFree(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }

    /* 初始化param结构体 */
    param->appId = appId;
    (void)memcpy_s(param->uuid, sizeof(NLSTK_SsapUuid_S), uuid, sizeof(NLSTK_SsapUuid_S));

    /* 调用schedule函数生成异步任务 */
    uint32_t ret = SchedulePostTask(SsapcAppGetServAsyn, (void *)param, FreeServObtainAsyn);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[NLSTK_SSAPC] post task fail for NLSTK_SsapClientGetServicesByUuidAsyn");

    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E SsapClientRead(int32_t appId, uint16_t handle, uint8_t type)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func SsapClientReadCommon");

    /* 申请内存 */
    NLSTK_ReadReqInfo_S *param = (NLSTK_ReadReqInfo_S *)SDF_MemZalloc(sizeof(NLSTK_ReadReqInfo_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc info fail");

    /* 初始化param结构体 */
    param->appId = appId;
    param->handle = handle;
    param->type = type;

    /* 调用schedule函数生成异步任务 */
    uint32_t ret = SchedulePostTask(SsapcAppRead, (void *)param, SDF_MemFree);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL, "[NLSTK_SSAPC] post task fail");

    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E SsapClientReadProps(int32_t appId, uint16_t *handles, uint8_t num, uint8_t type)
{
    NLSTK_ReadPropsInfo_S *param = (NLSTK_ReadPropsInfo_S *)SDF_MemZalloc(sizeof(NLSTK_ReadPropsInfo_S) +
        num * sizeof(uint16_t));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc info fail");

    param->appId = appId;
    param->type = type;
    param->num = num;
    (void)memcpy_s(param->handles, num * sizeof(uint16_t), handles, num * sizeof(uint16_t));

    uint32_t ret = SchedulePostTask(SsapcAppReadProps, (void *)param, SDF_MemFree);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL, "[NLSTK_SSAPC] post task fail");

    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E SsapClientReadByUuid
(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handleStart, uint16_t handleEnd, uint8_t type)
{
    NLSTK_LOG_INFO("[NLSTK_SSAPC] enter func SsapClientReadByUuid");

    /* 申请内存 */
    NLSTK_ReadByUuidReqInfo_S *param = (NLSTK_ReadByUuidReqInfo_S *)SDF_MemZalloc(sizeof(NLSTK_ParamFindByUuid_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc info-uuid fail");

    /* 初始化param结构体 */
    param->appId = appId;
    param->uuid = *uuid;
    param->handleStart = handleStart;
    param->handleEnd = handleEnd;
    param->dataType = type;

    /* 调用schedule函数生成异步任务 */
    uint32_t ret = SchedulePostTask(SsapcAppReadByUuid, (void *)param, SDF_MemFree);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL, "[NLSTK_SSAPC] post task fail");

    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientReadProperty(int32_t appId, uint16_t handle)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientReadProperty, handle is %d, appId is %d", handle, appId);
    return SsapClientRead(appId, handle, DESC_TYPE_PROPERTY_RESERVE);
}

NLSTK_Errcode_E NLSTK_SsapClientReadProperties(int32_t appId, uint16_t *handles, uint8_t num)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientReadProperties, appId is %d", appId);
    NLSTK_CHECK_RETURN(handles != NULL, NLSTK_ERRCODE_POINTER_NULL, "[NLSTK_SSAPC] input handles is null");
    NLSTK_CHECK_RETURN(num > 1, NLSTK_ERRCODE_PARAM_ERR, "[NLSTK_SSAPC] input num error");
    return SsapClientReadProps(appId, handles, num, DESC_TYPE_PROPERTY_RESERVE);
}

NLSTK_Errcode_E NLSTK_SsapClientReadPropertyByUuid
(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handleStart, uint16_t handleEnd)
{
    NLSTK_LOG_INFO("[NLSTK_SSAPC] enter func NLSTK_SsapClientReadPropertyByUuid");
    return SsapClientReadByUuid(appId, uuid, handleStart, handleEnd, DESC_TYPE_PROPERTY_RESERVE);
}

NLSTK_Errcode_E NLSTK_SsapClientReadDescriptor(int32_t appId, uint16_t handle, uint8_t type)
{
    NLSTK_LOG_INFO("[NLSTK_SSAPC] enter func NLSTK_SsapClientReadDescriptor");
    return SsapClientRead(appId, handle, type);
}

static NLSTK_Errcode_E SsapClientGetCpcd(int32_t appId, uint16_t handle, bool isNtf)
{
    NLSTK_LOG_INFO("[NLSTK_SSAPC] enter func SsapClientGetCpcd");
    NLSTK_SsapClientGetPropertyInfo_S *param =
        (NLSTK_SsapClientGetPropertyInfo_S *)SDF_MemZalloc(sizeof(NLSTK_SsapClientGetPropertyInfo_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc param fail");

    param->appId = appId;
    param->handle = handle;
    param->isNtf = isNtf;

    uint32_t ret = SchedulePostTask(SsapcAppGetCpcd, (void *)param, SDF_MemFree);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL, "[NLSTK_SSAPC] post task fail");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientGetPropertyNtf(int32_t appId, uint16_t handle)
{
    NLSTK_LOG_INFO("[NLSTK_SSAPC] enter func NLSTK_SsapClientGetPropertyNtf");
    return SsapClientGetCpcd(appId, handle, true);
}

NLSTK_Errcode_E NLSTK_SsapClientGetPropertyInd(int32_t appId, uint16_t handle)
{
    NLSTK_LOG_INFO("[NLSTK_SSAPC] enter func NLSTK_SsapClientGetPropertyInd");
    return SsapClientGetCpcd(appId, handle, false);
}

static NLSTK_Errcode_E SsapClientSetCpcd(int32_t appId, uint16_t handle, bool enable, bool isNtf)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func SsapClientSetCpcd");
    NLSTK_SsapClientSetPropertyInfo_S *param =
        (NLSTK_SsapClientSetPropertyInfo_S *)SDF_MemZalloc(sizeof(NLSTK_SsapClientSetPropertyInfo_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc param fail");

    param->appId = appId;
    param->handle = handle;
    param->isNtf = isNtf;
    param->enable = enable;

    uint32_t ret = SchedulePostTask(SsapcAppSetCpcd, (void *)param, SDF_MemFree);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL, "[NLSTK_SSAPC] post task fail");
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientSetPropertyNtf(int32_t appId, uint16_t handle, bool enable)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientSetPropertyNtf");
    return SsapClientSetCpcd(appId, handle, enable, true);
}
NLSTK_Errcode_E NLSTK_SsapClientSetPropertyInd(int32_t appId, uint16_t handle, bool enable)
{
    NLSTK_LOG_INFO("[NLSTK_SSAPC] enter func NLSTK_SsapClientSetPropertyInd");
    return SsapClientSetCpcd(appId, handle, enable, false);
}

static void FreeClientWriteBaseInfo(void *arg)
{
    if (arg == NULL) {
        return;
    }
    NLSTK_SsapClientWriteBaseInfo_S *param = (NLSTK_SsapClientWriteBaseInfo_S *)arg;
    if (param->value != NULL) {
        if (param->value->data != NULL) {
            SDF_MemFree(param->value->data);
        }
        SDF_MemFree(param->value);
    }
    SDF_MemFree(param);
}

static NLSTK_Errcode_E SsapClientWrite
(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value, uint8_t type, bool withoutRsp)
{
    NLSTK_CHECK_RETURN(value && value->data && value->len > 0, NLSTK_ERRCODE_POINTER_NULL,
        "[NLSTK_SSAPC] input param is null");

    /* 申请内存 */
    NLSTK_SsapClientWriteBaseInfo_S *param =
        (NLSTK_SsapClientWriteBaseInfo_S *)SDF_MemZalloc(sizeof(NLSTK_SsapClientWriteBaseInfo_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc writeinfo fail");

    NLSTK_VariableData_S *lengthValue =
        (NLSTK_VariableData_S *)SDF_MemZalloc(sizeof(NLSTK_VariableData_S));
    if (lengthValue == NULL) {
        SDF_MemFree(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    uint16_t dataLen = value->len;
    lengthValue->data = (uint8_t *)SDF_MemZalloc(dataLen);
    if (lengthValue->data == NULL) {
        SDF_MemFree(lengthValue);
        SDF_MemFree(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    lengthValue->len = dataLen;
    (void)memcpy_s(lengthValue->data, dataLen, value->data, dataLen);

    /* 初始化param结构体 */
    param->appId = appId;
    param->handle = handle;
    param->value = lengthValue;
    param->type = type;

    /* 调用schedule函数生成异步任务 */
    uint32_t ret = NLSTK_ERR;
    if (withoutRsp) {
        ret = SchedulePostTask(SsapcAppWriteCmd, (void *)param, FreeClientWriteBaseInfo);
    } else {
        ret = SchedulePostTask(SsapcAppWriteReq, (void *)param, FreeClientWriteBaseInfo);
    }
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL, "[NLSTK_SSAPC] post task fail in SsapClientWrite");

    return NLSTK_ERRCODE_SUCCESS;
}

static void FreeClientCallMethodInfo(void *arg)
{
    if (arg == NULL) {
        return;
    }
    NLSTK_SsapClientCallMethodInfo_S *param = (NLSTK_SsapClientCallMethodInfo_S *)arg;
    if (param->value.data != NULL) {
        SDF_MemFree(param->value.data);
    }
    SDF_MemFree(param);
}

static NLSTK_Errcode_E SsapClientCallMethod
(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value, bool withoutRsp)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func SsapClientCallMethod");
    NLSTK_CHECK_RETURN(value, NLSTK_ERRCODE_POINTER_NULL, "[NLSTK_SSAPC] input param is null");

    /* 申请内存 */
    NLSTK_SsapClientCallMethodInfo_S *param =
        (NLSTK_SsapClientCallMethodInfo_S *)SDF_MemZalloc(sizeof(NLSTK_SsapClientCallMethodInfo_S));
    NLSTK_CHECK_RETURN(param, NLSTK_ERRCODE_MALLOC_FAIL, "[NLSTK_SSAPC] malloc writeinfo fail");

    /* 初始化param结构体 */
    param->appId = appId;
    param->handle = handle;
    param->value.len = value->len;
    param->value.data = (uint8_t *)SDF_MemZalloc(param->value.len);
    if (param->value.data == NULL) {
        SDF_MemFree(param);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    (void)memcpy_s(param->value.data, value->len, value->data, value->len);

    /* 调用schedule函数生成异步任务 */
    uint32_t ret = NLSTK_ERR;
    if (withoutRsp) {
        ret = SchedulePostTask(SsapcAppCallMethodCmd, (void *)param, FreeClientCallMethodInfo);
    } else {
        ret = SchedulePostTask(SsapcAppCallMethodReq, (void *)param, FreeClientCallMethodInfo);
    }
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL, "[NLSTK_SSAPC] post task fail in SsapClientWrite");

    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E NLSTK_SsapClientWriteProperty
(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value, bool withoutRsp)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientWriteProperty");
    return SsapClientWrite(appId, handle, value, DESC_TYPE_PROPERTY_RESERVE, withoutRsp);
}

NLSTK_Errcode_E NLSTK_SsapClientWriteDescriptor
(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value, uint8_t type, bool withoutRsp)
{
    NLSTK_LOG_INFO("[NLSTK_SSAPC] enter func NLSTK_SsapClientWriteDescriptor");
    return SsapClientWrite(appId, handle, value, type, withoutRsp);
}

NLSTK_Errcode_E NLSTK_SsapClientCallMethod
(int32_t appId, uint16_t handle, NLSTK_VariableData_S *value, bool withoutRsp)
{
    NLSTK_LOG_DEBUG("[NLSTK_SSAPC] enter func NLSTK_SsapClientCallMethod");
    return SsapClientCallMethod(appId, handle, value, withoutRsp);
}

NLSTK_Errcode_E NLSTK_SsapCleanClientApp(NLSTK_SsapClientCleanAppResultCb clientCleanAppResultCb)
{
    // 这里不需要释放内存，因为传入的是一个函数指针，该地址在整个系统生命周期内都是固定的，因此不需要进行释放
    uint32_t ret = SchedulePostTask(SsapcClientCleanApp, (void *)clientCleanAppResultCb, NULL);
    NLSTK_CHECK_RETURN(ret == NLSTK_OK, NLSTK_ERRCODE_TASK_FAIL,
                         "[NLSTK_SSAPC] post task fail in NLSTK_SsapCleanClientApp");
    return NLSTK_ERRCODE_SUCCESS;
}