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
#ifndef MOCK_SERVICE_MANAGER_PLUGIN_H
#define MOCK_SERVICE_MANAGER_PLUGIN_H

#include <utility>
#include <vector>
#include "IServiceManagerPlugin.h"

namespace OHOS {
namespace Nearlink {

// 外部芯片插件接口的测试替身：ControlAntennaFix 记录调用供断言，其余接口 no-op。
// fixCalls 无需加锁：回调都在 antenna 串行队列上执行，用例在哨兵同步后才读取。
class MockServiceManagerPlugin : public ServiceManagerPluginInterface {
public:
    using AntennaFixCall = std::pair<bool, AntennaFixScene>;

    std::vector<AntennaFixCall> fixCalls;
    void Clear()
    {
        fixCalls.clear();
    }
    void ControlAntennaFix(bool enable, AntennaFixScene scene) override
    {
        fixCalls.emplace_back(enable, scene);
    }

    // 其余纯虚接口与本套件无关，no-op（参数不命名以规避未使用告警）
    void SetSleAdapterFunc(GetSleAdapterFunc) override {}
    void SetAdapterStateObserver(SetAdapterStateObserverFunc) override {}
    void SetPeripheralCallback(SetPeripheralCallbackFunc) override {}
    void Init() override {}
    void DeInit() override {}
    void PowerMgrProc() override {}
    void HighPowerProc(uint16_t) override {}
    void SleTvMgrProc(const std::string &) override {}
    void HidDataStatisticsProc(const std::string &) override {}
    void UpdateSleFreqBandAbility(const std::string &) override {}
    void IsNeedCustomParam(bool &, int, uint16_t) override {}
    void UpdateCustomParam(uint16_t &, uint16_t &, int) override {}
    void CollaborationProc(CollaborationProcType) override {}
    void SvcCmdProc(std::string, int32_t, const std::vector<std::u16string> &, int32_t &, std::string &) override {}
    void SleReconnectProc(bool &, int, const RawAddress &, int, std::set<int>) override {}
    void PeerDeviceTypeProc(PeerDeviceTypeProcType, const RawAddress &) override {}
    void SetPowerModeProc(SetPowerModeProcType) override {}
    void RegisterCallbackExt(RegisterCallbackModule, int32_t &) override {}
    void GetLocalVocieCallFrameFourAbility(bool &) override {}
    void SetAcbSubrate(bool &, const RawAddress &, const SleAcbSubrateParam &) override {}
    void SetConnFrameType4Subrate(const RawAddress &) override {}
    void RejectSetSubrate(const RawAddress &) override {}
    std::string GetBundleName(BundleNameType) override
    {
        return "";
    }
};

// 进程级唯一 mock 实例，跨用例共享
MockServiceManagerPlugin &GetAntennaMockPlugin();
}  // namespace Nearlink
}  // namespace OHOS
#endif  // MOCK_SERVICE_MANAGER_PLUGIN_H
