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

#ifndef SLE_INTERFACE_PROFILE_ICCE_CLIENT_H
#define SLE_INTERFACE_PROFILE_ICCE_CLIENT_H

#include "SleInterfaceProfile.h"

namespace OHOS {
namespace Nearlink {
class IcceObserver {
public:
    virtual ~IcceObserver() = default;
    /**
     * ConnectionState Changed
     * @param  device  sle address
     * @param  curState       new state
     * @param  prevState       old state
     */
    virtual void OnConnectionStateChanged(const RawAddress &device, int curState, int prevState) {};
};

class ProfileIcce : public SleInterfaceProfile {
public:
    /**
     * @brief  register observer
     *
     * @param  observer         function pointer
     */
    virtual void RegisterObserver(IcceObserver &observer) = 0;
    /**
     * @brief  deregister observer
     *
     * @param  observer         function pointer
     */
    virtual void DeregisterObserver(IcceObserver &observer) = 0;
};

} // namespace Sle
} // namespace OHOS
#endif // SLE_INTERFACE_PROFILE_ICCE_CLIENT_H