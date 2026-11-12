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

#include "iservice_registry.h"
#include "log.h"

namespace OHOS {
// Unit tests must not fire real samgr IPC (e.g. StartAbility system intents) on the device.
// Product paths that reach SystemAbilityManagerClient get no manager here and take their
// documented null-check error path instead. Symbol isolation follows the
// mock_ServiceManagerPluginLoader precedent: hidden visibility keeps these definitions
// inside the test binary only.
__attribute__((visibility("hidden"))) SystemAbilityManagerClient& SystemAbilityManagerClient::GetInstance()
{
    HILOGI("[SystemAbilityManagerClient Mocker] GetInstance");
    static SystemAbilityManagerClient instance;
    return instance;
}

__attribute__((visibility("hidden"))) sptr<ISystemAbilityManager> SystemAbilityManagerClient::GetSystemAbilityManager()
{
    HILOGI("[SystemAbilityManagerClient Mocker] GetSystemAbilityManager return nullptr");
    return nullptr;
}

__attribute__((visibility("hidden"))) void SystemAbilityManagerClient::DestroySystemAbilityManagerObject()
{
    HILOGI("[SystemAbilityManagerClient Mocker] DestroySystemAbilityManagerObject");
}
} // namespace OHOS
