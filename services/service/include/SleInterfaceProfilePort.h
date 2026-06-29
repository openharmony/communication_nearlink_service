/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * ProtProfiletributed under the License is ProtProfiletributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SLE_INTERFACE_PROFILE_PORT_H
#define SLE_INTERFACE_PROFILE_PORT_H

#include "SleInterfaceProfile.h"
#include "sle_uuid.h"

namespace OHOS {
namespace Nearlink {
class PortObserver {
public:
    virtual ~PortObserver() = default;
    /**
     * ConnectionState Changed
     * @param  deviceAddress  sle address
     * @param  state          new state
     * @param  oldState       old state
     */
    virtual void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) {};
};

class ProfilePort : public SleInterfaceProfile {
public:
    /**
     * @brief  register observer
     *
     * @param  observer         function pointer
     */
    virtual void RegisterObserver(PortObserver &observer) = 0;
    /**
     * @brief  deregister observer
     *
     * @param  observer         function pointer
     */
    virtual void DeregisterObserver(PortObserver &observer) = 0;
    virtual uint16_t GetRemotePortByUuid(const RawAddress &device, const Uuid::UUID128Bit& uuid) = 0;
    virtual int AddPortByUuid(const Uuid::UUID128Bit& uuid, uint16_t portId) = 0;
    virtual int DeletePortByUuid(const Uuid::UUID128Bit& uuid, uint16_t portId) = 0;
};

} // namespace Sle
} // namespace OHOS
#endif // SLE_INTERFACE_PROFILE_PORT_H