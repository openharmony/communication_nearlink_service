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

namespace OHOS {
namespace Nearlink {
class ProfileServicesContextCallback : public utility::IContextCallback {
public:
    explicit ProfileServicesContextCallback(ProfileServiceManager &psm) : psm_(psm){};
    ~ProfileServicesContextCallback() = default;

    void OnEnable(const std::string &name, bool ret)
    {
        LOG_INFO("name=%{public}s, ret=%{public}d\n", name.c_str(), ret);
        psm_.OnEnable(name, ret);
    }

    void OnDisable(const std::string &name, bool ret)
    {
        LOG_INFO("name=%{public}s, ret=%{public}d\n", name.c_str(), ret);
        psm_.OnDisable(name, ret);
    }

private:
    ProfileServiceManager &psm_;
};

SleInterfaceProfileManager &SleInterfaceProfileManager::GetInstance()
{
    return ProfileServiceManager::GetInstance();
}

ProfileServiceManager &ProfileServiceManager::GetInstance()
{
    HILOGI("state_machine_test enter");
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

    std::unordered_set<SleUuid> systemSupportProfileServices;

    int calledCountEnable_ = 0;
    int calledCountDisable_ = 0;
    int calledCountOnAllEnabled_ = 0;
    int calledCountOnAllDisabled_ = 0;

    SLE_DISALLOW_COPY_AND_ASSIGN(impl);
};

ProfileServiceManager::ProfileServiceManager()
    : pimpl(std::make_unique<ProfileServiceManager::impl>())
{}

ProfileServiceManager::~ProfileServiceManager()
{}

void ProfileServiceManager::Start() const
{
    pimpl->calledCountEnable_ = 0;
    pimpl->calledCountDisable_ = 0;
    pimpl->calledCountOnAllEnabled_ = 0;
    pimpl->calledCountOnAllDisabled_ = 0;
}

void ProfileServiceManager::CreateSleProfileServices() const
{}

void ProfileServiceManager::Stop() const
{}

SleInterfaceProfile *ProfileServiceManager::GetProfileService(const std::string &name) const
{
    return nullptr;
}

bool ProfileServiceManager::Enable(const SleTransport transport) const
{
    HILOGI("[ProfileServiceManager Mocker] Enable");
    pimpl->calledCountEnable_++;
    return true;
}

void ProfileServiceManager::OnAllEnabled(const SleTransport transport) const
{
    HILOGI("[ProfileServiceManager Mocker] OnAllEnabled");
    pimpl->calledCountOnAllEnabled_++;
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
    return true;
}

void ProfileServiceManager::EnableCompleteNotify(const SleTransport transport) const
{}

bool ProfileServiceManager::Disable(const SleTransport transport) const
{
    HILOGI("[ProfileServiceManager Mocker] Disable");
    pimpl->calledCountDisable_++;
    return true;
}

void ProfileServiceManager::OnAllDisabled(const SleTransport transport) const
{
    HILOGI("[ProfileServiceManager Mocker] OnAllDisabled");
    pimpl->calledCountOnAllDisabled_++;
}

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
    return {};
}

void ProfileServiceManager::SetSystemSupportProfileServices()
{}

std::unordered_set<SleUuid> ProfileServiceManager::GetSystemSupportProfileServices() const
{
    return pimpl->systemSupportProfileServices;
}

SleConnectState ProfileServiceManager::GetProfileServiceConnectState(const uint32_t profileID) const
{
    return SleConnectState::DISCONNECTED;
}

SleConnectState ProfileServiceManager::GetProfileServicesConnectState() const
{
    return SleConnectState::DISCONNECTED;
}
}  // namespace Nearlink
}  // namespace OHOS