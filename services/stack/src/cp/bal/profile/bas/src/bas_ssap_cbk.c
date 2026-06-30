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
#include "bas_common.h"
#include "bas_ssap_cbk.h"
#include "bas_stm.h"
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_public_define.h"

static void BasRegisterAppCbk(int32_t appId, SLE_Addr_S *addr, NLSTK_Errcode_E ret);
static void BasConnectStateChangeCbk(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason);
static void BasGetServicesCbk(int32_t appId, NLSTK_SsapUuid_S *uuid,
    NLSTK_SsapServ_S *service, uint16_t serviceNum, NLSTK_SsapClientFreeFunc func);
static void BasReadPropertyCbk(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret);
static void BasPropertyChangedCbk(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property);
static void BasSetPropertyNtfCbk(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable,
    NLSTK_Errcode_E ret);

NLSTK_SsapAppClientCb_S BasGetSsapCbk(void)
{
    NLSTK_SsapAppClientCb_S cbk = {
        .onRegisterApp = BasRegisterAppCbk,                    // NLSTK_SsapClientRegAppAsyn cbk
        .onConnectionStateChanged = BasConnectStateChangeCbk,  // NLSTK_SsapClientConnect cbk
        .onGetServices = BasGetServicesCbk,                    // NLSTK_SsapClientGetServicesAsyn cbk
        .onReadProperty = BasReadPropertyCbk,                  // NLSTK_SsapClientReadProperty cbk
        .onPropertyChanged = BasPropertyChangedCbk,            // 对端主动上报property变化回调
        .onSetPropertyNtf = BasSetPropertyNtfCbk,              // NLSTK_SsapClientSetPropertyNtf cbk
    };
    return cbk;
}

static void BasRegisterAppCbk(int32_t appId, SLE_Addr_S *addr, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[BAS] enter BasRegisterAppCbk");
    BasDeviceInfo_S *devInfo = BasFindDeviceInfo(addr);
    NLSTK_CHECK_RETURN_VOID(devInfo != NULL, "[BAS] devInfo not found");
    BasStmParam_S msg = {.what = BAS_ON_REGISTERED_CBK, .extData = (void *)&appId};
    BasClientStmCall(devInfo, msg);
}

static void BasConnectStateChangeCbk(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason)
{
    NLSTK_LOG_INFO("[BAS] enter BasConnectStateChangeCbk");
    BasDeviceInfo_S *devInfo = BasFindDeviceInfoByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(devInfo != NULL, "[BAS] devInfo not found");
    BasStmParam_S msg = {.what = BAS_ON_LINK_STATE_CHANGE, .extData = (void *)&state};
    BasClientStmCall(devInfo, msg);
}

static void BasGetServicesCbk(int32_t appId, NLSTK_SsapUuid_S *uuid,
    NLSTK_SsapServ_S *service, uint16_t serviceNum, NLSTK_SsapClientFreeFunc func)
{
    NLSTK_LOG_INFO("[BAS] enter BasGetServicesCbk");
    BasDeviceInfo_S *devInfo = BasFindDeviceInfoByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(devInfo != NULL, "[BAS] devInfo not found");
    BasServiceMsg_S serviceMsg = {.serviceNum = serviceNum, .service = service, .func = func};
    BasStmParam_S msg = {.what = BAS_ON_GET_SERVICE_CBK, .extData = (void *)&serviceMsg};
    BasClientStmCall(devInfo, msg);
}

static void BasReadPropertyCbk(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[BAS] enter BasReadPropertyCbk");
    BasDeviceInfo_S *devInfo = BasFindDeviceInfoByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(devInfo != NULL, "[BAS] dev not found");
    BasReadPropertyMsg_S readMsg = {.property = property, .ret = ret};
    BasStmParam_S msg = {.what = BAS_ON_READ_PROPERTY, .extData = (void *)&readMsg};
    BasClientStmCall(devInfo, msg);
}

static void BasPropertyChangedCbk(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    NLSTK_LOG_INFO("[BAS] enter BasPropertyChangedCbk");
    BasDeviceInfo_S *devInfo = BasFindDeviceInfoByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(devInfo != NULL, "[BAS] dev not found");
    BasStmParam_S msg = {.what = BAS_ON_NOTIFY_PROPERTY, .extData = (void *)property};
    BasClientStmCall(devInfo, msg);
}

static void BasSetPropertyNtfCbk(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable,
    NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[BAS] enter BasSetPropertyNtfCbk");
    BasDeviceInfo_S *dev = BasFindDeviceInfoByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[BAS] dev not found");
    BasSetPropertyNtfMsg_S setNtfMsg = { .uuid = uuid, .handle = handle, .enable = enable, .ret = ret };
    BasStmParam_S msg = { .what = BAS_ON_SET_NOTIFICATION, .extData = (void *)&setNtfMsg };
    BasClientStmCall(dev, msg);
}