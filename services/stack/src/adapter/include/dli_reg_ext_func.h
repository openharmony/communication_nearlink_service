/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef DLI_REG_EXT_FUNC_H
#define DLI_REG_EXT_FUNC_H

#include <stdint.h>
#include "dli_def.h"
#include "dli_cmd_struct.h"
#include "dli_callback.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t (*DLI_AddDeviceToAcceptFilterListExtPtr)(DLI_AddrExtStru *param);
typedef uint32_t (*DLI_EnableFreqBandExtPtr)(DLI_FreqBandExtParam *param);
typedef uint32_t (*DLI_ACBEnableSubratePtr)(DLI_ACBEnableSubrateParam *param);
typedef uint32_t (*DLI_SetLocalPrivateFeaturesPtr)(DLI_LocalPrivateFeatures *features);
typedef uint32_t (*DLI_ACBSetSubratePtr)(DLI_ACBSubrateParam *param);
typedef uint32_t (*DLI_SetConnFramePowerLevelPtr)(DLI_SetConnFramePowerLevelParam *param);
typedef uint32_t (*DLI_ReadLocalPrivateFeaturesPtr)(void);
typedef uint32_t (*DLI_ReadLocalMeasureCapsExtPtr)(void);
typedef uint32_t (*DLI_ReadRemoteMeasureCapsExtPtr)(DLI_ReadRemoteMeasureCapsParam *param);
typedef uint32_t (*DLI_SetMeasureParamExtPtr)(DLI_MeasureConfigExtParam *param);
typedef uint32_t (*DLI_SetMeasureEnableExtPtr)(DLI_SetMeasureEnableParam *param);

typedef uint32_t (*DLI_SetICGTestParamExtPtr)(DLI_ICGTestParam *param, bool mcast, DLI_ICGCbkParam *cbkParam);
typedef uint32_t (*DLI_SetICGLabelPtr)(DLI_ICGLabelParam *param, bool mcast, DLI_ICGCbkParam *cbkParam);
typedef uint32_t (*DLI_CreateICBExtPtr)(DLI_ICBConnectionParam *param, bool mcast, DLI_ICGCbkParam *cbkParam);
typedef uint32_t (*DLI_UpdateICGParamPtr)(DLI_ICGUpdatedParam *param, bool mcast, DLI_ICGCbkParam *cbkParam);

typedef struct {
    DLI_AddDeviceToAcceptFilterListExtPtr addDeviceToAcceptFilterListExt;
    DLI_EnableFreqBandExtPtr enableFreqBandExt;
    DLI_ACBEnableSubratePtr acbEnableSubrate;
    DLI_SetLocalPrivateFeaturesPtr setLocalPrivateFeatures;
    DLI_ACBSetSubratePtr acbSetSubrate;
    DLI_SetConnFramePowerLevelPtr setConnFramePowerLevel;
    DLI_ReadLocalPrivateFeaturesPtr getLocalPrivateFeatures;
    DLI_GetExtRegOpcodePtr getExtRegOpcode;
    DLI_ReadLocalMeasureCapsExtPtr readLocalMeasureCapsExt;
    DLI_ReadRemoteMeasureCapsExtPtr readRemoteMeasureCapsExt;
    DLI_SetMeasureParamExtPtr setMeasureParamExt;
    DLI_SetMeasureEnableExtPtr setMeasureEnableExt;
    DLI_SetICGTestParamExtPtr setICGTestParamExt;
    DLI_SetICGLabelPtr setICGLabel;
    DLI_CreateICBExtPtr createICBExt;
    DLI_UpdateICGParamPtr updateICGParam;
} DLI_ExtFuncList;

void DLI_RegisterExtFunc(void *soHandle);

void DLI_DeregisterExtFunc(void);

DLI_ExtFuncList *DLI_GetExtFuncList(void);

#ifdef __cplusplus
}
#endif

#endif