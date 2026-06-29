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

#ifndef DLI_CALLBACK_H
#define DLI_CALLBACK_H

#include <stdint.h>
#include "dli_layer_stru.h"
#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DLI_SWITCH_OPEN = 1,
    DLI_CHIP_KILL = 6
} DLI_SWITCH_TYPE;

typedef enum {
    OPEN_SUCCESS,
    OPEN_FAIL
} DLI_SWITCH_OPER_RESULT;

typedef enum {
    RECEIVE_DATA_TIMEOUT = 5
} DLI_SWITCH_ERROR_CODE;

typedef uint32_t (*DLI_PostOtherThreadPtr)(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb);
typedef uint32_t (*DLI_PostOtherBlockedThreadPtr)(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout);
typedef void (*DLI_DftReportKillPtr)(uint16_t switchType);
typedef void (*DLI_RecvAcbHandlerPtr)(uint16_t lcid, SDF_Buff_S *buf);
typedef uint16_t (*DLI_GetExtRegOpcodePtr)(uint16_t code);
typedef bool (*HADM_ProcessCsCapsPtr)(const uint8_t *rawData, uint32_t len, bool isLocal);

typedef struct {
    DLI_PostOtherThreadPtr postOtherThread;               /* 必选，切其他线程 */
    DLI_PostOtherBlockedThreadPtr postOtherBlockedThread; /* 必选，切其他线程，阻塞当前线程 */
    DLI_RecvAcbHandlerPtr recvAcbHandler;                 /* 必选. 异步数据接收回调 */
    DLI_DftReportKillPtr dftReportKill;                   /* 可选，dft上报 */
    DLI_GetExtRegOpcodePtr getExtRegOpcode;               /* 可选，在处理命令异常的情况下,
                                          获取自定义的命令的映射关系,最终回调到自定义事件处理 */
    HADM_ProcessCsCapsPtr hadmProcessCsCaps;              /* 可选，处理测距能力其他字段 */
} DLI_Callback;

typedef void (*DLI_DataNumChangecbk)(DLI_DataType type, uint16_t dataNum);

/**
 * @brief  设置DLI内部处理回调，用于解耦其他模块的依赖
 * @param  [in] callback: 回调任务
 * @return 0: 成功, OTHER: 失败
 */
uint32_t DLI_SetCallback(const DLI_Callback *callback);

/**
 * @brief  注册DLI data num变化监听
 * @param  [in] cbk: DLI data num变化回调函数
 */
void DLI_DataNumChangeRegister(DLI_DataNumChangecbk cbk);

#ifdef __cplusplus
}
#endif

#endif