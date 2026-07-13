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

#include "collab_ext_func_wrapper.h"
#include <stdio.h>
#include "adapter_log.h"
#include "collab_reg_ext_func.h"

uint32_t COLLAB_StmDevdScanInit(const COLLAB_StmDevdScanCbk_S *cbk, const COLLAB_DevdCollabStmFunc_S *func)
{
    COLLAB_ExtFuncList *funcList = COLLAB_GetExtFuncList();
    if (funcList == NULL || funcList->stmDevdScanInit == NULL) {
        ADAPTER_LOGW("[COLLAB]get func stmDevdScanInit failed");
        return 0;
    }

    return funcList->stmDevdScanInit(cbk, func);
}

uint32_t COLLAB_StmDevdScanInitAfterReg(const COLLAB_StmDevdScanInitAfterReg_Cbk_S *initAfterCbk)
{
    COLLAB_ExtFuncList *funcList = COLLAB_GetExtFuncList();
    if (funcList == NULL || funcList->stmDevdScanInitAfterReg == NULL) {
        ADAPTER_LOGW("[COLLAB]get func stmDevdScanInitAfterReg failed");
        return 0;
    }

    return funcList->stmDevdScanInitAfterReg(initAfterCbk);
}

void COLLAB_StmDevdScanDeInit(void)
{
    COLLAB_ExtFuncList *funcList = COLLAB_GetExtFuncList();
    if (funcList == NULL || funcList->stmDevdScanDeInit == NULL) {
        ADAPTER_LOGW("[COLLAB]get func stmDevdScanDeInit failed");
        return;
    }

    funcList->stmDevdScanDeInit();
}

uint32_t COLLAB_CmInit(const COLLAB_CollabCmCbk_S *cbk, const COLLAB_CmCollabFunc_S *func)
{
    COLLAB_ExtFuncList *extFuncList = COLLAB_GetExtFuncList();
    if (extFuncList == NULL || extFuncList->cmInit == NULL) {
        ADAPTER_LOGW("[COLLAB]extFuncList or cmInit is null");
        return 0;
    }
    return extFuncList->cmInit(cbk, func);
}

void COLLAB_CmDeInit(void)
{
    COLLAB_ExtFuncList *extFuncList = COLLAB_GetExtFuncList();
    if (extFuncList == NULL || extFuncList->cmDeInit == NULL) {
        ADAPTER_LOGW("[COLLAB]extFuncList or cmDeInit is null");
        return;
    }
    extFuncList->cmDeInit();
}

uint32_t COLLAB_TransFuncRegister(const COLLAB_TransFuncExt *func)
{
    COLLAB_ExtFuncList *funcList = COLLAB_GetExtFuncList();
    if (funcList == NULL || funcList->transFuncRegister == NULL) {
        ADAPTER_LOGW("[COLLAB]get func transFuncRegister failed");
        return NLSTK_ERRCODE_FAIL;
    }

    return funcList->transFuncRegister(func);
}

uint32_t COLLAB_PreAssignTransBuffer(uint8_t apLinkNum)
{
    COLLAB_ExtFuncList *funcList = COLLAB_GetExtFuncList();
    if (funcList == NULL || funcList->preAssignTransBuffer == NULL) {
        ADAPTER_LOGW("[COLLAB]get func preAssignTransBuffer failed");
        return NLSTK_ERRCODE_FAIL;
    }

    return funcList->preAssignTransBuffer(apLinkNum);
}

uint32_t COLLAB_ContinueAssignTransBuffer(uint8_t apOccupiedBufferNum)
{
    COLLAB_ExtFuncList *funcList = COLLAB_GetExtFuncList();
    if (funcList == NULL || funcList->continueAssignTransBuffer == NULL) {
        ADAPTER_LOGW("[COLLAB]get func continueAssignTransBuffer failed");
        return NLSTK_ERRCODE_FAIL;
    }

    return funcList->continueAssignTransBuffer(apOccupiedBufferNum);
}

uint32_t COLLAB_AdvInit(COLLAB_CollabAdvCbk_S *cbk)
{
    COLLAB_ExtFuncList *extFuncList = COLLAB_GetExtFuncList();
    if (extFuncList == NULL || extFuncList->advInit == NULL) {
        ADAPTER_LOGW("[COLLAB]extFuncList or advInit is null");
        return 0;
    }
    return extFuncList->advInit(cbk);
}
