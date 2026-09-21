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
#include "IServiceManagerPlugin.h"
#include "SleAdapterWrapper.h"
#include "log.h"

// 符号隔离：本文件定义的 mock 符号全部 hidden（不进动态符号表）
// 原因：libnearlink_service_impl(.so) 内部代码会动态解析 ServiceManagerPluginLoader::GetInstance
// 等符号，若被解析到 mock 版本，.so 启动链会拿到 mock 实例而崩溃（已验证）
// 白盒（同一可执行文件）对 hidden 符号的引用不受影响，仍走 mock 实例

namespace OHOS {
namespace Nearlink {

__attribute__((visibility("hidden")))
ServiceManagerPluginLoader::ServiceManagerPluginLoader()
    : loader_(DEFAULT_LIB_NAME, DEFAULT_LIB_CREATE_FUNC_NAME, DEFAULT_LIB_DESTROY_FUNC_NAME),
      sleAdapterWrapper_(std::make_unique<SleAdapterWrapper>())
{
    HILOGI("[ServiceManagerPluginLoader Mocker] Constructor");
}

__attribute__((visibility("hidden")))
ServiceManagerPluginLoader::~ServiceManagerPluginLoader()
{
    HILOGI("[ServiceManagerPluginLoader Mocker] Destructor");
}

__attribute__((visibility("hidden")))
ServiceManagerPluginLoader* ServiceManagerPluginLoader::GetInstance(void)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] GetInstance");
    static ServiceManagerPluginLoader instance;
    return &instance;
}

__attribute__((visibility("hidden")))
void ServiceManagerPluginLoader::Init()
{
    HILOGI("[ServiceManagerPluginLoader Mocker] Init");
}

__attribute__((visibility("hidden")))
void ServiceManagerPluginLoader::DeInit()
{
    HILOGI("[ServiceManagerPluginLoader Mocker] DeInit");
}

__attribute__((visibility("hidden")))
void ServiceManagerPluginLoader::LoadPluginInterfaceLib()
{
    HILOGI("[ServiceManagerPluginLoader Mocker] LoadPluginInterfaceLib");
}

__attribute__((visibility("hidden")))
bool ServiceManagerPluginLoader::IsLibraryLoaded(void)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] IsLibraryLoaded, return true");
    return true;
}

__attribute__((visibility("hidden")))
void ServiceManagerPluginLoader::HighPowerProc(uint16_t lcid)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] HighPowerProc, lcid=%{public}u", lcid);
}

__attribute__((visibility("hidden")))
void ServiceManagerPluginLoader::SleTvMgrProc(const std::string &address)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] SleTvMgrProc, address=%{public}s", address.c_str());
}

__attribute__((visibility("hidden")))
void ServiceManagerPluginLoader::UpdateSleFreqBandAbility(const std::string &address)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] UpdateSleFreqBandAbility, address=%{public}s", address.c_str());
}

__attribute__((visibility("hidden")))
void ServiceManagerPluginLoader::CollaborationProc(CollaborationProcType type)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] CollaborationProc, type=%{public}d", static_cast<int>(type));
}

__attribute__((visibility("hidden")))
void ServiceManagerPluginLoader::SetAcbSubrate(
    bool &ret, const RawAddress &device, const SleAcbSubrateParam &subrateParam)
{
    HILOGI("[ServiceManagerPluginLoader Mocker] SetAcbSubrate, address=%{public}s, onlySubrate=%{public}d, "
        "subrate=%{public}d", device.GetAddress().c_str(), subrateParam.onlySubrate, subrateParam.subrate);
    ret = true;
}


// Supplement mocks for methods referenced by other directly-compiled service
// sources (see test/utils/service_tdd.gni)，避免归档排除后符号缺失。
ServiceManagerPluginInterface *ServiceManagerPluginInterface::GetInstance()
{
    return nullptr;
}

void ServiceManagerPluginLoader::SetSleAdapterFunc(GetSleAdapterFunc func)
{}

void ServiceManagerPluginLoader::SetAdapterStateObserver(SetAdapterStateObserverFunc func)
{}

void ServiceManagerPluginLoader::SetPeripheralCallback(SetPeripheralCallbackFunc func)
{}

void ServiceManagerPluginLoader::PowerMgrProc()
{}

void ServiceManagerPluginLoader::HidDataStatisticsProc(const std::string &address)
{}

void ServiceManagerPluginLoader::IsNeedCustomParam(bool &isNeedCustomParam, int appearance, uint16_t interval)
{}

void ServiceManagerPluginLoader::UpdateCustomParam(uint16_t &intervalMin, uint16_t &intervalMax, int appearance)
{}

void ServiceManagerPluginLoader::SvcCmdProc(std::string cmd, int32_t fd, const std::vector<std::u16string> &args,
    int32_t &svcResult, std::string &info)
{}

void ServiceManagerPluginLoader::SleReconnectProc(bool &isNeedReconn, int acbConnState, const RawAddress &peerAddr,
    int reason, std::set<int> reasonList)
{}

void ServiceManagerPluginLoader::PeerDeviceTypeProc(PeerDeviceTypeProcType proctype, const RawAddress &device)
{}

void ServiceManagerPluginLoader::SetPowerModeProc(SetPowerModeProcType proctype)
{}

void ServiceManagerPluginLoader::RegisterCallbackExt(RegisterCallbackModule module, int32_t &result)
{}

void ServiceManagerPluginLoader::GetLocalVocieCallFrameFourAbility(bool &isSupport)
{}

void ServiceManagerPluginLoader::SetConnFrameType4Subrate(const RawAddress &device)
{}

void ServiceManagerPluginLoader::RejectSetSubrate(const RawAddress &device)
{}

void ServiceManagerPluginLoader::ControlAntennaFix(bool enable, AntennaFixScene scene)
{}

std::string ServiceManagerPluginLoader::GetBundleName(BundleNameType type)
{
    return "";
}

}  // namespace Nearlink
}  // namespace OHOS
