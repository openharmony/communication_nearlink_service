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
#include "nearlink_sle_datatransfer_service.cpp"
#include "SleInterfaceDataTransfer.h"
#include "nearlink_host_server.h"
#include "qosm_trans_channel.h"
#include "log.h"

using namespace std;
using namespace OHOS::Nearlink;
namespace OHOS {
namespace {
constexpr int HOST_FUZZ_DELAY_10_MS = 50;
const std::string CAR_UUID = "060D";
constexpr int32_t INTERVAL_TYPE = 0x24;
int g_fd = -1;
ThreadUtil &g_threadUtil = ThreadUtil::GetInstance();
}
class MockNearlinkSleDataTransferCallback : public OHOS::Nearlink::ISleDataTransferServiceCallback {
public:
    MockNearlinkSleDataTransferCallback() = default;
    ~MockNearlinkSleDataTransferCallback() override = default;

    void OnConnectionStateChanged(const DataTransferConnectionParams &connectionParams, int fd) override
    {
        g_fd = fd;
        if (fd > 0) {
            HILOGI("connected sucess");
        }
        HILOGI("OnConnectionStateChanged process");
    }

    std::string GetRandomAddr(const std::string &address, uint32_t tokenId) override
    {
        HILOGI("GetRandomAddr process address=%{public}s", GetEncryptAddr(address).c_str());
        return address;
    }
};

namespace {
std::shared_ptr<MockNearlinkSleDataTransferCallback> g_callback = std::make_shared<MockNearlinkSleDataTransferCallback>(
);
}

void CreatePortAndDestroyPortFuzzTest001()
{
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    std::string uuid = CAR_UUID;
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
}

void CreatePortAndDestroyPortFuzzTest002(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string uuid = BuildUuid(provider).ConvertToString();
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
}

void RegisterSleDataTransferServiceCallbackFuzzTest001()
{
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
}

void DeregisterSleDataTransferServiceCallbackFuzzTest001()
{
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.DeregisterSleDataTransferServiceCallback();
}

void ConnectAndDisConnectFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = CAR_UUID;
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    param.SetPort(portId);
    instance.Connect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    int32_t state = instance.GetConnectionState(param);
    NL_CHECK_RETURN(state == static_cast<int32_t>(SleConnectState::CONNECTED), "state is error");
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
}

void ConnectAndDisConnectFuzzTest002(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = BuildUuid(provider).ConvertToString();
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.Connect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    int32_t state = instance.GetConnectionState(param);
    NL_CHECK_RETURN(state == static_cast<int32_t>(SleConnectState::CONNECTING), "state is error");
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
}

void HandleConnectEventFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    size_t length = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    std::vector<uint8_t> datas = provider.ConsumeBytes<uint8_t>(length);
    while (datas.size() < length) {
        datas.push_back(0);
    }
    uint8_t tcid = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    uint16_t dstPort = max(static_cast<int>(provider.ConsumeIntegral<uint16_t>()), 1);

    std::string uuid = CAR_UUID;
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    AppConnectParamMapping temp;
    temp.address = address;
    temp.dstPort = dstPort;
    temp.tcid = tcid;
    temp.state = static_cast<int32_t>(SleConnectState::CONNECTED);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portId, UINT16_MAX, temp);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    NL_CHECK_RETURN(g_fd > 0, "fd is invalid");

    std::shared_ptr<OutputStream> outputStream = std::make_shared<OutputStream>(g_fd);
    size_t totalLen = 0;
    DataTransferDataParams dataTransfer(address, uuid, portId, datas);
    std::unique_ptr<uint8_t[]> packageData = NearlinkDataTransferDataParams::SerializeData(dataTransfer, totalLen);
    outputStream->Write(packageData.get(), totalLen);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.HandleReceiveDataEvent(temp.address, temp.dstPort, datas);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
    if (g_fd > 0) {
        close(g_fd);
        g_fd = -1;
    }
}

void HandleConnectEventFuzzTest002(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = BuildUuid(provider).ConvertToString();
    size_t length = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    std::vector<uint8_t> datas = provider.ConsumeBytes<uint8_t>(length);
    while (datas.size() < length) {
        datas.push_back(0);
    }
    uint8_t tcid = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    uint16_t dstPort = max(static_cast<int>(provider.ConsumeIntegral<uint16_t>()), 1);

    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    AppConnectParamMapping temp;
    temp.address = address;
    temp.dstPort = dstPort;
    temp.state = static_cast<int32_t>(SleConnectState::CONNECTED);
    temp.tcid = tcid;
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portId, UINT16_MAX, temp);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    NL_CHECK_RETURN(g_fd > 0, "fd is invalid");

    std::shared_ptr<OutputStream> outputStream = std::make_shared<OutputStream>(g_fd);
    uint16_t len = length;
    TRANS_Addr_S stackAddr;
    RawAddress addr(address);
    addr.ConvertToUint8(stackAddr.devAddr.addr);
    stackAddr.dstPort = temp.dstPort;
    instance.ReceiveDataCallback(&stackAddr, datas.data(), len);
    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
    if (g_fd > 0) {
        close(g_fd);
        g_fd = -1;
    }
}

void ChannelStatusCallbackFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = BuildUuid(provider).ConvertToString();
    uint8_t tcid = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    QOSM_TransChannelRspParams_S respParams;
    RawAddress addr(address);
    addr.ConvertToUint8(respParams.addr.addr);
    respParams.tcid = tcid;
    respParams.srcPort = portId;
    respParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;
    respParams.mtu = UINT16_MAX;
    instance.ChannelStatusCallback(&respParams);
}

void SendDataStateCallbackFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = BuildUuid(provider).ConvertToString();
    uint8_t tcid = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    uint16_t dstPort = max(static_cast<int>(provider.ConsumeIntegral<uint16_t>()), 1);

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");

    AppConnectParamMapping temp;
    temp.address = address;
    temp.dstPort = dstPort;
    temp.tcid = tcid;
    temp.state = static_cast<int32_t>(SleConnectState::CONNECTED);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, CAR_PORT, UINT16_MAX, temp);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    NL_CHECK_RETURN(g_fd > 0, "fd is invalid");

    RawAddress addr(address);
    SLE_Addr_S devAddr;
    addr.ConvertToUint8(devAddr.addr);
    uint8_t result = TransferState::TRANSFER_AVAILABLE;
    instance.SendDataStateCallback(&devAddr, tcid, portId, result);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
}

void OnAcbStateChangedFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);

    std::string uuid = CAR_UUID;
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    instance.Connect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    RawAddress addr(address);
    int32_t state = static_cast<int32_t>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED);
    int reason = 0;
    instance.pimpl->sleDataTransferAcbObserverImp_->OnAcbStateChanged(addr, state, reason);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
}

void PortServiceOnConnectionStateChangedFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = BuildUuid(provider).ConvertToString();

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    instance.Connect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    RawAddress addr(address);
    int state = static_cast<int>(SleConnectState::CONNECTED);
    int oldState = static_cast<int>(SleConnectState::CONNECTING);
    instance.pimpl->dataTransferPortObserver_.OnConnectionStateChanged(addr, state, oldState);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
}

void PortServiceOnConnectionStateChangedFuzzTest002(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = BuildUuid(provider).ConvertToString();

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    instance.Connect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    RawAddress addr(address);
    int state = static_cast<int>(SleConnectState::DISCONNECTED);
    int oldState = static_cast<int>(SleConnectState::CONNECTING);
    instance.pimpl->dataTransferPortObserver_.OnConnectionStateChanged(addr, state, oldState);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
}

void OnConnectionStateChangedFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);

    std::string uuid = CAR_UUID;
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.Connect(param);
    RawAddress addr(address);
    int32_t state = static_cast<int32_t>(SleConnectState::CONNECTED);
    int32_t preState = static_cast<int32_t>(SleConnectState::DISCONNECTED);
    int reason = 0;
    instance.pimpl->sleDataTransferAcbObserverImp_->OnConnectionStateChanged(addr, state, preState, reason);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
}

void UpdateConnectIntervalFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string device = BuildAddressString(provider);

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    int32_t intervalType = INTERVAL_TYPE;
#ifdef WATCH_STANDARD
    bool res = instance.UpdateConnectInterval(device, intervalType);
    NL_CHECK_RETURN(res, "UpdateConnectInterval is err");
#endif
}

}

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
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    HILOGI("datatransfer_fuzzer EnableSle");
    hostServer->EnableSle();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
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
    OHOS::CreatePortAndDestroyPortFuzzTest001();
    OHOS::CreatePortAndDestroyPortFuzzTest002(data, size);
    OHOS::RegisterSleDataTransferServiceCallbackFuzzTest001();
    OHOS::ConnectAndDisConnectFuzzTest001(data, size);
    OHOS::ConnectAndDisConnectFuzzTest002(data, size);
    OHOS::HandleConnectEventFuzzTest001(data, size);
    OHOS::HandleConnectEventFuzzTest002(data, size);
    OHOS::ChannelStatusCallbackFuzzTest001(data, size);
    OHOS::SendDataStateCallbackFuzzTest001(data, size);
    OHOS::OnAcbStateChangedFuzzTest001(data, size);
    OHOS::PortServiceOnConnectionStateChangedFuzzTest001(data, size);
    OHOS::PortServiceOnConnectionStateChangedFuzzTest002(data, size);
    OHOS::OnConnectionStateChangedFuzzTest001(data, size);
    OHOS::UpdateConnectIntervalFuzzTest001(data, size);
    OHOS::DeregisterSleDataTransferServiceCallbackFuzzTest001();
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
