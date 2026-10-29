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

#include "nearlink_dft_local_data.h"

#include <chrono>
#include <atomic>
#include <mutex>
#include "log.h"

#ifdef NEARLINK_TELEPHONY_EXIST
#include "core_service_client.h"
#include "network_state.h"
#include "string_ex.h"
#include "telephony_errors.h"
#include "telephony_observer.h"
#include "telephony_observer_client.h"
#endif

namespace OHOS {
namespace Nearlink {
namespace {
constexpr size_t MCC_LEN = 3; // plmnNumeric 前 3 位为 MCC
const std::string COUNTRY_INFO_SEPARATOR = "_";
constexpr int64_t FETCH_FAIL_COOLDOWN_MS = 30000; // 取数/订阅失败后 30s 重试冷却，避免频繁 IPC
// 冷却时间戳哨兵：0 表示未发生（与稳态时钟零点无关），判据对 0 豁免冷却
constexpr int64_t FAIL_TIME_NONE = 0;
// 本地返回码（非 telephony 错误码域）：本次未发起注册（停服/节流/未编译 telephony 依赖）
constexpr int32_t REGISTER_RET_NOT_ATTEMPTED = -1;

// 服务运行状态，注册与取数的同源判据（Start 置 true，Stop 置 false）
std::atomic<bool> g_serviceRunning = false;

#ifdef NEARLINK_TELEPHONY_EXIST
constexpr uint32_t COUNTRY_INFO_OBSERVER_MASK = Telephony::TelephonyObserverBroker::OBSERVER_MASK_SIM_STATE |
    Telephony::TelephonyObserverBroker::OBSERVER_MASK_NETWORK_STATE;
constexpr int32_t SLOT_ID_FOR_ALL = -1; // -1 监听全部卡槽，避免切卡后漏掉状态变化

// SIM/网络状态变化订阅者
class DftTelephonyStateObserver : public Telephony::TelephonyObserver {
public:
    void OnSimStateUpdated(int32_t slotId, Telephony::CardType type, Telephony::SimState state,
        Telephony::LockReason reason) override
    {
        DftLocalDataManager::GetInstance().RefreshCountryInfo();
    }

    void OnNetworkStateUpdated(int32_t slotId, const sptr<Telephony::NetworkState> &networkState) override
    {
        DftLocalDataManager::GetInstance().RefreshCountryInfo();
    }
};

// 持有观察者引用，保证回调期间对象存活
sptr<Telephony::TelephonyObserver> g_countryInfoObserver = nullptr;
// 订阅注册状态，供上报路径补注册
std::atomic<bool> g_countryInfoObserverRegistered = false;
// 只保护注册状态与节流判定，跨进程注册/取消在锁外执行，避免持锁等待 IPC
std::mutex g_observerMutex;
// 补注册节流时间戳，由 g_observerMutex 保护
int64_t g_lastRegisterTryTimeMs = FAIL_TIME_NONE;
// 注册在飞标记，由 g_observerMutex 保护；停服复位只清节流不清它，避免在飞注册跨 Stop→Start 并发两条
bool g_registerInFlight = false;
#endif
}

DftLocalDataManager &DftLocalDataManager::GetInstance()
{
    static DftLocalDataManager instance;
    return instance;
}

void DftLocalDataManager::SetServiceRunning(bool running)
{
    g_serviceRunning = running;
}

std::string DftLocalDataManager::GetCountryInfo(bool useCache)
{
    if (!g_serviceRunning) {
        return ""; // 停服后不再取数，避免把停服窗口的值写回缓存
    }
#ifdef NEARLINK_TELEPHONY_EXIST
    if (!g_countryInfoObserverRegistered) {
        RegisterCountryInfoObserver(); // 补注册失败不影响取数
    }
#endif
    uint64_t gen = 0;
    {
        std::lock_guard<std::mutex> lock(countryInfoMutex_);
        gen = gen_; // 先取代际再判运行态：停服复位后取到新代际，随后必观察到停服态
        if (!g_serviceRunning) {
            return ""; // 停服窗口内不再取数，避免把停服期间的值写回缓存
        }
        if (useCache && isLoaded_) {
            return countryInfo_;
        }
        if (useCache && lastFetchFailTime_ != FAIL_TIME_NONE &&
            GetCurrentTimeMs() - lastFetchFailTime_ < FETCH_FAIL_COOLDOWN_MS) {
            // 冷却期内回退，避免频繁触发 IPC
            return countryInfo_;
        }
    }
    // 取数不持锁：跨进程查询会阻塞回调刷新与其余上报入口
    std::string countryInfo;
    if (FetchCountryInfo(countryInfo)) {
        std::lock_guard<std::mutex> lock(countryInfoMutex_);
        if (gen == gen_ && (!useCache || !isLoaded_)) {
            // 代际校验拒绝停服复位的陈旧取数；useCache=false 稳态同样刷新缓存
            countryInfo_ = countryInfo;
            isLoaded_ = true;
            lastFetchFailTime_ = FAIL_TIME_NONE;
        }
        return countryInfo_;
    }
    std::lock_guard<std::mutex> lock(countryInfoMutex_);
    lastFetchFailTime_ = GetCurrentTimeMs();
    // 失败回退缓存
    return countryInfo_;
}

void DftLocalDataManager::RefreshCountryInfo()
{
    if (!g_serviceRunning) {
        return; // 停服后回调不再取数
    }
    uint64_t gen = 0;
    uint64_t seq = 0;
    {
        std::lock_guard<std::mutex> lock(countryInfoMutex_);
        gen = gen_; // 先取代际再判运行态（同上报路径，堵住停服窗口起飞）
        if (!g_serviceRunning) {
            return; // 停服窗口内不再取数
        }
        if (lastRefreshFailTime_ != FAIL_TIME_NONE &&
            GetCurrentTimeMs() - lastRefreshFailTime_ < FETCH_FAIL_COOLDOWN_MS) {
            return; // 冷却期内抑制重复跨进程查询
        }
        seq = ++refreshSeq_;
    }
    std::string countryInfo;
    if (FetchCountryInfo(countryInfo)) {
        std::lock_guard<std::mutex> lock(countryInfoMutex_);
        if (gen != gen_ || seq <= lastRefreshCommitSeq_) {
            return; // 停服复位或比已提交结果更旧的取数，丢弃
        }
        countryInfo_ = countryInfo;
        isLoaded_ = true;
        lastRefreshFailTime_ = FAIL_TIME_NONE;
        lastRefreshCommitSeq_ = seq;
        return;
    }
    std::lock_guard<std::mutex> lock(countryInfoMutex_);
    if (seq == refreshSeq_) { // 仅最后起飞的取数失败才写冷却，旧失败不污染已提交结果
        lastRefreshFailTime_ = GetCurrentTimeMs();
    }
}

int32_t DftLocalDataManager::RegisterCountryInfoObserver()
{
#ifdef NEARLINK_TELEPHONY_EXIST
    sptr<Telephony::TelephonyObserver> observer = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_observerMutex);
        if (!g_serviceRunning || g_registerInFlight) {
            return REGISTER_RET_NOT_ATTEMPTED; // 停服或在飞，本次不发起
        }
        if (g_countryInfoObserverRegistered) {
            return Telephony::TELEPHONY_SUCCESS;
        }
        if (g_lastRegisterTryTimeMs != FAIL_TIME_NONE &&
            GetCurrentTimeMs() - g_lastRegisterTryTimeMs < FETCH_FAIL_COOLDOWN_MS) {
            return REGISTER_RET_NOT_ATTEMPTED; // 节流期内不重复注册
        }
        g_lastRegisterTryTimeMs = GetCurrentTimeMs(); // 先落占用标记再放锁，避免重复注册
        g_registerInFlight = true;
        observer = new DftTelephonyStateObserver();
    }
    // isUpdate=true 注册后立即收到当前状态回调，完成缓存预热；跨进程注册在锁外执行
    int32_t ret = Telephony::TelephonyObserverClient::GetInstance().AddStateObserver(
        observer, SLOT_ID_FOR_ALL, COUNTRY_INFO_OBSERVER_MASK, true);
    bool needUndo = false;
    {
        std::lock_guard<std::mutex> lock(g_observerMutex);
        g_registerInFlight = false; // 注册已返回，在飞标记收尾
        if (ret != Telephony::TELEPHONY_SUCCESS) {
            HILOGE("AddStateObserver failed, ret=%{public}d", ret);
            return ret;
        }
        if (!g_serviceRunning) {
            needUndo = true; // 注册完成前服务已停止，撤销刚建立的订阅
        } else {
            g_countryInfoObserver = observer;
            g_countryInfoObserverRegistered = true;
        }
    }
    if (needUndo) {
        int32_t undoRet = Telephony::TelephonyObserverClient::GetInstance().RemoveStateObserver(
            SLOT_ID_FOR_ALL, COUNTRY_INFO_OBSERVER_MASK);
        if (undoRet != Telephony::TELEPHONY_SUCCESS) {
            HILOGE("undo RemoveStateObserver failed, ret=%{public}d", undoRet);
        }
    }
    return needUndo ? REGISTER_RET_NOT_ATTEMPTED : Telephony::TELEPHONY_SUCCESS;
#else
    return REGISTER_RET_NOT_ATTEMPTED;
#endif
}

void DftLocalDataManager::UnregisterCountryInfoObserver()
{
#ifdef NEARLINK_TELEPHONY_EXIST
    ResetCountryInfo(); // 复位缓存并递增代际，在飞取数提交时将被判为陈旧
    bool needRemove = false;
    {
        std::lock_guard<std::mutex> lock(g_observerMutex);
        g_lastRegisterTryTimeMs = FAIL_TIME_NONE; // 复位节流：立即重启可马上注册
        if (!g_countryInfoObserverRegistered) {
            return;
        }
        needRemove = true; // 先落取消标记再放锁，跨进程取消在锁外执行
    }
    int32_t ret = Telephony::TelephonyObserverClient::GetInstance().RemoveStateObserver(
        SLOT_ID_FOR_ALL, COUNTRY_INFO_OBSERVER_MASK);
    {
        std::lock_guard<std::mutex> lock(g_observerMutex);
        // 失败同样清本地状态：失败码无法区分对端是否仍保留注册（SA 重启后内存记录即丢失），
        // 保留状态会堵死重建通道。重建安全性的前提：对端记录存续期间，其持有的 proxy 远端引用
        // 经内核保持旧 broker 存活，回调仍经旧 broker 到达，刷新通道不中断；记录已失时对端按
        // slot/mask/pid 去重新建记录，不会产生重复订阅
        if (ret != Telephony::TELEPHONY_SUCCESS) {
            HILOGE("RemoveStateObserver failed, ret=%{public}d", ret);
        }
        g_countryInfoObserver = nullptr;
        g_countryInfoObserverRegistered = false;
    }
#endif
}

void DftLocalDataManager::ResetCountryInfo()
{
    std::lock_guard<std::mutex> lock(countryInfoMutex_);
    countryInfo_ = "";
    isLoaded_ = false;
    lastFetchFailTime_ = FAIL_TIME_NONE;
    lastRefreshFailTime_ = FAIL_TIME_NONE;
    ++gen_; // 代际递增：在飞取数的提交被拒绝
}

bool DftLocalDataManager::FetchCountryInfo(std::string &countryInfo)
{
#ifdef NEARLINK_TELEPHONY_EXIST
    int32_t slotId = -1;
    int32_t ret = Telephony::CoreServiceClient::GetInstance().GetPrimarySlotId(slotId);
    if (ret != Telephony::TELEPHONY_SUCCESS || slotId < 0) {
        HILOGE("GetPrimarySlotId failed, ret=%{public}d, slotId=%{public}d", ret, slotId);
        return false;
    }

    std::u16string countryCode16;
    ret = Telephony::CoreServiceClient::GetInstance().GetISOCountryCodeForSim(slotId, countryCode16);
    if (ret != Telephony::TELEPHONY_SUCCESS || countryCode16.empty()) {
        HILOGE("GetISOCountryCodeForSim failed, ret=%{public}d", ret);
        return false;
    }

    sptr<Telephony::NetworkState> networkState = nullptr;
    ret = Telephony::CoreServiceClient::GetInstance().GetNetworkState(slotId, networkState);
    if (ret != Telephony::TELEPHONY_SUCCESS || networkState == nullptr) {
        HILOGE("GetNetworkState failed, ret=%{public}d", ret);
        return false;
    }

    std::string plmnNumeric = networkState->GetPlmnNumeric();
    if (plmnNumeric.length() <= MCC_LEN) {
        HILOGE("plmnNumeric invalid, len=%{public}zu", plmnNumeric.length());
        return false;
    }

    countryInfo = Str16ToStr8(countryCode16) + COUNTRY_INFO_SEPARATOR + plmnNumeric.substr(0, MCC_LEN) +
        COUNTRY_INFO_SEPARATOR + plmnNumeric.substr(MCC_LEN);
    return true;
#else
    return false;
#endif
}

int64_t DftLocalDataManager::GetCurrentTimeMs()
{
    // 单调时钟：系统时间回拨/前跳不影响冷却与节流计时
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

}  // namespace Nearlink
}  // namespace OHOS
