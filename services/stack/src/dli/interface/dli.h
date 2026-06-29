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
 * @brief        dli模块的北向接口
 */
#ifndef DLI_H
#define DLI_H

#include "dli_event_struct.h"
#include "dli_cmd_struct.h"
#include "dli_opcode.h"
#include "dli_callback.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  DLI模块初始化
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_Init(void);

/**
 * @brief  DLI模块去初始化
 */
void DLI_DeInit(void);

/**
 * @brief  DLI开关使能
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_Enable(void);

/**
 * @brief  DLI开关去使能
 */
void DLI_Disable(void);

/**
 * @brief  各模块向DLI注册事件处理回调
 * @param  [in]  < module > 模块名
 * @param  [in]  < innerTable > 注册的公共事件处理的映射表, 已知ModuleType模块可为空，新增模块为必选项
 * @param  [in]  < innerSize > innerTable表的大小, innerTable为空时需要为0
 * @param  [in]  < table > regOpcode与回调的映射表,不可为空
 * @param  [in]  < size > 表的大小
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size);

/**
 * @brief  各模块向DLI解注册事件处理回调
 * @param  [in]  < module > 模块名
 * @param  [in]  < innerTable > 注册的公共事件处理的映射表, 已知ModuleType模块可为空，新增模块为必选项
 * @param  [in]  < innerSize > innerTable表的大小, innerTable为空时需要为0
 * @param  [in]  < table > regOpcode与回调的映射表, 不可为空
 * @param  [in]  < size > table表的大小
 */
void DLI_CmdCbkUnReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size);

/**
 * @brief  各模块向DLI注册发送完成事件处理回调
 * @param  [in]  < module > 模块名
 * @param  [in]  < cbk > 模块对应的回调函数指针
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_RegNOCPEventCbk(DLI_RegModuleType module, DLI_NOCPEventCbk cbk);

/**
 * @brief  各模块向DLI解注册发送完成事件处理回调
 * @param  [in]  < module > 模块名
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_UnregNOCPEventCbk(DLI_RegModuleType module);
#ifdef __cplusplus
}
#endif

#endif