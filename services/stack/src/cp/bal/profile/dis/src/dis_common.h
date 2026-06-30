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
#ifndef DIS_COMMON_H
#define DIS_COMMON_H

#include "nlstk_dis_def.h"
#include "dis_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DIS_APPID_UNREGISTERED = 0,
    DIS_APPID_REGISTERING,
    DIS_LINK_DISCONNECTED,
    DIS_LINK_CONNECTED,
    DIS_SERVICE_FOUND,
    DIS_DEV_CONNECTED,
    DIS_STM_STATE_BUTT,
} DisStmState_E;

typedef enum {
    DIS_ON_USER_CONNECT = 0,
    DIS_ON_REGISTERED_CBK,
    DIS_ON_LINK_STATE_CHANGE,
    DIS_ON_GET_SERVICE_CBK,
    DIS_ON_READ_PROPERTY_CBK,
    DIS_ON_USER_DISCONNECT,
    DIS_STM_EVENT_BUTT,
} DisStmEvent_E;

typedef struct {
    int what;
    void *extData;
    int32_t reason;
} DisStmParam;

bool DisClientAddInfoIntoVector(DisDeviceInfo_S *info);

void DisClientInitIfEmpty(void);

DisDeviceInfo_S *DisFindDeviceInfo(SLE_Addr_S *addr);

DisDeviceInfo_S *DisFindDeviceInfoByAppId(int32_t appId);

void DisRemoveDeviceInfo(SLE_Addr_S *addr);

uint8_t DisCountConnectedDevices(void);

void DisUseStateChangeCallback(SLE_Addr_S *addr, NLSTK_DisConnectState_E curState,
    NLSTK_DisConnectState_E prevState, NLSTK_Errcode_E errNumb);

void DisRegClientCbkIn(NLSTK_DisClientCbk_S *cbk);

NLSTK_SsapUuid_S DisConvertUuidToStru(uint16_t uuid);

void DisDisable(void);

#ifdef __cplusplus
}
#endif
#endif
