/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thread>
#include "slecontroller_fuzzer.h"
#include "fuzzer/FuzzedDataProvider.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_sle_controller_stub.h"
#include "nearlink_sle_controller_server.h"
#include "nearlink_host_server.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"

using namespace std;
using namespace OHOS::Nearlink;

namespace OHOS {
namespace {
constexpr int SLE_CONTROLLER_FUZZ_DELAY_50_MS = 50;
constexpr int SLE_CONTROLLER_FUZZ_DELAY_100_MS = 100;
constexpr int SLE_CONTROLLER_FUZZ_DELAY_5000_MS = 5000;
sptr<NearlinkSleControllerServer> g_sleController = new NearlinkSleControllerServer();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}

int32_t SleControllerOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_sleController, TRANSACTION_ERR, "g_sleController is nullptr");
    MessageOption option = { MessageOption::TF_SYNC };
    HILOGI("g_sleController OnRemoteRequest, cmd(%{public}d)", code);
    return g_sleController->OnRemoteRequest(code, data, reply, option);
}

void SetSleCoexParamFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleControllerStub::GetDescriptor());
    uint16_t maxBitRate = provider.ConsumeIntegral<uint16_t>();
    uint8_t dutyCycle = provider.ConsumeIntegral<uint8_t>();
    data.WriteUint16(maxBitRate);
    data.WriteUint8(dutyCycle);
    int32_t ret = SleControllerOnRemoteRequest(
        NearlinkSleControllerInterfaceCode::NL_SET_SLE_COEX_PARAM, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}

void UpdateConnectIntervalFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleControllerStub::GetDescriptor());
    std::string address = BuildAddressString(provider);
    int32_t intervalType = provider.ConsumeIntegral<int32_t>();
    data.WriteString(address);
    data.WriteInt32(intervalType);

    int32_t ret = SleControllerOnRemoteRequest(
        NearlinkSleControllerInterfaceCode::NL_SLE_UPDATE_INTERVAL, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(SLE_CONTROLLER_FUZZ_DELAY_50_MS));
}
} // namespace

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    // mock token
    NearlinkMockNativeToken mock("nearlink_service");

    OHOS::sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    HILOGI("slecontroller_fuzzer OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SLE_CONTROLLER_FUZZ_DELAY_100_MS));
    HILOGI("slecontroller_fuzzer EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::SLE_CONTROLLER_FUZZ_DELAY_5000_MS));
    return 0;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        OHOS::g_threadUtil.ClearThreadStateMap();
        return 0;
    }

    OHOS::g_threadUtil.InitThreadStateMap();
    OHOS::SetSleCoexParamFuzzTest(data, size);
    OHOS::UpdateConnectIntervalFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
