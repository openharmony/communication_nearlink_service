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
#ifndef ICCE_UTILS_H
#define ICCE_UTILS_H

#include <string.h>
#include "ssap_type.h"

#ifdef __cplusplus
extern "C" {
#endif

uint16_t IcceConvertUuidTo16Bits(NLSTK_SsapUuid_S uuidStru);

NLSTK_SsapUuid_S IcceConvertUuidToStru(uint16_t uuid);

bool IcceCompAppId(void *ptr, void *args);

bool IcceCompAddr(void *ptr, void *args);

#ifdef __cplusplus
}
#endif
#endif