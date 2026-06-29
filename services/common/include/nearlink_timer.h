/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef NEARLINK_TIMER_H
#define NEARLINK_TIMER_H

#include "BaseDef.h"
#include <mutex>
#include <functional>

namespace OHOS {
namespace Nearlink {

class NearlinkTimer {
public:
    class TimerInfo {
        public:
            TimerInfo();
            ~TimerInfo();
            void OnTrigger();
            void SetCallbackInfo(const std::function<void()> &callback);

        public:
            timespec when_ {0, 0};
            uint64_t timerId_ {0};
            int fd_ {-1};
            bool isPeriodic_ {false};
            uint64_t ms_ {0};
            std::mutex mutex_;

        private:
            std::function<void()> callback_ {nullptr};
    };

    explicit NearlinkTimer(const std::function<void()> &callback);
    ~NearlinkTimer();
    bool Start(int ms, bool isPeriodic = false);
    bool Stop();

private:
    std::shared_ptr<TimerInfo> timer_ {nullptr};

    friend class TimerManager;
    SLE_DISALLOW_COPY_AND_ASSIGN(NearlinkTimer);
};
} // namespace Nearlink
} // namespace OHOS

#endif // NEW_TIMER_H