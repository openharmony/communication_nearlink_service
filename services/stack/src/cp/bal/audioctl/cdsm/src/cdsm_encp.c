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
#include "cdsm_encp.h"
#include "cdsm_tbl.h"
#include "nlstk_sm_algos.h"
#include "nlstk_public_define.h"
#include "cpfwk_log.h"
#include "ssap_utils.h"
#include "sdf_mem.h"
#include "securec.h"
#include "sdf_string.h"

#define HMAC_SM3_MAC_LEN 32
#define AES_CMAC_MAC_LEN 16
#define RANDOM_LEN 2
#define OUTPUT_LEN 4

uint32_t CDSM_GenCoopSetId(uint8_t *key, uint8_t keyLen, uint8_t type, uint16_t *rand, uint32_t *out)
{
    CP_CHECK_LOG_RETURN(key != NULL && keyLen == CDSM_KEY_LEN && rand != NULL && out != NULL,
        NLSTK_ERR, "[CDSM] param error");
    uint8_t macLen = 0;
    uint8_t algo = 0;
    switch (type) {
        case CDSM_HMAC_SM3:
            macLen = HMAC_SM3_MAC_LEN;
            algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA1;
            break;
        case CDSM_AES_CMAC:
            macLen = AES_CMAC_MAC_LEN;
            algo = SM_KEY_DERIVATION_ALGORITHM_ABILITY_HA2;
            break;
        default:
            break;
    }
    CP_CHECK_LOG_RETURN(macLen != 0, NLSTK_ERR, "[CDSM] unsupport type: %u", type);

    uint8_t randIn[RANDOM_LEN] = {0};
    if (!SmGenRandNum(randIn, RANDOM_LEN)) {
        CP_LOG_ERROR("[CDSM] sm gen random num failed!");
        return NLSTK_ERR;
    }
    NLSTK_SmDerivedMac_S input = {0};
    input.algo = algo;
    (void)memcpy_s(input.key, SM_OCTETS_16, key, keyLen);
    input.buff = randIn;
    input.buffSize = RANDOM_LEN;
    uint8_t *mac = (uint8_t *)SDF_MemZalloc(macLen);
    CP_CHECK_LOG_RETURN(mac != NULL, NLSTK_ERR, "[CDSM] mac malloc error");
    if (!SmCmacGenerate(&input, mac, macLen)) {
        CP_LOG_ERROR("[CDSM] SmCmacGenerate failed!");
        SDF_MemFree(mac);
        return NLSTK_ERR;
    }

    uint8_t outMac[OUTPUT_LEN] = {0};
    (void)memcpy_s(outMac, OUTPUT_LEN, mac, OUTPUT_LEN);
    *rand = SSAP_BYTE_TO_UINT16_LITTLE(randIn);
    *out = SSAP_BYTE_TO_UINT32_LITTLE(outMac);
    SDF_MemFree(mac);

    return NLSTK_OK;
}