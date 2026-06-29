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
#ifndef HID_HOST_SERVICE_H
#define HID_HOST_SERVICE_H

#include <list>
#include <vector>
#include <memory.h>
#include <cmath>
#include <cstring>
#include "SleInterfaceProfileHidHost.h"
#include "nearlink_def.h"
#include "nearlink_safe_map.h"
#include "BaseObserverList.h"
#include "ProfileServiceManager.h"
#include "context.h"
#include "nearlink_types.h"
#include "HidHostDefines.h"

namespace OHOS {
namespace Nearlink {
class HidHostService : public ProfileHidHost, public utility::Context {
public:
    static HidHostService *GetService();
    explicit HidHostService();
    virtual ~HidHostService();
    utility::Context *GetContext() override;
    void Enable(void) override;
    void Disable(void) override;
    int Connect(const RawAddress &device) override;
    std::list<RawAddress> GetConnectDevices() override;
    int GetConnectState(void) override;
    int Disconnect(const RawAddress &device) override;
    std::list<RawAddress> GetDevicesByStates(std::vector<int> states) override;
    int GetDeviceState(const RawAddress &device) override;
    void RegisterObserver(HidHostObserver &hidHostObserver) override;
    void DeregisterObserver(HidHostObserver &hidHostObserver) override;
    void NotifyStateChanged(const RawAddress &device, int state, int preState, int32_t ret);
    void ShutDownDone(bool isAllDisconnected);
    void ConnectHidInterface(const RawAddress &device);
    int SendData(const HidReportInfo &reportInfo);

    int HidHostVCUnplug(std::string device, uint8_t id, uint16_t size, uint8_t type) override;
    int HidHostSendReport(std::string device, uint8_t type, uint16_t size, const uint8_t* report) override;
    int HidHostSendReport(std::string device, uint8_t type, uint16_t size, std::string &report) override;
    int HidHostSetReport(std::string device, uint8_t type, uint16_t size, const uint8_t* report) override;
    int HidHostGetReport(std::string device, uint8_t id, uint16_t size, uint8_t type) override;
    int GetHidDeviceInfo(const RawAddress &device, int infoType) override;

    void ReceiveHandShake(const RawAddress &addr, uint16_t err);
    int ReceiveControlData(const HidReportInfo &reportInfo);

private:
    void StartUp();
    void ShutDown();
    uint8_t GetConnectionsDeviceNum();
    void ProcessConnectedEvent(const RawAddress &device);
    void ProcessDisconnectedEvent(const RawAddress &device);
    void ProcessConnectDftEvent(const RawAddress &device, int state, int preState, int32_t ret);
    NEARLINK_DECLARE_IMPL();
};
} // namespace Sle
} // namespace OHOS
#endif // HID_HOST_SERVICE_H