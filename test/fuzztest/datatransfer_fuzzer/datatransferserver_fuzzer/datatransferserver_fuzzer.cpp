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
#include "fuzzer/FuzzedDataProvider.h"
#include "../datatransferserver_fuzzer/datatransferserver_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_sle_datatransfer.h"
#include "nearlink_sle_datatransfer_stub.h"
#include "nearlink_sle_datatransfer_server.h"
#include "nearlink_host_server.h"
#include "ThreadUtil.h"
#include "log.h"
#include "securec.h"
#include "raw_address.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
class MockNearlinkSleDataTransferCallbackStub : public IRemoteStub<INearlinkSleDataTransferCallback> {
public:
    void OnConnectionStateChanged(const NearlinkSleDataTransferConnectionParams &connectionParams, int fd) {}
};

namespace {
constexpr int HOST_FUZZ_DELAY_10_MS = 50;
constexpr int HOST_FUZZ_DELAY_100_MS = 100;
constexpr int HOST_FUZZ_DELAY_5000_MS = 5000;
sptr<NearlinkSleDataTransferServer> g_sleDataTransfer = new (std::nothrow) NearlinkSleDataTransferServer();
sptr<INearlinkSleDataTransferCallback> g_sleDataTransferCb =
    new (std::nothrow) MockNearlinkSleDataTransferCallbackStub();
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
} // namespace

int32_t SleDataTransferOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_sleDataTransfer, TRANSACTION_ERR, "g_sleDataTransfer is nullptr");
    MessageOption option = { MessageOption::TF_SYNC };
    HILOGI("SleDataTransferOnRemoteRequest, cmd(%{public}d)", code);
    return g_sleDataTransfer->OnRemoteRequest(code, data, reply, option);
}

void RegisterSleDataTransferCallbackFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    if (g_sleDataTransferCb != nullptr) {
        data.WriteRemoteObject(g_sleDataTransferCb->AsObject());
    }
    int32_t ret = SleDataTransferOnRemoteRequest(
        NearlinkSleDataTransferInterfaceCode::SLE_REGISTER_SLE_DATATRANSFER_CALLBACK, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void DeregisterSleDataTransferCallbackFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    if (g_sleDataTransferCb != nullptr) {
        data.WriteRemoteObject(g_sleDataTransferCb->AsObject());
    }
    int32_t ret = SleDataTransferOnRemoteRequest(
        NearlinkSleDataTransferInterfaceCode::SLE_DE_REGISTER_SLE_DATATRANSFER_CALLBACK, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void CreatePortFuzzTest()
{
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());

    std::string uuid = "060D";
    data.WriteString(uuid);

    int32_t ret = SleDataTransferOnRemoteRequest(NearlinkSleDataTransferInterfaceCode::SLE_CREATE_PORT, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void SocketEmptyMsgFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    uint16_t port = provider.ConsumeIntegral<uint16_t>();
    data.WriteUint16(port);

    int32_t ret = SleDataTransferOnRemoteRequest(NearlinkSleDataTransferInterfaceCode::SLE_SOCKET_EMPTY_PORT,
        data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void DestroyPortFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    std::string uuid = "060D";
    data.WriteString(uuid);
    uint16_t port = provider.ConsumeIntegral<uint16_t>();
    data.WriteUint16(port);

    int32_t ret = SleDataTransferOnRemoteRequest(NearlinkSleDataTransferInterfaceCode::SLE_DESTROY_PORT, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}
void ConnectFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    NearlinkSleDataTransferConnectionParams param;
    std::string address = BuildAddressString(provider);
    std::string uuid = "060D";
    uint16_t port = 40960;

    param.SetAddress(address);
    param.SetUuid(uuid);
    param.SetPort(port);
    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    data.WriteParcelable(&param);

    int32_t ret = SleDataTransferOnRemoteRequest(
        NearlinkSleDataTransferInterfaceCode::SLE_CONNECT, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        int32_t connState = reply.ReadInt32();
        HILOGI("connState(%{public}d)", connState);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void DisconnectFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    NearlinkSleDataTransferConnectionParams param;
    std::string address = BuildAddressString(provider);
    std::string uuid = "060D";
    uint16_t port = 40960;

    param.SetAddress(address);
    param.SetUuid(uuid);
    param.SetPort(port);
    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    data.WriteParcelable(&param);

    int32_t ret = SleDataTransferOnRemoteRequest(
        NearlinkSleDataTransferInterfaceCode::SLE_DISCONNECT, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        int32_t connState = reply.ReadInt32();
        HILOGI("connState(%{public}d)", connState);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void GetConnectionStateFuzzTest(const uint8_t *fuzzData, size_t size)
{
    FuzzedDataProvider provider(fuzzData, size);
    MessageParcel data;
    MessageParcel reply;

    NearlinkSleDataTransferConnectionParams param;
    std::string address = BuildAddressString(provider);
    std::string uuid = "060D";
    uint16_t port = 40960;

    param.SetAddress(address);
    param.SetUuid(uuid);
    param.SetPort(port);
    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    data.WriteParcelable(&param);

    int32_t ret = SleDataTransferOnRemoteRequest(
        NearlinkSleDataTransferInterfaceCode::SLE_GET_CONNECTION_STATE, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    NlErrCode exception = static_cast<NlErrCode>(reply.ReadInt32());
    if (exception == NL_NO_ERROR) {
        int32_t connState = reply.ReadInt32();
        HILOGI("connState(%{public}d)", connState);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void UpdateConnectIntervalTest(const uint8_t *fuzzData, size_t size)
{
    HILOGI("start");
    FuzzedDataProvider provider(fuzzData, size);
    std::string device = BuildAddressString(provider);
    int32_t intervalType = provider.ConsumeIntegral<int32_t>();

    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    data.WriteString(device);
    data.WriteInt32(intervalType);
    int32_t ret = SleDataTransferOnRemoteRequest(
        NearlinkSleDataTransferInterfaceCode::SLE_UPDATE_INTERVAL, data, reply);
    if (ret != NO_ERROR) {
        HILOGI("send req failed, ret(%{public}d)", ret);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
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
    HILOGI("datatransfer_fuzzer OnStart");
    hostServer->OnStart();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_100_MS));
    HILOGI("datatransfer_fuzzer EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_5000_MS));
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
    OHOS::RegisterSleDataTransferCallbackFuzzTest();
    OHOS::UpdateConnectIntervalTest(data, size);
    OHOS::CreatePortFuzzTest();
    OHOS::SocketEmptyMsgFuzzTest(data, size);
    OHOS::DestroyPortFuzzTest(data, size);
    OHOS::ConnectFuzzTest(data, size);
    OHOS::DisconnectFuzzTest(data, size);
    OHOS::GetConnectionStateFuzzTest(data, size);
    OHOS::DeregisterSleDataTransferCallbackFuzzTest();
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
