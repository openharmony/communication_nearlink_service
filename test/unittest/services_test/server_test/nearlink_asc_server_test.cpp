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
#include "nearlink_asc_stub.h"
#include "nearlink_asc_server.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "log.h"
 
namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
 
namespace {
sptr<NearlinkASCServer> g_sleAscServer = new (std::nothrow) NearlinkASCServer();
}
 
class NearlinkASCStubTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};
 
void NearlinkASCStubTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkASCStubTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    HILOGI("SetUpTestCase end NearlinkASCStubTest");
}
 
void NearlinkASCStubTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkASCStubTest");
}
 
void NearlinkASCStubTest::SetUp()
{
    HILOGI("SetUp NearlinkASCStubTest.");
}
 
void NearlinkASCStubTest::TearDown()
{
    HILOGI("TearDown NearlinkASCStubTest.");
}
 
int32_t AscServerOnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    NL_CHECK_RETURN_RET(g_sleAscServer, TRANSACTION_ERR, "g_sleAscServer is nullptr");
    MessageOption option = {MessageOption::TF_SYNC};
    HILOGI("NearlinkASCStubTest, cmd(%{public}d)", code);
    return g_sleAscServer->OnRemoteRequest(code, data, reply, option);
}
 
/**
 * @tc.number: AudioControl001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkASCStubTest, AudioControl001, TestSize.Level1)
{
    HILOGI("NearlinkASCStubTest:AudioControl001 start");
    std::string addr = "00:11:22:33:44:55";
    NearlinkRawAddress device = NearlinkRawAddress(addr);
 
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());
    data.WriteParcelable(&device);
    AudioStreamType streamType = AUDIO_STREAM_MUSIC;
    int32_t cmd = 1;
    data.WriteUint32(streamType);
    data.WriteInt32(cmd);
    int32_t ret = AscServerOnRemoteRequest(NearlinkASCInterfaceCode::NL_ASC_CONTROL,
        data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkASCStubTest:AudioControl001 end");
}

/**
 * @tc.number: GetAudioDeviceList001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkASCStubTest, GetAudioDeviceList001, TestSize.Level1)
{
    HILOGI("NearlinkASCStubTest:GetAudioDeviceList001 start");
    std::string addr = "00:11:22:33:44:55";
    NearlinkRawAddress device = NearlinkRawAddress(addr);
 
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());

    int32_t ret = AscServerOnRemoteRequest(NearlinkASCInterfaceCode::NL_ASC_GET_AUDIO_DEVICE_LIST,
        data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkASCStubTest:GetAudioDeviceList001 end");
}

/**
 * @tc.number: GetVirtualAudioDeviceList001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkASCStubTest, GetVirtualAudioDeviceList001, TestSize.Level1)
{
    HILOGI("NearlinkASCStubTest:GetVirtualAudioDeviceList001 start");
    std::string addr = "00:11:22:33:44:55";
    NearlinkRawAddress device = NearlinkRawAddress(addr);
 
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());

    int32_t ret = AscServerOnRemoteRequest(NearlinkASCInterfaceCode::NL_ASC_GET_VIRTUAL_AUDIO_DEVICE_LIST,
        data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkASCStubTest:GetVirtualAudioDeviceList001 end");
}
 
/**
 * @tc.number: GetSupportStreamType001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkASCStubTest, GetSupportStreamType001, TestSize.Level1)
{
    HILOGI("NearlinkASCStubTest:GetSupportStreamType001 start");
    std::string addr = "00:11:22:33:44:55";
    NearlinkRawAddress device = NearlinkRawAddress(addr);
 
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());
    data.WriteParcelable(&device);

    int32_t ret = AscServerOnRemoteRequest(NearlinkASCInterfaceCode::NL_ASC_GET_SUPPORT_STREAM_TYPE,
        data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_NE(NL_NO_ERROR, result);
    HILOGI("NearlinkASCStubTest:GetSupportStreamType001 end");
}

/**
 * @tc.number: GetAudioDeviceCodecInfo001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkASCStubTest, GetAudioDeviceCodecInfo001, TestSize.Level1)
{
    HILOGI("NearlinkASCStubTest:GetAudioDeviceCodecInfo001 start");
    std::string addr = "00:11:22:33:44:55";
    NearlinkRawAddress device = NearlinkRawAddress(addr);
 
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());
    data.WriteParcelable(&device);

    int32_t ret = AscServerOnRemoteRequest(NearlinkASCInterfaceCode::NL_ASC_GET_AUDIO_DEVICE_CODEC_INFO,
        data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkASCStubTest:GetAudioDeviceCodecInfo001 end");
}

/**
 * @tc.number: SetActiveSinkDevice001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkASCStubTest, SetActiveSinkDevice001, TestSize.Level1)
{
    HILOGI("NearlinkASCStubTest:SetActiveSinkDevice001 start");
    std::string addr = "00:11:22:33:44:55";
    NearlinkRawAddress device = NearlinkRawAddress(addr);
 
    MessageParcel data;
    MessageParcel reply;
    data.WriteInterfaceToken(NearlinkASCStub::GetDescriptor());
    data.WriteParcelable(&device);
    AudioStreamType streamType = AUDIO_STREAM_MUSIC;
    data.WriteUint32(streamType);

    int32_t ret = AscServerOnRemoteRequest(NearlinkASCInterfaceCode::NL_ASC_SET_ACTIVE_SINK_DEVICE,
        data, reply);
    EXPECT_EQ(NO_ERROR, ret);
    NlErrCode result = static_cast<NlErrCode>(reply.ReadInt32());
    EXPECT_EQ(NL_NO_ERROR, result);
    HILOGI("NearlinkASCStubTest:SetActiveSinkDevice001 end");
}
 
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS