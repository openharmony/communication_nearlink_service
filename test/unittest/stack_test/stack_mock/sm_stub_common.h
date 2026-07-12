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

#include "sdf_mem.h"
#include "sdf_map.h"
#include "sdf_string.h"
#include "nlstk_log.h"
#include "dli.h"
#include "dli_cmd.h"
#include "dli_errno.h"
#include "cm_def.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "nlstk_init_api.h"
#include "nlstk_cfgdb.h"
#include "nlstk_cfgdb_api.h"
#include "cp_worker.h"

#include "sm_slink.h"
#include "sm_algos.h"
#include "sm_passcode.h"
#include "sm_password.h"
#include "sm_psk.h"
#include "sm_oob.h"
#include "sm.h"
#include "nlstk_sm.h"
#include "nlstk_sm_api.h"

#ifndef SM_STUB_COMMON_H
#define SM_STUB_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

extern SLE_Addr_S g_gAddr;
extern SLE_Addr_S g_tAddr;

uint32_t NLSTK_CfgdbGetPublicAddressForGStub(SLE_Addr_S *addr);
uint32_t NLSTK_CfgdbGetPublicAddressForTStub(SLE_Addr_S *addr);
uint32_t CM_GetLogicLinkByLcidForGStub(uint16_t lcid, CM_LogicLink_S *logicLink);
uint32_t CM_GetLogicLinkByLcidForTStub(uint16_t lcid, CM_LogicLink_S *logicLink);
uint32_t CM_GetLogicLinkByAddrForGStub(SLE_Addr_S *addr, CM_LogicLink_S *logicLink);
uint32_t CM_GetLogicLinkByAddrForTStub(SLE_Addr_S *addr, CM_LogicLink_S *logicLink);
uint32_t DLI_CmdCbkRegStub(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size);
void SendBuffer(DLI_ControllerDataEvt *buff);
void Crypto_RandNumGenerateStub(uint8_t *out, uint8_t len);
void Crypto_PubPriKeyPairGenerateStub(NLSTK_SmKeyPair_S *keyPair);
void Crypto_SecKeyGenerateStub(NLSTK_SmKeyPair_S *keyPair, uint8_t *secKey, uint8_t secKeyLen);
void Crypto_DerivedKeyGenerateStub(NLSTK_SmDerivedMac_S *input, uint8_t *output, uint8_t outputLen);
void WaitTimeoutCbk(void *slinkIn);

#ifdef __cplusplus
}
#endif

#endif