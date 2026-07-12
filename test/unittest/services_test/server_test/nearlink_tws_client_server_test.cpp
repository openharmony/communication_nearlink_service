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
#include "nearlink_tws_client_server.h"
#include "nearlink_asc_audio_stream_info.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "log.h"


namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
constexpr int HOST_FUZZ_DELAY_50_MS = 50;
constexpr int HOST_SERVER_TDD_DELAY_1000_MS = 1000;
sptr<NearlinkTwsClientServer> g_twsServer = new (std::nothrow) NearlinkTwsClientServer();
}

class NearlinkTwsClientServerStubTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkTwsClientServerStubTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkTwsClientServerStubTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkTwsClientServerStubTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkTwsClientServerStubTest");
}

void NearlinkTwsClientServerStubTest::SetUp()
{
    HILOGI("SetUp NearlinkTwsClientServerStubTest.");
}

void NearlinkTwsClientServerStubTest::TearDown()
{
    HILOGI("TearDown NearlinkTwsClientServerStubTest.");
}

class TwsClientObserverStubImplTest : public IRemoteStub<INearlinkTwsClientObserver> {
public:
    explicit TwsClientObserverStubImplTest()
    {
        HILOGI("TwsClientObserverStubImplTest ");
    }

    ~TwsClientObserverStubImplTest()
    {
        HILOGI("~TwsClientObserverStubImplTest ");
    }

    void OnTwsRemoteInfo(const std::string &address, const std::vector<uint8_t> &value) override
    {
        HILOGI("OnTwsRemoteInfo");
    }

private:
};

int32_t TwsClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_twsServer, TRANSACTION_ERR, "g_twsServer is nullptr");
    MessageOption option = { MessageOption::TF_SYNC };
    HILOGI("g_twsServer OnRemoteRequest, cmd(%{public}d)", code);
    return g_twsServer->OnRemoteRequest(code, data, reply, option);
}

/**
 * @tc.name: RegisterApplication
 * @tc.desc: Test the RegisterApplication function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, RegisterApplication001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:RegisterApplication001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    sptr<TwsClientObserverStubImplTest> clientCallback = new (std::nothrow) TwsClientObserverStubImplTest();
    data.WriteRemoteObject(clientCallback->AsObject());
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_TWS_REGISTER_APP, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:RegisterApplication001 end");
}

/**
 * @tc.name: RegisterApplication
 * @tc.desc: Test the RegisterApplication function, observer == nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, RegisterApplication002, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:RegisterApplication002 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_TWS_REGISTER_APP, data, reply);
    EXPECT_NE(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:RegisterApplication002 end");
}

/**
 * @tc.name: DeregisterApplication
 * @tc.desc: Test the DeregisterApplication function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, DeregisterApplication001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:DeregisterApplication001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    sptr<TwsClientObserverStubImplTest> clientCallback = new (std::nothrow) TwsClientObserverStubImplTest();
    data.WriteRemoteObject(clientCallback->AsObject());
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_TWS_DEREGISTER_APP, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:DeregisterApplication001 end");
}

/**
 * @tc.name: DeregisterApplication
 * @tc.desc: Test the DeregisterApplication function, observer == nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, DeregisterApplication002, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:DeregisterApplication002 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_TWS_DEREGISTER_APP, data, reply);
    EXPECT_NE(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:DeregisterApplication002 end");
}


/**
 * @tc.name: EnableWearDetection
 * @tc.desc: Test the EnableWearDetection function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, EnableWearDetection001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:EnableWearDetection001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string address = "11:22:33:44:55:66";
    data.WriteString(address);
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_ENABLE_WEAR_DETECTION, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:EnableWearDetection001 end");
}

/**
 * @tc.name: DisableWearDetection
 * @tc.desc: Test the DisableWearDetection function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, DisableWearDetection001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:DisableWearDetection001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string address = "11:22:33:44:55:66";
    data.WriteString(address);
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_DISABLE_WEAR_DETECTION, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:DisableWearDetection001 end");
}

/**
 * @tc.name: GetWearDetectionState
 * @tc.desc: Test the GetWearDetectionState function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, GetWearDetectionState001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:GetWearDetectionState001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string address = "11:22:33:44:55:66";
    data.WriteString(address);
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_GET_WEAR_DETECTION_STATE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:GetWearDetectionState001 end");
}

/**
 * @tc.name: IsDeviceWearing
 * @tc.desc: Test the IsDeviceWearing function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, IsDeviceWearing001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:IsDeviceWearing001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string address = "11:22:33:44:55:66";
    data.WriteString(address);
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_IS_DEVICE_WEARING, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:IsDeviceWearing001 end");
}

/**
 * @tc.name: IsWearDetectionSupported
 * @tc.desc: Test the IsWearDetectionSupported function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, IsWearDetectionSupported001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:IsWearDetectionSupported001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string address = "11:22:33:44:55:66";
    data.WriteString(address);
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_IS_WEAR_DETECTION_SUPPORTED, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:IsWearDetectionSupported001 end");
}

/**
 * @tc.name: GetTwsRoleInfo
 * @tc.desc: Test the GetTwsRoleInfo function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, GetTwsRoleInfo001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:GetTwsRoleInfo001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string address = "11:22:33:44:55:66";
    data.WriteString(address);
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_GET_TWS_ROLE_INFO, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:GetTwsRoleInfo001 end");
}

/**
 * @tc.name: GetTwsAudioDelay
 * @tc.desc: Test the GetTwsAudioDelay function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, GetTwsAudioDelay001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:GetTwsAudioDelay001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string address = "11:22:33:44:55:66";
    data.WriteString(address);
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_GET_TWS_AUDIO_DELAY, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:GetTwsAudioDelay001 end");
}

/**
 * @tc.name: SendUserSelection
 * @tc.desc: Test the SendUserSelection function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, SendUserSelection001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:SendUserSelection001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string address = "11:22:33:44:55:66";
    data.WriteString(address);
    NearlinkASCAudioStreamInfo streamInfo {};
    data.WriteParcelable(&streamInfo);
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_SEND_USER_SELECTION, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:SendUserSelection001 end");
}

/**
 * @tc.name: QueryStreamState
 * @tc.desc: Test the QueryStreamState function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, QueryStreamState001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:QueryStreamState001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string address = "11:22:33:44:55:66";
    data.WriteString(address);
    int32_t ret = TwsClientOnRemoteRequest(NearlinkTwsClientInterfaceCode::NL_QUERY_STREAM_STATE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:QueryStreamState001 end");
}

/**
 * @tc.name: IsSupportVirtualAutoConnect
 * @tc.desc: Test the IsSupportVirtualAutoConnect function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, IsSupportVirtualAutoConnect001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:IsSupportVirtualAutoConnect001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string address = "11:22:33:44:55:66";
    data.WriteString(address);
    int32_t ret = TwsClientOnRemoteRequest(
        NearlinkTwsClientInterfaceCode::NL_IS_SUPPORT_VIRTUAL_AUTO_CONNECT, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:IsSupportVirtualAutoConnect001 end");
}

/**
 * @tc.name: SetVirtualAutoConnectType
 * @tc.desc: Test the SetVirtualAutoConnectType function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkTwsClientServerStubTest, SetVirtualAutoConnectType001, TestSize.Level1)
{
    HILOGI("NearlinkTwsClientServerTest:SetVirtualAutoConnectType001 start");
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkTwsClientStub::GetDescriptor());
    std::string address = "11:22:33:44:55:66";
    data.WriteString(address);
    int32_t connType = 0;
    data.WriteInt32(connType);
    int32_t businessType = 0;
    data.WriteInt32(businessType);
    int32_t ret = TwsClientOnRemoteRequest(
        NearlinkTwsClientInterfaceCode::NL_SET_VIRTUAL_AUTO_CONNECT_TYPE, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    HILOGI("NearlinkTwsClientServerTest:SetVirtualAutoConnectType001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
