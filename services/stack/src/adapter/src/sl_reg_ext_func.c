/**
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

#include "sl_reg_ext_func.h"

#include <dlfcn.h>
#include <stdio.h>

#include "securec.h"

#include "adapter_log.h"

static SL_ExtFuncList g_funcList = {0};

void SL_RegisterExtFunc(void *soHandle)
{
    if (soHandle == NULL) {
        ADAPTER_LOGW("null handle\n");
        return;
    }

    g_funcList.hostInit = (SL_HostInitPtr)dlsym(soHandle, "SL_HostInit");
    g_funcList.hostDeinit = (SL_HostDeinitPtr)dlsym(soHandle, "SL_HostDeinit");
    ADAPTER_LOGI("sl register ext func finished\n");
}

void SL_DeregisterExtFunc(void)
{
    (void)memset_s(&g_funcList, sizeof(SL_ExtFuncList), 0, sizeof(SL_ExtFuncList));
    ADAPTER_LOGI("sl deregister ext func finished\n");
}

SL_ExtFuncList *SL_GetExtFuncList(void)
{
    return &g_funcList;
}