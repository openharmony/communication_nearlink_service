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
#ifndef BAS_SERVICE_H
#define BAS_SERVICE_H

#include <list>
#include <cstring>
#include "BasClientStackAdapter.h"
#include "BaseObserverList.h"
#include "ClassCreator.h"
#include "context.h"
#include "nearlink_timer.h"
#include "nearlink_types.h"
#include "ProfileServiceManager.h"
#include "SleInterfaceProfileBas.h"
#include "ThreadUtil.h"

namespace OHOS {
namespace Nearlink {
class BasService : public ProfileBas, public utility::Context {
public:
    static BasService *GetInstance();
    explicit BasService();
    ~BasService() override;

    utility::Context *GetContext() override;

    void Enable() override;
    void Disable() override;

    void RegisterObserver(BasObserver &basObserver) override;
    void DeregisterObserver(BasObserver &basObserver) override;
    void RegisterDeviceObserver(IDeviceBatteryCallback &deviceBatteryObserver) override;
    void DeregisterDeviceObserver(IDeviceBatteryCallback &deviceBatteryObserver) override;
    int Connect(const RawAddress &device) override;
    std::list<RawAddress> GetConnectDevices() override;
    int GetConnectState() override;
    int Disconnect(const RawAddress &device) override;
    void GetDeviceBatteryLevel(const RawAddress &device) override;
    void NotifyStateChanged(const RawAddress &device, SleConnectState curState, SleConnectState preState) override;
    void NotifyBatteryLevelEvent(const RawAddress &device, int8_t batteryLevel) override;
    void NotifyBatteryLevelChanged(const RawAddress &device, int8_t batteryLevel) override;
private:
    NEARLINK_DECLARE_IMPL();
};
} // namespace Nearlink
} // namespace OHOS
#endif // BAS_SERVICE_H