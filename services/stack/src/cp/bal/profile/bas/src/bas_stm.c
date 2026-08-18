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
#include <stdint.h>
#include "bas_stm.h"
#include "bas_def.h"
#include "bas_type.h"
#include "bas_common.h"
#include "bas_ssap_cbk.h"
#include "nlstk_log.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_ssap_app_client.h"
#include "sdf_mem.h"
#include "ssap_type.h"

typedef struct {
    uint16_t handle;
    NLSTK_VariableData_S *info;
} HandleMapEntry;

typedef void (*BasStmDispatch)(BasDeviceInfo_S *devInfo, BasStmParam_S param);

static void BasUnregStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param);
static void BasRegisteringStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param);
static void BasLinkDisconnectedStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param);
static void BasLinkConnectStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param);
static void BasServiceFoundStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param);
static void BasReadPropertyStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param);
static void BasSetNotificationStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param);
static void BasConnectedStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param);
static void BasDisconnectedStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param);

void BasClientStmCall(BasDeviceInfo_S *devInfo, BasStmParam_S event)
{
    NLSTK_CHECK_RETURN_VOID((devInfo != NULL), "[BAS] stm input is null");
    NLSTK_CHECK_RETURN_VOID(devInfo->state < BAS_STM_STATE_BUTT, "[BAS] stm invalid input");
    NLSTK_LOG_INFO(
        "[BAS] app id(%d) start the stm, state is %d, event is %d", devInfo->appId, devInfo->state, event.what);
    static BasStmDispatch basStateMachine[BAS_STM_STATE_BUTT] = {
        [BAS_APPID_UNREGISTERED] = BasUnregStateDispatch,
        [BAS_APPID_REGISTERING] = BasRegisteringStateDispatch,
        [BAS_LINK_DISCONNECTED] = BasLinkDisconnectedStateDispatch,
        [BAS_LINK_CONNECTING] = BasLinkConnectStateDispatch,
        [BAS_SERVICE_FOUND] = BasServiceFoundStateDispatch,
        [BAS_READ_PROPERTY] = BasReadPropertyStateDispatch,
        [BAS_DEVICE_CONNECTED] = BasConnectedStateDispatch,
        [BAS_SET_NOTIFICATION] = BasSetNotificationStateDispatch,
        [BAS_DEVICE_DISCONNECTED] = BasDisconnectedStateDispatch,
    };
    BasStmDispatch stmDispatch = basStateMachine[devInfo->state];
    if (stmDispatch == NULL) {
        NLSTK_LOG_INFO("[BAS] stm dispatch is null");
        return;
    }
    stmDispatch(devInfo, event);
}

static void BasUnregStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    switch (param.what) {
        // 收到用户发起连接事件，此时还未注册AppId，进入注册AppId流程、
        case BAS_ON_USER_CONNECT: {
            BAS_DEVICE_STATE_CHANGE(devInfo, BAS_APPID_REGISTERING);
            BasStateChangeCbk(&devInfo->addr, BAS_CONNECTING, BAS_DISCONNECTED, NLSTK_ERRCODE_SUCCESS);
            BasStmParam_S newMsg = {.what = BAS_ON_USER_CONNECT, .extData = NULL};
            BasClientStmCall(devInfo, newMsg);
            break;
        }
        default:
            break;
    }
}

static void BasRegisteringStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    switch (param.what) {
        // 收到用户发起连接事件，执行注册AppId流程
        case BAS_ON_USER_CONNECT: {
            NLSTK_SsapAppClientCb_S cbk = BasGetSsapCbk();
            NLSTK_ConnParam_S connParam = {0};
            NLSTK_Errcode_E ret = NLSTK_SsapClientRegAppAsyn(&devInfo->addr, &connParam, &cbk);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_DISCONNECTED);
                BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTING, ret);
            }
            break;
        }
        // 收到appid注册成功回调，执行建链流程
        case BAS_ON_REGISTERED_CBK: {
            int32_t appId = *(int32_t *)param.extData;
            if (appId < 0) {
                NLSTK_LOG_ERROR("[BAS] invalid appId");
                BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_DISCONNECTED);
                BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTING, NLSTK_ERRCODE_PARAM_ERR);
                return;
            }
            devInfo->appId = appId;
            BAS_DEVICE_STATE_CHANGE(devInfo, BAS_LINK_DISCONNECTED);
            BasStateChangeCbk(&devInfo->addr, BAS_CONNECTING, BAS_DISCONNECTED, NLSTK_ERRCODE_SUCCESS);
            BasStmParam_S newMsg = {.what = BAS_ON_USER_CONNECT, .extData = NULL};
            BasClientStmCall(devInfo, newMsg);
            break;
        }
        default:
            break;
    }
}

static void BasLinkDisconnectedStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    switch (param.what) {
        // 收到用户发起连接事件，此时已注册AppId，进入建链流程
        case BAS_ON_USER_CONNECT: {
            BAS_DEVICE_STATE_CHANGE(devInfo, BAS_LINK_CONNECTING);
            BasStateChangeCbk(&devInfo->addr, BAS_CONNECTING, BAS_DISCONNECTED, NLSTK_ERRCODE_SUCCESS);
            BasStmParam_S newMsg = {.what = BAS_ON_USER_CONNECTING, .extData = NULL};
            BasClientStmCall(devInfo, newMsg);
            break;
        }
        case BAS_ON_USER_CONNECTING: {
            BAS_DEVICE_STATE_CHANGE(devInfo, BAS_LINK_CONNECTING);
            BasStmParam_S newMsg = {.what = BAS_ON_USER_CONNECTING, .extData = NULL};
            BasClientStmCall(devInfo, newMsg);
            break;
        }
        default:
            break;
    }
}

static void BasLinkConnectStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    switch (param.what) {
        case BAS_ON_USER_CONNECTING: {
            NLSTK_Errcode_E ret = NLSTK_SsapClientConnect(devInfo->appId);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_DISCONNECTED);
                BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTING, NLSTK_ERRCODE_TASK_FAIL);
            }
            break;
        }
        // 收到底层链路状态变化回调，若为已连接，继续执行profile连接流程：获取Bas服务
        case BAS_ON_LINK_STATE_CHANGE: {
            uint8_t state = *(uint8_t *)param.extData;
            if (state == SSAP_CONNECT_STATE_CONNECTED) {
                BAS_DEVICE_STATE_CHANGE(devInfo, BAS_SERVICE_FOUND);
                BasStmParam_S newMsg = {.what = BAS_ON_USER_CONNECTING, .extData = NULL};
                BasClientStmCall(devInfo, newMsg);
            } else if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
                BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_DISCONNECTED);
                BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTING, NLSTK_ERRCODE_SUCCESS);
            }
            break;
        }
        // 收到用户断开连接事件
        case BAS_ON_USER_DISCONNECT: {
            if (NLSTK_SsapClientDisconnect(devInfo->appId) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[BAS] disconnect fail");
            }
            break;
        }
        default:
            break;
    }
}

static void BasOnServiceFoundState(BasDeviceInfo_S *devInfo, BasStmParam_S msg)
{
    BasServiceMsg_S *getServiceMsg = (BasServiceMsg_S *)msg.extData;
    if (getServiceMsg == NULL || getServiceMsg->service == NULL || getServiceMsg->serviceNum == 0) {
        NLSTK_LOG_ERROR("[BAS] get service msg is null");
        BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_DISCONNECTED);
        BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTING, NLSTK_ERRCODE_POINTER_NULL);
        BasRemoveDeviceInfo(&devInfo->addr);
        return;
    }
    if (BasBuildService(devInfo, getServiceMsg->service, getServiceMsg->serviceNum) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BAS] bas service not found or create handle index fail");
        BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_DISCONNECTED);
        BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTING, NLSTK_ERRCODE_MALLOC_FAIL);
        BasRemoveDeviceInfo(&devInfo->addr);
    } else {
        BAS_DEVICE_STATE_CHANGE(devInfo, BAS_READ_PROPERTY);
        BasStmParam_S newMsg = {.what = BAS_ON_USER_CONNECTING, .extData = NULL};
        BasClientStmCall(devInfo, newMsg);
    }
    if (getServiceMsg->func != NULL) {
        getServiceMsg->func(getServiceMsg->service, getServiceMsg->serviceNum);
    }
}

static void BasOnStateChangedAfterCreateLinkState(BasDeviceInfo_S *devInfo, BasStmParam_S msg)
{
    uint8_t state = *(uint8_t *)msg.extData;
    if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
        BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_DISCONNECTED);
        BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTING, NLSTK_ERRCODE_SUCCESS);
        BasRemoveDeviceInfo(&devInfo->addr);
    }
}

static void BasServiceFoundStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    switch (param.what) {
        // CONNECTING状态，收到连接状态改变回调，状态变为CONNECTED,调用NLSTK_SsapClientGetServicesByUuid
        case BAS_ON_USER_CONNECTING: {
            if (NLSTK_SsapClientGetServicesAsyn(devInfo->appId) != NLSTK_ERRCODE_SUCCESS) {
                BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_DISCONNECTED);
                BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTING, (NLSTK_Errcode_E)param.reason);
                BasRemoveDeviceInfo(&devInfo->addr);
            }
            break;
        }
        // 收到获取服务回调，解析服务属性句柄，进入读取属性流程
        case BAS_ON_GET_SERVICE_CBK: {
            BasOnServiceFoundState(devInfo, param);
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case BAS_ON_LINK_STATE_CHANGE: {
            BasOnStateChangedAfterCreateLinkState(devInfo, param);
            break;
        }
        // 收到用户断开连接事件
        case BAS_ON_USER_DISCONNECT: {
            if (NLSTK_SsapClientDisconnect(devInfo->appId) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[HID] disconnect fail");
            }
            break;
        }
        default:
            break;
    }
}

static void BasOnUserConnectingInReadPropertyState(BasDeviceInfo_S *devInfo, BasStmParam_S msg)
{
    for (size_t i = 0; i < devInfo->indexHandle->size; i++) {
        uint16_t *handle = SDF_VectorElementAt(devInfo->indexHandle, i);
        int ret = NLSTK_SsapClientReadProperty(devInfo->appId, *handle);
        if (ret != NLSTK_ERRCODE_SUCCESS) {
            NLSTK_LOG_ERROR("[BAS] read report info fail");
            BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_DISCONNECTED);
            BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTING, ret);
            BasRemoveDeviceInfo(&devInfo->addr);
            return;
        }
        devInfo->lastReadHandle = *handle;
    }
}

static void UpdatePropertyInfo(NLSTK_VariableData_S *info, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    NLSTK_CHECK_RETURN_VOID(property->value.len <= BAS_PROPERTY_LENGTH, "[BAS] property len is over limit");
    info->data = (uint8_t *)SDF_MemZalloc(property->value.len);
    NLSTK_CHECK_RETURN_VOID(info->data != NULL, "[BAS] mem alloc failed");
    info->len = property->value.len;
    (void)memcpy_s(info->data, property->value.len, property->value.data, property->value.len);
}

static void BasOnReadPropertyInReadPropertyState(BasDeviceInfo_S *devInfo, BasStmParam_S msg)
{
    BasReadPropertyMsg_S *readMsg = (BasReadPropertyMsg_S *)msg.extData;
    if (readMsg == NULL || readMsg->ret != NLSTK_ERRCODE_SUCCESS || readMsg->property == NULL ||
        readMsg->property->value.data == NULL) {
        NLSTK_LOG_ERROR("[BAS] read property fail");
        BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_DISCONNECTED);
        BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTING, NLSTK_ERRCODE_POINTER_NULL);
        BasRemoveDeviceInfo(&devInfo->addr);
        return;
    }
    HandleMapEntry handleMap[] = {
        {devInfo->devHandleInfo.remainBatPctHdl, &devInfo->devDeviceInfo.remainBatPctInfo},
        {devInfo->devHandleInfo.remainBatHdl, &devInfo->devDeviceInfo.remainBatInfo},
        {devInfo->devHandleInfo.batCapacityHdl, &devInfo->devDeviceInfo.batCapacityInfo},
        {devInfo->devHandleInfo.batRateCapacityHdl, &devInfo->devDeviceInfo.batRateCapacityInfo},
        {devInfo->devHandleInfo.remainWorkingTimeHdl, &devInfo->devDeviceInfo.remainWorkingTime},
    };

    size_t handleMapSize = sizeof(handleMap) / sizeof(handleMap[0]);
    uint16_t propertyHandle = readMsg->property->handle;

    for (size_t i = 0; i < handleMapSize; i++) {
        if (propertyHandle == handleMap[i].handle) {
            UpdatePropertyInfo(handleMap[i].info, readMsg->property);
            if (propertyHandle == devInfo->devHandleInfo.remainBatPctHdl &&
                readMsg->property->value.len == BAS_MANDATORY_PROPERTY_LENGTH) {
                // 处理电池剩余容量占比信息
                NLSTK_VariableData_S *value = &devInfo->devDeviceInfo.remainBatPctInfo;
                value->len = readMsg->property->value.len;
                value->data = (uint8_t *)SDF_MemZalloc(value->len);
                NLSTK_CHECK_RETURN_VOID(value->data != NULL, "[BAS] malloc fail for remainBatPctInfo");
                (void)memcpy_s(value->data, value->len, readMsg->property->value.data, value->len);
            }
        }
    }
    if (readMsg->property->handle == devInfo->lastReadHandle) {
        BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_CONNECTED);
        BasStmParam_S newMsg = {.what = BAS_ON_USER_CONNECTING, .extData = NULL};
        BasClientStmCall(devInfo, newMsg);
    }
}

static void BasReadPropertyStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    switch (param.what) {
        // 收到用户发起连接事件，执行获取读取属性流程
        case BAS_ON_USER_CONNECTING: {
            BasOnUserConnectingInReadPropertyState(devInfo, param);
            break;
        }
        // 收到读取属性回调，存储属性值，若读取属性完成，进入设置属性通知流程
        case BAS_ON_READ_PROPERTY: {
            BasOnReadPropertyInReadPropertyState(devInfo, param);
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case BAS_ON_LINK_STATE_CHANGE: {
            BasOnStateChangedAfterCreateLinkState(devInfo, param);
            break;
        }
        // 收到用户断开连接事件
        case BAS_ON_USER_DISCONNECT: {
            if (NLSTK_SsapClientDisconnect(devInfo->appId) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[BAS] disconnect fail");
            }
            break;
        }
        default:
            break;
    }
}

static void BasOnUserConnectingInSetNotificationState(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    NLSTK_Errcode_E ret = NLSTK_SsapClientSetPropertyNtf(devInfo->appId, devInfo->devHandleInfo.remainBatPctHdl, true);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[BAS] set property ntf fail");
        BAS_DEVICE_STATE_CHANGE(devInfo, BAS_DEVICE_DISCONNECTED);
        BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTING, ret);
        BasRemoveDeviceInfo(&devInfo->addr);
        return;
    }
    devInfo->lastSetNtfHandle = devInfo->devHandleInfo.remainBatPctHdl;
}

static void BasOnSetNtfInSetNotificationState(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    BasSetPropertyNtfMsg_S *setNtfMsg = (BasSetPropertyNtfMsg_S *)param.extData;
    if (setNtfMsg == NULL || setNtfMsg->ret != NLSTK_ERRCODE_SUCCESS || setNtfMsg->enable == false) {
        NLSTK_LOG_ERROR("[BAS] set property ntf res error");
        return;
    }
}

static void BasOnNotifyPropertyInConnectedState(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    NLSTK_SsapClientReadPropertyInfo_S *ntfMsg = (NLSTK_SsapClientReadPropertyInfo_S *)param.extData;
    NLSTK_CHECK_RETURN_VOID(ntfMsg != NULL && ntfMsg->value.data != NULL,
        "[BAS] notify property msg is null");
    if (ntfMsg->handle == devInfo->devHandleInfo.remainBatPctHdl) {
        NLSTK_VariableData_S *value = &devInfo->devDeviceInfo.remainBatPctInfo;
        value->len = ntfMsg->value.len;
        if (value->len != BAS_MANDATORY_PROPERTY_LENGTH) {
            NLSTK_LOG_ERROR("[BAS] property data len is invalid");
            return;
        }
        value->data = (uint8_t *)SDF_MemZalloc(value->len);
        NLSTK_CHECK_RETURN_VOID(value->data != NULL, "[BAS] malloc fail for remainBatPctInfo");
        (void)memcpy_s(value->data, value->len, ntfMsg->value.data, value->len);
        BasNotifyCbk(&devInfo->addr, BAS_BATTERY_PERCENTAGE, value);
    }
}

static void BasOnReadPropertyInConnectedState(BasDeviceInfo_S *devInfo, BasStmParam_S msg)
{
    BasReadPropertyMsg_S *readMsg = (BasReadPropertyMsg_S *)msg.extData;
    NLSTK_CHECK_RETURN_VOID(readMsg != NULL && readMsg->property != NULL, "[BAS] read property msg is null");
    // 根据属性句柄匹配对应的字段
    if (readMsg->property->handle == devInfo->devHandleInfo.remainBatPctHdl && readMsg->ret == NLSTK_ERRCODE_SUCCESS) {
        // 处理电池剩余容量占比信息
        NLSTK_VariableData_S *value = &devInfo->devDeviceInfo.remainBatPctInfo;
        value->len = readMsg->property->value.len;
        if (value->len != BAS_MANDATORY_PROPERTY_LENGTH) {
            NLSTK_LOG_ERROR("[BAS] property data len is invalid");
            return;
        }
        value->data = (uint8_t *)SDF_MemZalloc(value->len);
        NLSTK_CHECK_RETURN_VOID(value->data != NULL, "[BAS] malloc fail for remainBatPctInfo");
        (void)memcpy_s(value->data, value->len, readMsg->property->value.data, value->len);
        BasReadCbk(&devInfo->addr, BAS_BATTERY_PERCENTAGE, value, NLSTK_ERRCODE_SUCCESS);
    } else {
        // 未匹配到句柄，返回错误
        NLSTK_LOG_ERROR("[BAS] property handle not supported");
        BasReadCbk(&devInfo->addr, BAS_UNSUPPORTED_PROPERTY, NULL, NLSTK_ERRCODE_FAIL);
    }
}

static void BasSetNotificationStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    switch (param.what) {
        // 收到用户发起连接事件，执行订阅通知流程
        case BAS_ON_USER_CONNECTING: {
            BasOnUserConnectingInSetNotificationState(devInfo, param);
            break;
        }
        // 收到订阅通知回调，若订阅全部完成且无异常，将状态切换为已连接
        case BAS_ON_SET_NOTIFICATION: {
            BasOnSetNtfInSetNotificationState(devInfo, param);
            break;
        }
        // 收到属性通知回调，向上层回调通知结果
        case BAS_ON_NOTIFY_PROPERTY: {
            BasOnNotifyPropertyInConnectedState(devInfo, param);
            break;
        }
        // 收到读取属性回调，向上层回调读取结果
        case BAS_ON_READ_PROPERTY: {
            BasOnReadPropertyInConnectedState(devInfo, param);
            break;
        }
        // 收到底层链路状态变化回调，若底层链路断连，向上层回调状态为未连接
        case BAS_ON_LINK_STATE_CHANGE: {
            BasOnStateChangedAfterCreateLinkState(devInfo, param);
            break;
        }
        // 收到用户断开连接事件
        case BAS_ON_USER_DISCONNECT: {
            if (NLSTK_SsapClientDisconnect(devInfo->appId) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[BAS] disconnect fail");
            }
            break;
        }
        default:
            break;
    }
}

static void BasConnectedStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    switch (param.what) {
        // 收到属性通知回调，向上层回调通知结果
        case BAS_ON_NOTIFY_PROPERTY: {
            BasOnNotifyPropertyInConnectedState(devInfo, param);
            break;
        }
        // 收到用户发起连接事件，此时连接流程已全部完成，向上层回调状态为已连接
        case BAS_ON_USER_CONNECTING: {
            BasStateChangeCbk(&devInfo->addr, BAS_CONNECTED, BAS_CONNECTING, NLSTK_ERRCODE_SUCCESS);
            BAS_DEVICE_STATE_CHANGE(devInfo, BAS_SET_NOTIFICATION);
            BasStmParam_S newMsg = {.what = BAS_ON_USER_CONNECTING, .extData = NULL};
            BasClientStmCall(devInfo, newMsg);
            break;
        }
        case BAS_ON_LINK_STATE_CHANGE: {
            uint8_t state = *(uint8_t *)param.extData;
            if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
                BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTED, NLSTK_ERRCODE_SUCCESS);
                if (NLSTK_SsapClientDisconnect(devInfo->appId) != NLSTK_ERRCODE_SUCCESS) {
                    NLSTK_LOG_ERROR("[BAS] disconnect fail");
                }
                BasRemoveDeviceInfo(&devInfo->addr);
            }
            break;
        }
        case BAS_ON_USER_DISCONNECT: {
            BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTED, NLSTK_ERRCODE_SUCCESS);
            if (NLSTK_SsapClientDisconnect(devInfo->appId) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[BAS] disconnect fail");
            }
            BasRemoveDeviceInfo(&devInfo->addr);
            break;
        }
        default:
            break;
    }
}

static void BasDisconnectedStateDispatch(BasDeviceInfo_S *devInfo, BasStmParam_S param)
{
    switch (param.what) {
        case BAS_ON_LINK_STATE_CHANGE: {
            BasOnStateChangedAfterCreateLinkState(devInfo, param);
            break;
        }
        case BAS_ON_USER_DISCONNECT: {
            BasStateChangeCbk(&devInfo->addr, BAS_DISCONNECTED, BAS_CONNECTED, NLSTK_ERRCODE_SUCCESS);
            if (NLSTK_SsapClientDisconnect(devInfo->appId) != NLSTK_ERRCODE_SUCCESS) {
                NLSTK_LOG_ERROR("[BAS] disconnect fail");
            }
            BasRemoveDeviceInfo(&devInfo->addr);
            break;
        }
        default:
            break;
    }
}