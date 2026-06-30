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

#include "nearlink_access_token_mock.h"
#include "nearlink_def.h"
#include "nearlink_host.h"
#include "nearlink_sle_datatransfer.cpp"
#include "log.h"
#include "sle_uuid.h"
#include "nearlink_socket_manager.h"
#include "nearlink_sle_datatransfer_connection_params.h"
#include "nearlink_datatransfer_parcel.h"

namespace OHOS::Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Nearlink;

namespace {
const std::string CAR_UUID = "060D";
const std::string PORT_UUID = "00090000-0001-0002-0000-000000000099";
}
class SleDataTransferTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SleDataTransferTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    if (!NearlinkHost::GetInstance().IsSleEnabled()) {
        HILOGI("start enable nearlink");
        NearlinkHost::GetInstance().EnableNl();
        sleep(2);
    }
    HILOGI("SetUpTestCase end");
}

void SleDataTransferTest::TearDownTestCase()
{}

void SleDataTransferTest::SetUp()
{}

void SleDataTransferTest::TearDown()
{}

class SleDataTransferCallbackTest final : public OHOS::Nearlink::SleDataTransferCallback {
public:
    SleDataTransferCallbackTest(){};
    ~SleDataTransferCallbackTest(){};

private:
    void OnConnectionStateChanged(const OHOS::Nearlink::ConnectionParams &result)
    {
        HILOGI("OnConnectionStateChanged");
    }
    void OnReceiveData(const OHOS::Nearlink::DataParams &result)
    {
        HILOGI("OnReceiveData");
    }
};

/**
 * @tc.number: CreateSleDataTransfer001
 * @tc.name: DataTransfer instance.
 * @tc.desc: DataTransfer instance success.
 */
HWTEST_F(SleDataTransferTest, CreateSleDataTransfer001, TestSize.Level1)
{
    HILOGI("CreateSleDataTransfer001 start");
    std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    EXPECT_NE(nullptr, instance);
    instance = nullptr;
    HILOGI("CreateSleDataTransfer001 end");
}

/**
 * @tc.number: CreateAndDestroyPort001
 * @tc.name: CreatePort DestroyPort success
 * @tc.desc: CreatePort DestroyPort success
 */
HWTEST_F(SleDataTransferTest, CreateAndDestroyPort001, TestSize.Level1)
{
    HILOGI("CreateAndDestroyPort001 start");
    std::string uuid = CAR_UUID;
    std::shared_ptr<SleDataTransferCallbackTest> callback = std::make_shared<SleDataTransferCallbackTest>();
    std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    NlErrCode ret = instance->CreatePort(uuid, callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);
    ret = instance->DestroyPort(uuid);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);
    HILOGI("CreateAndDestroyPort001 end");
}

/**
 * @tc.number: CreateAndDestroyPort002
 * @tc.name: CreatePort duplicate
 * @tc.desc: CreatePort duplicate
 */
HWTEST_F(SleDataTransferTest, CreateAndDestroyPort002, TestSize.Level1)
{
    HILOGI("CreateAndDestroyPort002 start");
    std::string uuid = PORT_UUID;
    std::shared_ptr<SleDataTransferCallbackTest> callback = std::make_shared<SleDataTransferCallbackTest>();
    std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    NlErrCode ret = instance->CreatePort(uuid, callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    EXPECT_EQ(NL_ERR_DATATRANSFER_DUPLICATE_REGISTER, instance->CreatePort(uuid, callback));
    sleep(1);
    ret = instance->DestroyPort(uuid);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);
    HILOGI("CreateAndDestroyPort002 end");
}

/**
 * @tc.number: Connect001
 * @tc.name: Connect
 * @tc.desc: Connect
 */
HWTEST_F(SleDataTransferTest, Connect001, TestSize.Level1)
{
    HILOGI("Connect001 start");
    std::string uuid = "060D";
    std::string address = "00:00:00:00:00:00";
    std::shared_ptr<SleDataTransferCallbackTest> callback = std::make_shared<SleDataTransferCallbackTest>();
    std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    ConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);
    param.SetTransMode(static_cast<uint8_t>(ConnectionParams::PortTransMode::TRANSPORT_MODE_BASIC));
    param.SetFrameType(static_cast<uint8_t>(SleConnFrameType::SLE_CONN_FRAME_TYPE_1));
    NlErrCode ret = instance->CreatePort(uuid, callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);
    ret = instance->Connect(param);
    EXPECT_EQ(NL_NO_ERROR, ret);

    ret = instance->DestroyPort(uuid);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);
    HILOGI("Connect001 end");
}


/**
 * @tc.number: Disconnect001
 * @tc.name: Connect
 * @tc.desc: Connect
 */
HWTEST_F(SleDataTransferTest, Disconnect001, TestSize.Level1)
{
    HILOGI("Disconnect001 start");
    std::string uuid = "060D";
    std::string address = "00:00:00:00:00:00";
    std::shared_ptr<SleDataTransferCallbackTest> callback = std::make_shared<SleDataTransferCallbackTest>();
    std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    NlErrCode ret = instance->CreatePort(uuid, callback);
    ConnectionParams param;
    param.SetAddress(address);
    param.SetUuid(uuid);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);

    ret = instance->Disconnect(param);
    EXPECT_EQ(NL_NO_ERROR, ret);

    ret = instance->DestroyPort(uuid);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);
    HILOGI("Disconnect001 end");
}

/**
 * @tc.name: GetConnectionState001
 * @tc.desc: GetConnectionState
 * @tc.type: GetConnectionState
 */
HWTEST_F(SleDataTransferTest, GetConnectionState001, TestSize.Level1)
{
    HILOGI("GetConnectionState001 start");
    std::string uuid = "060D";
    std::string address = "00:00:00:00:00:00";
    std::shared_ptr<SleDataTransferCallbackTest> callback = std::make_shared<SleDataTransferCallbackTest>();
    std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    ConnStateParams param(address, uuid);

    NlErrCode ret = instance->CreatePort(uuid, callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);
    int32_t connState = 0;
    ret = instance->GetConnectionState(param, connState);
    EXPECT_EQ(NL_NO_ERROR, ret);

    ret = instance->DestroyPort(uuid);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);
    HILOGI("GetConnectionState001 end");
}

/**
 * @tc.name: DataParams001
 * @tc.desc: DataParams
 * @tc.type: DataParams
 */
HWTEST_F(SleDataTransferTest, DataParams001, TestSize.Level1)
{
    HILOGI("DataParams001 start");
    std::string uuid = CAR_UUID;
    std::string address = "00:00:00:00:00:00";
    DataParams param1(address, uuid);
    size_t length = 6;
    std::array<uint8_t, 6> data = {'1', '2', '3', '4', '5', '6'};
    param1.SetData(data.data(), length);
    EXPECT_EQ(address, param1.GetAddress());

    std::string uuid2 = PORT_UUID;
    std::string address2 = "11:22:33:44:55:66";
    DataParams param2 = param1;
    EXPECT_EQ(uuid, param2.GetUuid());
    param2.SetAddress(address2);
    param2.SetUuid(uuid2);

    DataParams param3(param2);
    EXPECT_EQ(address2, param3.GetAddress());
    HILOGI("DataParams001 end");
}

/**
 * @tc.name: ConnectParams001
 * @tc.desc: ConnectParams
 * @tc.type: ConnectParams
 */
HWTEST_F(SleDataTransferTest, ConnectParams001, TestSize.Level1)
{
    HILOGI("ConnectParams001 start");
    std::string uuid = CAR_UUID;
    std::string address = "00:00:00:00:00:00";
    int32_t state = static_cast<int32_t>(SleConnectState::CONNECTED);
    uint16_t mtu = 1024;
    ConnectionParams param;
    param.SetAddress(address);
    EXPECT_EQ(address, param.GetAddress());
    param.SetUuid(uuid);
    EXPECT_EQ(uuid, param.GetUuid());
    param.SetState(state);
    EXPECT_EQ(state, param.GetState());
    param.SetMtu(mtu);
    EXPECT_EQ(mtu, param.GetMtu());

    HILOGI("ConnectParams001 end");
}

/**
 * @tc.name: ConnStateParams001
 * @tc.desc: ConnStateParams
 * @tc.type: ConnStateParams
 */
HWTEST_F(SleDataTransferTest, ConnStateParams001, TestSize.Level1)
{
    HILOGI("ConnStateParams001 start");
    std::string uuid = CAR_UUID;
    std::string address = "00:00:00:00:00:00";
    ConnStateParams param;
    param.SetAddress(address);
    EXPECT_EQ(address, param.GetAddress());
    param.SetUuid(uuid);
    EXPECT_EQ(uuid, param.GetUuid());

    HILOGI("ConnStateParams001 end");
}

/**
 * @tc.name: WriteData001
 * @tc.desc: WriteData
 * @tc.type: WriteData
 */
HWTEST_F(SleDataTransferTest, WriteData001, TestSize.Level1)
{
    HILOGI("WriteData001 start");
    std::string uuid = CAR_UUID;
    std::string address = "00:00:00:00:00:00";
    std::shared_ptr<SleDataTransferCallbackTest> callback = std::make_shared<SleDataTransferCallbackTest>();
    std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    DataParams param(address, uuid);
    size_t length = 6;
    std::array<uint8_t, 6> data = {'1', '2', '3', '4', '5', '6'};
    param.SetData(data.data(), length);

    NlErrCode ret = instance->CreatePort(uuid, callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);
    ret = instance->WriteData(param);
    EXPECT_EQ(NL_NO_ERROR, ret);

    ret = instance->DestroyPort(uuid);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);
    HILOGI("WriteData001 end");
}

/**
 * @tc.name: IsSupportHighSpeedDataTransfer001
 * @tc.desc: IsSupportHighSpeedDataTransfer
 * @tc.type: IsSupportHighSpeedDataTransfer
 */
HWTEST_F(SleDataTransferTest, IsSupportHighSpeedDataTransfer001, TestSize.Level1)
{
    HILOGI("IsSupportHighSpeedDataTransfer001 start");
    std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    bool enable = OHOS::system::GetBoolParameter("const.nearlink.enable.port", true);
    bool ret = instance->IsSupportHighSpeedDataTransfer();
    EXPECT_EQ(enable, ret);
    sleep(1);
    HILOGI("IsSupportHighSpeedDataTransfer001 end");
}

/**
 * @tc.name: UpdateConnectInterval001
 * @tc.desc: UpdateConnectInterval
 * @tc.type: UpdateConnectInterval
 */
HWTEST_F(SleDataTransferTest, UpdateConnectInterval001, TestSize.Level1)
{
    HILOGI("UpdateConnectInterval001 start");
    std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    std::string device = "00:00:00:00:00:00";
    int32_t intervalType = 0x24;
    bool ret = instance->UpdateConnectInterval(device, intervalType);
    EXPECT_EQ(false, ret);
    HILOGI("UpdateConnectInterval001 end");
}

/**
 * @tc.name: OnConnectionStateChanged001
 * @tc.desc: OnConnectionStateChanged
 * @tc.type: OnConnectionStateChanged
 */
HWTEST_F(SleDataTransferTest, OnConnectionStateChanged001, TestSize.Level1)
{
    HILOGI("WriteData001 start");
    std::string uuid = "060D";
    std::string address = "00:00:00:00:00:00";
    std::shared_ptr<SleDataTransferCallbackTest> callback = std::make_shared<SleDataTransferCallbackTest>();
    std::shared_ptr<SleDataTransfer> instance = SleDataTransfer::CreateSleDataTransfer();
    std::shared_ptr<PortSocketManager> manager = std::make_shared<PortSocketManager>();

    NlErrCode ret = instance->CreatePort(uuid, callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);

    auto serviceDataCallback = [](std::shared_ptr<InputStream> inputStream, uint16_t port,
        std::string addr) {
        HILOGI("service data callback success");
    };
    DataTransferConnectionParams param;
    uint16_t mtu = 65535;
    uint16_t portId = 40960;
    uint16_t dstPort = 1;
    int32_t state = static_cast<int32_t>(SleConnectState::CONNECTED);

    param.SetUuid(uuid);
    param.SetPort(portId);
    param.SetMtu(mtu);
    param.SetAddress(address);
    param.SetState(state);
    NearlinkSleDataTransferConnectionParams connectionParams(param);

    int fd = manager->CreateSocketPair(portId, address, mtu, serviceDataCallback);
    EXPECT_EQ(fd > 0, true);
    instance->pimpl->callbackImp_->OnConnectionStateChanged(connectionParams, fd);
    const std::vector<uint8_t> datas = {1, 2, 3, 4, 5, 6};
    DataTransferDataParams dataParams(address, uuid, dstPort, datas);
    // send data to framework by socket
    size_t totalLen = 0;
    std::unique_ptr<uint8_t[]> packageData = NearlinkDataTransferDataParams::SerializeData(dataParams, totalLen);
    EXPECT_EQ(packageData != nullptr, true);
    SocketTransState res = manager->SendData(dataParams.port_, dataParams.address_, packageData.get(), totalLen,
        datas.size());
    EXPECT_EQ(SLE_TRANS_RESULT_SUCCESS, res);
    sleep(1);
    connectionParams.SetState(static_cast<int32_t>(SleConnectState::DISCONNECTED));
    instance->pimpl->callbackImp_->OnConnectionStateChanged(connectionParams, -1);
    sleep(1);
    ret = instance->DestroyPort(uuid);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(1);
    HILOGI("WriteData001 end");
}


}  // namespace TEST
}  // namespace OHOS::Nearlink
