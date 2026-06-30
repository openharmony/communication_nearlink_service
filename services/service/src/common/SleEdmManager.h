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

#ifndef SLE_EDM_MANAGER_H
#define SLE_EDM_MANAGER_H

#include <string>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include "nearlink_safe_hashmap.h"
#include "SleCommonEventSubscriber.h"

namespace OHOS {
namespace Nearlink {
const std::string ALLOW_PROTOCOL_TYPE_SSAP = "SSAP";
const std::string ALLOW_PROTOCOL_TYPE_DATA_TRANSFER = "DATA_TRANSFER";

/**
 * @brief  Represents nearlink allowlist manager.
 */
class SleEdmManager : public std::enable_shared_from_this<SleEdmManager> {
public:
    /**
     * @brief Get nearlink allowlist manager singleton instance pointer.
     *
     * @return Returns the singleton instance pointer.
     * @since 6
     */
    static std::shared_ptr<SleEdmManager> GetInstance();
    bool IsAllowedConnect(const std::string &profile);
    void OnEdmListChanged();
    void Init();
    SleEdmManager() = default;
    ~SleEdmManager() = default;
private:
    std::shared_ptr<SleCommonEventSubscriber> accountDisallowedEventSubscriber_ = nullptr;

    NearlinkSafeHashMap<std::string, std::vector<std::string>> accountForbiddenlist_;
    bool UpdateAccountForbiddenlist();

    std::atomic_bool isInit_ = false;
};
} // namespace Nearlink
} // namespace OHOS

#endif // SLE_EDM_MANAGER_H