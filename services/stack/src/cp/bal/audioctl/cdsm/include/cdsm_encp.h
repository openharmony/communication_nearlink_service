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
#ifndef CDSM_ENCP_H
#define CDSM_ENCP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CDSM_INVALID_SET_ID 0xFFFFFFFF

typedef enum CDSM_HashType {
    CDSM_HMAC_SM3 = 1,
    CDSM_AES_CMAC,
} CDSM_HashType_E;

uint32_t CDSM_GenCoopSetId(uint8_t *key, uint8_t keyLen, uint8_t type, uint16_t *rand, uint32_t *out);

#ifdef __cplusplus
}
#endif

#endif