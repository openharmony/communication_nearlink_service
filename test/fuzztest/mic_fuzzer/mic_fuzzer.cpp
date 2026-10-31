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
#include "mic_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_host_server.h"
#include "ThreadUtil.h"
#include "log.h"
#ifdef NEARLINK_FUZZ_AUDIO_PARTS
#include "MicService.h"
#include "MicClientStackAdapter.h"
#endif

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr int MIC_FUZZ_DELAY_50_MS = 50;   // 收敛 DoInMicThread 异步任务
constexpr int MIC_FUZZ_DELAY_100_MS = 100; // init OnStart 后等待
constexpr int MIC_FUZZ_DELAY_5000_MS = 5000; // init EnableSle 后等待（基线判例）
constexpr uint8_t MIC_FUZZ_STATE_NUM = 5;  // SleConnectState 枚举成员数
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

#ifdef NEARLINK_FUZZ_AUDIO_PARTS
class FuzzMicObserver : public MicObserver {
public:
    void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override {}
};

class FuzzMicStateObserver : public MicStateObserver {
public:
    void OnMicStateChanged(const RawAddress &device, uint8_t micState) override {}
};

/* MicService 全生命周期直调：公有构造，栈上实例覆盖 ctor/dtor + 幂等分支 */
void DriveMicService(MicService &micSvc, const RawAddress &device, SleConnectState curState,
    SleConnectState preState, uint8_t micStateRaw)
{
    FuzzMicObserver micObserver;
    FuzzMicStateObserver micStateObserver;
    micSvc.RegisterObserver(micObserver);
    micSvc.RegisterMicStateObserver(micStateObserver);

    micSvc.Enable();
    micSvc.Enable();  // 已使能幂等分支
    micSvc.Connect(device);
    micSvc.Disconnect(device);
    micSvc.GetConnectState();
    micSvc.GetConnectDevices();
    micSvc.GetContext();

    // NotifyStateChanged：随机(含非法)状态对（随机性整改：两条常量语义对删除，
    // (CONNECTED,CONNECTING)/(DISCONNECTED,CONNECTED) 与 %5 FDP 对同域，由种子/引擎保证）
    micSvc.NotifyStateChanged(device, curState, preState);

    // UpdateMicState：合法 ON 插入/同态跳过/ON->OFF 迁移 + 随机(含非法)值
    micSvc.UpdateMicState(device, MIC_ON);
    micSvc.UpdateMicState(device, MIC_ON);
    micSvc.UpdateMicState(device, MIC_OFF);
    micSvc.UpdateMicState(device, micStateRaw);
    micSvc.IsMicOpen(device);

    micSvc.Disable();
    micSvc.Disable();  // 已去使能幂等分支

    micSvc.DeregisterMicStateObserver(micStateObserver);
    micSvc.DeregisterObserver(micObserver);
}

/* 局部生命周期段：构造/驱动/等待落定/析构（DoInMicThread 残留任务防 this 悬垂） */
void MicLocalLifecycle(const RawAddress &device, SleConnectState curState, SleConnectState preState,
    uint8_t micStateRaw)
{
    MicService micService;
    DriveMicService(micService, device, curState, preState, micStateRaw);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_FUZZ_DELAY_50_MS));
}

/* MicClientStackAdapter 全公开方法直调（回调注册为栈侧全局静态函数，重复注册安全） */
void MicAdapterDrive(const RawAddress &device)
{
    MicClientStackAdapter clientAdapter;
    clientAdapter.RegisterCallBackToStack();
    clientAdapter.Connect(device);
    clientAdapter.Disconnect(device);
}

void MicServiceFuzzTestInner(FuzzedDataProvider &fdp)
{
    HILOGI("MicFuzzTest,start run MicServiceFuzzTestInner");
    RawAddress device = BuildRawAddress(fdp);
    SleConnectState curState = static_cast<SleConnectState>(fdp.ConsumeIntegral<uint8_t>() % MIC_FUZZ_STATE_NUM);
    SleConnectState preState = static_cast<SleConnectState>(fdp.ConsumeIntegral<uint8_t>() % MIC_FUZZ_STATE_NUM);
    uint8_t micStateRaw = fdp.ConsumeIntegral<uint8_t>();

    MicLocalLifecycle(device, curState, preState, micStateRaw);

    // 无 ProfileServiceManager 引导时返回 nullptr，覆盖判空跳过分支；有则轻驱管理器实例
    MicService *micServiceStatic = MicService::GetService();
    if (micServiceStatic == nullptr) {
        HILOGI("MicFuzzTest GetService is nullptr, skip");
    } else {
        micServiceStatic->GetConnectState();
        micServiceStatic->IsMicOpen(device);
    }

    MicAdapterDrive(device);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_FUZZ_DELAY_50_MS));
}
#endif
}

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
    HILOGI("MicFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::MIC_FUZZ_DELAY_100_MS));
    HILOGI("MicFuzzTest EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::MIC_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
#ifdef NEARLINK_FUZZ_AUDIO_PARTS
    FuzzedDataProvider fdp(data, size);
    OHOS::MicServiceFuzzTestInner(fdp);
#else
    (void) size;
    HILOGI("MicFuzzTest audio parts absent, skip");
#endif
    return 0;
}
