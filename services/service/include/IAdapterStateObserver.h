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
#ifndef I_ADAPTER_STATE_OBSERVER_H
#define I_ADAPTER_STATE_OBSERVER_H

#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {

struct SwitchCallerInfo {
    SwitchCallerInfo() : fullTokenId(0), callerUid(-1)
    {}
    SwitchCallerInfo(uint64_t fullToken, int32_t uid)
        : fullTokenId(fullToken), callerUid(uid)
    {}

    uint64_t fullTokenId {0};
    int32_t callerUid {-1};

    bool operator==(const SwitchCallerInfo& other) const {
        return (fullTokenId == other.fullTokenId) &&
               (callerUid == other.callerUid);
    }
};

/**
 * @brief Represents adapter state change observer during enable/disable.
 */
class IAdapterStateObserver {
public:
    /**
     * @brief A destructor used to delete the <b>IAdapterStateObserver</b> instance.
     */
    virtual ~IAdapterStateObserver() = default;

    /**
     * @brief IAdapterStateObserver state change function.
     *
     * @param transport Transport type when state change.
     * @param state Change to the new state.
     */
    virtual void OnStateChange(const SleTransport transport, const SleStateID state) {}

    /**
     * @brief IAdapterStateObserver disable response function.
     *
     * @param isHalfDisable Whether nearlink should be disabled to half. True means half, false means off
     * @param callerInfo Information of caller who called DisabledNl, indicates which observer should be responded to.
     */
    virtual void OnDisableResponse(bool isHalfDisable, SwitchCallerInfo callerInfo) {};
};

}  // namespace Nearlink
}  // namespace OHOS

#endif  // I_ADAPTER_STATE_OBSERVER_H