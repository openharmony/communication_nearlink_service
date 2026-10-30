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

#include "nearlinkobservermap_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "log.h"
#include "securec.h"
#include "nearlink_advertiser_def.h"
#include "remote_observer_list.h"
#include "iremote_stub.h"
#include "i_nearlink_hadm_client_callback.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr uint8_t OBSERVER_TOTAL = 3;
}

/* 本地 mock stub：RemoteObserverList<T> 要求 T 实现 IRemoteBroker（AsObject） */
class MockFuzzHadmClientCallbackStub : public IRemoteStub<INearlinkHadmClientCallback> {
public:
    MockFuzzHadmClientCallbackStub() = default;
    ~MockFuzzHadmClientCallbackStub() override = default;

    void OnSoundingResult(const NearlinkRawAddress &addr, const NearlinkHadmClientSoundingResult &result) override
    {
        (void)addr;
        (void)result;
    }

    void OnSoundingStateChange(const NearlinkRawAddress &addr, int newState, int errorCode) override
    {
        (void)addr;
        (void)newState;
        (void)errorCode;
    }
};

/* SVB-02：远端观察者 map 注册/注销/分发组合与边界，Size 前后对比，验证用计数+日志 */
void RemoteObserverListFuzzTest(FuzzedDataProvider &fdp)
{
    /* 空 sptr 注册：产品 Register 未判空即 observer->AsObject()（remote_observer_list.h Register），
     * 会解引用空指针，跳过该分支仅记录 */
    RemoteObserverList<INearlinkHadmClientCallback> observerMap;
    sptr<MockFuzzHadmClientCallbackStub> cbObservers[OBSERVER_TOTAL] = {};
    for (uint8_t i = 0; i < OBSERVER_TOTAL; i++) {
        cbObservers[i] = new (std::nothrow) MockFuzzHadmClientCallbackStub();
        if (cbObservers[i] == nullptr) {
            HILOGI("observer(%{public}u) alloc failed, skip", i);
            return;
        }
    }

    int sizeBefore = observerMap.Size();
    int firstRet = observerMap.Register(cbObservers[0]) ? 1 : 0;    // 首次注册：期望 true
    int dupRet = observerMap.Register(cbObservers[0]) ? 1 : 0;      // 重复注册：期望 false
    int notRegRet = observerMap.Deregister(cbObservers[1]) ? 1 : 0; // 注销未注册：期望 false
    (void)observerMap.Register(cbObservers[1]);
    (void)observerMap.Register(cbObservers[2]);
    int forEachCount = 0;
    observerMap.ForEach([&forEachCount](sptr<INearlinkHadmClientCallback> obs) -> void {
        (void)obs;
        forEachCount++;
    });
    (void)observerMap.Deregister(cbObservers[1]);
    int afterDeregCount = 0;
    observerMap.ForEach([&afterDeregCount](sptr<INearlinkHadmClientCallback> obs) -> void {
        (void)obs;
        afterDeregCount++;
    });
    int sizeEnd = observerMap.Size();
    HILOGI("sizeBefore(%{public}d) first(%{public}d) dup(%{public}d) notReg(%{public}d) "
        "forEach(%{public}d) afterDereg(%{public}d) sizeEnd(%{public}d)",
        sizeBefore, firstRet, dupRet, notRegRet, forEachCount, afterDeregCount, sizeEnd);

    /* FDP 决定对哪个观察者注册/注销，逐轮对比 Size 与分发计数 */
    uint8_t rounds = fdp.ConsumeIntegral<uint8_t>() % 5; // 0..4
    for (uint8_t r = 0; r < rounds; r++) {
        uint8_t idx = fdp.ConsumeIntegral<uint8_t>() % OBSERVER_TOTAL;
        bool doRegister = fdp.ConsumeBool();
        int ret = doRegister ? (observerMap.Register(cbObservers[idx]) ? 1 : 0)
                             : (observerMap.Deregister(cbObservers[idx]) ? 1 : 0);
        int forEachRound = 0;
        observerMap.ForEach([&forEachRound](sptr<INearlinkHadmClientCallback> obs) -> void {
            (void)obs;
            forEachRound++;
        });
        HILOGI("round(%{public}u) idx(%{public}u) doRegister(%{public}d) ret(%{public}d) "
            "forEach(%{public}d) size(%{public}d)",
            r, idx, doRegister ? 1 : 0, ret, forEachRound, observerMap.Size());
    }
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

    RemoteObserverListFuzzTest(provider);
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

