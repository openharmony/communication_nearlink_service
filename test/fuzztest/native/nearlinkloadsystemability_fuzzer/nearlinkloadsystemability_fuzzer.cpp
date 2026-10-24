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

#include "nearlinkloadsystemability_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "log.h"
#include "securec.h"
#include "nearlink_advertiser_def.h"
#include "nearlink_sa_manager.h"
#include "nearlink_def_types.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr uint8_t REGISTER_INFO_MAX = 8;
constexpr const char *FUZZ_PROFILE_NAME = "FuzzProfile";
}

/* NearlinkSaManager 注册/注销/状态分发组合：无 SA 环境下 RegisterFunc 走订阅超时分支（50ms 有界等待）
 * 后转 STOPPED，OnStateChanged 经 GetRemoteProfile 判空短路，回调计数仅日志验证。
 * 注：serviceStarted/StoppedFunc 仅由 OnServiceStarted/OnServiceStopped（SA 订阅回调，私有）触发，
 * 无真实 SA 时不可达；stateOnFunc 需 remote 非空（GetRemoteProfile 判空），无 SA 时仅 stateOffFunc 可达。 */
void SaManagerFuzzTest(FuzzedDataProvider &fdp)
{
    NearlinkSaManager &saManager = NearlinkSaManager::GetInstance();
    int32_t profileIds[REGISTER_INFO_MAX] = {};
    int startedCount = 0;
    int stoppedCount = 0;
    int stateOnCount = 0;
    int stateOffCount = 0;

    /* 构造 N 个注册信息（四 lambda 计数），逐个注册；profileId 由 RegisterFunc 分配返回 */
    uint8_t infoCount = fdp.ConsumeIntegral<uint8_t>() % REGISTER_INFO_MAX; // 0..7
    int registered = 0;
    for (uint8_t i = 0; i < infoCount; i++) {
        std::shared_ptr<NearlinkRegisterInfo> info =
            std::make_shared<NearlinkRegisterInfo>(FUZZ_PROFILE_NAME + std::to_string(i));
        info->serviceStartedFunc_ = [&startedCount](sptr<IRemoteObject> remote) -> void {
            (void)remote;
            startedCount++;
        };
        info->serviceStoppedFunc_ = [&stoppedCount]() -> void {
            stoppedCount++;
        };
        info->stateOnFunc_ = [&stateOnCount](sptr<IRemoteObject> remote) -> void {
            (void)remote;
            stateOnCount++;
        };
        info->stateOffFunc_ = [&stateOffCount](sptr<IRemoteObject> remote) -> void {
            (void)remote;
            stateOffCount++;
        };
        profileIds[i] = saManager.RegisterFunc(info);
        if (profileIds[i] != INVALID_PROFILE_ID) {
            registered++;
        }
    }
    HILOGI("infoCount(%{public}u) registered(%{public}d)", infoCount, registered);

    /* OnStateChanged 两轮：transport 命中 SLE 分支/随机（FDP），status 取 TURN_ON/TURN_OFF 两态 */
    for (int round = 0; round < 2; round++) {
        bool useSleTransport = fdp.ConsumeBool();
        int32_t transportFuzz = fdp.ConsumeIntegral<int32_t>();
        int32_t transport = useSleTransport ? static_cast<int32_t>(SleTransport::ADAPTER_SLE) : transportFuzz;
        int32_t status = (fdp.ConsumeIntegral<uint8_t>() % 2 == 0)
            ? static_cast<int32_t>(SleStateID::STATE_TURN_ON)
            : static_cast<int32_t>(SleStateID::STATE_TURN_OFF);
        saManager.OnStateChanged(transport, status);
    }

    /* 注销部分已注册 profileId + 一个 FDP 随机 id（未命中分支），再触发一次：已注销不再分发 */
    uint8_t deregCount = fdp.ConsumeIntegral<uint8_t>() % REGISTER_INFO_MAX;
    for (uint8_t i = 0; i < deregCount && i < infoCount; i++) {
        saManager.DeregisterFunc(profileIds[i]);
    }
    saManager.DeregisterFunc(fdp.ConsumeIntegral<int32_t>()); // 随机未命中 id
    saManager.OnStateChanged(static_cast<int>(SleTransport::ADAPTER_SLE),
        static_cast<int>(SleStateID::STATE_TURN_OFF));

    /* 收尾全量注销：lambda 按引用捕获本函数栈计数器，注册表是进程级单例，
     * 残留注册会在下一输入的 OnStateChanged 里悬垂调用（stack-use-after-return） */
    for (uint8_t i = 0; i < infoCount; i++) {
        if (profileIds[i] != INVALID_PROFILE_ID) {
            saManager.DeregisterFunc(profileIds[i]);
        }
    }

    /* GetRemoteProfile：无 SA 环境判空返回 */
    std::string profileName = fdp.ConsumeRandomLengthString(32);
    sptr<IRemoteObject> remote = saManager.GetRemoteProfile(profileName);
    HILOGI("counts started(%{public}d) stopped(%{public}d) stateOn(%{public}d) stateOff(%{public}d) "
        "remoteNull(%{public}d)",
        startedCount, stoppedCount, stateOnCount, stateOffCount, remote == nullptr ? 1 : 0);
}

bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
{
    HILOGI("start");
    FuzzedDataProvider provider(data, size);

    AdvertiserData advertiserData;
    advertiserData.AddManufacturerData(provider.ConsumeIntegral<uint16_t>(), BuildAddressString(provider));
    advertiserData.GetManufacturerData();
    advertiserData.GetServiceData();
    advertiserData.GetServiceUuids();
    advertiserData.GetAdvFlag();
    advertiserData.GetPayload();
    advertiserData.SetAdvFlag(provider.ConsumeIntegral<uint8_t>());
    advertiserData.SetPayload(BuildAddressString(provider));
    advertiserData.AddManufacturerData(provider.ConsumeIntegral<uint16_t>(), BuildAddressString(provider));
    advertiserData.AddServiceData(BuildUuid(provider), BuildAddressString(provider));
    advertiserData.AddServiceUuid(BuildUuid(provider));
    advertiserData.GetIncludeDeviceName();
    advertiserData.SetIncludeDeviceName(provider.ConsumeBool());
    advertiserData.GetIncludeTxPower();
    advertiserData.SetIncludeTxPower(provider.ConsumeBool());

    SaManagerFuzzTest(provider);
    return true;
}
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        HILOGI("invalid data");
        return 0;
    }

    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}

