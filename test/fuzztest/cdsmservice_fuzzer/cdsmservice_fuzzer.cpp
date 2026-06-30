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
#include "cdsmservice_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "ThreadUtil.h"
#include "log.h"
#include "nearlink_host_server.h"
#include "nearlink_cdsm_client_server.h"
#include "nearlink_raw_address.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {

class MockNearlinkCdsmClientCallbackStub : public IRemoteStub<INearlinkCdsmClientCallback> {
public:
    void OnCdsInfoChanged(const NearlinkCdsInfoParcel &cdsInfo) {}
};

namespace {
    constexpr int HOST_FUZZ_DELAY_100_MS = 100;
    constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
    sptr<NearlinkCdsmClientServer> g_ascClient = new (std::nothrow) NearlinkCdsmClientServer();
    sptr<INearlinkCdsmClientCallback> g_ascClientCb = new (std::nothrow) MockNearlinkCdsmClientCallbackStub();
    ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

int32_t CdsmClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_ascClient, TRANSACTION_ERR, "cdsmClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("CdsmClientOnRemoteRequest, cmd(%{public}d)", code);
    return g_ascClient->OnRemoteRequest(code, data, reply, option);
}

void RegisterApplicationFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    std::string addrStr = BuildAddressString(provider);
    NearlinkRawAddress addr = NearlinkRawAddress(addrStr);
    data.WriteParcelable(&addr);

    data.WriteInterfaceToken(NearlinkCdsmClientStub::GetDescriptor());
    if (g_ascClientCb != nullptr) {
        data.WriteRemoteObject(g_ascClientCb->AsObject());
    }

    int32_t ret = CdsmClientOnRemoteRequest(
        NearlinkCdsmClientInterfaceCode::NL_REGISTER_CDSM_CLIENT_CALLBACK,data, reply);
    if (ret != NL_NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
}

void DeregisterApplicationFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkCdsmClientStub::GetDescriptor());

    int32_t ret = CdsmClientOnRemoteRequest(
        NearlinkCdsmClientInterfaceCode::NL_DE_REGISTER_CDSM_CLIENT_CALLBACK, data, reply);
    if (ret != NL_NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
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
        OHOS::g_threadUtil.ClearThreadStateMap();
        return 0;
    }

    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::RegisterApplicationFuzzTest(data, size);
    OHOS::DeregisterApplicationFuzzTest();
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
