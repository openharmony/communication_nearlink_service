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

    virtual NlErrCode EnableNearlink(SleAutoConnectPolicy, int32_t loadSaTimeoutMs) = 0;
    virtual NlErrCode DisableNearlink() = 0;
    virtual NlErrCode DisableNearlinkToOff() = 0;
    virtual NlErrCode EnableNearlinkToHalf(int32_t loadSaTimeoutMs) = 0;
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
        const SleAutoConnectPolicy autoConnPolicy = SleAutoConnectPolicy::AUTO_CONN_GENERAL,
        int32_t loadSaTimeoutMs = 0);  // loadSaTimeoutMs: SA 加载超时(ms)，<=0 时由开关动作使用默认超时
    void SetNoAutoConnect(bool noAutoConnect);

private:
    NlErrCode ProcessSwitchOperationEvent(NearlinkSwitchEvent event,
        const SleAutoConnectPolicy autoConnPolicy, int32_t loadSaTimeoutMs);
    NlErrCode ProcessStateEvent(NearlinkSwitchEvent event);
    NlErrCode ProcessEnableNearlinkEvent(const SleAutoConnectPolicy autoConnPolicy,
        int32_t loadSaTimeoutMs);
    NlErrCode ProcessEnableNearlinkToHalfEvent(int32_t loadSaTimeoutMs);
    NlErrCode ProcessDisableNearlinkEvent(void);
    NlErrCode ProcessDisableNearlinkToOffEvent(void);
    NlErrCode ProcessNearlinkOnEvent(void);
    NlErrCode ProcessNearlinkOffEvent(void);
    NlErrCode ProcessNearlinkHalfEvent(void);
    NlErrCode ProcessDisableResponseHalfEvent(void);
    NlErrCode ProcessDisableResponseOffEvent(void);
    NlErrCode ProcessNearlinkSwitchAction(std::function<NlErrCode(void)> action, NearlinkSwitchEvent cachedEvent);
    NlErrCode FinishSwitchAction(NearlinkSwitchEvent switchEvent, uint32_t actionGen, NlErrCode ret);
    NlErrCode ProcessNearlinkSwitchCachedEvent(NearlinkSwitchEvent event);
    NlErrCode ProcessNearlinkSwitchActionFinished(
        NearlinkSwitchEvent curSwitchActionEvent, std::vector<NearlinkSwitchEvent> expectedEventVec);
    void DeduplicateCachedEvent(NearlinkSwitchEvent curEvent);
    void RemoveIgnoredCachedEvent(size_t ignoredCnt);
    void LogNearlinkSwitchEvent(NearlinkSwitchEvent event);
    void OnTaskTimeout(uint32_t actionGen);

    const uint64_t DEFAULT_TASK_TIMEOUT = 8000000;  // 8s
    uint64_t taskTimeout_ = DEFAULT_TASK_TIMEOUT;
    const uint32_t MAX_CONSECUTIVE_TIMEOUT_CNT = 3;  // 连续超时达到该次数后清空缓存队列
    // 连续超时次数：动作正常完成、动作立即失败、超时且无缓存事件三处清零；
    // 上限仅在“每次超时都命中非空缓存且期间无动作正常完成”时可达
    uint32_t consecutiveTimeoutCnt_ = 0;
    ffrt::task_handle taskTimeoutHandle_;
    ffrt::queue ffrtQueue_;
    // 在途动作代次：动作启动与终结时递增，用于作废旧动作返回与陈旧超时任务的状态写入
    uint32_t actionGeneration_ = 0;

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
