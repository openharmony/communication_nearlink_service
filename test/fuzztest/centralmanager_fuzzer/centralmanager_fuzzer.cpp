/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "centralmanager_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_sle_central_manager_stub.h"
#include "nearlink_sle_central_manager_server.h"
#include "nearlink_host_server.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

class MockNearlinkSleCentralManagerStub : public IRemoteStub<INearlinkSleCentralManagerCallback> {
public:
    void OnScanCallback(const NearlinkSleScanResult &result) {}
    void OnSleBatchScanResultsEvent(std::vector<NearlinkSleScanResult> &results) {}
    void OnStartOrStopScanEvent(int resultCode, bool isStartScan) {}
};

namespace {
constexpr int HOST_FUZZ_DELAY_50_MS = 50;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
sptr<NearlinkSleCentralManagerServer> g_centralManager = new (std::nothrow) NearlinkSleCentralManagerServer();
sptr<INearlinkSleCentralManagerCallback> g_centralManagerCb = new (std::nothrow) MockNearlinkSleCentralManagerStub();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
uint32_t g_scannerId = 0;
}

int32_t CentralManagerOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_centralManager, TRANSACTION_ERR, "g_centralManager is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("CentralManagerOnRemoteRequest, cmd(%{public}d)", code);
    return g_centralManager->OnRemoteRequest(code, data, reply, option);
}

void RegisterSleCentralManagerCallbackFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    if (g_centralManagerCb != nullptr) {
        data.WriteRemoteObject(g_centralManagerCb->AsObject());
    }
    data.WriteBool(provider.ConsumeBool());

    int32_t ret = CentralManagerOnRemoteRequest(
        NearlinkSleCentralManagerInterfaceCode::SLE_REGISTER_SLE_CENTRAL_MANAGER_CALLBACK, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
        return;
    }
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        g_scannerId = reply.ReadUint32();
        HILOGI("g_scannerId(%{public}d)", g_scannerId);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void DeRegisterSleCentralManagerCallbackFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteInt32(g_scannerId); // scannerId
    if (g_centralManagerCb != nullptr) {
        data.WriteRemoteObject(g_centralManagerCb->AsObject());
    }

    int32_t ret = CentralManagerOnRemoteRequest(
        NearlinkSleCentralManagerInterfaceCode::SLE_DE_REGISTER_SLE_CENTRAL_MANAGER_CALLBACK, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void StartFullScanInnerFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteUint32(g_scannerId); // scannerId
    NearlinkSleScanSettings settings;
    settings.SetReportDelay(provider.ConsumeIntegral<long>());
    settings.SetScanMode(provider.ConsumeIntegral<int>());
    settings.SetLegacy(provider.ConsumeBool());
    settings.SetPhy(provider.ConsumeIntegral<int>());
    data.WriteParcelable(&settings);

    int32_t ret = CentralManagerOnRemoteRequest(
        NearlinkSleCentralManagerInterfaceCode::SLE_START_FULL_SCAN, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void StartScanWithFilterFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteUint32(g_scannerId); // scannerId
    NearlinkSleScanSettings settings;
    settings.SetReportDelay(provider.ConsumeIntegral<long>());
    settings.SetScanMode(provider.ConsumeIntegral<int>());
    settings.SetLegacy(provider.ConsumeBool());
    settings.SetPhy(provider.ConsumeIntegral<int>());
    data.WriteParcelable(&settings);
    data.WriteUint32(1); // filter size
    NearlinkSleScanFilter scanFilter;
    scanFilter.SetDeviceId("");
    scanFilter.SetName("test");
    data.WriteParcelable(&scanFilter);

    int32_t ret = CentralManagerOnRemoteRequest(
        NearlinkSleCentralManagerInterfaceCode::SLE_START_SCAN_WITH_FILTER, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void StopScanFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleCentralManagerStub::GetDescriptor());
    data.WriteUint32(g_scannerId); // scannerId

    int32_t ret = CentralManagerOnRemoteRequest(
        NearlinkSleCentralManagerInterfaceCode::SLE_STOP_SCAN, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
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
    HILOGI("advertiser_fuzzer OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("advertiser_fuzzer EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        OHOS::g_threadUtil.ClearThreadStateMap();
        return 0;
    }

    /* Run your code on data */
    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::RegisterSleCentralManagerCallbackFuzzTest(data, size);
    OHOS::StartFullScanInnerFuzzTest(data, size);
    OHOS::StartScanWithFilterFuzzTest(data, size);
    OHOS::StopScanFuzzTest();
    OHOS::DeRegisterSleCentralManagerCallbackFuzzTest();
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}

