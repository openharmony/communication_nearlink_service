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

#include "ServiceManagerPluginLoader.h"
#include "SleAdapterWrapper.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

ServiceManagerPluginLoader::ServiceManagerPluginLoader()
    : loader_(DEFAULT_LIB_NAME, DEFAULT_LIB_CREATE_FUNC_NAME, DEFAULT_LIB_DESTROY_FUNC_NAME),
      sleAdapterWrapper_(std::make_unique<SleAdapterWrapper>())
{
    HILOGI("[ServiceManagerPluginLoader Mocker] Constructor");
}

ServiceManagerPluginLoader* ServiceManagerPluginLoader::GetInstance(void)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] GetInstance");
    static ServiceManagerPluginLoader instance;
    return &instance;
}

void ServiceManagerPluginLoader::Init()
{
    HILOGI("[ServiceManagerPluginLoader Mocker] Init");
}

void ServiceManagerPluginLoader::DeInit()
{
    HILOGI("[ServiceManagerPluginLoader Mocker] DeInit");
}

void ServiceManagerPluginLoader::LoadPluginInterfaceLib()
{
    HILOGI("[ServiceManagerPluginLoader Mocker] LoadPluginInterfaceLib");
}

bool ServiceManagerPluginLoader::IsLibraryLoaded(void)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] IsLibraryLoaded, return true");
    return true;
}

void ServiceManagerPluginLoader::HighPowerProc(uint16_t lcid)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] HighPowerProc, lcid=%{public}u", lcid);
}

void ServiceManagerPluginLoader::SleTvMgrProc(const std::string &address)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] SleTvMgrProc, address=%{public}s", address.c_str());
}

void ServiceManagerPluginLoader::UpdateSleFreqBandAbility(const std::string &address)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] UpdateSleFreqBandAbility, address=%{public}s", address.c_str());
}

void ServiceManagerPluginLoader::CollaborationProc(CollaborationProcType type)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] CollaborationProc, type=%{public}d", static_cast<int>(type));
}

}  // namespace Nearlink
}  // namespace OHOS
