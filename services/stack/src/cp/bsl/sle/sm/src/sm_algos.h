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
#ifndef SM_ALGOS_H
#define SM_ALGOS_H

#include <stdint.h>
#include <stdbool.h>
#include "sm_slink.h"
#include "sm_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

bool SmGenPubPriKey(NLSTK_SmKeyPair_S *keyPair);

bool SmGenDhKey(NLSTK_SmKeyPair_S *keyPair, uint8_t *secKey, uint8_t secKeyLen);

bool SmGenLinkKey(SmSLink_S *slink);

bool SmGenConfirmNum(uint8_t *key, uint8_t keyLen, SmSLink_S *slink, SmConfirmNum_S *confirm);

NLSTK_SmCryptoAlgoFuncs_S* SmGetAlgoFuncs(void);

#ifdef __cplusplus
}
#endif

#endif /* SM_ALGOS_H */