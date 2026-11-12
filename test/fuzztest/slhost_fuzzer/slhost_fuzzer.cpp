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

#include <thread>
#include "fuzzer/FuzzedDataProvider.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_host_stub.h"
#include "nearlink_host_server.h"
#include "nearlink_errorcode.h"
#include "log.h"
#include "raw_address.h"
#include "SleServiceManager.h"
#include "SysStateMachine.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

// ---------------------------------------------------------------------------
// slhost fuzzer: host IPC face under the always-true IsSleEnabled /
// IsSleAvailableToCaller mock (mock_nearlink_host_server.cpp), complementing
// host_fuzzer's real-value environment, plus a SleServiceManager deep drive.
// Safety: Reset()/FactoryReset()/ClearAllStorage()/Stop()/AdapterStop() and
// OnChipResetNotify() are NEVER called (Reset kills the process). No
// *_TIME_OUT message is ever posted; after each switch request the machine is
// polled back into a stable state so the 10s turning timers are closed by the
// adapter/profile CMP path.
// ---------------------------------------------------------------------------
class SlHostFuzzAdapterStateObserverStub : public IAdapterStateObserver {
public:
    void OnStateChange(const SleTransport transport, const SleStateID state) {}
    void OnDisableResponse(bool isHalfDisable, SwitchCallerInfo callerInfo) {}
};

class SlHostFuzzSystemStateObserverStub : public ISystemStateObserver {
public:
    void OnSystemStateChange(const SleSystemState state) {}
};

namespace {
constexpr int SLHOST_FUZZ_DELAY_50_MS = 50;
constexpr int SLHOST_FUZZ_DELAY_100_MS = 100;
constexpr int SLHOST_FUZZ_DELAY_5000_MS = 5000;
constexpr int SLE_STATE_WAIT_LOOPS = 40; // 40 * 50ms = 2s bounded wait per step
constexpr uint8_t WALK_TABLE_SIZE = 5;
constexpr uint8_t WALK_MAX_STEPS = 6;
}

int32_t SlHostOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    NL_CHECK_RETURN_RET(hostServer, TRANSACTION_ERR, "hostServer is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("SlHostOnRemoteRequest cmd(%{public}d)", code);
    return hostServer->OnRemoteRequest(code, data, reply, option);
}

void SlHostStateQueryFuzzTest(const uint8_t *fuzzData, size_t size)
{
    static constexpr NearlinkHostInterfaceCode IPC_CODE_TABLE[] = {
        NearlinkHostInterfaceCode::NL_GET_SLE_FULL_STATE,
        NearlinkHostInterfaceCode::NL_IS_SLE_ENABLED,
        NearlinkHostInterfaceCode::NL_IS_SLE_AVAILABLE,
        NearlinkHostInterfaceCode::NL_IS_SLE_HALF_DISABLED,
        NearlinkHostInterfaceCode::NL_IS_SLE_DISABLED,
        NearlinkHostInterfaceCode::NL_GET_ADAPTER_CONNECTION_STATE,
    };
    constexpr uint8_t IPC_CODE_TABLE_SIZE = 6;
    FuzzedDataProvider provider(fuzzData, size);
    const uint8_t shots = provider.ConsumeIntegralInRange<uint8_t>(1, 3);
    for (uint8_t i = 0; i < shots; i++) {
        MessageParcel data;
        MessageParcel reply;
        data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
        NearlinkHostInterfaceCode code =
            IPC_CODE_TABLE[provider.ConsumeIntegral<uint8_t>() % IPC_CODE_TABLE_SIZE];
        int32_t ret = SlHostOnRemoteRequest(code, data, reply);
        if (ret != NO_ERROR) {
            HILOGI("send req failed, ret(%{public}d)", ret);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SLHOST_FUZZ_DELAY_50_MS));
    }
}

bool SlHostWaitSleStableState(SleServiceManager *manager)
{
    for (int i = 0; i < SLE_STATE_WAIT_LOOPS; i++) {
        SleStateID state = manager->GetState(SleTransport::ADAPTER_SLE);
        if (SleServiceManager::IsStateStable(state)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SLHOST_FUZZ_DELAY_50_MS));
    }
    HILOGE("sle state not stable after bounded wait");
    return false;
}

void SlHostServiceManagerFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    SleServiceManager *manager = SleServiceManager::GetInstance();
    if (manager == nullptr) {
        HILOGE("SleServiceManager is nullptr");
        return;
    }
    static SlHostFuzzAdapterStateObserverStub adapterObserver;
    static SlHostFuzzSystemStateObserverStub systemObserver;

    // observer register / duplicate-register / deregister branches
    manager->RegisterStateObserver(adapterObserver);
    manager->RegisterSystemStateObserver(systemObserver);
    manager->RegisterStateObserver(adapterObserver); // duplicate -> rejected branch
    manager->DeregisterStateObserver(adapterObserver);
    manager->DeregisterSystemStateObserver(systemObserver);
    manager->DeregisterSystemStateObserver(systemObserver); // not registered -> rejected branch
    manager->RegisterStateObserver(adapterObserver);        // register again after deregister
    manager->RegisterSystemStateObserver(systemObserver);

    // query family (both transports where applicable)
    SleEventType reason = static_cast<SleEventType>(provider.ConsumeIntegral<uint8_t>() % 10);
    SwitchCallerInfo callerInfo = {};
    callerInfo.fullTokenId = provider.ConsumeIntegral<uint64_t>();
    callerInfo.callerUid = provider.ConsumeIntegral<int32_t>();
    SleAutoConnectPolicy autoConnPolicy =
        static_cast<SleAutoConnectPolicy>(provider.ConsumeIntegral<uint8_t>() % 3);
    SleStateID fuzzState = static_cast<SleStateID>(provider.ConsumeIntegral<uint8_t>() % 9);
    SleStateID stateSle = manager->GetState(SleTransport::ADAPTER_SLE);
    SleStateID stateSlb = manager->GetState(SleTransport::ADAPTER_SLB);
    SleConnectState connectState = manager->GetAdapterConnectState();
    int maxAudio = manager->GetMaxNumConnectedAudioDevices();
    SleStateID switchParamState = manager->GetNearlinkSwitchStateFromSystemParameter();
    bool stateStable = SleServiceManager::IsStateStable(fuzzState);
    bool switchRestricted = manager->IsSleSwitchRestricted();
    bool autoConnAudio = manager->IsEnableAutoConnectAudioDevices();
    bool autoConnUserDisc = manager->IsEnableAutoConnectUserDisconnectedDevices();
    bool disabling = manager->IsDisabling();
    HILOGI("state(%{public}d/%{public}d), conn(%{public}d), maxAudio(%{public}d), param(%{public}d), "
           "stable(%{public}d), restricted(%{public}d), audioConn(%{public}d), userDiscConn(%{public}d), "
           "disabling(%{public}d)", static_cast<int>(stateSle), static_cast<int>(stateSlb),
           static_cast<int>(connectState), maxAudio, static_cast<int>(switchParamState),
           static_cast<int>(stateStable), static_cast<int>(switchRestricted), static_cast<int>(autoConnAudio),
           static_cast<int>(autoConnUserDisc), static_cast<int>(disabling));

    // state machine walk (core TurnOff <-> TurnOn <-> TurnHalf, one failure variant)
    // ponytail: enable-direction failure CMP(false) injection removed — the early-exit path in
    // HandleProfileEnableCmpMsg leaves the one-shot adapterTimer_ armed; a stray fire in
    // TurningOn/TurningOffToHalf triggers Reset()=SIGKILL. See host_fuzzer for details.
    const uint8_t walkSteps = provider.ConsumeIntegralInRange<uint8_t>(1, WALK_MAX_STEPS);
    for (uint8_t step = 0; step < walkSteps; step++) {
        SleStateID before = manager->GetState(SleTransport::ADAPTER_SLE);
        uint8_t sel = provider.ConsumeIntegral<uint8_t>() % WALK_TABLE_SIZE;
        HILOGI("walk step(%{public}u/%{public}u) from state(%{public}d) sel(%{public}u)",
            static_cast<unsigned>(step) + 1, static_cast<unsigned>(walkSteps), static_cast<int>(before),
            static_cast<unsigned>(sel));
        switch (sel) {
            case 0U:
                manager->Enable(SleTransport::ADAPTER_SLE, reason, callerInfo, autoConnPolicy);
                break;
            case 1U:
                manager->Disable(SleTransport::ADAPTER_SLE, reason, callerInfo);
                break;
            case 2U:
                manager->EnableToHalf(SleTransport::ADAPTER_SLE, reason, callerInfo);
                break;
            case 3U:
                manager->DisableToOff(SleTransport::ADAPTER_SLE, reason, callerInfo);
                break;
            case 4U:
            default:
                manager->DisableToOff(SleTransport::ADAPTER_SLE, reason, callerInfo);
                manager->OnProfileServicesDisableComplete(SleTransport::ADAPTER_SLE, false);
                break;
        }
        SlHostWaitSleStableState(manager);
    }
    // corrective: leave the machine enabled for the next fuzz input
    SleStateID cur = manager->GetState(SleTransport::ADAPTER_SLE);
    if (cur != SleStateID::STATE_TURN_ON) {
        manager->Enable(SleTransport::ADAPTER_SLE, reason, callerInfo, autoConnPolicy);
        SlHostWaitSleStableState(manager);
    }

    // pair devices removed (both transports) and adapter handle queries
    std::vector<RawAddress> devices;
    devices.push_back(RawAddress(BuildAddressString(provider)));
    devices.push_back(RawAddress(BuildAddressString(provider)));
    manager->OnPairDevicesRemoved(SleTransport::ADAPTER_SLE, devices);
    manager->OnPairDevicesRemoved(SleTransport::ADAPTER_SLB, devices);
    SleInterfaceAdapter *adapterSle = manager->GetAdapter(SleTransport::ADAPTER_SLE);
    if (adapterSle != nullptr) {
        std::string localName = adapterSle->GetLocalName();
        HILOGI("sle adapter local name(%{public}s)", localName.c_str());
    }
    SleInterfaceAdapter *adapterSlb = manager->GetAdapter(SleTransport::ADAPTER_SLB);
    if (adapterSlb != nullptr) {
        std::string localAddr = adapterSlb->GetLocalAddress();
        HILOGI("slb adapter local addr(%{public}s)", localAddr.c_str());
    }

    // sys state notify with benign state only ("Stopping"/"FactoryResetting"
    // would satisfy stop/reset promises, keep them away)
    manager->OnSysStateChange(SYS_STATE_STARTED);
    manager->OnSysStateExit(SYS_STATE_STARTED);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SLHOST_FUZZ_DELAY_50_MS));
}
}  // namespace OHOS

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    // mock token
    NearlinkMockNativeToken mock("nearlink_service");

    OHOS::sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    if (hostServer == nullptr) {
        return 0;
    }
    HILOGI("slhost_fuzzer OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SLHOST_FUZZ_DELAY_100_MS));
    HILOGI("slhost_fuzzer EnableSle");
    hostServer->EnableSle();
    // IsSleEnabled is mocked to true, walk the service manager chain deeper
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SLHOST_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    OHOS::SlHostStateQueryFuzzTest(data, size);
    OHOS::SlHostServiceManagerFuzzTest(data, size);

    return 0;
}
