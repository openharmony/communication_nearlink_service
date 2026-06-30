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

#ifndef NAI_DFT_H
#define NAI_DFT_H

#include <stdint.h>

#include "nearlink_dft_manager_c.h"
#include "nlstk_dft_api.h"
#include "sdf_mem.h"
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DFT_PEER_INFO_PARAMS_SIZE 2
#define NAI_DEVICE_ADDR 1
#define TIME_STAMP_SIZE 20
#define DFT_ADDR_SIZE 20
#define DFT_STRING_LEN 64

typedef enum {
    PARAM_INDEX_0 = 0,
    PARAM_INDEX_1,
    PARAM_INDEX_2,
} DFT_PARAM_INDEX;

typedef enum {
    NAI_DFT_PARAM_SIZE_2 = 2,
    NAI_DFT_PARAM_SIZE_3,
} DFT_PARAM_SIZE;

typedef enum TransType {
    NL_TRANS_SLB = 0,
    NL_TRANS_SLE = 1
} TransType;

void DftGetTimestamp(char *timestamp, uint16_t timelen);
DftParamC CreateUi16ParamC(uint16_t paramId, uint16_t value);
DftParamC CreateStrParamC(int paramId, uint16_t size, const char *addr);
DftParamC CreateRefParamC(uint16_t paramId, DftSubEventRefC *ref);
DftSubEventRefC *CreateSubEventRef(uint16_t paramId, DftParamC params[], size_t size);

DftParamC *CreateAddrTimestampParams(const SLE_Addr_S *addr, uint16_t len, uint16_t paramId);
DftParamC *CreateAddrUi16Params(const SLE_Addr_S *addr, uint16_t len, uint16_t paramId, uint16_t paramVal);

void DftFreeBasicTypeParams(DftParamC *params, size_t size);
void DftFreeParamsWithSubParam(DftParamC *params, size_t size);

uint32_t NAI_DftInit(void);

struct SDF_EncryptedLogString GetAddr(const SLE_Addr_S *addr);
#define DFT_GET_ADDR(addr) ((const char *)(GetAddr(addr).buf))

#ifdef __cplusplus
}
#endif

#endif /* NAI_DFT_H */
