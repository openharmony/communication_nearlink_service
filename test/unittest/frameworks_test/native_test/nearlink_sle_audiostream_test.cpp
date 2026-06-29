/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "nearlink_ASC_source.h"
#include "nearlink_asc_server.h"
#include "log.h"
#include "parameters.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class SleAudioStreamTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SleAudioStreamTest::SetUpTestCase()
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

void SleAudioStreamTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
}

void SleAudioStreamTest::SetUp()
{
    HILOGI("SetUp start");
}

void SleAudioStreamTest::TearDown()
{
    HILOGI("TearDown start");
}

class SleAudioStreamObserverTest final: public OHOS::Nearlink::SleAudioStreamObserver {
public:
    SleAudioStreamObserverTest()
    {}
    ~SleAudioStreamObserverTest() = default;

    void OnAddSleAudioDevice(const NearlinkRemoteDevice &device, uint32_t streamType, int32_t mediaVolume,
        int32_t callVolume)
    {
        HILOGI("OnAddSleAudioDevice %{public}d %{public}d %{public}d", streamType, mediaVolume, callVolume);
    }

    void OnDeleteSleAudioDevice(const NearlinkRemoteDevice &device)
    {
        HILOGI("OnDeleteSleAudioDevice");
    }

    void OnAddSleVirtualAudioDevice(const NearlinkRemoteDevice &device, uint32_t streamType)
    {
        HILOGI("OnAddSleVirtualAudioDevice %{public}d", streamType);
    }

    void OnDeleteSleVirtualAudioDevice(const NearlinkRemoteDevice &device)
    {
        HILOGI("OnDeleteSleVirtualAudioDevice");
    }

    void OnStartPlayingResult(const NearlinkRemoteDevice &device, AudioStreamType streamType, int result)
    {
        HILOGI("OnStartPlayingResult %{public}d %{public}d", streamType, result);
    }

    void OnStopPlayingResult(const NearlinkRemoteDevice &device, AudioStreamType streamType, int result)
    {
        HILOGI("OnStopPlayingResult %{public}d %{public}d", streamType, result);
    }

    void OnSleAudioDeviceActionChanged(const NearlinkRemoteDevice &device,
        std::vector<struct AudioStreamInfo> &streamInfo, int action)
    {
        HILOGI("OnSleAudioDeviceActionChanged %{public}d", action);
    }
};

/**
 * @tc.number: CreateSleAudioStream001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioStreamTest, CreateSleAudioStream001, TestSize.Level1)
{
    HILOGI("CreateSleAudioStream001 start");
    std::shared_ptr<SleAudioStreamObserverTest> observer = std::make_shared<SleAudioStreamObserverTest>();
    std::shared_ptr<SleAudioStream> ptr = SleAudioStream::CreateSleAudioStream(observer);
    EXPECT_NE(nullptr, ptr);
    HILOGI("CreateSleAudioStream001 end");
}

/**
 * @tc.number: GetSleAudioDeviceList001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioStreamTest, GetSleAudioDeviceList001, TestSize.Level1)
{
    HILOGI("GetSleAudioDeviceList001 start");
    std::shared_ptr<SleAudioStreamObserverTest> observer = std::make_shared<SleAudioStreamObserverTest>();
    std::shared_ptr<SleAudioStream> ptr = SleAudioStream::CreateSleAudioStream(observer);
    EXPECT_NE(nullptr, ptr);

    std::vector<NearlinkRemoteDevice> devices;
    ptr->GetSleAudioDeviceList(devices);
    HILOGI("GetSleAudioDeviceList001 end");
}

/**
 * @tc.number: GetSleAudioDeviceCodecInfo001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioStreamTest, GetSleAudioDeviceCodecInfo001, TestSize.Level1)
{
    HILOGI("GetSleAudioDeviceCodecInfo001 start");
    std::shared_ptr<SleAudioStreamObserverTest> observer = std::make_shared<SleAudioStreamObserverTest>();
    std::shared_ptr<SleAudioStream> ptr = SleAudioStream::CreateSleAudioStream(observer);
    EXPECT_NE(nullptr, ptr);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    std::map<AudioStreamType, AudioStreamCodecInfo> mapCodec;
    mapCodec = ptr->GetSleAudioDeviceCodecInfo(*device);
    HILOGI("GetSleAudioDeviceCodecInfo001 end");
}

/**
 * @tc.number: GetSleVirtualAudioDeviceList001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioStreamTest, GetSleVirtualAudioDeviceList001, TestSize.Level1)
{
    HILOGI("GetSleVirtualAudioDeviceList001 start");
    std::shared_ptr<SleAudioStreamObserverTest> observer = std::make_shared<SleAudioStreamObserverTest>();
    std::shared_ptr<SleAudioStream> ptr = SleAudioStream::CreateSleAudioStream(observer);
    EXPECT_NE(nullptr, ptr);

    std::vector<NearlinkRemoteDevice> devices;
    ptr->GetSleVirtualAudioDeviceList(devices);
    HILOGI("GetSleVirtualAudioDeviceList001 end");
}

/**
 * @tc.number: GetSupportStreamType001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioStreamTest, GetSupportStreamType001, TestSize.Level1)
{
    HILOGI("GetSupportStreamType001 start");
    std::shared_ptr<SleAudioStreamObserverTest> observer = std::make_shared<SleAudioStreamObserverTest>();
    std::shared_ptr<SleAudioStream> ptr = SleAudioStream::CreateSleAudioStream(observer);
    EXPECT_NE(nullptr, ptr);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    uint32_t type = ptr->GetSupportStreamType(*device);
    EXPECT_EQ(0, type);
    HILOGI("GetSupportStreamType001 end");
}

/**
 * @tc.number: SetActiveSinkDevice001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioStreamTest, SetActiveSinkDevice001, TestSize.Level1)
{
    HILOGI("SetActiveSinkDevice001 start");
    std::shared_ptr<SleAudioStreamObserverTest> observer = std::make_shared<SleAudioStreamObserverTest>();
    std::shared_ptr<SleAudioStream> ptr = SleAudioStream::CreateSleAudioStream(observer);
    EXPECT_NE(nullptr, ptr);
    bool isAudioSupported = OHOS::system::GetIntParameter("const.nearlink.audio", 0) != 0;
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    uint64_t supportStreamType = 2;
    NlErrCode ret = ptr->SetActiveSinkDevice(*device, supportStreamType);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }
    HILOGI("SetActiveSinkDevice001 end");
}

/**
 * @tc.number: StartPlaying001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioStreamTest, StartPlaying001, TestSize.Level1)
{
    HILOGI("StartPlaying001 start");
    std::shared_ptr<SleAudioStreamObserverTest> observer = std::make_shared<SleAudioStreamObserverTest>();
    std::shared_ptr<SleAudioStream> ptr = SleAudioStream::CreateSleAudioStream(observer);
    EXPECT_NE(nullptr, ptr);
    bool isAudioSupported = OHOS::system::GetIntParameter("const.nearlink.audio", 0) != 0;
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    AudioStreamType streamType = AUDIO_STREAM_MUSIC;
    NlErrCode ret = ptr->StartPlaying(*device, streamType);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }
    HILOGI("StartPlaying001 end");
}

/**
 * @tc.number: StopPlaying001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioStreamTest, StopPlaying001, TestSize.Level1)
{
    HILOGI("StopPlaying001 start");
    std::shared_ptr<SleAudioStreamObserverTest> observer = std::make_shared<SleAudioStreamObserverTest>();
    std::shared_ptr<SleAudioStream> ptr = SleAudioStream::CreateSleAudioStream(observer);
    EXPECT_NE(nullptr, ptr);
    bool isAudioSupported = OHOS::system::GetIntParameter("const.nearlink.audio", 0) != 0;
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    AudioStreamType streamType = AUDIO_STREAM_MUSIC;
    NlErrCode ret = ptr->StopPlaying(*device, streamType);
    if (isAudioSupported) {
        EXPECT_EQ(NL_NO_ERROR, ret);
    } else {
        EXPECT_NE(NL_NO_ERROR, ret);
    }
    HILOGI("StopPlaying001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
