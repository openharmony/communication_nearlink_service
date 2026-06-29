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

#ifndef DLI_EXT_FUNC_WRAPPER_H
#define DLI_EXT_FUNC_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>
#include "dli_cmd_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t DLI_AddDeviceToAcceptFilterListExt(DLI_AddrExtStru *param);

uint32_t DLI_EnableFreqBandExt(DLI_FreqBandExtParam *param);

uint32_t DLI_ACBEnableSubrate(DLI_ACBEnableSubrateParam *param);

uint32_t DLI_SetLocalPrivateFeatures(DLI_LocalPrivateFeatures *features);

uint32_t DLI_ACBSetSubrate(DLI_ACBSubrateParam *param);

uint32_t DLI_SetConnFramePowerLevel(DLI_SetConnFramePowerLevelParam *param);

uint32_t DLI_ReadLocalPrivateFeatures(void);

uint32_t DLI_SetICGTestParamExt(DLI_ICGTestParam *param, bool mcast, DLI_ICGCbkParam *cbkParam);

uint32_t DLI_SetICGLabelExt(DLI_ICGLabelParam *param, bool mcast, DLI_ICGCbkParam *cbkParam);

uint32_t DLI_CreateICBExt(DLI_ICBConnectionParam *param, bool mcast, DLI_ICGCbkParam *cbkParam);

uint32_t DLI_UpdateICGParamExt(DLI_ICGUpdatedParam *param, bool mcast, DLI_ICGCbkParam *cbkParam);

uint32_t DLI_ReadLocalMeasureCapsExt(void);

uint32_t DLI_ReadRemoteMeasureCapsExt(DLI_ReadRemoteMeasureCapsParam *param);

uint32_t DLI_SetMeasureParamExt(DLI_MeasureConfigExtParam *param);

uint32_t DLI_SetMeasureEnableExt(DLI_SetMeasureEnableParam *param);

#ifdef __cplusplus
}
#endif

#endif