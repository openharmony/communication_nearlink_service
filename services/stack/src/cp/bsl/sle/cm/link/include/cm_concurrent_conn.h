/****************************************************************************
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
****************************************************************************/
#ifndef CM_CONCURRENT_CONN_H
#define CM_CONCURRENT_CONN_H

#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 并发连接管理模块初始化
 */
uint32_t CM_ConcurrentConnInit(void);

/**
 * @brief 并发连接管理模块去初始化
 */
void CM_ConcurrentConnDeInit(void);

/**
 * @brief 对于主动发起的操作完成后，通知待连接设备列表完成结果
 * @param [in] addr 完成连接结果的地址 1. 实际完成的地址 2. 连接超时的地址，地址内容为全0格式: 00:00:**:**:**:00
 * @param [in] connectResult 参见参考链路连接状态CM_ConnectLinkState_E定义
 */
void CM_ConcurrentConnDoingComplete(const SLE_Addr_S *addr, uint8_t connectResult);

/**
 * @brief 通知Link Collab结果
 */
void CM_ConcurrentConnNotifyLinkCollabResult(void);

#ifdef __cplusplus
}
#endif

#endif