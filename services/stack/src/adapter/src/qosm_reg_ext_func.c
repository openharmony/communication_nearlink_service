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

#include "qosm_reg_ext_func.h"

#include <dlfcn.h>
#include <stdio.h>

#include "securec.h"

#include "adapter_log.h"

static QOSM_ExtFuncList g_funcList = {0};

void QOSM_RegisterExtFunc(void *soHandle)
{
    if (soHandle == NULL) {
        ADAPTER_LOGW("null handle\n");
        return;
    }

    g_funcList.antennaDfxSendQueryCmd =
        (QOSM_AntennaDfxSendQueryCmdPtr)dlsym(soHandle, "QOSM_AntennaDfxSendQueryCmd");
    if (g_funcList.antennaDfxSendQueryCmd == NULL) {
        ADAPTER_LOGE("null antennaDfxSendQueryCmd\n");
        return;
    }
    g_funcList.antennaDfxGetAntennaPolicy =
        (QOSM_AntennaDfxGetAntennaPolicyPtr)dlsym(soHandle, "QOSM_AntennaDfxGetAntennaPolicy");
    if (g_funcList.antennaDfxGetAntennaPolicy == NULL) {
        ADAPTER_LOGE("null antennaDfxGetAntennaPolicy\n");
        return;
    }
    g_funcList.isLastEnableFreqBandByRecommend =
        (QOSM_IsLastEnableFreqBandByRecommendPtr)dlsym(soHandle, "IsLastEnableFreqBandByRecommend");
    if (g_funcList.isLastEnableFreqBandByRecommend == NULL) {
        ADAPTER_LOGE("null isLastEnableFreqBandByRecommend\n");
        return;
    }
    g_funcList.isNeedRecoverFreqBandAbility =
        (QOSM_IsNeedRecoverFreqBandAbilityPtr)dlsym(soHandle, "IsNeedRecoverFreqBandAbility");
    if (g_funcList.isNeedRecoverFreqBandAbility == NULL) {
        ADAPTER_LOGE("null isNeedRecoverFreqBandAbility\n");
        return;
    }

    ADAPTER_LOGI("qosm register ext func finished\n");
}

void QOSM_DeregisterExtFunc(void)
{
    (void)memset_s(&g_funcList, sizeof(QOSM_ExtFuncList), 0, sizeof(QOSM_ExtFuncList));
    ADAPTER_LOGI("qosm deregister ext func finished\n");
}

QOSM_ExtFuncList *QOSM_GetExtFuncList(void)
{
    return &g_funcList;
}