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
#ifndef HID_COMMON_H
#define HID_COMMON_H

#include <stdbool.h>
#include "sdf_addr.h"
#include "hid_def.h"
#include "hid_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HID_DEV_STATE_CHANGE(dev, newState)                                                    \
    do {                                                                                       \
        NLSTK_LOG_INFO("[HID] addr = %s, old state = %u, new state = %u",                        \
            GET_ENC_ADDR(&(dev)->addr), (dev)->state, (newState));                             \
        (dev)->state = (newState);                                                             \
    } while (0)

HidClientCallBack_S *HidGetUserCbk(void);

void HidSetUserCbk(HidClientCallBack_S *cbk);

void HidReadCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value, NLSTK_Errcode_E ret);

void HidWriteCbk(SLE_Addr_S *addr, HidPropertyType_E type, NLSTK_Errcode_E ret);

void HidNotifyCbk(SLE_Addr_S *addr, HidPropertyType_E type, void *value);

void HidStateChangeCbk(SLE_Addr_S *addr, HidConnectState_E state, HidConnectState_E preState, NLSTK_Errcode_E ret);

bool HidDeviceInit(void);

void HidDeviceDeinit(void);

bool HidAddDevice(HidDevice_S *dev);

void HidRemoveDevice(SLE_Addr_S *addr);

size_t HidGetDeviceNum(uint8_t connState);

bool HidGetDevices(uint8_t connState, SLE_Addr_S *addrs, size_t num);

HidDevice_S *HidFindDeviceByAppId(int32_t appId);

HidDevice_S *HidFindDeviceByAddr(SLE_Addr_S *addr);

#ifdef __cplusplus
}
#endif

#endif