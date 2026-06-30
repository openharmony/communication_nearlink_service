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

#ifndef SERVICE_MANAGER_PLUGIN_LOADER
#define SERVICE_MANAGER_PLUGIN_LOADER

#include "DynamicLibraryLoader.h"
#include "ThreadUtil.h"
#include "IServiceManagerPlugin.h"
#include <memory>


namespace OHOS {
namespace Nearlink {
class ServiceManagerPluginLoader : public ServiceManagerPluginInterface {
public:
    const char* const DEFAULT_LIB_NAME = "libnearlink_service_plugin_impl_ext.z.so";
    const char* const DEFAULT_LIB_CREATE_FUNC_NAME = "CreateServicePluginInterface";
    const char* const DEFAULT_LIB_DESTROY_FUNC_NAME = "DestroyServicePluginInterface";

    explicit ServiceManagerPluginLoader();
    ~ServiceManagerPluginLoader() override;
    static ServiceManagerPluginLoader *GetInstance(void);

    void Init() override;
    void DeInit() override;

    void LoadPluginInterfaceLib(void);
    bool IsLibraryLoaded(void);
    void SetSleAdapterFunc(GetSleAdapterFunc func) override;
    void SetAdapterStateObserver(SetAdapterStateObserverFunc func) override;
    void SetPeripheralCallback(SetPeripheralCallbackFunc func) override;

    /**
     * @brief 以下是plugin调用的函数声明
     * @note  新增功能时，请在此注释下方添加对应的函数声明
     */
    void PowerMgrProc() override;
    void HighPowerProc(uint16_t lcid) override;
    void SleTvMgrProc(const std::string &address) override;
    void HidDataStatisticsProc(const std::string &address) override;
    void RssiChangedCbkProc(const std::string &address, int8_t rssi) override;
    void UpdateSleFreqBandAbility(const std::string &address) override;
    void CollaborationProc(CollaborationProcType type) override;
    void SvcCmdProc(std::string cmd, int32_t fd, const std::vector<std::u16string> &args, int32_t &svcResult,
        std::string &info) override;
    void SleReconnectProc(bool &isNeedReconn, int acbConnState, const RawAddress &peerAddr,
        int reason, std::set<int> reasonList) override;
    // adapter模块相关函数 start
    void PeerDeviceTypeProc(PeerDeviceTypeProcType proctype, const RawAddress &device) override;
    void SetPowerModeProc(SetPowerModeProcType proctype) override;
    void RegisterCallbackExt(RegisterCallbackModule module, int32_t &result) override;
    void GetLocalVocieCallFrameFourAbility(bool &isSupport) override;
    // adapter模块相关函数 end
    // chiputil模块函数 start
    void SetAcbSubrate(bool &ret, const RawAddress &device, const SleAcbSubrateParam &subrateParam) override;
    void SetConnFrameType4Subrate(const RawAddress &device) override;
    // chiputil模块函数 end

    std::string GetBundleName(BundleNameType type) override;
    // plugin调用的函数声明结束

private:
    CxxDynamicLibraryLoader<ServiceManagerPluginInterface> loader_;
    std::unique_ptr<SleInterfaceAdapterWrapper> sleAdapterWrapper_;
};

}  // namespace Nearlink
}  // namespace OHOS
#endif