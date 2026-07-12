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

#ifndef DLI_LAYER_CALLBACK_H
#define DLI_LAYER_CALLBACK_H

#include <stdint.h>
#include "dli_layer_stru.h"
#include "dli_callback.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*DLI_RecvEventHandler)(uint16_t event, void *context, const uint8_t *data, uint32_t len);

// 设置/清空 dli数据流的写文件回调函数, 如果耗时较久，需要切换其他线程去写文件
void DLI_SetRecvEventCallback(DLI_RecvEventHandler handler);
void DLI_AcbRecvHander(uint16_t lcid, SDF_Buff_S *buf);
void DLI_EventRecvHandler(uint16_t event, void *context, const uint8_t *data, uint32_t len);
void DLI_DftReportKill(uint16_t switchType);
DLI_Callback *DLI_GetCallback(void);
void DLI_DataNumChange(DLI_DataType type, uint16_t dataNum);

#ifdef __cplusplus
}
#endif
#endif