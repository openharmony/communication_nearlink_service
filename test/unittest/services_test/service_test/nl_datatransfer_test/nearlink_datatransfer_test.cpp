/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "nearlink_access_token_mock.h"
#include "ClassCreator.h"
#include "log.h"
#include <thread>
#include "qosm_trans_channel.h"

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
    OHOS::Nearlink::NearlinkAccessTokenMock::SetNativeTokenInfo();
    HILOGI("SetUpTestCase end");
}

void NearlinkDataTransferTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
}

void NearlinkDataTransferTest::SetUp()
{
    HILOGI("SetUp start");
}

void NearlinkDataTransferTest::TearDown()
{
    HILOGI("TearDown start");
    if (g_fd > 0) {
        close(g_fd);
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
 * @tc.number: UpdateConnectInterval001
 * @tc.name: Test UpdateConnectInterval.
 * @tc.desc: FUNC
 */
HWTEST_F(NearlinkDataTransferTest, UpdateConnectInterval001, TestSize.Level1)
{
    HILOGI("UpdateConnectInterval001 start");

    SleDataTransferService &instance = SleDataTransferService::GetInstance();
    std::string device = DEVICE_SLE_ADDR;
    int32_t intervalType = INTERVAL_TYPE;
#ifdef WATCH_STANDARD
    bool res = instance.UpdateConnectInterval(device, intervalType);
    EXPECT_EQ(res, true);
#endif
    HILOGI("UpdateConnectInterval001 end");
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

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS