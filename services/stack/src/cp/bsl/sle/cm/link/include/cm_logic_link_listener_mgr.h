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
 * this file contains logic link listener manager
 *
 ***************************************************************************/

#ifndef CM_LOGIC_LINK_LISTENER_MGR_H
#define CM_LOGIC_LINK_LISTENER_MGR_H

#include "cm_def.h"
#include "cm_logic_link_api.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t CM_RegLogicLinkCbks(CM_LogicLinkCbks_S *cbks);

void CM_NotifyLogicLinkDtapCbks(CM_LogicLinkState_S *state);

void CM_NotifyLogicLinkCbks(CM_LogicLinkState_S *state);

void CM_ExecLogicLinkModuleCbks(uint8_t moduleId, CM_LogicLinkState_S *state);

void CM_ExecLogicLinkRemoteFeaturesCbks(CM_LogicLinkRemoteFeatures_S *param);

void CM_ExecLogicLinkConnUpdateParamCbks(CM_LogicLinkConnUpdateParam_S *param);

uint32_t CM_UnRegLogicLinkCbks(uint16_t moduleId);

#ifdef __cplusplus
}
#endif

#endif /* CM_LOGIC_LINK_LISTENER_MGR_H */