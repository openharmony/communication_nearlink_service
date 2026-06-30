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
#include "hid_stm.h"
#include "hid_type.h"
#include "hid_utils.h"
#include "hid_common.h"
#include "hid_client.h"
#include "hid_client_init.h"

NLSTK_Errcode_E HidEnable(void)
{
    NLSTK_LOG_INFO("[HID] enter HidEnable");
    NLSTK_CHECK_RETURN(HidDeviceInit(), NLSTK_ERRCODE_FAIL, "[HID] device init fail");
    return NLSTK_ERRCODE_SUCCESS;
}

void HidDisable(void)
{
    NLSTK_LOG_INFO("[HID] enter HidDisable");
    HidDeviceDeinit();
}

void HidRegClientCbkInner(void *arg)
{
    NLSTK_LOG_INFO("[HID] enter HidRegClientCbkInner");
    HidClientCallBack_S *cbk = (HidClientCallBack_S *)arg;
    NLSTK_CHECK_RETURN_VOID(cbk != NULL, "[HID] cbk is null");
    HidSetUserCbk(cbk);
}

void HidConnectInner(void *arg)
{
    NLSTK_LOG_INFO("[HID] enter HidConnectInner");
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[HID] addr is null");
    HidDevice_S *dev = HidFindDeviceByAddr(addr);
    if (dev == NULL) {
        dev = (HidDevice_S *)SDF_MemZalloc(sizeof(HidDevice_S));
        NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev malloc fail");
        (void)memcpy_s(&dev->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        dev->appId = HID_INVALID_APPID;
        dev->state = HID_STATE_UNINIT;
        SDF_Traits reportTraits = { .dtor = HidFreeReport };
        dev->report = SDF_CreateVector(reportTraits);
        if (dev->report == NULL) {
            NLSTK_LOG_ERROR("[HID] report vector create fail");
            SDF_MemFree(dev);
            return;
        }
        SDF_Traits handleTraits = { .dtor = SDF_MemFree };
        dev->service.indexHandle = SDF_CreateVector(handleTraits);
        if (dev->service.indexHandle == NULL) {
            NLSTK_LOG_ERROR("[HID] index handle vector create fail");
            SDF_DestroyVector(dev->report);
            SDF_MemFree(dev);
            return;
        }
        if (!HidAddDevice(dev)) {
            NLSTK_LOG_ERROR("[HID] add device fail");
            SDF_DestroyVector(dev->report);
            SDF_DestroyVector(dev->service.indexHandle);
            SDF_MemFree(dev);
            return;
        }
    }
    HidStmParam_S msg = { .what = HID_ON_USER_CONNECT, .extData = NULL };
    HidStateMachineCall(dev, msg);
}

void HidDisconnectInner(void *arg)
{
    NLSTK_LOG_INFO("[HID] enter HidDisconnectInner");
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[HID] addr is null");
    HidDevice_S *dev = HidFindDeviceByAddr(addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
    HidStmParam_S msg = { .what = HID_ON_USER_DISCONNECT, .extData = NULL };
    HidStateMachineCall(dev, msg);
}

void HidGetInformationInner(void *arg)
{
    NLSTK_LOG_INFO("[HID] enter HidGetInformationInner");
    HidGetInfoParam_S *param = (HidGetInfoParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HID] param is null");
    HidDevice_S *dev = HidFindDeviceByAddr(&param->addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
    HidStmParam_S msg = { .what = HID_ON_USER_GET_INFO, .extData = arg };
    HidStateMachineCall(dev, msg);
}

void HidReadPropertyInner(void *arg)
{
    NLSTK_LOG_INFO("[HID] enter HidReadPropertyInner");
    HidReadParam_S *param = (HidReadParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HID] param is null");
    HidDevice_S *dev = HidFindDeviceByAddr(&param->addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
    HidStmParam_S msg = { .what = HID_ON_USER_READ, .extData = arg };
    HidStateMachineCall(dev, msg);
}

void HidWritePropertyInner(void *arg)
{
    NLSTK_LOG_INFO("[HID] enter HidWritePropertyInner");
    HidWriteParam_S *param = (HidWriteParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HID] param is null");
    HidDevice_S *dev = HidFindDeviceByAddr(&param->addr);
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[HID] dev not found");
    HidStmParam_S msg = { .what = HID_ON_USER_WRITE, .extData = arg };
    HidStateMachineCall(dev, msg);
}

void HidGetConnectStateInner(void *arg)
{
    NLSTK_LOG_INFO("[HID] enter HidGetConnectStateInner");
    HidGetConnStateParam_S *param = (HidGetConnStateParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HID] param is null");
    HidDevice_S *dev = HidFindDeviceByAddr(&param->addr);
    if (dev == NULL) {
        param->state = HID_DISCONNECTED;
        return;
    }
    if (dev->state == HID_STATE_UNINIT) {
        param->state = HID_DISCONNECTED;
    } else if (dev->state == HID_STATE_CONNECTED) {
        param->state = HID_CONNECTED;
    } else {
        param->state = HID_CONNECTING;
    }
}

void HidGetConnectedDeviceInner(void *arg)
{
    NLSTK_LOG_INFO("[HID] enter HidGetConnectedDeviceInner");
    HidGetConnDevParam_S *param = (HidGetConnDevParam_S *)arg;
    NLSTK_CHECK_RETURN_VOID(param != NULL, "[HID] param is null");
    param->num = 0;
    param->freeFunc = SDF_MemFree;
    size_t num = HidGetDeviceNum(HID_STATE_CONNECTED);
    if (num == 0) {
        return;
    }
    param->addrs = (SLE_Addr_S *)SDF_MemZalloc(num * sizeof(SLE_Addr_S));
    NLSTK_CHECK_RETURN_VOID(param->addrs != NULL, "[HID] addrs malloc fail");
    if (!HidGetDevices(HID_STATE_CONNECTED, param->addrs, num)) {
        SDF_MemFree(param->addrs);
        param->addrs = NULL;
        return;
    }
    param->num = num;
}