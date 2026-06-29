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

#ifndef QOSM_AUTORATE_NOTIFY_H
#define QOSM_AUTORATE_NOTIFY_H

#include "qosm_icg_types.h"
#include "cm_icb_def.h"
#include "qosm_autorate_common.h"

#ifdef __cplusplus
extern "C" {
#endif

void QOSM_ICGMgrParamNotify(void *param);

void QOSM_UpdateICG(CM_ICBConnection *connection);
void QOSM_UpdateICGComplete(QOSM_ICGInfo *icgInfo);

bool QOSM_IsLevelSupported(QOSM_ICGInfo *icgInfo, struct QosLevelLabel *level, QOSM_QosLevel targetLevel);
void QOSM_CopySupportedBitrate(QOSM_ICGInfo *icgInfo, const uint16_t *supportedBitrate,
    uint32_t supportedBitrateCnt);
void QOSM_SetLevelSupported(QOSM_ICGInfo *icgInfo, struct QosLevelLabel *level, uint32_t supportedBitrateCnt,
    const uint16_t *supportedBitrate, QOSM_QosLevel targetLevel);

#ifdef __cplusplus
}
#endif
#endif