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

#ifndef SLE_CALLER_STATE_CHECK_H
#define SLE_CALLER_STATE_CHECK_H

#include "SleInterfaceManager.h"
#include "nearlink_caller_identity.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief 复用入口采集的调用方身份，避免半开态下重复读取 IPCSkeleton/AccessTokenKit。
 *        与 CheckSleAvailableToCaller() 判定完全等价。
 */
inline NlErrCode CheckSleAvailableToCaller(const CallerIdentity &caller)
{
    SleStateID state = SleInterfaceManager::GetInstance()->GetState(SleTransport::ADAPTER_SLE);
    if (state == SleStateID::STATE_TURN_ON ||
        (state == SleStateID::STATE_TURN_HALF && caller.IsNative())) {
        return NL_NO_ERROR;
    }
    return NL_ERR_SLE_OFF;
}

} // namespace Nearlink
} // namespace OHOS

#endif // SLE_CALLER_STATE_CHECK_H
