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
#ifndef BAS_COMMON_H
#define BAS_COMMON_H

#include "bas_type.h"
#include "bas_def.h"
#include "nlstk_bas_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BAS_DEVICE_STATE_CHANGE(info, newState)                                                    \
    do {                                                                                       \
        NLSTK_LOG_INFO("[BAS] addr = %s, old state = %u, new state = %u",                        \
            GET_ENC_ADDR(&(info)->addr), (info)->state, (newState));                             \
        (info)->state = (newState);                                                             \
    } while (0)

BasClientCallBack_S *BasGetUserCbk(void);

bool BasClientAddInfoIntoVector(BasDeviceInfo_S *info);

void BasClientInitIfEmpty(void);

BasDeviceInfo_S *BasFindDeviceInfo(SLE_Addr_S *addr);

BasDeviceInfo_S *BasFindDeviceInfoByAppId(int32_t appId);

void BasRemoveDeviceInfo(SLE_Addr_S *addr);

uint8_t BasCountConnectedDevices(void);

void BasStateChangeCbk(SLE_Addr_S *addr, NLSTK_BasConnectState_E curState,
    NLSTK_BasConnectState_E prevState, NLSTK_Errcode_E errNumb);

void BasReadCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value, NLSTK_Errcode_E ret);

void BasNotifyCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value);

void BasRegClientCbkIn(BasClientCallBack_S *cbk);

NLSTK_SsapUuid_S BasConvertUuidToStru(uint16_t uuid);

uint16_t BasConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru);

void BasClearDeviceData(void *ptr);

bool BasClientInit(void);

void BasClientDeinit(void);

bool BasCompareAddr(void *ptr, void *args);

bool BasCompAppId(void *ptr, void *args);

NLSTK_Errcode_E BasBuildService(BasDeviceInfo_S *devInfo, NLSTK_SsapServ_S *service, uint16_t serviceNum);

#ifdef __cplusplus
}
#endif
#endif
