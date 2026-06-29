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

#ifndef COLLAB_EXT_FUNC_WRAPPER_H
#define COLLAB_EXT_FUNC_WRAPPER_H

#include <stdint.h>
#include "nlstk_reg_collab_stm_scan_ext.h"
#include "nlstk_reg_collab_cm_ext.h"
#include "nlstk_reg_collab_trans_ext.h"
#include "nlstk_reg_collab_adv_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

// collab stm devd scan api
uint32_t COLLAB_StmDevdScanInit(const COLLAB_StmDevdScanCbk_S *cbk, const COLLAB_DevdCollabStmFunc_S *func);

uint32_t COLLAB_StmDevdScanInitAfterReg(const COLLAB_StmDevdScanInitAfterReg_Cbk_S *initAfterCbk);

void COLLAB_StmDevdScanDeInit(void);

// collab stm cm api
uint32_t COLLAB_CmInit(const COLLAB_CollabCmCbk_S *cbk, const COLLAB_CmCollabFunc_S *func);

void COLLAB_CmDeInit(void);

// collab trans api
uint32_t COLLAB_TransFuncRegister(const COLLAB_TransFuncExt *func);

uint32_t COLLAB_PreAssignTransBuffer(uint8_t apLinkNum);

uint32_t COLLAB_ContinueAssignTransBuffer(uint8_t apOccupiedBufferNum);

// collab adv api
uint32_t COLLAB_AdvInit(COLLAB_CollabAdvCbk_S *cbk);

#ifdef __cplusplus
}
#endif

#endif