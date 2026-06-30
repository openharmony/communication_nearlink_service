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
#ifndef LOG_TAG
#define LOG_TAG "nl_fwk_switch_module"
#endif

#include "nearlink_switch_module.h"

#include <algorithm>
#include "parameters.h"
#include "log_util.h"


namespace OHOS {
namespace Nearlink {
namespace {
const char *NEARLINK_SWITCH_SYSTEM_PARAMETER_NAME = "persist.nearlink.switch_enable";
const char *NEARLINK_SWITCH_STATE_OFF = "0";
const char *NEARLINK_SWITCH_STATE_ON = "1";
const char *NEARLINK_SWITCH_STATE_HALF = "2";

const char *ToEventString(NearlinkSwitchEvent event)
{
    switch (event) {
        case NearlinkSwitchEvent::ENABLE_NEARLINK: return "ENABLE_NEARLINK";
        case NearlinkSwitchEvent::DISABLE_NEARLINK: return "DISABLE_NEARLINK";
        case NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF: return "DISABLE_NEARLINK_TO_OFF";
        case NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF: return "ENABLE_NEARLINK_TO_HALF";
        case NearlinkSwitchEvent::NEARLINK_ON: return "NEARLINK_ON";
        case NearlinkSwitchEvent::NEARLINK_OFF: return "NEARLINK_OFF";
        case NearlinkSwitchEvent::NEARLINK_HALF: return "NEARLINK_HALF";
        case NearlinkSwitchEvent::DISABLE_TO_HALF_RESPONSE: return "DISABLE_TO_HALF_RESPONSE";
        case NearlinkSwitchEvent::DISABLE_TO_OFF_RESPONSE: return "DISABLE_TO_OFF_RESPONSE";
        default: break;
    }
    return "Unknown event";
}
} // namespace

void NearlinkSwitchModule::LogNearlinkSwitchEvent(NearlinkSwitchEvent event)
{
    bool needLog = (event == NearlinkSwitchEvent::NEARLINK_ON ||
        event == NearlinkSwitchEvent::NEARLINK_OFF ||
        event == NearlinkSwitchEvent::NEARLINK_HALF) ? isNlSwitchProcessing_.load() : true;
    if (needLog) {
        HILOGI("[NearlinkSwitchModule] Process Event: %{public}s", ToEventString(event));
    }
}

NlErrCode NearlinkSwitchModule::ProcessNearlinkSwitchEvent(
    NearlinkSwitchEvent event, const SleAutoConnectPolicy autoConnPolicy)
{
    NL_CHECK_RETURN_RET(switchAction_, NL_ERR_INTERNAL_ERROR, "switchAction is nullptr");

    std::lock_guard<ffrt::mutex> lock(nearlinkSwitchEventMutex_);
    LogNearlinkSwitchEvent(event);
    switch (event) {
        case NearlinkSwitchEvent::ENABLE_NEARLINK:
            return ProcessEnableNearlinkEvent(autoConnPolicy);
        case NearlinkSwitchEvent::DISABLE_NEARLINK:
            return ProcessDisableNearlinkEvent();
        case NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF:
            return ProcessDisableNearlinkToOffEvent();
        case NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF:
            return ProcessEnableNearlinkToHalfEvent();
        case NearlinkSwitchEvent::NEARLINK_ON:
            return ProcessNearlinkOnEvent();
        case NearlinkSwitchEvent::NEARLINK_OFF:
            return ProcessNearlinkOffEvent();
        case NearlinkSwitchEvent::NEARLINK_HALF:
            return ProcessNearlinkHalfEvent();
        case NearlinkSwitchEvent::DISABLE_TO_HALF_RESPONSE:
            return ProcessDisableResponseHalfEvent();
        case NearlinkSwitchEvent::DISABLE_TO_OFF_RESPONSE:
            return ProcessDisableResponseOffEvent();
        default: break;
    }
    HILOGE("[NearlinkSwitchModule] Invalid event: %{public}s", ToEventString(event));
    return NL_ERR_INTERNAL_ERROR;
}

void NearlinkSwitchModule::OnTaskTimeout(void)
{
    HILOGW("[NearlinkSwitchModule] Nearlink switch action timeout, clear resources");
    std::lock_guard<ffrt::mutex> lock(nearlinkSwitchEventMutex_);
    isNlSwitchProcessing_ = false;
    cachedEventVec_.clear();
}

NlErrCode NearlinkSwitchModule::ProcessNearlinkSwitchAction(
    std::function<NlErrCode(void)> action, NearlinkSwitchEvent switchEvent)
{
    currentSwitchEvent_ = switchEvent;
    if (isNlSwitchProcessing_.load()) {
        cachedEventVec_.push_back(switchEvent);
        HILOGW("[NearlinkSwitchModule] NlSwich action is processing, cache the %{public}s event",
            ToEventString(switchEvent));
        return NL_NO_ERROR;
    }

    ffrt::task_attr taskAttr;
    taskAttr.name("nl_switch").delay(taskTimeout_);
    taskTimeoutHandle_ = ffrtQueue_.submit_h([switchWptr = weak_from_this()]() -> void {
        auto switchSptr = switchWptr.lock();
        if (switchSptr == nullptr) {
            HILOGE("switchSptr is nullptr");
            return;
        }
        switchSptr->OnTaskTimeout();
    }, taskAttr);

    isNlSwitchProcessing_ = true;
    NlErrCode ret = action();
    if (ret != NL_NO_ERROR) {
        isNlSwitchProcessing_ = false;
        ffrtQueue_.cancel(taskTimeoutHandle_);
    }
    // Invalid operaton is considered successful.
    if (ret == NL_ERR_INVALID_SWITCH_OPERATION) {
        HILOGI("[NearlinkSwitchModule] switch operaton %{public}s is invalid", ToEventString(switchEvent));
        ret = NL_NO_ERROR;
        // 结束本次开关操作，并将下一个缓存的操作事件抛入任务队列
        ProcessNearlinkSwitchActionFinished(switchEvent, {NearlinkSwitchEvent::ENABLE_NEARLINK,
            NearlinkSwitchEvent::DISABLE_NEARLINK, NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF,
            NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF});
    }
    return ret;
}

NlErrCode NearlinkSwitchModule::ProcessEnableNearlinkEvent(const SleAutoConnectPolicy autoConnPolicy)
{
    return ProcessNearlinkSwitchAction([switchWptr = weak_from_this(), autoConnPolicy]() -> NlErrCode {
            auto switchSptr = switchWptr.lock();
            NL_CHECK_RETURN_RET(switchSptr != nullptr, NL_ERR_INTERNAL_ERROR, "switchSptr is nullptr");
            return switchSptr->switchAction_->EnableNearlink(autoConnPolicy);
        }, NearlinkSwitchEvent::ENABLE_NEARLINK);
}

NlErrCode NearlinkSwitchModule::ProcessDisableNearlinkEvent()
{
    return ProcessNearlinkSwitchAction([switchWptr = weak_from_this()]() -> NlErrCode {
            auto switchSptr = switchWptr.lock();
            NL_CHECK_RETURN_RET(switchSptr != nullptr, NL_ERR_INTERNAL_ERROR, "switchSptr is nullptr");
            return switchSptr->switchAction_->DisableNearlink();
        }, NearlinkSwitchEvent::DISABLE_NEARLINK);
}

NlErrCode NearlinkSwitchModule::ProcessEnableNearlinkToHalfEvent()
{
    return ProcessNearlinkSwitchAction([switchWptr = weak_from_this()]() -> NlErrCode {
            auto switchSptr = switchWptr.lock();
            NL_CHECK_RETURN_RET(switchSptr != nullptr, NL_ERR_INTERNAL_ERROR, "switchSptr is nullptr");
            return switchSptr->switchAction_->EnableNearlinkToHalf();
        }, NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF);
}

NlErrCode NearlinkSwitchModule::ProcessDisableNearlinkToOffEvent()
{
    return ProcessNearlinkSwitchAction([switchWptr = weak_from_this()]() -> NlErrCode {
            auto switchSptr = switchWptr.lock();
            NL_CHECK_RETURN_RET(switchSptr != nullptr, NL_ERR_INTERNAL_ERROR, "switchSptr is nullptr");
            return switchSptr->switchAction_->DisableNearlinkToOff();
        }, NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF);
}

NlErrCode NearlinkSwitchModule::ProcessNearlinkOnEvent()
{
    return ProcessNearlinkSwitchActionFinished(NearlinkSwitchEvent::ENABLE_NEARLINK,
        {NearlinkSwitchEvent::DISABLE_NEARLINK, NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF});
}

NlErrCode NearlinkSwitchModule::ProcessNearlinkOffEvent()
{
    if (disableStatus_ == NearlinkDisableStatus::STANDING_BY) {
        // DisableNlToOff动作结束
        return ProcessNearlinkSwitchActionFinished(NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF,
            {NearlinkSwitchEvent::ENABLE_NEARLINK, NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF});
    } 
    if (disableStatus_ == NearlinkDisableStatus::DISABLING_TO_OFF) {
        // 此次星闪进入OFF由非三态场景的DisableNl触发，DisableNl动作结束
        disableStatus_ = NearlinkDisableStatus::STANDING_BY;
        return ProcessNearlinkSwitchActionFinished(NearlinkSwitchEvent::DISABLE_NEARLINK,
            {NearlinkSwitchEvent::ENABLE_NEARLINK, NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF});
    }
    if (disableStatus_ == NearlinkDisableStatus::DISABLING_TO_HALF) {
        // 此次星闪进入OFF由三态场景的DisableNl触发，等待星闪开至半关状态
        HILOGI("[NearlinkSwitchModule] waiting for NEARLINK_HALF event");
        return NL_NO_ERROR;
    }
    HILOGE("Invalid disableStatus_: %{public}d", static_cast<int>(disableStatus_));
    return NL_ERR_INTERNAL_ERROR;
}

NlErrCode NearlinkSwitchModule::ProcessNearlinkHalfEvent()
{
    if (disableStatus_ == NearlinkDisableStatus::DISABLING_TO_HALF) {
        // 此次星闪进入HALF由三态场景的DisableNl触发，DisableNl动作结束
        disableStatus_ = NearlinkDisableStatus::STANDING_BY;
        return ProcessNearlinkSwitchActionFinished(NearlinkSwitchEvent::DISABLE_NEARLINK,
            {NearlinkSwitchEvent::ENABLE_NEARLINK, NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF});
    }
    // EnableNlToHalf动作结束
    return ProcessNearlinkSwitchActionFinished(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF,
        {NearlinkSwitchEvent::ENABLE_NEARLINK, NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF});
}

NlErrCode NearlinkSwitchModule::ProcessDisableResponseHalfEvent()
{
    HILOGI("[NearlinkSwitchModule] disable response received, target state is half");
    disableStatus_ = NearlinkDisableStatus::DISABLING_TO_HALF;
    return NL_NO_ERROR;
}

NlErrCode NearlinkSwitchModule::ProcessDisableResponseOffEvent()
{
    HILOGI("[NearlinkSwitchModule] disable response received, target state is off");
    disableStatus_ = NearlinkDisableStatus::DISABLING_TO_OFF;
    return NL_NO_ERROR;
}

NlErrCode NearlinkSwitchModule::ProcessNearlinkSwitchActionFinished(
    NearlinkSwitchEvent curSwitchActionEvent, std::vector<NearlinkSwitchEvent> expectedEventVec)
{
    isNlSwitchProcessing_ = false;
    ffrtQueue_.cancel(taskTimeoutHandle_);
    DeduplicateCachedEvent(curSwitchActionEvent);

    // Expect process the next event in 'expectedProcessEventVec'
    auto it = std::find_if(cachedEventVec_.begin(), cachedEventVec_.end(), [&expectedEventVec](auto event) {
        return std::find(expectedEventVec.begin(), expectedEventVec.end(), event) != expectedEventVec.end();
    });
    if (it != cachedEventVec_.end()) {
        NearlinkSwitchEvent event = *it;
        if (it != cachedEventVec_.begin()) {
            // Ignore the cached events in front of 'expectedEventVec'
            size_t ignoredCnt = static_cast<size_t>(std::distance(cachedEventVec_.begin(), it));
            RemoveIgnoredCachedEvent(ignoredCnt);
        }
        cachedEventVec_.erase(cachedEventVec_.begin());
        return ProcessNearlinkSwitchCachedEvent(event);
    }

    cachedEventVec_.clear();
    return NL_NO_ERROR;
}

NlErrCode NearlinkSwitchModule::ProcessNearlinkSwitchCachedEvent(NearlinkSwitchEvent event)
{
    HILOGI("[NearlinkSwitchModule] Process cached %{public}s event", ToEventString(event));
    ffrtQueue_.submit([switchWptr = weak_from_this(), event]() -> void {
        auto switchSptr = switchWptr.lock();
        if (switchSptr == nullptr) {
            HILOGE("switchSptr is nullptr");
            return;
        }
        switchSptr->ProcessNearlinkSwitchEvent(event);
    });
    return NL_NO_ERROR;
}

void NearlinkSwitchModule::DeduplicateCachedEvent(NearlinkSwitchEvent curEvent)
{
    // 从缓存事件列表里找到最后一个curEvent，保留该事件之后的缓存事件
    auto it = std::find(cachedEventVec_.rbegin(), cachedEventVec_.rend(), curEvent);
    if (it != cachedEventVec_.rend()) {
        // The it.base() is greater than cachedEventVec_.begin(), so std::distance > 0.
        size_t ignoredCnt = static_cast<size_t>(std::distance(cachedEventVec_.begin(), it.base())) - 1;
        RemoveIgnoredCachedEvent(ignoredCnt);
    }
}

void NearlinkSwitchModule::RemoveIgnoredCachedEvent(size_t ignoredCnt)
{
    size_t ignoredEventCnt = ignoredCnt > cachedEventVec_.size() ?
        cachedEventVec_.size() : ignoredCnt;
    std::string log = "";
    for (size_t i = 0; i < ignoredEventCnt; i++) {
        // The last event current process event, not ignored
        log += ToEventString(cachedEventVec_[i]);
        log += " ";
    }
    if (!log.empty()) {
        HILOGW("[NearlinkSwitchModule] Ignore cached event: %{public}s", log.c_str());
    }
    cachedEventVec_.erase(cachedEventVec_.begin(), cachedEventVec_.begin() + ignoredEventCnt + 1);
}
}  // namespace Nearlink
}  // namespace OHOS
