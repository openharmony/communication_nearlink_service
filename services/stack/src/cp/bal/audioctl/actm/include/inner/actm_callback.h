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
#ifndef ACTM_CALLBACK_H
#define ACTM_CALLBACK_H

#include "actm_api_type.h"

#ifdef __cplusplus
extern "C" {
#endif

void ActmSetCallback(NLSTK_ActmCbk_S *cbk);

void ActmEventCbk(SLE_Addr_S *addr, uint8_t event, uint8_t result, void *param);

void ActmPropCbk(SLE_Addr_S *addr, uint8_t num, NLSTK_ActmProp_S *prop);

void ActmBitCbk(NLSTK_ActmBitrateChange_S *bitrate);

void ActmLocationCbk(SLE_Addr_S *addr, bool isLeft);

void ActmStreamTypeCbk(SLE_Addr_S *addr, uint32_t availableStreamType);

void ActmCallBitUpDownCbk(NLSTK_ActmAutoRateSendMsg_S *upDownParam);

#ifdef __cplusplus
}
#endif
#endif