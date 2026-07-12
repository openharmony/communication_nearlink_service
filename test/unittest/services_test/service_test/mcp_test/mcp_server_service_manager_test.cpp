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
#include "SleInterfaceProfileCdsm.h"
#include "SleInterfaceProfileASC.h"
#include "SleInterfaceProfileTws.h"
#include "McpServerServiceManager.cpp"
#include "log.h"
#include "mock_avsession_manager.h"
#include "mock_avsession_controller.h"
#include "stub.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::AVSession;
using ::testing::Return;
using ::testing::_;
using ::testing::Invoke;

constexpr int MCP_SERVICE_UT_DELAY_50_MS = 50;
constexpr int MCP_SERVICE_UT_DELAY_1200_MS = 1200;
constexpr int CALL_TIMES = 50;
static std::shared_ptr<MockAVSessionController> g_mockAVSessionController = std::make_shared<MockAVSessionController>();
static std::shared_ptr<McpServerServiceManager> g_McpManager = std::make_shared<McpServerServiceManager>();

class McpServerServiceManagerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void McpServerServiceManagerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase McpServerServiceManagerTest start");
    Stub stub;
    stub.set(ADDR(AVSessionManager, GetInstance), MockAVSessionManager::StubGetInstance);

    EXPECT_CALL(MockAVSessionManager::StubGetInstance(), RegisterSessionListenerForAllUsers(_)).Times(CALL_TIMES)
        .WillOnce(Return(AVSESSION_SUCCESS));
    EXPECT_CALL(MockAVSessionManager::StubGetInstance(), GetAllSessionDescriptors(_)).Times(CALL_TIMES)
        .WillOnce(Invoke([](std::vector<AVSessionDescriptor> &avSessionDescriptor) -> int32_t {
            AVSessionDescriptor descriptor1;
            descriptor1.sessionId_ = "123";
            descriptor1.pid_ = 1;
            descriptor1.uid_ = 1;
            descriptor1.isTopSession_ = true;
            avSessionDescriptor.push_back(descriptor1);
            return AVSESSION_SUCCESS;
        }));
    EXPECT_CALL(MockAVSessionManager::StubGetInstance(), CreateController(_, _)).Times(CALL_TIMES)
        .WillOnce(Invoke([](const std::string& sessionId,
                    std::shared_ptr<AVSessionController>& controller) -> int32_t {
            controller = g_mockAVSessionController;
            return AVSESSION_SUCCESS;
        }));

    g_McpManager->SetTwsProfileFunc([]() {
        return static_cast<ProfileTws *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_TWS));
    });
    g_McpManager->SetCdsmProfileFunc([]() {
        return static_cast<ProfileCdsm *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_CDSM));
    });
    g_McpManager->SetAscProfileFunc([]() {
        return static_cast<ProfileASC *>(
            SleInterfaceProfileManager::GetInstance().GetProfileService(PROFILE_NAME_ASC));
    });
    g_McpManager->Init();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("SetUpTestCase McpServerServiceManagerTest end");
}

void McpServerServiceManagerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase McpServerServiceManagerTest");
    g_McpManager->DeInit();
    g_mockAVSessionController = nullptr;
}

void McpServerServiceManagerTest::SetUp()
{
    HILOGI("SetUp McpServerServiceManagerTest.");
    g_McpManager->InitMediaListener();
}

void McpServerServiceManagerTest::TearDown()
{
    HILOGI("TearDown McpServerServiceManagerTest.");
}

/*
 * @tc.number: OnMetaDataChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnMetaDataChange001, TestSize.Level1)
{
    HILOGI("OnMetaDataChange001 start");
    const AVMetaData data;

    g_McpManager->pimpl->avControllerObserver_->OnMetaDataChange(data);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnMetaDataChange001 end");
}

/*
 * @tc.number: OnPlaybackStateChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnPlaybackStateChange001, TestSize.Level1)
{
    HILOGI("OnPlaybackStateChange001 start");
    AVPlaybackState state;
    g_McpManager->pimpl->avControllerObserver_->OnPlaybackStateChange(state);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnPlaybackStateChange001 end");
}

/*
 * @tc.number: OnSessionDestroy001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnSessionDestroy001, TestSize.Level1)
{
    HILOGI("OnSessionDestroy001 start");
    g_McpManager->pimpl->avControllerObserver_->OnSessionDestroy();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnSessionDestroy001 end");
}

/*
 * @tc.number: OnActiveStateChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnActiveStateChange001, TestSize.Level1)
{
    HILOGI("OnActiveStateChange001 start");
    g_McpManager->pimpl->avControllerObserver_->OnActiveStateChange(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnActiveStateChange001 end");
}

/*
 * @tc.number: OnValidCommandChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnValidCommandChange001, TestSize.Level1)
{
    HILOGI("OnValidCommandChange001 start");
    std::vector<int32_t> cmds;
    g_McpManager->pimpl->avControllerObserver_->OnValidCommandChange(cmds);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnValidCommandChange001 end");
}

/*
 * @tc.number: OnOutputDeviceChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnOutputDeviceChange001, TestSize.Level1)
{
    HILOGI("OnOutputDeviceChange001 start");
    int32_t param = 0;
    const OutputDeviceInfo info;
    g_McpManager->pimpl->avControllerObserver_->OnOutputDeviceChange(param, info);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnOutputDeviceChange001 end");
}

/*
 * @tc.number: OnOutputDeviceChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnSessionEventChange001, TestSize.Level1)
{
    HILOGI("OnSessionEventChange001 start");
    std::string event;
    AAFwk::WantParams args;
    g_McpManager->pimpl->avControllerObserver_->OnSessionEventChange(event, args);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnSessionEventChange001 end");
}

/*
 * @tc.number: OnOutputDeviceChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnQueueItemsChange001, TestSize.Level1)
{
    HILOGI("OnQueueItemsChange001 start");
    std::vector<AVQueueItem> item;
    g_McpManager->pimpl->avControllerObserver_->OnQueueItemsChange(item);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnQueueItemsChange001 end");
}

/*
 * @tc.number: OnQueueTitleChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnQueueTitleChange001, TestSize.Level1)
{
    HILOGI("OnQueueTitleChange001 start");
    std::string title;
    g_McpManager->pimpl->avControllerObserver_->OnQueueTitleChange(title);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnQueueTitleChange001 end");
}

/*
 * @tc.number: OnExtrasChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnExtrasChange001, TestSize.Level1)
{
    HILOGI("OnExtrasChange001 start");
    AAFwk::WantParams extras;
    g_McpManager->pimpl->avControllerObserver_->OnExtrasChange(extras);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnExtrasChange001 end");
}

/*
 * @tc.number: OnAVCallStateChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnAVCallStateChange001, TestSize.Level1)
{
    HILOGI("OnAVCallStateChange001 start");
    AVCallState avCallState;
    g_McpManager->pimpl->avControllerObserver_->OnAVCallStateChange(avCallState);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnAVCallStateChange001 end");
}

/*
 * @tc.number: OnRendererStateChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnRendererStateChange001, TestSize.Level1)
{
    HILOGI("OnRendererStateChange001 start");
    std::vector<std::shared_ptr<AudioStandard::AudioRendererChangeInfo>> audioRendererChangeInfos;
    std::shared_ptr<AudioStandard::AudioRendererChangeInfo> rendererChangeInfo =
        std::make_shared<AudioStandard::AudioRendererChangeInfo>();
    rendererChangeInfo->clientPid = 100;
    rendererChangeInfo->createrUID = 100;
    rendererChangeInfo->rendererState = AudioStandard::RendererState::RENDERER_RUNNING;
    audioRendererChangeInfos.push_back(rendererChangeInfo);

    g_McpManager->pimpl->rendererStateCallback_->OnRendererStateChange(audioRendererChangeInfos);
    g_McpManager->pimpl->rendererStateCallback_->OnRendererStateChange(audioRendererChangeInfos); // repeat

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_1200_MS));
    HILOGI("OnRendererStateChange001 end");
}

/*
 * @tc.number: OnRendererStateChange002
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnRendererStateChange002, TestSize.Level1)
{
    HILOGI("OnRendererStateChange002 start");
    std::vector<std::shared_ptr<AudioStandard::AudioRendererChangeInfo>> audioRendererChangeInfos;
    std::shared_ptr<AudioStandard::AudioRendererChangeInfo> rendererChangeInfo =
        std::make_shared<AudioStandard::AudioRendererChangeInfo>();
    rendererChangeInfo->clientPid = 100;
    rendererChangeInfo->createrUID = 100;
    rendererChangeInfo->rendererState = AudioStandard::RendererState::RENDERER_PAUSED;
    audioRendererChangeInfos.push_back(rendererChangeInfo);

    g_McpManager->pimpl->rendererStateCallback_->OnRendererStateChange(audioRendererChangeInfos);
    g_McpManager->pimpl->rendererStateCallback_->OnRendererStateChange(audioRendererChangeInfos); // repeat

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_1200_MS));
    HILOGI("OnRendererStateChange002 end");
}

/*
 * @tc.number: OnTopSessionChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnTopSessionChange001, TestSize.Level1)
{
    HILOGI("OnTopSessionChange001 start");
    AVSessionDescriptor avSessionDescriptor;
    avSessionDescriptor.sessionId_ = "123";
    avSessionDescriptor.pid_ = 1;
    avSessionDescriptor.uid_ = 1;
    avSessionDescriptor.isTopSession_ = true;

    g_McpManager->pimpl->avSessionObserver_->OnTopSessionChange(avSessionDescriptor);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnTopSessionChange001 end");
}

/*
 * @tc.number: OnSessionRelease001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnSessionRelease001, TestSize.Level1)
{
    HILOGI("OnSessionRelease001 start");
    AVSessionDescriptor avSessionDescriptor;
    avSessionDescriptor.sessionId_ = "123";
    avSessionDescriptor.pid_ = 1;
    avSessionDescriptor.uid_ = 1;
    avSessionDescriptor.isTopSession_ = true;

    g_McpManager->pimpl->avSessionObserver_->OnSessionRelease(avSessionDescriptor);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnSessionRelease001 end");
}

/*
 * @tc.number: OnSessionCreate001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, OnSessionCreate001, TestSize.Level1)
{
    HILOGI("OnSessionCreate001 start");
    AVSessionDescriptor avSessionDescriptor;
    avSessionDescriptor.sessionId_ = "123";
    avSessionDescriptor.pid_ = 1;
    avSessionDescriptor.uid_ = 1;
    avSessionDescriptor.isTopSession_ = true;

    g_McpManager->pimpl->avSessionObserver_->OnSessionCreate(avSessionDescriptor);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnSessionCreate001 end");
}

/*
 * @tc.number: IsActiveDevice001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, IsActiveDevice001, TestSize.Level1)
{
    HILOGI("IsActiveDevice001 start");
    std::string addr = "00:11:22:33:44:55";

    bool ret = g_McpManager->pimpl->IsActiveDevice(addr);
    EXPECT_EQ(true, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("IsActiveDevice001 end");
}

/*
 * @tc.number: CreateAndDestoryManager001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, CreateAndDestoryManager001, TestSize.Level1)
{
    HILOGI("CreateAndDestoryManager001 start");
    McpServerServiceManager* manager = CreateMcpMediaInterface();
    EXPECT_NE(nullptr, manager);
    DestroyMcpMediaInterface(manager);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("CreateAndDestoryManager001 end");
}

/*
 * @tc.number: WriteAudioSinkDeviceUe001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, WriteAudioSinkDeviceUe001, TestSize.Level1)
{
    HILOGI("WriteAudioSinkDeviceUe001 start");
    int mcpEvt = MCP_EVT_START_SUCCESS; // invalid param
    std::string addr = "00:11:22:33:44:55";
    int32_t result = 0;
    g_McpManager->pimpl->WriteAudioSinkDeviceUe(mcpEvt, addr, result);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("WriteAudioSinkDeviceUe001 end");
}

/*
 * @tc.number: SendKeyEvent001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, SendKeyEvent001, TestSize.Level1)
{
    HILOGI("SendKeyEvent001 start");
    McpMessage event;
    event.whatM = MCP_EVT_START_SUCCESS; // invalid param
    g_McpManager->pimpl->SendKeyEvent(event);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("SendKeyEvent001 end");
}

/*
 * @tc.number: ConvertAVPlaybackStateToMcpState001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, ConvertAVPlaybackStateToMcpState001, TestSize.Level1)
{
    HILOGI("ConvertAVPlaybackStateToMcpState001 start");
    int state = static_cast<int>(AVPlaybackState::PLAYBACK_STATE_IDLE); // invalid param
    int32_t ret = g_McpManager->pimpl->ConvertAVPlaybackStateToMcpState(state);
    EXPECT_EQ(NLSTK_MCP_STATE_UNINITIALIZED, ret);
    state = static_cast<int>(AVPlaybackState::PLAYBACK_STATE_PLAY); // valid
    ret = g_McpManager->pimpl->ConvertAVPlaybackStateToMcpState(state);
    EXPECT_EQ(NLSTK_MCP_STATE_PLAYING, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("ConvertAVPlaybackStateToMcpState001 end");
}

/*
 * @tc.number: CheckPlaybackStateChanged001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, CheckPlaybackStateChanged001, TestSize.Level1)
{
    HILOGI("CheckPlaybackStateChanged001 start");
    AVPlaybackState state;
    state.SetState(AVPlaybackState::PLAYBACK_STATE_PLAY);
    bool ret = g_McpManager->pimpl->CheckPlaybackStateChanged(state);
    EXPECT_EQ(true, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("CheckPlaybackStateChanged001 end");
}

/*
 * @tc.number: UpdateMediaBasicInfo001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceManagerTest, UpdateMediaBasicInfo001, TestSize.Level1)
{
    HILOGI("UpdateMediaBasicInfo001 start");
    g_McpManager->pimpl->UpdateMediaBasicInfo();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("UpdateMediaBasicInfo001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS