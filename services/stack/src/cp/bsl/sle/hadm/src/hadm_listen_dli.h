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
/*
 * @file hadm_listen_dli.h
 * @brief HADM模块的DLI回调函数管理模块
 * @details 该模块负责注册和处理DLI事件的回调函数，包括远端参数读取、CS配置、CS使能、IQ数据报告和测量状态报告等功能
 */

#ifndef HADM_LISTEN_DLI_H
#define HADM_LISTEN_DLI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册DLI回调函数
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 注册DLI事件的回调函数，包括远端参数读取、CS配置、CS使能、IQ数据报告和测量状态报告等功能
 */
uint32_t HadmRegDliCbk(void);

/**
 * @brief 反注册DLI回调函数
 * @return void
 * @details 反注册DLI事件的回调函数
 */
void HadmUnRegDliCbk(void);

#ifdef __cplusplus
}
#endif
#endif /* HADM_LISTEN_DLI_H */