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
#include "vcpservice_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "ThreadUtil.h"
#include "log.h"
#include "nearlink_host_server.h"
#include "nearlink_vcp_client_server.h"
#include "nearlink_vcp_client_stub.h"
#include "nearlink_raw_address.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
    constexpr int HOST_FUZZ_DELAY_50_MS = 50;
    constexpr int HOST_FUZZ_DELAY_100_MS = 100;
    constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
    ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

int32_t VcpClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    sptr<NearlinkVcpClientServer> vcpClient = new (std::nothrow) NearlinkVcpClientServer();
    NL_CHECK_RETURN_RET(vcpClient, TRANSACTION_ERR, "vcpClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("VcpClientOnRemoteRequest, cmd(%{public}d)", code);
    return vcpClient->OnRemoteRequest(code, data, reply, option);
}

void SetDeviceAbsoluteVolumeFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkVcpClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);
    data.WriteUint64(provider.ConsumeIntegral<uint64_t>()); // volumeLevel

    int32_t ret = VcpClientOnRemoteRequest(NL_VCP_CLIENT_SET_DEVICE_ABSOLUTE_VOLUME, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void GetDeviceMediaVolumeFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkVcpClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);

    int32_t ret = VcpClientOnRemoteRequest(NL_VCP_CLIENT_GET_MEDIA_VOLUME, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_50_MS));
}

void GetDeviceCallVolumeFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkVcpClientStub::GetDescriptor());
    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);

    int32_t ret = VcpClientOnRemoteRequest(NL_VCP_CLIENT_GET_CALL_VOLUME, data, reply);
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
    HILOGI("VcpFuzzTest OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("VcpFuzzTest EnableSle");
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

    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::SetDeviceAbsoluteVolumeFuzzTest(data, size);
    OHOS::GetDeviceMediaVolumeFuzzTest(data, size);
    OHOS::GetDeviceCallVolumeFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
