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

// ProfileServiceManager 整替 stub：裸进程下真实管理器不创建任何 profile service
//（adapter 为空），本 stub 与该行为等价；被测对象经各自夹具直接构造。
#include "ProfileServiceManager.h"

#include "log.h"

namespace OHOS {
namespace Nearlink {

SleInterfaceProfileManager &SleInterfaceProfileManager::GetInstance()
{
    return ProfileServiceManager::GetInstance();
}

ProfileServiceManager &ProfileServiceManager::GetInstance()
{
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
    HILOGI("[ProfileServiceManager Mocker] Start");
}

void ProfileServiceManager::CreateSleProfileServices() const
{
    HILOGI("[ProfileServiceManager Mocker] CreateSleProfileServices");
}

void ProfileServiceManager::Stop() const
{
    HILOGI("[ProfileServiceManager Mocker] Stop");
}

SleInterfaceProfile *ProfileServiceManager::GetProfileService(const std::string &name) const
{
    (void)name;
    return nullptr;
}

bool ProfileServiceManager::Enable(const SleTransport transport) const
{
    (void)transport;
    HILOGI("[ProfileServiceManager Mocker] Enable");
    return true;
}

void ProfileServiceManager::OnAllEnabled(const SleTransport transport) const
{
    (void)transport;
}

bool ProfileServiceManager::IsAllEnabled(const SleTransport transport) const
{
    (void)transport;
    return true;
}

void ProfileServiceManager::EnableProfiles(const SleTransport transport) const
{
    (void)transport;
}

void ProfileServiceManager::OnEnable(const std::string &name, bool ret) const
{
    (void)name;
    (void)ret;
}

void ProfileServiceManager::EnableCompleteProcess(const std::string &name, bool ret) const
{
    (void)name;
    (void)ret;
}

bool ProfileServiceManager::IsProfilesTurning(const SleTransport transport) const
{
    (void)transport;
    return false;
}

void ProfileServiceManager::EnableCompleteNotify(const SleTransport transport) const
{
    (void)transport;
}

bool ProfileServiceManager::Disable(const SleTransport transport) const
{
    (void)transport;
    HILOGI("[ProfileServiceManager Mocker] Disable");
    return true;
}

void ProfileServiceManager::OnAllDisabled(const SleTransport transport) const
{
    (void)transport;
}

bool ProfileServiceManager::IsAllDisabled(const SleTransport transport) const
{
    (void)transport;
    return true;
}

void ProfileServiceManager::DisableProfiles(const SleTransport transport) const
{
    (void)transport;
}

void ProfileServiceManager::OnDisable(const std::string &name, bool ret) const
{
    (void)name;
    (void)ret;
}

void ProfileServiceManager::DisableCompleteProcess(const std::string &name, bool ret) const
{
    (void)name;
    (void)ret;
}

void ProfileServiceManager::DisableCompleteNotify(const SleTransport transport) const
{
    (void)transport;
}

void ProfileServiceManager::CheckWaitEnableProfiles(const std::string &name, const SleTransport transport) const
{
    (void)name;
    (void)transport;
}

void ProfileServiceManager::GetProfileServicesSupportedUuids(std::vector<std::string> &uuids) const
{
    (void)uuids;
}

std::vector<uint32_t> ProfileServiceManager::GetProfileServicesList() const
{
    return {};
}

void ProfileServiceManager::SetSystemSupportProfileServices()
{}

std::unordered_set<SleUuid> ProfileServiceManager::GetSystemSupportProfileServices() const
{
    return std::unordered_set<SleUuid>();
}

SleConnectState ProfileServiceManager::GetProfileServiceConnectState(const uint32_t profileID) const
{
    (void)profileID;
    return SleConnectState::DISCONNECTED;
}

SleConnectState ProfileServiceManager::GetProfileServicesConnectState() const
{
    return SleConnectState::DISCONNECTED;
}
}  // namespace Nearlink
}  // namespace OHOS
