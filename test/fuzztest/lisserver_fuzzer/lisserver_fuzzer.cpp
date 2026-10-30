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
#include "lisserver_fuzzer.h"
#include <thread>
#include "fuzzer/FuzzedDataProvider.h"
#include "../nl_utils/fuzztest_utils.h"
#include "ThreadUtil.h"
#include "log.h"
#include "slem.h"
#include "LisServer.h"
#include "LisService.h"

using namespace std;
using namespace OHOS::Nearlink;
using namespace utility;
namespace OHOS {
namespace {
constexpr int LIS_SERVER_PROFILE_FUZZ_DELAY_MS = 5000;
constexpr uint8_t LIS_FUZZ_REPEAT_TIMES = 3; // Enable/Disable 幂等重复次数上限
constexpr int LIS_FUZZ_ASYNC_DELAY_MS = 50; // 等待 DoInLisThread 异步事件落定
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();

// LisService 全生命周期 + 已注册态重复注册/注销驱动
void DriveLisService(const RawAddress &device, uint8_t enableRepeat, uint8_t disableRepeat)
{
    // Enable/Disable 经 DoInLisThread 投递事件，需先初始化线程状态表
    g_threadUtil.InitThreadStateMap();
    {
        // 每次输入新建/析构 LisService，模拟完整服务生命周期
        LisService lisService;
        lisService.Enable();
        lisService.GetConnectState();
        lisService.GetConnectDevices();
        lisService.Connect(device);
        lisService.Disconnect(device);
        Context *context = lisService.GetContext();
        if (context == nullptr) {
            HILOGI("LisServerFuzzTest GetContext is nullptr, skip");
        }
        lisService.Disable();
        // isStarted_ 原子标志下的 Enable/Disable 重复幂等分支
        for (uint8_t i = 0; i <= enableRepeat; ++i) {
            lisService.Enable();
        }
        for (uint8_t i = 0; i <= disableRepeat; ++i) {
            lisService.Disable();
        }
        // Enable->Disable->Enable 交替
        lisService.Enable();
        lisService.Disable();
        lisService.Enable();
        // 析构前等待 Lis 线程残留任务执行完，避免任务中的 this 悬垂
        std::this_thread::sleep_for(std::chrono::milliseconds(LIS_FUZZ_ASYNC_DELAY_MS));
    }

    // 无 ProfileServiceManager 引导时返回 nullptr，覆盖判空跳过分支
    LisService *lisServiceStatic = LisService::GetService();
    if (lisServiceStatic == nullptr) {
        HILOGI("LisServerFuzzTest GetService is nullptr, skip");
    } else {
        lisServiceStatic->GetConnectState();
    }

    // 已注册态下的重复注册/注销分支
    LisServer::GetInstance().RegisterLisServerApplication();
    std::this_thread::sleep_for(std::chrono::milliseconds(LIS_FUZZ_ASYNC_DELAY_MS));
    LisServer::GetInstance().DeregisterLisServerApplication();
    g_threadUtil.ClearThreadStateMap();
}

// 强驱动段：新增 FDP 消费全部追加在旧消费之后（旧消费为空），旧种子语义不变
void DoStrongDriveLisService(FuzzedDataProvider &fdp)
{
    uint8_t enableRepeat = fdp.ConsumeIntegral<uint8_t>() % LIS_FUZZ_REPEAT_TIMES;
    uint8_t disableRepeat = fdp.ConsumeIntegral<uint8_t>() % LIS_FUZZ_REPEAT_TIMES;
    RawAddress device = BuildRawAddress(fdp);
    DriveLisService(device, enableRepeat, disableRepeat);
}
}

void DoSomethingLisServerWithMyAPI(const uint8_t* fuzzData, size_t size)
{
    /* 先开线程状态表：旧段 Register/Deregister 经 DoInLisThread 投递，表未开时任务被丢弃 */
    g_threadUtil.InitThreadStateMap();
    FuzzedDataProvider fdp(fuzzData, size);
    LisServer::GetInstance().RegisterLisServerApplication();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::LIS_SERVER_PROFILE_FUZZ_DELAY_MS));
    LisServer::GetInstance().DeregisterLisServerApplication();
    DoStrongDriveLisService(fdp);
}
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    int ret = slem_initialize();
    HILOGI("LisServerFuzzTest slem_initialize %{public}d", ret);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    (void) size;
    if (data == nullptr) {
        return 0;
    }
    OHOS::DoSomethingLisServerWithMyAPI(data, size);
    return 0;
}
