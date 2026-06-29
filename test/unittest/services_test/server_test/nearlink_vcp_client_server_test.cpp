/*
 * Copyright (C) 2025 Huawei Device Co., Ltd. All rights reserved.
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
#include "nearlink_vcp_client_stub.h"
#include "nearlink_vcp_client_server.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
sptr<NearlinkVcpClientServer> g_sleVcpClient = new (std::nothrow) NearlinkVcpClientServer();
}

class NearlinkVcpClientServerStubTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkVcpClientServerStubTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkVcpClientServerStubTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    HILOGI("SetUpTestCase end NearlinkVcpClientServerStubTest");
}

void NearlinkVcpClientServerStubTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkVcpClientServerStubTest");
}

void NearlinkVcpClientServerStubTest::SetUp()
{
    HILOGI("SetUp NearlinkVcpClientServerStubTest.");
}

void NearlinkVcpClientServerStubTest::TearDown()
{
    HILOGI("TearDown NearlinkVcpClientServerStubTest.");
}

int32_t VcpClientOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_sleVcpClient, TRANSACTION_ERR, "g_sleVcpClient is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("NearlinkVcpClientServerStubTest, cmd(%{public}d)", code);
    return g_sleVcpClient->OnRemoteRequest(code, data, reply, option);
}

/**
 * @tc.number: SetDeviceAbsoluteVolume001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVcpClientServerStubTest, SetDeviceAbsoluteVolume001, TestSize.Level1)
{
    HILOGI("NearlinkVcpClientServerStubTest:SetDeviceAbsoluteVolume001 start");
    std::string addr = "00:11:22:33:44:55";
    NearlinkRawAddress device = NearlinkRawAddress(addr);
    int32_t volume = 0;
    uint8_t volumeType = static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_MEDIA);

    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkVcpClientStub::GetDescriptor());
    data.WriteParcelable(&device);
    data.WriteInt32(volume);
    data.WriteUint8(volumeType);
    int32_t ret = VcpClientOnRemoteRequest(NearlinkVcpInterfaceCode::NL_VCP_CLIENT_SET_DEVICE_ABSOLUTE_VOLUME,
        data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkVcpClientServerStubTest:SetDeviceAbsoluteVolume001 end");
}

/**
 * @tc.number: GetDeviceMediaVolume001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVcpClientServerStubTest, GetDeviceMediaVolume001, TestSize.Level1)
{
    HILOGI("NearlinkVcpClientServerStubTest:GetDeviceMediaVolume001 start");
    std::string addr = "00:11:22:33:44:55";
    NearlinkRawAddress device = NearlinkRawAddress(addr);

    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkVcpClientStub::GetDescriptor());
    data.WriteParcelable(&device);
    int32_t ret = VcpClientOnRemoteRequest(NearlinkVcpInterfaceCode::NL_VCP_CLIENT_GET_MEDIA_VOLUME, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkVcpClientServerStubTest:GetDeviceMediaVolume001 end");
}

/**
 * @tc.number: GetDeviceCallVolume001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVcpClientServerStubTest, GetDeviceCallVolume001, TestSize.Level1)
{
    HILOGI("NearlinkVcpClientServerStubTest:GetDeviceCallVolume001 start");
    std::string addr = "00:11:22:33:44:55";
    NearlinkRawAddress device = NearlinkRawAddress(addr);

    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkVcpClientStub::GetDescriptor());
    data.WriteParcelable(&device);
    int32_t ret = VcpClientOnRemoteRequest(NearlinkVcpInterfaceCode::NL_VCP_CLIENT_GET_CALL_VOLUME, data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkVcpClientServerStubTest:GetDeviceCallVolume001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS