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

#ifndef NEARLINK_TEST_ENABLE_WAIT_H
#define NEARLINK_TEST_ENABLE_WAIT_H

#include <chrono>
#include <thread>

#include "nearlink_host.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
// EnableNl 返回只代表服务受理，开关完全开启是异步完成的；轮询到位返回 true，超时返回 false
inline bool WaitForSleEnabled(int timeoutMs = 20000)
{
    for (int waitedMs = 0; !NearlinkHost::GetInstance().IsSleEnabled(); waitedMs += 500) {
        if (waitedMs >= timeoutMs) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return true;
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS

#endif // NEARLINK_TEST_ENABLE_WAIT_H
