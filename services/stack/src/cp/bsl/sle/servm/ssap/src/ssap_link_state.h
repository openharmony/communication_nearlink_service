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
#ifndef SSAP_LINK_STATE_H
#define SSAP_LINK_STATE_H

#include <stdbool.h>
#include "sdf_addr.h"
#include "nlstk_ssap_app_link.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 处理用户的链路连接请求
 * @details 根据实际链路状态决定是否发起建链操作
 * @param [in] addr 目标链路地址，不能为空
 * @param [in] connParam 建链参数，不能为空
 * @return NLSTK_SsapConnectLinkState_E 连接状态，可能的返回值包括：
 *         - SSAP_CONNECT_STATE_DISCONNECTED：建链失败或断开
 *         - SSAP_CONNECT_STATE_CONNECTING：正在建链
 *         - SSAP_CONNECT_STATE_CONNECTED：已连接
 *         - SSAP_CONNECT_STATE_DISCONNECTING：正在断开，调用者需要缓存请求并在断开后重试
 * @note addr 参数不能为空，否则直接返回 SSAP_CONNECT_STATE_DISCONNECTED
 */
NLSTK_SsapConnectLinkState_E SsapLinkHandleUserConnect(SLE_Addr_S *addr, const NLSTK_ConnParam_S *connParam);

/**
 * @brief 处理用户的链路断开请求
 * @details 根据实际链路状态决定是否发起断链操作
 * @param [in] addr 目标链路地址，不能为空
 * @return NLSTK_SsapConnectLinkState_E 断开状态，可能的返回值包括：
 *         - SSAP_CONNECT_STATE_DISCONNECTED：链路已断开或断开失败
 *         - SSAP_CONNECT_STATE_DISCONNECTING：链路正在断开过程中
 *         - SSAP_CONNECT_STATE_CONNECTING：链路正在连接过程中，调用者需要缓存请求并在断开后重试
 * @note addr 参数不能为空，否则直接返回 SSAP_CONNECT_STATE_DISCONNECTED
 */
NLSTK_SsapConnectLinkState_E SsapLinkHandleUserDisconnect(SLE_Addr_S *addr);

/**
 * @brief 从CM模块记录链路状态
 * @details 根据CM上报的状态更新链路状态管理结构，当状态为断开时释放资源
 * @param [in] addr 链路地址，不能为空
 * @param [in] state CM上报的链路状态
 * @return void 无返回值
 * @note addr参数不能为空，否则直接返回
 */
void SsapLinkHandleRecordLinkStateFromCm(SLE_Addr_S *addr, NLSTK_SsapConnectLinkState_E state);
bool SsapGetClientCleanUp(void);
bool SsapGetServerCleanUp(void);
void SsapResetServerCleanUp(void);
void SsapResetClientCleanUp(void);
void SsapRemoveLink(SLE_Addr_S *addr);
bool SsapIsAllLinkCleanUp(void);
void SsapRemoveAllLink(void);
void SsapLinkStateDeinit(void);

#ifdef __cplusplus
}
#endif
#endif