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
#ifndef NLSTK_SSAP_LINK_H
#define NLSTK_SSAP_LINK_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum NLSTK_SsapConnectLinkState {
    SSAP_CONNECT_STATE_IDLE = 0x00, /* 空闲状态,表示客户端未调用NLSTK_SsapClientConnect发起建链 */
    SSAP_CONNECT_STATE_CONNECTING, /* 连接中状态,表示客户端已经调用NLSTK_SsapClientConnect发起建链，但是链路还未建立 */
    SSAP_CONNECT_STATE_CONNECTED, /* 已经连接状态,表示链路已经建立 */
    SSAP_CONNECT_STATE_DISCONNECTING, /* 断开中状态,表示客户端已经调用NLSTK_SsapClientDisConnect发起断链，但是链路还未断开 */
    SSAP_CONNECT_STATE_DISCONNECTED, /* 已经断开状态,表示链路已经断开 */
    SSAP_CONNECT_STATE_BUTT,
} NLSTK_SsapConnectLinkState_E;

NLSTK_Errcode_E NLSTK_SsapClientConnect(int32_t appId);

NLSTK_Errcode_E NLSTK_SsapClientDisconnect(int32_t appId);

#ifdef __cplusplus
}
#endif
#endif