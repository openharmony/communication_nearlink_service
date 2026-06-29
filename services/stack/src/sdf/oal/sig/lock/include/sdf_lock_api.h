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
 * @file sdf_lock_api.h
 * @brief 锁模块对外接口
 * @version 1.0
 * @date 2024-8-7
 */

#ifndef SDF_LOCK_API_H
#define SDF_LOCK_API_H

#include <stddef.h>
#include <stdint.h>

#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SDF_MUTEX_NORMAL = 0,
    SDF_MUTEX_ERRORCHECK = 1,
    SDF_MUTEX_RECURSIVE = 2,
} SDF_MutexType_E;

/**
 * @brief           互斥锁初始化钩子
 * @param[IN/OUT]   pLock 锁指针
 * @param[IN]       pAttr 锁的属性
 * @return          int
 * @retval          成功返回0,失败返回其他
 * @remarks         NA
 */
typedef uint32_t (*SDF_MutexInitHook)(SDF_MutexLock pLock, const SDF_MutexLockAttr pAttr);

/**
 * @brief           互斥锁去初始化钩子
 * @param[IN/OUT]   pLock 锁指针
 * @return          void
 * @retval          NA
 * @remarks         NA
 */
typedef void (*SDF_MutexDeinitHook)(SDF_MutexLock pLock);

/**
 * @brief           互斥锁加锁钩子
 * @param[IN/OUT]   pLock 锁指针
 * @return          int
 * @retval          成功返回0,失败返回其他
 * @remarks         NA
 */
typedef uint32_t (*SDF_MutexLockHook)(SDF_MutexLock pLock);

/**
 * @brief           互斥锁解锁钩子
 * @param[IN/OUT]   pLock 锁指针
 * @return          void
 * @retval          NA
 * @remarks         NA
 */
typedef void (*SDF_MutexUnlockHook)(SDF_MutexLock pLock);

/**
 * @brief           锁属性初始化钩子
 * @param[IN/OUT]   attr 属性指针
 * @return          void
 * @retval          NA
 * @remarks         NA
 */
typedef uint32_t (*SDF_MutexAttrInitHook)(SDF_MutexLockAttr pAttr);

/**
 * @brief           锁属性设置钩子
 * @param[IN/OUT]   attr 属性指针
 * @return          void
 * @retval          NA
 * @remarks         NA
 */
typedef uint32_t (*SDF_MutexAttrSetHook)(SDF_MutexLockAttr pAttr, SDF_MutexType_E pType);

typedef struct SDF_MutexLockHooks {
    SDF_MutexInitHook initHook;
    SDF_MutexDeinitHook deinitHook;
    SDF_MutexLockHook lockHook;
    SDF_MutexLockHook tryLockHook;
    SDF_MutexUnlockHook unlockHook;
    SDF_MutexAttrInitHook attrInitHook;
    SDF_MutexAttrSetHook attrSetHook;

    size_t lockSize; // 结构体大小
} SDF_MutexLockHooks_S;

/**
 * @brief           互斥锁方法注册接口
 * @param[IN/OUT]   pHooks 读写锁方法集
 * @return          int
 * @retval          成功返回0,失败返回其他
 * @remarks         NA
 */
uint32_t SDF_MutexReg(const SDF_MutexLockHooks_S *pHooks);

#ifdef __cplusplus
}
#endif

#endif /* SDF_LOCK_API_H */
