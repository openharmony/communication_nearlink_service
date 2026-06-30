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

#include "nearlink_timer.h"
#include <algorithm>
#include <cstring>
#include <future>
#include <map>
#include <memory>
#include <thread>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include "securec.h"
#include "log.h"
#include <cinttypes>
#include "qos.h"

namespace OHOS {
namespace Nearlink {

static const uint64_t NS_PER_MS_SLE = 1000000;
static const uint64_t NS_PER_SECOND_SLE = 1000000000;

class TimerManager {
public:
    static TimerManager &GetInstance();

    uint64_t CreateTimer(std::shared_ptr<NearlinkTimer::TimerInfo> timerInfo);
    void DestroyTimer(std::shared_ptr<NearlinkTimer::TimerInfo> timerInfo);

    bool StartTimer(std::shared_ptr<NearlinkTimer::TimerInfo> timerInfo);
    bool StopTimer(std::shared_ptr<NearlinkTimer::TimerInfo> timerInfo);

private:
    static const int maxEpollEvents = 128;
    TimerManager();
    ~TimerManager();

    void Initialize(std::promise<int> startPromise);
    void CloseEpollAndStopFD(int &epollFd, int &stopFd);
    void OnTimer(std::promise<int> startPromise);
    void OnCallback(std::shared_ptr<NearlinkTimer::TimerInfo> timerInfo) const;
    uint64_t TimespecToNanoseconds(struct timespec ts);

    int epollFd_ {-1};
    int stopFd_ {-1};
    std::mutex mutex_ {};
    std::map<uint64_t, std::shared_ptr<NearlinkTimer::TimerInfo>> registeredMap_;
    std::unique_ptr<std::thread> thread_ {};
    uint64_t currentId_ {0};
};

TimerManager &TimerManager::GetInstance()
{
    static TimerManager instance;
    return instance;
}

TimerManager::TimerManager()
{
    std::promise<int> startPromise;
    std::future<int> startFuture = startPromise.get_future();
    thread_ = std::make_unique<std::thread>(&TimerManager::OnTimer, this, std::move(startPromise));
    startFuture.wait();
}

TimerManager::~TimerManager()
{
    eventfd_write(stopFd_, 1);
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }

    if (epollFd_ != -1) {
        close(epollFd_);
    }
    if (stopFd_ != -1) {
        close(stopFd_);
    }
}

void TimerManager::CloseEpollAndStopFD(int &epollFd, int &stopFd)
{
    if (epollFd != -1) {
        close(epollFd);
        epollFd = -1;
    }
    if (stopFd != -1) {
        close(stopFd);
        stopFd = -1;
    }
}


void TimerManager::Initialize(std::promise<int> startPromise)
{
    epollFd_ = epoll_create1(EPOLL_CLOEXEC);
    stopFd_ = eventfd(0, 0);
    if ((epollFd_ == -1) || (stopFd_ == -1)) {
        LOG_ERROR("TimerManager: Create epoll failed!!");
        CloseEpollAndStopFD(epollFd_, stopFd_);
        startPromise.set_value(-1);
        return;
    }

    int ret;
    struct epoll_event event = {};

    event.data.ptr = nullptr;
    event.events = EPOLLIN | EPOLLWAKEUP;
    CHECK_EXCEPT_INTR(ret = epoll_ctl(epollFd_, EPOLL_CTL_ADD, stopFd_, &event));
    if (ret == -1) {
        LOG_ERROR("TimerManager: Epoll add event failed!!");
        startPromise.set_value(-1);
        return;
    }

    startPromise.set_value(0);
}

void TimerManager::OnCallback(std::shared_ptr<NearlinkTimer::TimerInfo> timerInfo) const
{
    if (timerInfo == nullptr) {
        return;
    }
    uint64_t num = 0;
    int ret = read(timerInfo->fd_, &num, sizeof(uint64_t));
    if (ret == static_cast<int>(sizeof(uint64_t))) {
        if (num > 1) {
            LOG_WARN("Timer has expired more than one time.");
        }
        timerInfo->OnTrigger();
    } else if (errno == EAGAIN) {
        LOG_INFO("Timer is stopped or reset before callback called.");
    } else {
        LOG_ERROR("Unknown error type.");
    }
}

void TimerManager::OnTimer(std::promise<int> startPromise)
{
    int ret = OHOS::QOS::SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
    if (ret != 0) {
        HILOGE("Increase thread priority failed, ret: %{public}d", ret);
    }
    Initialize(std::move(startPromise));

    struct epoll_event events[maxEpollEvents];
    for (;;) {
        int nfds = -1;
        CHECK_EXCEPT_INTR(nfds = epoll_wait(epollFd_, events, maxEpollEvents, -1));
        if (nfds == -1) {
            return;
        }

        for (int i = 0; i < nfds; ++i) {
            std::unique_lock<std::mutex> lock(mutex_);
            if (events[i].data.ptr == nullptr) {
                eventfd_t val;
                eventfd_read(this->stopFd_, &val);
                return;
            }
            NearlinkTimer::TimerInfo *timer = (NearlinkTimer::TimerInfo *)events[i].data.ptr;
            auto iter = registeredMap_.find(timer->timerId_);
            if (iter == registeredMap_.end()) {
                continue;
            }
            std::shared_ptr<NearlinkTimer::TimerInfo> smartPtr = iter->second;
            lock.unlock();

            OnCallback(smartPtr);

            std::lock_guard<std::mutex> itemLock(smartPtr->mutex_);
            if (smartPtr->isPeriodic_) {
                StartTimer(smartPtr);
            }
        }
    }

    OHOS::QOS::ResetThreadQos();
}

uint64_t TimerManager::CreateTimer(std::shared_ptr<NearlinkTimer::TimerInfo> timerInfo)
{
    if (timerInfo == nullptr || timerInfo->fd_ == -1) {
        return 0;
    }
    struct epoll_event event = {};
    event.data.ptr = timerInfo.get();
    event.events = EPOLLIN | EPOLLWAKEUP;

    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, timerInfo->fd_, &event) == -1) {
        LOG_ERROR("TimerManager add timer failed");
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (currentId_ == std::numeric_limits<uint64_t>::max()) {
        currentId_ = 0;
    }
    uint64_t timerId = (++currentId_);
    timerInfo->timerId_ = timerId;
    registeredMap_[timerId] = timerInfo;
    HILOGD("NlTimer, id: %{public}" PRIu64 "", timerId);
    return timerId;
}

void TimerManager::DestroyTimer(std::shared_ptr<NearlinkTimer::TimerInfo> timerInfo)
{
    if (timerInfo == nullptr || timerInfo->fd_ == -1) {
        return;
    }
    if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, timerInfo->fd_, nullptr) == -1) {
        LOG_ERROR("TimerManager remove timer failed");
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (registeredMap_.count(timerInfo->timerId_) != 0) {
        registeredMap_.erase(timerInfo->timerId_);
    }
    HILOGD("NlTimer, id: %{public}" PRIu64 "", timerInfo->timerId_);
}

bool TimerManager::StartTimer(std::shared_ptr<NearlinkTimer::TimerInfo> timerInfo)
{
    if (timerInfo == nullptr) {
        return false;
    }
    HILOGD("NlTimer, id: %{public}" PRIu64 ", timeout: %{public}" PRIu64 "", timerInfo->timerId_, timerInfo->ms_);
    struct itimerspec newValue = {{0, 0}, {0, 0}};

    timespec now{0, 0};
    NL_CHECK_RETURN_RET(clock_gettime(CLOCK_BOOTTIME_ALARM, &now) != -1, false, "Failed clock_gettime.");
    uint64_t whenNsec = TimespecToNanoseconds(timerInfo->when_) + timerInfo->ms_ * NS_PER_MS_SLE;
    uint64_t nowNsec = TimespecToNanoseconds(now);
    if (whenNsec <= nowNsec) {
        whenNsec = nowNsec + timerInfo->ms_ * NS_PER_MS_SLE;
    }
    timerInfo->when_.tv_sec = static_cast<time_t>(whenNsec / NS_PER_SECOND_SLE);
    timerInfo->when_.tv_nsec = static_cast<int>(whenNsec % NS_PER_SECOND_SLE);
    newValue.it_value = timerInfo->when_;

    NL_CHECK_RETURN_RET(timerfd_settime(timerInfo->fd_, TFD_TIMER_ABSTIME, &newValue, nullptr) != -1,
        false, "Failed in timerFd_settime");
    return true;
}

bool TimerManager::StopTimer(std::shared_ptr<NearlinkTimer::TimerInfo> timerInfo)
{
    if (timerInfo == nullptr) {
        return true;
    }
    HILOGD("NlTimer, id: %{public}" PRIu64 "", timerInfo->timerId_);
    struct itimerspec its;
    (void)memset_s(&its, sizeof(its), 0, sizeof(its));

    NL_CHECK_RETURN_RET(timerfd_settime(timerInfo->fd_, TFD_TIMER_ABSTIME, &its, nullptr) != -1,
        false, "Failed in timerFd_settime");
    return true;
}

uint64_t TimerManager::TimespecToNanoseconds(struct timespec ts)
{
    return (static_cast<uint64_t>(ts.tv_sec) * NS_PER_SECOND_SLE) + static_cast<uint64_t>(ts.tv_nsec);
}

NearlinkTimer::TimerInfo::TimerInfo()
{
    fd_ = timerfd_create(CLOCK_BOOTTIME_ALARM, 0);
    if (fd_ == -1) {
        LOG_ERROR("timerfd_create ERROR");
    }
}

NearlinkTimer::TimerInfo::~TimerInfo()
{
    NL_CHECK_RETURN(fd_ != -1, "invalid fd: %{public}d", fd_);
    if (close(fd_) == -1) {
        LOG_ERROR("NlTimer close timer fd: %{public}d failed", fd_);
    }
}

void NearlinkTimer::TimerInfo::OnTrigger()
{
    if (callback_ != nullptr) {
        callback_();
    }
}

void NearlinkTimer::TimerInfo::SetCallbackInfo(const std::function<void()> &callback)
{
    this->callback_ = callback;
}

NearlinkTimer::NearlinkTimer(const std::function<void()> &callback)
{
    timer_ = std::make_shared<TimerInfo>();
    timer_->SetCallbackInfo(callback);
    uint64_t timerId = TimerManager::GetInstance().CreateTimer(timer_);
    if (timerId == 0) {
        HILOGE("create timer failed");
    }
}

NearlinkTimer::~NearlinkTimer()
{
    TimerManager::GetInstance().StopTimer(timer_);
    TimerManager::GetInstance().DestroyTimer(timer_);
}

bool NearlinkTimer::Start(int ms, bool isPeriodic)
{
    NL_CHECK_RETURN_RET(ms > 0, false, "invalid ms: %{public}d", ms);
    Stop();
    std::lock_guard<std::mutex> lock(timer_->mutex_);
    if (timer_->timerId_ == 0) {
        return false;
    }
    NL_CHECK_RETURN_RET(clock_gettime(CLOCK_BOOTTIME_ALARM, &timer_->when_) != -1, false, "Failed clock_gettime.");
    timer_->isPeriodic_ = isPeriodic;
    timer_->ms_ = static_cast<uint64_t>(ms);
    bool ret = TimerManager::GetInstance().StartTimer(timer_);
    NL_CHECK_RETURN_RET(ret, false, "start timer failed");
    return true;
}

bool NearlinkTimer::Stop()
{
    std::lock_guard<std::mutex> lock(timer_->mutex_);
    if (timer_->timerId_ == 0) {
        return true;
    }
    bool ret = TimerManager::GetInstance().StopTimer(timer_);
    NL_CHECK_RETURN_RET(ret, false, "stop timer failed");
    timer_->when_ = {0, 0};
    timer_->isPeriodic_ = false;
    timer_->ms_ = 0;
    return true;
}
} // namespace Nearlink
} // OHOS