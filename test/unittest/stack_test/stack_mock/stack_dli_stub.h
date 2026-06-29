/*
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

#ifndef STACK_DLI_STUB_H
#define STACK_DLI_STUB_H

#include "dli_opcode.h"
#include "dli_cmd_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

void TEST_DLI_Init();

void TEST_DLI_DeInit();

uint32_t TEST_DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size);

void TEST_DLI_CmdCbkUnReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size);

uint32_t TEST_DLI_SetMeasureParam(DLI_SetMeasureConfigParam *param);

uint32_t TEST_DLI_SetMeasureEnable(DLI_SetMeasureEnableParam *param);

uint32_t TEST_DLI_ReadRemoteMeasureCaps(DLI_ReadRemoteMeasureCapsParam *param);

uint32_t TEST_DLI_EnableIMGEncryption(DLI_IMGEncryptParam *param);

/**
 * @brief 创建或初始化逻辑链路。
 * 部分模块启动时，会调用DLI_CmdCbkReg函数，基于业务注册DLI监听。
 * 在DT测试环境下，测试代码中首先调用 STUB_DLI_Init 函数，完成DLI模块的公共桩初始化
 * 在初始化之后，在DT测试代码中可以通过调用 TEST_DLI_EventCbk 函数，来触发DLI命令的回调
 * @param opcode 用于唯一标识要触发的命令。
 * @param status 命令执行结果，成功或失败。
 * @param cmdRes 命令执行结果内容。
 * @return None
 */
void TEST_DLI_EventCbk(uint8_t opcode, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);

#ifdef __cplusplus
}
#endif

#endif

