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

#include "dli_reg_ext_func.h"

#include <dlfcn.h>
#include <stdio.h>

#include "securec.h"

#include "adapter_log.h"

static DLI_ExtFuncList g_dliFuncList = {0};

void DLI_RegisterExtFunc(void *soHandle)
{
    if (soHandle == NULL) {
        ADAPTER_LOGW("null handle");
        return;
    }

    g_dliFuncList.addDeviceToAcceptFilterListExt =
        (DLI_AddDeviceToAcceptFilterListExtPtr)dlsym(soHandle, "DLI_AddDeviceToAcceptFilterListExt");
    g_dliFuncList.enableFreqBandExt = (DLI_EnableFreqBandExtPtr)dlsym(soHandle, "DLI_EnableFreqBandExt");
    g_dliFuncList.acbEnableSubrate = (DLI_ACBEnableSubratePtr)dlsym(soHandle, "DLI_ACBEnableSubrate");
    g_dliFuncList.setLocalPrivateFeatures =
        (DLI_SetLocalPrivateFeaturesPtr)dlsym(soHandle, "DLI_SetLocalPrivateFeatures");
    g_dliFuncList.acbSetSubrate = (DLI_ACBSetSubratePtr)dlsym(soHandle, "DLI_ACBSetSubrate");
    g_dliFuncList.setConnFramePowerLevel = (DLI_SetConnFramePowerLevelPtr)dlsym(soHandle, "DLI_SetConnFramePowerLevel");
    g_dliFuncList.getLocalPrivateFeatures =
        (DLI_ReadLocalPrivateFeaturesPtr)dlsym(soHandle, "DLI_ReadLocalPrivateFeatures");
    g_dliFuncList.getExtRegOpcode = (DLI_GetExtRegOpcodePtr)dlsym(soHandle, "DLI_GetExtRegOpcode");
    g_dliFuncList.readLocalMeasureCapsExt =
        (DLI_ReadLocalMeasureCapsExtPtr)dlsym(soHandle, "DLI_ReadLocalMeasureCapsExt");
    g_dliFuncList.readRemoteMeasureCapsExt =
        (DLI_ReadRemoteMeasureCapsExtPtr)dlsym(soHandle, "DLI_ReadRemoteMeasureCapsExt");
    g_dliFuncList.setMeasureParamExt = (DLI_SetMeasureParamExtPtr)dlsym(soHandle, "DLI_SetMeasureParamExt");
    g_dliFuncList.setMeasureEnableExt = (DLI_SetMeasureEnableExtPtr)dlsym(soHandle, "DLI_SetMeasureEnableExt");
    g_dliFuncList.setICGTestParamExt = (DLI_SetICGTestParamExtPtr)dlsym(soHandle, "DLI_SetICGTestParam");
    g_dliFuncList.setICGLabel = (DLI_SetICGLabelPtr)dlsym(soHandle, "DLI_SetICGLabel");
    g_dliFuncList.createICBExt = (DLI_CreateICBExtPtr)dlsym(soHandle, "DLI_CreateICB");
    g_dliFuncList.updateICGParam = (DLI_UpdateICGParamPtr)dlsym(soHandle, "DLI_UpdateICGParam");
    ADAPTER_LOGI("dli register ext func finished");
}

void DLI_DeregisterExtFunc(void)
{
    (void)memset_s(&g_dliFuncList, sizeof(DLI_ExtFuncList), 0, sizeof(DLI_ExtFuncList));
    ADAPTER_LOGI("dli deregister ext func finished");
}

DLI_ExtFuncList *DLI_GetExtFuncList(void)
{
    return &g_dliFuncList;
}