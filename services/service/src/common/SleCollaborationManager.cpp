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

#include "SleCollaborationManager.h"

#include "SleServiceManager.h"
#include "ThreadUtil.h"
#include "nearlink_datashare_helper.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

namespace {
const std::string COLLABORATION_SERVICE_PARAM_VALUE_ON = "1";
} // namespace

SleCollaborationManager::SleCollaborationManager()
{}

SleCollaborationManager::~SleCollaborationManager()
{}

SleCollaborationManager &SleCollaborationManager::GetInstance()
{
    static SleCollaborationManager instance;
    return instance;
}

void SleCollaborationManager::Init()
{
    HILOGI("enter");
    isCollaborationOn_.store(GetCollaborationStateFromDataShare());
    NL_CHECK_RETURN(NearlinkDataShareHelper::GetInstance().RegisterCollaborationChangeObserver(),
        "RegisterCollaborationChangeObserver failed");
}

bool SleCollaborationManager::GetCollaborationStateFromDataShare() const
{
    return NearlinkDataShareHelper::GetInstance().GetCollaborationState();
}

bool SleCollaborationManager::IsCollaborationOn() const
{
    return isCollaborationOn_.load();
}

void SleCollaborationManager::ProcessNextCollaborationEvent()
{
    HILOGD("enter");
    CurrentCollaborationEventExecuteComplete();
}

void SleCollaborationManager::OnCollaborationStateChanged(bool isCollaborationOn)
{
    HILOGI("isCollaborationOn: %{public}d", isCollaborationOn);
    if (isCollaborationOn_ == isCollaborationOn) {
        HILOGI("CollaborationOn state is unchanged");
        return;
    }
    isCollaborationOn_.store(isCollaborationOn);
    {
        std::lock_guard<std::mutex> lock(cachedCollaborationEventVecMutex_);
        cachedCollaborationEventVec_.push_back(isCollaborationOn);
        if (cachedCollaborationEventVec_.size() > 1) { // 1 means one cached collaboration event to be processed.
            HILOGW("Collaboration change too frequent, cache collaboration event: %{public}d", isCollaborationOn);
            return;
        }
    }

    if (isCollaborationOn) {
        ProcessCollaborationOnEvent();
    } else {
        ProcessCollaborationOffEvent();
    }
}

void SleCollaborationManager::ProcessCollaborationOffEvent()
{
    HILOGI("enter");
    SleStateID currentNlState = SleServiceManager::GetInstance()->GetState(ADAPTER_SLE);
    if (!SleServiceManager::IsStateStable(currentNlState)) {
        // 星闪中间状态，暂不处理多设备协同事件
        HILOGI("Turning state, currentNlState is %{public}d", static_cast<int>(currentNlState));
        return;
    }
    if (currentNlState == SleStateID::STATE_TURN_HALF) {
        HILOGI("disenable nearlink because collaboration is off");
        DisableNlWhenCollaborationOff();
    } else {
        HILOGI("Ignore collaboration off event, currentNlState is %{public}d", static_cast<int>(currentNlState));
        CurrentCollaborationEventExecuteComplete();
    }
    return;
}

void SleCollaborationManager::ProcessCollaborationOnEvent()
{
    HILOGI("enter");
    SleStateID currentNlState = SleServiceManager::GetInstance()->GetState(ADAPTER_SLE);
    if (!SleServiceManager::IsStateStable(currentNlState)) {
        // 星闪中间状态，暂不处理多设备协同事件
        HILOGI("Turning state, currentNlState is %{public}d", static_cast<int>(currentNlState));
        return;
    }
    // 若星闪关闭，且飞行模式关闭，需要处理多设备协同开启事件，星闪应该变为半关状态
    bool isAirplaneOn = NearlinkDataShareHelper::GetInstance().GetAirplaneModeState();
    if (currentNlState == SleStateID::STATE_TURN_OFF && !isAirplaneOn) {
        HILOGI("switch nearlink from disable to half-disable because collaboration is on");
        SwitchNlToHalfWhenCollaborationOn();
    } else {
        HILOGI("Ignore collaboration on event, currentNlState is %{public}d", static_cast<int>(currentNlState));
        CurrentCollaborationEventExecuteComplete();
    }
    return;
}

void SleCollaborationManager::DisableNlWhenCollaborationOff()
{
    HILOGI("enter");
    SwitchCallerInfo callerInfo = SleInterfaceManager::GetCallerInfo();
    if (SleServiceManager::GetInstance()->DisableToOff(
        ADAPTER_SLE, SleEventType::COLLABORATION_TRIGGERED, callerInfo) != NL_NO_ERROR) {
        HILOGE("Nearlink turn off failed");
        CurrentCollaborationEventExecuteComplete();
    }
}

void SleCollaborationManager::SwitchNlToHalfWhenCollaborationOn()
{
    HILOGI("enter");
    SwitchCallerInfo callerInfo = SleInterfaceManager::GetCallerInfo();
    if (SleServiceManager::GetInstance()->EnableToHalf(
        ADAPTER_SLE, SleEventType::COLLABORATION_TRIGGERED, callerInfo) != NL_NO_ERROR) {
        HILOGE("Nearlink turn half failed");
        CurrentCollaborationEventExecuteComplete();
    }
}

void SleCollaborationManager::CurrentCollaborationEventExecuteComplete()
{
    HILOGD("enter");
    bool isNextEventProcessNeeded  = false;
    bool collaborationEvent = false;
    {
        std::lock_guard<std::mutex> lock(cachedCollaborationEventVecMutex_);
        if (cachedCollaborationEventVec_.empty()) {
            HILOGD("cachedCollaborationEventVec empty");
            return;
        }
        // 队头为当前处理的多设备协同状态，队尾为需要处理的最后一次的设备协同状态，若两者不一致，需要处理新的多设备协同状态
        isNextEventProcessNeeded = cachedCollaborationEventVec_.front() != cachedCollaborationEventVec_.back();
        HILOGI("isNextEventProcessNeeded: %{public}d", isNextEventProcessNeeded);
        collaborationEvent = cachedCollaborationEventVec_.back();
        cachedCollaborationEventVec_.clear();
    }

    // 1) 星闪打开状态，无需处理多设备协同事件
    // 2) 飞行模式打开状态，无需处理多设备协同事件
    // 其余情况（包括星闪中间态）需要后续处理
    SleStateID currentNlState = SleServiceManager::GetInstance()->GetState(ADAPTER_SLE);
    bool isNlOn = (currentNlState == SleStateID::STATE_TURN_ON);
    bool isAirplaneOn = NearlinkDataShareHelper::GetInstance().GetAirplaneModeState();
    bool isInvalidCollaborationModeEvent = isNlOn || isAirplaneOn;
    if (isNextEventProcessNeeded && !isInvalidCollaborationModeEvent) {
        HILOGI("Process cached airplane mode change event: %{public}d", collaborationEvent);
        OnCollaborationStateChanged(collaborationEvent);
    }
}

void SleCollaborationChangeObserver::OnChange()
{
    bool isCollaborationOn = NearlinkDataShareHelper::GetInstance().GetCollaborationState();
    SleCollaborationManager::GetInstance().OnCollaborationStateChanged(isCollaborationOn);
}

} // namespace Nearlink
} // namespace OHOS