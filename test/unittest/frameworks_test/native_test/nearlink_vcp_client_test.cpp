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
#include "nearlink_vcp_client.h"
#include "log.h"
#include "parameters.h"

namespace OHOS::Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Nearlink;

class SleVcpClientTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SleVcpClientTest::SetUpTestCase()
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

void SleVcpClientTest::TearDownTestCase()
{}

void SleVcpClientTest::SetUp()
{}

void SleVcpClientTest::TearDown()
{}

/**
 * @tc.number: GetProfile001
 * @tc.name: VolumeControllerClient instance.
 * @tc.desc: VolumeControllerClient instance success.
 */
HWTEST_F(SleVcpClientTest, GetProfile001, TestSize.Level1)
{
    HILOGI("GetProfile001 start");
    auto instance = VolumeControllerClient::GetProfile();
    EXPECT_NE(nullptr, instance);
    instance = nullptr;
    HILOGI("GetProfile001 end");
}

/**
 * @tc.number: SetDeviceAbsoluteVolume001
 * @tc.name: VolumeControllerClient instance.
 * @tc.desc: VolumeControllerClient instance success.
 */
HWTEST_F(SleVcpClientTest, SetDeviceAbsoluteVolume001, TestSize.Level1)
{
    HILOGI("SetDeviceAbsoluteVolume001 start");
    bool isAudioSupported = OHOS::system::GetIntParameter("const.nearlink.audio", 0) != 0;
    auto instance = VolumeControllerClient::GetProfile();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    int32_t volume = 0;
    uint8_t volumeType = static_cast<uint8_t>(VolumeStreamType::SLE_STREAM_MEDIA);
    int32_t ret = instance->SetDeviceAbsoluteVolume(*device, volume, volumeType);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }
    HILOGI("SetDeviceAbsoluteVolume001 end");
}

/**
 * @tc.number: GetDeviceMediaVolume001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleVcpClientTest, GetDeviceMediaVolume001, TestSize.Level1)
{
    HILOGI("GetDeviceMediaVolume001 start");
    bool isAudioSupported = OHOS::system::GetIntParameter("const.nearlink.audio", 0) != 0;
    auto instance = VolumeControllerClient::GetProfile();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    int32_t mediaVolume = 0;
    int32_t ret = instance->GetDeviceMediaVolume(*device, mediaVolume);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }
    HILOGI("GetDeviceMediaVolume001 end");
}

/**
 * @tc.number: GetDeviceCallVolume001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleVcpClientTest, GetDeviceCallVolume001, TestSize.Level1)
{
    HILOGI("GetDeviceCallVolume001 start");
    bool isAudioSupported = OHOS::system::GetIntParameter("const.nearlink.audio", 0) != 0;
    auto instance = VolumeControllerClient::GetProfile();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    int32_t callVolume = 0;
    int32_t ret = instance->GetDeviceCallVolume(*device, callVolume);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }
    HILOGI("GetDeviceCallVolume001 end");
}

}  // namespace TEST
}  // namespace OHOS::Nearlink
