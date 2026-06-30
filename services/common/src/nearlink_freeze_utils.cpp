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

#include "nearlink_freeze_utils.h"
#include "log.h"
#include "iservice_registry.h"
#include "res_type.h"
#include "res_sched_client.h"
#include <mutex>

namespace OHOS {
namespace Nearlink {
namespace {
    constexpr int32_t DELAY_INTERVAL = 3 * 60 * 1000 * 1000;
    constexpr int32_t FIRST_APPLICATION_UID = 10000;
    constexpr int32_t RES_SCHED_SYS_ABILITY_ID = 1901;
    constexpr int32_t SUSPEND_MANAGER_SYSTEM_ABILITY_ID = 1910;
    const char* FROZEN_WITHOUT_TRANSFER = "FROZEN_WITHOUT_TRANSFER";
    const char* FROZEN_TASK_QUEUE = "FROZEN_NL_TASK_QUEUE";
    const char* FROZEN_TASK = "FROZEN_NL_TASK";
}

NearlinkFreezeUtil::~NearlinkFreezeUtil()
{
    if (ffrtQueue_) {
        ffrtQueue_.reset();
    }
}

NearlinkFreezeUtil *NearlinkFreezeUtil::GetInstance(void)
{
    static NearlinkFreezeUtil instance;
    return &instance;
}

void NearlinkFreezeUtil::InitQueueTask()
{
    ffrtQueue_ = std::make_shared<ffrt::queue>(FROZEN_TASK_QUEUE,
        ffrt::queue_attr().qos(ffrt::qos_user_interactive));
    if (!ffrtQueue_) {
        HILOGE("init frozen queue fail.");
        return;
    }
    SubscribeRssSystemAbility();
    ffrtQueue_->submit([this]() {
        CheckPeriodical();
    }, ffrt::task_attr().name(FROZEN_TASK).delay(DELAY_INTERVAL));
}

void NearlinkFreezeUtil::CheckPeriodical()
{
    SetFreezeState();
    if (!ffrtQueue_) {
        return;
    }
    ffrt_queue_t* queue = reinterpret_cast<ffrt_queue_t*>(ffrtQueue_.get());
    if (queue == nullptr) {
        return;
    }
    if (!ffrt_queue_has_task(*queue, FROZEN_TASK)) {
        ffrtQueue_->submit([this]() {
            CheckPeriodical();
        }, ffrt::task_attr().name(FROZEN_TASK).delay(DELAY_INTERVAL));
    }
}

void NearlinkFreezeUtil::SetFreezeState()
{
    std::vector<AppInfo> tmpReport;
    hasDataReport_.Iterate([&tmpReport, this](AppInfo appInfo, bool &isReported) {
        if (!isReported) {
            ReportNlDataToRss(appInfo.pid, appInfo.uid, false, appInfo.type, FROZEN_WITHOUT_TRANSFER);
            tmpReport.emplace_back(appInfo);
        } else {
            isReported = false;
        }
    });
    for (auto info : tmpReport) {
        hasDataReport_.Erase(info);
    }
}

void NearlinkFreezeUtil::RequestActive(
    const int32_t pid, const int32_t uid, const std::string dataType, const std::string &reason)
{
    if (uid < FIRST_APPLICATION_UID) {
        return;
    }
    AppInfo appInfo(pid, uid, dataType);
    if (IsNeedReport(appInfo)) {
        ReportNlDataToRss(pid, uid, true, dataType, reason);
        hasDataReport_.EnsureInsert(appInfo, true);
    }
    return;
}

bool NearlinkFreezeUtil::IsNeedReport(AppInfo appInfo)
{
    {
        std::unique_lock<std::mutex> lock(initMutex_);
        if (!ffrtQueue_) {
            InitQueueTask();
        }
    }
    
    bool needReport = true;
    auto func = [&needReport](const AppInfo key, bool value) -> void {
        needReport = value;
        return;
    };
    if (!hasDataReport_.GetValueAndOpt(appInfo, func) || !needReport) {
        HILOGI("nl request active pid: %{public}d, uid: %{public}d, type: %{public}s",
            appInfo.pid, appInfo.uid, appInfo.type.c_str());
        return true;
    }
    return false;
}

void NearlinkFreezeUtil::ReportNlDataToRss(
    const int32_t pid, const int32_t uid, const bool &isTransfer,
    const std::string &type, const std::string &reason)
{
    std::unordered_map<std::string, std::string> payload;
    payload["PID"] = std::to_string(pid);
    payload["UID"] = std::to_string(uid);
    payload["ISTRANSFER"] = isTransfer ? "true" : "false";
    payload["TYPE"] = type;
    payload["REASON"] = reason;
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(
        ResourceSchedule::ResType::RES_TYPE_NEARLINK_SERVICE_EVENT,
        ResourceSchedule::ResType::NlServiceEvent::NL_DATA_TRANSFER,
        payload);
}

void NearlinkFreezeUtil::ReportNlConnectStateToRss(
    const int32_t pid, const int32_t uid,
    const std::string &action, const std::string &type, const std::string &reason)
{
    if (uid < FIRST_APPLICATION_UID) {
        return;
    }
    AppInfo tempAppInfo(pid, uid, type);
    if (type == "SSAP") {
        bool needDisConnect = false;
        auto func = [&action, &needDisConnect](const AppInfo key, int &value) -> void {
            if(action == "CONNECT") {
                value++;
                return;
            } else {
                value--;
            }
            needDisConnect = (value == 0);
        };
        if (!ssapConnectState_.GetValueAndOpt(tempAppInfo, func)) {
            ssapConnectState_.Insert(tempAppInfo, 1);
        }
        if(!needDisConnect){
            return;
        }
    }
    std::unordered_map<std::string, std::string> payload;
    payload["PID"] = std::to_string(pid);
    payload["UID"] = std::to_string(uid);
    payload["ACTION"] = action;
    payload["TYPE"] = type;
    payload["REASON"] = reason;
    ResourceSchedule::ResSchedClient::GetInstance().ReportData(
        ResourceSchedule::ResType::RES_TYPE_NEARLINK_SERVICE_EVENT,
        ResourceSchedule::ResType::NlServiceEvent::NL_CONNECT_STATE,
        payload);
    if (action == "CONNECT") {
        RequestActive(pid, uid, type, reason); // 当有连接时，主动上报有数据
    }
}

void NearlinkFreezeUtil::SubscribeRssSystemAbility()
{
    sptr<ISystemAbilityManager> samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!samgrProxy) {
        HILOGE("failed to get samgrProxy");
        return;
    }
    if (statusChangeListener_) {
        HILOGE("has been subscribed");
        return;
    }
    const std::set<int> saIdSet = {RES_SCHED_SYS_ABILITY_ID, SUSPEND_MANAGER_SYSTEM_ABILITY_ID};
    auto addFunc = [this](int32_t systemAbilityId) {
        hasDataReport_.Iterate([this](AppInfo appInfo, bool &isReported) {
            if(isReported) {
                ReportNlDataToRss(appInfo.pid, appInfo.uid, true, appInfo.type.c_str(), "RSS SA die");
            }
            return;
        });
    };
    auto removeFunc = [](int32_t systemAbilityId) {
        return;
    };
    statusChangeListener_ = new NearlinkSystemAbilitySubscriber(saIdSet, addFunc, removeFunc);
    int32_t ret = samgrProxy->SubscribeSystemAbility(RES_SCHED_SYS_ABILITY_ID, statusChangeListener_);
    if (ret != ERR_OK) {
        HILOGE("subscribe systemAbilityId: call manager service failed!");
        statusChangeListener_ = nullptr;
        return;
    }
    ret = samgrProxy->SubscribeSystemAbility(SUSPEND_MANAGER_SYSTEM_ABILITY_ID, statusChangeListener_);
    if (ret != ERR_OK) {
        HILOGE("subscribe systemAbilityId: core service failed!");
        statusChangeListener_ = nullptr;
        return;
    }
}
}  // namespace Nearlink
}  // namespace OHOS