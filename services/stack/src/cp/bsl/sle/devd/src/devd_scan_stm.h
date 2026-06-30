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

#ifndef SCAN_STM_H
#define SCAN_STM_H

#include "sdf_stm.h"
#include "sdf_vector.h"
#include "devd_scan_type.h"
#include "nlstk_devd_scan_type.h"
#include "nlstk_scan_api.h"
#include "nlstk_stm_collab_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEVD_STM_M(pStm, field) (((NLSTK_DevdStateMachine_S *)(pStm))->field)

typedef enum DevdScanStmEvent {
    DEVD_POTIENTIAL_UPDATE = NLSTK_DEVD_POTIENTIAL_UPDATE,
    DEVD_START_OK = NLSTK_DEVD_START_OK,
    DEVD_START_ERR = NLSTK_DEVD_START_ERR,
    DEVD_STOP_OK = NLSTK_DEVD_STOP_OK,
    DEVD_STOP_ERR = NLSTK_DEVD_STOP_ERR,
    DEVD_INTERNAL_TIMEOUT = NLSTK_DEVD_INTERNAL_TIMEOUT,
} DevdScanStmEvent_E;

typedef enum DevdScanState {
    DEVD_STATE_STOPPED = NLSTK_DEVD_STATE_STOPPED,
    DEVD_STATE_STARTING = NLSTK_DEVD_STATE_STARTING,
    DEVD_STATE_STARTED = NLSTK_DEVD_STATE_STARTED,
    DEVD_STATE_STOPPING = NLSTK_DEVD_STATE_STOPPING,
} DevdScanState_E;

NLSTK_Errcode_E DevdSetStmStateName(uint8_t devdScanState, const char *scanStateName);

const char *DevdGetStmStateName(uint8_t devdScanState);

NLSTK_Errcode_E DevdCollabRegFunc(const NLSTK_DevdCollabScanFunc_S *cbks);

void DevdCollabUnRegFunc(void);

void DevdStateMachineDtor(NLSTK_DevdStateMachine_S *stm);

NLSTK_DevdStateMachine_S *DevdStateMachineCtor(void);

const NLSTK_DevdCollabScanFunc_S *DevdGetCollabScanFunc(void);

#ifdef __cplusplus
}
#endif

#endif /* SM_STM_H */