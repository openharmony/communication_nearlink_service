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
#include <atomic>
#include "fuzzer/FuzzedDataProvider.h"
#include "advertiser_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_sle_advertiser_stub.h"
#include "nearlink_sle_advertiser_server.h"
#include "nearlink_host_server.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

/* 广播句柄由实现侧异步经 OnStartResultEvent 交付：在此捕获，
 * 供同一次输入内的 SetData/Enable/Disable/Stop 复用（句柄一致）。 */
std::atomic<int32_t> g_advHandle { -1 };
std::atomic<uint32_t> g_advEventSeq { 0 };

class MockNearlinkSleAdvertiseCallbackStub : public IRemoteStub<INearlinkSleAdvertiseCallback> {
public:
    void OnAutoStopAdvEvent(int32_t advHandle)
    {
        (void)advHandle;
        g_advEventSeq.fetch_add(1);
    }
    void OnStartResultEvent(int32_t result, int32_t advHandle, int32_t opcode)
    {
        (void)result;
        (void)opcode;
        g_advHandle.store(advHandle);
        g_advEventSeq.fetch_add(1);
    }
    void OnEnableResultEvent(int32_t result, int32_t advHandle)
    {
        (void)result;
        (void)advHandle;
        g_advEventSeq.fetch_add(1);
    }
    void OnDisableResultEvent(int32_t result, int32_t advHandle)
    {
        (void)result;
        (void)advHandle;
        g_advEventSeq.fetch_add(1);
    }
    void OnStopResultEvent(int32_t result, int32_t advHandle)
    {
        (void)result;
        (void)advHandle;
        g_advEventSeq.fetch_add(1);
    }
    void OnSetAdvDataEvent(int32_t result, int32_t advHandle)
    {
        (void)result;
        (void)advHandle;
        g_advEventSeq.fetch_add(1);
    }
};

/* 广播结果事件异步派发：有界等待序号增长，等不到则用随机句柄走未命中分支 */
bool WaitAdvEvent(uint32_t seqBefore)
{
    for (int waited = 0; waited < 100; waited++) {
        if (g_advEventSeq.load() > seqBefore) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return g_advEventSeq.load() > seqBefore;
}

namespace {
constexpr int HOST_FUZZ_DELAY_10_MS = 50;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
constexpr uint32_t MESSAGE_SIZE = NearlinkSleAdvertiserInterfaceCode::NL_SLE_ADVERTISER_BUTT;
sptr<NearlinkSleAdvertiserServer> g_sleAdvertiser = new (std::nothrow) NearlinkSleAdvertiserServer();
sptr<INearlinkSleAdvertiseCallback> g_sleAdvertiserCb = new (std::nothrow) MockNearlinkSleAdvertiseCallbackStub();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

int32_t SleAdvertiserOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_sleAdvertiser, TRANSACTION_ERR, "g_sleAdvertiser is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("SleAdvertiserOnRemoteRequest, cmd(%{public}d)", code);
    return g_sleAdvertiser->OnRemoteRequest(code, data, reply, option);
}

void RegisterSleAdvertiserCallbackFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    if (g_sleAdvertiserCb != nullptr) {
        data.WriteRemoteObject(g_sleAdvertiserCb->AsObject());
    }
    int32_t ret = SleAdvertiserOnRemoteRequest(
        NearlinkSleAdvertiserInterfaceCode::SLE_REGISTER_SLE_ADVERTISER_CALLBACK, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void DeregisterSleAdvertiserCallbackFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    if (g_sleAdvertiserCb != nullptr) {
        data.WriteRemoteObject(g_sleAdvertiserCb->AsObject());
    }
    int32_t ret = SleAdvertiserOnRemoteRequest(
        NearlinkSleAdvertiserInterfaceCode::SLE_DE_REGISTER_SLE_ADVERTISER_CALLBACK, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void CreateAdvSettings(NearlinkSleAdvertiserSettings &settings, FuzzedDataProvider &provider)
{
    settings.SetConnectable(provider.ConsumeBool()); // 1
    settings.SetInterval(provider.ConsumeIntegral<uint16_t>()); // 2
    settings.SetLegacyMode(provider.ConsumeBool()); // 1
    settings.SetTxPower(provider.ConsumeIntegral<uint8_t>()); // 1
    std::vector<uint8_t> data = provider.ConsumeBytes<uint8_t>(RawAddress::SLE_ADDRESS_BYTE_LEN);
    size_t len = data.size();
    std::array<uint8_t, RawAddress::SLE_ADDRESS_BYTE_LEN> addr = {0};
    std::copy_n(data.begin(), len, addr.begin());
    settings.SetOwnAddr(addr);
    settings.SetOwnAddrType(provider.ConsumeIntegral<int8_t>()); // 1
    settings.SetLinkRole(provider.ConsumeIntegral<uint8_t>());
}

void CreateAdvData(NearlinkSleAdvertiserData &advData, FuzzedDataProvider &provider)
{
    advData.SetAdvFlag(provider.ConsumeIntegral<uint8_t>()); // 1
    advData.SetIncludeDeviceName(provider.ConsumeBool()); // 1
    advData.SetIncludeTxPower(provider.ConsumeBool()); // 1
}

void StartAdvertisingFuzzTest(FuzzedDataProvider &provider)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());

    NearlinkSleAdvertiserSettings settings;
    CreateAdvSettings(settings, provider);
    data.WriteParcelable(&settings);

    NearlinkSleAdvertiserData advData;
    CreateAdvData(advData, provider);
    data.WriteParcelable(&advData);

    data.WriteParcelable(&advData);

    int32_t advHandle = provider.ConsumeIntegralInRange<int32_t>(0, 10);
    data.WriteInt32(advHandle);

    int32_t ret = SleAdvertiserOnRemoteRequest(
        NearlinkSleAdvertiserInterfaceCode::SLE_START_ADVERTISING, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void StopAdvertisingFuzzTest(int32_t advHandle)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    data.WriteInt32(advHandle);

    int32_t ret = SleAdvertiserOnRemoteRequest(
        NearlinkSleAdvertiserInterfaceCode::SLE_STOP_ADVERTISING, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void GetAdvertiserHandleFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());

    int32_t ret = SleAdvertiserOnRemoteRequest(
        NearlinkSleAdvertiserInterfaceCode::SLE_GET_ADVERTISER_HANDLE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void SetAdvertisingDataFuzzTest(FuzzedDataProvider &provider, int32_t advHandle)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    NearlinkSleAdvertiserData advData;
    CreateAdvData(advData, provider);
    data.WriteParcelable(&advData);
    data.WriteParcelable(&advData);
    data.WriteInt32(advHandle);

    int32_t ret = SleAdvertiserOnRemoteRequest(
        NearlinkSleAdvertiserInterfaceCode::SLE_SET_ADVERTISING_DATA, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void AdvertiserFuzzTest(const uint8_t *fuzzData, size_t size)
{
    HILOGI("start");
    FuzzedDataProvider provider(fuzzData, size);
    uint32_t code = (provider.ConsumeIntegral<uint32_t>() % MESSAGE_SIZE);

    MessageParcel data;
    std::u16string descriptor = NearlinkSleAdvertiserStub::GetDescriptor();
    data.WriteInterfaceToken(descriptor);
    data.WriteInt32(provider.ConsumeIntegral<int32_t>());
    data.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    int32_t ret = SleAdvertiserOnRemoteRequest(code, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void EnableAdvertisingFuzzTest(int32_t advHandle)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    data.WriteInt32(advHandle);

    int32_t ret = SleAdvertiserOnRemoteRequest(
        NearlinkSleAdvertiserInterfaceCode::SLE_ENABLE_ADVERTISING, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void DisableAdvertisingFuzzTest(int32_t advHandle)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleAdvertiserStub::GetDescriptor());
    data.WriteInt32(advHandle);

    int32_t ret = SleAdvertiserOnRemoteRequest(
        NearlinkSleAdvertiserInterfaceCode::SLE_DISABLE_ADVERTISING, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}
}

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void) argc;
    (void) argv;
    // mock token
    NearlinkMockNativeToken mock("nearlink_service");
    // mock enable nearlink
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

    OHOS::g_threadUtil.InitThreadStateMap();
    {
        FuzzedDataProvider provider(data, size);
        OHOS::RegisterSleAdvertiserCallbackFuzzTest();
        uint32_t seq = OHOS::g_advEventSeq.load();
        OHOS::StartAdvertisingFuzzTest(provider);
        int32_t advHandle = -1;
        if (OHOS::WaitAdvEvent(seq)) {
            advHandle = OHOS::g_advHandle.load();
        }
        if (advHandle < 0) {
            /* 未等到开始结果：用随机句柄保留未命中分支 */
            advHandle = provider.ConsumeIntegralInRange<int32_t>(0, 10);
        }
        OHOS::GetAdvertiserHandleFuzzTest(data, size);
        OHOS::SetAdvertisingDataFuzzTest(provider, advHandle);
        (void)OHOS::WaitAdvEvent(OHOS::g_advEventSeq.load());
        OHOS::EnableAdvertisingFuzzTest(advHandle);
        (void)OHOS::WaitAdvEvent(OHOS::g_advEventSeq.load());
        OHOS::DisableAdvertisingFuzzTest(advHandle);
        (void)OHOS::WaitAdvEvent(OHOS::g_advEventSeq.load());
        OHOS::StopAdvertisingFuzzTest(advHandle);
        (void)OHOS::WaitAdvEvent(OHOS::g_advEventSeq.load());
        OHOS::AdvertiserFuzzTest(data, size);
        OHOS::DeregisterSleAdvertiserCallbackFuzzTest();
    }
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
