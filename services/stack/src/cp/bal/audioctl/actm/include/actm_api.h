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
#ifndef ACTM_API_H
#define ACTM_API_H

#include "actm_api_type.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t NLSTK_ActmReadRemoteProp(SLE_Addr_S *addr);

uint32_t NLSTK_ActmDisconnect(SLE_Addr_S *addr);

uint32_t NLSTK_ActmCreateStream(SLE_Addr_S* addr, NLSTK_ActmStreamParam_S *param);

uint32_t NLSTK_ActmDeleteStream(SLE_Addr_S* addr, uint8_t streamId);

uint32_t NLSTK_ActmConfigAudioStream(SLE_Addr_S *addr, NLSTK_ActmConfigParam_S *param);

uint32_t NLSTK_ActmStartAudioStream(SLE_Addr_S *addr, NLSTK_ActmConfigParam_S *param);

uint32_t NLSTK_ActmOpenAudioStream(SLE_Addr_S *addr, NLSTK_ActmOpenParam_S *param);

uint32_t NLSTK_ActmChangeAudioStream(SLE_Addr_S *addr, NLSTK_ActmChangeParam_S *param);

uint32_t NLSTK_ActmReleaseAudioStream(SLE_Addr_S *addr, NLSTK_ActmReleaseParam_S *param);

uint32_t NLSTK_ActmRegisterCallback(NLSTK_ActmCbk_S *cbk);

uint32_t NLSTK_ActmSetDirection(SLE_Addr_S *addr, uint8_t direction);

uint32_t NLSTK_ActmUpdateBitrate(SLE_Addr_S *addr, NLSTK_ActmBitrateParam_S *param);

uint32_t NLSTK_ActmRecvAutoRateMsg(SLE_Addr_S *addr, NLSTK_ActmAutoRateRecvMsg_S *param);

#ifdef __cplusplus
}
#endif
#endif