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


#ifndef CM_LINK_COLLAB_FUNC_H
#define CM_LINK_COLLAB_FUNC_H
#include <stdbool.h>
#include "sdf_addr.h"
#include "nlstk_stm_collab_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t CM_LinkCollabRegFunc(const CM_LinkCollabFunc_S *funcs);
void CM_LinkCollabUnRegFunc(void);
bool CM_IsNeedLinkCollabReq(void);
bool CM_StartLinkCollabReq(uint8_t connInitiateType, const SLE_Addr_S *directConnAddr);
bool CM_NotifyLinkCollabResult(void);

#ifdef __cplusplus
}
#endif

#endif