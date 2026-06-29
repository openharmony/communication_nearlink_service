/**
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

/****************************************************************************
 *
 * The init and deinit of icb link
 *
 ***************************************************************************/

#ifndef CM_ICB_INIT_H
#define CM_ICB_INIT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  同步链路初始化
 * @param  [in] 无
 * @return CM_SUCCESS: 成功
 */
uint32_t CM_ICBInit(void);

/**
 * @brief  同步链路去初始化
 * @param  [in] 无
 * @return CM_SUCCESS: 成功
 */
uint32_t CM_ICBDeinit(void);

/**
 * @brief  同步链路使能
 * @param  [in] 无
 * @return CM_SUCCESS: 成功
 */
uint32_t CM_ICBEnable(void);

/**
 * @brief  同步链路去使能
 * @param  [in] 无
 * @return CM_SUCCESS: 成功
 */
uint32_t CM_ICBDisable(void);

/**
 * @brief  同步链路是否已经初始化
 * @param  [in] 无
 * @return true: 已初始，false: 未初始化
 */
bool CM_ICBIsInited(void);

#ifdef __cplusplus
}
#endif

#endif