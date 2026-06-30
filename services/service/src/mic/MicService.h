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
#ifndef MIC_CLIENT_SERVICE_H
#define MIC_CLIENT_SERVICE_H

#include "nearlink_types.h"
#include "SleInterfaceProfileMic.h"

namespace OHOS {
namespace Nearlink {
class MicService : public ProfileMic, public utility::Context {
public:
    static MicService *GetService();
    explicit MicService();
    ~MicService() override;
    utility::Context *GetContext() override;

    void Enable() override;
    void Disable() override;

    int Connect(const RawAddress &device) override;
    int Disconnect(const RawAddress &device) override;
    int GetConnectState() override;
    std::list<RawAddress> GetConnectDevices() override;

    void RegisterObserver(MicObserver &serviceObserver) override;
    void DeregisterObserver(MicObserver &serviceObserver) override;

    void RegisterMicStateObserver(MicStateObserver &observer) override;
    void DeregisterMicStateObserver(MicStateObserver &observer) override;

    void UpdateMicState(const RawAddress &device, uint8_t micState);
    bool IsMicOpen(const RawAddress &device);
    void NotifyStateChanged(const RawAddress &device, SleConnectState curState, SleConnectState preState);

private:

    NEARLINK_DECLARE_IMPL();
};

}
}

#endif // MIC_CLIENT_SERVICE_H
