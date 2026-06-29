/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
 * Description: sle dli layer header
 */
#ifndef STACK_SLEM_INCLUDE_SLE_DLI_LAYER_H
#define STACK_SLEM_INCLUDE_SLE_DLI_LAYER_H

#include <stdint.h>

/* interfaces for slem */
uint32_t sle_dli_init(void);

uint32_t sle_dli_enable(void);

uint32_t sle_dli_disable(void);

uint32_t sle_dli_cleanup(void);

#endif