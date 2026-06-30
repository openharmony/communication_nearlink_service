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
#ifndef CCP_STACK_ADAPTER_H
#define CCP_STACK_ADAPTER_H

#include <cstdint>
#include <memory>

#include "call_manager_info.h"
#include "CcpDefines.h"
#include "raw_address.h"
#include "nlstk_ccp_ccs_define.h"

namespace OHOS {
namespace Nearlink {
using Telephony::CallAttributeInfo;
using Telephony::TelCallState;

class CcpStackAdapter {
public:
    explicit CcpStackAdapter();
    ~CcpStackAdapter();

    void CreateCcsInstance(uint8_t instanceId);
    void DeleteCcsInstance(int32_t instanceId);
    void UpdateCallStateInfo(int32_t instanceId, std::shared_ptr<TotalCallStateProp> prop);
    void UpdateCallInOutInfo(int32_t instanceId, std::shared_ptr<CallInOutInfoProp> prop);
    void UpdateCallTerminateInfo(int32_t instanceId, std::shared_ptr<CallTerminateInfoProp> prop);
    void NotifyCallControlSuccess(uint32_t requestId, int32_t instanceId);
    void NotifyCallControlFail(uint32_t requestId, int32_t instanceId);
    void AuthorizeResult(int32_t instanceId, uint32_t requestId, int32_t prop);

private:
    bool NewCallStateInfo(NLSTK_CcpCallStatues_S *callState, size_t count);
    void FreeCallStateInfo(NLSTK_CcpCallStatues_S *callState, size_t count);
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // CCP_STACK_ADAPTER_H
