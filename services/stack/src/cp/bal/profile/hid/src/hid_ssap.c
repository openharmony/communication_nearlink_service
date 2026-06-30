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
#include "nlstk_log.h"
#include "nlstk_ssap_app_client.h"
#include "hid_type.h"
#include "hid_utils.h"
#include "hid_common.h"
#include "hid_stm.h"
#include "hid_ssap.h"

static void HidFindServicesByUuidCb(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_Errcode_E ret);
static void HidConnectStateChangeCb(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason);
static void HidReadPropertyCb(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret);
static void HidGetPropertyNtfCb(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable,
    NLSTK_Errcode_E ret);
static void HidSetPropertyNtfCb(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable,
    NLSTK_Errcode_E ret);
static void HidPropertyChangedCb(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property);
static void HidWritePropertyCb(int32_t appId, NLSTK_SsapClientWritePropertyInfo_S *property, NLSTK_Errcode_E ret);
static void HidRegisterAppCb(int32_t appId, SLE_Addr_S *addr, NLSTK_Errcode_E ret);
static void HidGetServiceCb(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_SsapServ_S *service, uint16_t serviceNum,
                             NLSTK_SsapClientFreeFunc func);

NLSTK_SsapAppClientCb_S HidGetSsapCbk(void)
{
    NLSTK_SsapAppClientCb_S cbk = {
        .onFindServiceByUuid = HidFindServicesByUuidCb,
        .onConnectionStateChanged = HidConnectStateChangeCb,
        .onReadProperty = HidReadPropertyCb,
        .onGetPropertyNtf = HidGetPropertyNtfCb,
        .onSetPropertyNtf = HidSetPropertyNtfCb,
        .onPropertyChanged = HidPropertyChangedCb,
        .onWriteProperty = HidWritePropertyCb,
        .onRegisterApp = HidRegisterAppCb,
        .onGetServices = HidGetServiceCb,
    };
    return cbk;
}

static void HidFindServicesByUuidCb(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[HID] enter HidFindServicesByUuidCb");
    HidDevice_S *dev = HidFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
}

static void HidConnectStateChangeCb(int32_t appId, uint8_t state, NLSTK_Errcode_E ret, int32_t reason)
{
    NLSTK_LOG_INFO("[HID] enter HidConnectStateChangeCb");
    HidDevice_S *dev = HidFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
    HidStmParam_S msg = { .what = HID_ON_STATE_CHANGED, .extData = (void *)&state };
    HidStateMachineCall(dev, msg);
}

static void HidReadPropertyCb(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[HID] enter HidReadPropertyCb");
    HidDevice_S *dev = HidFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
    HidReadPropertyMsg_S readMsg = { .property = property, .ret = ret };
    HidStmParam_S msg = { .what = HID_ON_READ_PROPERTY, .extData = (void *)&readMsg };
    HidStateMachineCall(dev, msg);
}

static void HidGetPropertyNtfCb(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable,
    NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[HID] enter HidGetPropertyNtfCb");
    HidDevice_S *dev = HidFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
}

static void HidSetPropertyNtfCb(int32_t appId, NLSTK_SsapUuid_S *uuid, uint16_t handle, bool enable,
    NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[HID] enter HidSetPropertyNtfCb");
    HidDevice_S *dev = HidFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
    HidSetPropertyNtfMsg_S setNtfMsg = { .uuid = uuid, .handle = handle, .enable = enable, .ret = ret };
    HidStmParam_S msg = { .what = HID_ON_SET_NOTIFICATION, .extData = (void *)&setNtfMsg };
    HidStateMachineCall(dev, msg);
}

static void HidPropertyChangedCb(int32_t appId, NLSTK_SsapClientReadPropertyInfo_S *property)
{
    HidDevice_S *dev = HidFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
    HidStmParam_S msg = { .what = HID_ON_NOTIFY_PROPERTY, .extData = (void *)property };
    HidStateMachineCall(dev, msg);
}

static void HidWritePropertyCb(int32_t appId, NLSTK_SsapClientWritePropertyInfo_S *property, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[HID] enter HidWritePropertyCb");
    HidDevice_S *dev = HidFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
    HidWritePropertyMsg_S WriteMsg = { .property = property, .ret = ret };
    HidStmParam_S msg = { .what = HID_ON_WRITE_PROPERTY, .extData = (void *)&WriteMsg };
    HidStateMachineCall(dev, msg);
}

static void HidRegisterAppCb(int32_t appId, SLE_Addr_S *addr, NLSTK_Errcode_E ret)
{
    NLSTK_LOG_INFO("[HID] enter HidRegisterAppCb");
    HidDevice_S *dev = HidFindDeviceByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
    HidStmParam_S msg = { .what = HID_ON_REGISTER_APP, .extData = (void *)&appId };
    HidStateMachineCall(dev, msg);
}

static void HidGetServiceCb(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_SsapServ_S *service, uint16_t serviceNum,
                             NLSTK_SsapClientFreeFunc func)
{
    NLSTK_LOG_INFO("[HID] enter HidGetServiceCb");
    HidDevice_S *dev = HidFindDeviceByAppId(appId);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
    HidGetServiceMsg_S getServiceMsg = { .serviceNum = serviceNum, .service = service, .func = func };
    HidStmParam_S msg = { .what = HID_ON_GET_SERVICE, .extData = (void *)&getServiceMsg };
    HidStateMachineCall(dev, msg);
}