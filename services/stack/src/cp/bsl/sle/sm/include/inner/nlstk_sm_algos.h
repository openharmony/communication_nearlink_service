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
#ifndef NLSTK_SM_ALGOS_H
#define NLSTK_SM_ALGOS_H

#include <stdint.h>
#include <stdbool.h>
#include "nlstk_sm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

bool SmGenRandNum(uint8_t *out, uint8_t len);

bool SmCmacGenerate(NLSTK_SmDerivedMac_S *input, uint8_t *output, uint8_t outputLen);

bool SmGenSha256(NLSTK_VariableData_S *value, NLSTK_SmSha256Hash *shaHash);

#ifdef __cplusplus
}
#endif

#endif /* NLSTK_SM_ALGOS_H */