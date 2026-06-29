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
#ifndef NLSTK_MICP_CLIENT_H
#define NLSTK_MICP_CLIENT_H

#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NLSTK_MICP_STATE_CONNECTING = 0,
    NLSTK_MICP_STATE_CONNECTED,
    NLSTK_MICP_STATE_DISCONNECTING,
    NLSTK_MICP_STATE_DISCONNECTED,
} NLSTK_MicpConnectState_E;

typedef enum {
    NLSTK_MICP_MIC_OFF = 0x00,
    NLSTK_MICP_MIC_ON = 0x01,
} NLSTK_MicpState_E;

typedef void (*NLSTK_MicpConnectEventCbk)(SLE_Addr_S *addr, NLSTK_MicpConnectState_E curState,
    NLSTK_MicpConnectState_E preState, uint8_t errorCode);
typedef void (*NLSTK_MicpMicStateCbk)(SLE_Addr_S *addr, uint8_t micState);

typedef struct {
    NLSTK_MicpConnectEventCbk eventCbk;
    NLSTK_MicpMicStateCbk micStateCbk;
} NLSTK_MicpCbk_S;

uint32_t NLSTK_MicpConnect(SLE_Addr_S *addr);

uint32_t NLSTK_MicpDisconnect(SLE_Addr_S *addr);

uint32_t NLSTK_MicpRegisterCallback(NLSTK_MicpCbk_S *cbk);

#ifdef __cplusplus
}
#endif
#endif