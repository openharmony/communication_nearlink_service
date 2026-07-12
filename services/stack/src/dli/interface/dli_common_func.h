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

#ifndef DLI_COMMON_FUNC_H
#define DLI_COMMON_FUNC_H

#include <stdint.h>
#include "dli_cmd_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t (*DLI_CmdCbkRegisterPtr)(const ModuleType module,
    const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size);
typedef void (*DLI_CmdCbkDeregisterPtr)(const ModuleType module,
    const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size);
typedef uint32_t (*DLI_SetCmdPtr)(DLI_CmdParams *param);
typedef DLI_ExecuteCmdCbk (*DLI_GetCbkPtr)(const uint16_t opcode);

typedef struct {
    DLI_CmdCbkRegisterPtr cmdCbkReg;
    DLI_CmdCbkDeregisterPtr cmdCbkDereg;
    DLI_SetCmdPtr setCmd;
    DLI_GetCbkPtr getCbk;
} DLI_OpenFuncList;

#ifdef __cplusplus
}
#endif

#endif