/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <cstdlib>
#include "fuzzer/FuzzedDataProvider.h"
#include "adaptercfg_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "SleProperties.h"
#include "slem.h"
#include "time_utils.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr int ADAPTERCFG_FUZZ_DELAY_50_MS = 50;
constexpr int SLE_BOND_MODE_BUTT = 2;
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();

// IAdapterSleObserver 各回调在接口头内已有默认空实现，空子类即可作注册观察者
class FuzzSleAdapterObserver : public IAdapterSleObserver {
};

static void SlemDliFailureStub()
{
    return;
}
} // namespace

// A. SleProperties（services/service/src/adapter/SleProperties.cpp）直调：
// 名称/地址/可绑定模式/IO 能力读写全 API + 配置装载/保存 + 观察者注册对。
// SetLocalAddress 在 SleProperties.h 声明但 SleProperties.cpp 未定义，调用会链接失败，不驱动（已登记）。
void SlePropertiesFuzzTest(const uint8_t *fuzzData, size_t size)
{
    SleProperties &properties = SleProperties::GetInstance();
    FuzzedDataProvider fdp(fuzzData, size);

    std::string localName = BuildAddressString(fdp);
    int32_t bondableMode = fdp.ConsumeIntegral<uint8_t>() % SLE_BOND_MODE_BUTT; // 合法/非法两档
    int32_t ioCapability = fdp.ConsumeIntegral<int32_t>();

    (void)properties.GetLocalName();
    (void)properties.SetLocalName(localName);
    (void)properties.GetLocalAddress();
    (void)properties.GetLocalSleAddress();
    (void)properties.GetBondableMode();
    (void)properties.SetBondableMode(bondableMode);
    (void)properties.GetBondableMode();
    (void)properties.GetIoCapability();
    (void)properties.SetIoCapability(ioCapability);
    (void)properties.GetIoCapability();
    (void)properties.GetAddrFromController(); // 内部经 UpdateConfig(SLE_CONFIG_LOCAL_ADDRESS)
    (void)properties.LoadSleConfigInfo();     // 内部经 ReadSleHostInfo
    (void)properties.SaveDefaultValues();

    FuzzSleAdapterObserver observer;
    properties.RegisterSleAdapterObserver(observer);
    properties.DeregisterSleAdapterObserver(observer);
}

// B. slem（services/stack/src/nai/slem/src/slem.c）轻量接口直调；
// slem_initialize/enable/disable/close 有真实栈生命周期副作用，仅在
// LLVMFuzzerInitialize 中一次性成对驱动（见下），不进每轮 input。
void SlemLightFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    SlemCallbacks callbacks;
    callbacks.dliFailure = SlemDliFailureStub;
    if (fdp.ConsumeBool()) {
        slem_registerCallbacks(&callbacks);
    } else {
        slem_deregisterCallbacks();
    }
    slem_deregisterCallbacks();
    (void)slem_is_enabled();
}

// C. time_utils（services/stack/src/utils/src/time_utils.c）直调：纯取时函数，
// 相邻双采样覆盖取值路径（闰年/月末等日历边界由系统时钟侧决定，接口无入参可注入）。
void TimeUtilsFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider fdp(fuzzData, size);
    uint64_t realMs = DP_GetRealTimeMs();
    uint64_t monoMs = DP_GetMonoTimeMs();
    if (fdp.ConsumeBool()) {
        realMs = DP_GetRealTimeMs();
        monoMs = DP_GetMonoTimeMs();
    }
    HILOGI("adaptercfg time sample real=%{public}llu mono=%{public}llu",
        static_cast<unsigned long long>(realMs), static_cast<unsigned long long>(monoMs));
}
} // namespace OHOS

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    // mock token
    NearlinkMockNativeToken mock("nearlink_service");

    // slem 生命周期一次性驱动（actm_fuzzer 同款裸进程基线范式：
    // initialize 走部分初始化+回滚，enable 后立即 disable/close 成对收敛，
    // 均为一次性，不与每轮 input 交叉）。
    (void)slem_initialize();
    (void)slem_enable();
    (void)slem_disable();
    (void)slem_close();
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    uint8_t *fuzzData = static_cast<uint8_t *>(malloc(size));
    if (fuzzData == nullptr) {
        return 0;
    }
    if (memcpy_s(fuzzData, size, data, size) != EOK) {
        free(fuzzData);
        return 0;
    }
    OHOS::SlePropertiesFuzzTest(fuzzData, size);
    OHOS::SlemLightFuzzTest(fuzzData, size);
    OHOS::TimeUtilsFuzzTest(fuzzData, size);
    free(fuzzData);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::ADAPTERCFG_FUZZ_DELAY_50_MS));
    return 0;
}
