/****************************************************************************
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
****************************************************************************/

/****************************************************************************
 *
 * this file contains QOSM module definitions and related apis.
 *
 ***************************************************************************/

#ifndef QOSM_H
#define QOSM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化QOS管理模块
 * @return 返回0表示初始化成功，否则返回错误代码
 */
uint32_t QOSM_Init(void);

/**
 * @brief 反初始化QOS管理模块
 */
void QOSM_DeInit(void);

/**
 * @brief  使能QOSM模块
 * @return SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_Enable(void);

/**
 * @brief  关闭QOSM模块
 * @return void
 */
void QOSM_Disable(void);

#ifdef __cplusplus
}
#endif
#endif