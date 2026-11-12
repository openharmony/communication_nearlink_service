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

// 测试目的：整替 ProfileServiceManager（沿用 nl_datatransfer_test / state_machine_test 的
// mock_profile_service_manager.cpp 模式），使 ASCService::GetService() 在测试环境下可用。
// 背景：asc_test 用 mock_sle_adapter.cpp 替换了 SleAdapter.cpp（无 REGISTER_CLASS_CREATOR 注册），
// SleServiceManager::Start() -> CreateAdapters() 得到 nullptr adapter，真实 ProfileServiceManager
// 不会创建任何 profile service，ASCService::GetService() 返回 nullptr，
// 首个调用用例 OnConnectionStateChanged_001 对空指针调 RegisterObserver 即 SIGSEGV。
// mock 使 GetProfileService(PROFILE_NAME_ASC) 返回静态 ASCService 实例
// （与 mock_cdsm_service.cpp 中 CdsmService::GetService() 返回静态实例做法一致），
// 其余 profile 名返回 nullptr，走产品码既有判空分支。

#include "ProfileServiceManager.h"

#include "log.h"

#include "ASCService.h"
#include "SleInterfaceProfile.h"
#include "TwsService.h"

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
    if (name == PROFILE_NAME_ASC) {
        // 惰性构造：构造仅向 stack 注册回调，与用例内 new ASCService() 的已验证路径一致
        static ASCService instance;
        return &instance;
    }
    if (name == PROFILE_NAME_TWS) {
        // 测试目的：SetMusicMuteWhenAudioRelease 的 vendor 耳机静音路径需要非空 TWS
        // profile（ASCService.cpp:6817-6819 判空即返回 NL_ERR_INTERNAL_ERROR，正是
        // SetMusicMuteWhenAudioRelease_001 末段期望 NL_NO_ERROR 实得 1009700099 的失败点）。
        // mock_tws_service.cpp 已提供静态实例，且 TwsGetDeviceAudioMusicType 对
        // coDeviceStr 返回 DISABLE（静音路径前置条件）。其余产品消费点
        // （IsLeftEarDevice/IsRolePrimary/SetDeviceRole）均在 IsVendorDevice 门后，
        // 仅显式 SetMockVendorAudioDevice 的用例会走到该实例。
        return TwsService::GetService();
    }
    return nullptr;
}

bool ProfileServiceManager::Enable(const SleTransport transport) const
{
    HILOGI("[ProfileServiceManager Mocker] Enable");
    return true;
}

void ProfileServiceManager::OnAllEnabled(const SleTransport transport) const
{
    HILOGI("[ProfileServiceManager Mocker] OnAllEnabled");
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
    HILOGI("[ProfileServiceManager Mocker] Disable");
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
    return SleConnectState::DISCONNECTED;
}

SleConnectState ProfileServiceManager::GetProfileServicesConnectState() const
{
    return SleConnectState::DISCONNECTED;
}
}  // namespace Nearlink
}  // namespace OHOS
