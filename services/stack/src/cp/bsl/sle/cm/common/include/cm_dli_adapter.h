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
 * The adapter of dli.
 *
 ***************************************************************************/

#ifndef CM_DLI_ADAPTER_H
#define CM_DLI_ADAPTER_H

#include <stdint.h>
#include "dli_cmd_struct.h"
#include "dli_layer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CM_DLI_ADAPTER_CM = 0,
    CM_DLI_ADAPTER_ICB,
    CM_DLI_ADAPTER_DTAP,
    CM_DLI_ADAPTER_MODULE_MAX
} CM_DLI_ADAPTER_MODULE;

typedef enum {
    CM_DLI_ADAPTER_CONNECT = 0,
    CM_DLI_ADAPTER_DISCONNECT,
    CM_DLI_ADAPTER_TYPE_MAX
} CM_DLI_ADAPTER_TYPE;

typedef void (*CM_DliCbk)(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);

uint32_t CM_DliAdapterInit(void);

uint32_t CM_DliAdapterDeinit(void);

uint32_t CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type, CM_DliCbk cbk);

uint32_t CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE module, CM_DLI_ADAPTER_TYPE type);

#ifdef __cplusplus
}
#endif

#endif