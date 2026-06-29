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
#include "SleInterfaceManager.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {

#define CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(func, ...) \
do { \
    auto servicePlugin = loader_.GetLibInstance(); \
    NL_CHECK_RETURN(servicePlugin, "service manager plugin lib load failed"); \
    servicePlugin->func(__VA_ARGS__); \
} while (0)

ServiceManagerPluginLoader::ServiceManagerPluginLoader()
    : loader_(DEFAULT_LIB_NAME, DEFAULT_LIB_CREATE_FUNC_NAME, DEFAULT_LIB_DESTROY_FUNC_NAME),
      sleAdapterWrapper_(std::make_unique<SleAdapterWrapper>())
{}

ServiceManagerPluginLoader::~ServiceManagerPluginLoader()
{}

void ServiceManagerPluginLoader::LoadPluginInterfaceLib()
{
    HILOGI("enter");
    loader_.OpenLib();
    auto servicePlugin = loader_.GetLibInstance();
    NL_CHECK_RETURN(servicePlugin, "Get service plugin interface failed");
    servicePlugin->SetSleAdapterFunc([this]() {
        return sleAdapterWrapper_.get();
    });

    servicePlugin->SetAdapterStateObserver([](IAdapterStateObserver &observer) {
        bool ret = SleInterfaceManager::GetInstance()->RegisterStateObserver(observer);
        HILOGI("RegisterStateObserver, ret(%{public}d)", ret);
    });
}

bool ServiceManagerPluginLoader::IsLibraryLoaded(void)
{
    return loader_.IsLibraryLoaded();
}

void ServiceManagerPluginLoader::SetSleAdapterFunc(GetSleAdapterFunc func)
{}

void ServiceManagerPluginLoader::SetAdapterStateObserver(SetAdapterStateObserverFunc func)
{}

void ServiceManagerPluginLoader::SetPeripheralCallback(SetPeripheralCallbackFunc func)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(SetPeripheralCallback, func);
}

ServiceManagerPluginInterface *ServiceManagerPluginInterface::GetInstance()
{
    return ServiceManagerPluginLoader::GetInstance();
}

bool ServiceManagerPluginInterface::IsLibraryLoaded(void)
{
    return ServiceManagerPluginLoader::GetInstance()->IsLibraryLoaded();
}

ServiceManagerPluginLoader* ServiceManagerPluginLoader::GetInstance(void)
{
    static ServiceManagerPluginLoader instance;
    return &instance;
}

void ServiceManagerPluginLoader::Init()
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(Init);
}

void ServiceManagerPluginLoader::DeInit()
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(DeInit);
}

/*
 * @brief Plugin模块开发指南
 *
 * 新增Plugin模块时，需要完成以下步骤：
 *
 * 加入顺序按照模块所在文件夹的字母顺序排列
 *
 * 1. IServiceManagerPlugin.h 新增接口声明
 *    - 如果同一模块需要调用多个函数，请在接口中通过枚举区分
 *      (如: SLE_POWER_MANAGER_MODULE)
 *    - 函数参数设计原则:
 *      - 如果需要出参, 请通过增加入参(函数指针或回调)带入
 *
 * 2. ServiceManagerPluginLoader.cpp 新增调用包装
 *    - 使用 CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED 宏调用
 *
 * 示例:
 * // IServiceManagerPlugin.h
 * enum PluginModuleType { SLE_POWER_MANAGER_MODULE = 0, SLE_TV_MANAGER_MODULE };
 * virtual void PluginModuleProc(PluginModuleType moduleType, int32_t param) = 0;
 *
 * // ServiceManagerPluginLoader.h
 * void PluginModuleProc(PluginModuleType moduleType, int32_t param) override;
 *
 * // ServiceManagerPluginLoader.cpp
 * void PluginModuleProc(PluginModuleType moduleType, int32_t param)
 * {
 *     CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(PluginModuleProc, moduleType, param);
 * }
 */

void ServiceManagerPluginLoader::PowerMgrProc()
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(PowerMgrProc);
}

void ServiceManagerPluginLoader::HighPowerProc(uint16_t lcid)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(HighPowerProc, lcid);
}

void ServiceManagerPluginLoader::SleTvMgrProc(const std::string &address)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(SleTvMgrProc, address);
}

void ServiceManagerPluginLoader::HidDataStatisticsProc(const std::string &address)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(HidDataStatisticsProc, address);
}

void ServiceManagerPluginLoader::RssiChangedCbkProc(const std::string &address, int8_t rssi)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(RssiChangedCbkProc, address, rssi);
}

void ServiceManagerPluginLoader::UpdateSleFreqBandAbility(const std::string &address)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(UpdateSleFreqBandAbility, address);
}

void ServiceManagerPluginLoader::CollaborationProc(CollaborationProcType type)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(CollaborationProc, type);
}

void ServiceManagerPluginLoader::SvcCmdProc(
    std::string cmd, int32_t fd, const std::vector<std::u16string> &args, int32_t &svcResult, std::string &info)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(SvcCmdProc, cmd, fd, args, svcResult, info);
}

void ServiceManagerPluginLoader::SleReconnectProc(bool &isNeedReconn, int acbConnState, const RawAddress &peerAddr,
    int reason, std::set<int> reasonList)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(SleReconnectProc, isNeedReconn, acbConnState, peerAddr,
        reason, reasonList);
}
// adapter模块函数 start
void ServiceManagerPluginLoader::PeerDeviceTypeProc(PeerDeviceTypeProcType proctype, const RawAddress &device)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(PeerDeviceTypeProc, proctype, device);
}

void ServiceManagerPluginLoader::RegisterCallbackExt(RegisterCallbackModule module, int32_t &result)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(RegisterCallbackExt, module, result);
}

void ServiceManagerPluginLoader::GetLocalVocieCallFrameFourAbility(bool &isSupport)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(GetLocalVocieCallFrameFourAbility, isSupport);
}

void ServiceManagerPluginLoader::SetPowerModeProc(SetPowerModeProcType proctype)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(SetPowerModeProc, proctype);
}
// adapter模块函数 end

// chiputil模块函数 start
void ServiceManagerPluginLoader::SetAcbSubrate(
    bool &ret, const RawAddress &device, const SleAcbSubrateParam &subrateParam)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(SetAcbSubrate, ret, device, subrateParam);
}

void ServiceManagerPluginLoader::SetConnFrameType4Subrate(const RawAddress &device)
{
    CALL_FUNC_IF_SERVICE_MANAGER_PLUGIN_LIB_LOADED(SetConnFrameType4Subrate, device);
}

// chiputil模块函数 end

std::string ServiceManagerPluginLoader::GetBundleName(BundleNameType type)
{
    auto servicePlugin = loader_.GetLibInstance();
    NL_CHECK_RETURN_RET(servicePlugin, "", "service manager plugin lib load failed");
    return servicePlugin->GetBundleName(type);
}

}  // namespace Nearlink
}  // namespace OHOS