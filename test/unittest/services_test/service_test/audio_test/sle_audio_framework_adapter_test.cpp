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
#include <gmock/gmock.h>
#include <thread>
#include <vector>
#include <memory>

#include "nearlink_access_token_mock.h"
#include "SleAudioFrameworkAdapter.cpp"
#include "SleAudioFrameworkAdapter.h"
#include "McpServerService.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using ::testing::Return;
using ::testing::_;
using ::testing::Invoke;

namespace {
constexpr int TEST_DELAY_100_MS = 100;
constexpr int TEST_DELAY_1000_MS = 1000;
constexpr uint32_t DEFAULT_STREAM_ID = 1;
constexpr uint32_t DEFAULT_ROUTE_FLAG = 1;
constexpr int DEFAULT_STREAM_USAGE = 0;
constexpr int TEST_LIST_SIZE = 5;
constexpr int TEST_INDEX_0 = 0;
constexpr int TEST_INDEX_1 = 1;
constexpr int TEST_INDEX_2 = 2;
constexpr int LOOP_COUNT_TWO = 2;
constexpr int LOOP_COUNT_THREE = 3;
constexpr size_t LIST_SIZE_ONE = 1;
constexpr size_t LIST_SIZE_THREE = 3;
constexpr size_t BUNDLE_NAME_LENGTH_ZERO = 0;
constexpr uint32_t TIMER_STREAM_ID = 100;
constexpr uint32_t MAX_ROUTE_FLAG_LIST_SIZE_TEST = 256;
constexpr int TWO = 2;
constexpr int THREE = 3;
}

class SleAudioFrameworkAdapterTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SleAudioFrameworkAdapterTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase SleAudioFrameworkAdapterTest start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    HILOGI("SetUpTestCase SleAudioFrameworkAdapterTest end");
}

void SleAudioFrameworkAdapterTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase SleAudioFrameworkAdapterTest");
}

void SleAudioFrameworkAdapterTest::SetUp()
{
    HILOGI("SetUp SleAudioFrameworkAdapterTest.");
}

void SleAudioFrameworkAdapterTest::TearDown()
{
    HILOGI("TearDown SleAudioFrameworkAdapterTest.");
    SleAudioFrameworkAdapter::GetInstance().StopEmptyMuteScMcpPauseTimer(
        MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM);
    SleAudioFrameworkAdapter::GetInstance().StopEmptyMuteScMcpPauseTimer(
        MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO);
    SleAudioFrameworkAdapter::GetInstance().StopEmptyMuteScMcpPauseTimer(
        MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO);
    SleAudioFrameworkAdapter::GetInstance().ClearRouteFlagList();
}

RouteFlagInfo CreateRouteFlagInfo(uint32_t routeFlag, uint32_t streamId,
    uint32_t streamUsage, AudioStandard::RendererState status, const std::string& bundleName)
{
    return RouteFlagInfo {
        .routeFlag = routeFlag,
        .streamId = streamId,
        .streamUsage = streamUsage,
        .status = status,
        .bundleName = bundleName,
    };
}

AudioStandard::RendererStreamInfo CreateRendererStreamInfo(
    AudioStandard::RendererState state, AudioStandard::StreamUsage usage, uint32_t streamId,
    const std::string& bundleName)
{
    return AudioStandard::RendererStreamInfo {
        .state_ = state,
        .usage_ = usage,
        .streamId_ = streamId,
        .bundleName_ = bundleName,
    };
}

static inline RawAddress CreateTestDevice()
{
    return RawAddress("00:11:22:33:44:55");
}

/**
 * @tc.number: GetInstance001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, GetInstance001, TestSize.Level1)
{
    HILOGI("GetInstance001 start");
    SleAudioFrameworkAdapter& instance1 = SleAudioFrameworkAdapter::GetInstance();
    SleAudioFrameworkAdapter& instance2 = SleAudioFrameworkAdapter::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
    HILOGI("GetInstance001 end");
}

/**
 * @tc.number: RouteFlagListOperation001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, RouteFlagListOperation001, TestSize.Level1)
{
    HILOGI("RouteFlagListOperation001 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.ClearRouteFlagList();

    auto info = CreateRouteFlagInfo(DEFAULT_ROUTE_FLAG, DEFAULT_STREAM_ID,
        DEFAULT_STREAM_USAGE, AudioStandard::RendererState::RENDERER_STOPPED, "com.test.app");
    adapter.AddMemberRouteFlagList(info);

    const std::vector<RouteFlagInfo>& list = adapter.GetRouteFlagList();
    EXPECT_EQ(list.size(), LIST_SIZE_ONE);
    EXPECT_EQ(list[TEST_INDEX_0].routeFlag, DEFAULT_ROUTE_FLAG);
    EXPECT_EQ(list[TEST_INDEX_0].streamId, DEFAULT_STREAM_ID);

    adapter.RemoveMemberRouteFlagList(DEFAULT_ROUTE_FLAG);
    EXPECT_TRUE(list.empty());
    HILOGI("RouteFlagListOperation001 end");
}

/**
 * @tc.number: RouteFlagListClear001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, RouteFlagListClear001, TestSize.Level1)
{
    HILOGI("RouteFlagListClear001 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.ClearRouteFlagList();

    auto info = CreateRouteFlagInfo(DEFAULT_ROUTE_FLAG, DEFAULT_STREAM_ID,
        DEFAULT_STREAM_USAGE, AudioStandard::RendererState::RENDERER_STOPPED, "com.test.app");
    adapter.AddMemberRouteFlagList(info);
    adapter.ClearRouteFlagList();

    const std::vector<RouteFlagInfo>& list = adapter.GetRouteFlagList();
    EXPECT_TRUE(list.empty());
    HILOGI("RouteFlagListClear001 end");
}

/**
 * @tc.number: GetBundleName001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, GetBundleName001, TestSize.Level1)
{
    HILOGI("GetBundleName001 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.ClearRouteFlagList();

    auto info = CreateRouteFlagInfo(DEFAULT_ROUTE_FLAG, DEFAULT_STREAM_ID,
        DEFAULT_STREAM_USAGE, AudioStandard::RendererState::RENDERER_STOPPED, "com.test.bundle");
    adapter.AddMemberRouteFlagList(info);

    std::string bundleName = adapter.GetBundleName(DEFAULT_STREAM_ID);
    EXPECT_STREQ(bundleName.c_str(), "com.test.bundle");
    HILOGI("GetBundleName001 end");
}

/**
 * @tc.number: GetBundleName002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, GetBundleName002, TestSize.Level1)
{
    HILOGI("GetBundleName002 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.ClearRouteFlagList();

    std::string bundleName = adapter.GetBundleName(DEFAULT_STREAM_ID + 1);
    EXPECT_STREQ(bundleName.c_str(), "");
    HILOGI("GetBundleName002 end");
}

/**
 * @tc.number: IsCallStreamType001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, IsCallStreamType001, TestSize.Level1)
{
    HILOGI("IsCallStreamType001 start");
    EXPECT_TRUE(SleAudioFrameworkAdapter::GetInstance().isCallStreamType(
        OHOS::AudioStandard::STREAM_USAGE_VOICE_RINGTONE));
    EXPECT_TRUE(SleAudioFrameworkAdapter::GetInstance().isCallStreamType(
        OHOS::AudioStandard::STREAM_USAGE_VOICE_ASSISTANT));
    EXPECT_TRUE(SleAudioFrameworkAdapter::GetInstance().isCallStreamType(
        OHOS::AudioStandard::STREAM_USAGE_RINGTONE));
    EXPECT_TRUE(SleAudioFrameworkAdapter::GetInstance().isCallStreamType(
        OHOS::AudioStandard::STREAM_USAGE_VOICE_MODEM_COMMUNICATION));
    EXPECT_TRUE(SleAudioFrameworkAdapter::GetInstance().isCallStreamType(
        OHOS::AudioStandard::STREAM_USAGE_VOICE_CALL_ASSISTANT));
    EXPECT_TRUE(SleAudioFrameworkAdapter::GetInstance().isCallStreamType(
        OHOS::AudioStandard::STREAM_USAGE_VOICE_COMMUNICATION));
    EXPECT_TRUE(SleAudioFrameworkAdapter::GetInstance().isCallStreamType(
        OHOS::AudioStandard::STREAM_USAGE_VIDEO_COMMUNICATION));
    HILOGI("IsCallStreamType001 end");
}

/**
 * @tc.number: IsCallStreamType002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, IsCallStreamType002, TestSize.Level1)
{
    HILOGI("IsCallStreamType002 start");
    EXPECT_FALSE(SleAudioFrameworkAdapter::GetInstance().isCallStreamType(
        OHOS::AudioStandard::STREAM_USAGE_MUSIC));
    EXPECT_FALSE(SleAudioFrameworkAdapter::GetInstance().isCallStreamType(
        OHOS::AudioStandard::STREAM_USAGE_MOVIE));
    EXPECT_FALSE(SleAudioFrameworkAdapter::GetInstance().isCallStreamType(
        OHOS::AudioStandard::STREAM_USAGE_GAME));
    EXPECT_FALSE(SleAudioFrameworkAdapter::GetInstance().isCallStreamType(
        OHOS::AudioStandard::STREAM_USAGE_AUDIOBOOK));
    HILOGI("IsCallStreamType002 end");
}

/**
 * @tc.number: IsSlePipe001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, IsSlePipe001, TestSize.Level1)
{
    HILOGI("IsSlePipe001 start");
    EXPECT_TRUE(SleAudioFrameworkAdapter::GetInstance().IsSlePipe(
        OHOS::AudioStandard::HDI_ADAPTER_TYPE_SLE));
    EXPECT_TRUE(SleAudioFrameworkAdapter::GetInstance().IsSlePipe(
        OHOS::AudioStandard::HDI_ADAPTER_TYPE_PRIMARY));
    EXPECT_FALSE(SleAudioFrameworkAdapter::GetInstance().IsSlePipe(
        OHOS::AudioStandard::HDI_ADAPTER_TYPE_USB));
    EXPECT_FALSE(SleAudioFrameworkAdapter::GetInstance().IsSlePipe(
        OHOS::AudioStandard::HDI_ADAPTER_TYPE_A2DP));
    HILOGI("IsSlePipe001 end");
}

/**
 * @tc.number: TimerOperation001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, TimerOperation001, TestSize.Level1)
{
    HILOGI("TimerOperation001 start");
    auto timer = std::make_shared<NearlinkTimer>([]() {});
    SleAudioFrameworkAdapter::GetInstance().SetEmptyMuteCheckTimer(
        MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM, timer);

    auto retrievedTimer = SleAudioFrameworkAdapter::GetInstance().GetEmptyMuteCheckTimer(
        MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM);
    EXPECT_NE(retrievedTimer, nullptr);

    std::shared_ptr<NearlinkTimer> timer1 = SleAudioFrameworkAdapter::GetInstance().GetEmptyMuteCheckTimer(
        MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO);
    EXPECT_EQ(timer1, nullptr);

    std::shared_ptr<NearlinkTimer> timer2 = SleAudioFrameworkAdapter::GetInstance().GetEmptyMuteCheckTimer(
        MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO);
    EXPECT_EQ(timer2, nullptr);
    HILOGI("TimerOperation001 end");
}

/**
 * @tc.number: TimerOperation002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, TimerOperation002, TestSize.Level1)
{
    HILOGI("TimerOperation002 start");
    auto timer = std::make_shared<NearlinkTimer>([]() {});
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.SetEmptyMuteCheckTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM, timer);
    adapter.SetEmptyMuteCheckTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO, timer);
    adapter.SetEmptyMuteCheckTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO, timer);

    EXPECT_NE(adapter.GetEmptyMuteCheckTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM), nullptr);
    EXPECT_NE(adapter.GetEmptyMuteCheckTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO), nullptr);
    EXPECT_NE(adapter.GetEmptyMuteCheckTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO), nullptr);

    adapter.StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM);
    adapter.StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO);
    adapter.StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO);

    EXPECT_EQ(adapter.GetEmptyMuteCheckTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM), nullptr);
    EXPECT_EQ(adapter.GetEmptyMuteCheckTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO), nullptr);
    EXPECT_EQ(adapter.GetEmptyMuteCheckTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO), nullptr);
    HILOGI("TimerOperation002 end");
}

/**
 * @tc.number: UpdateRouteFlagList001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, UpdateRouteFlagList001, TestSize.Level1)
{
    HILOGI("UpdateRouteFlagList001 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.ClearRouteFlagList();

    std::map<uint32_t, AudioStandard::RendererStreamInfo> rendererStreams;
    rendererStreams[DEFAULT_STREAM_ID] = CreateRendererStreamInfo(
        AudioStandard::RendererState::RENDERER_RUNNING,
        OHOS::AudioStandard::STREAM_USAGE_UNKNOWN,
        DEFAULT_STREAM_ID,
        "com.update.test");

    adapter.UpdateRouteFlagList(DEFAULT_ROUTE_FLAG, rendererStreams);
    const std::vector<RouteFlagInfo>& list = adapter.GetRouteFlagList();
    EXPECT_EQ(list.size(), LIST_SIZE_ONE);
    EXPECT_EQ(list[TEST_INDEX_0].routeFlag, DEFAULT_ROUTE_FLAG);
    EXPECT_EQ(list[TEST_INDEX_0].streamId, DEFAULT_STREAM_ID);
    HILOGI("UpdateRouteFlagList001 end");
}

/**
 * @tc.number: UpdateRouteFlagList002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, UpdateRouteFlagList002, TestSize.Level1)
{
    HILOGI("UpdateRouteFlagList002 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.ClearRouteFlagList();

    std::map<uint32_t, AudioStandard::RendererStreamInfo> rendererStreams;
    std::vector<AudioStandard::StreamUsage> usageTypes = {
        AudioStandard::STREAM_USAGE_UNKNOWN,
        AudioStandard::STREAM_USAGE_MUSIC,
        AudioStandard::STREAM_USAGE_VOICE_COMMUNICATION
    };

    for (int i = 0; i < LOOP_COUNT_THREE; i++) {
        AudioStandard::RendererStreamInfo info = CreateRendererStreamInfo(
            (i % TWO == 0) ? AudioStandard::RendererState::RENDERER_STOPPED
                           : AudioStandard::RendererState::RENDERER_RUNNING,
            usageTypes[i],
            DEFAULT_STREAM_ID + i,
            "com.test.app" + std::to_string(i));
        rendererStreams[DEFAULT_STREAM_ID + i] = info;
    }

    adapter.UpdateRouteFlagList(DEFAULT_ROUTE_FLAG, rendererStreams);
    const std::vector<RouteFlagInfo>& list = adapter.GetRouteFlagList();
    EXPECT_EQ(list.size(), LIST_SIZE_THREE);
    HILOGI("UpdateRouteFlagList002 end");
}

/**
 * @tc.number: UpdateRouteFlagList003
 * @tc.name:
 * @tc.desc: 测试bundleName长度超限截断
 */
HWTEST_F(SleAudioFrameworkAdapterTest, UpdateRouteFlagList003, TestSize.Level1)
{
    HILOGI("UpdateRouteFlagList003 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.ClearRouteFlagList();

    std::map<uint32_t, AudioStandard::RendererStreamInfo> rendererStreams;
    std::string longBundleName(MAX_ROUTE_FLAG_LIST_SIZE_TEST, 'a');
    rendererStreams[DEFAULT_STREAM_ID] = CreateRendererStreamInfo(
        AudioStandard::RendererState::RENDERER_RUNNING,
        OHOS::AudioStandard::STREAM_USAGE_UNKNOWN,
        DEFAULT_STREAM_ID,
        longBundleName);

    adapter.UpdateRouteFlagList(DEFAULT_ROUTE_FLAG, rendererStreams);
    const std::vector<RouteFlagInfo>& list = adapter.GetRouteFlagList();
    EXPECT_EQ(list.size(), LIST_SIZE_ONE);
    EXPECT_EQ(list[TEST_INDEX_0].bundleName.length(), BUNDLE_NAME_LENGTH_ZERO);
    HILOGI("UpdateRouteFlagList003 end");
}

/**
 * @tc.number: JudgeEmptyStream001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, JudgeEmptyStream001, TestSize.Level1)
{
    HILOGI("JudgeEmptyStream001 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.ClearRouteFlagList();

    adapter.AddMemberRouteFlagList(CreateRouteFlagInfo(DEFAULT_ROUTE_FLAG, DEFAULT_STREAM_ID,
        DEFAULT_STREAM_USAGE, AudioStandard::RendererState::RENDERER_STOPPED, "app1"));
    adapter.AddMemberRouteFlagList(CreateRouteFlagInfo(DEFAULT_ROUTE_FLAG + 1, DEFAULT_STREAM_ID + 1,
        TEST_INDEX_1, AudioStandard::RendererState::RENDERER_STOPPED, "app2"));

    RawAddress device = CreateTestDevice();
    adapter.JudgeEmptyStreamByRendererState(device);
    HILOGI("JudgeEmptyStream001 end");
}

/**
 * @tc.number: JudgeEmptyStream002
 * @tc.name:
 * @tc.desc: 测试通话流存在时提前返回
 */
HWTEST_F(SleAudioFrameworkAdapterTest, JudgeEmptyStream002, TestSize.Level1)
{
    HILOGI("JudgeEmptyStream002 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.ClearRouteFlagList();

    adapter.AddMemberRouteFlagList(CreateRouteFlagInfo(DEFAULT_ROUTE_FLAG, DEFAULT_STREAM_ID,
        AudioStandard::STREAM_USAGE_VOICE_RINGTONE, AudioStandard::RendererState::RENDERER_STOPPED, "app1"));
    adapter.AddMemberRouteFlagList(CreateRouteFlagInfo(DEFAULT_ROUTE_FLAG + 1, DEFAULT_STREAM_ID + 1,
        DEFAULT_STREAM_USAGE, AudioStandard::RendererState::RENDERER_STOPPED, "app2"));

    RawAddress device = CreateTestDevice();
    adapter.JudgeEmptyStreamByRendererState(device);
    HILOGI("JudgeEmptyStream002 end");
}

/**
 * @tc.number: JudgeDataMuteStream001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, JudgeDataMuteStream001, TestSize.Level1)
{
    HILOGI("JudgeDataMuteStream001 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.ClearRouteFlagList();
    RawAddress device = CreateTestDevice();
    adapter.JudgeDataMuteStreamByRendererState(
        device, DEFAULT_STREAM_ID, false);
    HILOGI("JudgeDataMuteStream001 end");
}

/**
 * @tc.number: JudgeVolumeMuteStream001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAudioFrameworkAdapterTest, JudgeVolumeMuteStream001, TestSize.Level1)
{
    HILOGI("JudgeVolumeMuteStream001 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.ClearRouteFlagList();

    adapter.AddMemberRouteFlagList(CreateRouteFlagInfo(DEFAULT_ROUTE_FLAG, DEFAULT_STREAM_ID,
        AudioStandard::STREAM_USAGE_VOICE_RINGTONE, AudioStandard::RendererState::RENDERER_STOPPED, "app1"));

    RawAddress device = CreateTestDevice();
    adapter.JudgeVolumeMuteStreamByRendererState(device, DEFAULT_STREAM_ID, true);
    HILOGI("JudgeVolumeMuteStream001 end");
}

/**
 * @tc.number: StartEmptyMuteScMcpPauseTimer001
 * @tc.name:
 * @tc.desc: 测试定时器启动和停止
 */
HWTEST_F(SleAudioFrameworkAdapterTest, StartEmptyMuteScMcpPauseTimer001, TestSize.Level1)
{
    HILOGI("StartEmptyMuteScMcpPauseTimer001 start");
    SleAudioFrameworkAdapter& adapter = SleAudioFrameworkAdapter::GetInstance();
    adapter.StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM);
    adapter.StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_DATA_ZERO);
    adapter.StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_MUTE_STREAM_VOL_ZERO);

    RawAddress device = CreateTestDevice();
    adapter.StartEmptyMuteScMcpPauseTimer(
        device, true, TIMER_STREAM_ID, MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM);
    std::this_thread::sleep_for(std::chrono::milliseconds(TEST_DELAY_100_MS));
    adapter.StopEmptyMuteScMcpPauseTimer(MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM);
    HILOGI("StartEmptyMuteScMcpPauseTimer001 end");
}

/**
 * @tc.number: StartEmptyMuteScMcpPauseTimer002
 * @tc.name:
 * @tc.desc: 测试isStart为false时不启动定时器
 */
HWTEST_F(SleAudioFrameworkAdapterTest, StartEmptyMuteScMcpPauseTimer002, TestSize.Level1)
{
    HILOGI("StartEmptyMuteScMcpPauseTimer002 start");
    RawAddress device = CreateTestDevice();
    SleAudioFrameworkAdapter::GetInstance().StartEmptyMuteScMcpPauseTimer(
        device, false, TIMER_STREAM_ID, MCP_MUTE_CONTROL::NL_SLE_MCP_EMPTY_STREAM);
    HILOGI("StartEmptyMuteScMcpPauseTimer002 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS