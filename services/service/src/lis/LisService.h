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
#ifndef LIS_SERVICE_H
#define LIS_SERVICE_H

#include <atomic>
#include "context.h"
#include "SleInterfaceProfile.h"
#include "message.h"
#include "ProfileServiceManager.h"

namespace OHOS {
namespace Nearlink {
class LisService : public SleInterfaceProfile, public utility::Context {
public:
    static LisService *GetService();
    explicit LisService();
    virtual ~LisService();
    utility::Context *GetContext() override;
    void Enable() override;
    void Disable() override;
    int Connect(const RawAddress &device) override { return 0; };
    int Disconnect(const RawAddress &device) override { return 0; };
    std::list<RawAddress> GetConnectDevices() override { return {}; };
    int GetConnectState() override { return 0; };

private:
    void StartUp();
    void ShutDown();
    void ProcessEvent(const utility::Message &event);
    void PostEvent(const utility::Message &event);
    // service status
    std::atomic_bool isStarted_ = ATOMIC_FLAG_INIT;
};
} // namespace Sle
} // namespace OHOS
#endif // LIS_SERVICE_H