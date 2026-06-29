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

#include "hadm_reg_ext_func.h"

#include <dlfcn.h>
#include <stdio.h>
#include "securec.h"
#include "adapter_log.h"

static HADM_ExtFuncList g_hadmFuncList = {0};

void HADM_RegisterExtFunc(void *soHandle)
{
    if (soHandle == NULL) {
        ADAPTER_LOGW("null handle");
        return;
    }

    g_hadmFuncList.readRemoteExtFeatures =
        (HADM_ReadRemoteExtFeaturesPtr)dlsym(soHandle, "DLI_ReadRemoteExtFeatures");
    g_hadmFuncList.processCsCaps =
        (HADM_ProcessCsCapsPtr)dlsym(soHandle, "HADM_ExtProcessCsCaps");
    g_hadmFuncList.checkAndUpdateMultiToneConfig =
        (HADM_CheckAndUpdateMultiToneConfigPtr)dlsym(soHandle, "HADM_ExtCheckAndUpdateMultiToneConfig");
    g_hadmFuncList.clearRemoteCsCaps =
        (HADM_ClearRemoteCsCapsPtr)dlsym(soHandle, "HADM_ExtClearRemoteCsCaps");

    ADAPTER_LOGI("hadm register ext func finished");
}

void HADM_DeregisterExtFunc(void)
{
    (void)memset_s(&g_hadmFuncList, sizeof(HADM_ExtFuncList), 0, sizeof(HADM_ExtFuncList));
    ADAPTER_LOGI("hadm deregister ext func finished");
}

HADM_ExtFuncList *HADM_GetExtFuncList(void)
{
    return &g_hadmFuncList;
}
