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
#ifndef NLSTK_SCHEDULE_H
#define NLSTK_SCHEDULE_H

#include <stdint.h>
#include "sdf_timer.h"
#include "sdf_worker.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动调度实例
 *
 * @return uint32_t
 * - NLSTK_OK: 成功
 * - NLSTK_ERR：失败
 */
uint32_t ScheduleEnable(void);

/**
 * @brief 停止调度实例
 */
void ScheduleDisable(void);

/**
 * @brief 向调度实例队列中添加任务，异步执行
 *
 * @param[in] cb 任务钩子
 * @param[in] arg 任务参数，所有权完全转移至调度实例，即使失败也由实例释放
 * @param[in] freeCb 任务参数释放钩子
 *
 * @return uint32_t
 * - NLSTK_OK: 成功
 * - NLSTK_ERR：失败
 */
uint32_t SchedulePostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb);

/**
 * @brief 向调度实例队列中添加任务，同步执行
 *
 * @param[in] cb 任务钩子
 * @param[in] arg 任务参数，所有权完全转移至调度实例，即使失败也由实例释放
 * @param[in] freeCb 任务参数释放钩子
 * @param[in] timeout 超时时间，毫秒
 *
 * @return uint32_t
 * - NLSTK_OK: 成功
 * - NLSTK_ERR：失败
 */
uint32_t SchedulePostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout);

/**
 * @brief 向调度实例队列中添加定时器
 *
 * @param[out] handle 定时器handle出参
 * @param[in] param 定时器参数
 *
 * @return uint32_t
 * - NLSTK_OK: 成功
 * - NLSTK_ERR：失败
 */
uint32_t ScheduleTimerAdd(int *handle, SDF_TimerParam *param);

/**
 * @brief 从调度实例队列中删除定时器
 *
 * @param[in] handle 定时器handle
 *
 */
void ScheduleTimerDel(int handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
