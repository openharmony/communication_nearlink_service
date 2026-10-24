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

#include "nearlinkhostloadcallback_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "log.h"
#include "securec.h"
#include "nearlink_advertiser_def.h"
#include "nearlink_host_load_callback.h"
#include "nearlink_host.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

/* NearlinkHostLoadCallBack 转发 NearlinkHost::LoadSystemAbilitySuccess/Fail：
 * 实现仅 proxyConVar_.notify_one()，不解引用 remoteObject（空 sptr 安全）且幂等（重复调用安全）。
 * 空 remote 用默认构造 sptr 临时对象传入（与 nullptr 同值） */
void HostLoadCallBackFuzzTest(FuzzedDataProvider &fdp)
{
    sptr<NearlinkHostLoadCallBack> loadCallBack = new (std::nothrow) NearlinkHostLoadCallBack();
    if (loadCallBack == nullptr) {
        HILOGI("loadCallBack is nullptr, skip");
        return;
    }

    loadCallBack->OnLoadSystemAbilitySuccess(fdp.ConsumeIntegral<int32_t>(), sptr<IRemoteObject>());
    loadCallBack->OnLoadSystemAbilityFail(fdp.ConsumeIntegral<int32_t>());

    /* 重复/交替 x N（N 由 FDP 决定），重复对同 id 调两次 */
    uint8_t rounds = fdp.ConsumeIntegral<uint8_t>() % 4; // 0..3
    for (uint8_t r = 0; r < rounds; r++) {
        if (fdp.ConsumeBool()) {
            int32_t successId = fdp.ConsumeIntegral<int32_t>();
            loadCallBack->OnLoadSystemAbilitySuccess(successId, sptr<IRemoteObject>());
            loadCallBack->OnLoadSystemAbilitySuccess(successId, sptr<IRemoteObject>()); // 重复
        } else {
            int32_t failId = fdp.ConsumeIntegral<int32_t>();
            loadCallBack->OnLoadSystemAbilityFail(failId);
            loadCallBack->OnLoadSystemAbilityFail(failId); // 重复
        }
    }

    /* NearlinkHost 单例直调同一序列 */
    NearlinkHost &host = NearlinkHost::GetInstance();
    host.LoadSystemAbilitySuccess(sptr<IRemoteObject>());
    host.LoadSystemAbilityFail();
    uint8_t hostRounds = fdp.ConsumeIntegral<uint8_t>() % 4; // 0..3
    for (uint8_t r = 0; r < hostRounds; r++) {
        if (fdp.ConsumeBool()) {
            host.LoadSystemAbilitySuccess(sptr<IRemoteObject>());
            host.LoadSystemAbilitySuccess(sptr<IRemoteObject>()); // 重复
        } else {
            host.LoadSystemAbilityFail();
            host.LoadSystemAbilityFail(); // 重复
        }
    }
    HILOGI("HostLoadCallBackFuzzTest done, rounds(%{public}u) hostRounds(%{public}u)", rounds, hostRounds);
}

bool DoSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
{
    HILOGI("start");
    FuzzedDataProvider provider(data, size);

    AdvertiserData advertiserData;
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

    HostLoadCallBackFuzzTest(provider);
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

