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
#include <vector>
#include "fuzzer/FuzzedDataProvider.h"
#include "icceservice_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_host_server.h"
// 栈侧 icce_utils（C 接口）须先于 IcceServerStackAdapter.h：两份 icce_utils.h 同用 ICCE_UTILS_H
// include guard，先含栈侧才能保住 IcceConvert* 声明（service 侧本驱动未用其声明）
#include "src/icce_type.h"
#include "src/icce_utils.h"
#include "IcceService.h"
#include "IcceServerStackAdapter.h"
#include "ThreadUtil.h"
#include "log.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr int ICCE_FUZZ_DELAY_50_MS = 50;   // 收敛 DoInIcceThread 异步任务
constexpr int ICCE_FUZZ_DELAY_100_MS = 100; // init OnStart 后等待
constexpr int ICCE_FUZZ_DELAY_5000_MS = 5000; // init EnableSle 后等待（基线判例）
constexpr uint8_t ICCE_FUZZ_LIFECYCLE_ROUNDS = 2; // 生命周期重复轮数上限
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

class FuzzIcceObserver : public IcceObserver {
public:
    void OnConnectionStateChanged(const RawAddress &device, int curState, int prevState) override {}
};

/* IcceService 全生命周期直调：公有构造，栈上实例覆盖 ctor/dtor + Enable/Disable 幂等分支 */
void DriveIcceService(IcceService &icceSvc, const RawAddress &device)
{
    FuzzIcceObserver icceObserver;
    icceSvc.RegisterObserver(icceObserver);

    icceSvc.Enable();  // EnableTask：RegisterCallBackToStack + CreateServerInfo
    icceSvc.Enable();  // 已使能幂等分支
    icceSvc.Connect(device);     // DftCacheIcceStart + ConnectTask（连接数/支持性校验分支）
    icceSvc.Disconnect(device);
    icceSvc.GetPort(device);     // promise 型 getter（线程状态表默认 ENABLED）
    icceSvc.GetConnectState();
    icceSvc.GetConnectDevices();
    icceSvc.GetContext();
    icceSvc.Disable();  // DisableTask：DestroyServerInfo
    icceSvc.Disable();  // 已去使能幂等分支

    icceSvc.DeregisterObserver(icceObserver);
}

/* 局部生命周期段：构造/驱动/等待落定/析构（DoInIcceThread 残留任务防 this 悬垂） */
void IcceLocalLifecycle(const RawAddress &device)
{
    IcceService icceService;
    DriveIcceService(icceService, device);
    std::this_thread::sleep_for(std::chrono::milliseconds(ICCE_FUZZ_DELAY_50_MS));
}

/* IcceServerStackAdapter 全公开方法直调（Create/Destroy 成败两分支由栈侧初始化状态决定） */
void DriveIcceServerAdapter()
{
    IcceServerStackAdapter serverAdapter;
    serverAdapter.CreateServerInfo();
    serverAdapter.DestroyServerInfo();
}

/* 栈侧 icce_utils.c 四个纯函数直调（合法/越界 uuid 字节与地址比较两种结果） */
void DriveIcceUtils(FuzzedDataProvider &fdp)
{
    NLSTK_SsapUuid_S uuidStru = {};
    std::vector<uint8_t> uuidBytes = fdp.ConsumeBytes<uint8_t>(sizeof(uuidStru.uuid));
    for (size_t i = 0; i < uuidBytes.size(); ++i) {
        uuidStru.uuid[i] = uuidBytes[i];
    }
    uint16_t uuid16 = IcceConvertUuidTo16Bits(uuidStru);

    NLSTK_SsapUuid_S uuidStru2 = IcceConvertUuidToStru(uuid16);
    uint16_t uuid16Round = IcceConvertUuidTo16Bits(uuidStru2);
    HILOGI("IcceFuzzTest uuid round-trip %{public}u -> %{public}u", uuid16, uuid16Round);

    IcceDevice_S dev = IcceDevice_S{};
    dev.appId = fdp.ConsumeIntegral<int32_t>();
    int32_t appIdSame = dev.appId;
    int32_t appIdDiff = appIdSame + 1;
    IcceCompAppId(&dev, &appIdSame);
    IcceCompAppId(&dev, &appIdDiff);

    dev.addr.type = fdp.ConsumeIntegral<uint8_t>();
    for (size_t i = 0; i < SLE_ADDR_LEN; ++i) {
        dev.addr.addr[i] = fdp.ConsumeIntegral<uint8_t>();
    }
    IcceCompAddr(&dev, &dev.addr);  // 自比较相等分支
    SLE_Addr_S diffAddr = dev.addr;
    diffAddr.addr[0] ^= 0xFF;
    IcceCompAddr(&dev, &diffAddr);  // 不等分支
}

void IcceServiceFuzzTestInner(FuzzedDataProvider &fdp)
{
    HILOGI("IcceFuzzTest,start run IcceServiceFuzzTestInner");
    RawAddress device = BuildRawAddress(fdp);
    // 生命周期重复 1-2 轮：二轮 Enable 在已 Disable 状态上重建 server 实例
    uint8_t lifecycleRepeat = fdp.ConsumeIntegral<uint8_t>() % ICCE_FUZZ_LIFECYCLE_ROUNDS;
    for (uint8_t i = 0; i <= lifecycleRepeat; ++i) {
        IcceLocalLifecycle(device);
    }

    // 无 ProfileServiceManager 引导时返回 nullptr，覆盖判空跳过分支；有则轻驱管理器实例
    IcceService *icceServiceStatic = IcceService::GetService();
    if (icceServiceStatic == nullptr) {
        HILOGI("IcceFuzzTest GetService is nullptr, skip");
    } else {
        icceServiceStatic->GetConnectState();
        icceServiceStatic->GetPort(device);
    }

    DriveIcceServerAdapter();
    DriveIcceUtils(fdp);
    std::this_thread::sleep_for(std::chrono::milliseconds(ICCE_FUZZ_DELAY_50_MS));
}
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
    HILOGI("IcceFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::ICCE_FUZZ_DELAY_100_MS));
    HILOGI("IcceFuzzTest EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::ICCE_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    FuzzedDataProvider fdp(data, size);
    OHOS::IcceServiceFuzzTestInner(fdp);
    return 0;
}
