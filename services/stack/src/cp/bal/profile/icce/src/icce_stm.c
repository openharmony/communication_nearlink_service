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
#include "nlstk_log.h"
#include "sdf_addr.h"
#include "ssap_type.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_ssap_app_client.h"
#include "icce_utils.h"
#include "icce_common.h"
#include "icce_stm.h"

typedef void (*IcceStmHandle)(IcceDevice_S *icceInfo, IcceStmParam param);

static void IcceHandleEvcInUnregState(IcceDevice_S *icceInfo, IcceStmParam param);
static void IcceHandleEvcInRegState(IcceDevice_S *icceInfo, IcceStmParam param);
static void IcceHandleEvcInLinkDisconnectedState(IcceDevice_S *icceInfo, IcceStmParam param);
static void IcceHandleEvcInLinkConnectedState(IcceDevice_S *icceInfo, IcceStmParam param);
static void IcceHandleEvcInServiceFoundState(IcceDevice_S *icceInfo, IcceStmParam param);
static void IcceHandleEvcInConnectedState(IcceDevice_S *icceInfo, IcceStmParam param);

static void UpdateDeviceHandleInfo(NLSTK_SsapServ_S *service, IcceDevice_S *icceInfo)
{
    NLSTK_CHECK_RETURN_VOID(service != NULL, "[ICCE] get service is null");
    icceInfo->icceServiceHandle = service->handle;
    NLSTK_SsapPrty_S *property = service->properties;
    NLSTK_CHECK_RETURN_VOID(property != NULL, "[ICCE] get property is null");
    icceInfo->portHandle = property->handle;
}

static void IcceGetServicesByUuidCb(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_SsapServ_S *service,
    uint16_t serviceNum, NLSTK_SsapClientFreeFunc func)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceGetServicesByUuidCb");
    IcceDevice_S *icceInfo = IcceFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(icceInfo != NULL, "[ICCE] get dev info is null");
    IcceStmParam param = { .what = ICCE_ON_GET_SERVICE_CBK, .extData = NULL };
    // 因为ICCE是单例，所以这里只取第一个
    if (serviceNum > 0) {
        UpdateDeviceHandleInfo(service, icceInfo);
    } else {
        NLSTK_LOG_ERROR("[ICCE] get service num is 0");
    }
    if (func != NULL) {
        func(service, serviceNum);
    }
    IcceClientStmCall(icceInfo, param);
}

static void IcceConnectStateChangeCb(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceConnectStateChangeCb");
    IcceDevice_S *icceInfo = IcceFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(icceInfo != NULL, "[ICCE] get dev info is null");
    IcceStmParam param = {.what = ICCE_ON_LINK_STATE_CHANGE, .extData = (void *)&state, .reason = reason};
    IcceClientStmCall(icceInfo, param);
}

static void IcceRegisterAppCb(int32_t appId, SLE_Addr_S *addr, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceRegisterAppCb");
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[ICCE] addr is null");
    IcceDevice_S *icceInfo = IcceFindDeviceByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(icceInfo != NULL, "[ICCE] get dev info is null");
    IcceStmParam param = { .what = ICCE_ON_REGISTERED_CBK, .extData = (void *)&appId };
    IcceClientStmCall(icceInfo, param);
}

static uint32_t ConvertToUint32LittleEndian(NLSTK_VariableData_S *value, bool *error)
{
    if (value->len > ICCE_PORT_LEN) {
        *error = true;
        return 0;
    }

    uint32_t result = 0;
    for (int i = 0; i < value->len; i++) {
        result |= (value->data[i] << (0x08 * i));
    }

    *error = false;
    return result;
}

static void IcceReadPropertyCb(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[ICCE] enter IcceReadPropertyCb");
    NLSTK_CHECK_RETURN_VOID(property != NULL, "[ICCE] property is null");
    IcceDevice_S *icceInfo = IcceFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(icceInfo != NULL, "[ICCE] get dev info is null");

    IcceStmParam param = { .what = ICCE_ON_READ_PROPERTY_CBK, .extData = (void *)property };
    bool error = false;
    uint32_t port = ConvertToUint32LittleEndian(&property->value, &error);

    NLSTK_CHECK_RETURN_VOID(!error, "[ICCE] Property convert failed");
    icceInfo->devicesInfo.iccePort = port;
    IcceClientStmCall(icceInfo, param);
}

void IcceClientStmCall(IcceDevice_S *icceInfo, IcceStmParam param)
{
    NLSTK_CHECK_RETURN_VOID((icceInfo != NULL), "[ICCE] stm input is null");
    NLSTK_CHECK_RETURN_VOID(icceInfo->state < ICCE_STM_STATE_BUTT, "[ICCE] stm invalid input");
    NLSTK_LOG_INFO("[ICCE] app id(%d) start the stm, state is %d, msg is %d", icceInfo->appId,
        icceInfo->state, param.what);
    static IcceStmHandle stateMachine[ICCE_STM_STATE_BUTT] = {
        [ICCE_APPID_UNREGISTERED] = IcceHandleEvcInUnregState,
        [ICCE_APPID_REGISTERING] = IcceHandleEvcInRegState,
        [ICCE_LINK_DISCONNECTED] = IcceHandleEvcInLinkDisconnectedState,
        [ICCE_LINK_CONNECTED] = IcceHandleEvcInLinkConnectedState,
        [ICCE_SERVICE_FOUND] = IcceHandleEvcInServiceFoundState,
        [ICCE_DEV_CONNECTED] = IcceHandleEvcInConnectedState,
    };
    IcceStmHandle func = stateMachine[icceInfo->state];
    if (func == NULL) {
        NLSTK_LOG_INFO("[ICCE] stm func is null");
        return;
    }
    func(icceInfo, param);
}

static void IcceHandleEvcInUnregState(IcceDevice_S *icceInfo, IcceStmParam param)
{
    switch (param.what) {
        // UNREGISTERED状态，收到注册app的回调，状态变为REGISTERED,调用NLSTK_SsapClientRegAppAsyn
        case ICCE_ON_USER_CONNECT: {
            NLSTK_SsapAppClientCb_S cbk = {0};
            cbk.onGetServices = IcceGetServicesByUuidCb;
            cbk.onConnectionStateChanged = IcceConnectStateChangeCb;
            cbk.onReadProperty = IcceReadPropertyCb;
            cbk.onRegisterApp = IcceRegisterAppCb;
            NLSTK_ConnParam_S connParam = { 0 };
            if (NLSTK_SsapClientRegAppAsyn(&icceInfo->addr, &connParam, &cbk) == NLSTK_ERRCODE_SUCCESS) {
                if (IcceGetUserCbk()->connectStateChangeCbk != NULL) {
                    IcceGetUserCbk()->connectStateChangeCbk(&icceInfo->addr,
                        ICCE_CONNECTING, ICCE_DISCONNECTED, NLSTK_ERRCODE_SUCCESS);
                }
                icceInfo->state = ICCE_APPID_REGISTERING;
                NLSTK_LOG_INFO("[ICCE] state is ICCE_APPID_REGISTERING now");
            }
            break;
        }
        default:
            break;
    }
}

static void IcceHandleEvcInRegState(IcceDevice_S *icceInfo, IcceStmParam param)
{
    switch (param.what) {
        case ICCE_ON_REGISTERED_CBK: {
            int32_t appId = *(int32_t *)param.extData;
            icceInfo->appId = appId;
            if (NLSTK_SsapClientConnect(appId) != NLSTK_ERRCODE_SUCCESS) {
                if (IcceGetUserCbk()->connectStateChangeCbk != NULL) {
                    IcceGetUserCbk()->connectStateChangeCbk(&icceInfo->addr,
                        ICCE_DISCONNECTED, ICCE_CONNECTING, NLSTK_ERRCODE_TASK_FAIL);
                }
                icceInfo->state = ICCE_APPID_UNREGISTERED;
                return;
            }
            icceInfo->state = ICCE_LINK_DISCONNECTED;
            NLSTK_LOG_INFO("[ICCE] state is ICCE_LINK_DISCONNECTED now");
            break;
        }
        default:
            break;
    }
}

static void IcceHandleEvcInLinkDisconnectedState(IcceDevice_S *icceInfo, IcceStmParam param)
{
    switch (param.what) {
        case ICCE_ON_USER_CONNECT: {
            if (NLSTK_SsapClientConnect(icceInfo->appId) != NLSTK_ERRCODE_SUCCESS) {
                if (IcceGetUserCbk()->connectStateChangeCbk != NULL) {
                    IcceGetUserCbk()->connectStateChangeCbk(&icceInfo->addr,
                        ICCE_DISCONNECTED, ICCE_CONNECTING, NLSTK_ERRCODE_TASK_FAIL);
                }
            }
            break;
        }
        // CONNECTING状态，收到连接状态改变回调，状态变为LINK_CONNECTED,调用NLSTK_SsapClientGetServicesByUuid
        case ICCE_ON_LINK_STATE_CHANGE: {
            uint8_t state = *(uint8_t *)param.extData;
            NLSTK_SsapUuid_S uuid = IcceConvertUuidToStru(ICCE_SERVICE_UUID);
            if (state == SSAP_CONNECT_STATE_CONNECTED &&
                NLSTK_SsapClientGetServicesByUuidAsyn(icceInfo->appId, &uuid) == NLSTK_ERRCODE_SUCCESS) {
                icceInfo->state = ICCE_LINK_CONNECTED;
                NLSTK_LOG_INFO("[ICCE] state is ICCE_LINK_CONNECTED now");
            } else if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
                icceInfo->state = ICCE_LINK_DISCONNECTED;
                if (IcceGetUserCbk()->connectStateChangeCbk != NULL) {
                    IcceGetUserCbk()->connectStateChangeCbk(&icceInfo->addr,
                        ICCE_DISCONNECTED, ICCE_CONNECTING, (NLSTK_Errcode_E)param.reason);
                }
            }
            break;
        }
        default:
            break;
    }
}

static void IcceHandleEvcInLinkConnectedState(IcceDevice_S *icceInfo, IcceStmParam param)
{
    switch (param.what) {
        case ICCE_ON_GET_SERVICE_CBK: {
            icceInfo->state = ICCE_SERVICE_FOUND;
            NLSTK_LOG_ERROR("[ICCE] state is ICCE_SERVICE_FOUND now");
            uint32_t ret = NLSTK_SsapClientReadProperty(icceInfo->appId, icceInfo->portHandle);
            if (ret != NLSTK_ERRCODE_SUCCESS) {
                if (IcceGetUserCbk()->connectStateChangeCbk != NULL) {
                    IcceGetUserCbk()->connectStateChangeCbk(&icceInfo->addr,
                        ICCE_DISCONNECTED, ICCE_CONNECTING, NLSTK_ERRCODE_TASK_FAIL);
                }
            }
            break;
        }
        case ICCE_ON_LINK_STATE_CHANGE: {
            uint8_t state = *(uint8_t *)param.extData;
            if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
                icceInfo->state = ICCE_LINK_DISCONNECTED;
                if (IcceGetUserCbk()->connectStateChangeCbk != NULL) {
                    IcceGetUserCbk()->connectStateChangeCbk(&icceInfo->addr,
                        ICCE_DISCONNECTED, ICCE_CONNECTING, (NLSTK_Errcode_E)param.reason);
                }
            }
            break;
        }
        default:
            break;
    }
}

static void IcceHandleEvcInServiceFoundState(IcceDevice_S *icceInfo, IcceStmParam param)
{
    switch (param.what) {
        case ICCE_ON_READ_PROPERTY_CBK: {
            icceInfo->state = ICCE_DEV_CONNECTED;
            NLSTK_LOG_INFO("[ICCE] state is ICCE_DEV_CONNECTED now");
            if (IcceGetUserCbk()->connectStateChangeCbk != NULL) {
                IcceGetUserCbk()->connectStateChangeCbk(&icceInfo->addr,
                    ICCE_CONNECTED, ICCE_CONNECTING, NLSTK_ERRCODE_SUCCESS);
            }
            break;
        }
        case ICCE_ON_LINK_STATE_CHANGE: {
            uint8_t state = *(uint8_t *)param.extData;
            if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
                icceInfo->state = ICCE_LINK_DISCONNECTED;
                if (IcceGetUserCbk()->connectStateChangeCbk != NULL) {
                    IcceGetUserCbk()->connectStateChangeCbk(&icceInfo->addr,
                        ICCE_DISCONNECTED, ICCE_CONNECTING, (NLSTK_Errcode_E)param.reason);
                }
            }
            break;
        }
        default:
            break;
    }
}

static void IcceHandleEvcInConnectedState(IcceDevice_S *icceInfo, IcceStmParam param)
{
    switch (param.what) {
        case ICCE_ON_USER_DISCONNECT: {
            icceInfo->state = ICCE_LINK_DISCONNECTED; // 需要做断链操作
            if (IcceGetUserCbk()->connectStateChangeCbk != NULL) {
                IcceGetUserCbk()->connectStateChangeCbk(&icceInfo->addr, ICCE_DISCONNECTED,
                    ICCE_CONNECTED, NLSTK_ERRCODE_SUCCESS);
            }
            break;
        }
        case ICCE_ON_LINK_STATE_CHANGE: {
            uint8_t state = *(uint8_t *)param.extData;
            if (state == SSAP_CONNECT_STATE_DISCONNECTED) {
                icceInfo->state = ICCE_LINK_DISCONNECTED;
                if (IcceGetUserCbk()->connectStateChangeCbk != NULL) {
                    IcceGetUserCbk()->connectStateChangeCbk(&icceInfo->addr,
                        ICCE_DISCONNECTED, ICCE_CONNECTED, (NLSTK_Errcode_E)param.reason);
                }
            }
            break;
        }
        default:
            break;
    }
}