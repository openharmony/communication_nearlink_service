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
 * this file contains icb qos manager: listen controller icb qos event,
 * and output max bitrate by rssi, ack rate and packet interval.
 *
 ***************************************************************************/

#ifndef QOSM_ICG_CALLBACK_H
#define QOSM_ICG_CALLBACK_H

#include "qosm_icg_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void QOSM_NotifyParamChangedSuccessCb(QOSM_ICGInfo *icgInfo, QOSM_ParamState state);
void QOSM_NotifyParamChangedFailCb(uint8_t qosId, QOSM_ParamState state);
void QOSM_NotifyConnFailCbk(uint8_t qosId, QOSM_ConnectionState state, uint16_t *connHandle, uint8_t linkCnt);
void QOSM_NotifyConnCbk(QOSM_ICGInfo *icgInfo, QOSM_ConnectionState state, uint32_t result,
    QOSM_ConnParam *link, uint8_t linkCnt);
void QOSM_NotifyDataPathChangedCb(uint8_t qosId, QOSM_DataPathState state,
    uint32_t result, uint16_t connHandle, uint8_t direction);
void QOSM_NotifyReportedBitrateChangedCb(const QOSM_ICGInfo *icgInfo, uint16_t reportedDirection,
    uint8_t labelId, const QOSM_LinkParam *qosParam);
void QOSM_SendAutoRateMsg(const QOSM_ICGInfo *icgInfo, uint16_t direction, uint8_t labelId,
    uint8_t msgType, uint32_t result);
bool QOSM_AutorateIsBitrateChangeCbkValid(void);

void QOSM_AutorateUplayerCallbackInit(const QOSM_AutoRateCallback *callback);
void QOSM_AutorateUplayerCallbackUninit(void);

#ifdef __cplusplus
}
#endif
#endif