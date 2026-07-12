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
#include "cloudpair_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_cloud_pair_stub.h"
#include "nearlink_cloud_pair_server.h"
#include "nearlink_host_server.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr size_t FUZZ_CLOUD_PAIR_LIST_SIZE = 5;
constexpr int CONTROLLER_MANAGER_FUZZ_DELAY_50_MS = 50;
constexpr int CONTROLLER_MANAGER_FUZZ_DELAY_100_MS = 100;
constexpr int CONTROLLER_MANAGER_FUZZ_DELAY_5000_MS = 5000;
constexpr int TOKEN_BYTE_LEN = 32;
sptr<NearlinkCloudPairServer> g_cloudPair = new (std::nothrow) NearlinkCloudPairServer();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
} // namespace

int32_t ControllerManagerOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_cloudPair, TRANSACTION_ERR, "g_cloudPair is nullptr");
    MessageOption option = { MessageOption::TF_SYNC };
    HILOGI("g_cloudPair OnRemoteRequest, cmd(%{public}d)", code);
    return g_cloudPair->OnRemoteRequest(code, data, reply, option);
}

void UpdateCloudDeviceInfoListFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    std::vector<NearlinkCloudPairDevice> cloudDeviceInfos {};
    for (size_t i = 0; i < FUZZ_CLOUD_PAIR_LIST_SIZE; i++) {
        NearlinkCloudPairDevice dev;
        dev.SetBtAddr(BuildAddressString(provider));
        dev.SetDeviceName(BuildAddressString(provider));
        dev.SetToken(provider.ConsumeBytes<uint8_t>(TOKEN_BYTE_LEN));
        dev.SetReportAddr(BuildAddressString(provider));
        dev.SetMembersAddr(vector<string>{BuildAddressString(provider), BuildAddressString(provider)});
        dev.SetModel(BuildAddressString(provider));
        dev.SetSubModelId(BuildAddressString(provider));
        dev.SetDeviceIconId(BuildAddressString(provider));
        cloudDeviceInfos.emplace_back(dev);
    }
    data.WriteInterfaceToken(NearlinkCloudPairStub::GetDescriptor());
    int32_t cloudDevSize = static_cast<int32_t>(cloudDeviceInfos.size());
    data.WriteInt32(cloudDevSize);
    for (auto &dev : cloudDeviceInfos) {
        data.WriteParcelable(&dev);
    }
    int32_t ret = ControllerManagerOnRemoteRequest(NearlinkCloudPairInterfaceCode::NL_UPDATE_CLOUD_DEVICE_INFO_LIST,
        data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::CONTROLLER_MANAGER_FUZZ_DELAY_50_MS));
}

void GetCloudPairStateFuzzTest(const uint8_t *fuzzData, size_t size)
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkCloudPairStub::GetDescriptor());
    int32_t ret = ControllerManagerOnRemoteRequest(NearlinkCloudPairInterfaceCode::NL_GET_CLOUD_PAIR_STATE,
        data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::CONTROLLER_MANAGER_FUZZ_DELAY_50_MS));
}
} // namespace OHOS

// fuzzer init
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    // mock token
    NearlinkMockNativeToken mock("nearlink_service");

    OHOS::sptr<NearlinkHostServer> hostServer = NearlinkHostServer::GetInstance();
    HILOGI("controllermanager_fuzzer OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::CONTROLLER_MANAGER_FUZZ_DELAY_100_MS));
    HILOGI("controllermanager_fuzzer EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::CONTROLLER_MANAGER_FUZZ_DELAY_5000_MS));
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
    OHOS::UpdateCloudDeviceInfoListFuzzTest(data, size);
    OHOS::GetCloudPairStateFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}