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

#ifndef OHOS_NEARLINK_NEARLINK_VERIFICATION_MANAGER_H
#define OHOS_NEARLINK_NEARLINK_VERIFICATION_MANAGER_H

#include <memory>

#include "i_verification_strategy.h"
#include "verification_type.h"
#include "verification_context.h"
#include "nearlink_safe_list.h"
#include "nearlink_safe_map.h"

namespace OHOS {
namespace Nearlink {

class NearlinkVerificationManager {
public:
    static NearlinkVerificationManager& GetInstance();
    bool CheckVerification(VerificationType type, const VerificationContext& ctx);
    void RegisterStrategy(std::shared_ptr<IVerificationStrategy> strategy);
    void LoadStrategies();
    void ClearStrategies();

private:
    NearlinkVerificationManager();
    ~NearlinkVerificationManager();

    NearlinkVerificationManager(const NearlinkVerificationManager&) = delete;
    NearlinkVerificationManager& operator=(const NearlinkVerificationManager&) = delete;

    NearlinkSafeList<std::shared_ptr<IVerificationStrategy>> strategies_;
    NearlinkSafeMap<VerificationType, bool> defaultReturnMap_;
};

} // namespace Nearlink
} // namespace OHOS

#endif // OHOS_NEARLINK_NEARLINK_VERIFICATION_MANAGER_H