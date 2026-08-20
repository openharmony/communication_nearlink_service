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

#ifndef NEARLINK_FREEZE_UTILS_H
#define NEARLINK_FREEZE_UTILS_H

#include <string>
#include "ffrt_inner.h"
#include "nearlink_safe_map.h"
#include "nearlink_system_ability_subscriber.h"

namespace OHOS {
namespace Nearlink {
#ifdef RES_SCHED_SUPPORT

class NearlinkFreezeUtil {
struct AppInfo;
public:
    NearlinkFreezeUtil() = default;
    ~NearlinkFreezeUtil();
    static NearlinkFreezeUtil *GetInstance(void);
    void InitQueueTask();
    void CheckPeriodical();
    void SetFreezeState();
    void RequestActive(const int32_t pid, const int32_t uid, const std::string dataType, const std::string &reason);
    bool IsNeedReport(AppInfo appInfo);
    void ReportNlDataToRss(const int32_t pid, const int32_t uid, const bool &isTransfer,
                           const std::string &type, const std::string &reason);
    void ReportNlConnectStateToRss(const int32_t pid, const int32_t uid,
                                   const std::string &action, const std::string &type, const std::string &reason);
private:
    void SubscribeRssSystemAbility();
    struct AppInfo {
        int32_t pid;
        int32_t uid;
        std::string type;
        AppInfo(int32_t pid_, int32_t uid_, std::string type_) : pid(pid_), uid(uid_), type(type_)
    {};

    bool operator == (const AppInfo &appInfo) const {
        return pid == appInfo.pid && uid == appInfo.uid && type == appInfo.type;
    }

    bool operator < (const AppInfo &appInfo) const {
        if (pid != appInfo.pid) {
            return pid < appInfo.pid;
        }
        if (uid != appInfo.uid) {
            return uid < appInfo.uid;
        }
        return type < appInfo.type;
    }
    };
    NearlinkSafeMap<AppInfo, bool> hasDataReport_{};
    NearlinkSafeMap<AppInfo, int> ssapConnectState_{};
    std::shared_ptr<ffrt::queue> ffrtQueue_ = nullptr;
    std::mutex initMutex_;
    sptr<NearlinkSystemAbilitySubscriber> statusChangeListener_ { nullptr };
};
#endif
}  // namespace Nearlink
}  // namespace OHOS
#endif  // NEARLINK_FREEZE_UTILS_H