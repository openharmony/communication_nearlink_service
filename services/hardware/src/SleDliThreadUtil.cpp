/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "SleDliThreadUtil.h"

#include "datetime_ex.h"
#include "log.h"
#include "ffrt_inner.h"

static const std::string DLI_THREAD_NAME = "sle_dli";

struct SleDliThreadUtil::impl {
    struct ThreadTask {
        std::string name = "";
        std::atomic_bool trigger = false;
        ffrt::task_handle taskHandle = nullptr;
    };
    class TaskQueue {
    public:
        explicit TaskQueue(const char *name) : queue_(name) {}
        ~TaskQueue() = default;

        void PostTask(const ThreadUtilFunc &func);
        int GetQueueId(void);

    private:
        ffrt::queue queue_;
    };

    impl();
    ~impl() = default;
    std::shared_ptr<TaskQueue> CreateTaskQueue();

    std::shared_ptr<TaskQueue> taskQueue_;
};

SleDliThreadUtil::impl::impl()
{}

SleDliThreadUtil::SleDliThreadUtil() : pimpl(std::make_unique<impl>())
{
    std::lock_guard<ffrt::mutex> lock(taskQueueMutex_);
    threadState_ = ThreadState::ENABLED;
}

SleDliThreadUtil::~SleDliThreadUtil()
{
    std::lock_guard<ffrt::mutex> lock(taskQueueMutex_);
    threadState_ = ThreadState::DISABLED;
}

void SleDliThreadUtil::impl::TaskQueue::PostTask(
    const ThreadUtilFunc &func)
{
    queue_.submit(func);
}

int SleDliThreadUtil::impl::TaskQueue::GetQueueId(void)
{
    // Get the ffrt queue id, for debugging
    int id = -1;
    auto handle = queue_.submit_h([&id]() {
        id = ffrt::get_queue_id();
    });
    queue_.wait(handle);
    return id;
}

void SleDliThreadUtil::PostTask(const ThreadUtilFunc &func)
{
    std::lock_guard<ffrt::mutex> lock(taskQueueMutex_);
    // Check thread state for unit test.
    NL_CHECK_RETURN(threadState_ == ThreadState::ENABLED, "dli thread is no enabled");

    if (pimpl->taskQueue_ != nullptr) {
        pimpl->taskQueue_->PostTask(func);
        return;
    }
    // If the thread does not exist, create it.
    pimpl->taskQueue_ = pimpl->CreateTaskQueue();
    // Execute the first task.
    if (pimpl->taskQueue_) {
        pimpl->taskQueue_->PostTask(func);
    }
}

std::shared_ptr<SleDliThreadUtil::impl::TaskQueue> SleDliThreadUtil::impl::CreateTaskQueue()
{
    std::string threadName = DLI_THREAD_NAME;
    auto taskQueue = std::make_shared<TaskQueue>(threadName.c_str());
    NL_CHECK_RETURN_RET(taskQueue, nullptr, "Create %{public}s TaskQueue failed", threadName.c_str());

    int queueId = taskQueue->GetQueueId();
    HILOGI("sle_ffrt_queue: queueId(%{public}d),  name(%{public}s)", queueId, threadName.c_str());

    return taskQueue;
}

SleDliThreadUtil &SleDliThreadUtil::GetInstance()
{
    static SleDliThreadUtil instance;
    return instance;
}

// Only for test.
void SleDliThreadUtil::InitThread()
{
    HILOGI("enter");
    std::lock_guard<ffrt::mutex> lock(taskQueueMutex_);
    threadState_ = ThreadState::ENABLED;
}

// Only for test.
void SleDliThreadUtil::DestroyQueue()
{
    HILOGI("enter");
    std::lock_guard<ffrt::mutex> lock(taskQueueMutex_);
    threadState_ = ThreadState::DISABLED;
    pimpl->taskQueue_ = nullptr;
}