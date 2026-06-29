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
#ifndef CDSM_API_H
#define CDSM_API_H

#include <stdint.h>
#include "cdsm_event.h"
#include "sdf_vector.h"
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t NLSTK_CdsmCreateSet(SLE_Addr_S *addr);

void NLSTK_CdsmRemoveSet(uint32_t gid);

void NLSTK_CdsmRecoverMeb(uint32_t gid, uint8_t num, SLE_Addr_S *addr);

void NLSTK_CdsmStartAdv(SLE_Addr_S *addr);

void NLSTK_CdsmStopAdv(SLE_Addr_S *addr);

uint32_t NLSTK_CdsmConnect(SLE_Addr_S *addr);

uint32_t NLSTK_CdsmDisconnect(SLE_Addr_S *addr);

uint32_t NLSTK_CdsmFindGidByAddr(SLE_Addr_S *addr);

SDF_Vector_S *NLSTK_CdsmFindAllAddrByGid(uint32_t gid);

void NLSTK_CdsmRegisterEventCbk(NLSTK_CdsmEventCbk func);

#ifdef __cplusplus
}
#endif

#endif