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

#ifndef OHOS_NEARLINK_STANDARD_HIVIEW_UE_H
#define OHOS_NEARLINK_STANDARD_HIVIEW_UE_H

#include <mutex>
#include <atomic>
#include "data_ability_observer_stub.h"

namespace OHOS {
namespace Nearlink {

class SleHiviewUe {
public:
    SleHiviewUe();
    ~SleHiviewUe();
    static SleHiviewUe &GetInstance();

    void Init();
    void OnHiviewUeStateChanged(bool isHiviewUeOn);

private:
    bool GetHiviewUeStateFromDataShare() const;
    std::atomic<bool> isHiviewUeOn_ = true;
};

class SleHiviewUeChangeObserver : public AAFwk::DataAbilityObserverStub {
public:
    explicit SleHiviewUeChangeObserver() {};
    ~SleHiviewUeChangeObserver() override {};
    void OnChange() override;
};
} // namespace Nearlink
} // namespace OHOS

#endif // SLE_COLLABORATION_MANAGER_H