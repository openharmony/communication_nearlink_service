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

#include "nearlink_verification_manager.h"
#include "DynamicLibraryLoader.h"
#include <functional>
#include <string>
#include "log.h"

namespace OHOS {
namespace Nearlink {

namespace {
const char* const VERIFICATION_EXT_LIB_NAME = "libnearlink_verification.z.so";
const char* const LOAD_STRATEGY_FUNC_NAME = "LoadVerificationStrategies";
}

NearlinkVerificationManager::NearlinkVerificationManager()
{
    defaultReturnMap_.Insert(VerificationType::CLOUD_PAIR_CHECK, true);
    defaultReturnMap_.Insert(VerificationType::SCAN_RSSI_FILTER, true);
    defaultReturnMap_.Insert(VerificationType::SCAN_RESULT_FILTER, false);
    defaultReturnMap_.Insert(VerificationType::SCAN_MATCH_FILTER, true);
    defaultReturnMap_.Insert(VerificationType::SWITCH_CONTROL, false);
    defaultReturnMap_.Insert(VerificationType::CONTROLLER_5G, true);
    defaultReturnMap_.Insert(VerificationType::CONTROLLER_HIGH_POWER, true);
    defaultReturnMap_.Insert(VerificationType::CONTROLLER_COEX, true);
    defaultReturnMap_.Insert(VerificationType::CONTROLLER_CHIP_LOG, true);
    defaultReturnMap_.Insert(VerificationType::CONTROLLER_BT_ADDR, true);
    defaultReturnMap_.Insert(VerificationType::DATATRANSFER_PROXY, false);
}

NearlinkVerificationManager::~NearlinkVerificationManager() = default;

void NearlinkVerificationManager::LoadStrategies()
{
    CDynamicLibraryLoader loader(VERIFICATION_EXT_LIB_NAME);
    if (!loader.IsLibraryLoaded()) {
        loader.OpenLib();
        HILOGI("[VERIFICATION_EXT] load verification_ext lib");
    }

    auto loadFn = reinterpret_cast<void(*)()>(loader.GetSymbol(LOAD_STRATEGY_FUNC_NAME));
    if (loadFn != nullptr) {
        loadFn();
        HILOGI("[VERIFICATION_EXT] strategies loaded, count: %{public}zu", strategies_.Size());
    } else {
        HILOGE("[VERIFICATION_EXT] LoadVerificationStrategies not found");
    }
}

NearlinkVerificationManager& NearlinkVerificationManager::GetInstance()
{
    static NearlinkVerificationManager instance;
    return instance;
}

bool NearlinkVerificationManager::CheckVerification(VerificationType type, const VerificationContext& ctx)
{
    bool result = false;
    bool found = strategies_.Find([&type, &ctx, &result](const std::shared_ptr<IVerificationStrategy>& strategy) {
        if (strategy != nullptr && strategy->GetType() == type) {
            result = strategy->Check(ctx);
            return true;
        }
        return false;
    });

    if (!found) {
        (void)defaultReturnMap_.GetValue(type, result);
    }
    return result;
}

void NearlinkVerificationManager::RegisterStrategy(std::shared_ptr<IVerificationStrategy> strategy)
{
    if (strategy != nullptr) {
        strategies_.Insert(strategy);
    }
}

void NearlinkVerificationManager::ClearStrategies()
{
    strategies_.Clear();
}

} // namespace Nearlink
} // namespace OHOS