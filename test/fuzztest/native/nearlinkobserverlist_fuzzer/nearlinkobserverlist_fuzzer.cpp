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

#include "nearlinkobserverlist_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "log.h"
#include "securec.h"
#include "nearlink_advertiser_def.h"
#include "BaseObserverList.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr uint8_t OBSERVER_POOL_SIZE = 8;
}

/* 本地观察者基类：虚 OnEvent(int) + 计数成员（tag 由 FDP 填充），配两个派生实现驱动分发 */
class FuzzLocalObserver {
public:
    FuzzLocalObserver() = default;
    explicit FuzzLocalObserver(int tag) : tag_(tag) {}
    virtual ~FuzzLocalObserver() = default;

    virtual void OnEvent(int event)
    {
        (void)event;
        eventCount_++;
    }

    int GetEventCount() const
    {
        return eventCount_;
    }

    int GetTag() const
    {
        return tag_;
    }

private:
    int tag_ = 0;
    int eventCount_ = 0;
};

class FuzzLocalObserverA final : public FuzzLocalObserver {
public:
    using FuzzLocalObserver::FuzzLocalObserver;
    void OnEvent(int event) override
    {
        FuzzLocalObserver::OnEvent(event);
    }
};

class FuzzLocalObserverB final : public FuzzLocalObserver {
public:
    using FuzzLocalObserver::FuzzLocalObserver;
    void OnEvent(int event) override
    {
        FuzzLocalObserver::OnEvent(event);
    }
};

/* SVB-02：注册/注销/通知分发组合与边界（空注册/重复注册/注销后通知），分发验证用计数+日志，不 assert */
void BaseObserverListFuzzTest(FuzzedDataProvider &fdp)
{
    BaseObserverList<FuzzLocalObserver> observerList;
    FuzzLocalObserverA obsA(fdp.ConsumeIntegral<int>());
    FuzzLocalObserverB obsB(fdp.ConsumeIntegral<int>());
    FuzzLocalObserverB obsC(fdp.ConsumeIntegral<int>());

    int firstRet = observerList.Register(obsA) ? 1 : 0;     // 首次注册：期望 true
    int dupRet = observerList.Register(obsA) ? 1 : 0;       // 重复注册同一对象：期望 false
    int notRegRet = observerList.Deregister(obsB) ? 1 : 0;  // 注销未注册对象：期望 false
    (void)observerList.Register(obsB);
    (void)observerList.Register(obsC);
    (void)observerList.Deregister(obsA);                    // 注销 obsA 后分发：期望 0 回调
    int emptyDispatch = 0;
    observerList.ForEach([&emptyDispatch](FuzzLocalObserver &obs) -> void {
        obs.OnEvent(0);
        emptyDispatch++;
    });
    HILOGI("first(%{public}d) dup(%{public}d) notReg(%{public}d) dispatchAfterDereg(%{public}d) "
        "tags(%{public}d/%{public}d/%{public}d)",
        firstRet, dupRet, notRegRet, emptyDispatch, obsA.GetTag(), obsB.GetTag(), obsC.GetTag());

    /* 逐个注册 N 个（N 由 FDP 决定）后每轮分发计数 */
    uint8_t registerCount = fdp.ConsumeIntegral<uint8_t>() % 9; // 0..8
    FuzzLocalObserverB observerPool[OBSERVER_POOL_SIZE];
    int registered = 0;
    for (uint8_t i = 0; i < registerCount && i < OBSERVER_POOL_SIZE; i++) {
        registered += observerList.Register(observerPool[i]) ? 1 : 0;
    }
    int dispatched = 0;
    observerList.ForEach([&dispatched](FuzzLocalObserver &obs) -> void {
        obs.OnEvent(1);
        dispatched++;
    });
    HILOGI("registerCount(%{public}u) registered(%{public}d) dispatched(%{public}d)",
        registerCount, registered, dispatched);

    /* 交替注销再分发，注销哪个观察者由 FDP 决定 */
    uint8_t rounds = fdp.ConsumeIntegral<uint8_t>() % 4; // 0..3
    for (uint8_t r = 0; r < rounds; r++) {
        uint8_t idx = fdp.ConsumeIntegral<uint8_t>() % OBSERVER_POOL_SIZE;
        int deregRet = observerList.Deregister(observerPool[idx]) ? 1 : 0;
        int remain = 0;
        observerList.ForEach([&remain](FuzzLocalObserver &obs) -> void {
            obs.OnEvent(2);
            remain++;
        });
        HILOGI("round(%{public}u) deregIdx(%{public}u) ret(%{public}d) remain(%{public}d)",
            r, idx, deregRet, remain);
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

    BaseObserverListFuzzTest(provider);
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

