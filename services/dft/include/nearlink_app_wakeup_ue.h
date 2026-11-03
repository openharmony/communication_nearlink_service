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

#ifndef NEARLINK_APP_WAKEUP_UE_H
#define NEARLINK_APP_WAKEUP_UE_H

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "nearlink_safe_list.h"
#include "nearlink_safe_map.h"

namespace OHOS {
namespace EventFwk {
class CommonEventData;
class CommonEventSubscriber;
}  // namespace EventFwk

namespace Nearlink {

class NearlinkTimer;

/**
 * Business type of the app wakeup behavior.
 */
enum class AppBehaviorType : int64_t {
    SSAP_TRANSFER = 0,
    PORT_TRANSFER = 1,
    SCAN_TRANSFER = 2,
    ADVERTISER_TRANSFER = 3,
    SSAP_CALLBACK = 4,
    PORT_CALLBACK = 5,
};

/**
 * Collects app wakeup behaviors while screen is off and the device is not charging,
 * aggregates them by caller and business type, and reports them to hiview through the
 * NEARLINK_APP_WAKEUP_UE event periodically. The counting unit is one app wakeup: an
 * app api call or a service layer callback delivered to the app.
 */
class NearlinkAppWakeupUe {
public:
    static NearlinkAppWakeupUe &GetInstance(void);

    /**
     * @Description Stop the report timer and unsubscribe the common events, called when
     *              the service stops or destructs.
     */
    void Shutdown(void);

    /**
     * @Description Record a wakeup behavior of the current ipc caller.
     *              Only used in ipc threads.
     * @param type Business type of the behavior.
     */
    void Record(AppBehaviorType type);

    /**
     * @Description Record a wakeup behavior of the specified caller.
     * @param appName Bundle name or process name of the caller.
     * @param uid uid of the caller.
     * @param pid pid of the caller.
     * @param type Business type of the behavior.
     */
    void Record(const std::string &appName, int64_t uid, int64_t pid, AppBehaviorType type);

    /**
     * @Description Record a wakeup behavior by caller token id, used in async callback threads.
     * @param tokenId Token id of the caller.
     * @param uid uid of the caller.
     * @param pid pid of the caller.
     * @param type Business type of the behavior.
     */
    void RecordByToken(uint32_t tokenId, int64_t uid, int64_t pid, AppBehaviorType type);

private:
    struct WakeupRecord {
        std::string appName;
        int64_t uid {-1};
        int64_t pid {-1};
        int64_t businessType {0};
        int64_t triggerCount {0};

        bool operator==(const WakeupRecord &other) const
        {
            return appName == other.appName && uid == other.uid && pid == other.pid &&
                businessType == other.businessType;
        }
    };

    NearlinkAppWakeupUe() = default;
    ~NearlinkAppWakeupUe();
    NearlinkAppWakeupUe(const NearlinkAppWakeupUe &) = delete;
    NearlinkAppWakeupUe &operator=(const NearlinkAppWakeupUe &) = delete;

    void LazyInit(void);
    void EnsureReportTimerStarted(void);
    void StopReportTimer(void);
    void RecordInternal(const std::string &appName, int64_t uid, int64_t pid, AppBehaviorType type);
    std::string GetAppNameByToken(uint32_t tokenId);
    void OnCommonEvent(const EventFwk::CommonEventData &data);
    void Report(void);
    void WriteEvent(const std::vector<WakeupRecord> &records);

    std::atomic<bool> isInited_ {false};
    std::atomic<bool> isScreenOn_ {true};
    std::atomic<bool> isCharging_ {true};  // unknown until the first battery event, treat as charging
    std::atomic<bool> isTimerStarted_ {false};
    std::mutex subscriberMutex_ {};   // protect subscriber_, written when subscribing, read when destructing
    std::mutex reportTimerMutex_ {};  // serialize the timer start/stop with isTimerStarted_ and reportTimer_
    std::mutex recordsMutex_ {};      // keep the find/size-check/insert aggregation of records_ atomic
    std::shared_ptr<NearlinkTimer> reportTimer_ {nullptr};  // guarded by reportTimerMutex_
    std::shared_ptr<EventFwk::CommonEventSubscriber> subscriber_ {nullptr};  // guarded by subscriberMutex_
    NearlinkSafeList<WakeupRecord> records_ {};
    NearlinkSafeMap<uint32_t, std::string> appNameCache_ {};
};

}  // namespace Nearlink
}  // namespace OHOS

#endif  // NEARLINK_APP_WAKEUP_UE_H
