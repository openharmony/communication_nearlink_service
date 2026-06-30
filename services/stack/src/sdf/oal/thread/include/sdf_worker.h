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
 * @file         sdf_worker.h
 * @brief        SDF Worker head file.
 */

#ifndef SDF_WORKER_H
#define SDF_WORKER_H

#include <stdint.h>
#include <stddef.h>

#include "sdf_dlist.h"
#include "sdf_mem.h"
#include "sdf_mutex.h"
#include "sdf_sem.h"
#include "sdf_struct.h"
#include "sdf_errno_base.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SDF_WORKER_ERROR_ADD_WORK SDF_MAKE_BSL_WORKER_ERRNO(1)

/**
 * @brief create worker
 *
 * @return worker
 */
SDF_Worker_S *SDF_CreateWorker(void);

/**
 * @brief destroy worker
 *
 * @param worker [IN] worker to be destroyed
 */
void SDF_DestroyWorker(SDF_Worker_S *worker);

/**
 * @brief add work
 *
 * @param worker [IN] worker
 * @param cb     [IN] work callback
 * @param arg    [IN] callback arguments
 * @param freeCb   [IN] work freeCb
 * @retval #SDF_OK
 * @retval #SDF_WORKER_ERROR_ADD_WORK
 */
uint32_t SDF_AddWork(SDF_Worker_S *worker, SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb);

/**
 * @brief run worker
 *
 * @param worker [IN] worker
 */
void SDF_WorkerRunOnce(SDF_Worker_S *worker);

#ifdef __cplusplus
}
#endif

#endif