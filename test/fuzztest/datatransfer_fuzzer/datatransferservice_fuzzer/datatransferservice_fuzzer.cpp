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
#include "../datatransferserver_fuzzer/datatransferserver_fuzzer.h"
#include "nearlink_native_token_mock.h"
#include "../nl_utils/fuzztest_utils.h"
#include "nearlink_sle_datatransfer_service.cpp"
#include "SleInterfaceDataTransfer.h"
#include "SleRemoteDeviceAdapter.h"
#include "nearlink_host_server.h"
#include "qosm_trans_channel.h"
#include "log.h"
#include "nearlink_fdsan_tag.h"

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
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
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
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
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

void TransferBizFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = BuildUuid(provider).ConvertToString();

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    size_t length = provider.ConsumeIntegral<uint8_t>() % 64 + 1; // 1..64
    std::vector<uint8_t> datas = provider.ConsumeBytes<uint8_t>(length);
    while (datas.size() < length) {
        datas.push_back(0);
    }
    uint8_t tcid = static_cast<uint8_t>(provider.ConsumeIntegral<uint16_t>() | 1); // 保证非 0
    uint16_t dstPort = provider.ConsumeIntegral<uint16_t>();
    uint16_t mtu = provider.ConsumeIntegral<uint16_t>();

    AppConnectParamMapping temp;
    temp.address = address;
    temp.dstPort = dstPort;
    temp.tcid = tcid;
    temp.state = static_cast<int32_t>(SleConnectState::CONNECTED);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portId, mtu, temp);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    DataTransferDataParams dataParams(address, uuid, portId, datas);
    instance.WriteData(dataParams);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);
    param.SetPort(portId);
    int32_t connState = instance.GetConnectionState(param);
    int transState = 0;
    int preTransState = 0;
    int transRet = instance.GetConnectionTransferState(portId, address, transState, preTransState);
    uint8_t result = static_cast<uint8_t>(provider.ConsumeIntegral<uint8_t>() % 2); // 0/1: AVAILABLE/BUSY
    instance.ChangeSocketState(portId, address, result);
    bool isValidPort = instance.IsValidSrcPort(portId);
    std::string devAddress = address;
    bool proxyExisted = instance.IsProxyConnectExisted(devAddress);
    instance.GetRemotePortByConnectionState(address,
        static_cast<int>(static_cast<SleConnectState>(provider.ConsumeIntegral<uint8_t>() % 5)),
        static_cast<int>(static_cast<SleConnectState>(provider.ConsumeIntegral<uint8_t>() % 5)));
    instance.ClearCacheByTokenId(provider.ConsumeIntegral<uint64_t>());
    instance.StopNlProxyIfExisted();
    HILOGI("TransferBizFuzzTest connState(%{public}d) transRet(%{public}d) transState(%{public}d) "
        "preTransState(%{public}d) isValidPort(%{public}d) proxyExisted(%{public}d)",
        connState, transRet, transState, preTransState, isValidPort, proxyExisted);

    instance.DestroyPort(uuid, portId);
    if (g_fd > 0) {
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
        g_fd = -1;
    }
}

void CarKeyConnectFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = CAR_UUID;

    SleRemoteDeviceAdapter *adapter = SleRemoteDeviceAdapter::GetInstance();
    NL_CHECK_RETURN(adapter != nullptr, "adapter is nullptr");
    std::shared_ptr<SlePeripheralDevice> peerDevice = std::make_shared<SlePeripheralDevice>();
    peerDevice->SetAddress(RawAddress(address));
    adapter->AddPeripheralDevice(address, peerDevice);
    adapter->SetAcbState(address, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    instance.Connect(param); // IsAcbConnected=true -> ConnectCarChannelAfterAcb
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    adapter->SetAcbState(address, static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    instance.Connect(param); // IsAcbConnected=false -> connect acb 分支
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
    adapter->RemovePeripheralDevice(address);
}

void TransferDisconnectFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = BuildUuid(provider).ConvertToString();
    uint8_t tcid = static_cast<uint8_t>(provider.ConsumeIntegral<uint16_t>() | 1); // 保证非 0
    uint16_t dstPort = provider.ConsumeIntegral<uint16_t>();
    uint16_t mtu = provider.ConsumeIntegral<uint16_t>();

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    AppConnectParamMapping temp;
    temp.address = address;
    temp.dstPort = dstPort;
    temp.tcid = tcid;
    temp.state = static_cast<int32_t>(SleConnectState::CONNECTED);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portId, mtu, temp);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    AppConnectParamMapping discTemp;
    discTemp.address = address;
    discTemp.dstPort = dstPort;
    discTemp.tcid = tcid;
    discTemp.state = static_cast<int32_t>(SleConnectState::DISCONNECTED);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_RELEASED, portId, mtu, discTemp);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    RawAddress addr(address);
    int state = static_cast<int>(SleConnectState::DISCONNECTED);
    int oldState = static_cast<int>(SleConnectState::CONNECTED);
    instance.pimpl->dataTransferPortObserver_.OnConnectionStateChanged(addr, state, oldState);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
    if (g_fd > 0) {
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
        g_fd = -1;
    }
}

// ---- PR-9 D-2 存量深挖：以下均为新增函数，不动既有函数的 FDP 读序 ----

// 连接态铺底（基线手法）：HandleConnectEvent(ESTABLISHED, port, mtu, temp{CONNECTED}) 注入已连接缓存
static AppConnectParamMapping BuildConnectedParam(const std::string &address, uint16_t dstPort, uint8_t tcid)
{
    AppConnectParamMapping temp;
    temp.address = address;
    temp.dstPort = dstPort;
    temp.tcid = tcid;
    temp.state = static_cast<int32_t>(SleConnectState::CONNECTED);
    return temp;
}

void WriteDataChannelFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = BuildUuid(provider).ConvertToString();
    uint8_t tcid = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    uint16_t dstPort = max(static_cast<int>(provider.ConsumeIntegral<uint16_t>()), 1);
    size_t length = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    std::vector<uint8_t> datas = provider.ConsumeBytes<uint8_t>(length);
    while (datas.size() < length) {
        datas.push_back(0);
    }

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    g_fd = -1;
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);
    param.SetPort(portId);

    // 连接态铺底后驱动 WriteData 族
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portId, UINT16_MAX,
        BuildConnectedParam(address, dstPort, tcid));
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    DataTransferDataParams writeParams(address, uuid, portId, datas);
    instance.WriteData(writeParams); // tcid>0 主路径：TRANS_SendData 成败两分支由栈返回值驱动
    DataTransferDataParams missAddrParams(address + ":00", uuid, portId, datas);
    instance.WriteData(missAddrParams); // 地址未命中：no AppConnectParamMapping

    // BUSY 迁移：SendDataStateCallback -> UpdateTransferState -> 查询族 preTransState 更新路径
    SLE_Addr_S devAddr;
    RawAddress rawAddr(address);
    rawAddr.ConvertToUint8(devAddr.addr);
    instance.SendDataStateCallback(&devAddr, tcid, portId, static_cast<uint8_t>(TransferState::TRANSFER_BUSY));
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    int transState = 0;
    int preTransState = 0;
    instance.GetConnectionTransferState(portId, address, transState, preTransState);
    instance.GetTransferState(portId, address); // 非可用态告警分支
    instance.ChangeSocketState(portId, address, static_cast<uint8_t>(TransferState::TRANSFER_AVAILABLE));
    instance.ChangeSocketState(portId, "FF:EE:00:00:00:00", 0xFF); // 地址不匹配：命中端口但 tempVec 为空
    instance.ChangeSocketState(UINT16_MAX, address, 0); // 端口未缓存分支
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    if (g_fd > 0) {
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
        g_fd = -1;
    }

    // tcid==0 错误分支：改注入 CONNECTING（无通道）后再写
    AppConnectParamMapping pendingParam;
    pendingParam.address = address;
    pendingParam.dstPort = dstPort;
    pendingParam.tcid = 0;
    pendingParam.state = static_cast<int32_t>(SleConnectState::CONNECTING);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISH_FAIL, portId, dstPort, pendingParam);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.WriteData(writeParams); // tcid invalid when writeData
    instance.GetConnectionState(param); // future 族：Init 窗口内调用
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid, portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void SocketDataFrameFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    uint8_t tcid = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    size_t length = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    std::vector<uint8_t> datas = provider.ConsumeBytes<uint8_t>(length);
    while (datas.size() < length) {
        datas.push_back(0);
    }

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    g_fd = -1;
    uint16_t portId = instance.CreatePort(CAR_UUID);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portId, UINT16_MAX,
        BuildConnectedParam(address, CAR_PORT, tcid));
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    NL_CHECK_RETURN(g_fd > 0, "fd is invalid");

    std::shared_ptr<OutputStream> outputStream = std::make_shared<OutputStream>(g_fd);
    // 帧1：合法帧 -> ReceivedData 完整解析 -> 线程内 WriteData
    DataTransferDataParams frameParams(address, CAR_UUID, portId, datas);
    size_t totalLen = 0;
    std::unique_ptr<uint8_t[]> packageData = NearlinkDataTransferDataParams::SerializeData(frameParams, totalLen);
    outputStream->Write(packageData.get(), totalLen);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    // 帧2：payload 端口不匹配 -> port not match
    uint16_t wrongPort = (portId == 0x1234) ? 0x4321 : 0x1234;
    DataTransferDataParams wrongPortParams(address, CAR_UUID, wrongPort, datas);
    size_t wrongLen = 0;
    std::unique_ptr<uint8_t[]> wrongData = NearlinkDataTransferDataParams::SerializeData(wrongPortParams, wrongLen);
    outputStream->Write(wrongData.get(), wrongLen);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    // 帧3：仅 8 字节总长(pLen==0) -> input stream data len is invalid
    std::vector<uint8_t> lenOnlyFrame(sizeof(size_t), 0);
    lenOnlyFrame[0] = static_cast<uint8_t>(sizeof(size_t));
    outputStream->Write(lenOnlyFrame.data(), lenOnlyFrame.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    // 帧4：短读 -> data len err
    std::vector<uint8_t> shortFrame = {0x01, 0x02, 0x03};
    outputStream->Write(shortFrame.data(), shortFrame.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    // 整改 P9-5：帧5 原始 FDP 字节直写（引擎探索任意畸形长度/内容；帧3/4 常量帧保留为保底）
    uint8_t rawLen = provider.ConsumeIntegral<uint8_t>() % 64;
    std::vector<uint8_t> rawFrame = provider.ConsumeBytes<uint8_t>(rawLen);
    if (!rawFrame.empty()) {
        outputStream->Write(rawFrame.data(), rawFrame.size());
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    // BUSY 早退 + AVAILABLE 恢复后悬留帧重试
    SLE_Addr_S devAddr;
    RawAddress rawAddr(address);
    rawAddr.ConvertToUint8(devAddr.addr);
    instance.SendDataStateCallback(&devAddr, tcid, portId, static_cast<uint8_t>(TransferState::TRANSFER_BUSY));
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    outputStream->Write(packageData.get(), totalLen);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.SendDataStateCallback(&devAddr, tcid, portId, static_cast<uint8_t>(TransferState::TRANSFER_AVAILABLE));
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    // 栈收数分发矩阵
    instance.HandleReceiveDataEvent(address, portId, datas); // 正常端口：DistributeDataFromStack 命中 socket
    instance.HandleReceiveDataEvent(address, CAR_PORT, datas); // CAR_PORT 命中 ICCE 缓存
    instance.HandleReceiveDataEvent("FF:EE:00:00:00:00", CAR_PORT, datas); // 无车钥匙应用注册
    instance.HandleReceiveDataEvent(address, UINT16_MAX, datas); // 端口未缓存
    TRANS_Addr_S stackAddr;
    RawAddress stackRawAddr(address);
    stackRawAddr.ConvertToUint8(stackAddr.devAddr.addr);
    stackAddr.dstPort = portId;
    uint16_t dataLen = static_cast<uint16_t>(datas.size());
    instance.ReceiveDataCallback(nullptr, datas.data(), dataLen); // stack cb addr null
    instance.ReceiveDataCallback(&stackAddr, datas.data(), 0); // stack cb data len invalid
    instance.ReceiveDataCallback(&stackAddr, datas.data(), dataLen);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(CAR_UUID);
    param.SetPort(portId);
    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(CAR_UUID, portId);
    if (g_fd > 0) {
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
        g_fd = -1;
    }
}

void ConnectFlowFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    std::string uuid = BuildUuid(provider).ConvertToString();
    uint8_t frameType = provider.ConsumeIntegral<uint8_t>() % 4; // QOSM_TransConnFrameType_E 4 成员
    uint8_t transMode = provider.ConsumeIntegral<uint8_t>() % 4; // QOSM_TransMode_E 4 业务成员

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    g_fd = -1;
    uint16_t portId = instance.CreatePort(uuid);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);
    param.SetPort(portId);
    param.SetTransMode(transMode);
    param.SetFrameType(frameType);

    instance.Connect(param); // -> ConnectPeerPortInner -> ConnectAction：缓存连接请求 + 起定时器
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.IsDeviceConnectingOrConnected(address); // 连接请求挂起时 true 链

    // GetRemotePortByConnectionState（PortServiceObserver 线程内实现）状态迁移矩阵
    // 整改 P9-3：状态对改 FDP 域消费（SleConnectState 5 成员 %5；成对消费先落具名局部变量，
    // 规避实参求值顺序未指定；原六组常量语义对下沉种子 seed_dt_pair_*）
    int curStateA = static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 5);
    int prevStateA = static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 5);
    instance.GetRemotePortByConnectionState(address, curStateA, prevStateA); // normal disconnect response, ignore
    int curStateB = static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 5);
    int prevStateB = static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 5);
    instance.GetRemotePortByConnectionState(address, curStateB, prevStateB); // invalid state changed
    int curStateC = static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 5);
    int prevStateC = static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 5);
    instance.GetRemotePortByConnectionState("FF:EE:00:00:00:00", curStateC, prevStateC); // not connect req
    int curStateD = static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 5);
    int prevStateD = static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 5);
    instance.GetRemotePortByConnectionState(address, curStateD, prevStateD); // 超时上报 + 清连接缓存/定时器
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    instance.Connect(param); // 超时清理后重连（重建请求缓存）
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    int curStateE = static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 5);
    int prevStateE = static_cast<int>(provider.ConsumeIntegral<uint8_t>() % 5);
    instance.GetRemotePortByConnectionState(address, curStateE, prevStateE); // -> GetRemotePortCreateChannel：对端端口为 0 失败链
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    instance.Connect(param); // 重建请求缓存
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.Connect(param); // 请求已缓存 -> ignore repeat connect
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.ConnectTimeout(address, portId); // 直调：请求命中 -> erase -> 清定时器 -> ReportDisconnectState 全路径
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.IsDeviceConnectingOrConnected(address); // 清理后 false 链
    instance.DestroyPort(uuid, portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void CarKeyFlowFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string address = BuildAddressString(provider);
    size_t length = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    std::vector<uint8_t> datas = provider.ConsumeBytes<uint8_t>(length);
    while (datas.size() < length) {
        datas.push_back(0);
    }

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    g_fd = -1;
    uint16_t portId = instance.CreatePort(CAR_UUID);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");
    uint16_t repeatPortId = instance.CreatePort(CAR_UUID); // 重复创建：ICCE 端口复用早退
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portId, DEFAULT_CAR_MTU,
        BuildConnectedParam(address, CAR_PORT, TCID_SLE_DEFAULT));
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    if (g_fd > 0) {
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
        g_fd = -1;
    }

    DataTransferConnectionParams carParams;
    carParams.SetAddress(address);
    carParams.SetUuid(CAR_UUID);
    carParams.SetPort(portId);
    instance.ConnectCarChannelAfterAcb(carParams); // ICCE 取对端端口 + 重复连接事件忽略
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    if (g_fd > 0) {
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
        g_fd = -1;
    }

    RawAddress rawAddr(address);
    int32_t acbDiscState = static_cast<int32_t>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED);
    instance.pimpl->sleDataTransferAcbObserverImp_->OnAcbStateChanged(rawAddr, acbDiscState, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    // 整改 P9-6：观察者状态对改 FDP 域消费（%5；成对消费先落具名局部变量；语义对下沉种子）
    int32_t connCur1 = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5);
    int32_t connPrev1 = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5);
    instance.pimpl->sleDataTransferAcbObserverImp_->OnConnectionStateChanged(rawAddr, connCur1, connPrev1, 0);
    int32_t connCur2 = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5);
    int32_t connPrev2 = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5);
    instance.pimpl->sleDataTransferAcbObserverImp_->OnConnectionStateChanged(rawAddr, connCur2, connPrev2, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    if (g_fd > 0) {
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
        g_fd = -1;
    }

    instance.Disconnect(carParams); // DisconnectInner ICCE 分支
    instance.HandleReceiveDataEvent(address, CAR_PORT, datas); // 车钥匙分发命中
    instance.HandleReceiveDataEvent("FF:EE:00:00:00:00", CAR_PORT, datas); // 无注册应用
    std::string proxyAddr;
    instance.IsProxyConnectExisted(proxyAddr); // future 族：Init 窗口内调用
    instance.StopNlProxyIfExisted();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    instance.ClearCacheByTokenId(tokenId); // 缓存淘汰：DisconnectCarWhenRemoteDie + DestroyPort 循环
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(CAR_UUID, portId); // 已淘汰时走 no cache 分支兜底
    HILOGI("carkey flow repeat port: %{public}u", static_cast<unsigned>(repeatPortId));
}

void MultiChannelRaceFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string addr1 = BuildAddressString(provider);
    std::string addr2 = BuildAddressString(provider);
    std::string uuid1 = BuildUuid(provider).ConvertToString();
    std::string uuid2 = BuildUuid(provider).ConvertToString();
    uint8_t statusSel = provider.ConsumeIntegral<uint8_t>() % 4; // QOSM_TransChannelStatus_E 4 成员
    uint8_t mtuSel = provider.ConsumeIntegral<uint8_t>() % 3;
    size_t length = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    std::vector<uint8_t> datas = provider.ConsumeBytes<uint8_t>(length);
    while (datas.size() < length) {
        datas.push_back(0);
    }
    const uint16_t mtuTable[3] = {0, 1, UINT16_MAX}; // MTU 协商边界档位
    uint16_t mtu = mtuTable[mtuSel];

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    g_fd = -1;
    uint16_t portA = instance.CreatePort(uuid1);
    NL_CHECK_RETURN(portA > 0, "portA is invalid");
    uint16_t portB = instance.CreatePort(uuid2);
    NL_CHECK_RETURN(portB > 0, "portB is invalid");

    // 并发通道矩阵：portA×addr1 / portA×addr2 / portB×addr1（同地址双端口竞争）
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portA, UINT16_MAX, BuildConnectedParam(addr1, 1, 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    if (g_fd > 0) {
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
        g_fd = -1;
    }
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portA, UINT16_MAX, BuildConnectedParam(addr2, 2, 1));
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    if (g_fd > 0) {
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
        g_fd = -1;
    }
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portB, mtu, BuildConnectedParam(addr1, 3, 2));
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    if (g_fd > 0) {
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
        g_fd = -1;
    }

    DataTransferDataParams writeA(addr1, uuid1, portA, datas);
    DataTransferDataParams writeB(addr1, uuid2, portB, datas);
    instance.WriteData(writeA); // 同地址双通道并发写
    instance.WriteData(writeB);
    DataTransferConnectionParams paramA;
    paramA.SetAddress(addr1);
    paramA.SetUuid(uuid1);
    paramA.SetPort(portA);
    instance.GetConnectionState(paramA);
    DataTransferConnectionParams paramB;
    paramB.SetAddress(addr1);
    paramB.SetUuid(uuid2);
    paramB.SetPort(portB);
    instance.GetConnectionState(paramB);

    instance.IsValidSrcPort(portA); // 命中
    instance.IsValidSrcPort(0); // 未命中
    instance.CheckChannelParamCallback(portB); // 栈 establishedCheck 同源

    QOSM_TransChannelRspParams_S respParams = {};
    RawAddress raw1(addr1);
    raw1.ConvertToUint8(respParams.addr.addr);
    respParams.srcPort = portB;
    respParams.dstPort = 3;
    respParams.tcid = 2;
    respParams.mtu = mtu;
    respParams.status = static_cast<QOSM_TransChannelStatus_E>(statusSel); // 四态分发
    instance.ChannelStatusCallback(&respParams);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portA, UINT16_MAX,
        BuildConnectedParam(addr1, 1, 1)); // 重复连接事件：socket 创建前判定忽略
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));

    // 销毁竞争：addr1 仍被 portB 占用 -> NotifyDisconnect 被拦截；addr2 无占用 -> 放行断链
    instance.DestroyPort(uuid1, portA);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(uuid2, portB);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void EdgeCallbackFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    uint8_t tcid = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    size_t length = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    std::string address = BuildAddressString(provider);
    std::vector<uint8_t> datas = provider.ConsumeBytes<uint8_t>(length);
    while (datas.size() < length) {
        datas.push_back(0);
    }

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    g_fd = -1;
    uint16_t portId = instance.CreatePort(CAR_UUID);
    NL_CHECK_RETURN(portId > 0, "portId is invalid");

    // 注销回调后的空回调分支与注册守卫
    instance.DeregisterSleDataTransferServiceCallback();
    AppConnectParamMapping pendingParam;
    pendingParam.address = address;
    pendingParam.dstPort = 1;
    pendingParam.tcid = tcid;
    pendingParam.state = static_cast<int32_t>(SleConnectState::CONNECTING);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISH_FAIL, portId, 1, pendingParam); // callback is nullptr
    AppConnectParamMapping discParam;
    discParam.address = address;
    discParam.dstPort = 1;
    discParam.tcid = tcid;
    discParam.state = static_cast<int32_t>(SleConnectState::DISCONNECTED);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_RELEASED, portId, 0, discParam); // DestroyPeerPort 分支
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.RegisterSleDataTransferServiceCallback(nullptr); // callback come null 守卫
    instance.RegisterSleDataTransferServiceCallback(g_callback); // 恢复注册
    instance.RegisterSleDataTransferServiceCallback(g_callback); // already register cb 守卫

    instance.DeregisterCallback(); // 栈回调注销（析构同款路径）
    instance.RegisterSleDataTransferCallbackToStack(); // 立即重注册恢复

    DataTransferDataParams missPortParams(address, CAR_UUID, UINT16_MAX, datas);
    instance.WriteData(missPortParams); // 端口未缓存
    int transState = 0;
    int preTransState = 0;
    instance.GetConnectionTransferState(UINT16_MAX, address, transState, preTransState); // no cache 分支
    instance.IsValidSrcPort(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
    instance.DestroyPort(CAR_UUID, portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::HOST_FUZZ_DELAY_10_MS));
}

void DataTransferCacheDirectFuzzTest001(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    std::string addr1 = BuildAddressString(provider);
    std::string addr2 = BuildAddressString(provider);
    uint8_t tcid = max(static_cast<int>(provider.ConsumeIntegral<uint8_t>()), 1);
    int32_t fuzzState = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5); // SleConnectState 5 成员

    // 缓存直驱：命中/失效/淘汰全路径（栈对象，无全局副作用）
    SleDataTransferCache cache;
    cache.SetUuid("060D");
    cache.SetTokenId(12345);
    cache.SetUid(678);
    cache.SetPid(910);
    cache.GetUuid();
    cache.GetTokenId();
    cache.GetUid();
    cache.GetPid();

    AppConnectParamMapping emptyEntry;
    emptyEntry.address = "";
    emptyEntry.tcid = tcid;
    cache.SetPortConnects(emptyEntry); // address invalid 拒绝插入

    AppConnectParamMapping entry1;
    entry1.address = addr1;
    entry1.randomAddress = addr2;
    entry1.dstPort = 1;
    entry1.tcid = tcid;
    entry1.state = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5); // 整改 P9-4：态入参 FDP
    cache.SetPortConnects(entry1);
    AppConnectParamMapping entry2;
    entry2.address = addr2;
    entry2.dstPort = 2;
    entry2.tcid = tcid;
    entry2.state = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5); // 整改 P9-4：态入参 FDP
    cache.SetPortConnects(entry2);
    std::vector<AppConnectParamMapping> connects = cache.GetPortConnects();

    AppConnectParamMapping queryParam;
    bool has1 = cache.GetAppConnectParamByAddr(addr1, queryParam); // 命中
    bool hasMiss = cache.GetAppConnectParamByAddr("FF:EE:00:00:00:00", queryParam); // 失效
    bool hasApp = cache.HasAppConnect();
    bool hasRemote = cache.HasRemoteAddressConnect(addr1);
    bool hasRemoteMiss = cache.HasRemoteAddressConnect("FF:EE:00:00:00:00");
    std::string appConnAddr;
    bool isApp = cache.IsAppConnect(appConnAddr);
    bool hasTarget = cache.HasTargetAddr(addr2);
    bool hasTargetMiss = cache.HasTargetAddr("FF:EE:00:00:00:00");
    std::string randomAddr = cache.GetRandomAddrByAddr(addr1); // 命中返回 randomAddress
    std::string randomMiss = cache.GetRandomAddrByAddr("FF:EE:00:00:00:00"); // 失效原样返回

    // 整改 P9-4：迁移/更新族全部态入参改 FDP 域消费（%5；具名局部变量先消费再传参）
    TransferState transState1 = static_cast<TransferState>(provider.ConsumeIntegral<uint8_t>() % 5);
    cache.UpdateTransferState(addr1, tcid, transState1); // 命中迁移
    TransferState transState2 = static_cast<TransferState>(provider.ConsumeIntegral<uint8_t>() % 5);
    cache.UpdateTransferState(addr1, static_cast<uint8_t>(tcid + 1), transState2); // 不匹配
    int32_t updState1 = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5);
    uint8_t gotTcid = cache.UpdateStateAndGetTcidByAddr(addr1, updState1); // 命中返回 tcid
    int32_t updState2 = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5);
    uint8_t missTcid = cache.UpdateStateAndGetTcidByAddr("FF:EE:00:00:00:00", updState2); // 失效返回 0
    int32_t updState3 = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5);
    cache.UpdateStateByAddr(addr2, updState3);
    int32_t updState4 = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5);
    cache.UpdateStateByAddr("FF:EE:00:00:00:00", updState4);
    int32_t updStateAll = static_cast<int32_t>(provider.ConsumeIntegral<uint8_t>() % 5);
    cache.UpdateState(updStateAll); // 全量置目标态
    bool hasAppAfter = cache.HasAppConnect(); // 置态后查询
    std::string appConnAddrAfter;
    bool isAppAfter = cache.IsAppConnect(appConnAddrAfter);
    bool hasRemoteAfter = cache.HasRemoteAddressConnect(addr1);
    cache.UpdateState(fuzzState); // FDP 状态注入（%5 枚举域）
    HILOGI("cache sweep: has1=%{public}d miss=%{public}d hasApp=%{public}d hasAppAfter=%{public}d "
           "gotTcid=%{public}u missTcid=%{public}u size=%{public}zu",
        has1, hasMiss, hasApp, hasAppAfter, static_cast<unsigned>(gotTcid), static_cast<unsigned>(missTcid),
        connects.size());
    (void)hasRemote;
    (void)hasRemoteMiss;
    (void)isApp;
    (void)hasTarget;
    (void)hasTargetMiss;
    (void)randomAddr;
    (void)randomMiss;
    (void)isAppAfter;
    (void)hasRemoteAfter;
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
    OHOS::WriteDataChannelFuzzTest001(data, size);
    OHOS::SocketDataFrameFuzzTest001(data, size);
    OHOS::ConnectFlowFuzzTest001(data, size);
    OHOS::CarKeyFlowFuzzTest001(data, size);
    OHOS::MultiChannelRaceFuzzTest001(data, size);
    OHOS::EdgeCallbackFuzzTest001(data, size);
    OHOS::DataTransferCacheDirectFuzzTest001(data, size);
    OHOS::DeregisterSleDataTransferServiceCallbackFuzzTest001();
    OHOS::TransferBizFuzzTest(data, size);
    OHOS::CarKeyConnectFuzzTest(data, size);
    OHOS::TransferDisconnectFuzzTest(data, size);
    OHOS::g_threadUtil.ClearThreadStateMap();
    return 0;
}
