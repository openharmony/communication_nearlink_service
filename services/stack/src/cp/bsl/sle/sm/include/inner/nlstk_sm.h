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
#ifndef NLSTK_SM_H
#define NLSTK_SM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void SmDeInit(void);
uint32_t SmInit(void);
void SmEnable(void);

/* SM Api interface functions */
bool SmIsSLinkAuthComplete(uint16_t lcid);              /* 查询安全链路是否已通过鉴权 */
bool SmIsSLinkEncryptComplete(uint16_t lcid);           /* 查询安全链路是否已通过加密 */

#ifdef __cplusplus
}
#endif

#endif /* NLSTK_SM_H */