/**
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

/****************************************************************************
 *
 * sm模块的回调处理
 *
 ***************************************************************************/

#ifndef DLI_SECU_EVENT_H
#define DLI_SECU_EVENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void DLI_EncryptParamReqCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_EncryptChangeCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_ControllerDataCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

#ifdef __cplusplus
}
#endif

#endif