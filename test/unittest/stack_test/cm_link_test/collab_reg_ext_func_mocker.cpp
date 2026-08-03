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
static COLLAB_CollabCmCbk_S g_collabCmCbk = {0};

static uint32_t COLLAB_CmInit(const COLLAB_CollabCmCbk_S *cbk, const COLLAB_CmCollabFunc_S *func)
{
    if (cbk != NULL) {
        g_collabCmCbk = *cbk;
    }
    return 0;
}

static void COLLAB_CmDeInit(void)
{
    (void)memset_s(&g_collabCmCbk, sizeof(COLLAB_CollabCmCbk_S), 0, sizeof(COLLAB_CollabCmCbk_S));
}

COLLAB_CollabCmCbk_S *COLLAB_GetCollabCmCbk(void)
{
    return &g_collabCmCbk;
}

void COLLAB_RegisterExtFunc(void *soHandle)
{
    (void)soHandle;
    g_funcCollabList.cmInit = COLLAB_CmInit;
    g_funcCollabList.cmDeInit = COLLAB_CmDeInit;
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