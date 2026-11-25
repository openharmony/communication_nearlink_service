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
 */
inline bool CheckSleAvailableToCaller()
{
    SleStateID state = SleInterfaceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE);
    return state == SleStateID::STATE_TURN_ON ||
        (state == SleStateID::STATE_TURN_HALF && NearLinkPermissionManager::IsNativeCaller());
}

/**
 * @brief 复用入口采集的调用方身份，半开态下 IsNative 只查询一次。
 */
inline bool CheckSleAvailableToCaller(CallerIdentity &caller)
{
    SleStateID state = SleInterfaceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE);
    return state == SleStateID::STATE_TURN_ON ||
        (state == SleStateID::STATE_TURN_HALF && caller.IsNative());
}

/**
 * @brief 与 framework IS_SLE_ENABLED 等价：仅 STATE_TURN_ON 放行。
 */
inline bool CheckSleEnabled()
{
    return SleInterfaceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE) ==
        SleStateID::STATE_TURN_ON;
}

} // namespace Nearlink
} // namespace OHOS

#endif // SLE_SERVER_STATE_CHECK_H
