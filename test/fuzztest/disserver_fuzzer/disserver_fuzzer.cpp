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
#include "disserver_fuzzer.h"
#include <thread>
#include "fuzzer/FuzzedDataProvider.h"
#include "../nl_utils/fuzztest_utils.h"
#include "DisServerStackAdapter.h"
#include "DisClientStackAdapter.h"
#include "DisService.h"
#include "ThreadUtil.h"
#include "log.h"
#include "slem.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr int DIS_SERVER_PROFILE_FUZZ_DELAY_MS = 5000;
constexpr uint8_t DIS_FUZZ_STATE_NUM = 5; // SleConnectState 枚举成员数
constexpr uint8_t DIS_FUZZ_REPEAT_TIMES = 3; // Enable/Disable 幂等重复次数上限
constexpr size_t DIS_FUZZ_SERVICE_DATA_LEN = 64; // GetDisServiceData 预置数据长度
constexpr int DIS_FUZZ_ASYNC_DELAY_MS = 50; // 等待 DoInDisThread 异步事件落定
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();

class FuzzDisObserver : public DisObserver {
public:
    void OnConnectionStateChanged(const RawAddress &deviceAddress, int state, int oldState) override
    {
        HILOGI("DisServerFuzzTest OnConnectionStateChanged state(%{public}d) oldState(%{public}d)", state, oldState);
    }
};

// DisService 全生命周期 + DisClientStackAdapter 直调驱动
void DriveDisService(const RawAddress &device, SleConnectState curState, SleConnectState preState,
    std::string &serviceData, uint8_t enableRepeat, uint8_t disableRepeat)
{
    // promise 型 getter 依赖线程状态表：先初始化，收尾清理，否则永久挂死
    g_threadUtil.InitThreadStateMap();
    {
        // 每次输入新建/析构 DisService，模拟完整服务生命周期
        DisService disService;
        FuzzDisObserver observer;
        disService.RegisterObserver(observer);
        disService.Enable();
        disService.GetConnectState();
        disService.GetConnectDevices();
        disService.Connect(device);
        disService.Disconnect(device);
        // 随机(含非法)状态组合
        disService.NotifyStateChanged(device, curState, preState);
        // 合法状态组合：CONNECTED<-CONNECTING 触发设备信息保存分支
        disService.NotifyStateChanged(device, SleConnectState::CONNECTED, SleConnectState::CONNECTING);
        disService.GetDeviceVendorId(device);
        disService.GetDeviceProductId(device);
        disService.GetDeviceVersion(device);
        disService.GetDeviceInformation(device);
        disService.GetDisServiceData(serviceData);
        disService.GetDisServiceAppearance();
        disService.Disable();
        disService.DeregisterObserver(observer);
        // Enable/Disable 重复幂等分支
        for (uint8_t i = 0; i <= enableRepeat; ++i) {
            disService.Enable();
        }
        for (uint8_t i = 0; i <= disableRepeat; ++i) {
            disService.Disable();
        }
        // Enable->Disable->Enable 交替
        disService.Enable();
        disService.Disable();
        disService.Enable();
        // 析构前等待 Dis 线程残留任务执行完，避免任务中的 this 悬垂
        std::this_thread::sleep_for(std::chrono::milliseconds(DIS_FUZZ_ASYNC_DELAY_MS));
    }

    // 无 ProfileServiceManager 引导时返回 nullptr，覆盖判空跳过分支
    DisService *disServiceStatic = DisService::GetDisService();
    if (disServiceStatic == nullptr) {
        HILOGI("DisServerFuzzTest GetDisService is nullptr, skip");
    } else {
        disServiceStatic->GetConnectState();
    }

    // DisClientStackAdapter 直调全方法（栈侧已由 slem_initialize 初始化）
    DisClientStackAdapter clientAdapter;
    clientAdapter.RegisteCallBackToStack();
    clientAdapter.Connect(device);
    clientAdapter.Disconnect(device);
    clientAdapter.GetDeviceVendorId(device);
    clientAdapter.GetDeviceProductId(device);
    clientAdapter.GetDeviceVersion(device);
    clientAdapter.GetAppearanceInfo(device);
    clientAdapter.GetNameInfo(device);
    DeviceInformation information = {};
    clientAdapter.GetDeviceInformation(device, information);
    std::this_thread::sleep_for(std::chrono::milliseconds(DIS_FUZZ_ASYNC_DELAY_MS));
    g_threadUtil.ClearThreadStateMap();
}

// 强驱动段：新增 FDP 消费全部追加在旧消费之后，旧种子语义不变
void DoStrongDriveDisService(FuzzedDataProvider &fdp)
{
    RawAddress device = BuildRawAddress(fdp);
    SleConnectState curState = static_cast<SleConnectState>(fdp.ConsumeIntegral<uint8_t>() % DIS_FUZZ_STATE_NUM);
    SleConnectState preState = static_cast<SleConnectState>(fdp.ConsumeIntegral<uint8_t>() % DIS_FUZZ_STATE_NUM);
    uint8_t enableRepeat = fdp.ConsumeIntegral<uint8_t>() % DIS_FUZZ_REPEAT_TIMES;
    uint8_t disableRepeat = fdp.ConsumeIntegral<uint8_t>() % DIS_FUZZ_REPEAT_TIMES;
    std::string serviceData = fdp.ConsumeRandomLengthString(DIS_FUZZ_SERVICE_DATA_LEN);
    DriveDisService(device, curState, preState, serviceData, enableRepeat, disableRepeat);
}
}

void DoSomethingDisServerWithMyAPI(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    DisServerInfo info;
    info.appearanceId_ = fdp.ConsumeIntegral<uint32_t>();
    info.manufactureInfo_ = fdp.ConsumeRandomLengthString(64);
    info.nameInfo_ = fdp.ConsumeRandomLengthString(64);
    info.versionInfo_ = fdp.ConsumeRandomLengthString(16);
    DisServerStackAdapter disServer;
    disServer.CreateServerInfo(info);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::DIS_SERVER_PROFILE_FUZZ_DELAY_MS));
    disServer.DestroyServerInfo();
    DoStrongDriveDisService(fdp);
}
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    int ret = slem_initialize();
    HILOGI("DisServerFuzzTest slem_initialize %{public}d", ret);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    (void) size;
    if (data == nullptr) {
        return 0;
    }
    OHOS::DoSomethingDisServerWithMyAPI(data, size);
    return 0;
}
