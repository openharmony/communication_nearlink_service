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

#include "nearlink_app_wakeup_ue.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common_event_data.h"
#include "common_event_manager.h"
#include "common_event_subscribe_info.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"
#include "hisysevent_c.h"
#include "ipc_skeleton.h"
#include "log.h"
#include "nearlink_dft_manager_c.h"
#include "nearlink_permission_manager.h"
#include "nearlink_timer.h"
#include "securec.h"

#ifdef NEARLINK_POWER_MGR_ENABLE
#include "power_mgr_client.h"
#endif

namespace OHOS {
namespace Nearlink {
namespace {
constexpr int REPORT_INTERVAL_MS = 30 * 60 * 1000;
constexpr size_t MAX_RECORD_NUM = 100;
constexpr size_t MAX_APP_NAME_CACHE_NUM = 100;
constexpr size_t PARAM_NUM = 5;

const std::string DEFAULT_APP_NAME = "recvApp";
const std::string DOMAIN_NAME = "NEARLINK_UE";
const std::string EVENT_NAME = "NEARLINK_APP_WAKEUP_UE";

const std::string PARAM_APP_NAME = "APP_NAME";
const std::string PARAM_APP_UID = "APP_UID";
const std::string PARAM_APP_PID = "APP_PID";
const std::string PARAM_BUSINESS_TYPE = "BUSINESS_TYPE";
const std::string PARAM_TRIGGER_COUNT = "TRIGGER_COUNT";

const std::string BATTERY_CHARGE_STATE_KEY = "chargeState";
// the same value as BatteryChargeState::CHARGE_STATE_ENABLE in battery_manager
constexpr int32_t BATTERY_CHARGE_STATE_ENABLE = 1;

class AppWakeupEventSubscriber : public EventFwk::CommonEventSubscriber {
public:
    AppWakeupEventSubscriber(const EventFwk::CommonEventSubscribeInfo &subscribeInfo,
        const std::function<void(const EventFwk::CommonEventData &)> &callback)
        : EventFwk::CommonEventSubscriber(subscribeInfo), callback_(callback)
    {}

    ~AppWakeupEventSubscriber() override = default;

    void OnReceiveEvent(const EventFwk::CommonEventData &data) override
    {
        if (callback_ != nullptr) {
            callback_(data);
        }
    }

private:
    std::function<void(const EventFwk::CommonEventData &)> callback_;
};

void InitArrayParam(HiSysEventParam &param, const std::string &name, HiSysEventParamType type, void *array,
    size_t arraySize)
{
    if (strcpy_s(param.name, MAX_LENGTH_OF_PARAM_NAME, name.c_str()) != 0) {
        return;
    }
    param.t = type;
    param.v.array = array;
    param.arraySize = arraySize;
}
}  // namespace

NearlinkAppWakeupUe &NearlinkAppWakeupUe::GetInstance(void)
{
    static NearlinkAppWakeupUe instance;
    return instance;
}

NearlinkAppWakeupUe::~NearlinkAppWakeupUe()
{
    Shutdown();
}

void NearlinkAppWakeupUe::Shutdown(void)
{
    StopReportTimer();
    std::shared_ptr<EventFwk::CommonEventSubscriber> subscriber = nullptr;
    {
        std::lock_guard<std::mutex> lock(subscriberMutex_);
        subscriber = subscriber_;
        subscriber_ = nullptr;
    }
    if (subscriber != nullptr) {
        (void)EventFwk::CommonEventManager::UnSubscribeCommonEvent(subscriber);
    }
    // reset the flag so the next record re-initializes (subscribe and refresh the states)
    // after the service restarts; the collected records are kept for the next report period
    isInited_.store(false);
    HILOGD("app wakeup ue shutdown, keep %{public}zu records", records_.Size());
}

void NearlinkAppWakeupUe::Record(AppBehaviorType type)
{
    RecordByToken(IPCSkeleton::GetCallingTokenID(), static_cast<int64_t>(IPCSkeleton::GetCallingUid()),
        static_cast<int64_t>(IPCSkeleton::GetCallingPid()), type);
}

void NearlinkAppWakeupUe::Record(const std::string &appName, int64_t uid, int64_t pid, AppBehaviorType type)
{
    RecordInternal(appName, uid, pid, type);
}

void NearlinkAppWakeupUe::RecordByToken(uint32_t tokenId, int64_t uid, int64_t pid, AppBehaviorType type)
{
    RecordInternal(GetAppNameByToken(tokenId), uid, pid, type);
}

std::string NearlinkAppWakeupUe::GetAppNameByToken(uint32_t tokenId)
{
    std::string appName;
    if (appNameCache_.GetValue(tokenId, appName)) {
        return appName;
    }
    // the token query may trigger ipc, do not hold the lock here
    appName = NearLinkPermissionManager::GetCallingName(tokenId);
    if (appNameCache_.Size() >= static_cast<int>(MAX_APP_NAME_CACHE_NUM)) {
        appNameCache_.Clear();
    }
    appNameCache_.EnsureInsert(tokenId, appName);
    return appName;
}

void NearlinkAppWakeupUe::RecordInternal(const std::string &appName, int64_t uid, int64_t pid,
    AppBehaviorType type)
{
    if (!DftHiviewUeStateIsOn()) {
        return;
    }
    LazyInit();
    std::string name = appName.empty() ? DEFAULT_APP_NAME : appName;
    if (isScreenOn_.load() || isCharging_.load()) {
        return;
    }
    int64_t businessType = static_cast<int64_t>(type);
    bool isFound = false;
    {
        std::lock_guard<std::mutex> lock(recordsMutex_);
        records_.IterateAndModify([&name, uid, pid, businessType, &isFound](WakeupRecord &record) {
            if (record.appName == name && record.uid == uid && record.pid == pid &&
                record.businessType == businessType) {
                record.triggerCount++;
                isFound = true;
            }
        });
        if (!isFound) {
            if (records_.Size() >= MAX_RECORD_NUM) {
                return;
            }
            records_.Insert(WakeupRecord {name, uid, pid, businessType, 1});
        }
    }
    EnsureReportTimerStarted();
}

void NearlinkAppWakeupUe::LazyInit(void)
{
    if (isInited_.exchange(true)) {
        return;
    }

    // the macro is only defined when the power manager is in the product; without it the
    // screen state keeps the conservative default (on), so off-screen wakeups are not
    // collected in that build config until the next screen off event
    bool isScreenOn = true;
#ifdef NEARLINK_POWER_MGR_ENABLE
    isScreenOn = PowerMgr::PowerMgrClient::GetInstance().IsScreenOn();
#endif
    isScreenOn_.store(isScreenOn);

    EventFwk::MatchingSkills matchingSkills;
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_CHARGING);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_DISCHARGING);
    matchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(matchingSkills);
    auto subscriber = std::make_shared<AppWakeupEventSubscriber>(subscribeInfo,
        [this](const EventFwk::CommonEventData &data) { OnCommonEvent(data); });
    bool isSubscribed = EventFwk::CommonEventManager::SubscribeCommonEvent(subscriber);
    if (!isSubscribed) {
        isInited_.store(false);  // allow retry on the next record
        return;
    }
    std::lock_guard<std::mutex> lock(subscriberMutex_);
    subscriber_ = subscriber;
}

void NearlinkAppWakeupUe::EnsureReportTimerStarted(void)
{
    std::lock_guard<std::mutex> lock(reportTimerMutex_);
    if (isTimerStarted_.load()) {
        return;
    }
    // keep the flag and the timer assignment inside the lock, so a concurrent stop cannot
    // miss the timer; starting the timer does not invoke the callback synchronously
    isTimerStarted_.store(true);
    auto timer = std::make_shared<NearlinkTimer>([this]() { Report(); });
    if (!timer->Start(REPORT_INTERVAL_MS, true)) {
        isTimerStarted_.store(false);
        return;
    }
    reportTimer_ = timer;
}

void NearlinkAppWakeupUe::StopReportTimer(void)
{
    std::lock_guard<std::mutex> lock(reportTimerMutex_);
    if (!isTimerStarted_.load()) {
        return;
    }
    isTimerStarted_.store(false);
    if (reportTimer_ != nullptr) {
        (void)reportTimer_->Stop();
        // release the timer now, the timer manager may be destructed before this singleton member
        reportTimer_ = nullptr;
    }
}

void NearlinkAppWakeupUe::OnCommonEvent(const EventFwk::CommonEventData &data)
{
    const std::string action = data.GetWant().GetAction();
    if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_ON) {
        isScreenOn_.store(true);
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_SCREEN_OFF) {
        isScreenOn_.store(false);
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_CHARGING) {
        isCharging_.store(true);
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_DISCHARGING) {
        isCharging_.store(false);
    } else if (action == EventFwk::CommonEventSupport::COMMON_EVENT_BATTERY_CHANGED) {
        // default to charging when the parameter is missing to keep the conservative behavior
        int32_t chargeState = data.GetWant().GetIntParam(BATTERY_CHARGE_STATE_KEY, BATTERY_CHARGE_STATE_ENABLE);
        isCharging_.store(chargeState == BATTERY_CHARGE_STATE_ENABLE);
    }
}

void NearlinkAppWakeupUe::Report(void)
{
    // always stop the timer after this round, the next record restarts it
    StopReportTimer();
    std::vector<WakeupRecord> records;
    // take away all records atomically, so the list is cleared for the next statistic period
    records_.FindAndRmv([&records](const WakeupRecord &record) {
        records.push_back(record);
        return true;
    });
    if (records.empty()) {
        return;
    }
    if (records.size() > MAX_RECORD_NUM) {
        records.resize(MAX_RECORD_NUM);
    }
    if (!DftHiviewUeStateIsOn()) {
        HILOGD("hiview ue off, drop %{public}zu records", records.size());
        return;
    }
    // report even if the screen is on or charging now, the records are still valid
    WriteEvent(records);
}

void NearlinkAppWakeupUe::WriteEvent(const std::vector<WakeupRecord> &records)
{
    std::vector<std::string> appNames;
    std::vector<const char *> appNamePtrs;
    std::vector<int64_t> uids;
    std::vector<int64_t> pids;
    std::vector<int64_t> businessTypes;
    std::vector<int64_t> triggerCounts;
    appNames.reserve(records.size());
    appNamePtrs.reserve(records.size());
    uids.reserve(records.size());
    pids.reserve(records.size());
    businessTypes.reserve(records.size());
    triggerCounts.reserve(records.size());
    for (const auto &record : records) {
        appNames.push_back(record.appName);
        uids.push_back(record.uid);
        pids.push_back(record.pid);
        businessTypes.push_back(record.businessType);
        triggerCounts.push_back(record.triggerCount);
    }
    for (const auto &name : appNames) {
        appNamePtrs.push_back(name.c_str());
    }

    HiSysEventParam params[PARAM_NUM];
    (void)memset_s(params, sizeof(params), 0, sizeof(params));
    InitArrayParam(params[0], PARAM_APP_NAME, HISYSEVENT_STRING_ARRAY, appNamePtrs.data(), appNamePtrs.size());
    InitArrayParam(params[1], PARAM_APP_UID, HISYSEVENT_INT64_ARRAY, uids.data(), uids.size());
    InitArrayParam(params[2], PARAM_APP_PID, HISYSEVENT_INT64_ARRAY, pids.data(), pids.size());
    InitArrayParam(params[3], PARAM_BUSINESS_TYPE, HISYSEVENT_INT64_ARRAY, businessTypes.data(),
        businessTypes.size());
    InitArrayParam(params[4], PARAM_TRIGGER_COUNT, HISYSEVENT_INT64_ARRAY, triggerCounts.data(),
        triggerCounts.size());
    int32_t ret = OH_HiSysEvent_Write(DOMAIN_NAME.c_str(), EVENT_NAME.c_str(), HISYSEVENT_BEHAVIOR, params,
        PARAM_NUM);
    if (ret != 0) {
        HILOGE("write app wakeup ue event failed, ret: %{public}d, records: %{public}zu", ret, records.size());
    }
}
}  // namespace Nearlink
}  // namespace OHOS
