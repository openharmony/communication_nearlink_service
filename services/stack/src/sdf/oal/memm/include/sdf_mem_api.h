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
 * @file         sdf_mem_api.h
 * @brief        SDF memory api head file.
 */

#ifndef SDF_MEM_API_H
#define SDF_MEM_API_H

#include <stdint.h>
#include <stddef.h>
#include "sdf_errno_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDF_MEMM_ERROR_INVALID_PARAM     SDF_MAKE_BSL_MEMM_ERRNO(2)

/**
 * @brief     内存申请钩子
 * @param[IN] size 内存大小
 * @return    void *
 * @retval    内存地址
 * @remarks
 */
typedef void* (*SDF_MemAllocHook)(size_t size);

/**
 * @brief     内存释放钩子
 * @param[IN] ptr 内存地址
 * @return    void
 * @remarks
 */
typedef void (*SDF_MemFreeHook)(void *ptr);

/**
 * @brief 通用内存操作集
 */
typedef struct SDF_MemHooks {
    SDF_MemAllocHook mAlloc;
    SDF_MemFreeHook  mFree;
} SDF_MemHooks_S;

/**
 * @brief     内存申请释放注册函数
 * @param[IN] pHooks 内存申请释放钩子,不能为NULL
 * @return    uint32_t
 * @retval    成功(SDF_OK)
 * @retval    失败(SDF_ERR)
 * @remarks   该接口可选用,未注册将默认使用glibc;
 */
uint32_t SDF_MemHookReg(SDF_MemHooks_S *pHooks);

#ifdef __cplusplus
}
#endif

#endif /* SDF_MEM_API_H */