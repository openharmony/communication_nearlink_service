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

#include "devd_reg_ext_func.h"

#include <dlfcn.h>
#include <stdio.h>

#include "securec.h"

#include "adapter_log.h"

static Devd_ExtFuncList g_funcList = {0};

void Devd_RegisterExtFunc(void *soHandle)
{
    if (soHandle == NULL) {
        ADAPTER_LOGW("null handle");
        return;
    }

    g_funcList.devdScanFilterInit = (DevdScanFilterInitPtr)dlsym(soHandle, "DevdScanFilterInit");
    g_funcList.devdScanFilterDeInit = (DevdScanFilterDeInitPtr)dlsym(soHandle, "DevdScanFilterDeInit");
    g_funcList.devdSetScanfilterSwitch = (DevdSetScanfilterSwitchPtr)dlsym(soHandle, "DevdSetScanfilterSwitch");
    g_funcList.devdIsSupportScanFilter = (DevdIsSupportScanFilterPtr)dlsym(soHandle, "DevdIsSupportScanFilter");
    g_funcList.devdSetScanfilter = (DevdSetScanfilterPtr)dlsym(soHandle, "DevdSetScanfilter");
    g_funcList.devdAddChipFilters = (DevdAddChipFiltersPtr)dlsym(soHandle, "DevdAddChipFilters");
    g_funcList.devdDeleteChipFilters = (DevdDeleteChipFiltersPtr)dlsym(soHandle, "DevdDeleteChipFilters");

    ADAPTER_LOGI("devd register ext func finished\n");
}

void Devd_DeregisterExtFunc(void)
{
    (void)memset_s(&g_funcList, sizeof(Devd_ExtFuncList), 0, sizeof(Devd_ExtFuncList));
    ADAPTER_LOGI("devd deregister ext func finished\n");
}

Devd_ExtFuncList *Devd_GetExtFuncList(void)
{
    return &g_funcList;
}