/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef NEARLINK_PHONE_STATE_H
#define NEARLINK_PHONE_STATE_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace Nearlink {
class NearlinkCallPhoneState {
public:
    NearlinkCallPhoneState() = default;
    ~NearlinkCallPhoneState() = default;

    int32_t GetActiveNum() const
    {
        return activeNum_;
    }
    void SetActiveNum(int32_t activeNum)
    {
        activeNum_ = activeNum;
    }
    int32_t GetHeldNum() const
    {
        return heldNum_;
    }
    void SetHeldNum(int32_t heldNum)
    {
        heldNum_ = heldNum;
    }
    int32_t GetCallState() const
    {
        return callState_;
    }
    void SetCallState(int32_t callState)
    {
        callState_ = callState;
    }
    std::string GetNumber() const
    {
        return number_;
    }
    void SetNumber(const std::string &number)
    {
        number_ = number;
    }
    int32_t GetCallType() const
    {
        return callType_;
    }
    void SetCallType(int32_t callType)
    {
        callType_ = callType;
    }
    std::string GetName() const
    {
        return name_;
    }
    void SetName(const std::string &name)
    {
        name_ = name;
    }

private:
    int32_t activeNum_{0};
    int32_t heldNum_{0};
    int32_t callState_{0};
    std::string number_{""};
    int32_t callType_{0};
    std::string name_{""};
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_PHONE_STATE_H
