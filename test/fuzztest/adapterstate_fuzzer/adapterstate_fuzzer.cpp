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

/* AdapterStateMachine 直调 harness（PR-8 Z-2，修复 0 行覆盖的 harness 缺陷）。
 *
 * 背景：host_fuzzer 唯一走查路径 init EnableSle 经 SleServiceManager::StartTask 的
 * slem_initialize()（裸进程必败）早退，CreateAdapters() 不执行，stateMachine_ 恒 nullptr，
 * 状态机 8 态从未实例化。本 harness 自建 FakeSleInterfaceAdapter 直接实例化状态机。
 *
 * SIGKILL 红线（勿删）：
 * - 只投 REQ/CMP 消息；绝不投任何 *_TIME_OUT 消息（TIME_OUT 分支会调
 *   SleServiceManager::Reset()，裸进程挂死）。
 * - 绝不调 Reset()/FactoryReset()/ClearAllStorage()。
 *
 * 定时器策略：每轮局部 shared_ptr 实例（作用域析构）。~NearlinkTimer =
 * TimerManager::StopTimer + DestroyTimer（nearlink_timer.cpp，无 join、无悬挂回调），
 * 轮末析构即取消 10s/1s 定时器，跨轮零残留。
 *
 * 设备侧效应登记（真机长跑前评估）：Entry(TurnOff) 首次真实触达 CheckAndStopService
 * ——设备上若开关 persist 参数为 ON 会走 ResetNearlinkService 写
 * persist.nearlink.reset_service（触发真实服务进程重启）；为 OFF 时 UnloadNearlinkSa
 * 的 samgr RPC 在 fuzz 进程无权限早退。裸进程/无权限环境均安全（蓝队 B-4 判定）。
 */

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include "adapterstate_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "ThreadUtil.h"
#include "log.h"
#include "AdapterStateMachine.h"
#include "SleInterfaceAdapter.h"
#include "context.h"

using namespace OHOS::Nearlink;

namespace OHOS {
namespace {
constexpr int ADAPTER_STATE_FUZZ_DELAY_50_MS = 50;
constexpr uint8_t ADAPTER_STATE_FUZZ_TARGET_MOD = 4; // 3 个稳定态 + 1 个非稳定态
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance(); // 构造即把全部线程置 ENABLED，勿再 per-round Init/Clear

/* 最小 Context：Name() 决定 transport 分支（SlbAdapter→ADAPTER_SLB / 其它→ADAPTER_SLE），
 * Enable/Disable/PostEnable 为空实现，不走真实驱动；开断言结果由 CMP 消息驱动 */
class FakeSleAdapterContext : public utility::Context {
public:
    explicit FakeSleAdapterContext(const std::string &name) : Context(name, "fuzz") {}
    ~FakeSleAdapterContext() override = default;
    void Enable() override {}
    void Disable() override {}
};

/* 自建假适配器：仅 GetContext() 返回真 context，其余 60 个纯虚接口最小 stub。
 * 状态机代码仅触达 GetContext()->Name()/Enable()/Disable()/PostEnable()。 */
class FakeSleInterfaceAdapter : public SleInterfaceAdapter {
public:
    explicit FakeSleInterfaceAdapter(const std::string &ctxName) : context_(ctxName) {}
    ~FakeSleInterfaceAdapter() override = default;

    utility::Context *GetContext() override
    {
        return &context_;
    }

    // gap
    std::string GetLocalAddress() const override
    {
        return "";
    }
    std::string GetLocalName() const override
    {
        return "";
    }
    bool SetLocalName(const std::string &name) const override
    {
        return false;
    }
    bool SleFreqHopping(const std::vector<uint8_t> &freq) override
    {
        return false;
    }
    bool SetBondableMode(int mode) const override
    {
        return false;
    }
    int GetBondableMode() const override
    {
        return 0;
    }
    // remote device information
    std::string GetDeviceName(const RawAddress &device) const override
    {
        return "";
    }
    std::string GetAliasName(const RawAddress &device) const override
    {
        return "";
    }
    int GetDeviceAppearance(const RawAddress &device) const override
    {
        return 0;
    }
    uint16_t GetDeviceVendorId(const RawAddress &device) const override
    {
        return 0;
    }
    uint16_t GetDeviceProductId(const RawAddress &device) const override
    {
        return 0;
    }
    uint16_t GetDeviceVersion(const RawAddress &device) const override
    {
        return 0;
    }
    DeviceModel GetDeviceModel(const RawAddress &device) const override
    {
        return DeviceModel();
    }
    bool SetAliasName(const RawAddress &device, const std::string &name) const override
    {
        return false;
    }
    std::vector<Uuid> GetDeviceUuids(const RawAddress &device) const override
    {
        return {};
    }
    // pair
    std::vector<RawAddress> GetPairedDevices() const override
    {
        return {};
    }
    std::vector<RawAddress> GetConnectedDevices() const override
    {
        return {};
    }
    void SetSleConnectionMode(int32_t connectionMode, int32_t duration) override {}
    bool StartPair(const RawAddress &device) override
    {
        return false;
    }
    bool StartCrediblePair(const RawAddress &device) override
    {
        return false;
    }
    bool DisconnectAction(const RawAddress &device, uint8_t discReason) const override
    {
        return false;
    }
    bool ConnectAllProfile(const RawAddress &device) override
    {
        return false;
    }
    void RemoveBgConnDevice(const std::string &device) const override {}
    bool DisconnectAllProfile(const RawAddress &device) override
    {
        return false;
    }
    bool IsBondedFromLocal(const RawAddress &device) const override
    {
        return false;
    }
    bool CancelPairing(const RawAddress &device) override
    {
        return false;
    }
    bool RemovePair(const RawAddress &device) override
    {
        return false;
    }
    bool RemoveAllPairs() override
    {
        return false;
    }
    bool SetPairingConfirmation(const RawAddress &device) const override
    {
        return false;
    }
    bool SetPairingPassCode(const RawAddress &device, const std::string &passCode) override
    {
        return false;
    }
    int GetPairState(const RawAddress &device) const override
    {
        return 0;
    }
    int GetAcbState(const RawAddress &device) const override
    {
        return 0;
    }
    uint32_t GetAcbCount() const override
    {
        return 0;
    }
    bool PairRequestReply(const RawAddress &device, bool accept) const override
    {
        return false;
    }
    // other
    bool IsAcbConnected(const RawAddress &device) const override
    {
        return false;
    }
    bool IsAcbEncrypted(const RawAddress &device) const override
    {
        return false;
    }
    uint8_t GetLinkRole(const RawAddress &device) const override
    {
        return 0;
    }
    bool FactoryReset() const override
    {
        return false;
    }
    void CancelAllConnection() override {}
    int GetProfileConnState(const RawAddress &device) override
    {
        return 0;
    }
    bool WrapperCdsmGetAllMemberInfo(const RawAddress &rawAddr, std::vector<RawAddress> &cdsmInfoAddr) override
    {
        return false;
    }
    bool UpdateSleVirtualDevice(int32_t cmd, const RawAddress &device) override
    {
        return false;
    }
    bool GetSleAddrByBtAddr(const std::string &btAddr, std::string &sleAddr) const override
    {
        return false;
    }
    bool GetBtAddrBySleAddr(const std::string &sleAddr, std::string &btAddr) const override
    {
        return false;
    }
    bool SetBtAddrBySleAddr(const std::string &sleAddr, const std::string &btAddr) const override
    {
        return false;
    }
    bool IsScanConnTypeAndFrameType4(const RawAddress &device, uint8_t connCompleteType) const override
    {
        return false;
    }
    void UpdateDeviceModelInfo(const std::string &address, const DeviceModel &model,
        const std::string &newModelId) override {}
    void DisconnectAllProfileForSilentPort(const RawAddress &device) override {}
    void ConnectAcb(const RawAddress &device) override {}
    void RemoveNotPairedCloudDevice(const RawAddress &device) const override {}
    bool DisconnectAcb(const RawAddress &device, uint8_t discReason) const override
    {
        return false;
    }
    void ClearBgConnDevice() const override {}
    bool UpdateRefusePolicy(const int32_t protocolType, const int32_t pid, const int64_t refuseTime) const override
    {
        return false;
    }
    void DelConnFrameType(const std::string &addr) override {}
    void SetConnFrameType(const std::string &addr, uint8_t frameType) override {}
    bool GetConnFrameType(const std::string &addr, uint8_t &frameType) const override
    {
        return false;
    }
    void NotifyPairStatusChanged(const RawAddress &device, int preStatus, int status, int reason) const override {}
    void NotifyConnectionStateChanged(
        const RawAddress &device, const SleConnectionChangedParam &connChangedParam) const override {}
    void SetPhy(const RawAddress &device, uint8_t frameType, uint8_t phyType) override {}

private:
    FakeSleAdapterContext context_;
};

using AdapterMsg = AdapterStateMachine::AdapterStateMessage;

/* make_shared 持有后再 Init（Init 内 shared_from_this 语义）。
 * Init 末尾 InitState(TurnOff) 会同步执行 Entry(TurnOff)：SleServiceManager::state_ 初始即
 * TURN_OFF → "state unchanged" 早退，安全。 */
std::shared_ptr<AdapterStateMachine> BuildMachine(bool useSlb)
{
    std::string ctxName = useSlb ? ADAPTER_NAME_SLB : ADAPTER_NAME_SLE;
    std::shared_ptr<AdapterStateMachine> machine = std::make_shared<AdapterStateMachine>();
    std::shared_ptr<FakeSleInterfaceAdapter> adapter = std::make_shared<FakeSleInterfaceAdapter>(ctxName);
    machine->Init(adapter);
    return machine;
}

/* 只投 REQ/CMP（arg1M 可控）；TIME_OUT 全部禁投（SIGKILL 红线） */
void PostMsg(const std::shared_ptr<AdapterStateMachine> &machine, AdapterMsg id, bool arg1)
{
    utility::Message msg(static_cast<int>(id), arg1 ? 1 : 0);
    machine->ProcessMessage(msg);
}

/* 走查到 TurnOn：先置 nextTarget=ON 防 Entry(TurnOn) 弹回（弹回会按旧目标态续投 REQ）
 * Off --ENABLE_REQ--> TurningOn --ADAPTER_ENABLE_CMP(true)--> --PROFILE_ENABLE_CMP(true)--> On */
void WalkToTurnOn(const std::shared_ptr<AdapterStateMachine> &machine)
{
    machine->SetNextTargetState(SleStateID::STATE_TURN_ON);
    PostMsg(machine, AdapterMsg::MSG_ENABLE_REQ, true);
    PostMsg(machine, AdapterMsg::MSG_ADAPTER_ENABLE_CMP, true);
    PostMsg(machine, AdapterMsg::MSG_PROFILE_ENABLE_CMP, true);
}

/* 走查到 TurnHalf：Off --HALF_DISABLE_REQ--> TurningOffToHalf --CMP(true)x2--> Half */
void WalkToTurnHalf(const std::shared_ptr<AdapterStateMachine> &machine)
{
    machine->SetNextTargetState(SleStateID::STATE_TURN_HALF);
    PostMsg(machine, AdapterMsg::MSG_HALF_DISABLE_REQ, true);
    PostMsg(machine, AdapterMsg::MSG_ADAPTER_ENABLE_CMP, true);
    PostMsg(machine, AdapterMsg::MSG_PROFILE_ENABLE_CMP, true);
}

void WalkToTurningOff(const std::shared_ptr<AdapterStateMachine> &machine)
{
    WalkToTurnOn(machine);
    PostMsg(machine, AdapterMsg::MSG_DISABLE_REQ, true);
}

void WalkToTurningHalfToOff(const std::shared_ptr<AdapterStateMachine> &machine)
{
    WalkToTurnHalf(machine);
    PostMsg(machine, AdapterMsg::MSG_DISABLE_REQ, true);
}

// 1. TurnOff（初始稳定态）：ENABLE_REQ→TurningOn；HALF_DISABLE_REQ→TurningOffToHalf
void RunCaseTurnOff(FuzzedDataProvider &fdp)
{
    bool useSlb = fdp.ConsumeBool();
    PostMsg(BuildMachine(useSlb), AdapterMsg::MSG_ENABLE_REQ, true);
    PostMsg(BuildMachine(useSlb), AdapterMsg::MSG_HALF_DISABLE_REQ, true);
}

// 2. TurningOn：ADAPTER_ENABLE_CMP / PROFILE_ENABLE_CMP（arg1M 双分支，false 走失败 CMP 迁移）
void RunCaseTurningOn(FuzzedDataProvider &fdp)
{
    bool useSlb = fdp.ConsumeBool();
    bool cmpOkA = fdp.ConsumeBool();
    bool cmpOkB = fdp.ConsumeBool();
    std::shared_ptr<AdapterStateMachine> machineA = BuildMachine(useSlb);
    PostMsg(machineA, AdapterMsg::MSG_ENABLE_REQ, true);
    PostMsg(machineA, AdapterMsg::MSG_ADAPTER_ENABLE_CMP, cmpOkA);

    std::shared_ptr<AdapterStateMachine> machineB = BuildMachine(useSlb);
    PostMsg(machineB, AdapterMsg::MSG_ENABLE_REQ, true);
    PostMsg(machineB, AdapterMsg::MSG_PROFILE_ENABLE_CMP, cmpOkB);
}

// 3. TurnOn（稳定态）：DISABLE_REQ→TurningOff；HALF_DISABLE_REQ→TurningOff（ON→OFF→HALF 流程）
void RunCaseTurnOn(FuzzedDataProvider &fdp)
{
    bool useSlb = fdp.ConsumeBool();
    std::shared_ptr<AdapterStateMachine> machineA = BuildMachine(useSlb);
    WalkToTurnOn(machineA);
    PostMsg(machineA, AdapterMsg::MSG_DISABLE_REQ, true);

    std::shared_ptr<AdapterStateMachine> machineB = BuildMachine(useSlb);
    WalkToTurnOn(machineB);
    PostMsg(machineB, AdapterMsg::MSG_HALF_DISABLE_REQ, true);
}

// 4. TurningOff：PROFILE_DISABLE_CMP（停 profileTimer/启 adapterTimer/ctx Disable）；
//    ADAPTER_DISABLE_CMP（→TurnOff，Entry(TurnOff) 首次真实走 CheckAndStopService 链）
void RunCaseTurningOff(FuzzedDataProvider &fdp)
{
    bool useSlb = fdp.ConsumeBool();
    bool cmpOkA = fdp.ConsumeBool();
    bool cmpOkB = fdp.ConsumeBool();
    std::shared_ptr<AdapterStateMachine> machineA = BuildMachine(useSlb);
    WalkToTurningOff(machineA);
    PostMsg(machineA, AdapterMsg::MSG_PROFILE_DISABLE_CMP, cmpOkA);

    std::shared_ptr<AdapterStateMachine> machineB = BuildMachine(useSlb);
    WalkToTurningOff(machineB);
    PostMsg(machineB, AdapterMsg::MSG_ADAPTER_DISABLE_CMP, cmpOkB);
}

// 5. TurningHalfToOff：消息对同 TurningOff（半关→全关路径）
void RunCaseTurningHalfToOff(FuzzedDataProvider &fdp)
{
    bool useSlb = fdp.ConsumeBool();
    bool cmpOkA = fdp.ConsumeBool();
    bool cmpOkB = fdp.ConsumeBool();
    std::shared_ptr<AdapterStateMachine> machineA = BuildMachine(useSlb);
    WalkToTurningHalfToOff(machineA);
    PostMsg(machineA, AdapterMsg::MSG_PROFILE_DISABLE_CMP, cmpOkA);

    std::shared_ptr<AdapterStateMachine> machineB = BuildMachine(useSlb);
    WalkToTurningHalfToOff(machineB);
    PostMsg(machineB, AdapterMsg::MSG_ADAPTER_DISABLE_CMP, cmpOkB);
}

// 6. TurningOffToHalf：ADAPTER_ENABLE_CMP / PROFILE_ENABLE_CMP（false 走失败 CMP→TurningHalfToOff）
void RunCaseTurningOffToHalf(FuzzedDataProvider &fdp)
{
    bool useSlb = fdp.ConsumeBool();
    bool cmpOkA = fdp.ConsumeBool();
    bool cmpOkB = fdp.ConsumeBool();
    std::shared_ptr<AdapterStateMachine> machineA = BuildMachine(useSlb);
    PostMsg(machineA, AdapterMsg::MSG_HALF_DISABLE_REQ, true);
    PostMsg(machineA, AdapterMsg::MSG_ADAPTER_ENABLE_CMP, cmpOkA);

    std::shared_ptr<AdapterStateMachine> machineB = BuildMachine(useSlb);
    PostMsg(machineB, AdapterMsg::MSG_HALF_DISABLE_REQ, true);
    PostMsg(machineB, AdapterMsg::MSG_PROFILE_ENABLE_CMP, cmpOkB);
}

// 7. TurningHalfToOn：Entry 自投 TURNING_TASK_CMP（唯一安全驱动点，Entry 后即迁移 TurnOn，
//    状态不可驻留；TIME_OUT 分支禁投）
void RunCaseTurningHalfToOn(FuzzedDataProvider &fdp)
{
    bool useSlb = fdp.ConsumeBool();
    std::shared_ptr<AdapterStateMachine> machine = BuildMachine(useSlb);
    WalkToTurnHalf(machine);
    PostMsg(machine, AdapterMsg::MSG_ENABLE_REQ, true);
}

// 8. TurnHalf（稳定态）：ENABLE_REQ→TurningHalfToOn；DISABLE_REQ→TurningHalfToOff
void RunCaseTurnHalf(FuzzedDataProvider &fdp)
{
    bool useSlb = fdp.ConsumeBool();
    std::shared_ptr<AdapterStateMachine> machineA = BuildMachine(useSlb);
    WalkToTurnHalf(machineA);
    PostMsg(machineA, AdapterMsg::MSG_ENABLE_REQ, true);

    std::shared_ptr<AdapterStateMachine> machineB = BuildMachine(useSlb);
    WalkToTurnHalf(machineB);
    PostMsg(machineB, AdapterMsg::MSG_DISABLE_REQ, true);
}

// B. 三个公开方法直调（稳定态 TurnOff 上）：SetNextTargetState 稳定/非稳定分支 +
//    GetNextTargetState + PostTargetMessage（按目标态投 ENABLE/DISABLE/HALF_DISABLE_REQ）
void RunPublicApiTrio(FuzzedDataProvider &fdp, uint8_t targetSel)
{
    bool useSlb = fdp.ConsumeBool();
    std::shared_ptr<AdapterStateMachine> machine = BuildMachine(useSlb); // 驻留稳定态 TurnOff
    switch (targetSel % ADAPTER_STATE_FUZZ_TARGET_MOD) {
        case 0:
            machine->SetNextTargetState(SleStateID::STATE_TURN_ON);
            break;
        case 1:
            machine->SetNextTargetState(SleStateID::STATE_TURN_OFF);
            break;
        case 2:
            machine->SetNextTargetState(SleStateID::STATE_TURN_HALF);
            break;
        default:
            machine->SetNextTargetState(SleStateID::STATE_TURNING_ON); // 非稳定态 → "not stable state" 分支
            break;
    }
    SleStateID curTarget = machine->GetNextTargetState();
    HILOGI("adapterstate_fuzzer next target state: %{public}d", static_cast<int>(curTarget));
    machine->PostTargetMessage(); // 稳定态直投目标 REQ；若已随迁移离开稳定态则为态内默认分支
}
} // namespace

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    /* FDP 读序（单流，整型自缓冲区尾端逆序消费；后续扩展只允许追加尾部，保旧 corpus 兼容）：
     * [尾1] targetSel: SetNextTargetState 选择器 %4（RunPublicApiTrio 用）
     * [尾2] useSlb: RunCaseTurnOff 的 context 名 SlbAdapter/SleAdapter → ADAPTER_SLB/ADAPTER_SLE
     * [尾3~尾5] RunCaseTurningOn: useSlb + cmpOkA + cmpOkB（CMP arg1M 成败双分支）
     * [尾6] RunCaseTurnOn: useSlb
     * [尾7~尾9] RunCaseTurningOff: useSlb + cmpOkA + cmpOkB
     * [尾10~尾12] RunCaseTurningHalfToOff: useSlb + cmpOkA + cmpOkB
     * [尾13~尾15] RunCaseTurningOffToHalf: useSlb + cmpOkA + cmpOkB
     * [尾16] RunCaseTurningHalfToOn: useSlb
     * [尾17] RunCaseTurnHalf: useSlb
     * [尾18] RunPublicApiTrio: useSlb */
    uint8_t targetSel = fdp.ConsumeIntegral<uint8_t>();

    OHOS::RunCaseTurnOff(fdp);
    OHOS::RunCaseTurningOn(fdp);
    OHOS::RunCaseTurnOn(fdp);
    OHOS::RunCaseTurningOff(fdp);
    OHOS::RunCaseTurningHalfToOff(fdp);
    OHOS::RunCaseTurningOffToHalf(fdp);
    OHOS::RunCaseTurningHalfToOn(fdp);
    OHOS::RunCaseTurnHalf(fdp);
    OHOS::RunPublicApiTrio(fdp, targetSel);

    std::this_thread::sleep_for(std::chrono::milliseconds(ADAPTER_STATE_FUZZ_DELAY_50_MS));
    return 0;
}
} // namespace OHOS
