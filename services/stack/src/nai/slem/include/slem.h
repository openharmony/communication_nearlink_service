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
 * Description: sle manager header
 */
#ifndef STACK_SLEM_INCLUDE_SLEM_H
#define STACK_SLEM_INCLUDE_SLEM_H

#include "slem_dli_callback.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SLB_CONTROLLER 1
#define SLE_CONTROLLER 2

#define SLE_CONNECTION_ROLE_PRIMARY 0x00
#define SLE_CONNECTION_ROLE_SECONDARY 0x01

void slem_registerCallbacks(SlemCallbacks *callbacks);

void slem_deregisterCallbacks(void);

int slem_initialize(void);

int slem_close(void);

int slem_enable(void);

int slem_disable(void);

int slem_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif