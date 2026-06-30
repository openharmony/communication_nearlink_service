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
#ifndef HID_STM_H
#define HID_STM_H

#include "hid_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HID_STATE_UNINIT = 0,
    HID_STATE_INIT = 1,
    HID_STATE_CREATE_LINK = 2,
    HID_STATE_GET_SERVICE = 3,
    HID_STATE_READ_PROPERTY = 4,
    HID_STATE_SET_NOTIFICATION = 5,
    HID_STATE_CONNECTED = 6,
    HID_STATE_BUFF = 7,
} HidStmState_E;

typedef enum {
    HID_ON_USER_CONNECT = 0,
    HID_ON_USER_CONNECTING = 1,
    HID_ON_USER_DISCONNECT = 2,
    HID_ON_USER_GET_INFO = 3,
    HID_ON_USER_READ = 4,
    HID_ON_USER_WRITE = 5,
    HID_ON_REGISTER_APP = 6,
    HID_ON_STATE_CHANGED = 7,
    HID_ON_GET_SERVICE = 8,
    HID_ON_FIND_SERVICE = 9,
    HID_ON_READ_PROPERTY = 10,
    HID_ON_WRITE_PROPERTY = 11,
    HID_ON_NOTIFY_PROPERTY = 12,
    HID_ON_SET_NOTIFICATION = 13,
} HidStmEvent_E;

typedef struct {
    int what;
    void *extData;
} HidStmParam_S;

void HidStateMachineCall(HidDevice_S *dev, HidStmParam_S msg);

#ifdef __cplusplus
}
#endif

#endif