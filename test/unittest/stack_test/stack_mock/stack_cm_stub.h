/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef STACK_CM_STUB_H
#define STACK_CM_STUB_H

#include "cm_def.h"
#include "dli_opcode.h"
#include "dli_cmd_struct.h"
#include "cm_trans_channel_api.h"

#ifdef __cplusplus
extern "C" {
#endif

void TEST_CM_Init();

void TEST_CM_DeInit();

uint32_t TEST_CM_GetLogicLinkConnectedSize(void);

uint32_t TEST_CM_RegLogicLinkListener(CM_LogicLinkCbks_S *logicLinkCbks);

uint32_t TEST_CM_UnRegLogicLinkListener(uint8_t moduleId);

uint32_t TEST_CM_RegTransChannelListener(CM_TransChannelCbk cbk);

void TEST_CM_TransChannelDo(CM_TransChannelStateList_S *channelState);

uint32_t TEST_CM_GetLogicLinkByAddr(SLE_Addr_S *addr, CM_LogicLink_S *logicLink);

void TEST_CM_UnRegTransChannelListener(void);

void TEST_CM_CreateLogicLink(uint16_t moduleId, CM_LogicLinkState_S *logicLinkState);

void TEST_CM_RemoteFeatureCbk(uint16_t moduleId, CM_LogicLinkRemoteFeatures_S *param);

void TEST_CM_ConnUpdateCbk(uint16_t moduleId, CM_LogicLinkConnUpdateParam_S *param);

#ifdef __cplusplus
}
#endif

#endif

