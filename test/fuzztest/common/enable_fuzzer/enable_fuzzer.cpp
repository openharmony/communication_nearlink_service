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
#include "enable_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_host_stub.h"
#include "nearlink_host_server.h"
#include "nearlink_errorcode.h"
#include "log.h"
#include "securec.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr int HOST_FUZZ_DELAY_10_MS = 50;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
}

int32_t HostOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    NL_CHECK_RETURN_RET(hostServer, TRANSACTION_ERR, "hostServer is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("HostOnRemoteRequest cmd(%{public}d)", code);
    return hostServer->OnRemoteRequest(code, data, reply, option);
}

void DisableSleFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_DISABLE_SLE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void EnableSleFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_ENABLE_SLE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void DisableSleToOffFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_DISABLE_SLE_TO_OFF, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void EnableSleToHalfFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_ENABLE_SLE_TO_HALF, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void FactoryResetFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkHostStub::GetDescriptor());
    int32_t ret = HostOnRemoteRequest(NearlinkHostInterfaceCode::NL_FACTORY_RESET, data, reply);
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

    OHOS::sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    if (hostServer == nullptr) {
        return 0;
    }
    HILOGI("host_fuzzer OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("host_fuzzer EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (data == nullptr) {
        return 0;
    }
    OHOS::DisableSleFuzzTest(data, size);
    OHOS::EnableSleFuzzTest(data, size);
    OHOS::DisableSleToOffFuzzTest(data, size);
    OHOS::EnableSleToHalfFuzzTest(data, size);
    OHOS::FactoryResetFuzzTest(data, size);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_5000_MS));
    return 0;
}

