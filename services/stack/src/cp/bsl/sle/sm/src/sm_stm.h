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
#ifndef SM_STM_H
#define SM_STM_H

#include "sdf_stm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SmStmEvent {
    /* normal cases */
    SM_PASSIVE_START,
    SM_ACTIVE_START,
    SM_RECOVER_START,
    SM_NEGO_SUCCESS,
    SM_AUTH_SUCCESS,
    SM_ENCP_SUCCESS,
    SM_REMOVE_PAIR,
    SM_ENCP_PARAM_REQ_REPLY,
    SM_MISS_HANDLE,
    /* error cases */
    SM_INTERNAL_ERROR = 0xFF00,
    SM_EXTERNAL_ERROR,
    SM_ENCP_FAIL,
    SM_LCHANNEL_DISCONN,
    SM_TIMEOUT,
} SmStmEvent_E;

typedef enum SmState {
    SM_STATE_INIT,
    SM_STATE_NEGO,
    SM_STATE_AUTH,
    SM_STATE_ENCP,
    SM_STATE_MISS,
    SM_STATE_FULL,
    SM_STATE_REMV,  // slink in REMV state would remove itself.
} SmState_E;

extern const char *g_smStateName[];

typedef struct SmSLink SmSLink_S;
typedef struct {
    StateMachine base;   // base class always be the first member.
    SmSLink_S *slink;  // borrow slink outside.
} SmStateMachine_S;

#define SM_STM_M(pStm, field) (((SmStateMachine_S *)(pStm))->field)

void SmStateMachineDtor(SmStateMachine_S *stm);
SmStateMachine_S *SmStateMachineCtor(SmSLink_S *slink);

#ifdef __cplusplus
}
#endif

#endif /* SM_STM_H */