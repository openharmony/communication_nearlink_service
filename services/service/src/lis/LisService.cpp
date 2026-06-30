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
#include "LisService.h"
#include "ClassCreator.h"
#include "SleServiceFfrtLog.h"
#include "SleUtils.h"
#include "LisDefines.h"
#include "LisServer.h"
#include "ThreadUtil.h"

namespace OHOS {
namespace Nearlink {
LisService::LisService() : utility::Context(PROFILE_NAME_LIS, "1.0.0")
{
    HILOGD("[LisService]%{public}s:%{public}s Create", PROFILE_NAME_LIS.c_str(), Name().c_str());
}

LisService::~LisService()
{
    HILOGD("[LisService]%{public}s:%{public}s Destroy", PROFILE_NAME_LIS.c_str(), Name().c_str());
}

utility::Context *LisService::GetContext()
{
    return this;
}

LisService *LisService::GetService()
{
    return static_cast<LisService *>(SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_LIS));
}

void LisService::Enable()
{
    HILOGI("[LisService] Enter");

    utility::Message event(LIS_SERVICE_STARTUP_EVT);
    PostEvent(event);
}

 void LisService::Disable()
{
    HILOGI("[LisService] Enter");

    utility::Message event(LIS_SERVICE_SHUTDOWN_EVT);
    PostEvent(event);
}

void LisService::StartUp()
{
    HILOGI("[LisService]:==========<start>==========");
    if (isStarted_.load()) {
        GetContext()->OnEnable(PROFILE_NAME_LIS, true);
        HILOGW("[LisService]:LisService has already been started before.");
        return;
    }
    GetContext()->OnEnable(PROFILE_NAME_LIS, true);
    isStarted_.store(true);
    LisServer::GetInstance().RegisterLisServerApplication();
    HILOGI("[LisService]:LisService started");
}
 
 void LisService::ShutDown()
{
    HILOGI("[LisService]:==========<shutdown>==========");
    if (!isStarted_.load()) {
        GetContext()->OnDisable(PROFILE_NAME_LIS, true);
        HILOGW("[LisService]:LisService has already been shutdown before.");
        return;
    }

    GetContext()->OnDisable(PROFILE_NAME_LIS, true);
    isStarted_.store(false);
    LisServer::GetInstance().DeregisterLisServerApplication();
    HILOGI("[LisService]:LisService shutdown");
}

void LisService::PostEvent(const utility::Message &event)
{
    DoInLisThread([this, event]() { this->ProcessEvent(event); });
}

void LisService::ProcessEvent(const utility::Message &event)
{
    HILOGI("[LisService] ProcessEvent:event[%{public}d]", event.whatM);
    switch (event.whatM) {
        case LIS_SERVICE_STARTUP_EVT:
            StartUp();
            break;
        case LIS_SERVICE_SHUTDOWN_EVT:
            ShutDown();
            break;
        default:
            break;
    }
}

REGISTER_CLASS_CREATOR(LisService);

} // namespace Sle
} // namespace OHOS