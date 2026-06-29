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
#ifndef ICCE_STM_H
#define ICCE_STM_H

#include "icce_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ICCE_APPID_UNREGISTERED = 0,
    ICCE_APPID_REGISTERING = 1,
    ICCE_LINK_DISCONNECTED = 2,
    ICCE_LINK_CONNECTED = 3,
    ICCE_SERVICE_FOUND = 4,
    ICCE_DEV_CONNECTED = 5,
    ICCE_STM_STATE_BUTT = 6,
} IcceStmState_E;

typedef enum {
    ICCE_ON_USER_CONNECT = 0,
    ICCE_ON_REGISTERED_CBK = 1,
    ICCE_ON_LINK_STATE_CHANGE = 2,
    ICCE_ON_GET_SERVICE_CBK = 3,
    ICCE_ON_READ_PROPERTY_CBK = 4,
    ICCE_ON_USER_DISCONNECT = 5,
    ICCE_STM_EVENT_BUTT = 6,
} IcceStmEvent_E;

typedef struct {
    int what;
    void *extData;
    int32_t reason;
} IcceStmParam;

void IcceClientStmCall(IcceDevice_S *icceInfo, IcceStmParam param);

#ifdef __cplusplus
}
#endif
#endif