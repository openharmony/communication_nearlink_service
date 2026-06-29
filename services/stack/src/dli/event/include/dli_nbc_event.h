/**
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

/****************************************************************************
 *
 * nbc模块的回调处理
 *
 ***************************************************************************/

#ifndef DLI_NBC_EVENT_H
#define DLI_NBC_EVENT_H

#include <stdint.h>
#include "dli_event_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

void DLI_ChanInfoCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);
void DLI_ChipResetNotifyCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

#ifdef __cplusplus
}
#endif

#endif