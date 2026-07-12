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

#ifndef DLI_THREAD_H
#define DLI_THREAD_H

#include <stdint.h>
#include "sdf_worker.h"
#include "sdf_evc.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t DLI_ThreadInit(void);
void DLI_ThreadDeinit(void);
int DLI_ThreadEvcHandleGet(void);
uint32_t DLI_ThreadAddWork(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb);
void DLI_ThreadPostEvent(void);
/* 如果失败，需要外部负责释放资源，如果成功，资源所有权转移，外部不需要释放 */
uint32_t DLI_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb);
/* 超时阻塞接口，当freeCb不会为空时，成功或者失败内部都会释放arg内存 */
uint32_t DLI_BlockPostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout);
uint32_t DLI_PostOtherThread(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb);
uint32_t DLI_PostOtherBlockedThread(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout);
#ifdef __cplusplus
}
#endif
#endif