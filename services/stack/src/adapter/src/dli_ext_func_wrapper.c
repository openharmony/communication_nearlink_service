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

#include "dli_ext_func_wrapper.h"

#include "dli_reg_ext_func.h"
#include "adapter_log.h"
#include "parameter_wrapper.h"

uint32_t DLI_AddDeviceToAcceptFilterListExt(DLI_AddrExtStru *param)
{
    DLI_ExtFuncList *funcList = DLI_GetExtFuncList();
    if (funcList == NULL || funcList->addDeviceToAcceptFilterListExt == NULL) {
        ADAPTER_LOGW("get func addDeviceToAcceptFilterListExt failed");
        return 0;
    }

    return funcList->addDeviceToAcceptFilterListExt(param);
}

uint32_t DLI_EnableFreqBandExt(DLI_FreqBandExtParam *param)
{
    DLI_ExtFuncList *funcList = DLI_GetExtFuncList();
    if (funcList == NULL || funcList->enableFreqBandExt == NULL) {
        ADAPTER_LOGW("get func enableFreqBandExt failed");
        return 0;
    }

    return funcList->enableFreqBandExt(param);
}

uint32_t DLI_ACBEnableSubrate(DLI_ACBEnableSubrateParam *param)
{
    DLI_ExtFuncList *funcList = DLI_GetExtFuncList();
    if (funcList == NULL || funcList->acbEnableSubrate == NULL) {
        ADAPTER_LOGW("get func acbEnableSubrate failed");
        return 0;
    }

    return funcList->acbEnableSubrate(param);
}

uint32_t DLI_SetLocalPrivateFeatures(DLI_LocalPrivateFeatures *features)
{
    DLI_ExtFuncList *funcList = DLI_GetExtFuncList();
    if (funcList == NULL || funcList->setLocalPrivateFeatures == NULL) {
        ADAPTER_LOGW("get func setLocalPrivateFeatures failed");
        return 0;
    }

    return funcList->setLocalPrivateFeatures(features);
}

uint32_t DLI_ACBSetSubrate(DLI_ACBSubrateParam *param)
{
    DLI_ExtFuncList *funcList = DLI_GetExtFuncList();
    if (funcList == NULL || funcList->acbSetSubrate == NULL) {
        ADAPTER_LOGW("get func acbSetSubrate failed");
        return 0;
    }

    return funcList->acbSetSubrate(param);
}

uint32_t DLI_SetConnFramePowerLevel(DLI_SetConnFramePowerLevelParam *param)
{
    DLI_ExtFuncList *funcList = DLI_GetExtFuncList();
    if (funcList == NULL || funcList->setConnFramePowerLevel == NULL) {
        ADAPTER_LOGW("get func setConnFramePowerLevel failed");
        return 0;
    }

    return funcList->setConnFramePowerLevel(param);
}

uint32_t DLI_ReadLocalPrivateFeatures(void)
{
    // host 7.0后下发该指令，与星闪音频能力绑定
    bool isAudioSupported = PropertyGetInt32("const.nearlink.audio", 0);
    if (!isAudioSupported) {
        return 0;
    }
    DLI_ExtFuncList *funcList = DLI_GetExtFuncList();
    if (funcList == NULL || funcList->getLocalPrivateFeatures == NULL) {
        ADAPTER_LOGW("get func getLocalPrivateFeatures failed");
        return 0;
    }
    return funcList->getLocalPrivateFeatures();
}

uint32_t DLI_SetICGAutorateParamExt(DLI_ICGAutorateParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    DLI_ExtFuncList *funcList = DLI_GetExtFuncList();
    if (funcList == NULL || funcList->setICGAutorateParamExt == NULL) {
        ADAPTER_LOGW("get func setICGAutorateParamExt failed");
        return 0;
    }
    return funcList->setICGAutorateParamExt(param, mcast, cbkParam);
}

uint32_t DLI_SetICGLabelExt(DLI_ICGLabelParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    DLI_ExtFuncList *funcList = DLI_GetExtFuncList();
    if (funcList == NULL || funcList->setICGLabel == NULL) {
        ADAPTER_LOGW("get func setICGLabel failed");
        return 0;
    }
    return funcList->setICGLabel(param, mcast, cbkParam);
}

uint32_t DLI_CreateICBExt(DLI_ICBConnectionParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    DLI_ExtFuncList *funcList = DLI_GetExtFuncList();
    if (funcList == NULL || funcList->createICBExt == NULL) {
        ADAPTER_LOGW("get func createICBExt failed");
        return 0;
    }
    return funcList->createICBExt(param, mcast, cbkParam);
}

uint32_t DLI_UpdateICGParamExt(DLI_ICGUpdatedParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    DLI_ExtFuncList *funcList = DLI_GetExtFuncList();
    if (funcList == NULL || funcList->updateICGParam == NULL) {
        ADAPTER_LOGW("get func updateICGParam failed");
        return 0;
    }
    return funcList->updateICGParam(param, mcast, cbkParam);
}