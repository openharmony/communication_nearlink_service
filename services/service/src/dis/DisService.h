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
#ifndef DIS_SERVICE_H
#define DIS_SERVICE_H

#include <list>
#include <mutex>
#include <vector>
#include <memory.h>
#include <cmath>
#include <cstring>
#include <mutex>
#include "SleInterfaceProfileDis.h"
#include "SleInterfaceAdapter.h"
#include "nearlink_def.h"
#include "BaseObserverList.h"
#include "ProfileServiceManager.h"
#include "context.h"
#include "nearlink_types.h"

namespace OHOS {
namespace Nearlink {
class DisService : public ProfileDis, public utility::Context {
public:
    static DisService *GetDisService();
    explicit DisService();
    virtual ~DisService();
    utility::Context *GetContext() override;
    void Enable() override;
    void Disable() override;
    void RegisterObserver(DisObserver &hidHostObserver) override;
    void DeregisterObserver(DisObserver &hidHostObserver) override;

    int Connect(const RawAddress &device) override;
    std::list<RawAddress> GetConnectDevices() override;
    int GetConnectState() override;
    int Disconnect(const RawAddress &device) override;
    int GetDeviceVendorId(const RawAddress &device) override;
    int GetDeviceProductId(const RawAddress &device) override;
    int GetDeviceVersion(const RawAddress &device) override;
    void NotifyStateChanged(const RawAddress &device, SleConnectState curState, SleConnectState preState) override;
    void GetDisServiceData(std::string &serviceData);
    int GetDisServiceAppearance();
    DeviceInformation GetDeviceInformation(const RawAddress &device) const override;

private:
    void LoadDeviceInfo();
    void SaveAppearanceInfo(const RawAddress &device);
    void SaveNameInfo(const RawAddress &device);
    struct impl;
    std::shared_ptr<impl> pimpl = nullptr;
};
} // namespace Sle
} // namespace OHOS
#endif // DIS_SERVICE_H