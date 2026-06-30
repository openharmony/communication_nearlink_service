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

#include "UnloadSa.h"
#include "log.h"
#include "system_ability_definition.h"
#include "iservice_registry.h"
#include "SleCollaborationManager.h"

namespace OHOS {
namespace Nearlink {

constexpr int32_t UNLOAD_SA_DELAY_MS = 15000;

UnloadSa& UnloadSa::GetInstance()
{
    static UnloadSa instance;
    return instance;
}

UnloadSa::UnloadSa() : isTimerStarted_(false)
{
    auto timeoutFunc = []() {
        UnloadSa::GetInstance().UnloadNearlinkSa();
    };
    unloadSaTimer_ = std::make_shared<NearlinkTimer>(timeoutFunc);
}

UnloadSa::~UnloadSa()
{
    unloadSaTimer_ = nullptr;
}

void UnloadSa::UnloadNearlinkSa()
{
    HILOGI("enter");
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    NL_CHECK_RETURN(samgr != nullptr, "GetSystemAbilityManager failed");
    int32_t ret = samgr->UnloadSystemAbility(NEARLINK_HOST_SYS_ABILITY_ID);
    NL_CHECK_RETURN(ret == ERR_OK, "UnloadSystemAbility failed with ret=%{public}d", ret);
    StopUnloadNearlinkSaTimer();
}

void UnloadSa::StartUnloadNearlinkSaTimer(const SleStateID state)
{
    if (state != SleStateID::STATE_TURN_OFF || isTimerStarted_.load()) {
        return;
    }
    HILOGI("start unload sa timer");
    unloadSaTimer_->Start(UNLOAD_SA_DELAY_MS, false);
    isTimerStarted_.store(true);
}

void UnloadSa::StopUnloadNearlinkSaTimer()
{
    if (!isTimerStarted_.load()) {
        return;
    }
    HILOGI("stop unload sa timer");
    unloadSaTimer_->Stop();
    isTimerStarted_.store(false);
}
} // namespace Nearlink
} // namespace OHOS