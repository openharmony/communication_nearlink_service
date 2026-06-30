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
#include "ssapc_app.h"
#include "sdf_mem.h"
#include "cpfwk_log.h"
#include "nlstk_log.h"
#include "nlstk_cfgdb.h"
#include "ssapc_client_api.h"
#include "ssap_manager.h"
#include "ssap_common.h"
#include "ssapc_cache.h"
#include "securec.h"
#include "ssap_link_state.h"
#include "ssap_utils.h"
#include "ssapc_app_util.h"
#include "ssap_pkt.h"

#define SSAP_BIT_POSITION 2

#define SSAPC_SERVICE_CHANGE_APPID 101 // 服务变更事件专用appId，区别于可申请的appId0-49

SsapcAppRegParam_S *g_regList[NLSTK_SSAP_CLIENT_APP_MAX_NUM] = { 0 };
NLSTK_SsapClientCleanAppResultCb g_cleanAppResult = NULL;

static void SsapcAppDiscServCompCb(int32_t appId, void *arg);

static int GetAppId(SsapcAppRegParam_S *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppInsertList");
    for (int i = 0; i < NLSTK_SSAP_CLIENT_APP_MAX_NUM; i++) {
        if (!g_regList[i]) {
            SsapcAppRegParam_S *newApp = (SsapcAppRegParam_S *)SDF_MemZalloc(sizeof(SsapcAppRegParam_S));
            NLSTK_CHECK_RETURN(newApp, SSAP_APP_INVALID_ID, "[SSAPC_APP] malloc param fails");
            (void)memcpy_s(newApp, sizeof(SsapcAppRegParam_S), param, sizeof(SsapcAppRegParam_S));
            g_regList[i] = newApp;
            g_regList[i]->cb = SsapcCbGet(param->cb);
            if (!g_regList[i]->cb) {
                CP_LOG_ERROR("[SSAPC_APP] get service from cache fail");
                SDF_MemFree(newApp);
                g_regList[i] = NULL;
                return SSAP_APP_INVALID_ID;
            }
            return i;
        }
    }
    CP_LOG_ERROR("[SSAPC_APP] client appId has been run out");
    return SSAP_APP_INVALID_ID;
}

SLE_Addr_S* GetAddrByAppId(int32_t appId)
{
    CP_CHECK_LOG_RETURN(IsAppIdValid(appId), NULL, "[SSAPC_APP] appId is invalid");
    return &g_regList[appId]->addr;
}

NLSTK_ConnParam_S* GetConnParamByAppId(int32_t appId)
{
    CP_CHECK_LOG_RETURN(IsAppIdValid(appId), NULL, "[SSAPC_APP] appId is invalid");
    return &g_regList[appId]->connParam;
}

bool IsAppIdValid(int32_t appId)
{
    if (appId == SSAPC_SERVICE_CHANGE_APPID) {
        return true;
    }
    if (appId < 0 || appId >= NLSTK_SSAP_CLIENT_APP_MAX_NUM) {
        return false;
    }
    return (g_regList[appId] != NULL);
}

// 此函数仅提供状态调整的基础函数，建议外部确保appId有效性后再调用，确保状态更新的准确性
void SsapcAppSetLinkState(int32_t appId, NLSTK_SsapConnectLinkState_E newState)
{
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] appId is invalid");
    g_regList[appId]->linkState = newState;
    return;
}

// 此函数仅提供状态调整的基础函数，建议外部确保appId有效性后再调用，确保状态更新的准确性
NLSTK_SsapConnectLinkState_E SsapcAppGetLinkState(int32_t appId)
{
    CP_CHECK_LOG_RETURN(IsAppIdValid(appId), SSAP_CONNECT_STATE_IDLE, "[SSAPC_APP] appId is invalid");
    return g_regList[appId]->linkState;
}

NLSTK_SsapClientLinkChangeEvent_E SsapcGetLinkOper(int32_t appId)
{
    NLSTK_CHECK_RETURN(IsAppIdValid(appId), SSAP_LINK_CHANGE_EVENT_BUTT, "[SSAPC_APP] appId is invalid");
    return g_regList[appId]->nextOperator;
}

void SsapcClearLinkOper(int32_t appId)
{
    NLSTK_CHECK_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] appId is invalid");
    g_regList[appId]->nextOperator = SSAP_LINK_CHANGE_EVENT_BUTT;
    return;
}

void SsapcCacheLinkOper(int32_t appId, NLSTK_SsapClientLinkChangeEvent_E oper)
{
    NLSTK_CHECK_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] appId is invalid");
    g_regList[appId]->nextOperator = oper;
    return;
}

uint32_t SsapcAppGetAppIdListByAddr(SLE_Addr_S *addr, int32_t *appIdList, uint32_t number)
{
    uint32_t index = 0;
    NLSTK_CHECK_RETURN(addr != NULL, index, "addr is null point in SsapcAppGetAppIdListByAddr");
    for (uint32_t i = 0; i < NLSTK_SSAP_CLIENT_APP_MAX_NUM && i < number; i++) {
        if ((g_regList[i] != NULL) && SDF_CompareSleAddr(&g_regList[i]->addr, addr) == 0) {
            appIdList[index] = (int32_t)i;
            index++;
        }
    }
    return index;
}

void SsapcAppLinkStateNofity(int32_t appId, NLSTK_SsapConnectLinkState_E state, NLSTK_Errcode_E errCode, int32_t reason)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppLinkStateNofity, appId: %d, state: %d, errCode: %d, reason: %d", appId,
                state, errCode, reason);
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] appId is invalid");
    NLSTK_SsapAppClientCb_S *cb = g_regList[appId]->cb;
    if (cb == NULL) {
        CP_LOG_INFO("the callback pinter of appId(%d) is null", appId);
        return;
    }
    if (cb->onConnectionStateChanged != NULL) {
        cb->onConnectionStateChanged(appId, state, errCode, reason);
    }
}

void SsapcAppNotifyClientCleanUp(void)
{
    int32_t appId = 0;
    for (appId = 0; appId < NLSTK_SSAP_CLIENT_APP_MAX_NUM; appId++) {
        if (g_regList[appId] == NULL) {
            continue;
        }
        if (g_regList[appId]->linkState == SSAP_CONNECT_STATE_DISCONNECTED ||
            g_regList[appId]->linkState == SSAP_CONNECT_STATE_IDLE) {
            NLSTK_LOG_INFO("clean up client appId(%d) when state change", appId);
            SsapcCbDestroy(g_regList[appId]->cb);
            SDF_MemFree(g_regList[appId]);
            g_regList[appId] = NULL;
            continue;
        }
        break;
    }
    if (appId == NLSTK_SSAP_CLIENT_APP_MAX_NUM) {
        NLSTK_LOG_INFO("clean up all client appId and notify service");
        if (g_cleanAppResult != NULL) {
            g_cleanAppResult();
        }
        SsapResetClientCleanUp();
    }
}

void SsapcAppRegister(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppRegister");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppRegister");
    SsapcAppRegParam_S *wanted = (SsapcAppRegParam_S *)param;
    wanted->appId = GetAppId(wanted);
    CP_LOG_INFO("[SSAPC_APP] SsapcAppRegister finish, appId: %d, addr:%s", wanted->appId,
                GET_ENC_ADDR(&(wanted->addr)));
}

void SsapcAppRegisterAsync(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppRegisterAsync");
    SsapcAppRegParam_S *wanted = (SsapcAppRegParam_S *)param;
    CP_CHECK_LOG_RETURN_VOID(wanted && wanted->cb, "[SSAPC_APP] param is null in SsapcAppRegisterAsync");
    if (wanted->cb->onRegisterApp) {
        wanted->appId = GetAppId(wanted);
        CP_LOG_INFO("[SSAPC_APP] SsapcAppRegister success, appId: %d, addr:%s",
            wanted->appId, GET_ENC_ADDR(&(wanted->addr)));
        wanted->cb->onRegisterApp(wanted->appId, &wanted->addr, NLSTK_ERRCODE_SUCCESS);
    }
}

void SsapcAppDeregister(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppDeregister");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppDeregister");
    SsapcAppRegParam_S *wanted = (SsapcAppRegParam_S *)param;
    CP_LOG_INFO("[SSAPC_APP] SsapcAppDeregister deregister appId is %d", wanted->appId);
    int32_t appId = wanted->appId;
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] appId is invalid");
    SsapcCacheCleanAppCpcd(appId, &(g_regList[appId]->addr));
    SsapcCbDestroy(g_regList[appId]->cb);
    SSAP_CleanTaskByAppId(&(g_regList[appId]->addr), appId);
    SDF_MemFree(g_regList[appId]);
    g_regList[appId] = NULL;
}

void SsapcAppSetInteractionTimeout(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppSetInteractionTimeout");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppSetInteractionTimeout");
    SsapcAppTimeoutParam_S *timeoutParam = (SsapcAppTimeoutParam_S *)param;
    CP_LOG_DEBUG("[SSAPC_APP] SsapcAppSetInteractionTimeout appId is %d, timeout is %d", timeoutParam->appId,
        timeoutParam->timeout);
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(timeoutParam->appId), "[SSAPC_APP] appId is invalid");
    g_regList[timeoutParam->appId]->timeout = timeoutParam->timeout;
}

static void SsapcServiceRediscoverCb(SLE_Addr_S *addr, SSAP_PduErrCode_E errCode)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcServiceRediscoverCb");
    NLSTK_SsapServ_S* services = NULL;
    uint16_t num = 0;
    NLSTK_Errcode_E ret = SsapcCacheGetServices(addr, SSAPC_SERVICE_CHANGE_APPID, NULL, &services, &num);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        CP_LOG_ERROR("[SSAPC_APP] SsapcServiceRediscoverCb get services failed");
        return;
    }
    for (int32_t appId = 0; appId < NLSTK_SSAP_CLIENT_APP_MAX_NUM; appId++) {
        if (g_regList[appId] == NULL || memcmp(&g_regList[appId]->addr, addr, SLE_ADDR_LEN) != 0 ||
            g_regList[appId]->cb == NULL) {
            continue;
        }
        NLSTK_SsapClientServiceRediscoverCb hook = g_regList[appId]->cb->onServiceRediscover;
        if (hook != NULL) {
            hook(appId, services, num);
        }
    }
    SsapcCacheFreeServices(services, num);
}

static void SsapcAppDiscCompleteCb(int32_t appId, SLE_Addr_S *addr, SSAP_PduErrCode_E errCode)
{
    if (appId == SSAPC_SERVICE_CHANGE_APPID) {
        SsapcServiceRediscoverCb(addr, errCode);
        return;
    }
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");
    bool isByUuid = false;
    NLSTK_SsapUuid_S uuid = {0};
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_FAIL;
    if (errCode == SSAP_ERRCODE_SUCCESS) {
        ret = NLSTK_ERRCODE_SUCCESS;
    } else {
        CP_LOG_ERROR("[SSAPC_APP] SsapcAppDiscCompleteCb SSAP_errCode is %d", errCode);
        ret = NLSTK_ERRCODE_FAIL;
    }
    SsapcCacheIsCurFindByUuid(addr, &isByUuid, &uuid);
    if (!isByUuid) {
        NLSTK_SsapClientFindServiceCb hook = g_regList[appId]->cb->onFindService;
        CP_CHECK_LOG_RETURN_VOID(hook, "NLSTK_SsapClientFindServiceCb hook is null");
        hook(appId, ret);
    } else {
        NLSTK_SsapClientFindServicesByUuidCb hook = g_regList[appId]->cb->onFindServiceByUuid;
        CP_CHECK_LOG_RETURN_VOID(hook, "NLSTK_SsapClientFindServicesByUuidCb hook is null");
        hook(appId, &uuid, ret);
    }
}

static int64_t GetTimeOutByAppId(int32_t appId)
{
    if (appId >= NLSTK_SSAP_CLIENT_APP_MAX_NUM) {
        return SSAP_INTERACTION_DEFAULT_TIMEOUT;
    }
    return g_regList[appId]->timeout;
}

static void SsapcFindNextMember(int32_t appId, SLE_Addr_S *addr)
{
    bool isFinish = false;
    SSAP_FindType_E findType = FIND_STRUCTURE_TYPE_PROPERTY;
    uint16_t startHandle = 0;
    uint16_t endHandle = 0;
    SsapcCacheGetNextFindMember(addr, &isFinish, &findType, &startHandle, &endHandle);
    if (!isFinish) {
        SSAP_ParamFind_S findParam = {0};
        findParam.type = findType;
        findParam.startHandle = startHandle;
        findParam.endHandle = endHandle;
        (void)memcpy_s(&findParam.addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        SSAP_FindPriorityReq(appId, GetTimeOutByAppId(appId), SsapcAppDiscServCompCb, &findParam);
    } else {
        SsapcCacheServDiscFinish(addr);
        SsapcAppDiscCompleteCb(appId, addr, SSAP_ERRCODE_SUCCESS);
    }
}

static void SsapcAppDiscServByHandleCompCb(int32_t appId, SSAP_DiscoveryComplete_S *complete)
{
    // 非成功，说明有异常，直接callback返回，并且清除已有的serv
    if ((complete->errCode != SSAP_ERRCODE_SUCCESS && complete->errCode != SSAP_ERRCODE_ITEM_INEXIST)) {
        SsapcCacheCleanServ(&complete->addr);
        SsapcAppDiscCompleteCb(appId, &complete->addr, complete->errCode);
        return;
    }

    if (complete->type == FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE) {
        SsapcCacheServDiscFinish(&complete->addr);
        // 通过注册的appid查找对应的上层回调钩子通知服务发现结果
        SsapcAppDiscCompleteCb(appId, &complete->addr, SSAP_ERRCODE_SUCCESS);
        return;
    } else if (complete->type == FIND_STRUCTURE_TYPE_PRIMARY_SERVICE) {
        uint16_t maxHandle = SsapcCacheGetMaxServHandle(&complete->addr);
        // maxHandle为0说明find未查询到服务，直接回调未找到
        if (maxHandle == 0) {
            SsapcAppDiscCompleteCb(appId, &complete->addr, SSAP_ERRCODE_ITEM_INEXIST);
            return;
        }
        // 说明服务全查完了，继续查属性
        if (complete->errCode == SSAP_ERRCODE_ITEM_INEXIST || maxHandle == SSAP_END_HANDLE) {
            SsapcFindNextMember(appId, &complete->addr);
        } else {
            // 继续查服务
            SSAP_ParamFind_S findParam = {0};
            findParam.type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
            findParam.startHandle = maxHandle + 1;
            findParam.endHandle = SSAP_END_HANDLE;
            (void)memcpy_s(&findParam.addr, sizeof(SLE_Addr_S), &complete->addr, sizeof(SLE_Addr_S));
            SSAP_FindPriorityReq(appId, GetTimeOutByAppId(appId), SsapcAppDiscServCompCb, &findParam);
        }
        return;
    }
    if (complete->errCode == SSAP_ERRCODE_ITEM_INEXIST) {
        // 说明当前服务下的对应type的member都查完了
        SsapcCacheServMemberDiscFinish(&complete->addr, complete->type, complete->preFindHandle);
    }
    SsapcFindNextMember(appId, &complete->addr);
}

static void SsapcAppDiscServByUuidCompCb(int32_t appId, SSAP_DiscoveryComplete_S *complete)
{
    // 非成功，说明有异常，直接callback返回，并且清除已有的serv
    if ((complete->errCode != SSAP_ERRCODE_SUCCESS && complete->errCode != SSAP_ERRCODE_ITEM_INEXIST)) {
        SsapcCacheCleanServ(&complete->addr);
        SsapcAppDiscCompleteCb(appId, &complete->addr, complete->errCode);
        return;
    }

    if (complete->type == FIND_STRUCTURE_TYPE_PRIMARY_SERVICE) {
        uint16_t maxHandle = SsapcCacheGetMaxServHandle(&complete->addr);
        // maxHandle为0说明find未查询到服务，直接回调未找到
        if (maxHandle == 0) {
            SsapcAppDiscCompleteCb(appId, &complete->addr, SSAP_ERRCODE_ITEM_INEXIST);
            return;
        }
        // 说明服务全查完了，继续查属性
        if (complete->errCode == SSAP_ERRCODE_ITEM_INEXIST || maxHandle == SSAP_END_HANDLE) {
            SsapcFindNextMember(appId, &complete->addr);
        } else {
            // 继续查服务
            SSAP_ParamFindByUuid_S findParam = {0};
            findParam.type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
            findParam.startHandle = maxHandle + 1;
            findParam.endHandle = SSAP_END_HANDLE;
            (void)memcpy_s(&findParam.addr, sizeof(SLE_Addr_S), &complete->addr, sizeof(SLE_Addr_S));
            (void)memcpy_s(&findParam.uuid, sizeof(NLSTK_SsapUuid_S), &complete->uuid, sizeof(NLSTK_SsapUuid_S));
            SSAP_FindByUuidPriorityReq(appId, g_regList[appId]->timeout, SsapcAppDiscServCompCb,
                &findParam);
        }
        return;
    }
    if (complete->errCode == SSAP_ERRCODE_ITEM_INEXIST) {
        // 说明当前服务下的对应type的member都查完了
        SsapcCacheServMemberDiscFinish(&complete->addr, complete->type, complete->preFindHandle);
    }
    SsapcFindNextMember(appId, &complete->addr);
}

static void SsapcAppDiscServCompCb(int32_t appId, void *arg)
{
    SSAP_DiscoveryComplete_S *complete = (SSAP_DiscoveryComplete_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(complete, "[SSAPC_APP] complete is null");
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");
    switch (complete->opCode) {
        case SSAP_FIND_STRUCTURE_REQ :
            SsapcAppDiscServByHandleCompCb(appId, complete);
            break;
        case SSAP_FIND_STRUCTURE_BY_UUID_REQ :
            SsapcAppDiscServByUuidCompCb(appId, complete);
            break;
        default:
            CP_LOG_ERROR("[SSAPC_APP] unknown type %d", complete->opCode);
            return;
    }
    return;
}

void SsapcAppDiscServ(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppDiscServ");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppDiscServ");
    SSAP_ParamFind_S *findParam = (SSAP_ParamFind_S *)param;

    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(findParam->appId), "[SSAPC_APP] appId is invalid");
    SLE_Addr_S *addr = GetAddrByAppId(findParam->appId);
    (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    (void)findParam->startHandle;
    (void)findParam->endHandle;
    if (!CfgdbGetManufacturerSupport(addr, CFGDB_FIND_SERVICE_STRUCTURE) &&
            findParam->type == FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE) {
        CP_LOG_INFO("[SSAPC_APP] not support find structure, enter find primary service");
        findParam->type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
    }
    CP_LOG_INFO("[SSAPC_APP] SsapcAppDiscServ appId is %d, start handle %d, end handle %d, type %d",
        findParam->appId, findParam->startHandle, findParam->endHandle, findParam->type);
    SSAP_FindReq(findParam->appId, g_regList[findParam->appId]->timeout, SsapcAppDiscServCompCb, findParam);
}

void SsapcAppDiscServByUuid(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppDiscServByUuid");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppDiscServByUuid");
    SSAP_ParamFindByUuid_S *findParam = (SSAP_ParamFindByUuid_S *)param;
    CP_LOG_INFO("[SSAPC_APP] SsapcAppDiscServ appId is %d, start handle %d, end handle %d, type %d, uuid %s",
        findParam->appId, findParam->startHandle, findParam->endHandle, findParam->type,
        SSAP_GET_ENC_UUID(&findParam->uuid));
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(findParam->appId), "[SSAPC_APP] appId is invalid");
    SLE_Addr_S *addr = GetAddrByAppId(findParam->appId);
    (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    (void)findParam->startHandle;
    (void)findParam->endHandle;
    SSAP_FindByUuidReq(findParam->appId, g_regList[findParam->appId]->timeout, SsapcAppDiscServCompCb,
        findParam);
}

void SsapcAppServChange(SLE_Addr_S *addr, uint16_t startHandle, uint16_t endHandle)
{
    SDF_UNUSED(endHandle);
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppServChange");
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[SSAPC_APP] addr is null");
    NLSTK_SsapUuid_S uuid = {0};
    SsapcCacheGetUuidByHandle(addr, &uuid, startHandle);
    NLSTK_SsapUuid_S zeroUuid = {0};
    if (memcmp(&uuid, &zeroUuid, sizeof(NLSTK_SsapUuid_S)) != 0) {
        for (int32_t appId = 0; appId < NLSTK_SSAP_CLIENT_APP_MAX_NUM; appId++) {
            if (g_regList[appId] == NULL || memcmp(&g_regList[appId]->addr, addr, SLE_ADDR_LEN) != 0 ||
                g_regList[appId]->cb == NULL) {
                continue;
            }
            NLSTK_SsapClientServiceChangeCb hook = g_regList[appId]->cb->onServiceChange;
            if (hook != NULL) {
                hook(appId, startHandle, &uuid);
            }
        }
    }
    SSAP_ParamFind_S findParam = {0};
    (void)memcpy_s(&findParam.addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    findParam.appId = SSAPC_SERVICE_CHANGE_APPID;
    findParam.startHandle = SSAP_START_HANDLE;
    findParam.endHandle = SSAP_END_HANDLE;
    if (CfgdbGetManufacturerSupport(addr, CFGDB_FIND_SERVICE_STRUCTURE)) {
        findParam.type = FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE;
    } else {
        findParam.type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
    }
    CP_LOG_INFO("[SSAPC_APP] SsapcAppServChange, find type: %d", findParam.type);
    SSAP_FindReq(SSAPC_SERVICE_CHANGE_APPID, SSAP_INTERACTION_DEFAULT_TIMEOUT, SsapcAppDiscServCompCb, &findParam);
}

void SsapcAppGetServ(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppGetServ");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppGetServ");
    NLSTK_SsapServObtain_S *obtainParam = (NLSTK_SsapServObtain_S *)param;
    CP_LOG_DEBUG("[SSAPC_APP] SsapcAppGetServ appId is %d", obtainParam->appId);
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(obtainParam->appId), "[SSAPC_APP] appId is invalid");
    SLE_Addr_S *addr = GetAddrByAppId(obtainParam->appId);
    NLSTK_Errcode_E ret = SsapcCacheGetServices(addr, obtainParam->appId, obtainParam->uuid, obtainParam->serv,
        obtainParam->num);
    CP_CHECK_LOG_RETURN_VOID(ret == NLSTK_ERRCODE_SUCCESS, "[SSAPC_APP] get service from cache fail");
    *(obtainParam->func) = SsapcCacheFreeServices;
}

void SsapcAppGetServAsyn(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppGetServAsyn");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppGetServAsyn");
    NLSTK_SsapServObtainAsyn_S *obtainAsynParam = (NLSTK_SsapServObtainAsyn_S *)param;
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(obtainAsynParam->appId), "[SSAPC_APP] appId is invalid");
    NLSTK_SsapClientGetServicesCb hook = g_regList[obtainAsynParam->appId]->cb->onGetServices;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] get serv cb is null");
    SLE_Addr_S *addr = GetAddrByAppId(obtainAsynParam->appId);
    NLSTK_SsapServ_S *service = NULL;
    uint16_t serviceNum = 0;
    NLSTK_Errcode_E ret = SsapcCacheGetServices(addr, obtainAsynParam->appId, obtainAsynParam->uuid, &service,
        &serviceNum);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        CP_LOG_ERROR("[SSAPC_APP] get service from cache fail");
        hook(obtainAsynParam->appId, obtainAsynParam->uuid, NULL, 0, SsapcCacheFreeServices);
    } else {
        hook(obtainAsynParam->appId, obtainAsynParam->uuid, service, serviceNum, SsapcCacheFreeServices);
    }
}

static void SsapcAppExchangeCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppExchangeCompCb");
    SSAP_ExchangeComplete_S *complete = (SSAP_ExchangeComplete_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(complete, "[SSAPC_APP] complete is null");

    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");

    NLSTK_SsapClientMtuChangedCb hook = g_regList[appId]->cb->onMtuChanged;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onMtuChanged callback func is null");
    hook(appId, complete->mtu, complete->errCode);
    return;
}

void SsapcAppExchangeMtu(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppExchangeMtu");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppExchangeMtu");
    SSAP_ExchangeInfoReqInfo_S *info = (SSAP_ExchangeInfoReqInfo_S *)param;
    CP_LOG_INFO("[SSAPC_APP] SsapcAppExchangeMtu appId is %d, mtu %d", info->appId, info-> mtu);
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(info->appId), "[SSAPC_APP] appId is invalid");
    SLE_Addr_S *addr = GetAddrByAppId(info->appId);
    (void)memcpy_s(&info->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SSAP_ExchangeInfoReq(info->appId, g_regList[info->appId]->timeout, SsapcAppExchangeCompCb, info);
}

static void SsapcAppReadPropertyCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppReadPropertyCompCb");
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(valuePkt, "[SSAPC_APP] valuePkt is null");

    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");
    CP_CHECK_LOG_RETURN_VOID(g_regList[appId]->cb, "[SSAPC_APP] cb is null");
    NLSTK_SsapClientReadPropertyCb hook = g_regList[appId]->cb->onReadProperty;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onReadProperty callback func is null");

    NLSTK_Errcode_E result = valuePkt->errorCode != 0 ? NLSTK_ERRCODE_FAIL : NLSTK_ERRCODE_SUCCESS;
    NLSTK_SsapClientReadPropertyInfo_S info = {0};
    info.value.len = valuePkt->value.len;
    info.value.data = valuePkt->value.value;
    info.handle = valuePkt->handle;
    info.errorCode = valuePkt->errorCode;
    SsapcCacheGetUuidByHandle(&valuePkt->addr, &info.uuid, valuePkt->handle);
    CP_LOG_INFO("[SSAPC_APP] read prop handle %d, uuid %s", valuePkt->handle, SSAP_GET_ENC_UUID(&info.uuid));
    hook(appId, &info, result);
}

static void SsapcAppReadDescriptorCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppReadDescriptorCompCb");
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(valuePkt, "[SSAPC_APP] valuePkt is null");

    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");
    NLSTK_SsapClientReadDescriptorCb hook = g_regList[appId]->cb->onReadDescriptor;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onReadDescriptor callback func is null");

    NLSTK_Errcode_E result = valuePkt->errorCode != 0 ? NLSTK_ERRCODE_FAIL : NLSTK_ERRCODE_SUCCESS;
    NLSTK_SsapClientReadDescriptorInfo_S info = {0};
    info.value.len = valuePkt->value.len;
    info.value.data = valuePkt->value.value;
    info.handle = valuePkt->handle;
    info.type = valuePkt->dataType;
    info.errorCode = valuePkt->errorCode;
    SsapcCacheGetUuidByHandle(&valuePkt->addr, &info.uuid, valuePkt->handle);
    CP_LOG_INFO("[SSAPC_APP] read prop handle %d, uuid %s", valuePkt->handle, SSAP_GET_ENC_UUID(&info.uuid));
    hook(appId, &info, result);
}

void SsapcAppRead(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppReadProperty");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppReadProperty");
    SSAP_ReadReqInfo_S *info = (SSAP_ReadReqInfo_S *)param;
    CP_LOG_DEBUG("[SSAPC_APP] SsapcAppReadProperty appId is %d, handle %d", info->appId, info->handle);
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(info->appId), "[SSAPC_APP] appId is invalid");
    SLE_Addr_S *addr = GetAddrByAppId(info->appId);
    (void)memcpy_s(&info->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SSAP_ReadReq(info->appId, g_regList[info->appId]->timeout,
        info->type == DESC_TYPE_PROPERTY_RESERVE ? SsapcAppReadPropertyCompCb : SsapcAppReadDescriptorCompCb, info);
}

static void SsapcAppReadPropertiesCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppReadPropertiesCompCb");
    SSAP_ReadByHandleComplete_S *complete = (SSAP_ReadByHandleComplete_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(complete, "[SSAPC_APP] complete is null");
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");

    NLSTK_SsapClientReadPropertiesCb hook = g_regList[appId]->cb->onReadProperties;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onReadProperties callback func is null");
    if (complete->errCode != SSAP_ERRCODE_SUCCESS) {
        hook(appId, 0, NULL, NLSTK_ERRCODE_FAIL);
        return;
    }
    uint8_t num = (uint8_t)complete->readVals->size;
    NLSTK_SsapClientReadPropertyInfo_S *props =
        (NLSTK_SsapClientReadPropertyInfo_S *)SDF_MemZalloc(sizeof(NLSTK_SsapClientReadPropertyInfo_S) * num);
    if (props == NULL) {
        hook(appId, 0, NULL, NLSTK_ERRCODE_FAIL);
        return;
    }
    for (uint8_t i = 0; i < num; i++) {
        SSAP_ValuePkt_S *cur = (SSAP_ValuePkt_S *)SDF_VectorElementAt(complete->readVals, i);
        props[i].handle = cur->handle;
        props[i].errorCode = cur->errorCode;
        SsapcCacheGetUuidByHandle(&complete->addr, &props[i].uuid, cur->handle);
        if (cur->value.len != 0) {
            props[i].value.data = (uint8_t *)SDF_MemZalloc(cur->value.len);
            if (props[i].value.data == NULL) {
                continue;
            }
            (void)memcpy_s(props[i].value.data, cur->value.len, cur->value.value, cur->value.len);
            props[i].value.len = cur->value.len;
        }
    }
    hook(appId, num, props, NLSTK_ERRCODE_SUCCESS);
    for (uint8_t i = 0; i < num; i++) {
        if (props[i].value.data != NULL) {
            SDF_MemFree(props[i].value.data);
        }
    }
    SDF_MemFree(props);
}

void SsapcAppReadProps(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppReadProps");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null");
    SSAP_ReadPropsInfo_S *info = (SSAP_ReadPropsInfo_S *)param;
    CP_LOG_DEBUG("[SSAPC_APP] SsapcAppReadProps appId is %d, num %d", info->appId, info->num);
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(info->appId), "[SSAPC_APP] appId is invalid");
    SLE_Addr_S *addr = GetAddrByAppId(info->appId);
    (void)memcpy_s(&info->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SSAP_ReadReqProps(info->appId, g_regList[info->appId]->timeout, SsapcAppReadPropertiesCompCb, info);
}

/**
 * @brief 根据UUID读取应用程序的回调函数
 *
 * @param complete 完成读取的信息
 * @return 无
 */
static void SsapcAppReadByUuidCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppReadByUuidCompCb");
    SSAP_ReadByUuidComplete_S *complete = (SSAP_ReadByUuidComplete_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(complete, "[SSAPC_APP] complete is null");
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");

    NLSTK_SsapClientReadPropertyByUuidCb hook = g_regList[appId]->cb->onReadPropertyByUuid;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onReadPropertyByUuid callback func is null");
    if (complete->errCode != SSAP_ERRCODE_SUCCESS || complete->readVals == NULL || complete->readVals->size == 0) {
        hook(appId, NULL, 0, NLSTK_ERRCODE_FAIL);
        return;
    }
    uint8_t num = (uint8_t)complete->readVals->size;
    NLSTK_SsapClientReadPropertyInfo_S *props =
        (NLSTK_SsapClientReadPropertyInfo_S *)SDF_MemZalloc(sizeof(NLSTK_SsapClientReadPropertyInfo_S) * num);
    if (props == NULL) {
        CP_LOG_ERROR("[SSAPC_APP] malloc fail");
        hook(appId, NULL, 0, NLSTK_ERRCODE_FAIL);
        return;
    }
    for (uint8_t i = 0; i < num; i++) {
        SSAP_ValuePkt_S *cur = (SSAP_ValuePkt_S *)SDF_VectorElementAt(complete->readVals, i);
        props[i].handle = cur->handle;
        props[i].errorCode = cur->errorCode;
        (void)memcpy_s(&props[i].uuid, sizeof(NLSTK_SsapUuid_S), &complete->uuid, sizeof(NLSTK_SsapUuid_S));
        if (cur->value.len != 0) {
            props[i].value.data = (uint8_t *)SDF_MemZalloc(cur->value.len);
            if (props[i].value.data == NULL) {
                continue;
            }
            (void)memcpy_s(props[i].value.data, cur->value.len, cur->value.value, cur->value.len);
            props[i].value.len = cur->value.len;
        }
    }
    hook(appId, props, num, NLSTK_ERRCODE_SUCCESS);
    for (uint8_t i = 0; i < num; i++) {
        if (props[i].value.data != NULL) {
            SDF_MemFree(props[i].value.data);
        }
    }
    SDF_MemFree(props);
}

void SsapcAppReadByUuid(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppReadPropertyByUuid");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppReadPropertyByUuid");
    SSAP_ReadByUuidReqInfo_S *info = (SSAP_ReadByUuidReqInfo_S *)param;
    CP_LOG_INFO("[SSAPC_APP] SsapcAppReadPropertyByUuid appId is %d, uuid %s",
        info->appId, SSAP_GET_ENC_UUID(&info->uuid));
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(info->appId), "[SSAPC_APP] appId is invalid");
    SLE_Addr_S *addr = GetAddrByAppId(info->appId);
    (void)memcpy_s(&info->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SSAP_ReadByUuidReq(info->appId, g_regList[info->appId]->timeout, SsapcAppReadByUuidCompCb, info);
}

static void SsapcGetCpcdNtfCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcGetCpcdNtfCompCb");
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(valuePkt, "[SSAPC_APP] valuePkt is null");
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");
    NLSTK_SsapClientGetPropertyNtfCb hook = g_regList[appId]->cb->onGetPropertyNtf;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onGetPropertyNtf callback func is null");
    NLSTK_SsapUuid_S uuid = {0};
    SsapcCacheGetUuidByHandle(&valuePkt->addr, &uuid, valuePkt->handle);
    if (valuePkt->errorCode != SSAP_ERRCODE_SUCCESS) {
        hook(appId, &uuid, valuePkt->handle, false, NLSTK_ERRCODE_FAIL);
        return;
    }
    if (valuePkt->value.len != SSAPC_SET_CPCD_LEN ||
        (*(uint16_t *)valuePkt->value.value) != CPCD_NOTIFICATION_ENABLE) {
        hook(appId, &uuid, valuePkt->handle, false, NLSTK_ERRCODE_SUCCESS);
        return;
    }
    hook(appId, &uuid, valuePkt->handle, true, NLSTK_ERRCODE_SUCCESS);
}

static void SsapcGetCpcdIndCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcGetCpcdIndCompCb");
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(valuePkt, "[SSAPC_APP] valuePkt is null");
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");
    NLSTK_SsapClientGetPropertyIndCb hook = g_regList[appId]->cb->onGetPropertyInd;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onGetPropertyInd callback func is null");
    NLSTK_SsapUuid_S uuid = {0};
    SsapcCacheGetUuidByHandle(&valuePkt->addr, &uuid, valuePkt->handle);
    if (valuePkt->errorCode != SSAP_ERRCODE_SUCCESS) {
        hook(appId, &uuid, valuePkt->handle, false, NLSTK_ERRCODE_FAIL);
        return;
    }
    if (valuePkt->value.len != SSAPC_SET_CPCD_LEN ||
        (*(uint16_t *)valuePkt->value.value) != CPCD_INDICATION_ENABLE) {
        hook(appId, &uuid, valuePkt->handle, false, NLSTK_ERRCODE_SUCCESS);
        return;
    }
    hook(appId, &uuid, valuePkt->handle, true, NLSTK_ERRCODE_SUCCESS);
}

void SsapcAppGetCpcd(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppGetCpcd");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppGetCpcd");
    NLSTK_SsapClientGetPropertyInfo_S *cpcdParam = (NLSTK_SsapClientGetPropertyInfo_S *)param;
    CP_LOG_INFO("[SSAPC_APP] SsapcAppGetCpcd appId is %d, handle %d, isNtf %d",
        cpcdParam->appId, cpcdParam->handle, cpcdParam->isNtf);
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(cpcdParam->appId), "[SSAPC_APP] appId is invalid");
    SLE_Addr_S *addr = GetAddrByAppId(cpcdParam->appId);
    SSAP_ReadReqInfo_S readReqInfo = { 0 };
    (void)memcpy_s(&readReqInfo.addr, sizeof(readReqInfo.addr), addr, sizeof(SLE_Addr_S));
    readReqInfo.handle = cpcdParam->handle;
    readReqInfo.type = DESC_TYPE_CLIENT_CONFIG;
    SSAP_ReadReq(cpcdParam->appId, g_regList[cpcdParam->appId]->timeout,
        cpcdParam->isNtf ? SsapcGetCpcdNtfCompCb : SsapcGetCpcdIndCompCb, &readReqInfo);
    return;
}

static void SsapcAppSetCpcdCbk(NLSTK_SsapAppClientCb_S *cb, NLSTK_SsapClientSetPropertyInfo_S *info,
    NLSTK_Errcode_E ret)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppSetCpcdCbk");
    CP_CHECK_LOG_RETURN_VOID(cb && info, "[SSAPC_APP] param is null");
    if (info->isNtf) {
        NLSTK_SsapClientGetPropertyNtfCb hook = cb->onSetPropertyNtf;
        CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onSetPropertyNtf callback func is null");
        NLSTK_SsapUuid_S uuid = {0};
        SLE_Addr_S *addr = GetAddrByAppId(info->appId);
        SsapcCacheGetUuidByHandle(addr, &uuid, info->handle);
        hook(info->appId, &uuid, info->handle, info->enable, ret);
    } else {
        NLSTK_SsapClientGetPropertyIndCb hook = cb->onSetPropertyInd;
        CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onSetPropertyInd callback func is null");
        NLSTK_SsapUuid_S uuid = {0};
        SLE_Addr_S *addr = GetAddrByAppId(info->appId);
        SsapcCacheGetUuidByHandle(addr, &uuid, info->handle);
        hook(info->appId, &uuid, info->handle, info->enable, ret);
    }
    return;
}

static void SsapcAppSetPropertyNtfCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppSetPropertyNtfCompCb");
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(valuePkt, "[SSAPC_APP] valuePkt is null");
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");

    NLSTK_SsapClientSetPropertyNtfCb hook = g_regList[appId]->cb->onSetPropertyNtf;
    CP_CHECK_LOGD_RETURN_VOID(hook, "[SSAPC_APP] onSetPropertyNtf callback func is null");

    NLSTK_SsapUuid_S uuid = {0};
    SsapcCacheGetUuidByHandle(&valuePkt->addr, &uuid, valuePkt->handle);
    if (valuePkt->errorCode != SSAP_ERRCODE_SUCCESS) {
        hook(appId, &uuid, valuePkt->handle, false, NLSTK_ERRCODE_FAIL);
        return;
    }
    if (valuePkt->value.len != SSAPC_SET_CPCD_LEN ||
        (*(uint16_t *)valuePkt->value.value) != CPCD_NOTIFICATION_ENABLE) {
        hook(appId, &uuid, valuePkt->handle, false, NLSTK_ERRCODE_SUCCESS);
        return;
    }
    hook(appId, &uuid, valuePkt->handle, true, NLSTK_ERRCODE_SUCCESS);
}

static void SsapcAppSetPropertyIndCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppSetPropertyIndCompCb");
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(valuePkt, "[SSAPC_APP] valuePkt is null");
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");

    NLSTK_SsapClientSetPropertyIndCb hook = g_regList[appId]->cb->onSetPropertyInd;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onSetPropertyInd callback func is null");

    NLSTK_SsapUuid_S uuid = {0};
    SsapcCacheGetUuidByHandle(&valuePkt->addr, &uuid, valuePkt->handle);
    if (valuePkt->errorCode != SSAP_ERRCODE_SUCCESS) {
        hook(appId, &uuid, valuePkt->handle, false, NLSTK_ERRCODE_FAIL);
        return;
    }
    if (valuePkt->value.len != SSAPC_SET_CPCD_LEN ||
        (*(uint16_t *)valuePkt->value.value) != CPCD_INDICATION_ENABLE) {
        hook(appId, &uuid, valuePkt->handle, false, NLSTK_ERRCODE_SUCCESS);
        return;
    }
    hook(appId, &uuid, valuePkt->handle, true, NLSTK_ERRCODE_SUCCESS);
}

void SsapcAppSetCpcd(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppSetCpcd");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppSetCpcd");
    NLSTK_SsapClientSetPropertyInfo_S *cpcdParam = (NLSTK_SsapClientSetPropertyInfo_S *)param;
    CP_LOG_INFO("[SSAPC_APP] SsapcAppSetCpcd appId is %d, handle %d, isNtf %d, enable %d",
        cpcdParam->appId, cpcdParam->handle, cpcdParam->isNtf, cpcdParam->enable);
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(cpcdParam->appId), "[SSAPC_APP] appId is invalid");
    SLE_Addr_S *addr = GetAddrByAppId(cpcdParam->appId);
    uint16_t cpcd = 0;
    if (cpcdParam->isNtf) {
        cpcd = (cpcdParam->enable) ? CPCD_NOTIFICATION_ENABLE : CPCD_ALL_DISABLE;
    } else {
        cpcd = (cpcdParam->enable) ? CPCD_INDICATION_ENABLE : CPCD_ALL_DISABLE;
    }
    NLSTK_Errcode_E ret = SsapcCacheSetCpcd(cpcdParam->appId, addr, cpcdParam->handle, cpcd);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        if (ret == NLSTK_ERRCODE_DIRECT_RETURN) {
            SsapcAppSetCpcdCbk(g_regList[cpcdParam->appId]->cb, cpcdParam, NLSTK_ERRCODE_SUCCESS);
            return;
        }
        CP_LOG_ERROR("[SSAPC_APP] cpcd set fail");
        SsapcAppSetCpcdCbk(g_regList[cpcdParam->appId]->cb, cpcdParam, NLSTK_ERRCODE_FAIL);
        return;
    }

    SSAP_WriteReqInfo_S *writeReqInfo =
        (SSAP_WriteReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteReqInfo_S) + SSAPC_SET_CPCD_LEN);
    if (!writeReqInfo) {
        CP_LOG_ERROR("[SSAPC_APP] malloc fail");
        SsapcAppSetCpcdCbk(g_regList[cpcdParam->appId]->cb, cpcdParam, NLSTK_ERRCODE_MALLOC_FAIL);
        return;
    }
    (void)memcpy_s(&writeReqInfo->addr, sizeof(writeReqInfo->addr), addr, sizeof(SLE_Addr_S));
    writeReqInfo->handle = cpcdParam->handle;
    writeReqInfo->type = DESC_TYPE_CLIENT_CONFIG;
    writeReqInfo->value.len = SSAPC_SET_CPCD_LEN;
    (void)memcpy_s(writeReqInfo->value.value, SSAPC_SET_CPCD_LEN, &cpcd, SSAPC_SET_CPCD_LEN);
    SSAP_WriteReq(cpcdParam->appId, g_regList[cpcdParam->appId]->timeout,
        cpcdParam->isNtf ? SsapcAppSetPropertyNtfCompCb : SsapcAppSetPropertyIndCompCb, writeReqInfo);
    SDF_MemFree(writeReqInfo);
    writeReqInfo = NULL;
    return;
}

static void SsapcAppWriteCmdCompCb(int32_t appId, SLE_Addr_S *addr, SSAP_WriteCmdInfo_S *writeCmdInfo)
{
    if (writeCmdInfo->type == DESC_TYPE_PROPERTY_RESERVE) {
        NLSTK_SsapClientWritePropertyCb hook = g_regList[appId]->cb->onWriteProperty;
        CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onWriteProperty callback func is null");
        NLSTK_SsapClientWritePropertyInfo_S info = {0};
        info.value.len = writeCmdInfo->value.len;
        info.value.data = writeCmdInfo->value.value;
        info.handle = writeCmdInfo->handle;
        info.errorCode = 0;
        SsapcCacheGetUuidByHandle(addr, &info.uuid, info.handle);
        hook(appId, &info, NLSTK_ERRCODE_SUCCESS);
    } else {
        NLSTK_SsapClientWriteDescriptorCb hook = g_regList[appId]->cb->onWriteDescriptor;
        CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onWriteDescriptor callback func is null");
        NLSTK_SsapClientWriteDescriptorInfo_S info = {0};
        info.value.len = writeCmdInfo->value.len;
        info.value.data = writeCmdInfo->value.value;
        info.handle = writeCmdInfo->handle;
        info.type = writeCmdInfo->type;
        info.errorCode = 0;
        SsapcCacheGetUuidByHandle(addr, &info.uuid, info.handle);
        hook(appId, &info, NLSTK_ERRCODE_SUCCESS);
    }
}

void SsapcAppWriteCmd(void *param)
{
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppWriteCmd");
    NLSTK_SsapClientWriteBaseInfo_S *info = (NLSTK_SsapClientWriteBaseInfo_S *)param;
    CP_LOG_DEBUG("[SSAPC_APP] SsapcAppWriteCmd appId is %d, handle %d, type %d",
        info->appId, info->handle, info->type);
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(info->appId), "[SSAPC_APP] appId is invalid");

    /* 判断参数合法性 */
    if (!(info->value) || info->value->len == 0 || !(info->value->data)) {
        CP_LOG_ERROR("[SSAPC_APP] param value is invalid");
        return;
    }

    /* 获取addr */
    SLE_Addr_S *addr = GetAddrByAppId(info->appId);

    /* 拷贝结构体 */
    SSAP_WriteCmdInfo_S *writeCmdInfo = (SSAP_WriteCmdInfo_S *)
        SDF_MemZalloc(sizeof(SSAP_WriteCmdInfo_S) + info->value->len);
    CP_CHECK_LOG_RETURN_VOID(writeCmdInfo, "[SSAPC_APP] malloc fail");
    (void)memcpy_s(&writeCmdInfo->addr, sizeof(writeCmdInfo->addr), addr, sizeof(SLE_Addr_S));
    writeCmdInfo->handle = info->handle;
    writeCmdInfo->type = info->type;
    writeCmdInfo->value.len = info->value->len;
    (void)memcpy_s(writeCmdInfo->value.value, info->value->len, info->value->data, info->value->len);
    SSAP_WriteCmd(writeCmdInfo);

    SsapcAppWriteCmdCompCb(info->appId, addr, writeCmdInfo);

    /* 释放内存 */
    SDF_MemFree(writeCmdInfo);
    writeCmdInfo = NULL;
    return;
}

static void SsapcAppWriteReqError(NLSTK_SsapAppClientCb_S *cb, NLSTK_SsapClientWriteBaseInfo_S *info,
    NLSTK_Errcode_E ret)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppWriteCmdError");
    CP_CHECK_LOG_RETURN_VOID(cb && info, "[SSAPC_APP] param is null");

    if (info->type == DESC_TYPE_PROPERTY_RESERVE) {
        /* 触发写属性回调 */
        NLSTK_SsapClientWritePropertyCb hook = cb->onWriteProperty;
        CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onWriteProperty callback func is null");
        NLSTK_SsapClientWritePropertyInfo_S property = {0};
        property.handle = info->handle;
        hook(info->appId, &property, ret);
    } else {
        /* 触发写描述符回调 */
        NLSTK_SsapClientWriteDescriptorCb hook = cb->onWriteDescriptor;
        CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onWriteDescriptor callback func is null");
        NLSTK_SsapClientWriteDescriptorInfo_S descriptor = {0};
        descriptor.handle = info->handle;
        descriptor.type = info->type;
        hook(info->appId, &descriptor, ret);
    }
    return;
}

static void SsapcAppWritePropertyCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppWritePropertyCompCb");
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(valuePkt, "[SSAPC_APP] valuePkt is null");
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");

    NLSTK_SsapClientWritePropertyCb hook = g_regList[appId]->cb->onWriteProperty;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onWriteProperty callback func is null");
    NLSTK_SsapClientWritePropertyInfo_S info = {0};
    info.value.len = valuePkt->value.len;
    info.value.data = valuePkt->value.value;
    info.handle = valuePkt->handle;
    info.errorCode = valuePkt->errorCode;
    SsapcCacheGetUuidByHandle(&valuePkt->addr, &info.uuid, valuePkt->handle);
    hook(appId, &info, valuePkt->errorCode == SSAP_ERRCODE_SUCCESS ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_FAIL);
}

static void SsapcAppWriteDescriptorCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppWriteDescriptorCompCb");
    SSAP_ValuePkt_S *valuePkt = (SSAP_ValuePkt_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(valuePkt, "[SSAPC_APP] valuePkt is null");
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");

    NLSTK_SsapClientWriteDescriptorCb hook = g_regList[appId]->cb->onWriteDescriptor;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onWriteProperty callback func is null");
    NLSTK_SsapClientWriteDescriptorInfo_S info = {0};
    info.value.len = valuePkt->value.len;
    info.value.data = valuePkt->value.value;
    info.handle = valuePkt->handle;
    info.errorCode = valuePkt->errorCode;
    info.type = valuePkt->dataType;
    SsapcCacheGetUuidByHandle(&valuePkt->addr, &info.uuid, valuePkt->handle);
    hook(appId, &info, valuePkt->errorCode == SSAP_ERRCODE_SUCCESS ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_FAIL);
}

void SsapcAppWriteReq(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppWriteReq");
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppWriteReq");
    NLSTK_SsapClientWriteBaseInfo_S *info = (NLSTK_SsapClientWriteBaseInfo_S *)param;
    CP_LOG_INFO("[SSAPC_APP] SsapcAppWriteReq appId is %d, handle %d, type %d",
        info->appId, info->handle, info->type);
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(info->appId), "[SSAPC_APP] appId is invalid");

    /* 判断参数合法性 */
    if (!(info->value) || info->value->len == 0 || !(info->value->data)) {
        SsapcAppWriteReqError(g_regList[info->appId]->cb, info, NLSTK_ERRCODE_POINTER_NULL);
        CP_LOG_ERROR("[SSAPC_APP] param value is invalid");
        return;
    }

    /* 获取addr */
    SLE_Addr_S *addr = GetAddrByAppId(info->appId);
    /* 拷贝结构体 */
    SSAP_WriteReqInfo_S *writeReqInfo = (SSAP_WriteReqInfo_S *)
        SDF_MemZalloc(sizeof(SSAP_WriteReqInfo_S) + info->value->len);
    if (!writeReqInfo) {
        SsapcAppWriteReqError(g_regList[info->appId]->cb, info, NLSTK_ERRCODE_MALLOC_FAIL);
        CP_LOG_ERROR("[SSAPC_APP] malloc fail");
        return;
    }
    (void)memcpy_s(&writeReqInfo->addr, sizeof(writeReqInfo->addr), addr, sizeof(SLE_Addr_S));
    writeReqInfo->handle = info->handle;
    writeReqInfo->type = info->type;
    writeReqInfo->value.len = info->value->len;
    (void)memcpy_s(writeReqInfo->value.value, info->value->len, info->value->data, info->value->len);
    SSAP_WriteReq(info->appId, g_regList[info->appId]->timeout,
        info->type == DESC_TYPE_PROPERTY_RESERVE ? SsapcAppWritePropertyCompCb : SsapcAppWriteDescriptorCompCb,
        writeReqInfo);

    /* 释放内存 */
    SDF_MemFree(writeReqInfo);
    writeReqInfo = NULL;
    return;
}

void SsapcAppCallMethodCmd(void *param)
{
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppCallMethodCmd");
    NLSTK_SsapClientCallMethodInfo_S *info = (NLSTK_SsapClientCallMethodInfo_S *)param;
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(info->appId), "[SSAPC_APP] appId is invalid");

    /* 判断参数合法性 */
    if (info->value.len == 0 || info->value.data == NULL) {
        CP_LOG_ERROR("[SSAPC_APP] param value is invalid");
        return;
    }

    /* 获取addr */
    SLE_Addr_S *addr = GetAddrByAppId(info->appId);

    /* 拷贝结构体 */
    SSAP_CallMethodCmdInfo_S *callCmdInfo = (SSAP_CallMethodCmdInfo_S *)
        SDF_MemZalloc(sizeof(SSAP_CallMethodCmdInfo_S) + info->value.len);
    CP_CHECK_LOG_RETURN_VOID(callCmdInfo, "[SSAPC_APP] malloc fail");
    (void)memcpy_s(&callCmdInfo->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    callCmdInfo->handle = info->handle;
    callCmdInfo->value.len = info->value.len;
    (void)memcpy_s(callCmdInfo->value.value, info->value.len, info->value.data, info->value.len);
    SSAP_CallMethodCmd(callCmdInfo);

    /* 释放内存 */
    SDF_MemFree(callCmdInfo);
    callCmdInfo = NULL;
    return;
}

static void SsapcAppCallMethodCompCb(int32_t appId, void *arg)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcAppCallMethodRetCb");
    SSAP_MethodResult_S *result = (SSAP_MethodResult_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(result, "[SSAPC_APP] result is null");

    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(appId), "[SSAPC_APP] invalid appId");

    NLSTK_SsapClientCallMethodCb hook = g_regList[appId]->cb->onCallMethod;
    CP_CHECK_LOG_RETURN_VOID(hook, "[SSAPC_APP] onCallMethod callback func is null");

    NLSTK_SsapClientCallMethodResult_S param = {0};
    param.handle = result->handle;
    param.errorCode = result->errorCode;
    param.value.len = result->value.len;
    param.value.data = result->value.value;
    SsapcCacheGetUuidByHandle(&result->addr, &param.uuid, result->handle);
    NLSTK_Errcode_E ret = param.errorCode == SSAP_ERRCODE_SUCCESS ? NLSTK_ERRCODE_SUCCESS : NLSTK_ERRCODE_FAIL;
    hook(appId, &param, ret);
    return;
}

void SsapcAppCallMethodReq(void *param)
{
    CP_CHECK_LOG_RETURN_VOID(param, "[SSAPC_APP] param is null in SsapcAppCallMethodReq");
    NLSTK_SsapClientCallMethodInfo_S *info = (NLSTK_SsapClientCallMethodInfo_S *)param;
    CP_CHECK_LOG_RETURN_VOID(IsAppIdValid(info->appId), "[SSAPC_APP] appId is invalid");

    /* 判断参数合法性 */
    if (info->value.len == 0 || info->value.data == NULL) {
        CP_LOG_ERROR("[SSAPC_APP] param value is invalid");
        return;
    }

    /* 获取addr */
    SLE_Addr_S *addr = GetAddrByAppId(info->appId);

    /* 拷贝结构体 */
    SSAP_CallMethodReqInfo_S *callReqInfo = (SSAP_CallMethodReqInfo_S *)
        SDF_MemZalloc(sizeof(SSAP_CallMethodReqInfo_S) + info->value.len);
    CP_CHECK_LOG_RETURN_VOID(callReqInfo, "[SSAPC_APP] malloc fail");
    (void)memcpy_s(&callReqInfo->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    callReqInfo->handle = info->handle;
    callReqInfo->value.len = info->value.len;
    (void)memcpy_s(callReqInfo->value.value, info->value.len, info->value.data, info->value.len);
    SSAP_CallMethodReq(info->appId, g_regList[info->appId]->timeout, SsapcAppCallMethodCompCb, callReqInfo);

    /* 释放内存 */
    SDF_MemFree(callReqInfo);
    callReqInfo = NULL;
    return;
}

static void SsapcAppPropertyNtfHandle(SSAP_ValuePkt_S *valuePkt)
{
    /* 必须根据cpcd的设置触发回调 */
    SDF_Vector_S *cpcdConfig = SsapcCacheGetCpcdConfig(&valuePkt->addr, valuePkt->handle);
    CP_CHECK_LOG_RETURN_VOID(cpcdConfig, "[SSAPC_APP] cpcd config is null");
    NLSTK_SsapClientReadPropertyInfo_S info = {0};
    info.value.len = valuePkt->value.len;
    info.value.data = valuePkt->value.value;
    info.handle = valuePkt->handle;
    SsapcCacheGetUuidByHandle(&valuePkt->addr, &info.uuid, valuePkt->handle);
    for (size_t i = 0; i < cpcdConfig->size; i++) {
        SsapCpcdCfg_S *cpcdCfg = (SsapCpcdCfg_S *)SDF_VectorElementAt(cpcdConfig, i);
        if (!IsAppIdValid(cpcdCfg->appId)) {
            continue;
        }
        if ((cpcdCfg->cpcdVal == CPCD_NOTIFICATION_ENABLE) && (valuePkt->opCode == SSAP_VALUE_NTF)) {
            NLSTK_SsapClientPropertyChangedCb hook = g_regList[cpcdCfg->appId]->cb->onPropertyChanged;
            if (hook != NULL) {
                hook(cpcdCfg->appId, &info);
            }
        }
        if ((cpcdCfg->cpcdVal == CPCD_INDICATION_ENABLE) && (valuePkt->opCode == SSAP_VALUE_IND)) {
            NLSTK_SsapClientPropertyChangedCb hook = g_regList[cpcdCfg->appId]->cb->onPropertyChanged;
            if (hook != NULL) {
                hook(cpcdCfg->appId, &info);
            }
        }
    }
}

static void SsapcAppEventNtfHandle(SSAP_ValuePkt_S *valuePkt)
{
    NLSTK_SsapClientEventInfo_S info = {0};
    info.value.len = valuePkt->value.len;
    info.value.data = valuePkt->value.value;
    info.handle = valuePkt->handle;
    for (int32_t appId = 0; appId < NLSTK_SSAP_CLIENT_APP_MAX_NUM; appId++) {
        if (g_regList[appId] == NULL || memcmp(&g_regList[appId]->addr, &valuePkt->addr, SLE_ADDR_LEN) != 0 ||
            g_regList[appId]->cb == NULL) {
            continue;
        }
        NLSTK_SsapClientEventCb hook = g_regList[appId]->cb->onEvent;
        if (hook != NULL) {
            hook(appId, &info);
        }
    }
}

void SsapcAppPropertyNtf(SSAP_ValuePkt_S *valuePkt)
{
    CP_CHECK_LOG_RETURN_VOID(valuePkt, "[SSAPC_APP] valuePkt is null");
    uint8_t mask = 0x01 << SSAP_BIT_POSITION;
    uint8_t bitValue = (valuePkt->controlCode & mask) >> SSAP_BIT_POSITION;
    if (bitValue == SSAP_PROPERTY_NTF) { // 属性的通知
        SsapcAppPropertyNtfHandle(valuePkt);
    } else if (bitValue == SSAP_EVENT_NTF) { // 事件的通知
        SsapcAppEventNtfHandle(valuePkt);
    }
    return;
}

void SsapcClientCleanApp(void *param)
{
    CP_LOG_DEBUG("[SSAPC_APP] enter func SsapcClientCleanApp");
    g_cleanAppResult = param;
    bool allClientAppClean = true;
    for (int32_t appId = 0; appId < NLSTK_SSAP_CLIENT_APP_MAX_NUM; appId++) {
        if (g_regList[appId] == NULL) {
            continue;
        }
        SsapcClearLinkOper(appId); // 清理所有链路缓存的操作
        if (g_regList[appId]->linkState == SSAP_CONNECT_STATE_DISCONNECTED ||
            g_regList[appId]->linkState == SSAP_CONNECT_STATE_IDLE) {
            NLSTK_LOG_INFO("clean up client appId(%d)", appId);
            SsapcCbDestroy(g_regList[appId]->cb);
            SDF_MemFree(g_regList[appId]);
            g_regList[appId] = NULL;
            continue;
        }
        allClientAppClean = false;
        if (g_regList[appId]->linkState == SSAP_CONNECT_STATE_CONNECTING ||
            g_regList[appId]->linkState == SSAP_CONNECT_STATE_CONNECTED ||
            g_regList[appId]->linkState == SSAP_CONNECT_STATE_DISCONNECTING) {
            NLSTK_LOG_INFO("remove link of client appId(%d), addrs %s", appId, GET_ENC_ADDR(&(g_regList[appId]->addr)));
            SsapRemoveLink(&(g_regList[appId]->addr));
        }
    }
    // 所有客户端应用都已清理完毕，回调给上层
    if (allClientAppClean == true) {
        NLSTK_LOG_INFO("clean up all client appId and notify service");
        if (g_cleanAppResult != NULL) {
            g_cleanAppResult();
        }
        SsapResetClientCleanUp();
    }
    return;
}

void SsapcClientAppDeinit(void)
{
    for (int32_t appId = 0; appId < NLSTK_SSAP_CLIENT_APP_MAX_NUM; appId++) {
        if (g_regList[appId] == NULL) {
            continue;
        }
        SsapcCbDestroy(g_regList[appId]->cb);
        SDF_MemFree(g_regList[appId]);
        g_regList[appId] = NULL;
    }
    g_cleanAppResult = NULL;
}