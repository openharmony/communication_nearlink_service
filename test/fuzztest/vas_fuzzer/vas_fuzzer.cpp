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
#include "vas_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_host_server.h"
#include "ThreadUtil.h"
#include "log.h"
#ifdef NEARLINK_FUZZ_TELEPHONY_PARTS
#include "VasService.h"
#include "VasStackAdapter.h"
#endif

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr int VAS_FUZZ_DELAY_50_MS = 50;   // 收敛 DoInVasThread 异步任务
constexpr int VAS_FUZZ_DELAY_100_MS = 100; // init OnStart 后等待
constexpr int VAS_FUZZ_DELAY_5000_MS = 5000; // init EnableSle 后等待（基线判例）
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

#ifdef NEARLINK_FUZZ_TELEPHONY_PARTS
/* VasService 全生命周期直调：公有构造，栈上实例覆盖 ctor/dtor + Enable/Disable 幂等分支；
 * WakeUpVoiceRecognition 依赖 samgr（裸进程早退走 OpenVoiceAssistantFail 分支） */
void DriveVasService(VasService &vasSvc, const RawAddress &device, uint32_t reqActivate, uint32_t reqClose)
{
    vasSvc.Enable();
    vasSvc.Enable();  // 已使能幂等分支（CreateVasInstance 链）
    vasSvc.OpenVoiceAssistant(device);   // SetVoiceAssistantOpened 链
    vasSvc.CloseVoiceAssistant(device);  // SetVoiceAssistantClosed 链
    vasSvc.HandleActivateVoiceAssistant(device, reqActivate);
    vasSvc.HandleCloseVoiceAssistant(device, reqClose);
    vasSvc.Connect(device);
    vasSvc.Disconnect(device);
    vasSvc.GetConnectState();
    vasSvc.GetConnectDevices();
    vasSvc.GetContext();
    vasSvc.Disable();
    vasSvc.Disable();  // 已去使能幂等分支（DeleteVasInstance 链）
}

/* 局部生命周期段：构造/驱动/等待落定/析构（DoInVasThread 残留任务防 this 悬垂） */
void VasLocalLifecycle(const RawAddress &device, uint32_t reqActivate, uint32_t reqClose)
{
    VasService vasService;
    DriveVasService(vasService, device, reqActivate, reqClose);
    // ASAN 真机实证 50ms 不足（DoInVasThread 残留任务在栈实例析构后执行 pimpl 解引用，
    // stack-use-after-scope @VasService.cpp:147）——局部生命周期段收敛等待提到 250ms，
    // 并由调用侧 FDP 门控降频（不是每个输入都进本段）
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
}

/* VasStackAdapter 全公开方法直调（每次调用仅走 NLSTK + DFT 上报，无全局注册副作用） */
void DriveVasStackAdapter(uint32_t reqOpenOk, uint32_t reqOpenFail, uint32_t reqCloseOk, uint32_t reqCloseFail)
{
    VasStackAdapter stackAdapter;
    stackAdapter.CreateVasInstance();
    stackAdapter.SetVoiceAssistantOpened();
    stackAdapter.SetVoiceAssistantClosed();
    stackAdapter.OpenVoiceAssistantSuccess(reqOpenOk);
    stackAdapter.OpenVoiceAssistantFail(reqOpenFail);
    stackAdapter.CloseVoiceAssistantSuccess(reqCloseOk);
    stackAdapter.CloseVoiceAssistantFail(reqCloseFail);
    stackAdapter.DeleteVasInstance();
}

void VasServiceFuzzTestInner(FuzzedDataProvider &fdp)
{
    HILOGI("VasFuzzTest,start run VasServiceFuzzTestInner");
    RawAddress device = BuildRawAddress(fdp);
    uint32_t reqActivate = fdp.ConsumeIntegral<uint32_t>();
    uint32_t reqClose = fdp.ConsumeIntegral<uint32_t>();
    uint32_t reqOpenOk = fdp.ConsumeIntegral<uint32_t>();
    uint32_t reqOpenFail = fdp.ConsumeIntegral<uint32_t>();
    uint32_t reqCloseOk = fdp.ConsumeIntegral<uint32_t>();
    uint32_t reqCloseFail = fdp.ConsumeIntegral<uint32_t>();

    if ((fdp.ConsumeIntegral<uint8_t>() & 0x07) == 0) {  // 局部生命周期段降频(1/8)，FDP 驱动选择
        VasLocalLifecycle(device, reqActivate, reqClose);
    }

    // 无 ProfileServiceManager 引导时返回 nullptr，覆盖判空跳过分支；有则轻驱管理器实例
    VasService *vasServiceStatic = VasService::GetService();
    if (vasServiceStatic == nullptr) {
        HILOGI("VasFuzzTest GetService is nullptr, skip");
    } else {
        vasServiceStatic->GetConnectState();
        vasServiceStatic->OpenVoiceAssistant(device);
    }

    DriveVasStackAdapter(reqOpenOk, reqOpenFail, reqCloseOk, reqCloseFail);
    std::this_thread::sleep_for(std::chrono::milliseconds(VAS_FUZZ_DELAY_50_MS));
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
    HILOGI("VasFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::VAS_FUZZ_DELAY_100_MS));
    HILOGI("VasFuzzTest EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::VAS_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
#ifdef NEARLINK_FUZZ_TELEPHONY_PARTS
    FuzzedDataProvider fdp(data, size);
    OHOS::VasServiceFuzzTestInner(fdp);
#else
    (void) size;
    HILOGI("VasFuzzTest telephony parts absent, skip");
#endif
    return 0;
}
