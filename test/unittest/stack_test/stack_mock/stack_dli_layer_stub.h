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

#ifndef STACK_DLI_LAYER_STUB_H
#define STACK_DLI_LAYER_STUB_H

#include <stdint.h>
#include "dli_layer_stru.h"

#ifdef __cplusplus
extern "C" {
#endif

void TEST_DLI_LyaerInit(void);
void TEST_DLI_LyaerDeInit(void);
uint32_t TEST_DLI_GetDataFragmentNumsStub(SDF_Buff_S *buf);
uint32_t TEST_DLI_GetFragmentMaxLenStub(void);
uint32_t TEST_DLI_SplitDataStub(DLI_DataStru *data, SDF_Buff_S *fragmentBuf[], uint32_t fragmentCnt);
uint32_t TEST_DLI_DataSendStub(DLI_DataStru *data);
void TEST_DLI_PostNextTaskStub(DLI_TaskType type);
uint32_t TEST_DLI_CmdSendStub(DLI_CmdStru *cmd);
#ifdef __cplusplus
}
#endif

#endif