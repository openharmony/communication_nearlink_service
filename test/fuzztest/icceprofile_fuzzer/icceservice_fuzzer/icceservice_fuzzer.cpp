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

#include "icceservice_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "../../nl_utils/fuzztest_utils.h"
#include "IcceService.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"
#include "ThreadUtil.h"
#include "slem.h"
#include <thread>

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
IcceService *g_icceService = new (std::nothrow) IcceService();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
constexpr int MESSAGE_BUTT = 3;
constexpr int DIS_SERVICE_FUZZ_DELAY_MS = 5000;
constexpr int ICCE_FUZZ_DELAY_50_MS = 50;
}

class IcceCallbackTest : public IcceObserver {
public:
    ~IcceCallbackTest() = default;
    void OnConnectionStateChanged(const RawAddress &device, int curState, int prevState) override {}
};

void DoSomethingInterestingWithIcceServiceAPI(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);

    auto addr = BuildAddressString(provider);
    RawAddress rawAddr(addr.c_str());
    IcceCallbackTest icceObserver;

    if (g_icceService == nullptr) {
        HILOGE("icceService is nullptr");
        return;
    }
    g_icceService->GetService();
    g_icceService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::DIS_SERVICE_FUZZ_DELAY_MS));
    g_icceService->Connect(rawAddr);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::DIS_SERVICE_FUZZ_DELAY_MS));
    g_icceService->GetContext();
    g_icceService->GetConnectDevices();
    g_icceService->GetConnectState();
    g_icceService->RegisterObserver(icceObserver);
    g_icceService->DeregisterObserver(icceObserver);
    g_icceService->GetPort(rawAddr);
    g_icceService->Disconnect(rawAddr);
    g_icceService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::ICCE_FUZZ_DELAY_50_MS));
}
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    int ret = slem_initialize();
    HILOGI("IcceServiceFuzzTest slem_initialize %{public}d", ret);
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        OHOS::g_threadUtil.ClearThreadStateMap();
        return 0;
    }

    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::DoSomethingInterestingWithIcceServiceAPI(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
