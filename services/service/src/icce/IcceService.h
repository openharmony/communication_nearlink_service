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
#ifndef ICCE_PROFILE_H
#define ICCE_PROFILE_H

#include <list>
#include "context.h"
#include "nearlink_types.h"
#include "raw_address.h"
#include "SleInterfaceProfileIcce.h"

namespace OHOS {
namespace Nearlink {

class IcceService : public ProfileIcce, public utility::Context {
public:
    static IcceService* GetService();
    explicit IcceService();
    virtual ~IcceService();

    utility::Context *GetContext() override;
    // server
    void Enable() override;
    void Disable() override;

    // client
    int Connect(const RawAddress &device) override;
    int Disconnect(const RawAddress &device) override;
    int32_t GetPort(const RawAddress &device);

    void RegisterObserver(IcceObserver &icceObserver) override;
    void DeregisterObserver(IcceObserver &icceObserver) override;
    std::list<RawAddress> GetConnectDevices() override { return {}; };
    int GetConnectState() override { return 0; };

private:
    void EnableTask();
    void DisableTask();
    void ConnectTask(const RawAddress &device);
    void DisconnectTask(const RawAddress &device);
    int32_t GetPortTask(const RawAddress &device);
    uint8_t GetConnectionsDeviceNum();

    NEARLINK_DECLARE_IMPL();
};

} // namespace Nearlink
} // namespace OHOS
#endif // ICCE_PROFILE_H