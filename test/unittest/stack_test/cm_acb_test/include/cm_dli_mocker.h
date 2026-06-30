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
#ifndef CM_DLI_MOCKER_H
#define CM_DLI_MOCKER_H

#include <stdint.h>
#include "dli_cmd_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

const DLI_CbkLineStru *DLI_GetCbk(void);
uint8_t DLI_GetCbkSize(void);
const DLI_CbkLineStru *DLI_GetDisconectCbk(void);
uint8_t DLI_GetDisconnectCbkSize(void);

#ifdef __cplusplus
}
#endif

#endif // CM_DLI_MOCKER_H