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

#ifndef NEARLINK_SWITCH_MODULE_H
#define NEARLINK_SWITCH_MODULE_H

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include "nearlink_def.h"
#include "nearlink_errorcode.h"
#include "ffrt_inner.h"

namespace OHOS {
namespace Nearlink {
class INearlinkSwitchAction {
public:
    INearlinkSwitchAction() = default;
    virtual ~INearlinkSwitchAction() = default;

    virtual NlErrCode EnableNearlink(SleAutoConnectPolicy) = 0;
    virtual NlErrCode DisableNearlink() = 0;
    virtual NlErrCode DisableNearlinkToOff() = 0;
    virtual NlErrCode EnableNearlinkToHalf() = 0;
};

enum class NearlinkSwitchEvent : int {
    NONE_EVENT = -1,
    ENABLE_NEARLINK = 0, // 开关操作事件，开启星闪
    DISABLE_NEARLINK, // 开关操作事件，关闭星闪
    DISABLE_NEARLINK_TO_OFF, // 开关操作事件，关闭星闪至全关
    ENABLE_NEARLINK_TO_HALF, // 开关操作事件，开启星闪至半关

    NEARLINK_ON, // 状态变化事件，星闪开启
    NEARLINK_OFF, // 状态变化事件，星闪全关
    NEARLINK_HALF, // 状态变化事件，星闪半关

    DISABLE_TO_HALF_RESPONSE, // 关星闪响应事件，星闪需要半关
    DISABLE_TO_OFF_RESPONSE, // 关星闪响应事件，星闪需要全关
};

enum class NearlinkDisableStatus : int {
    STANDING_BY = -1,
    DISABLING_TO_HALF,
    DISABLING_TO_OFF,
};

class NearlinkSwitchModule : public std::enable_shared_from_this<NearlinkSwitchModule> {
public:
    explicit NearlinkSwitchModule(std::unique_ptr<INearlinkSwitchAction> switchAction)
        : ffrtQueue_("nl_switch"), switchAction_(std::move(switchAction)) {}
    ~NearlinkSwitchModule() = default;

    NlErrCode ProcessNearlinkSwitchEvent(NearlinkSwitchEvent event,
        const SleAutoConnectPolicy autoConnPolicy = SleAutoConnectPolicy::AUTO_CONN_GENERAL);
    void SetNoAutoConnect(bool noAutoConnect);

private:
    NlErrCode ProcessEnableNearlinkEvent(
        const SleAutoConnectPolicy autoConnPolicy = SleAutoConnectPolicy::AUTO_CONN_GENERAL);
    NlErrCode ProcessEnableNearlinkToHalfEvent(void);
    NlErrCode ProcessDisableNearlinkEvent(void);
    NlErrCode ProcessDisableNearlinkToOffEvent(void);
    NlErrCode ProcessNearlinkOnEvent(void);
    NlErrCode ProcessNearlinkOffEvent(void);
    NlErrCode ProcessNearlinkHalfEvent(void);
    NlErrCode ProcessDisableResponseHalfEvent(void);
    NlErrCode ProcessDisableResponseOffEvent(void);
    NlErrCode ProcessNearlinkSwitchAction(std::function<NlErrCode(void)> action, NearlinkSwitchEvent cachedEvent);
    NlErrCode ProcessNearlinkSwitchCachedEvent(NearlinkSwitchEvent event);
    NlErrCode ProcessNearlinkSwitchActionFinished(
        NearlinkSwitchEvent curSwitchActionEvent, std::vector<NearlinkSwitchEvent> expectedEventVec);
    void DeduplicateCachedEvent(NearlinkSwitchEvent curEvent);
    void RemoveIgnoredCachedEvent(size_t ignoredCnt);
    void LogNearlinkSwitchEvent(NearlinkSwitchEvent event);
    void OnTaskTimeout(void);

    const uint64_t DEFAULT_TASK_TIMEOUT = 8000000;  // 8s
    uint64_t taskTimeout_ = DEFAULT_TASK_TIMEOUT;
    ffrt::task_handle taskTimeoutHandle_;
    ffrt::queue ffrtQueue_;

    std::unique_ptr<INearlinkSwitchAction> switchAction_ { nullptr };
    NearlinkSwitchEvent currentSwitchEvent_ = NearlinkSwitchEvent::NONE_EVENT;
    NearlinkDisableStatus disableStatus_ = NearlinkDisableStatus::STANDING_BY;
    std::atomic_bool isNlSwitchProcessing_ { false };
    std::vector<NearlinkSwitchEvent> cachedEventVec_ {};
    ffrt::mutex nearlinkSwitchEventMutex_ {};  // Used for ProcessNearlinkSwitchEvent function
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_SWITCH_MODULE_H
