/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
 * @file HADM_CM_Listener.h
 * @brief HADM模块的CM链路状态监听模块
 * @details 该模块负责注册和处理CM链路状态变化的回调函数，包括链路连接状态和远程特性报告的处理
 */

#ifndef HADM_LISTENER_CM_H
#define HADM_LISTENER_CM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 注册CM链路状态监听回调函数
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 注册CM链路状态变化的回调函数，包括链路连接状态和远程特性报告的处理
 */
uint32_t HadmRegListenCmLink(void);

/**
 * @brief 反注册CM链路状态监听回调函数
 * @return uint32_t 状态码，0表示成功，非0表示失败
 * @details 反注册CM链路状态变化的回调函数
 */
void HadmUnregListenCmLink(void);

#ifdef __cplusplus
}
#endif
#endif /* HADM_LISTENER_CM_H */