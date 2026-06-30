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

#ifndef SLE_INTERFACE_PROFILE_MIC_H
#define SLE_INTERFACE_PROFILE_MIC_H

#include "SleInterfaceProfile.h"

namespace OHOS {
namespace Nearlink {
class MicObserver {
public:
    virtual ~MicObserver() = default;
    /**
     * ConnectionState Changed
     * @param  deviceAddress  sle address
     * @param  state          new state
     * @param  oldState       old state
     */
    virtual void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) {};
};

class MicStateObserver {
public:
    virtual ~MicStateObserver() = default;

    virtual void OnMicStateChanged(const RawAddress &device, uint8_t micState) {};
};

class ProfileMic : public SleInterfaceProfile {
public:
    /**
     * @brief  register observer
     *
     * @param  observer         function pointer
     */
    virtual void RegisterObserver(MicObserver &observer) = 0;

    /**
     * @brief  deregister observer
     *
     * @param  observer         function pointer
     */
    virtual void DeregisterObserver(MicObserver &observer) = 0;

    /**
     * @brief  register mic property state observer
     *
     * @param  observer         function pointer
     */
    virtual void RegisterMicStateObserver(MicStateObserver &observer) = 0;

    /**
     * @brief  deregister mic property state observer
     *
     * @param  observer         function pointer
     */
    virtual void DeregisterMicStateObserver(MicStateObserver &observer) = 0;
};
} // namespace Nearlink
} // namespace OHOS
#endif // SLE_INTERFACE_PROFILE_MIC_H