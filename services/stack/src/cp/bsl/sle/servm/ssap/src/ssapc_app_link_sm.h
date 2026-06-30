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
#ifndef SSAPC_APP_LINK_SM_H
#define SSAPC_APP_LINK_SM_H

#include "ssapc_app.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 处理链路状态变化事件
 * @details 根据当前链路状态和事件类型，调用相应的处理函数。
 * @param [in] appId 应用程序的唯一标识符，不能为空
 * @param [in] event 链路状态变化的事件类型，必须是有效的事件类型
 * @param [in] reason 连接管理模块上报状态变化的原因，0表示空
 * @return void 无返回值
 * @note appId必须有效，否则函数可能无法正确处理
 *       event必须是有效的事件类型，否则函数不会执行任何操作
 */
void SsapClientLinkStateMachineCall(int32_t appId, NLSTK_SsapClientLinkChangeEvent_E event, uint8_t reason);

#ifdef __cplusplus
}
#endif
#endif