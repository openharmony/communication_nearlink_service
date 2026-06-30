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
 * @file sdf_sem_api.h
 * @brief 信号量模块对外接口
 * @version 1.0
 * @date 2024-8-7
 */

#ifndef SDF_SEM_API_H
#define SDF_SEM_API_H

#include <stdint.h>
#include <stddef.h>
#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief       信号量方法注册接口
 * @param[IN]   pHooks 信号量方法集
 * @return      int
 * @retval      成功返回0,失败返回其他
 * @remarks     NA
 */
uint32_t SDF_SemReg(const SDF_SemHooks_S *pHooks);

#ifdef __cplusplus
}
#endif

#endif /* SDF_SEM_API_H */
