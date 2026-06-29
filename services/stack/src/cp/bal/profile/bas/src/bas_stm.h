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
#ifndef BAS_STM_H
#define BAS_STM_H

#include "bas_common.h"
#include "bas_type.h"
#include "bas_ssap_cbk.h"
#include "nlstk_public_define.h"
#include "nlstk_ssap_app_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BAS_APPID_UNREGISTERED = 0,
    BAS_APPID_REGISTERING,
    BAS_LINK_DISCONNECTED,
    BAS_LINK_CONNECTING,
    BAS_SERVICE_FOUND,
    BAS_READ_PROPERTY,
    BAS_DEVICE_CONNECTED,
    // 可能会有设备1034属性未赋予写权限，setCpcd必现失败
    BAS_SET_NOTIFICATION,
    BAS_DEVICE_DISCONNECTED,
    BAS_STM_STATE_BUTT,
} BasStmState_E;

typedef enum {
    BAS_ON_USER_CONNECT = 0,
    BAS_ON_USER_CONNECTING,
    BAS_ON_REGISTERED_CBK,
    BAS_ON_LINK_DISCONNECTED,
    BAS_ON_LINK_STATE_CHANGE,
    BAS_ON_GET_SERVICE_CBK,
    BAS_ON_READ_PROPERTY,
    BAS_ON_SET_NOTIFICATION,
    BAS_ON_NOTIFY_PROPERTY,
    BAS_ON_USER_DISCONNECT,
    BAS_STM_EVENT_BUTT,
} BasStmEvent_E;

typedef struct {
    int what;
    void *extData;
    int32_t reason;
} BasStmParam_S;

void BasClientStmCall(BasDeviceInfo_S *devInfo, BasStmParam_S param);

#ifdef __cplusplus
}
#endif
#endif