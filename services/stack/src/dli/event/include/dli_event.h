/**
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
 */

/****************************************************************************
 *
 * dli回调事件的处理和分发
 *
 ***************************************************************************/

#ifndef DLI_EVENT_H
#define DLI_EVENT_H

#include <stdint.h>
#include "dli_def.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

void DLI_InitEventCbkList(void);

void DLI_DeInitEventCbkList(void);

uint32_t DLI_InnerEventCbkReg(const DLI_InnerCbkLineStru *table, const uint32_t size);

void DLI_InnerEventCbkUnReg(const DLI_InnerCbkLineStru *table, const uint32_t size);

void RecvEventHandler(uint16_t event, void *context, const uint8_t *data, uint32_t len);

void DLI_NumberOfCompletedPacketsCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_CommandStatusCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_CommandErrorCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_RemoveCbks(const DLI_CbkLineStru *table, const uint32_t size);

void DLI_AddCbks(const DLI_CbkLineStru *table, const uint32_t size);

DLI_ExecuteCmdCbk DLI_GetCbk(const uint16_t opcode);

void DLI_RunRegCbk(uint16_t regOpcode, void *context, void *arg, uint32_t size, uint16_t evtOpcode, uint16_t status);
#ifdef __cplusplus
}
#endif

#endif