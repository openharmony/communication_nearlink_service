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

#ifndef SLE_NAME_CHANGE_MANAGER_H
#define SLE_NAME_CHANGE_MANAGER_H

#include <string>
#include "datashare_helper.h"
#include "data_ability_observer_stub.h"
#include "ffrt_inner.h"
#include "nearlink_safe_weak_list.h"

namespace OHOS {
namespace Nearlink {

class ISleNameChangeListener {
public:
    virtual ~ISleNameChangeListener() = default;
    virtual void OnLocalNameChanged(const std::string &newLocalName) = 0;
};

class SleNameChangeManager {
public:
    SleNameChangeManager();
    ~SleNameChangeManager();
    static SleNameChangeManager &GetInstance();

    void RegisterLocalNameObserver(std::shared_ptr<ISleNameChangeListener> observer);
    void DeregisterLocalNameObserver(std::shared_ptr<ISleNameChangeListener> observer);
    
    void NotifyLocalNameChanged(const std::string &newLocalName);
private:
    NearlinkSafeWeakList<ISleNameChangeListener> nameObservers_ {};
    ffrt::mutex lastNotifiedNameMtx_;
    std::string lastNotifiedName_;
};

class SleNameChangeObserver : public AAFwk::DataAbilityObserverStub {
public:
    explicit SleNameChangeObserver() {};
    ~SleNameChangeObserver() override {};
    void OnChange() override;
};
} // namespace Nearlink
} // namespace OHOS

#endif // SLE_NAME_CHANGE_MANAGER_H