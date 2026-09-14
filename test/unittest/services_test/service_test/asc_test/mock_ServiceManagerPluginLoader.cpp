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

// 符号隔离：本文件定义的 mock 符号全部 hidden（不进动态符号表）
// 原因：libnearlink_service_impl(.so) 内部代码会动态解析 ServiceManagerPluginLoader::GetInstance
// 等符号，若被解析到 mock 版本，.so 启动链会拿到 mock 实例而崩溃（已验证）
// 白盒（同一可执行文件）对 hidden 符号的引用不受影响，仍走 mock 实例
#pragma GCC visibility push(hidden)

namespace OHOS {
namespace Nearlink {

ServiceManagerPluginLoader::ServiceManagerPluginLoader()
    : loader_(DEFAULT_LIB_NAME, DEFAULT_LIB_CREATE_FUNC_NAME, DEFAULT_LIB_DESTROY_FUNC_NAME),
      sleAdapterWrapper_(std::make_unique<SleAdapterWrapper>())
{
    HILOGI("[ServiceManagerPluginLoader Mocker] Constructor");
}

ServiceManagerPluginLoader::~ServiceManagerPluginLoader()
{
    HILOGI("[ServiceManagerPluginLoader Mocker] Destructor");
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

void ServiceManagerPluginLoader::SetAcbSubrate(
    bool &ret, const RawAddress &device, const SleAcbSubrateParam &subrateParam)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] SetAcbSubrate, address=%{public}s, onlySubrate=%{public}d, "
        "subrate=%{public}d", device.GetAddress().c_str(), subrateParam.onlySubrate, subrateParam.subrate);
    ret = true;
}

}  // namespace Nearlink
}  // namespace OHOS

#pragma GCC visibility pop
