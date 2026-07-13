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

#ifndef COLLAB_REG_EXT_FUNC_H
#define COLLAB_REG_EXT_FUNC_H

#include <stdint.h>
#include "nlstk_reg_collab_stm_scan_ext.h"
#include "nlstk_reg_collab_cm_ext.h"
#include "nlstk_reg_collab_trans_ext.h"
#include "nlstk_reg_collab_adv_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t (*COLLAB_StmDevdScanInitPtr)(const COLLAB_StmDevdScanCbk_S *cbk,
    const COLLAB_DevdCollabStmFunc_S *func);
typedef uint32_t (*COLLAB_StmDevdScanInitAfterRegPtr)(const COLLAB_StmDevdScanInitAfterReg_Cbk_S *initAfterCbk);
typedef void (*COLLAB_StmDevdScanDeInitPtr)(void);

typedef uint32_t (*COLLAB_CmInitPtr)(const COLLAB_CollabCmCbk_S *cbk, const COLLAB_CmCollabFunc_S *func);
typedef void (*COLLAB_CmDeInitPtr)(void);

typedef uint32_t (*COLLAB_TransFuncRegisterPtr)(const COLLAB_TransFuncExt *func);
typedef uint32_t (*COLLAB_PreAssignTransBufferPtr)(uint8_t apLinkNum);
typedef uint32_t (*COLLAB_ContinueAssignTransBufferPtr)(uint8_t apOccupiedBufferNum);

typedef uint32_t (*COLLAB_AdvInitPtr)(COLLAB_CollabAdvCbk_S *cbk);

typedef struct {
    COLLAB_StmDevdScanInitPtr stmDevdScanInit;
    COLLAB_StmDevdScanInitAfterRegPtr stmDevdScanInitAfterReg;
    COLLAB_StmDevdScanDeInitPtr stmDevdScanDeInit;
    COLLAB_CmInitPtr cmInit;
    COLLAB_CmDeInitPtr cmDeInit;
    COLLAB_TransFuncRegisterPtr transFuncRegister;
    COLLAB_PreAssignTransBufferPtr preAssignTransBuffer;
    COLLAB_ContinueAssignTransBufferPtr continueAssignTransBuffer;
    COLLAB_AdvInitPtr advInit;
} COLLAB_ExtFuncList;

void COLLAB_RegisterExtFunc(void *soHandle);

void COLLAB_DeregisterExtFunc(void);

COLLAB_ExtFuncList *COLLAB_GetExtFuncList(void);

#ifdef __cplusplus
}
#endif

#endif