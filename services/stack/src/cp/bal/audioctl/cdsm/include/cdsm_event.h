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
#ifndef CDSM_EVENT_H
#define CDSM_EVENT_H

#include <stdint.h>
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CDSM_EVENT_DISCONNECT = 0,
    CDSM_EVENT_CONNECT,
} NLSTK_CdsmEventType_E;

typedef enum {
    CDSM_PROFILE_DISCONNECT = 0,
    CDSM_PROFILE_CONNECT,
} NLSTK_CdsmProfileState_E;

typedef struct {
    SLE_Addr_S addr;
    uint8_t state;
} NLSTK_CdsmMemInfo_S;

typedef struct {
    SLE_Addr_S addr;
    uint32_t gid;
    uint8_t type;
    uint8_t num;
    NLSTK_CdsmMemInfo_S *memInfo;
} NLSTK_CdsmEvent_S;

typedef void (*NLSTK_CdsmEventCbk)(NLSTK_CdsmEvent_S *event);

#ifdef __cplusplus
}
#endif

#endif