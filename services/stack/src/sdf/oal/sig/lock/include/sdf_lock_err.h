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
 * @file         sdf_lock_err.h
 * @brief        锁模块错误码定义
*/

#ifndef SDF_LOCK_ERR_H
#define SDF_LOCK_ERR_H

#include "sdf_errno_base.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define SDF_LOCK_ERROR_MUTEX_FAIL          SDF_MAKE_BSL_LOCK_ERRNO(1)
#define SDF_LOCK_ERROR_INIT_FAIL           SDF_MAKE_BSL_LOCK_ERRNO(2)
#define SDF_LOCK_ERROR_INVALID_PARAM       SDF_MAKE_BSL_LOCK_ERRNO(3)
#define SDF_LOCK_ERROR_SEM_WAIT_FAIL       SDF_MAKE_BSL_LOCK_ERRNO(4)
#define SDF_LOCK_ERROR_TIME_FAIL           SDF_MAKE_BSL_LOCK_ERRNO(5)
#define SDF_LOCK_ERROR_UNLOCK_FAIL         SDF_MAKE_BSL_LOCK_ERRNO(6)
#define SDF_LOCK_ERROR_SEM_POST_FAIL       SDF_MAKE_BSL_LOCK_ERRNO(7)
#define SDF_LOCK_ERROR_SEM_INIT_FAIL       SDF_MAKE_BSL_LOCK_ERRNO(8)

#ifdef __cplusplus
}
#endif

#endif // SDF_LOCK_ERR_H