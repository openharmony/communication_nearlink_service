/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef PORT_SERVICE_H
#define PORT_SERVICE_H

#include <list>
#include <mutex>
#include <vector>
#include <memory.h>
#include <cmath>
#include <cstring>
#include <mutex>
#include "SleInterfaceProfilePort.h"
#include "SleInterfaceAdapter.h"
#include "nearlink_def.h"
#include "nearlink_types.h"
#include "nearlink_safe_map.h"
#include "BaseObserverList.h"
#include "ProfileServiceManager.h"
#include "context.h"

namespace OHOS {
namespace Nearlink {

class PortServiceConnParam {
public:
    explicit PortServiceConnParam(uint8_t frameType) : frameType_(frameType) {}
    uint8_t GetFrameType(void) const
    {
        return frameType_;
    }
private:
    uint8_t frameType_;
};

class PortService : public ProfilePort, public utility::Context {
public:
    static PortService *GetPortService();
    explicit PortService();
    virtual ~PortService();
    utility::Context *GetContext() override;
    void Enable() override;
    void Disable() override;
    void RegisterObserver(PortObserver &portObserver) override;
    void DeregisterObserver(PortObserver &portObserver) override;
    int Connect(const RawAddress &device) override;
    int ConnectWithParam(const RawAddress &device, const PortServiceConnParam &param);
    int Disconnect(const RawAddress &device) override;
    std::list<RawAddress> GetConnectDevices() override;
    int GetConnectState() override;
    int GetConnectState(const RawAddress &device);
    void NotifyStateChanged(const RawAddress &device, SleConnectState state, SleConnectState preState);
    int AddPortByUuid(const Uuid::UUID128Bit& uuid, uint16_t portId) override;
    int DeletePortByUuid(const Uuid::UUID128Bit& uuid, uint16_t portId) override;
    uint16_t GetRemotePortByUuid(const RawAddress &device, const Uuid::UUID128Bit& uuid) override;

private:
    NEARLINK_DECLARE_IMPL();
};
} // namespace Nearlink
} // namespace OHOS
#endif // PORT_SERVICE_H