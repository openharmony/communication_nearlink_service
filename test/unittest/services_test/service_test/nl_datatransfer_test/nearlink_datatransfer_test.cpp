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

#include <gtest/gtest.h>
#include "nearlink_socket_outputstream.h"
#include "nearlink_datatransfer_def.h"
#include "nearlink_datatransfer_parcel.h"
#include "nearlink_sle_datatransfer_service.cpp"
#include "SleInterfaceDataTransfer.h"
#include "ClassCreator.h"
#include "log.h"
#include <thread>
#include "qosm_trans_channel.h"
#include "nearlink_fdsan_tag.h"
#include "nearlink_verification_manager.h"
#include "port_type.h"
#include "mock_control.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
namespace {
int g_fd = -1;
const std::string DEVICE_SLE_ADDR = "11:22:33:44:55:66";
const std::string CAR_UUID = "060D";
const std::string PORT_UUID = "00090000-0001-0002-0000-000000000099";
constexpr int32_t INTERVAL_TYPE = 0x24;
constexpr uint16_t CAR_PORT = 40960;
constexpr uint16_t CAR_TCID = 0x1F;
constexpr uint16_t PORT_TCID = 0x80;
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

class NearlinkDataTransferTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkDataTransferTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    // 单测不启动数传与port工作线程：置为同步执行，投递入口的断言即时生效
    ThreadUtil::GetInstance().InitThreadStateMap();
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_DATATRANSFER, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_PORT, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    HILOGI("SetUpTestCase end");
}

void NearlinkDataTransferTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
}

void NearlinkDataTransferTest::SetUp()
{
    HILOGI("SetUp start");
    MockSleAdapterReset();
    MockPortClientReset();
    MockIcceServiceSetGetPort(40960);
    NearlinkVerificationManager::GetInstance().ClearStrategies();
}

void NearlinkDataTransferTest::TearDown()
{
    HILOGI("TearDown start");
    if (g_fd > 0) {
        fdsan_close_with_tag(g_fd, NEARLINK_FDSAN_TAG_SOCKET);
    }
    g_fd = -1;
}

/**
 * @tc.number: CreatePortAndDestroyPort001
 * @tc.name: Test CreatePort and DestroyPort for public port.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, CreatePortAndDestroyPort001, TestSize.Level1)
{
    HILOGI("CreatePortAndDestroyPort001 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    std::string uuid = CAR_UUID;
    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    portId = instance.CreatePort(uuid); // repeat createPort
    EXPECT_EQ(portId > 0, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    HILOGI("CreatePortAndDestroyPort001 end");
}

/**
 * @tc.number: CreatePortAndDestroyPort002
 * @tc.name: Test CreatePort and DestroyPort for private port.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, CreatePortAndDestroyPort002, TestSize.Level1)
{
    HILOGI("CreatePortAndDestroyPort002 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    std::string uuid = PORT_UUID;
    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    HILOGI("CreatePortAndDestroyPort002 end");
}

/**
 * @tc.number: RegisterSleDataTransferServiceCallback001
 * @tc.name: Test RegisterSleDataTransferServiceCallback.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, RegisterSleDataTransferServiceCallback001, TestSize.Level1)
{
    HILOGI("RegisterSleDataTransferServiceCallback001 start");
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    HILOGI("RegisterSleDataTransferServiceCallback001 end");
}

/**
 * @tc.number: DeregisterSleDataTransferServiceCallback001
 * @tc.name: Test DeregisterSleDataTransferServiceCallback.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, DeregisterSleDataTransferServiceCallback001, TestSize.Level1)
{
    HILOGI("DeregisterSleDataTransferServiceCallback001 start");
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.DeregisterSleDataTransferServiceCallback();
    HILOGI("DeregisterSleDataTransferServiceCallback001 end");
}

/**
 * @tc.number: ConnectAndDisconnect001
 * @tc.name: Test Connect and DisConnect for public port.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ConnectAndDisConnect001, TestSize.Level1)
{
    HILOGI("ConnectAndDisConnect001 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    std::string uuid = CAR_UUID;
    std::string address = DEVICE_SLE_ADDR;
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    param.SetPort(portId);
    instance.Connect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    int32_t state = instance.GetConnectionState(param);
    EXPECT_EQ(state == static_cast<int32_t>(SleConnectState::CONNECTED), true);
    DataTransferConnectionParams invParam = param;

    invParam.SetPort(0);
    state = instance.GetConnectionState(invParam);
    EXPECT_EQ(state == static_cast<int32_t>(SleConnectState::CONNECTING), true);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    instance.DeregisterSleDataTransferServiceCallback();
    HILOGI("ConnectAndDisConnect001 end");
}

/**
 * @tc.number: ConnectAndDisConnect002
 * @tc.name: Test Connect and DisConnect for private port.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ConnectAndDisConnect002, TestSize.Level1)
{
    HILOGI("ConnectAndDisConnect002 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    std::string uuid = PORT_UUID;
    std::string address = DEVICE_SLE_ADDR;
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.Connect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    int32_t state = instance.GetConnectionState(param);
    EXPECT_EQ(state == static_cast<int32_t>(SleConnectState::CONNECTING), true);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    instance.DeregisterSleDataTransferServiceCallback();
    HILOGI("ConnectAndDisConnect002 end");
}

/**
 * @tc.number: HandleConnectEvent001
 * @tc.name: Test HandleConnectEvent for public port.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, HandleConnectEvent001, TestSize.Level1)
{
    HILOGI("HandleConnectEvent001 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    std::string uuid = CAR_UUID; // CAR_UUID_ICCE
    std::string address = DEVICE_SLE_ADDR;
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    AppConnectParamMapping temp;
    temp.address = address;
    temp.dstPort = CAR_PORT;
    temp.tcid = CAR_TCID;
    temp.state = static_cast<int32_t>(SleConnectState::CONNECTED);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, param.GetPort(), UINT16_MAX, temp);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, param.GetPort(), UINT16_MAX, temp); // repeat
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(g_fd > 0, true);
    std::shared_ptr<OutputStream> outputStream = std::make_shared<OutputStream>(g_fd);
    std::vector<uint8_t> datas = {1, 2, 3, 4, 5, 6};
    size_t totalLen = 0;
    DataTransferDataParams dataTransfer(address, uuid, portId, datas);
    std::unique_ptr<uint8_t[]> packageData = NearlinkDataTransferDataParams::SerializeData(dataTransfer, totalLen);
    outputStream->Write(packageData.get(), totalLen);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    instance.HandleReceiveDataEvent(temp.address, temp.dstPort, datas);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    instance.DeregisterSleDataTransferServiceCallback();
    HILOGI("HandleConnectEvent001 end");
}

/**
 * @tc.number: HandleConnectEvent002
 * @tc.name: Test HandleConnectEvent for private port.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, HandleConnectEvent002, TestSize.Level1)
{
    HILOGI("HandleConnectEvent002 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    std::string uuid = CAR_UUID;
    std::string address = DEVICE_SLE_ADDR;
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    AppConnectParamMapping temp;
    temp.address = address;
    temp.dstPort = portId;
    temp.tcid = CAR_TCID;
    temp.state = static_cast<int32_t>(SleConnectState::CONNECTED);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portId, UINT16_MAX, temp);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(g_fd > 0, true);
    std::shared_ptr<OutputStream> outputStream = std::make_shared<OutputStream>(g_fd);
    std::vector<uint8_t> datas = {1, 2, 3, 4, 5, 6};
    uint16_t len = datas.size();
    TRANS_Addr_S stackAddr;
    RawAddress addr(address);
    addr.ConvertToUint8(stackAddr.devAddr.addr);
    stackAddr.dstPort = temp.dstPort;
    instance.ReceiveDataCallback(&stackAddr, datas.data(), len);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    instance.DeregisterSleDataTransferServiceCallback();
    HILOGI("HandleConnectEvent002 end");
}

/**
 * @tc.number: ChannelStatusCallback001
 * @tc.name: Test ChannelStatusCallback.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ChannelStatusCallback001, TestSize.Level1)
{
    HILOGI("HandleConnectEvent002 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    std::string uuid = PORT_UUID;
    std::string address = DEVICE_SLE_ADDR;
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    QOSM_TransChannelRspParams_S respParams;
    RawAddress addr(address);
    addr.ConvertToUint8(respParams.addr.addr);
    respParams.tcid = PORT_TCID;
    respParams.srcPort = portId;
    respParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;
    respParams.mtu = UINT16_MAX;
    instance.ChannelStatusCallback(&respParams);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(g_fd > 0, true);

    instance.Disconnect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    instance.DeregisterSleDataTransferServiceCallback();
    HILOGI("HandleConnectEvent002 end");
}

/**
 * @tc.number: SendDataStateCallback001
 * @tc.name: Test SendDataStateCallback.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, SendDataStateCallback001, TestSize.Level1)
{
    HILOGI("SendDataStateCallback001 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    std::string uuid = CAR_UUID; // CAR_UUID_ICCE
    std::string address = DEVICE_SLE_ADDR;
    uint8_t tcid = CAR_TCID;
    uint16_t dstPort = CAR_PORT;

    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    AppConnectParamMapping temp;
    temp.address = address;
    temp.dstPort = dstPort;
    temp.tcid = tcid;
    temp.state = static_cast<int32_t>(SleConnectState::CONNECTED);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, portId, UINT16_MAX, temp);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(g_fd > 0, true);
    RawAddress addr(address);
    SLE_Addr_S devAddr;
    addr.ConvertToUint8(devAddr.addr);
    uint8_t result = TransferState::TRANSFER_AVAILABLE;
    instance.SendDataStateCallback(&devAddr, tcid, portId, result);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    instance.DeregisterSleDataTransferServiceCallback();
    HILOGI("SendDataStateCallback001 end");
}

/**
 * @tc.number: OnAcbStateChanged001
 * @tc.name: Test OnAcbStateChanged.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, OnAcbStateChanged001, TestSize.Level1)
{
    HILOGI("OnAcbStateChanged001 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    std::string uuid = CAR_UUID; // CAR_UUID_ICCE
    std::string address = DEVICE_SLE_ADDR;
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    instance.Connect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    RawAddress addr(address);
    int32_t state = static_cast<int32_t>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED);
    int reason = 0;
    instance.pimpl->sleDataTransferAcbObserverImp_->OnAcbStateChanged(addr, state, reason);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    instance.DeregisterSleDataTransferServiceCallback();
    HILOGI("OnAcbStateChanged001 end");
}

/**
 * @tc.number: PortServiceOnConnectionStateChanged001
 * @tc.name: Test PortServiceOnConnectionStateChanged.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, PortServiceOnConnectionStateChanged001, TestSize.Level1)
{
    HILOGI("PortServiceOnConnectionStateChanged001 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    std::string uuid = PORT_UUID;
    std::string address = DEVICE_SLE_ADDR;
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    instance.Connect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    RawAddress addr(address);
    int state = static_cast<int>(SleConnectState::CONNECTED);
    int oldState = static_cast<int>(SleConnectState::CONNECTING);
    instance.pimpl->dataTransferPortObserver_.OnConnectionStateChanged(addr, state, oldState);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    instance.DeregisterSleDataTransferServiceCallback();
    HILOGI("PortServiceOnConnectionStateChanged001 end");
}

/**
 * @tc.number: PortServiceOnConnectionStateChanged002
 * @tc.name: Test PortServiceOnConnectionStateChanged.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, PortServiceOnConnectionStateChanged002, TestSize.Level1)
{
    HILOGI("PortServiceOnConnectionStateChanged002 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    std::string uuid = PORT_UUID;
    std::string address = DEVICE_SLE_ADDR;
    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    instance.Connect(param);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    RawAddress addr(address);
    int state = static_cast<int>(SleConnectState::DISCONNECTED);
    int oldState = static_cast<int>(SleConnectState::CONNECTING);
    instance.pimpl->dataTransferPortObserver_.OnConnectionStateChanged(addr, state, oldState);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    instance.DeregisterSleDataTransferServiceCallback();
    HILOGI("PortServiceOnConnectionStateChanged002 end");
}

/**
 * @tc.number: OnConnectionStateChanged001
 * @tc.name: Test OnConnectionStateChanged.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, OnConnectionStateChanged001, TestSize.Level1)
{
    HILOGI("OnConnectionStateChanged001 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    instance.RegisterSleDataTransferServiceCallback(g_callback);
    std::string uuid = CAR_UUID; // CAR_UUID_ICCE
    std::string address = DEVICE_SLE_ADDR;
    uint8_t tcid = CAR_TCID;

    DataTransferConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);

    uint16_t portId = instance.CreatePort(uuid);
    EXPECT_EQ(portId > 0, true);
    param.SetPort(portId);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.Connect(param);

    RawAddress addr(address);
    int32_t state = static_cast<int32_t>(SleConnectState::CONNECTED);
    int32_t preState = static_cast<int32_t>(SleConnectState::DISCONNECTED);
    int reason = 0;
    instance.pimpl->sleDataTransferAcbObserverImp_->OnConnectionStateChanged(addr, state, preState, reason);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    instance.DestroyPort(uuid, portId);
    instance.DeregisterSleDataTransferServiceCallback();
    HILOGI("OnConnectionStateChanged001 end");
}

/**
 * @tc.number: DisconnectCarWhenRemoteDie001
 * @tc.name: Test DisconnectCarWhenRemoteDie.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, DisconnectCarWhenRemoteDie001, TestSize.Level1)
{
    HILOGI("DisconnectCarWhenRemoteDie001 start");
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    std::string uuid = PORT_UUID;
    std::string address = DEVICE_SLE_ADDR;
    instance.DisconnectCarWhenRemoteDie(uuid, address);
    HILOGI("DisconnectCarWhenRemoteDie001 end");
}

/**
 * @tc.number: DataTransferCache001
 * @tc.name: Test DataTransferCache001.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, DataTransferCache001, TestSize.Level1)
{
    HILOGI("DataTransferCache001 start");
    std::shared_ptr<SleDataTransferCache> dataCache = std::make_shared<SleDataTransferCache>();
    AppConnectParamMapping temp;
    temp.address = DEVICE_SLE_ADDR;
    temp.state == static_cast<int32_t>(SleConnectState::CONNECTING);
    dataCache->SetPortConnects(temp);
    bool ret = dataCache->HasAppConnect();
    EXPECT_EQ(ret, true);
    HILOGI("DataTransferCache001 end");
}

namespace {
const std::string ADDR_OTHER = "AA:BB:CC:DD:EE:FF";

class RecordingCallback : public OHOS::Nearlink::ISleDataTransferServiceCallback {
public:
    struct Event {
        std::string uuid;
        std::string address;
        uint16_t port;
        int32_t state;
        int fd;
    };

    void OnConnectionStateChanged(const DataTransferConnectionParams &params, int fd) override
    {
        Event event;
        event.uuid = params.GetUuid();
        event.address = params.GetAddress();
        event.port = params.GetPort();
        event.state = params.GetState();
        event.fd = fd;
        events_.push_back(event);
    }

    std::string GetRandomAddr(const std::string &address, uint32_t tokenId) override
    {
        return address;
    }

    std::vector<Event> events_;
};

class StubProxyStrategy : public IVerificationStrategy {
public:
    explicit StubProxyStrategy(bool result) : result_(result) {}

    bool Check(const VerificationContext &ctx) override
    {
        return result_;
    }

    VerificationType GetType() override
    {
        return VerificationType::DATATRANSFER_PROXY;
    }

private:
    bool result_;
};

AppConnectParamMapping MakeCacheParam(const std::string &addr, int32_t state, uint8_t tcid = 0,
    int32_t transState = 0)
{
    AppConnectParamMapping temp;
    temp.address = addr;
    temp.state = state;
    temp.tcid = tcid;
    temp.transState = transState;
    temp.preTransState = 0;
    return temp;
}

void SetCacheParam(SleDataTransferService &instance, uint16_t port, const AppConnectParamMapping &param)
{
    EXPECT_TRUE(instance.pimpl->UpdateAppConnectParamMapping(port, param));
}

DataTransferConnectionParams MakeConnectParams(const std::string &uuid, const std::string &addr, uint16_t port)
{
    DataTransferConnectionParams params;
    params.SetUuid(uuid);
    params.SetAddress(addr);
    params.SetPort(port);
    return params;
}
}

/**
 * @tc.number: ConnectPeerPortInner_CarKeyOccupied001
 * @tc.name: Test car key connection blocks peer port connect.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ConnectPeerPortInner_CarKeyOccupied001, TestSize.Level1)
{
    // 车钥匙已在连且代理校验通过时，普通端口的连接请求被直接拒绝并回调断开状态
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    uint16_t carPort = instance.CreatePort(CAR_UUID);
    SetCacheParam(instance, carPort, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED)));
    uint16_t appPort = instance.CreatePort(PORT_UUID);
    NearlinkVerificationManager::GetInstance().RegisterStrategy(std::make_shared<StubProxyStrategy>(true));

    instance.ConnectPeerPortInner(MakeConnectParams(PORT_UUID, DEVICE_SLE_ADDR, appPort));
    instance.DestroyPort(CAR_UUID, carPort);
    instance.DestroyPort(PORT_UUID, appPort);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_EQ(callback->events_.size(), static_cast<size_t>(1));
    EXPECT_EQ(callback->events_[0].state, static_cast<int32_t>(SleConnectState::DISCONNECTED));
    EXPECT_EQ(callback->events_[0].uuid, PORT_UUID);
}

/**
 * @tc.number: ConnectPeerPortInner_PortDisconnected001
 * @tc.name: Test connect request cached and repeat ignored when port disconnected.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ConnectPeerPortInner_PortDisconnected001, TestSize.Level1)
{
    // 对端端口未连时发起连接：请求入缓存并置连接中；同地址重复请求被去重忽略
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t port = instance.CreatePort(PORT_UUID);
    DataTransferConnectionParams params = MakeConnectParams(PORT_UUID, DEVICE_SLE_ADDR, port);

    instance.ConnectPeerPortInner(params);
    instance.ConnectPeerPortInner(params);
    DataTransferConnectionParams query = params;
    EXPECT_EQ(instance.GetConnectionState(query),
        static_cast<int32_t>(SleConnectState::CONNECTING));
    EXPECT_EQ(instance.pimpl->appConnectInfo_.count(DEVICE_SLE_ADDR), static_cast<size_t>(1));

    instance.pimpl->ClearConnectReqAndTimer(DEVICE_SLE_ADDR);
    SetCacheParam(instance, port, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::DISCONNECTED)));
    instance.DestroyPort(PORT_UUID, port);
}

/**
 * @tc.number: ConnectPeerPortInner_PortConnected001
 * @tc.name: Test connected port reports cached state directly.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ConnectPeerPortInner_PortConnected001, TestSize.Level1)
{
    // 端口已连且缓存里有该地址的已连参数时，不再重建通道，直接按缓存状态回调
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    MockPortClientSetConnectState(PORT_CONNECTED);
    uint16_t port = instance.CreatePort(PORT_UUID);
    SetCacheParam(instance, port, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED)));

    instance.ConnectPeerPortInner(MakeConnectParams(PORT_UUID, DEVICE_SLE_ADDR, port));
    instance.DestroyPort(PORT_UUID, port);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_EQ(callback->events_.size(), static_cast<size_t>(1));
    EXPECT_EQ(callback->events_[0].state, static_cast<int32_t>(SleConnectState::CONNECTED));
    EXPECT_EQ(callback->events_[0].fd, -1);
}

/**
 * @tc.number: ConnectPeerPortInner_NoParamFallback001
 * @tc.name: Test missing remote port reports disconnect.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ConnectPeerPortInner_NoParamFallback001, TestSize.Level1)
{
    // 端口已连但缓存无该地址参数时走建通道流程；对端端口拿不到则回退上报断开
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    MockPortClientSetConnectState(PORT_CONNECTED);
    MockPortClientSetDevicePortId(0);
    uint16_t port = instance.CreatePort(PORT_UUID);

    instance.ConnectPeerPortInner(MakeConnectParams(PORT_UUID, DEVICE_SLE_ADDR, port));
    instance.DestroyPort(PORT_UUID, port);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_EQ(callback->events_.size(), static_cast<size_t>(1));
    EXPECT_EQ(callback->events_[0].state, static_cast<int32_t>(SleConnectState::DISCONNECTED));
    DataTransferConnectionParams query = MakeConnectParams(PORT_UUID, DEVICE_SLE_ADDR, port);
    EXPECT_EQ(instance.GetConnectionState(query),
        static_cast<int32_t>(SleConnectState::DISCONNECTED));
}

/**
 * @tc.number: ConnectCarInner_AcbSplit001
 * @tc.name: Test car connect splits by ACB connection state.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ConnectCarInner_AcbSplit001, TestSize.Level1)
{
    // ACB 未连时先缓存连接中并触发全 profile 连接；ACB 已连时直接按已建通道回调
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    uint16_t carPort = instance.CreatePort(CAR_UUID);
    DataTransferConnectionParams params = MakeConnectParams(CAR_UUID, DEVICE_SLE_ADDR, carPort);

    MockSleAdapterSetAcbConnected(false);
    instance.ConnectCarInner(params);
    EXPECT_EQ(MockSleAdapterGetConnectAllProfileCount(), static_cast<uint32_t>(1));
    DataTransferConnectionParams query = params;
    EXPECT_EQ(instance.GetConnectionState(query),
        static_cast<int32_t>(SleConnectState::CONNECTING));

    MockSleAdapterSetAcbConnected(true);
    instance.ConnectCarInner(params);
    instance.Disconnect(params);
    instance.DestroyPort(CAR_UUID, carPort);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_GE(callback->events_.size(), static_cast<size_t>(1));
    EXPECT_EQ(callback->events_.back().state, static_cast<int32_t>(SleConnectState::CONNECTED));
    EXPECT_GT(callback->events_.back().fd, 0);
}

/**
 * @tc.number: ConnectCarChannelAfterAcb_IccePortFail001
 * @tc.name: Test car channel falls back to default port when icce port invalid.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ConnectCarChannelAfterAcb_IccePortFail001, TestSize.Level1)
{
    // 车钥匙取对端端口失败时兜底用默认端口继续建链，仍按已连接回调
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    MockIcceServiceSetGetPort(-1);
    uint16_t carPort = instance.CreatePort(CAR_UUID);
    SetCacheParam(instance, carPort, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTING)));

    DataTransferConnectionParams params = MakeConnectParams(CAR_UUID, DEVICE_SLE_ADDR, carPort);
    instance.ConnectCarChannelAfterAcb(params);
    instance.Disconnect(params);
    instance.DestroyPort(CAR_UUID, carPort);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_EQ(callback->events_.size(), static_cast<size_t>(1));
    EXPECT_EQ(callback->events_[0].state, static_cast<int32_t>(SleConnectState::CONNECTED));
    EXPECT_GT(callback->events_[0].fd, 0);
}

/**
 * @tc.number: ConnectTimeout_ReportDisconnect001
 * @tc.name: Test connect timeout clears request and reports disconnect.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ConnectTimeout_ReportDisconnect001, TestSize.Level1)
{
    // 连接超时后请求缓存与定时器被清掉，应用收到断开状态回调
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    uint16_t port = instance.CreatePort(PORT_UUID);
    instance.ConnectPeerPortInner(MakeConnectParams(PORT_UUID, DEVICE_SLE_ADDR, port));

    instance.ConnectTimeout(DEVICE_SLE_ADDR, port);
    instance.DestroyPort(PORT_UUID, port);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_EQ(callback->events_.size(), static_cast<size_t>(1));
    EXPECT_EQ(callback->events_[0].state, static_cast<int32_t>(SleConnectState::DISCONNECTED));
    EXPECT_TRUE(instance.pimpl->appConnectInfo_.empty());
}

/**
 * @tc.number: CreatePortMergeRule001
 * @tc.name: Test port reuse rule by uuid and tokenId.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, CreatePortMergeRule001, TestSize.Level1)
{
    // 普通端口按 uuid 复用；车钥匙端口按 uuid 加 tokenId 复用，token 不同则新开
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t appPort = instance.CreatePortInner(PORT_UUID, 111, 1, 1);
    uint16_t appPortAgain = instance.CreatePortInner(PORT_UUID, 222, 2, 2);
    EXPECT_EQ(appPortAgain, appPort);
    uint16_t carPort = instance.CreatePortInner(CAR_UUID, 111, 1, 1);
    uint16_t carPortAgain = instance.CreatePortInner(CAR_UUID, 111, 1, 1);
    EXPECT_EQ(carPortAgain, carPort);
    uint16_t carPortNew = instance.CreatePortInner(CAR_UUID, 333, 3, 3);
    EXPECT_NE(carPortNew, carPort);

    instance.DestroyPort(PORT_UUID, appPort);
    instance.DestroyPort(CAR_UUID, carPort);
    instance.DestroyPort(CAR_UUID, carPortNew);
}

/**
 * @tc.number: CreatePortPoolExhausted001
 * @tc.name: Test create port returns zero when pool exhausted.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, CreatePortPoolExhausted001, TestSize.Level1)
{
    // 端口池全部占满时新建端口返回零；释放后可再次分配
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    for (uint16_t i = PORT_DEFAULT; i <= PORT_MAX; i++) {
        instance.ports_.insert(i);
    }
    EXPECT_EQ(instance.CreatePort(PORT_UUID), static_cast<uint16_t>(0));
    instance.ports_.clear();
    uint16_t port = instance.CreatePort(PORT_UUID);
    EXPECT_GT(port, static_cast<uint16_t>(0));
    instance.DestroyPort(PORT_UUID, port);
}

/**
 * @tc.number: ProxyConnectDetect001
 * @tc.name: Test proxy connect detect with stub verification strategy.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ProxyConnectDetect001, TestSize.Level1)
{
    // 代理校验桩返回真且车钥匙在连时判定存在代理连接；返回假时判定不存在
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t carPort = instance.CreatePort(CAR_UUID);
    SetCacheParam(instance, carPort, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED)));
    auto &manager = NearlinkVerificationManager::GetInstance();

    manager.RegisterStrategy(std::make_shared<StubProxyStrategy>(true));
    std::string devAddress;
    EXPECT_TRUE(instance.IsProxyConnectExisted(devAddress));

    manager.ClearStrategies();
    manager.RegisterStrategy(std::make_shared<StubProxyStrategy>(false));
    EXPECT_FALSE(instance.IsProxyConnectExisted(devAddress));
    instance.StopNlProxyIfExisted();

    manager.ClearStrategies();
    instance.DestroyPort(CAR_UUID, carPort);
}

/**
 * @tc.number: AcbObserver_ConnectedBuildCarChannel001
 * @tc.name: Test ACB connected event builds car channel.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, AcbObserver_ConnectedBuildCarChannel001, TestSize.Level1)
{
    // ACB 连接成功事件触发对连接中的车钥匙端口补建通道并回调已连接
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    uint16_t carPort = instance.CreatePort(CAR_UUID);
    SetCacheParam(instance, carPort, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTING)));

    instance.pimpl->sleDataTransferAcbObserverImp_->OnConnectionStateChanged(
        RawAddress(DEVICE_SLE_ADDR), static_cast<int32_t>(SleConnectState::CONNECTED),
        static_cast<int32_t>(SleConnectState::DISCONNECTED), 0);

    DataTransferConnectionParams params = MakeConnectParams(CAR_UUID, DEVICE_SLE_ADDR, carPort);
    instance.Disconnect(params);
    instance.DestroyPort(CAR_UUID, carPort);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_EQ(callback->events_.size(), static_cast<size_t>(1));
    EXPECT_EQ(callback->events_[0].state, static_cast<int32_t>(SleConnectState::CONNECTED));
    EXPECT_GT(callback->events_[0].fd, 0);
}

/**
 * @tc.number: AcbObserver_DisconnectedUpdatesState001
 * @tc.name: Test ACB disconnect event updates car key state.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, AcbObserver_DisconnectedUpdatesState001, TestSize.Level1)
{
    // ACB 断开事件把在连车钥匙端口置为断开并回调；端口广播断开时不重复处理
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    uint16_t carPort = instance.CreatePort(CAR_UUID);
    SetCacheParam(instance, carPort, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED)));

    instance.pimpl->sleDataTransferAcbObserverImp_->OnConnectionStateChanged(
        RawAddress(DEVICE_SLE_ADDR), static_cast<int32_t>(SleConnectState::DISCONNECTED),
        static_cast<int32_t>(SleConnectState::CONNECTED), 0);
    EXPECT_TRUE(callback->events_.empty());

    instance.pimpl->sleDataTransferAcbObserverImp_->OnAcbStateChanged(
        RawAddress(DEVICE_SLE_ADDR), static_cast<int32_t>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED), 0);
    instance.DestroyPort(CAR_UUID, carPort);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_EQ(callback->events_.size(), static_cast<size_t>(1));
    EXPECT_EQ(callback->events_[0].state, static_cast<int32_t>(SleConnectState::DISCONNECTED));
    DataTransferConnectionParams query = MakeConnectParams(CAR_UUID, DEVICE_SLE_ADDR, carPort);
    EXPECT_EQ(instance.GetConnectionState(query),
        static_cast<int32_t>(SleConnectState::DISCONNECTED));
}

/**
 * @tc.number: GetTransferState_BusyAndMissingPort001
 * @tc.name: Test transfer state query reports busy and inner fail.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, GetTransferState_BusyAndMissingPort001, TestSize.Level1)
{
    // 传输状态为忙时查询原样上报；端口不存在时返回内部错误码
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t port = instance.CreatePort(PORT_UUID);
    SetCacheParam(instance, port, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED), CAR_TCID, TransferState::TRANSFER_BUSY));

    EXPECT_EQ(instance.GetTransferState(port, DEVICE_SLE_ADDR),
        static_cast<int>(TransferState::TRANSFER_BUSY));
    EXPECT_EQ(instance.GetTransferState(port + 1, DEVICE_SLE_ADDR),
        static_cast<int>(TransferReturnValue::SLE_TRANS_INNER_FAIL));

    instance.DestroyPort(PORT_UUID, port);
}

/**
 * @tc.number: DisconnectInner_ByUuidType001
 * @tc.name: Test disconnect path split by car key and normal port.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, DisconnectInner_ByUuidType001, TestSize.Level1)
{
    // 车钥匙断开只改状态并回调；普通端口带通道号则释放通道，无通道号则拦截
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    uint16_t carPort = instance.CreatePort(CAR_UUID);
    SetCacheParam(instance, carPort, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED)));
    uint16_t portWithTcid = instance.CreatePort(PORT_UUID);
    SetCacheParam(instance, portWithTcid, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED), CAR_TCID));
    uint16_t portNoTcid = instance.CreatePortInner(PORT_UUID + "1", 900, 9, 9);
    SetCacheParam(instance, portNoTcid, MakeCacheParam(ADDR_OTHER,
        static_cast<int32_t>(SleConnectState::CONNECTED)));

    DataTransferConnectionParams carParams = MakeConnectParams(CAR_UUID, DEVICE_SLE_ADDR, carPort);
    instance.Disconnect(carParams);
    DataTransferConnectionParams portParams = MakeConnectParams(PORT_UUID, DEVICE_SLE_ADDR, portWithTcid);
    instance.Disconnect(portParams);
    DataTransferConnectionParams noTcidParams = MakeConnectParams(PORT_UUID + "1", ADDR_OTHER, portNoTcid);
    instance.Disconnect(noTcidParams);
    size_t eventCount = callback->events_.size();
    std::string eventUuid = eventCount > 0 ? callback->events_[0].uuid : "";
    DataTransferConnectionParams queryCar = MakeConnectParams(CAR_UUID, DEVICE_SLE_ADDR, carPort);
    int32_t carState = instance.GetConnectionState(queryCar);
    DataTransferConnectionParams queryPort = MakeConnectParams(PORT_UUID, DEVICE_SLE_ADDR, portWithTcid);
    int32_t portState = instance.GetConnectionState(queryPort);
    DataTransferConnectionParams queryNoTcid = MakeConnectParams(PORT_UUID + "1", ADDR_OTHER, portNoTcid);
    int32_t noTcidState = instance.GetConnectionState(queryNoTcid);

    instance.DestroyPort(CAR_UUID, carPort);
    instance.DestroyPort(PORT_UUID, portWithTcid);
    instance.DestroyPort(PORT_UUID + "1", portNoTcid);
    instance.DeregisterSleDataTransferServiceCallback();

    EXPECT_EQ(eventCount, static_cast<size_t>(1));
    EXPECT_EQ(eventUuid, CAR_UUID);
    EXPECT_EQ(carState, static_cast<int32_t>(SleConnectState::DISCONNECTED));
    EXPECT_EQ(portState, static_cast<int32_t>(SleConnectState::DISCONNECTING));
    EXPECT_EQ(noTcidState, static_cast<int32_t>(SleConnectState::DISCONNECTING));
}

/**
 * @tc.number: DestroyPort_ByUuidType001
 * @tc.name: Test destroy port cleanup rules.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, DestroyPort_ByUuidType001, TestSize.Level1)
{
    // 空uuid被拒绝；普通端口销毁会补发断链通知；端口与映射缓存均被清掉
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t port = instance.CreatePort(PORT_UUID);
    SetCacheParam(instance, port, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED), CAR_TCID));

    instance.DestroyPort("", port);
    EXPECT_EQ(instance.pimpl->FindUuidByPort(port), PORT_UUID);

    uint32_t disconnectCountBefore = MockPortClientGetDisconnectCount();
    instance.DestroyPort(PORT_UUID, port);
    EXPECT_EQ(instance.pimpl->FindUuidByPort(port), "");
    EXPECT_EQ(instance.ports_.count(port), static_cast<size_t>(0));
    EXPECT_GT(MockPortClientGetDisconnectCount(), disconnectCountBefore);
}

/**
 * @tc.number: PortObserver_TimeoutPath001
 * @tc.name: Test port observer timeout path reports disconnect.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, PortObserver_TimeoutPath001, TestSize.Level1)
{
    // 端口连接超时事件把挂起请求逐个回退为断开；无挂起请求时不再重复回调
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    uint16_t port = instance.CreatePort(PORT_UUID);
    instance.ConnectPeerPortInner(MakeConnectParams(PORT_UUID, DEVICE_SLE_ADDR, port));

    instance.pimpl->dataTransferPortObserver_.OnConnectionStateChanged(
        RawAddress(DEVICE_SLE_ADDR), static_cast<int>(SleConnectState::DISCONNECTED),
        static_cast<int>(SleConnectState::CONNECTING));
    instance.pimpl->dataTransferPortObserver_.OnConnectionStateChanged(
        RawAddress(DEVICE_SLE_ADDR), static_cast<int>(SleConnectState::DISCONNECTED),
        static_cast<int>(SleConnectState::CONNECTING));
    instance.DestroyPort(PORT_UUID, port);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_EQ(callback->events_.size(), static_cast<size_t>(1));
    EXPECT_EQ(callback->events_[0].state, static_cast<int32_t>(SleConnectState::DISCONNECTED));
}

/**
 * @tc.number: PortObserver_ConnectedPath001
 * @tc.name: Test port observer connected path creates channel.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, PortObserver_ConnectedPath001, TestSize.Level1)
{
    // 端口连接成功事件清掉请求缓存并对每个请求建通道；对端端口缺失时回退断开
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    MockPortClientSetDevicePortId(0);
    uint16_t port = instance.CreatePort(PORT_UUID);
    instance.ConnectPeerPortInner(MakeConnectParams(PORT_UUID, DEVICE_SLE_ADDR, port));

    instance.pimpl->dataTransferPortObserver_.OnConnectionStateChanged(
        RawAddress(DEVICE_SLE_ADDR), static_cast<int>(SleConnectState::CONNECTED),
        static_cast<int>(SleConnectState::CONNECTING));
    instance.DestroyPort(PORT_UUID, port);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_EQ(callback->events_.size(), static_cast<size_t>(1));
    EXPECT_EQ(callback->events_[0].state, static_cast<int32_t>(SleConnectState::DISCONNECTED));
    EXPECT_TRUE(instance.pimpl->appConnectInfo_.empty());
}

/**
 * @tc.number: NotifyDisconnect_GuardedByPendingRequest001
 * @tc.name: Test notify disconnect guarded by pending request.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, NotifyDisconnect_GuardedByPendingRequest001, TestSize.Level1)
{
    // 有挂起连接请求时不补发断链；请求清空且无在连记录时才真正下发断链
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t port = instance.CreatePort(PORT_UUID);
    instance.ConnectPeerPortInner(MakeConnectParams(PORT_UUID, DEVICE_SLE_ADDR, port));

    uint32_t disconnectCountBefore = MockPortClientGetDisconnectCount();
    instance.NotifyDisconnect(DEVICE_SLE_ADDR);
    EXPECT_EQ(MockPortClientGetDisconnectCount(), disconnectCountBefore);

    instance.pimpl->ClearConnectReqAndTimer(DEVICE_SLE_ADDR);
    SetCacheParam(instance, port, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::DISCONNECTED)));
    instance.NotifyDisconnect(DEVICE_SLE_ADDR);
    EXPECT_GT(MockPortClientGetDisconnectCount(), disconnectCountBefore);

    instance.DestroyPort(PORT_UUID, port);
}

/**
 * @tc.number: ClearCacheByTokenId001
 * @tc.name: Test clear cache by tokenId disconnects car key.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ClearCacheByTokenId001, TestSize.Level1)
{
    // 按tokenId清缓存时车钥匙地址触发断链动作并销毁端口；普通uuid不触发
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t carPort = instance.CreatePortInner(CAR_UUID, 888, 8, 8);
    SetCacheParam(instance, carPort, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED)));

    uint32_t actionCountBefore = MockSleAdapterGetDisconnectActionCount();
    instance.DisconnectCarWhenRemoteDie(ADDR_OTHER, PORT_UUID);
    EXPECT_EQ(MockSleAdapterGetDisconnectActionCount(), actionCountBefore);

    instance.ClearCacheByTokenId(888);
    EXPECT_EQ(instance.pimpl->FindUuidByPort(carPort), "");
    EXPECT_GT(MockSleAdapterGetDisconnectActionCount(), actionCountBefore);
}

/**
 * @tc.number: WriteData_Guards001
 * @tc.name: Test write data guards on cache and tcid.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, WriteData_Guards001, TestSize.Level1)
{
    // 无缓存、通道号缺失分别被参数错误拦截；空数据发送走栈失败路径
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t port = instance.CreatePort(PORT_UUID);
    SetCacheParam(instance, port, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED)));

    std::vector<uint8_t> datas = {1, 2, 3};
    DataTransferDataParams missing(DEVICE_SLE_ADDR, PORT_UUID, port + 1, datas);
    EXPECT_EQ(instance.WriteData(missing), static_cast<int>(NLSTK_ERRCODE_PARAM_ERR));

    DataTransferDataParams noTcid(DEVICE_SLE_ADDR, PORT_UUID, port, datas);
    EXPECT_EQ(instance.WriteData(noTcid), static_cast<int>(NLSTK_ERRCODE_PARAM_ERR));

    SetCacheParam(instance, port, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED), CAR_TCID));
    DataTransferDataParams emptyData(DEVICE_SLE_ADDR, PORT_UUID, port, std::vector<uint8_t> {});
    EXPECT_EQ(instance.WriteData(emptyData), static_cast<int>(TRANS_FAIL));

    instance.DestroyPort(PORT_UUID, port);
}

/**
 * @tc.number: ReceivedData_BusyAndFail001
 * @tc.name: Test received data short circuit on busy and fail state.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ReceivedData_BusyAndFail001, TestSize.Level1)
{
    // 传输忙时收数据直接返回真；传输失败时直接返回假，都不触碰输入流
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t port = instance.CreatePort(PORT_UUID);
    SetCacheParam(instance, port, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED), CAR_TCID, TransferState::TRANSFER_BUSY));
    EXPECT_TRUE(instance.ReceivedData(nullptr, port, DEVICE_SLE_ADDR));

    SetCacheParam(instance, port, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED), CAR_TCID, TransferState::TRANSFER_FAIL));
    EXPECT_FALSE(instance.ReceivedData(nullptr, port, DEVICE_SLE_ADDR));

    instance.DestroyPort(PORT_UUID, port);
}

/**
 * @tc.number: HandleConnectEvent_RepeatIgnored001
 * @tc.name: Test repeat connect event ignored and disconnect handled.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, HandleConnectEvent_RepeatIgnored001, TestSize.Level1)
{
    // 同状态的重复连接事件被忽略只回调一次；断开事件回调断开并回收对端socket
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    uint16_t carPort = instance.CreatePort(CAR_UUID);

    AppConnectParamMapping connected = MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED));
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, carPort, UINT16_MAX, connected);
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, carPort, UINT16_MAX, connected);

    AppConnectParamMapping disconnected = MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::DISCONNECTED));
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_RELEASED, carPort, UINT16_MAX, disconnected);
    instance.DestroyPort(CAR_UUID, carPort);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_EQ(callback->events_.size(), static_cast<size_t>(2));
    EXPECT_EQ(callback->events_[0].state, static_cast<int32_t>(SleConnectState::CONNECTED));
    EXPECT_EQ(callback->events_[1].state, static_cast<int32_t>(SleConnectState::DISCONNECTED));
}

/**
 * @tc.number: ChannelStatusCallback_StateMapping001
 * @tc.name: Test channel status maps to connection state.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, ChannelStatusCallback_StateMapping001, TestSize.Level1)
{
    // 通道释放映射为断开；未识别的通道状态映射为连接中
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    uint16_t port = instance.CreatePort(PORT_UUID);
    SetCacheParam(instance, port, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTING)));

    QOSM_TransChannelRspParams_S respParams = {};
    RawAddress addr(DEVICE_SLE_ADDR);
    addr.ConvertToUint8(respParams.addr.addr);
    respParams.tcid = CAR_TCID;
    respParams.srcPort = port;
    respParams.dstPort = 40000;
    respParams.mtu = UINT16_MAX;

    respParams.status = QOSM_TRANS_CHANNEL_RELEASED;
    instance.ChannelStatusCallback(&respParams);
    respParams.status = static_cast<QOSM_TransChannelStatus_E>(200);
    instance.ChannelStatusCallback(&respParams);
    instance.DestroyPort(PORT_UUID, port);
    instance.DeregisterSleDataTransferServiceCallback();

    ASSERT_EQ(callback->events_.size(), static_cast<size_t>(2));
    EXPECT_EQ(callback->events_[0].state, static_cast<int32_t>(SleConnectState::DISCONNECTED));
    EXPECT_EQ(callback->events_[1].state, static_cast<int32_t>(SleConnectState::CONNECTING));
}

/**
 * @tc.number: HandleReceiveDataEvent_CarPortDispatch001
 * @tc.name: Test receive data dispatch rules on car port.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, HandleReceiveDataEvent_CarPortDispatch001, TestSize.Level1)
{
    // 车钥匙默认端口的入栈数据分发成功；无车钥匙注册或端口不存在分别返回错误码
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    auto callback = std::make_shared<RecordingCallback>();
    instance.RegisterSleDataTransferServiceCallback(callback);
    uint16_t carPort = instance.CreatePort(CAR_UUID);
    SetCacheParam(instance, carPort, MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTING)));

    AppConnectParamMapping connected = MakeCacheParam(DEVICE_SLE_ADDR,
        static_cast<int32_t>(SleConnectState::CONNECTED));
    instance.HandleConnectEvent(QOSM_TRANS_CHANNEL_ESTABLISHED, carPort, UINT16_MAX, connected);

    std::vector<uint8_t> datas = {1, 2, 3};
    EXPECT_EQ(instance.HandleReceiveDataEvent(DEVICE_SLE_ADDR, CAR_PORT, datas),
        static_cast<int>(SLE_TRANS_SUCCESS));
    EXPECT_EQ(instance.HandleReceiveDataEvent(ADDR_OTHER, CAR_PORT, datas),
        static_cast<int>(TransferReturnValue::SLE_TRANS_INNER_FAIL));
    EXPECT_EQ(instance.HandleReceiveDataEvent(DEVICE_SLE_ADDR, 4242, datas),
        static_cast<int>(SLE_TRANS_RESULT_INTERNAL_FAULT));

    instance.DestroyPort(CAR_UUID, carPort);
    instance.DeregisterSleDataTransferServiceCallback();
}

/**
 * @tc.number: DataTransferEntryGuards001
 * @tc.name: Test entry guards of channel check and receive callback.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, DataTransferEntryGuards001, TestSize.Level1)
{
    // 已建端口的通道校验通过；未建端口拒绝；空地址与零长度的收包回调直接拦截
    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    uint16_t port = instance.CreatePort(PORT_UUID);
    EXPECT_TRUE(instance.CheckChannelParamCallback(port));
    EXPECT_FALSE(instance.CheckChannelParamCallback(port + 1));
    instance.DestroyPort(PORT_UUID, port);

    uint8_t data[] = {1, 2, 3};
    EXPECT_EQ(instance.ReceiveDataCallback(nullptr, data, sizeof(data)),
        static_cast<int>(TransferReturnValue::SLE_TRANS_INNER_FAIL));
    TRANS_Addr_S stackAddr = {};
    EXPECT_EQ(instance.ReceiveDataCallback(&stackAddr, data, 0),
        static_cast<int>(TransferReturnValue::SLE_TRANS_INNER_FAIL));
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
