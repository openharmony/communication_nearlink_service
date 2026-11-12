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

#include "UnloadSa.h"

#include <atomic>

namespace OHOS {
namespace Nearlink {

// 卸载系统能力与15秒定时器被阻断，仅留调用计数供断言；真实实现见 src/common/UnloadSa.cpp
std::atomic<int> g_unloadSaCallCount = 0;

UnloadSa &UnloadSa::GetInstance()
{
    static UnloadSa instance;
    return instance;
}

UnloadSa::UnloadSa() : isTimerStarted_(false) {}

UnloadSa::~UnloadSa() = default;

void UnloadSa::UnloadNearlinkSa()
{
    g_unloadSaCallCount++;
}

void UnloadSa::StartUnloadNearlinkSaTimer(const SleStateID state) {}

void UnloadSa::StopUnloadNearlinkSaTimer() {}

void UnloadSa::NearlinkHostExtOnIdle() {}

void UnloadSa::NearlinkHostExtOnIdleInner() {}
}  // namespace Nearlink
}  // namespace OHOS
