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

#ifndef NEARLINK_REGISTER_INFO_H
#define NEARLINK_REGISTER_INFO_H

#include <cstdint>
#include <functional>
#include <string>

#include "iremote_broker.h"

namespace OHOS {
namespace Nearlink {
constexpr int32_t INVALID_PROFILE_ID = -1;

class NearlinkRegisterInfo {
public:
    explicit NearlinkRegisterInfo(std::string name) : profileName_(name)
    {}
    ~NearlinkRegisterInfo() = default;

    std::string profileName_ = "";
    std::function<void(sptr<IRemoteObject>)> serviceStartedFunc_;
    std::function<void(void)> serviceStoppedFunc_;
    std::function<void(sptr<IRemoteObject>)> stateOnFunc_;
    std::function<void(sptr<IRemoteObject>)> stateOffFunc_;
};
} // namespace Nearlink
} // namespace OHOS
#endif // NEARLINK_REGISTER_INFO_H
