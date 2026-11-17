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
#ifndef CCP_SYSTEM_INTERFACE_H
#define CCP_SYSTEM_INTERFACE_H

#include <cstdint>

#include "system_ability_definition.h"
#include "system_ability_status_change_stub.h"
#include "call_manager_info.h"
#include "nearlink_errorcode.h"
#include "SleServiceFfrtLog.h"

namespace OHOS {
namespace Nearlink {
class CcpSystemInterface {
public:
    static CcpSystemInterface &GetInstance();
    void Start();
    void Stop();

    void InitNearlinkCallClient();
    void DeInitNearlinkCallClient();

    bool CheckNowHasDeviceConnected();

    /* Check if there is an incoming call. */
    bool IsRinging(Telephony::TelCallState state) const;

    NlErrCode AnswerCall();
    NlErrCode HangUpCall();
    NlErrCode RejectCall();
    std::vector<Telephony::CallAttributeInfo> GetCurrentCallList();

private:
    CcpSystemInterface() = default;
    ~CcpSystemInterface() = default;

    class SystemAbilityStatusChange : public SystemAbilityStatusChangeStub {
    public:
        void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
        void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;
    };

    sptr<SystemAbilityStatusChange> statusChangeListener_{nullptr};

    static constexpr int32_t TOTAL_SLOT_ID = -1;
    /* -1表示获取所有卡的通话信息 */
    int32_t slotId_ = TOTAL_SLOT_ID;
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // CCP_SYSTEM_INTERFACE_H
