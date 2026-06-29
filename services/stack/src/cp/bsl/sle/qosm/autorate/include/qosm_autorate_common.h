/****************************************************************************
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
 ****************************************************************************/

/****************************************************************************
 *
 * this file contains common functions shared between autorate modules.
 *
 ***************************************************************************/

#ifndef QOSM_AUTORATE_COMMON_H
#define QOSM_AUTORATE_COMMON_H

#include "qosm_icg_types.h"
#include "cm_icb_def.h"
#include "qosm_autorate_timeout_process.h"

#ifdef __cplusplus
extern "C" {
#endif

void QOSM_LevelDownProc(QOSM_ICGInfo *icgInfo, bool isByReport);
bool QOSM_LevelUpProc(QOSM_ICGInfo *icgInfo);
uint32_t QOSM_UpdateLabelId(QOSM_ICGInfo *icgInfo);
void QOSM_ResetReportParam(QOSM_ICGInfo *icgInfo);
void QOSM_ICBQualityReportCheck(QOSM_ICGInfo *icgInfo);
bool QOSM_IsAutorateSupported(QOSM_ICGInfo *icgInfo);
bool QOSM_AutorateIsDspOn(void);
void QOSM_AutorateSetDspOn(bool on);
void QOSM_AutorateUpdateDspStatus(void);
void QOSM_ResetUpdateParam(QOSM_ICGInfo *icgInfo);
bool QOSM_RollbackWhenDownLevel(QOSM_ICGInfo *icgInfo);
void QOSM_ResetAllICBAckRateOverCnt(QOSM_ICGInfo *icgInfo);
bool QOSM_HandleDownwardBitrateConstraint(QOSM_ICGInfo *icgInfo);
bool QOSM_ICGMgrIsDutyCycleMatch(uint8_t dutyCycle);
void QOSM_ProcAutoRateMsg(void *param);
void QOSM_HandleFrameTypeChanged(QOSM_ICGInfo *icgInfo, uint8_t result, bool isLevelUp);
void QOSM_HandlePeerBitrateChanged(QOSM_ICGInfo *icgInfo, uint8_t targetLabel, uint8_t result, bool isLevelUp);
void QOSM_SetVoiceCallAutorateAbility(QOSM_ICGInfo *icgInfo);

#ifdef __cplusplus
}
#endif
#endif