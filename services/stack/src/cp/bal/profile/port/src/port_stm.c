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
#include "nlstk_public_define.h"
#include "nlstk_log.h"
#include "sdf_mem.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_port_def.h"
#include "ssap_utils.h"
#include "port_type.h"
#include "port_client.h"
#include "port_stm.h"

#define PORT_MAX_NUM 20

typedef void (*PortStateMachineDispatch)(PortInfoCache_S *portInfoCache, PortStmParam_S msg);

static void PortIdleStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg);
static void PortRegisterAppStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg);
static void PortCreateLinkStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg);
static void PortGetServiceStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg);
static void PortFindServiceStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg);
static void PortReadPropertyStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg);
static void PortSetNtfStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg);
static void PortConnectedStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg);

void PortStateMachineCall(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    static PortStateMachineDispatch portStateMachine[PORT_STATE_BUTT] = {
        [PORT_STATE_IDLE] = PortIdleStateDispatch,
        [PORT_STATE_REGISTER_APP] = PortRegisterAppStateDispatch,
        [PORT_STATE_CREATE_LINK] = PortCreateLinkStateDispatch,
        [PORT_STATE_GET_SERVICE] = PortGetServiceStateDispatch,
        [PORT_STATE_FIND_SERVICE] = PortFindServiceStateDispatch,
        [PORT_STATE_READ_PROPERTY] = PortReadPropertyStateDispatch,
        [PORT_STATE_SET_NET] = PortSetNtfStateDispatch,
        [PORT_STATE_CONNECTED] = PortConnectedStateDispatch,
    };
    NLSTK_CHECK_RETURN_VOID(portInfoCache != NULL, "[PORT] portInfoCache is null");
    NLSTK_LOG_INFO("[PORT] enter PortStateMachine, addr = %s, state = %u, msg = %u",
        GET_ENC_ADDR(&portInfoCache->addr), portInfoCache->stmState, msg.what);
    NLSTK_CHECK_RETURN_VOID(portInfoCache->stmState < PORT_STATE_BUTT, "[PORT] stmState error");
    portStateMachine[portInfoCache->stmState](portInfoCache, msg);
}

static void OnPortRegisterApp(int32_t appId, SLE_Addr_S *addr, NLSTK_Errcode_E ret)
{
    SDF_UNUSED(ret);
    NLSTK_LOG_DEBUG("[PORT] OnPortRegisterApp enter");
    PortInfoCache_S *portInfoCache = PortFindInfoCacheByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(portInfoCache != NULL, "[PORT] portInfoCache not found");
    PortStmParam_S msg = { .what = PORT_ON_REGISTER_APP, .extData = (void *)&appId };
    PortStateMachineCall(portInfoCache, msg);
}

static void OnPortConnectionStateChanged(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason)
{
    SDF_UNUSED(ret);
    NLSTK_LOG_DEBUG("[PORT] OnPortConnectionStateChanged enter");
    PortInfoCache_S *portInfoCache = PortFindInfoCacheByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(portInfoCache != NULL, "[PORT] portInfoCache not found");
    PortConnStateMsg_S connStateMsg = { .state = state, .ret = ret, .reason = reason };
    PortStmParam_S msg = { .what = PORT_ON_STATE_CHANGED, .extData = (void *)&connStateMsg };
    PortStateMachineCall(portInfoCache, msg);
}

static void OnPortGetServiceByUuid(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_SsapServ_S *service,
    uint16_t serviceNum, NLSTK_SsapClientFreeFunc func)
{
    SDF_UNUSED(uuid);
    NLSTK_LOG_DEBUG("[PORT] OnPortGetServiceByUuid enter");
    PortInfoCache_S *portInfoCache = PortFindInfoCacheByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(portInfoCache != NULL, "[PORT] portInfoCache not found");
    PortGetServiceMsg_S getServiceMsg = { .serviceNum = serviceNum, .service = service, .func = func };
    PortStmParam_S msg = { .what = PORT_ON_GET_SERVICE, .extData = (void *)&getServiceMsg };
    PortStateMachineCall(portInfoCache, msg);
}

static void OnPortFindServiceByUuid(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_Errcode_E ret)
{
    SDF_UNUSED(uuid);
    NLSTK_LOG_INFO("[PORT] OnPortFindServiceByUuid enter");
    PortInfoCache_S *portInfoCache = PortFindInfoCacheByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(portInfoCache != NULL, "[PORT] portInfoCache not found");
    PortStmParam_S msg = { .what = PORT_ON_FIND_SERVICE, .extData = (void *)&ret };
    PortStateMachineCall(portInfoCache, msg);
}

static void OnReadPortProperty(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_DEBUG("[PORT] OnReadPortProperty enter");
    PortInfoCache_S *portInfoCache = PortFindInfoCacheByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(portInfoCache != NULL, "[PORT] portInfoCache not found");
    PortReadPropertyMsg_S readMsg = { .property = property, .ret = ret };
    PortStmParam_S msg = { .what = PORT_ON_READ_PROPERTY, .extData = (void *)&readMsg };
    PortStateMachineCall(portInfoCache, msg);
}

static void OnPortSetPropertyNtf(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable,
    NLSTK_Errcode_E ret)
{
    SDF_UNUSED(uuid);
    NLSTK_LOG_DEBUG("[PORT] OnPortSetPropertyNtf enter");
    PortInfoCache_S *portInfoCache = PortFindInfoCacheByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(portInfoCache != NULL, "[PORT] portInfoCache not found");
    PortSetPropertyNtfMsg_S ntfMsg = { .handle = handle, .enable = enable, .ret = ret };
    PortStmParam_S msg = { .what = PORT_ON_SET_NTF, .extData = (void *)&ntfMsg };
    PortStateMachineCall(portInfoCache, msg);
}

static void OnPortPropertyChanged(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    NLSTK_LOG_DEBUG("[PORT] OnPortPropertyChanged enter");
    PortInfoCache_S *portInfoCache = PortFindInfoCacheByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(portInfoCache != NULL, "[PORT] portInfoCache not found");
    PortReadPropertyMsg_S changedMsg = { .property = property, .ret = NLSTK_ERRCODE_SUCCESS };
    PortStmParam_S msg = { .what = PORT_ON_PROPERTY_CHANGED, .extData = (void *)&changedMsg };
    PortStateMachineCall(portInfoCache, msg);
}

static void PortIdleStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，此时还未注册AppId，进入注册AppId流程
        case PORT_ON_USER_CONNECT: {
            PORT_STM_STATE_CHANGE(portInfoCache, PORT_STATE_REGISTER_APP);
            PortStateChangeCbk(&portInfoCache->addr, PORT_CONNECTING, PORT_DISCONNECTED, NLSTK_ERRCODE_SUCCESS);
            PortStmParam_S newMsg = { .what = PORT_NEW_STATE_ENTRY, .extData = NULL };
            PortStateMachineCall(portInfoCache, newMsg);
            break;
        }
        default:
            break;
    }
}

static void PortRegisterAppStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，执行注册AppId流程
        case PORT_NEW_STATE_ENTRY: {
            NLSTK_SsapAppClientCb_S cbk = {0};
            cbk.onReadProperty = &OnReadPortProperty;
            cbk.onConnectionStateChanged = &OnPortConnectionStateChanged;
            cbk.onFindServiceByUuid = &OnPortFindServiceByUuid;
            cbk.onRegisterApp = &OnPortRegisterApp;
            cbk.onGetServices = &OnPortGetServiceByUuid;
            cbk.onSetPropertyNtf = &OnPortSetPropertyNtf;
            cbk.onPropertyChanged = &OnPortPropertyChanged;
            NLSTK_Errcode_E ret = NLSTK_SsapClientRegAppAsyn(&portInfoCache->addr, &portInfoCache->connParam, &cbk);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                PORT_STM_STATE_CHANGE(portInfoCache, PORT_STATE_IDLE);
                PortStateChangeCbk(&portInfoCache->addr, PORT_DISCONNECTED, PORT_CONNECTING, ret);
                PortInfoCacheRemove(&portInfoCache->addr);
            }
            break;
        }
        // 收到注册AppId成功事件，说明上一次用户调用了连接，继续执行连接流程
        case PORT_ON_REGISTER_APP: {
            int32_t appId = *(int32_t *)msg.extData;
            if (appId < 0) {
                NLSTK_LOG_ERROR("[PORT] invalid appId");
                PortStateChangeCbk(&portInfoCache->addr, PORT_DISCONNECTED, PORT_CONNECTING, NLSTK_ERRCODE_PARAM_ERR);
                PortInfoCacheRemove(&portInfoCache->addr);
                return;
            }
            portInfoCache->appId = appId;
            PORT_STM_STATE_CHANGE(portInfoCache, PORT_STATE_CREATE_LINK);
            PortStmParam_S newMsg = { .what = PORT_NEW_STATE_ENTRY, .extData = NULL };
            PortStateMachineCall(portInfoCache, newMsg);
            break;
        }
        default:
            break;
    }
}

static void PortSwitchToDisconnected(PortInfoCache_S *portInfo, NLSTK_PortConnectState_E preState, NLSTK_Errcode_E ret)
{
    PortStateChangeCbk(&portInfo->addr, PORT_DISCONNECTED, preState, ret);
    NLSTK_SsapClientDeregAppAsync(portInfo->appId);
    PortInfoCacheRemove(&portInfo->addr);
}

static void PortSwitchToDisconnecting(PortInfoCache_S *portInfo, NLSTK_PortConnectState_E preState, NLSTK_Errcode_E ret)
{
    if (NLSTK_SsapClientDisconnect(portInfo->appId) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[PORT] disconnect fail");
    }
    PortSwitchToDisconnected(portInfo, preState, ret);
}

static void PortCreateLinkStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，执行建链流程
        case PORT_NEW_STATE_ENTRY: {
            NLSTK_Errcode_E ret = NLSTK_SsapClientConnect(portInfoCache->appId);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                PortSwitchToDisconnected(portInfoCache, PORT_CONNECTING, ret);
            }
            break;
        }
        // 收到底层链路状态变化回调，若为已连接，继续执行profile连接流程：获取Port服务
        case PORT_ON_STATE_CHANGED: {
            PortConnStateMsg_S *connStateMsg = (PortConnStateMsg_S *)msg.extData;
            if (connStateMsg->state == SSAP_CONNECT_STATE_CONNECTED) {
                PORT_STM_STATE_CHANGE(portInfoCache, PORT_STATE_GET_SERVICE);
                PortStmParam_S newMsg = { .what = PORT_NEW_STATE_ENTRY, .extData = NULL };
                PortStateMachineCall(portInfoCache, newMsg);
            } else if (connStateMsg->state == SSAP_CONNECT_STATE_DISCONNECTED) {
                PortSwitchToDisconnected(portInfoCache, PORT_CONNECTING, connStateMsg->ret);
            }
            break;
        }
        // 收到用户断开连接事件
        case PORT_ON_USER_DISCONNECT: {
            PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, NLSTK_ERRCODE_SUCCESS);
            break;
        }
        default:
            break;
    }
}

static void PortHandleOnGetService(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    PortGetServiceMsg_S *getServiceMsg = (PortGetServiceMsg_S *)msg.extData;
    if (getServiceMsg == NULL || getServiceMsg->service == NULL || getServiceMsg->serviceNum == 0) {
        if (portInfoCache->findFlag == false) {
            portInfoCache->findFlag = true;
            PORT_STM_STATE_CHANGE(portInfoCache, PORT_STATE_FIND_SERVICE);
            PortStmParam_S newMsg = { .what = PORT_NEW_STATE_ENTRY, .extData = NULL };
            PortStateMachineCall(portInfoCache, newMsg);
        } else {
            NLSTK_LOG_ERROR("[PORT] get service inexist");
            PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, NLSTK_ERRCODE_FAIL);
        }
    } else {
        PORT_STM_STATE_CHANGE(portInfoCache, PORT_STATE_READ_PROPERTY);
        PortGetServiceMsg_S getMsg = { .serviceNum = getServiceMsg->serviceNum,
            .service = getServiceMsg->service, .func = getServiceMsg->func };
        PortStmParam_S newMsg = { .what = PORT_NEW_STATE_ENTRY, .extData = (void *)&getMsg };
        PortStateMachineCall(portInfoCache, newMsg);
    }
}

static void PortGetServiceStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，执行获取服务流程
        case PORT_NEW_STATE_ENTRY: {
            NLSTK_Errcode_E ret = NLSTK_SsapClientGetServicesByUuidAsyn(portInfoCache->appId, &g_portServiceStdUuid);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[PORT] get service fail");
                PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, ret);
            }
            break;
        }
        // 收到获取服务回调，解析服务属性句柄，进入读取属性流程
        case PORT_ON_GET_SERVICE: {
            PortHandleOnGetService(portInfoCache, msg);
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case PORT_ON_STATE_CHANGED: {
            PortConnStateMsg_S *connStateMsg = (PortConnStateMsg_S *)msg.extData;
            if (connStateMsg->state == SSAP_CONNECT_STATE_DISCONNECTED) {
                PortSwitchToDisconnected(portInfoCache, PORT_CONNECTING, connStateMsg->ret);
            }
            break;
        }
        // 收到用户断开连接事件
        case PORT_ON_USER_DISCONNECT: {
            PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, NLSTK_ERRCODE_SUCCESS);
            break;
        }
        default:
            break;
    }
}

static void PortFindServiceStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，执行获取服务流程
        case PORT_NEW_STATE_ENTRY: {
            NLSTK_Errcode_E ret = NLSTK_SsapClientDiscoverServicesByUuid(portInfoCache->appId, &g_portServiceStdUuid,
                0x0001, 0xFFFF, FIND_STRUCTURE_TYPE_PRIMARY_SERVICE); // 0x0001 ~ 0xFFFF handle范围
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[PORT] find service fail");
                PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, ret);
            }
            break;
        }
        // 收到获取服务回调，解析服务属性句柄，进入读取属性流程
        case PORT_ON_FIND_SERVICE: {
            NLSTK_Errcode_E ret = *(NLSTK_Errcode_E *)msg.extData;
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[PORT] find service fail");
                PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, ret);
            } else {
                PORT_STM_STATE_CHANGE(portInfoCache, PORT_STATE_GET_SERVICE);
                PortStmParam_S newMsg = { .what = PORT_NEW_STATE_ENTRY, .extData = NULL };
                PortStateMachineCall(portInfoCache, newMsg);
            }
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case PORT_ON_STATE_CHANGED: {
            PortConnStateMsg_S *connStateMsg = (PortConnStateMsg_S *)msg.extData;
            if (connStateMsg->state == SSAP_CONNECT_STATE_DISCONNECTED) {
                PortSwitchToDisconnected(portInfoCache, PORT_CONNECTING, connStateMsg->ret);
            }
            break;
        }
        // 收到用户断开连接事件
        case PORT_ON_USER_DISCONNECT: {
            PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, NLSTK_ERRCODE_SUCCESS);
            break;
        }
        default:
            break;
    }
}

static void PortFindAndReadProperty(PortInfoCache_S *portInfoCache, NLSTK_SsapServ_S *stackService)
{
    if (stackService->properties == NULL) {
        NLSTK_LOG_ERROR("[PORT] stackService->properties is null");
        PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, NLSTK_ERRCODE_POINTER_NULL);
        return;
    }
    for (size_t i = 0; i < stackService->propertyNum; i++) {
        if (memcmp(&stackService->properties[i].uuid, &g_portPropertyStdUuid, sizeof(NLSTK_SsapUuid_S)) == 0) {
            NLSTK_Errcode_E ret = NLSTK_SsapClientReadProperty(portInfoCache->appId,
                stackService->properties[i].handle);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[PORT] read property fail");
                PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, ret);
            }
            return;
        }
    }
    PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, NLSTK_ERRCODE_PARAM_ERR);
}

static void PortOnReadPropertyInReadPropertyState(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    PortReadPropertyMsg_S *readMsg = (PortReadPropertyMsg_S *)msg.extData;
    if (readMsg == NULL || readMsg->property == NULL) {
        NLSTK_LOG_ERROR("[PORT] read property fail");
        PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, NLSTK_ERRCODE_POINTER_NULL);
        return;
    }
    NLSTK_SsapClientReadPropertyInfo_S *propertyInfo = readMsg->property;
    if (propertyInfo->errorCode == NLSTK_ERRCODE_SUCCESS && propertyInfo->value.len == 0) {
        NLSTK_LOG_INFO("[PORT] read property property value is null");
        PORT_STM_STATE_CHANGE(portInfoCache, PORT_STATE_CONNECTED);
        PortStateChangeCbk(&portInfoCache->addr, PORT_CONNECTED, PORT_CONNECTING, NLSTK_ERRCODE_SUCCESS);
        return;
    }
    if (readMsg->ret != NLSTK_ERRCODE_SUCCESS || propertyInfo->value.len < sizeof(uint16_t) ||
        propertyInfo->value.data == NULL) {
        NLSTK_LOG_ERROR("[PORT] read property fail");
        PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, propertyInfo->errorCode);
        return;
    }
    uint8_t *buf = propertyInfo->value.data;
    uint16_t portNum = *(uint16_t *)buf;
    NLSTK_CHECK_RETURN_VOID(portNum <= PORT_MAX_NUM, "[PORT] portNum is too large");
    NLSTK_CHECK_RETURN_VOID(propertyInfo->value.len == (portNum * sizeof(PortPrivateInfo_S) + sizeof(uint16_t)),
        "[PORT] propertyInfo value len error");
    buf += sizeof(uint16_t);
    for (size_t i = 0; i < portNum; i++) {
        PortPrivateInfo_S *portInfo = (PortPrivateInfo_S *)SDF_MemZalloc(sizeof(PortPrivateInfo_S));
        NLSTK_CHECK_RETURN_VOID(portInfo, "[PORT] alloc portInfo failed");
        portInfo->portId = *(uint16_t *)buf;
        buf += sizeof(uint16_t);
        portInfo->manufactureId = *(uint16_t *)buf;
        buf += sizeof(uint16_t);
        (void)memcpy_s(&portInfo->uuid, sizeof(NLSTK_SsapUuid_S), buf, sizeof(NLSTK_SsapUuid_S));
        buf += sizeof(NLSTK_SsapUuid_S);
        NLSTK_LOG_INFO("[PORT] new read port id: %d, manufactureId: %d, uuid: %s, list size before: %zu",
            portInfo->portId, portInfo->manufactureId, SSAP_GET_ENC_UUID(&portInfo->uuid),
            portInfoCache->portInfoList->size);
        if (!SDF_VectorEmplaceBack(portInfoCache->portInfoList, portInfo)) {
            NLSTK_LOG_ERROR("[PORT] save port info fail");
            SDF_MemFree(portInfo);
        }
    }
    portInfoCache->propertyHandle = propertyInfo->handle;
    PORT_STM_STATE_CHANGE(portInfoCache, PORT_STATE_SET_NET);
    PortStmParam_S newMsg = { .what = PORT_NEW_STATE_ENTRY, .extData = NULL};
    PortStateMachineCall(portInfoCache, newMsg);
    return;
}

static void PortReadPropertyStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，执行获取读取属性流程
        case PORT_NEW_STATE_ENTRY: {
            PortGetServiceMsg_S *getServiceMsg = (PortGetServiceMsg_S *)msg.extData;
            NLSTK_SsapServ_S *stackService = getServiceMsg->service;
            for (size_t i = 0; i < getServiceMsg->serviceNum; i++) {
                PortFindAndReadProperty(portInfoCache, &stackService[i]);
            }
            if (getServiceMsg->func != NULL) {
                getServiceMsg->func(stackService, getServiceMsg->serviceNum);
            }
            break;
        }
        // 收到读取属性回调，存储属性值，若读取属性完成，进入设置属性通知流程
        case PORT_ON_READ_PROPERTY: {
            PortOnReadPropertyInReadPropertyState(portInfoCache, msg);
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case PORT_ON_STATE_CHANGED: {
            PortConnStateMsg_S *connStateMsg = (PortConnStateMsg_S *)msg.extData;
            if (connStateMsg->state == SSAP_CONNECT_STATE_DISCONNECTED) {
                PortSwitchToDisconnected(portInfoCache, PORT_CONNECTING, connStateMsg->ret);
            }
            break;
        }
        // 收到用户断开连接事件
        case PORT_ON_USER_DISCONNECT: {
            PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, NLSTK_ERRCODE_SUCCESS);
            break;
        }
        default:
            break;
    }
}

static void PortOnSetPropertyNtfInSetNtfState(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    PortSetPropertyNtfMsg_S *ntfMsg = (PortSetPropertyNtfMsg_S *)msg.extData;
    // 由于老版本的port server端不支持订阅，所以此处port订阅失败不认为port profile连接失败，仅作打印辅助定位
    if (ntfMsg == NULL || !ntfMsg->enable || ntfMsg->ret != NLSTK_ERRCODE_SUCCESS ||
        ntfMsg->handle != portInfoCache->propertyHandle) {
        NLSTK_LOG_ERROR("[PORT] set property ntf fail");
    }
    PORT_STM_STATE_CHANGE(portInfoCache, PORT_STATE_CONNECTED);
    PortStateChangeCbk(&portInfoCache->addr, PORT_CONNECTED, PORT_CONNECTING, NLSTK_ERRCODE_SUCCESS);
    return;
}

static void PortSetNtfStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    switch (msg.what) {
        // 收到用户发起连接事件，执行获取读取属性流程
        case PORT_NEW_STATE_ENTRY: {
            NLSTK_Errcode_E ret =
                NLSTK_SsapClientSetPropertyNtf(portInfoCache->appId, portInfoCache->propertyHandle, true);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[PORT] set property ntf fail");
                PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, ret);
                return;
            }
            break;
        }
        // 收到读取属性回调，存储属性值，若读取属性完成，进入设置属性通知流程
        case PORT_ON_SET_NTF: {
            PortOnSetPropertyNtfInSetNtfState(portInfoCache, msg);
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case PORT_ON_STATE_CHANGED: {
            PortConnStateMsg_S *connStateMsg = (PortConnStateMsg_S *)msg.extData;
            if (connStateMsg->state == SSAP_CONNECT_STATE_DISCONNECTED) {
                PortSwitchToDisconnected(portInfoCache, PORT_CONNECTING, connStateMsg->ret);
            }
            break;
        }
        // 收到用户断开连接事件
        case PORT_ON_USER_DISCONNECT: {
            PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTING, NLSTK_ERRCODE_SUCCESS);
            break;
        }
        default:
            break;
    }
}

static void PortOnPropertyChangedInConnectedState(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    PortReadPropertyMsg_S *readMsg = (PortReadPropertyMsg_S *)msg.extData;
    if (readMsg == NULL || readMsg->property == NULL) {
        NLSTK_LOG_ERROR("[PORT] property ntf changed fail");
        return;
    }
    NLSTK_SsapClientReadPropertyInfo_S *propertyInfo = readMsg->property;
    if (propertyInfo->value.len < sizeof(uint16_t) || propertyInfo->value.data == NULL) {
        NLSTK_LOG_ERROR("[PORT] property ntf changed fail");
        return;
    }
    uint8_t *buf = propertyInfo->value.data;
    uint16_t portNum = *(uint16_t *)buf;
    NLSTK_CHECK_RETURN_VOID(portNum <= PORT_MAX_NUM, "[PORT] portNum is too large");
    NLSTK_CHECK_RETURN_VOID(propertyInfo->value.len == (portNum * sizeof(PortPrivateInfo_S) + sizeof(uint16_t)),
        "[PORT] propertyInfo value len error");
    buf += sizeof(uint16_t);
    SDF_CleanVector(portInfoCache->portInfoList);
    for (size_t i = 0; i < portNum; i++) {
        PortPrivateInfo_S *portInfo = (PortPrivateInfo_S *)SDF_MemZalloc(sizeof(PortPrivateInfo_S));
        NLSTK_CHECK_RETURN_VOID(portInfo, "[PORT] alloc portInfo failed");
        portInfo->portId = *(uint16_t *)buf;
        buf += sizeof(uint16_t);
        portInfo->manufactureId = *(uint16_t *)buf;
        buf += sizeof(uint16_t);
        (void)memcpy_s(&portInfo->uuid, sizeof(NLSTK_SsapUuid_S), buf, sizeof(NLSTK_SsapUuid_S));
        buf += sizeof(NLSTK_SsapUuid_S);
        NLSTK_LOG_INFO("[PORT] new change port id: %d, manufactureId: %d, uuid: %s, list size before: %zu",
            portInfo->portId, portInfo->manufactureId, SSAP_GET_ENC_UUID(&portInfo->uuid),
            portInfoCache->portInfoList->size);
        if (!SDF_VectorEmplaceBack(portInfoCache->portInfoList, portInfo)) {
            NLSTK_LOG_ERROR("[PORT] save port info fail");
            SDF_MemFree(portInfo);
        }
    }
    return;
}

static void PortConnectedStateDispatch(PortInfoCache_S *portInfoCache, PortStmParam_S msg)
{
    switch (msg.what) {
        case PORT_ON_PROPERTY_CHANGED: {
            PortOnPropertyChangedInConnectedState(portInfoCache, msg);
            break;
        }
        // 收到用户发起连接事件，此时连接流程已全部完成，向上层回调状态为已连接
        case PORT_ON_USER_CONNECT: {
            PortStateChangeCbk(&portInfoCache->addr, PORT_CONNECTED, PORT_CONNECTED, NLSTK_ERRCODE_SUCCESS);
            break;
        }
        // 收到用户断开连接事件
        case PORT_ON_USER_DISCONNECT: {
            PortSwitchToDisconnecting(portInfoCache, PORT_CONNECTED, NLSTK_ERRCODE_SUCCESS);
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case PORT_ON_STATE_CHANGED: {
            PortConnStateMsg_S *connStateMsg = (PortConnStateMsg_S *)msg.extData;
            if (connStateMsg->state == SSAP_CONNECT_STATE_DISCONNECTED) {
                PortSwitchToDisconnected(portInfoCache, PORT_CONNECTED, connStateMsg->ret);
            }
            break;
        }
        default:
            break;
    }
}