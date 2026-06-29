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

#include "common_reg_ext_func.h"

#include <dlfcn.h>
#include <stdio.h>

#include "securec.h"

#include "adapter_log.h"

static COMMON_ExtFuncList g_funcList = {0};

void COMMON_RegisterExtFunc(void *soHandle)
{
    if (soHandle == NULL) {
        ADAPTER_LOGW("null handle\n");
        return;
    }

    g_funcList.isSupportQueryQosInfo = (COMMON_IsSupportQueryQosInfoPtr)dlsym(soHandle, "IsSupportQueryQosInfo");
    g_funcList.isSupportConnHighSpeed = (COMMON_IsSupportConnHighSpeedPtr)dlsym(soHandle, "IsSupportConnHighSpeed");
    g_funcList.isSupportSetMaxInterval = (COMMON_IsSupportSetMaxIntervalPtr)dlsym(soHandle, "IsSupportSetMaxInterval");
    g_funcList.isSupportConnFramePowerLevel =
        (COMMON_IsSupportConnFramePowerLevelPtr)dlsym(soHandle, "IsSupportConnFramePowerLevel");
    g_funcList.isSupportScanFilter = (COMMON_IsSupportScanFilterPtr)dlsym(soHandle, "IsSupportScanFilter");

    ADAPTER_LOGI("common register ext func finished\n");
}

void COMMON_DeregisterExtFunc(void)
{
    (void)memset_s(&g_funcList, sizeof(COMMON_ExtFuncList), 0, sizeof(COMMON_ExtFuncList));
    ADAPTER_LOGI("common deregister ext func finished\n");
}

COMMON_ExtFuncList *COMMON_GetExtFuncList(void)
{
    return &g_funcList;
}