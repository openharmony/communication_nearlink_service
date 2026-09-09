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

#include "ani_nearlink_manager_callback.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {
namespace {
std::shared_ptr<AniNearlinkManagerObserver> g_aniNearlinkManagerObserver =
    std::make_shared<AniNearlinkManagerObserver>();
}

std::vector<::taihe::optional<::taihe::callback<void(::ohos::nearlink::manager::NearlinkState data)>>>
    g_stateChangedObserverVec {};
std::shared_mutex g_stateChangedMutex;

void AniNearlinkManagerObserver::OnStateChanged(const int transport, const int status)
{
    HILOGI("transport is %{public}d, status is %{public}d", transport, status);
    if (status < static_cast<int>(SleStateID::STATE_TURNING_ON) ||
        status > static_cast<int>(SleStateID::STATE_TURN_HALF)) {
            HILOGE("Invalid status value: %{public}d", status);
            return;
    }
    ::ohos::nearlink::manager::NearlinkState result = ohos::nearlink::manager::NearlinkState::from_value(status);
    decltype(g_stateChangedObserverVec) callbacks;
    {
        std::shared_lock<std::shared_mutex> guard(g_stateChangedMutex);
        callbacks = g_stateChangedObserverVec;
    }
    for (auto callback : callbacks) {
        if (callback.has_value()) {
            (*callback)(result);
        }
    }
}

void AniNearlinkManager::CallbackInit()
{
    HILOGI("enter");
    NearlinkHost::GetInstance().RegisterObserver(g_aniNearlinkManagerObserver);
}
}
}