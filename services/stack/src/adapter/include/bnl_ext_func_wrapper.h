/**
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

#ifndef BNL_EXT_FUNC_WRAPPER_H
#define BNL_EXT_FUNC_WRAPPER_H

#include <stdint.h>
#include "nlstk_bnl_type_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

void BNL_ProxyLinkStateChange(NLSTK_BnlLogicLinkState_S *state);

void BNL_ProxyRecvMsg(NLSTK_BnlSendMsg_S *msg);

uint32_t BNL_ProxyInit(NLSTK_BnlProxyFunc_S *func);

void BNL_ProxyDeInit(void);

#ifdef __cplusplus
}
#endif

#endif