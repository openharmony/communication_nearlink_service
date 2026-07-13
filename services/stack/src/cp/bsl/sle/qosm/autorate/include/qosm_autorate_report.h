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
 * this file contains autorate report decision logic:
 * QOSM_ICBQualityReport periodic quality report to decision up/down bitrate.
 *
 ***************************************************************************/

#ifndef QOSM_AUTORATE_REPORT_H
#define QOSM_AUTORATE_REPORT_H

#include "qosm_icg_types.h"
#include "cm_icb_def.h"
#include "qosm_autorate_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void QOSM_PrintQualityReportParam(CM_ICBQuality *param);
void QOSM_ICBQualityReport(CM_ICBQuality *param);
void QOSM_DspStatusCbk(bool isOn);
void QOSM_DspFlowCtrlCbk(uint16_t connHandle, bool enterFlowCtrl);
bool QOSM_ExecuteBitrateChangeDecision(uint16_t connHandle, uint16_t qosIndex, uint16_t reportedDirection,
    uint16_t reportedQosLevel);

#ifdef __cplusplus
}
#endif
#endif