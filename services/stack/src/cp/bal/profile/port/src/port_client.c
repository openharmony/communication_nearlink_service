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
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "sdf_vector.h"
#include "sdf_addr.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_port_def.h"
#include "port_type.h"
#include "port_stm.h"
#include "port_client_init.h"
#include "port_client.h"

#define PORT_BASE_LEN (sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(NLSTK_SsapUuid_S))

static SDF_Vector_S *g_portPrivateCache = NULL;
static NLSTK_PortClientCallBack_S g_portClientCbk = {0};

static void FreePortCache(void *obj)
{
    if (obj == NULL) {
        return;
    }
    PortInfoCache_S *portCache = (PortInfoCache_S *)obj;
    SDF_DestroyVector(portCache->portInfoList);
    SDF_MemFree(portCache);
}

NLSTK_Errcode_E PortClientEnable(void)
{
    NLSTK_LOG_INFO("[PORT] PortClientEnable enter");
    if (g_portPrivateCache != NULL) {
        SDF_DestroyVector(g_portPrivateCache);
        g_portPrivateCache = NULL;
    }
    SDF_Traits traits = {.dtor = FreePortCache};
    g_portPrivateCache = SDF_CreateVector(traits);
    NLSTK_CHECK_RETURN(g_portPrivateCache, NLSTK_ERRCODE_POINTER_NULL, "[PORT] create g_portPrivateCache failed");
    return NLSTK_ERRCODE_SUCCESS;
}

void PortClientDisable(void)
{
    NLSTK_LOG_INFO("[PORT] PortClientDisable enter");
    SDF_DestroyVector(g_portPrivateCache);
    g_portPrivateCache = NULL;
}

void PortStateChangeCbk(SLE_Addr_S *addr, NLSTK_PortConnectState_E curState, NLSTK_PortConnectState_E preState,
    NLSTK_Errcode_E ret)
{
    if (g_portClientCbk.connectStateChangeCbk) {
        g_portClientCbk.connectStateChangeCbk(addr, curState, preState, ret);
    }
}

void PortClientRegCbkInner(void *arg)
{
    NLSTK_LOG_INFO("[PORT] PortClientRegCbkInner enter");
    NLSTK_CHECK_RETURN_VOID(arg, "[PORT] arg is null");
    NLSTK_PortClientCallBack_S *cbk = (NLSTK_PortClientCallBack_S *)arg;
    (void)memcpy_s(&g_portClientCbk, sizeof(NLSTK_PortClientCallBack_S), cbk, sizeof(NLSTK_PortClientCallBack_S));
}

void PortClientDeregCbkInner(void *arg)
{
    SDF_UNUSED(arg);
    NLSTK_LOG_INFO("[PORT] PortClientDeregCbkInner enter");
    g_portClientCbk.connectStateChangeCbk = NULL;
}

static bool PortAddrCompFunc(void *ptr, void *args)
{
    NLSTK_CHECK_RETURN(ptr && args, false, "ptr or args is null");
    PortInfoCache_S *cache = (PortInfoCache_S *)ptr;
    SLE_Addr_S *target = (SLE_Addr_S *)args;
    return memcmp(&cache->addr, target, sizeof(SLE_Addr_S)) == 0;
}

PortInfoCache_S *PortFindInfoCacheByAddr(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(g_portPrivateCache != NULL, NULL, "[PORT] g_portPrivateCache is null");
    NLSTK_CHECK_RETURN(addr != NULL, NULL, "[PORT] addr is null");
    size_t index = 0;
    NLSTK_CHECK_RETURN(SDF_VectorFindFirst(g_portPrivateCache, PortAddrCompFunc, addr, &index), NULL,
        "[PORT] addr not found");
    PortInfoCache_S *portInfoCache = (PortInfoCache_S *)SDF_VectorElementAt(g_portPrivateCache, index);
    return portInfoCache;
}

static bool PortAppIdCompFunc(void *ptr, void *args)
{
    NLSTK_CHECK_RETURN(ptr && args, false, "ptr or args is null");
    PortInfoCache_S *cache = (PortInfoCache_S *)ptr;
    int32_t *target = (int32_t *)args;
    return memcmp(&cache->appId, target, sizeof(int32_t)) == 0;
}

PortInfoCache_S *PortFindInfoCacheByAppId(int32_t appId)
{
    NLSTK_CHECK_RETURN(g_portPrivateCache != NULL, NULL, "[PORT] g_portPrivateCache is null");
    size_t index = 0;
    NLSTK_CHECK_RETURN(SDF_VectorFindFirst(g_portPrivateCache, PortAppIdCompFunc, &appId, &index), NULL,
        "[PORT] appId not found");
    PortInfoCache_S *portInfoCache = (PortInfoCache_S *)SDF_VectorElementAt(g_portPrivateCache, index);
    return portInfoCache;
}

static PortInfoCache_S *PortInfoCacheCreate(SLE_Addr_S *addr, const NLSTK_ConnParam_S *connParam)
{
    PortInfoCache_S *portInfoCache = PortFindInfoCacheByAddr(addr);
    if (portInfoCache == NULL) {
        portInfoCache = (PortInfoCache_S *)SDF_MemZalloc(sizeof(PortInfoCache_S));
        NLSTK_CHECK_RETURN(portInfoCache, NULL, "[PORT] alloc portInfoCache failed");
        (void)memcpy_s(&portInfoCache->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        SDF_Traits portInfoListTraits = {.dtor = SDF_MemFree};
        portInfoCache->portInfoList = SDF_CreateVector(portInfoListTraits);
        if (!portInfoCache->portInfoList) {
            NLSTK_LOG_ERROR("[PORT] create portInfoCache->portInfoList failed");
            SDF_MemFree(portInfoCache);
            return NULL;
        }
        portInfoCache->stmState = PORT_STATE_IDLE;
        portInfoCache->appId = -1;
        (void)memcpy_s(&portInfoCache->connParam, sizeof(NLSTK_ConnParam_S), connParam, sizeof(NLSTK_ConnParam_S));
        if (!SDF_VectorEmplaceBack(g_portPrivateCache, portInfoCache)) {
            NLSTK_LOG_ERROR("[PORT] emplace_back portInfoCache failed");
            SDF_DestroyVector(portInfoCache->portInfoList);
            SDF_MemFree(portInfoCache);
            return NULL;
        }
    }
    return portInfoCache;
}

void PortInfoCacheRemove(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN_VOID(g_portPrivateCache != NULL, "[PORT] g_portPrivateCache is null");
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[PORT] addr is null");
    size_t index = 0;
    NLSTK_CHECK_RETURN_VOID(SDF_VectorFindFirst(g_portPrivateCache, PortAddrCompFunc, addr, &index),
        "[PORT] addr not found");
    NLSTK_LOG_INFO("[PORT] remove port cache, addr = %s", GET_ENC_ADDR(addr));
    SDF_VectorRemove(g_portPrivateCache, index);
}

void PortConnectInner(void *arg)
{
    NLSTK_CHECK_RETURN_VOID(arg, "[PORT] arg is null");
    NLSTK_ConnAddrParam_S *connAddrParam = (NLSTK_ConnAddrParam_S *)arg;
    SLE_Addr_S *addr = &connAddrParam->addr;
    NLSTK_ConnParam_S *param = &connAddrParam->param;
    NLSTK_LOG_DEBUG("[PORT] PortConnectInner enter, addr: %s, frameType:%hhu", GET_ENC_ADDR(addr), param->frameType);
    PortInfoCache_S *portInfoCache = PortInfoCacheCreate(addr, param);
    if (portInfoCache == NULL) {
        NLSTK_LOG_ERROR("[PORT] PortInfoCacheCreate failed");
        PortStateChangeCbk(addr, PORT_DISCONNECTED, PORT_DISCONNECTED, NLSTK_ERRCODE_POINTER_NULL);
        return;
    }
    PortStmParam_S msg = { .what = PORT_ON_USER_CONNECT, .extData = NULL };
    PortStateMachineCall(portInfoCache, msg);
}

void PortDisconnectInner(void *arg)
{
    NLSTK_LOG_INFO("[PORT] PortDisconnectInner enter");
    NLSTK_CHECK_RETURN_VOID(arg, "[PORT] arg is null");
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    PortInfoCache_S *portInfoCache = PortFindInfoCacheByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(portInfoCache != NULL, "[PORT] portInfoCache not found");
    PortStmParam_S msg = { .what = PORT_ON_USER_DISCONNECT, .extData = NULL };
    PortStateMachineCall(portInfoCache, msg);
}

void PortGetConnectStateInner(void *arg)
{
    NLSTK_LOG_DEBUG("[PORT] PortGetConnectStateInner enter");
    NLSTK_CHECK_RETURN_VOID(arg, "[PORT] arg is null");
    PortGetConnStateParam_S *param = (PortGetConnStateParam_S *)arg;
    PortInfoCache_S *portInfoCache = PortFindInfoCacheByAddr(&param->addr);
    if (portInfoCache == NULL) {
        param->state = PORT_DISCONNECTED;
        return;
    }
    if (portInfoCache->stmState == PORT_STATE_CONNECTED) {
        param->state = PORT_CONNECTED;
    } else {
        param->state = PORT_CONNECTING;
    }
}

static bool PortUuidCompFunc(void *ptr, void *args)
{
    NLSTK_CHECK_RETURN(ptr && args, false, "ptr or args is null");
    PortPrivateInfo_S *portInfo = (PortPrivateInfo_S *)ptr;
    NLSTK_SsapUuid_S *target = (NLSTK_SsapUuid_S *)args;
    return memcmp(&portInfo->uuid, target, sizeof(NLSTK_SsapUuid_S)) == 0;
}

void PortGetDevicePortIdByUuidInner(void *arg)
{
    NLSTK_LOG_DEBUG("[PORT] PortGetDevicePortIdByUuidInner enter");
    NLSTK_CHECK_RETURN_VOID(arg, "[PORT] arg is null");
    NLSTK_CHECK_RETURN_VOID(g_portPrivateCache, "[PORT] g_portPrivateCache is null");
    PortGetPortIdParam_S *param = (PortGetPortIdParam_S *)arg;
    PortInfoCache_S *portInfoCache = PortFindInfoCacheByAddr(&param->addr);
    NLSTK_CHECK_RETURN_VOID(portInfoCache != NULL, "[PORT] portInfoCache not found");
    size_t portIndex = 0;
    bool ret = SDF_VectorFindFirst(portInfoCache->portInfoList, PortUuidCompFunc, &param->uuid, &portIndex);
    NLSTK_CHECK_RETURN_VOID(ret, "[PORT] not found uuid in portInfoCache->portInfoList");
    PortPrivateInfo_S *portInfo = (PortPrivateInfo_S *)SDF_VectorElementAt(portInfoCache->portInfoList, portIndex);
    param->portId = portInfo->portId;
}

void PortGetConnectDeviceNumInner(void *arg)
{
    NLSTK_LOG_DEBUG("[PORT] PortGetConnectDeviceNumInner enter");
    NLSTK_CHECK_RETURN_VOID(arg, "[PORT] arg is null");
    NLSTK_CHECK_RETURN_VOID(g_portPrivateCache, "[PORT] g_portPrivateCache is null");
    PortGetConnDevNumParam_S *param = (PortGetConnDevNumParam_S *)arg;
    param->portConnectNum = (int)g_portPrivateCache->size;
}