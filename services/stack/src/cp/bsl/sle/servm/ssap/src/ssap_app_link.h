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
#ifndef SSAP_APP_LINK_H
#define SSAP_APP_LINK_H

#include "ssapc_app.h"
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t frameType;
} NLSTK_SsapConnParam;

typedef struct {
    SLE_Addr_S addr;
    NLSTK_SsapConnParam param;
} NLSTK_SsapConnAddrParam;

void SsapClientConnect(void *param);

void SsapClientDisconnect(void *param);

void SsapTriggerLinkStateMachineChange(SLE_Addr_S *addr, NLSTK_SsapClientLinkChangeEvent_E event, uint8_t reason);

#ifdef __cplusplus
}
#endif
#endif