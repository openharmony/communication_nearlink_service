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
#include "nlstk_log.h"
#include "hid_utils.h"
#include "hid_common.h"

static HidClientCallBack_S g_hidClientCbk = {0};
static SDF_Vector_S *g_hidDevice = NULL;    // 当前未考虑最大规格

HidClientCallBack_S *HidGetUserCbk(void)
{
    return &g_hidClientCbk;
}

void HidSetUserCbk(HidClientCallBack_S *cbk)
{
    (void)memcpy_s(&g_hidClientCbk, sizeof(HidClientCallBack_S), cbk, sizeof(HidClientCallBack_S));
}

void HidReadCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value, NLSTK_Errcode_E ret)
{
    if (g_hidClientCbk.readPropertyCbk != NULL) {
        g_hidClientCbk.readPropertyCbk(addr, type, value, ret);
    }
}

void HidWriteCbk(SLE_Addr_S *addr, HidPropertyType_E type, NLSTK_Errcode_E ret)
{
    if (g_hidClientCbk.writePropertyCbk != NULL) {
        g_hidClientCbk.writePropertyCbk(addr, type, ret);
    }
}

void HidNotifyCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value)
{
    if (g_hidClientCbk.notifyPropertyCbk != NULL) {
        g_hidClientCbk.notifyPropertyCbk(addr, type, value);
    }
}

void HidStateChangeCbk(SLE_Addr_S *addr, HidConnectState_E state, HidConnectState_E preState, NLSTK_Errcode_E ret)
{
    if (g_hidClientCbk.connectStateChangeCbk != NULL) {
        g_hidClientCbk.connectStateChangeCbk(addr, state, preState, ret);
    }
}

bool HidDeviceInit(void)
{
    if (g_hidDevice != NULL) {
        SDF_DestroyVector(g_hidDevice);
        g_hidDevice = NULL;
    }
    SDF_Traits traits = { .dtor = HidFreeDevice };
    g_hidDevice = SDF_CreateVector(traits);
    NLSTK_CHECK_RETURN(g_hidDevice != NULL, false, "[HID] g_hidDevice create fail");
    return true;
}

void HidDeviceDeinit(void)
{
    SDF_DestroyVector(g_hidDevice);
    g_hidDevice = NULL;
}

bool HidAddDevice(HidDevice_S *dev)
{
    NLSTK_CHECK_RETURN(dev != NULL && g_hidDevice != NULL, false, "[HID] dev or g_hidDevice is null");
    if (!SDF_VectorEmplaceBack(g_hidDevice, dev)) {
        NLSTK_LOG_ERROR("[HID] emplace back fail");
        return false;
    }
    return true;
}

void HidRemoveDevice(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN_VOID(addr != NULL && g_hidDevice != NULL, "[HID] addr or g_hidDevice is null");
    size_t index = 0;
    NLSTK_CHECK_RETURN_VOID(SDF_VectorFindFirst(g_hidDevice, HidCompAddr, addr, &index), "[HID] addr not found");
    SDF_VectorRemove(g_hidDevice, index);
}

size_t HidGetDeviceNum(uint8_t connState)
{
    NLSTK_CHECK_RETURN(g_hidDevice != NULL, 0, "[HID] g_hidDevice is null");
    size_t res = 0;
    for (size_t i = 0; i < g_hidDevice->size; ++i) {
        HidDevice_S *dev = (HidDevice_S *)SDF_VectorElementAt(g_hidDevice, i);
        if (dev->state == connState) {
            res++;
        }
    }
    return res;
}

bool HidGetDevices(uint8_t connState, SLE_Addr_S *addrs, size_t num)
{
    NLSTK_CHECK_RETURN(addrs != NULL, false, "[HID] addrs is null");
    NLSTK_CHECK_RETURN(g_hidDevice != NULL, false, "[HID] g_hidDevice is null");
    size_t cnt = 0;
    for (size_t i = 0; i < g_hidDevice->size; ++i) {
        HidDevice_S *dev = (HidDevice_S *)SDF_VectorElementAt(g_hidDevice, i);
        if (dev->state == connState && cnt < num) {
            (void)memcpy_s(&addrs[cnt], sizeof(SLE_Addr_S), &dev->addr, sizeof(SLE_Addr_S));
            cnt++;
        }
    }
    return true;
}

HidDevice_S *HidFindDeviceByAppId(int32_t appId)
{
    NLSTK_CHECK_RETURN(g_hidDevice != NULL, NULL, "[HID] g_hidDevice is null");
    size_t index = 0;
    NLSTK_CHECK_RETURN(SDF_VectorFindFirst(g_hidDevice, HidCompAppId, &appId, &index), NULL, "[HID] appId not found");
    HidDevice_S *dev = (HidDevice_S *)SDF_VectorElementAt(g_hidDevice, index);
    return dev;
}

HidDevice_S *HidFindDeviceByAddr(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(g_hidDevice != NULL, NULL, "[HID] g_hidDevice is null");
    NLSTK_CHECK_RETURN(addr != NULL, NULL, "[HID] addr is null");
    size_t index = 0;
    NLSTK_CHECK_RETURN(SDF_VectorFindFirst(g_hidDevice, HidCompAddr, addr, &index), NULL, "[HID] addr not found");
    HidDevice_S *dev = (HidDevice_S *)SDF_VectorElementAt(g_hidDevice, index);
    return dev;
}