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
#ifndef NLSTK_DFT_H
#define NLSTK_DFT_H

#include <stdint.h>
#include "sdf_addr.h"
#include "nlstk_dft_api.h"

#ifdef __cplusplus
extern "C" {
#endif

void DftCacheTimestamp(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId);
void DftCache(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId,
    NLSTK_DftParamValueType_E paramType, void *param);
void DftReport(SLE_Addr_S *addr, uint16_t eventId, uint16_t paramId, uint16_t res);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NLSTK_DFT_H */