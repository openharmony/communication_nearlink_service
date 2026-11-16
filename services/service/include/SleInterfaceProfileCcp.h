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

#ifndef SLE_INTERFACE_PROFILE_CCP_H
#define SLE_INTERFACE_PROFILE_CCP_H

#include "SleInterfaceProfile.h"

namespace OHOS {
namespace Nearlink {
class ProfileCcp : public SleInterfaceProfile {
public:
    virtual void HandleVoipStart(const RawAddress &device) = 0;
    virtual void HandleVoipStop(const RawAddress &device) = 0;
    virtual void TryResumeCurrentCalls() = 0;

    /**
     * @brief 蜂窝通话状态变更直调入口（NearlinkCcpClientServer 经 IPC 转达；
     *        任意线程可调，内部投递至 ccp 线程串行执行）
     *
     * @param[in] callId id of the changed call, -1 means no specific call.
     * @param[in] numActive number of active calls, diagnostic only; the real state
     *            comes from the call list re-queried by CcpService.
     * @param[in] numHeld number of held calls, diagnostic only; the real state
     *            comes from the call list re-queried by CcpService.
     * @param[in] callState current phone call state.
     * @param[in] number call number.
     */
    virtual void PhoneStateChanged(int32_t callId, int32_t numActive, int32_t numHeld, int32_t callState,
        const std::string &number) = 0;

    /**
     * @brief 蜂窝单路通话明细变更直调入口（NearlinkCcpClientServer 经 IPC 转达；
     *        任意线程可调，内部投递至 ccp 线程串行执行）
     *        仅蜂窝路（callType 非 TYPE_VOIP）的 DISCONNECTED 状态触发 callId 映射清理
     *
     * @param[in] callId id of the changed call, -1 means no specific call.
     * @param[in] callState state of the changed call.
     * @param[in] callType call type of the changed call, same value as Telephony::CallType,
     *            a VoIP call type is filtered out by this entry.
     */
    virtual void CallDetailsChanged(int32_t callId, int32_t callState, int32_t callType) = 0;
};
} // namespace Nearlink
} // namespace OHOS
#endif // SLE_INTERFACE_PROFILE_CCP_H
