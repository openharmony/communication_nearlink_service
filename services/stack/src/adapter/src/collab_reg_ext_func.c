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

#include "collab_reg_ext_func.h"

#include <dlfcn.h>
#include <stdio.h>
#include "securec.h"
#include "adapter_log.h"

static COLLAB_ExtFuncList g_funcCollabList = {0};

void COLLAB_RegisterExtFunc(void *soHandle)
{
    if (soHandle == NULL) {
        ADAPTER_LOGW("null handle");
        return;
    }
    // devd scan
    g_funcCollabList.stmDevdScanInit = (COLLAB_StmDevdScanInitPtr)dlsym(soHandle, "COLLAB_StmDevdScanInit");
    g_funcCollabList.stmDevdScanInitAfterReg = (COLLAB_StmDevdScanInitAfterRegPtr)dlsym(soHandle,
        "COLLAB_StmDevdScanInitAfterReg");
    g_funcCollabList.stmDevdScanDeInit = (COLLAB_StmDevdScanDeInitPtr)dlsym(soHandle, "COLLAB_StmDevdScanDeInit");

    // cm link
    g_funcCollabList.cmInit = (COLLAB_CmInitPtr)dlsym(soHandle, "COLLAB_CmInit");
    g_funcCollabList.cmDeInit = (COLLAB_CmDeInitPtr)dlsym(soHandle, "COLLAB_CmDeInit");

    // tran
    g_funcCollabList.transFuncRegister = (COLLAB_TransFuncRegisterPtr)dlsym(soHandle, "COLLAB_TransFuncRegister");
    g_funcCollabList.preAssignTransBuffer =
        (COLLAB_PreAssignTransBufferPtr)dlsym(soHandle, "COLLAB_PreAssignTransBuffer");
    g_funcCollabList.continueAssignTransBuffer =
        (COLLAB_ContinueAssignTransBufferPtr)dlsym(soHandle, "COLLAB_ContinueAssignTransBuffer");

    // adv
    g_funcCollabList.advInit = (COLLAB_AdvInitPtr)dlsym(soHandle, "COLLAB_AdvInit");

    ADAPTER_LOGI("collab register ext func finished");
}

void COLLAB_DeregisterExtFunc(void)
{
    (void)memset_s(&g_funcCollabList, sizeof(COLLAB_ExtFuncList), 0, sizeof(COLLAB_ExtFuncList));
    ADAPTER_LOGI("collab deregister ext func finished");
}

COLLAB_ExtFuncList *COLLAB_GetExtFuncList(void)
{
    return &g_funcCollabList;
}