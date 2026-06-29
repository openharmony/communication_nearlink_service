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
#ifndef SM_DHKEY_H
#define SM_DHKEY_H

#include <stdint.h>
#include "sm_slink.h"

#ifdef __cplusplus
extern "C" {
#endif

void SmSendGNodeDhKey(SmSLink_S *slink);
void SmRecvGNodeDhKey(SmSLink_S *slink, const uint8_t *pkg, size_t size);
void SmSendTNodeDhKey(SmSLink_S *slink);
void SmRecvTNodeDhKey(SmSLink_S *slink, const uint8_t *pkg, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* SM_DHKEY_H */