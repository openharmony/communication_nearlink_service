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

#ifndef QOSM_TRANS_CHANNEL_TUNING_H
#define QOSM_TRANS_CHANNEL_TUNING_H

#include <stdint.h>
#include <stdbool.h>
#include "cm_def.h"
#include "qosm_trans_channel.h"
#include "qosm_trans_channel_inner.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    QOSM_TransChannelStatusCbk statusCbk; /* 必填 */
    QOSM_LogicLinkFindCbk logicLinkFindCbk; /* 必填 */
    QOSM_DecreaseChannelSizeCbk decreaseChannelSizeCbk; /* 必填 */
} QOSM_TcTuningCbks_S;

typedef struct {
    QOSM_TransChannelRspParams_S rspParams;
} QOSM_TcTuningCtx_S;

void QOSM_TcTuningCbksRegister(const QOSM_TcTuningCbks_S *args);

void QOSM_TcTuningCbksUnregister(void);

void QOSM_TcTuningLogicLinkSetPhyCbk(CM_LogicLinkSetPhy_S *param);

void QOSM_TcTuningLogicLinkSetMcsCbk(CM_LogicLinkSetMcs_S *param);

void QOSM_TcTuningLogicLinkConnUpdateParamCbk(CM_LogicLinkConnUpdateParam_S *param);

uint32_t QOSM_TcTuningStartWithStm(const QOSM_TcTuningCtx_S *ctx);

void QOSM_TcTuningStopStm(uint16_t lcid);

#ifdef __cplusplus
}
#endif

#endif  // QOSM_TRANS_CHANNEL_TUNING_H