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

#ifndef SLE_SERVER_STATE_CHECK_H
#define SLE_SERVER_STATE_CHECK_H

#include "SleInterfaceManager.h"
#include "nearlink_caller_identity.h"
#include "nearlink_permission_manager.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief 与 framework IsSleAvailableToCaller 等价：全开放行；半开仅 native 放行。
 *
 * 契约：本函数与 NearlinkHostServer::IsSleAvailableToCaller 弱符号实现、
 * identity 重载三处判定必须同步；任一处语义变更需同步修改其余两处。
 *
 * 弱符号边界：产品构建如重写 NearlinkHostServer::IsSleAvailableToCaller，
 * 本函数不跟随（当前发布构建不重写）。走弱符号的是
 * NearlinkHostServer::IsSleAvailableToCallerInner 族入口，它们把失败折算为
 * NL_ERR_SLE_OFF；本函数用于其余 server IPC 入口，直接返回 NL_ERR_SLE_OFF。
 */
inline bool CheckSleAvailableToCaller()
{
    SleStateID state = SleInterfaceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE);
    return state == SleStateID::STATE_TURN_ON ||
        (state == SleStateID::STATE_TURN_HALF && NearLinkPermissionManager::IsNativeCaller());
}

/**
 * @brief 复用入口采集的调用方身份，半开态下 IsNative 只查询一次。
 *
 * 契约：与无参重载、framework 弱符号实现保持同一判定。
 */
inline bool CheckSleAvailableToCaller(CallerIdentity &caller)
{
    SleStateID state = SleInterfaceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE);
    return state == SleStateID::STATE_TURN_ON ||
        (state == SleStateID::STATE_TURN_HALF && caller.IsNative());
}

/**
 * @brief 与 framework IS_SLE_ENABLED 等价：仅 STATE_TURN_ON 放行。
 *
 * 契约：与 NearlinkHostServer::IsSleEnabled 弱符号实现保持同一判定。
 * 产品构建如重写该弱符号，本函数不跟随（当前发布构建不重写）；
 * 走弱符号的 NearlinkHostServer::IsSleEnabledInner 族入口失败折算为
 * NL_ERR_SLE_OFF，与本函数返回值一致。
 */
inline bool CheckSleEnabled()
{
    return SleInterfaceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE) ==
        SleStateID::STATE_TURN_ON;
}

} // namespace Nearlink
} // namespace OHOS

#endif // SLE_SERVER_STATE_CHECK_H
