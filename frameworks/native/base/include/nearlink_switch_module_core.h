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

#ifndef NEARLINK_SWITCH_MODULE_CORE_H
#define NEARLINK_SWITCH_MODULE_CORE_H

#include "nearlink_switch_module.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief base.so implementation class of framework switch module.
 *
 * Kept in sync with NearlinkSwitchModule declared in nearlink_switch_module.h;
 * INearlinkSwitchAction and switch enums are reused from that header.
 */
class NearlinkSwitchModuleCore : public std::enable_shared_from_this<NearlinkSwitchModuleCore> {
public:
    explicit NearlinkSwitchModuleCore(std::unique_ptr<INearlinkSwitchAction> switchAction)
        : ffrtQueue_("nl_switch"), switchAction_(std::move(switchAction)) {}
    ~NearlinkSwitchModuleCore() = default;

    NlErrCode ProcessNearlinkSwitchEvent(NearlinkSwitchEvent event,
        const SleAutoConnectPolicy autoConnPolicy = SleAutoConnectPolicy::AUTO_CONN_GENERAL,
        int32_t loadSaTimeoutMs = 0);
    void SetNoAutoConnect(bool noAutoConnect);

private:
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
    NlErrCode ProcessNearlinkSwitchAction(
        std::function<NlErrCode(const NearlinkSwitchActionValidChecker &)> action,
        NearlinkSwitchEvent cachedEvent, int32_t loadSaTimeoutMs = -1);
    NlErrCode FinishSwitchAction(NearlinkSwitchEvent switchEvent, uint32_t actionSeq, NlErrCode ret);
    NlErrCode ProcessNearlinkSwitchCachedEvent(NearlinkSwitchEvent event);
    NlErrCode ProcessNearlinkSwitchActionFinished(
        NearlinkSwitchEvent curSwitchActionEvent, std::vector<NearlinkSwitchEvent> expectedEventVec);
    void DeduplicateCachedEvent(NearlinkSwitchEvent curEvent);
    void RemoveIgnoredCachedEvent(size_t ignoredCnt);
    void LogNearlinkSwitchEvent(NearlinkSwitchEvent event);
    void OnTaskTimeout(uint32_t actionSeq);
    void SetTaskTimeout(uint32_t actionSeq, uint64_t delayUs);

    const uint64_t DEFAULT_TASK_TIMEOUT = 8000000;  // 8s
    uint64_t taskTimeout_ = DEFAULT_TASK_TIMEOUT;
    const int32_t DEFAULT_SA_LOAD_TIMEOUT_MS = 30000;
    const uint32_t MAX_CONSECUTIVE_TIMEOUT_CNT = 3;
    uint32_t consecutiveTimeoutCnt_ = 0;
    ffrt::task_handle taskTimeoutHandle_;
    ffrt::queue ffrtQueue_;
    uint32_t latestActionSeq_ = 0;

    std::unique_ptr<INearlinkSwitchAction> switchAction_ { nullptr };
    NearlinkSwitchEvent currentSwitchEvent_ = NearlinkSwitchEvent::NONE_EVENT;
    NearlinkDisableStatus disableStatus_ = NearlinkDisableStatus::STANDING_BY;
    std::atomic_bool isNlSwitchProcessing_ { false };
    std::vector<NearlinkSwitchEvent> cachedEventVec_ {};
    ffrt::mutex nearlinkSwitchEventMutex_ {};
};
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_SWITCH_MODULE_CORE_H
