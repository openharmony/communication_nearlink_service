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
#ifndef PORT_STM_H
#define PORT_STM_H

#include "port_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PORT_STATE_IDLE = 0,
    PORT_STATE_REGISTER_APP = 1,
    PORT_STATE_CREATE_LINK = 2,
    PORT_STATE_GET_SERVICE = 3,
    PORT_STATE_FIND_SERVICE = 4,
    PORT_STATE_READ_PROPERTY = 5,
    PORT_STATE_SET_NET = 6,
    PORT_STATE_CONNECTED = 7,
    PORT_STATE_BUTT = 8,
} PortStmState_E;

typedef enum {
    PORT_NEW_STATE_ENTRY = 0,
    PORT_ON_USER_CONNECT = 1,
    PORT_ON_USER_DISCONNECT = 2,
    PORT_ON_REGISTER_APP = 3,
    PORT_ON_STATE_CHANGED = 4,
    PORT_ON_GET_SERVICE = 5,
    PORT_ON_FIND_SERVICE = 6,
    PORT_ON_READ_PROPERTY = 7,
    PORT_ON_SET_NTF = 8,
    PORT_ON_PROPERTY_CHANGED = 9,
} PortStmEvent_E;

typedef struct {
    int what;
    void *extData;
} PortStmParam_S;

void PortStateMachineCall(PortInfoCache_S *portInfoCache, PortStmParam_S msg);

#ifdef __cplusplus
}
#endif

#endif