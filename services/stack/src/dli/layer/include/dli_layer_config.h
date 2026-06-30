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
 * @file         dli_config.h
 * @brief        dli保存读取到的驱动的配置信息，此头文件不对外，只在DLI内部使用
 */

#ifndef DLI_LAYER_CONFIG_H
#define DLI_LAYER_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#include "dli_layer_stru.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DLI_DEFAULT_CMD_NUM 1

void DLI_AllDataSet(uint16_t acbLen, uint16_t acbNum, uint16_t icbLen, uint16_t icbNum);
uint16_t DLI_DataLenGet(DLI_DataType type);
uint16_t DLI_DataNumGet(DLI_DataType type);

bool DLI_IsCmdAllow(void);
// must be used after DLI_IsCmdAllow
void DLI_CmdNumSubOne(void);
void DLI_CmdNumSet(uint8_t num);

#ifdef __cplusplus
}
#endif
#endif