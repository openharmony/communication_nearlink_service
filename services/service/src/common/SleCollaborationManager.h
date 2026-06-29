/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef SLE_COLLABORATION_MANAGER_H
#define SLE_COLLABORATION_MANAGER_H

#include <mutex>
#include <vector>
#include <atomic>
#include "data_ability_observer_stub.h"

namespace OHOS {
namespace Nearlink {

class SleCollaborationManager {
public:
    SleCollaborationManager();
    ~SleCollaborationManager();
    static SleCollaborationManager &GetInstance();

    void Init();
    void OnCollaborationStateChanged(bool isCollaborationOn);
    /**
     * @brief 处理下一个多设备协同事件，星闪状态稳定后调用，防止快速连续触发导致的乒乓效应
     */
    void ProcessNextCollaborationEvent();
    bool IsCollaborationOn() const;

private:
    bool GetCollaborationStateFromDataShare() const;
    void ProcessCollaborationOffEvent();
    void ProcessCollaborationOnEvent();

    void CurrentCollaborationEventExecuteComplete();
    void DisableNlWhenCollaborationOff();
    void SwitchNlToHalfWhenCollaborationOn();

    std::atomic<bool> isCollaborationOn_ = true;

    std::mutex cachedCollaborationEventVecMutex_ {};
    std::vector<bool> cachedCollaborationEventVec_ {};
};

class SleCollaborationChangeObserver : public AAFwk::DataAbilityObserverStub {
public:
    explicit SleCollaborationChangeObserver() {};
    ~SleCollaborationChangeObserver() override {};
    void OnChange() override;
};
} // namespace Nearlink
} // namespace OHOS

#endif // SLE_COLLABORATION_MANAGER_H