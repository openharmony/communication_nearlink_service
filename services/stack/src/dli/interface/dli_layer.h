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

#ifndef DLI_LAYER_H
#define DLI_LAYER_H

#include <stdbool.h>
#include "dli_layer_stru.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  命令发送接口, 异步
 * @param  [in] < cmd > 要发送的dli命令指针，如果失败，需要调用者释放cmd
 * @return 0 成功，其他失败
 */
uint32_t DLI_CmdSend(DLI_CmdStru *cmd);

/**
 * @brief  获取数据分片数, 同步
 * @param  [in] < buf > 要发送的buf
 * @return 数据分片数，0 失败
 */
uint32_t DLI_GetDataFragmentNums(SDF_Buff_S *buf);

/**
 * @brief  获取数据单片最大长度, 同步
 * @param  无
 * @return 单片最大长度，0 失败
 */
uint32_t DLI_GetFragmentMaxLen(void);

/**
 * @brief  获取数据包的个数
 * @param  type 数据类型，详见 DLI_DataType
 * @return 数据包的个数
 */
uint16_t DLI_DataNumGet(DLI_DataType type);

/**
 * @brief  数据分片接口, 同步
 * @param  [in] < buf > 要发送的buf
 * @param  [out] < fragmentBuf > 分片后的buf
 * @param  [in] < fragmentCnt > 分片数
 * @return 0 成功，其他失败
 */
uint32_t DLI_SplitData(DLI_DataStru *data, SDF_Buff_S *fragmentBuf[], uint32_t fragmentCnt);

/**
 * @brief  数据发送接口, 异步，长度不超过102400字节。发送成功由DLI释放data，发送失败需要调用者释放data
 * @param  [in] < data > 要发送的dli数据指针，如果失败，需要调用者释放data
 * @return 0 成功，其他失败
 */
uint32_t DLI_DataSend(DLI_DataStru *data);

/**
 * @brief  layer层的初始化接口，包含dli线程和资源的初始化
 * @param  无
 * @return 0 成功，其他失败
 */
uint32_t DLI_LayerInit(void);

/**
 * @brief  layer层的反初始化接口，包含dli线程和资源的反初始化
 * @param  无
 * @return 无
 */
void DLI_LayerDeinit(void);

/**
 * @brief  layer层的使能接口，包含注册驱动接收回调函数初始化
 * @param  无
 * @return 0 成功，其他失败
 */
uint32_t DLI_LayerEnable(void);

/**
 * @brief  layer层的反使能接口，包含注册驱动接收回调函数反初始化
 * @param  无
 * @return 无
 */
void DLI_LayerDisable(void);

/**
 * @brief  layer 触发下一个命令/数据任务
 * @param  [in] < type > 触发的下一个任务类型变量参数
 * @return 无
 */
void DLI_PostNextTask(DLI_TaskType type);
#ifdef __cplusplus
}
#endif
#endif // DLI_LAYER_H