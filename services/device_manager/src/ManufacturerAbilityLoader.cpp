/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "ManufacturerAbilityLoader.h"
#include "IManufacturerAbilityManager.h"
#include "log_util.h"

namespace OHOS {
namespace Nearlink {

ManufacturerAbilityLoader::ManufacturerAbilityLoader()
    : CxxDynamicLibraryLoader<IManufacturerAbilityManager>(
        DEFAULT_LIB_NAME, DEFAULT_UNLOAD_TIMER_MS,
        DEFAULT_LIB_CREATE_FUNC_NAME, DEFAULT_LIB_DESTROY_FUNC_NAME)
{}

ManufacturerAbilityLoader& ManufacturerAbilityLoader::GetInstance() {
    static ManufacturerAbilityLoader instance;
    return instance;
}

void ManufacturerAbilityLoader::Load() {
    if (!IsLibraryLoaded()) {
        OpenLib();
        HILOGI("[MANU_ABILITY] load manu_ability lib");
    }
    auto manager = GetLibInstance();
    NL_CHECK_RETURN(manager, "load failed");
    manager->Init();
}

void ManufacturerAbilityLoader::Unload() {
    auto manager = GetLibInstance();
    NL_CHECK_RETURN(manager, "unload failed");
    manager->UnInit();
    CloseLib();
}

std::array<uint8_t, SLE_MANU_ABILITY_LEN> ManufacturerAbilityLoader::GetLocalAbility() {
    auto manager = GetLibInstance();
    std::array<uint8_t, SLE_MANU_ABILITY_LEN> emptyArray = {0};
    NL_CHECK_RETURN_RET(manager, emptyArray, "failed");
    return manager->GetLocalAbility();
}

void ManufacturerAbilityLoader::FilterAbility(std::array<uint8_t, SLE_MANU_ABILITY_LEN>& ability) {
    auto manager = GetLibInstance();
    NL_CHECK_RETURN(manager, "failed");
    manager->FilterAbility(ability);
}

bool ManufacturerAbilityLoader::CheckAbility(uint8_t index) {
    auto manager = GetLibInstance();
    NL_CHECK_RETURN_RET(manager, false, "failed");
    return manager->CheckAbility(index);
}

bool ManufacturerAbilityLoader::GetAbilityValue(const RawAddress& device, uint8_t index) {
    auto manager = GetLibInstance();
    NL_CHECK_RETURN_RET(manager, false, "failed");
    return manager->GetAbilityValue(device, index);
}

int ManufacturerAbilityLoader::GetAbilityIndex(const std::string& abilityName) {
    auto manager = GetLibInstance();
    NL_CHECK_RETURN_RET(manager, -1, "failed");
    return manager->GetAbilityIndex(abilityName);
}

bool ManufacturerAbilityLoader::SetLocalAbility(uint8_t index, bool enabled) {
    auto manager = GetLibInstance();
    NL_CHECK_RETURN_RET(manager, false, "failed");
    return manager->SetLocalAbility(index, enabled);
}

} // namespace Nearlink
} // namespace OHOS
