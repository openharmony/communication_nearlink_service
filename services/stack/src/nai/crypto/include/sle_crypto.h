/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef SLE_CRYPTO_H
#define SLE_CRYPTO_H

#include "nlstk_sm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 密钥派生算法能力
 */
typedef enum {
    KEY_DERIVATION_ALGORITHM_ABILITY_HA1 = 0x01,         /*!< HMAC-SM3 */
    KEY_DERIVATION_ALGORITHM_ABILITY_HA2 = 0x02,         /*!< AES-CMAC 128 */
    KEY_DERIVATION_ALGORITHM_ABILITY_RESERVED,
} KeyDerivAlgoAbility;

/**
 * @brief 密钥协商算法能力
 */
typedef enum {
    KEY_NEGOTIATION_ALGORITHM_ABILITY_KE1 = 0x01,        /*!< SM2 */
    KEY_NEGOTIATION_ALGORITHM_ABILITY_KE2 = 0x02,        /*!< ECDH P-256 */
    KEY_NEGOTIATION_ALGORITHM_ABILITY_RESERVED,
} KeyNegoAlgoAbility;

void Crypto_RandNumGenerate(uint8_t *out, uint8_t len);
void Crypto_PubPriKeyPairGenerate(NLSTK_SmKeyPair_S *keyPair);
void Crypto_SecKeyGenerate(NLSTK_SmKeyPair_S *keyPair, uint8_t *secKey, uint8_t secKeyLen);
void Crypto_DerivedKeyGenerate(NLSTK_SmDerivedMac_S *input, uint8_t *output, uint8_t outputLen);
void Crypto_Sha256(NLSTK_VariableData_S *value, NLSTK_SmSha256Hash *shaHash);

#ifdef __cplusplus
}
#endif

#endif /* SLE_CRYPTO_H */