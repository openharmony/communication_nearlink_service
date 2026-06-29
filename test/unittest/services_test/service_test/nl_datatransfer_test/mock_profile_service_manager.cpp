/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "ProfileServiceManager.h"

#include <algorithm>

#include "nearlink_def.h"
#include "log.h"

#include "parameters.h"
#include "SleServiceManager.h"
#include "ClassCreator.h"
#include "ProfileInfo.h"
#include "profile_list.h"
#include "ThreadUtil.h"
#include "SleServiceFfrtLog.h"
#include "PortService.h"

namespace OHOS {
namespace Nearlink {

SleInterfaceProfileManager &SleInterfaceProfileManager::GetInstance()
{
    return ProfileServiceManager::GetInstance();
}

ProfileServiceManager &ProfileServiceManager::GetInstance()
{
    // C++11 static local variable initialization is thread-safe.
    static ProfileServiceManager instance;
    return instance;
}

void ProfileServiceManager::Initialize()
{
    GetInstance().Start();
}

void ProfileServiceManager::Uninitialize()
{
    GetInstance().Stop();
}

struct ProfileServiceManager::impl {
    impl() {}
    SLE_DISALLOW_COPY_AND_ASSIGN(impl);
};

ProfileServiceManager::ProfileServiceManager()
{
}

ProfileServiceManager::~ProfileServiceManager()
{}

void ProfileServiceManager::Start() const
{
    LOG_INFO("start");
}

void ProfileServiceManager::CreateSleProfileServices() const
{
    LOG_INFO("start");
}

void ProfileServiceManager::Stop() const
{
}

SleInterfaceProfile *ProfileServiceManager::GetProfileService(const std::string &name) const
{
    LOG_INFO("[Mock Profile] GetProfileService");
    SleInterfaceProfile *profile = nullptr;
    if (name == PROFILE_NAME_PORT) {
        LOG_INFO("[Mock Profile] port profile");
        static PortService portSerivce;
        return &portSerivce;
    }
    return nullptr;
}

bool ProfileServiceManager::Enable(const SleTransport transport) const
{
    return true;
}

void ProfileServiceManager::OnAllEnabled(const SleTransport transport) const
{
    LOG_INFO("%{public}d", transport);
}

bool ProfileServiceManager::IsAllEnabled(const SleTransport transport) const
{
    return true;
}

void ProfileServiceManager::EnableProfiles(const SleTransport transport) const
{}

void ProfileServiceManager::OnEnable(const std::string &name, bool ret) const
{}

void ProfileServiceManager::EnableCompleteProcess(const std::string &name, bool ret) const
{}

bool ProfileServiceManager::IsProfilesTurning(const SleTransport transport) const
{
    return false;
}

void ProfileServiceManager::EnableCompleteNotify(const SleTransport transport) const
{}

bool ProfileServiceManager::Disable(const SleTransport transport) const
{
    return true;
}

void ProfileServiceManager::OnAllDisabled(const SleTransport transport) const
{}

bool ProfileServiceManager::IsAllDisabled(const SleTransport transport) const
{
    return true;
}

void ProfileServiceManager::DisableProfiles(const SleTransport transport) const
{}

void ProfileServiceManager::OnDisable(const std::string &name, bool ret) const
{}

void ProfileServiceManager::DisableCompleteProcess(const std::string &name, bool ret) const
{}

void ProfileServiceManager::DisableCompleteNotify(const SleTransport transport) const
{}

void ProfileServiceManager::CheckWaitEnableProfiles(const std::string &name, const SleTransport transport) const
{}

void ProfileServiceManager::GetProfileServicesSupportedUuids(std::vector<std::string> &uuids) const
{}

std::vector<uint32_t> ProfileServiceManager::GetProfileServicesList() const
{
    std::vector<uint32_t> profileServicesList;
    return profileServicesList;
}

void ProfileServiceManager::SetSystemSupportProfileServices()
{}

std::unordered_set<SleUuid> ProfileServiceManager::GetSystemSupportProfileServices() const
{
    return std::unordered_set<SleUuid>();
}

SleConnectState ProfileServiceManager::GetProfileServiceConnectState(const uint32_t profileID) const
{
    return SleConnectState::DISCONNECTED;
}

SleConnectState ProfileServiceManager::GetProfileServicesConnectState() const
{
    return SleConnectState::DISCONNECTED;
}
}  // namespace Sle
}  // namespace OHOS