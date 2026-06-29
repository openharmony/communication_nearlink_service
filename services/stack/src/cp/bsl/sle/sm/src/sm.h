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
#ifndef SM_H
#define SM_H

#include <stdint.h>

#include "sdf_addr.h"
#include "sm_slink.h"
#include "nlstk_sm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

void SmStartPairing(void *addrIn);
void SmRemovePairing(void *addrIn);
void SmSetConfirm(void *addrIn);
void SmSetPassCode(void *passCodeIn);
void SmSetPassWord(void *passWordIn);
void SmSetLocalPsk(void *pskIn);
void SmRecoverKey(void *recoverKeyListIn);
void SmSetSecurityParams(void *paramsIn);
void SmRegExternalCbks(NLSTK_SmCallbacks_S *cbksIn);
void SmRegImgCbks(NLSTK_SmImgCallbacks_S *cbksIn);
void SmExternalCbks(uint8_t event, void *params);
void SmRemoveSLink(void *addrIn);
SmSLink_S *SmFindSLinkByLcid(uint16_t lcid);
SmSLink_S *SmFindSLink(SLE_Addr_S *addr);
SmSLink_S *SmFindOrCreateSLink(SLE_Addr_S *addr);
NLSTK_SmLocalParams_S *SmGetLocalParams(void);

/* Message Process */
bool SmSendMessage(SmSLink_S *slink, uint16_t opcode, const uint8_t *pkg, size_t size);
void SmPkgDispatcher(uint16_t opcode, SLE_Addr_S *addr, const uint8_t *pkg, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* SM_H */