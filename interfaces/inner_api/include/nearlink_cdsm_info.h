/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_CDSM_INFO_H
#define NEARLINK_CDSM_INFO_H

#include <mutex>
#include <cstdint>
#include <cstring>
#include "nearlink_def.h"

namespace OHOS {
namespace Nearlink {
class NearlinkCdsMemberInfo {
public:
    static constexpr uint8_t CDSM_STATE_DISCONNECT = 0;
    static constexpr uint8_t CDSM_STATE_CONNECTED = 1;

    NearlinkCdsMemberInfo() = default;

    ~NearlinkCdsMemberInfo() = default;

    explicit NearlinkCdsMemberInfo(const std::string &address) : addr_(address) { }

    NearlinkCdsMemberInfo(const NearlinkCdsMemberInfo &other) : addr_(other.addr_), state_(other.state_) { }

    NearlinkCdsMemberInfo &operator=(const NearlinkCdsMemberInfo &other)
    {
        if (this != &other) {
            addr_ = other.addr_;
            state_ = other.state_;
        }
        return *this;
    }

    void SetDeviceAddr(const std::string &addr)
    {
        addr_ = addr;
    }

    std::string GetDeviceAddr() const
    {
        return addr_;
    }

    void SetState(const uint8_t newState)
    {
        state_ = newState;
    }

    uint8_t GetState() const
    {
        return state_;
    }

protected:
    std::string addr_ = "";
    uint8_t state_ = CDSM_STATE_DISCONNECT;
};

class NearlinkCdsInfo {
public:
    NearlinkCdsInfo() = default;

    ~NearlinkCdsInfo() = default;

    NearlinkCdsInfo(const NearlinkCdsInfo &other)
    {
        members_ = other.members_;
    }

    NearlinkCdsInfo &operator=(const NearlinkCdsInfo &other)
    {
        if (this != &other) {
            members_ = other.members_;
        }
        return *this;
    }

    std::vector<NearlinkCdsMemberInfo> GetCdsMemberList() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return members_;
    }

    void AddCdsMemberInfo(const NearlinkCdsMemberInfo &memberInfo)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        members_.push_back(memberInfo);
    }

protected:
    std::vector<NearlinkCdsMemberInfo> members_;

private:
    mutable std::mutex mutex_{};
};
} // namespace Nearlink
}  // namespace OHOS
#endif  /// NEARLINK_CDSM_INFO_H
