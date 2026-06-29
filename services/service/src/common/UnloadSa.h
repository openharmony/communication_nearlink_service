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

#ifndef UNLOAD_SA_H
#define UNLOAD_SA_H

#include "nearlink_timer.h"
#include "nearlink_def.h"
#include <atomic>

namespace OHOS {
namespace Nearlink {

class UnloadSa {
public:
    /**
     * @brief Get unload sa singleton instance pointer.
     *
     * @return Returns the singleton instance pointer.
     * @since 6
     */
    static UnloadSa& GetInstance();

    /**
     * @brief Unload SystemAblity.
     *
     * @since 6
     */
    void UnloadNearlinkSa();

    /**
     * @brief start unload sa timer.
     *
     * @since 6
     */
    void StartUnloadNearlinkSaTimer(const SleStateID state);

    /**
     * @brief stop unload sa timer.
     *
     * @since 6
     */
    void StopUnloadNearlinkSaTimer();

private:
    UnloadSa();
    ~UnloadSa();
    std::shared_ptr<NearlinkTimer> unloadSaTimer_;
    std::atomic_bool isTimerStarted_ = false;
};
} // namespace Nearlink
} // namespace OHOS

#endif /* UNLOAD_SA_H */