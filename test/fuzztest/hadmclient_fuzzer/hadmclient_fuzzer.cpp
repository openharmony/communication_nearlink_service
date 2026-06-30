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
#include "fuzzer/FuzzedDataProvider.h"
#include "hadmclient_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_hadm_client_stub.h"
#include "nearlink_hadm_client_server.h"
#include "nearlink_host_server.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

class MockNearlinkHadmClientCallbackStub : public IRemoteStub<INearlinkHadmClientCallback> {
public:
    MockNearlinkHadmClientCallbackStub()
    {
        HILOGI("MockNearlinkHadmClientCallbackStub");
    }
    ~MockNearlinkHadmClientCallbackStub()
    {
        HILOGI("~MockNearlinkHadmClientCallbackStub");
    }
    void OnSoundingResult(const NearlinkRawAddress &addr, const NearlinkHadmClientSoundingResult &result) {}
    void OnSoundingStateChange(const NearlinkRawAddress &addr, int newState, int errorCode) {}
};

namespace {
constexpr int HOST_FUZZ_DELAY_MS = 50;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
constexpr uint32_t MESSAGE_SIZE = NearlinkHadmClientInterfaceCode::NL_HADM_CLIENT_MANAGER_BUTT;
sptr<NearlinkHadmClientServer> g_hadmClient = nullptr;
sptr<INearlinkHadmClientCallback> g_hadmClientCb = new (std::nothrow) MockNearlinkHadmClientCallbackStub();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

int32_t HadmClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    if (!g_hadmClient) {
        g_hadmClient = new (std::nothrow) NearlinkHadmClientServer();
    }
    NL_CHECK_RETURN_RET(g_hadmClient, TRANSACTION_ERR, "g_hadmClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("HadmClientOnRemoteRequest, cmd(%{public}d)", code);
    return g_hadmClient->OnRemoteRequest(code, data, reply, option);
}

void RegisterNearlinkHadmClientCallbackFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHadmClientStub::GetDescriptor());
    data.WriteUint32(provider.ConsumeIntegral<uint32_t>());
    if (g_hadmClientCb != nullptr) {
        data.WriteRemoteObject(g_hadmClientCb->AsObject());
    }

    int32_t ret = HadmClientOnRemoteRequest(
        NearlinkHadmClientInterfaceCode::NL_REGISTER_HADM_CLIENT_CALLBACK, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_MS));
}

void DeregisterNearlinkHadmClientCallbackFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHadmClientStub::GetDescriptor());
    data.WriteUint32(provider.ConsumeIntegral<uint32_t>());
    if (g_hadmClientCb != nullptr) {
        data.WriteRemoteObject(g_hadmClientCb->AsObject());
    }

    int32_t ret = HadmClientOnRemoteRequest(
        NearlinkHadmClientInterfaceCode::NL_DE_REGISTER_HADM_CLIENT_CALLBACK, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_MS));
}

void StartSoundingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHadmClientStub::GetDescriptor());
    data.WriteUint32(provider.ConsumeIntegral<uint32_t>());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr(addrStr);
    data.WriteParcelable(&addr);

    int32_t ret = HadmClientOnRemoteRequest(
        NearlinkHadmClientInterfaceCode::NL_HADM_CLIENT_START_SOUNDING, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_MS));
}

void StopSoundingFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHadmClientStub::GetDescriptor());
    data.WriteUint32(provider.ConsumeIntegral<uint32_t>());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr(addrStr);
    data.WriteParcelable(&addr);

    int32_t ret = HadmClientOnRemoteRequest(
        NearlinkHadmClientInterfaceCode::NL_HADM_CLIENT_STOP_SOUNDING, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_MS));
}

void GetHadmFeatureFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHadmClientStub::GetDescriptor());

    int32_t ret = HadmClientOnRemoteRequest(
        NearlinkHadmClientInterfaceCode::NL_GET_HADM_FEATURE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_MS));
}

void HadmClientFuzzTest(const uint8_t* fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    uint32_t code = (provider.ConsumeIntegral<uint32_t>() % MESSAGE_SIZE);
    auto addr = BuildAddressString(provider);

    MessageParcel data;
    MessageParcel reply;

    std::u16string descriptor = NearlinkHadmClientStub::GetDescriptor();
    data.WriteInterfaceToken(descriptor);
    data.WriteInt32(provider.ConsumeIntegral<int32_t>());
    data.WriteString(addr.c_str());
    data.RewindRead(0);

    int32_t ret = HadmClientOnRemoteRequest(code, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_MS));
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
    HILOGI("HadmClientFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("HadmClientFuzzTest EnableSle");
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
    OHOS::RegisterNearlinkHadmClientCallbackFuzzTest(data, size);
    OHOS::StartSoundingFuzzTest(data, size);
    OHOS::HadmClientFuzzTest(data, size);
    OHOS::StopSoundingFuzzTest(data, size);
    OHOS::DeregisterNearlinkHadmClientCallbackFuzzTest(data, size);
    OHOS::GetHadmFeatureFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}

