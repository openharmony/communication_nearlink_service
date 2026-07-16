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
#include <thread>
#include "nearlink_sle_datatransfer_stub.h"
#include "nearlink_sle_datatransfer_server.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class MockNearlinkSleDataTransferCallbackStub : public IRemoteStub<INearlinkSleDataTransferCallback> {
public:
    void OnConnectionStateChanged(const NearlinkSleDataTransferConnectionParams &connectionParams, int fd) {}
};

namespace {
constexpr uint16_t CAR_PORT = 40960;
sptr<NearlinkSleDataTransferServer> g_dataTransfer = new (std::nothrow) NearlinkSleDataTransferServer();
sptr<INearlinkSleDataTransferCallback> g_sleDataTransferCb =
    new (std::nothrow) MockNearlinkSleDataTransferCallbackStub();
int g_fd = -1;
uint16_t testPort = -1;
}


class NearlinkSleDataTransferStubTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkSleDataTransferStubTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkSleDataTransferStubTest");
}

void NearlinkSleDataTransferStubTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkSleDataTransferStubTest");
}

void NearlinkSleDataTransferStubTest::SetUp()
{
    HILOGI("SetUp NearlinkSleDataTransferStubTest.");
}

void NearlinkSleDataTransferStubTest::TearDown()
{
    HILOGI("TearDown NearlinkSleDataTransferStubTest.");
}

int32_t DataTransferOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_dataTransfer, TRANSACTION_ERR, "g_ssapClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("NearlinkSleDataTransferStubTest, cmd(%{public}d)", code);
    return g_dataTransfer->OnRemoteRequest(code, data, reply, option);
}

/**
 * @tc.name: RegisterSleDataTransferCallback
 * @tc.desc: Test the RegisterSleDataTransferCallback function of NearlinkDataTransferProxy.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSleDataTransferStubTest, RegisterSleDataTransferCallback001, TestSize.Level1)
{
    HILOGI("NearlinkSleDataTransferStubTest:RegisterSleDataTransferCallback001 start");
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    if (g_sleDataTransferCb != nullptr) {
        data.WriteRemoteObject(g_sleDataTransferCb->AsObject());
    }
    int32_t ret = DataTransferOnRemoteRequest(
        NearlinkSleDataTransferInterfaceCode::SLE_REGISTER_SLE_DATATRANSFER_CALLBACK, data, reply);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("NearlinkSleDataTransferStubTest:RegisterSleDataTransferCallback001 end");
}

/**
 * @tc.name: CreatePort
 * @tc.desc: Test the CreatePort function of NearlinkDataTransferProxy.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSleDataTransferStubTest, CreatePort001, TestSize.Level1)
{
    HILOGI("NearlinkSleDataTransferStubTest:CreatePort001 start");
    std::string uuid = "060D";
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    data.WriteString(uuid);
    int32_t ret = DataTransferOnRemoteRequest(NearlinkSleDataTransferInterfaceCode::SLE_CREATE_PORT, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    testPort = reply.ReadUint16();
    EXPECT_GT(testPort, 0);
    HILOGI("NearlinkSleDataTransferStubTest:CreatePort001 end");
}

/**
 * @tc.name: SocketEmptyMsg
 * @tc.desc: Test the SocketEmptyMsg function of NearlinkDataTransferProxy.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSleDataTransferStubTest, SocketEmptyMsg001, TestSize.Level1)
{
    HILOGI("NearlinkSleDataTransferStubTest:SocketEmptyMsg001 start");
    uint16_t portId = CAR_PORT;
    std::string address = "00:00:00:00:00:00";
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    data.WriteUint16(portId);
    data.WriteString(address);
    int32_t ret = DataTransferOnRemoteRequest(NearlinkSleDataTransferInterfaceCode::SLE_SOCKET_EMPTY_PORT, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSleDataTransferStubTest:SocketEmptyMsg001 end");
}

/**
 * @tc.name: Connect
 * @tc.desc: Test the Connect function of NearlinkDataTransferProxy.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSleDataTransferStubTest, Connect001, TestSize.Level1)
{
    HILOGI("NearlinkSleDataTransferStubTest:Connect001 start");
    std::string address = "00:00:00:00:00:00";
    std::string uuid = "060D";
    MessageParcel data;
    MessageParcel reply;
    NearlinkSleDataTransferConnectionParams param;

    param.SetAddress(address);
    param.SetUuid(uuid);
    param.SetPort(testPort);
    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    data.WriteParcelable(&param);
    int32_t ret = DataTransferOnRemoteRequest(NearlinkSleDataTransferInterfaceCode::SLE_CONNECT, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSleDataTransferStubTest:Connect001 end");
}

/**
 * @tc.name: Disconnect
 * @tc.desc: Test the Disconnect function of NearlinkDataTransferProxy.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSleDataTransferStubTest, Disconnect001, TestSize.Level1)
{
    HILOGI("NearlinkSleDataTransferStubTest:Disconnect001 start");
    std::string address = "00:00:00:00:00:00";
    std::string uuid = "060D";
    MessageParcel data;
    MessageParcel reply;
    NearlinkSleDataTransferConnectionParams param;

    param.SetAddress(address);
    param.SetUuid(uuid);
    param.SetPort(testPort);
    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    data.WriteParcelable(&param);
    int32_t ret = DataTransferOnRemoteRequest(NearlinkSleDataTransferInterfaceCode::SLE_DISCONNECT, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSleDataTransferStubTest:Disconnect001 end");
}

/**
 * @tc.name: GetConnectionState
 * @tc.desc: Test the GetConnectionState function of NearlinkDataTransferProxy.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSleDataTransferStubTest, GetConnectionState001, TestSize.Level1)
{
    HILOGI("NearlinkSleDataTransferStubTest:GetConnectionState001 start");
    std::string address = "00:00:00:00:00:00";
    std::string uuid = "060D";
    uint16_t port = 40960;
    MessageParcel data;
    MessageParcel reply;
    NearlinkSleDataTransferConnectionParams param;

    param.SetAddress(address);
    param.SetUuid(uuid);
    param.SetPort(port);
    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    data.WriteParcelable(&param);
    int32_t ret = DataTransferOnRemoteRequest(
        NearlinkSleDataTransferInterfaceCode::SLE_GET_CONNECTION_STATE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkSleDataTransferStubTest:GetConnectionState001 end");
}

/**
 * @tc.name: DestroyPort
 * @tc.desc: Test the DestroyPort function of NearlinkDataTransferProxy.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSleDataTransferStubTest, destroyPort001, TestSize.Level1)
{
    HILOGI("NearlinkSleDataTransferStubTest:destroyPort001 start");
    std::string uuid = "060D";
    uint16_t port = 40960;
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    data.WriteString(uuid);
    data.WriteUint16(port);
    int32_t ret = DataTransferOnRemoteRequest(NearlinkSleDataTransferInterfaceCode::SLE_DESTROY_PORT, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkSleDataTransferStubTest:destroyPort001 end");
}

/**
 * @tc.name: DeregisterSleDataTransferCallback
 * @tc.desc: Test the DeregisterSleDataTransferCallback function of NearlinkDataTransferProxy.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSleDataTransferStubTest, DeregisterSleDataTransferCallback001, TestSize.Level1)
{
    HILOGI("NearlinkSleDataTransferStubTest:DeregisterSleDataTransferCallback001 start");
    MessageParcel data;
    MessageParcel reply;

    data.WriteInterfaceToken(NearlinkSleDataTransferStub::GetDescriptor());
    if (g_sleDataTransferCb != nullptr) {
        data.WriteRemoteObject(g_sleDataTransferCb->AsObject());
    }
    int32_t ret = DataTransferOnRemoteRequest(
        NearlinkSleDataTransferInterfaceCode::SLE_DE_REGISTER_SLE_DATATRANSFER_CALLBACK, data, reply);
    EXPECT_EQ(NL_NO_ERROR, ret);
    HILOGI("NearlinkSleDataTransferStubTest:DeregisterSleDataTransferCallback001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS