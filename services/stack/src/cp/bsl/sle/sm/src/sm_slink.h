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
#ifndef SM_SLINK_H
#define SM_SLINK_H

#include "sdf_timer.h"
#include "sm_stm.h"
#include "sm_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_RECV_TIMEOUT_TIME 10000   // msec
#define SM_RECV_USER_CONFIRM_TIMEOUT_TIME 30000   // msec
#define TIMER_NO_USED_VALUE (-1)

typedef struct SmSLink SmSLink_S;

typedef void (*SmSLinkPkgDispatcher_S)(uint16_t opcode, SmSLink_S *slink, const uint8_t *pkg, size_t size);

struct SmSLink {
    SLE_Addr_S rmtAddr;
    SLE_Addr_S localAddr;
    // bind logic channel
    uint16_t lcid;
    SmNodeType_E role;
    // state machine for interaction
    SmStateMachine_S *stm;
    SmState_E curStateIndex; // for externel query
    SmSLinkPkgDispatcher_S dispatcher;
    uint16_t expectOpCode;
    int timerHandle;
    int uapiTimerHandle;     /* 鉴权码输入定时器句柄 */
    // keys for encrypt
    uint8_t dhKey[SM_DHKEY_LEN];
    uint8_t linkKey[SM_LINK_KEY_LEN];
    uint8_t priKey[SM_PRIVATE_KEY_LEN];
    uint8_t psk[SM_PSK_SEC_KEY_LEN];
    // pairing params
    SmDeviceParams_S gNode;
    SmDeviceParams_S tNode;
    SmPairingNegoParam_S negoParams;
    uint8_t encIntgCheck;               /* 加密和完整性保护 参考 SmEncIntgProtection_E */
};

void SmSLinkDtor(void *slinkIn);
SmSLink_S *SmSLinkCtor(const SLE_Addr_S *rmtAddr);
bool SmSLinkBindLogicLink(SmSLink_S *slink);
bool SmSLinkWaitExpectOpCode(SmSLink_S *slink, uint16_t expectOpCode, time_t timeout);
bool SmSLinkWaitUapiInput(SmSLink_S *slink, time_t timeout);
void SmSLinkWaitDelTimer(SmSLink_S *slink);
void SmSLinkWaitDelUapiTimer(SmSLink_S *slink);

void SmSLinkAuthCbk(SmSLink_S *slink, uint8_t status);
void SmSLinkEncpCbk(SmSLink_S *slink, uint8_t status);
void SmSLinkRequestCbk(SmSLink_S *slink, uint8_t type, SmAuthUserCode_S code);

#ifdef __cplusplus
}
#endif

#endif /* SM_SLINK_H */