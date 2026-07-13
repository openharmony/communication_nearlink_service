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

/****************************************************************************
 *
 * this file contains timeout process functions for autorate modules.
 *
 ***************************************************************************/

#ifndef QOSM_AUTORATE_TIMEOUT_PROCESS_H
#define QOSM_AUTORATE_TIMEOUT_PROCESS_H

#include "qosm_icg_types.h"
#include "cm_icb_def.h"

#ifdef __cplusplus
extern "C" {
#endif

void QOSM_ChangeLabelStopTimer(QOSM_ICGInfo *icgInfo);
void QOSM_ChangeLabelSetTimer(int *timerHandle, uint8_t qosId, uint16_t direction);

void QOSM_ICBUpLevelDelayStopTimer(QOSM_ICGInfo *icgInfo);
void QOSM_ICBUpLevelDelaySetTimer(int *timerHandle, uint8_t qosId);

void QOSM_LevelUpExceptionProc(uint8_t qosId);
void QOSM_LevelDownExceptionProc(uint8_t qosId);

#ifdef __cplusplus
}
#endif
#endif