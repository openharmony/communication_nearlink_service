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
#ifndef SLE_DLI_THREAD_UTIL_H
#define SLE_DLI_THREAD_UTIL_H

#include <functional>
#include "nearlink_safe_map.h" // SafeMap
#include "ffrt_inner.h"

using ThreadUtilFunc = std::function<void(void)>;

class SleDliThreadUtil {
public:
    void PostTask(const ThreadUtilFunc &func);
    void InitThread();
    void DestroyQueue();

    static SleDliThreadUtil &GetInstance();

private:
    enum ThreadState : int {
        ENABLED = 0,  // The task function is switched normally.
        DISABLED,  // The task function is not executed.
        NOT_SWITCH_THREAD,  // The task function is executed in the same thread.
    };

    ThreadState threadState_ = ThreadState::DISABLED;
    ffrt::mutex taskQueueMutex_{}; // Protects threadState_ and pimpl->taskQueue_.

    SleDliThreadUtil();
    ~SleDliThreadUtil();

    struct impl;
    std::unique_ptr<impl> pimpl;
};

#endif  // SLE_DLI_THREAD_UTIL_H