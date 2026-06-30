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

#include "SleNameChangeManager.h"

#include "nearlink_datashare_helper.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
using namespace std;

SleNameChangeManager::SleNameChangeManager()
{
    lastNotifiedName_ = NearlinkDataShareHelper::GetLocalDeviceName();
}

SleNameChangeManager::~SleNameChangeManager()
{}

SleNameChangeManager &SleNameChangeManager::GetInstance()
{
    static SleNameChangeManager instance;
    return instance;
}

void SleNameChangeManager::RegisterLocalNameObserver(std::shared_ptr<ISleNameChangeListener> observer)
{
    NL_CHECK_RETURN(observer, "observer is nullptr.");
    nameObservers_.Insert(observer);
}

void SleNameChangeManager::DeregisterLocalNameObserver(std::shared_ptr<ISleNameChangeListener> observer)
{
    NL_CHECK_RETURN(observer, "observer is nullptr.");
    nameObservers_.Erase(observer);
}

void SleNameChangeManager::NotifyLocalNameChanged(const std::string &newLocalName)
{
    {
        std::lock_guard<ffrt::mutex> lock(lastNotifiedNameMtx_);
        if (newLocalName == lastNotifiedName_) {
            HILOGI("device name is not changed");
            return;
        }
        lastNotifiedName_ = newLocalName;
    }
    nameObservers_.Iterate([&newLocalName](std::shared_ptr<ISleNameChangeListener> observer) -> void {
        observer->OnLocalNameChanged(newLocalName);
    });
}

void SleNameChangeObserver::OnChange()
{
    HILOGI("enter");
    std::string newDisplayName = NearlinkDataShareHelper::GetLocalDeviceName();
    NL_CHECK_RETURN(!newDisplayName.empty(), "device name empty");
    SleNameChangeManager::GetInstance().NotifyLocalNameChanged(newDisplayName);
}
} // namespace Nearlink
} // namespace OHOS