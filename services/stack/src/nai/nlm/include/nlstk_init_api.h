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
#ifndef NLSTK_INIT_API_H
#define NLSTK_INIT_API_H

#include <stdint.h>
#include <stdbool.h>
#include "nlstk_public_define.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 协议栈初始化函数
 *
 * 不可重入函数，仅在单线程环境下使用
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_InitStack(void);

/**
 * @brief 协议栈去初始化函数
 */
void NLSTK_DeinitStack(void);

/**
 * @brief 查询协议栈是否初始化
 *
 * @return bool
 * - true: 协议栈已经初始化
 * - false: 协议栈未初始化
 */
bool NLSTK_IsStackInited(void);

/**
 * @brief 协议栈使能函数
 *
 * 不可重入函数，仅在单线程环境下使用
 *
 * @return NLSTK_Errorcode_E
 * - NLSTK_ERRCODE_SUCCESS: 成功
 * - 其他值: 错误码，查看NLSTK_Errorcode_E
 */
NLSTK_Errcode_E NLSTK_EnableStack(void);

/**
 * @brief 协议栈去使能函数
 */
void NLSTK_DisableStack(void);

/**
 * @brief 查询协议栈是否使能
 *
 * @return bool
 * - true: 协议栈已经使能
 * - false: 协议栈未使能
 */
bool NLSTK_IsStackEnabled(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
