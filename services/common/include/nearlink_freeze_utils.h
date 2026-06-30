/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
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
        if (pid < appInfo.pid) {
            return true;
        } else if (uid < appInfo.uid) {
            return true;
        } else {
            return type < appInfo.type;
        }
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