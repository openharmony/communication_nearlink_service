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
#include <gmock/gmock.h>
#include <thread>
#include <vector>
#include <memory>

#include "nearlink_access_token_mock.h"
#include "McpServerService.cpp"
#include "McpDefines.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "nearlink_sle_datatransfer_server.h"
#include "mock_avsession_manager.h"
#include "mock_avsession_controller.h"
#include "log.h"
 #include "stub.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::AVSession;
using ::testing::Return;
using ::testing::_;
using ::testing::Invoke;

static std::shared_ptr<MockAVSessionController> g_mockAVSessionController = std::make_shared<MockAVSessionController>();

namespace {
constexpr int MCP_SERVICE_UT_DELAY_100_MS = 100;
constexpr int MCP_SERVICE_UT_DELAY_1000_MS = 1000;
sptr<NearlinkSleDataTransferServer> g_sleDataTransfer = new (std::nothrow) NearlinkSleDataTransferServer();
}

class McpServerServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};


void McpServerServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase McpServerServiceTest start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_1000_MS));
    HILOGI("SetUpTestCase McpServerServiceTest end");
}

void McpServerServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase McpServerServiceTest");
    g_mockAVSessionController = nullptr;
}

void McpServerServiceTest::SetUp()
{
    HILOGI("SetUp McpServerServiceTest.");
}

void McpServerServiceTest::TearDown()
{
    HILOGI("TearDown McpServerServiceTest.");
}

/**
 * @tc.number: GetService001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(McpServerServiceTest, GetService001, TestSize.Level1)
{
    HILOGI("GetService001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("GetService001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(McpServerServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    mcpServerService->Disable();
    mcpServerService->Disable(); // repeat
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("Disable001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(McpServerServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    mcpServerService->Enable();
    mcpServerService->Enable(); // repeat
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("Enable001 end");
}

/**
 * @tc.number: McpInit
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(McpServerServiceTest, McpInit001, TestSize.Level1)
{
    HILOGI("McpInit001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->McpInit();

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("McpInit001 end");
}


/**
 * @tc.number: InitMediaListener001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(McpServerServiceTest, InitMediaListener001, TestSize.Level1)
{
    HILOGI("InitMediaListener001 start");
    Stub stub;
    stub.set(ADDR(AVSessionManager, GetInstance), MockAVSessionManager::StubGetInstance);
    EXPECT_CALL(MockAVSessionManager::StubGetInstance(), RegisterSessionListenerForAllUsers(_)).Times(1)
        .WillOnce(Return(AVSESSION_SUCCESS));
    EXPECT_CALL(MockAVSessionManager::StubGetInstance(), GetAllSessionDescriptors(_)).Times(1)
        .WillOnce(Invoke([](std::vector<AVSessionDescriptor> &avSessionDescriptor) -> int32_t {
            AVSessionDescriptor descriptor1;
            descriptor1.sessionId_ = "123";
            descriptor1.pid_ = 1;
            descriptor1.uid_ = 1;
            descriptor1.isTopSession_ = true;
            avSessionDescriptor.push_back(descriptor1);
            return AVSESSION_SUCCESS;
        }));

    EXPECT_CALL(MockAVSessionManager::StubGetInstance(), CreateController(_, _)).Times(1)
        .WillOnce(Invoke([](const std::string& sessionId,
                    std::shared_ptr<AVSessionController>& controller) -> int32_t {
            controller = g_mockAVSessionController;
            return AVSESSION_SUCCESS;
        }));

    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);

    mcpServerService->LoadMediaSo();
    mcpServerService->InitMediaListener();
    EXPECT_NE(nullptr, mcpServerService);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("InitMediaListener001 end");
}

/**
 * @tc.number: SendKeyEventByWearDetection001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(McpServerServiceTest, SendKeyEventByWearDetection001, TestSize.Level1)
{
    HILOGI("SendKeyEventByWearDetection001 start");

    Stub stub;
    stub.set(ADDR(AVSessionManager, GetInstance), MockAVSessionManager::StubGetInstance);
    EXPECT_CALL(MockAVSessionManager::StubGetInstance(), SendSystemAVKeyEvent(_))
        .Times(1).WillOnce(Return(AVSESSION_SUCCESS));

    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);

    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    uint8_t key = static_cast<uint8_t>(MCP_ID_PLAY);
    mcpServerService->SendKeyEventByWearDetection(*device, key);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("SendKeyEventByWearDetection001 end");
}

/**
 * @tc.number: IsAVPlaybackStatePlay001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(McpServerServiceTest, IsAVPlaybackStatePlay001, TestSize.Level1)
{
    HILOGI("GetAVPlaybackState001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    bool ret = mcpServerService->GetMediaLoader()->IsAVPlaybackStatePlay();
    EXPECT_NE(true, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("GetAVPlaybackState001 end");
}

/*
 * @tc.number: StartMediaInstCallback001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceTest, StartMediaInstCallback001, TestSize.Level1)
{
    HILOGI("StartMediaInstCallback001 start");
    int32_t instanceId = 1;
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->StartMediaInstCallback(instanceId, ret);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("StartMediaInstCallback001 end");
}

/*
 * @tc.number: PlayCallback001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceTest, PlayCallback001, TestSize.Level1)
{
    HILOGI("PlayCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->PlayCallback(&sleAddr, requestId, instanceId);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("PlayCallback001 end");
}

/*
 * @tc.number: StopCallback001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceTest, StopCallback001, TestSize.Level1)
{
    HILOGI("StopCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->StopCallback(&sleAddr, requestId, instanceId);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("StopCallback001 end");
}

/*
 * @tc.number: PauseCallback001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceTest, PauseCallback001, TestSize.Level1)
{
    HILOGI("PauseCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->PauseCallback(&sleAddr, requestId, instanceId);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("PauseCallback001 end");
}

/*
 * @tc.number: FastForwardCallback001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceTest, FastForwardCallback001, TestSize.Level1)
{
    HILOGI("FastForwardCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->FastForwardCallback(&sleAddr, requestId, instanceId);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("FastForwardCallback001 end");
}

/*
 * @tc.number: PreviousMediaCallback001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceTest, PreviousMediaCallback001, TestSize.Level1)
{
    HILOGI("PreviousMediaCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->PreviousMediaCallback(&sleAddr, requestId, instanceId);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("PreviousMediaCallback001 end");
}

/*
 * @tc.number: NextMediaCallback001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceTest, NextMediaCallback001, TestSize.Level1)
{
    HILOGI("NextMediaCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->NextMediaCallback(&sleAddr, requestId, instanceId);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("NextMediaCallback001 end");
}

/*
 * @tc.number: AuthorizeCallback001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceTest, AuthorizeCallback001, TestSize.Level1)
{
    HILOGI("AuthorizeCallback001 start");
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    NLSTK_McpPropertyType_E mcpProperty = NLSTK_McpPropertyType_E::NLSTK_MCP_MEDIA_INSTANCE_NAME;
    NLSTK_ServicePropertyOpType_E serviceProperty = NLSTK_ServicePropertyOpType_E::NLSTK_SSAP_PROPERTY_READ;
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->AuthorizeCallback(requestId, instanceId, mcpProperty, serviceProperty);

    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("AuthorizeCallback001 end");
}

/*
 * @tc.number: RendererStreamStateChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceTest, RendererStreamStateChange001, TestSize.Level1)
{
    HILOGI("RendererStreamStopState001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    int state = 1;
    mcpServerService->GetMediaLoader()->RendererStreamStateChange(state);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("RendererStreamStopState001 end");
}

/*
 * @tc.number: InitAndDeinit001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(McpServerServiceTest, InitAndDeinit001, TestSize.Level1)
{
    HILOGI("InitAndDeinit001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    EXPECT_NE(nullptr, mcpServerService);
    auto mediaLoader = mcpServerService->GetMediaLoader();
    bool state = mediaLoader->IsLibraryLoaded();
    EXPECT_EQ(true, state);
    mediaLoader->Init();
    mediaLoader->DeInit();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::MCP_SERVICE_UT_DELAY_100_MS));
    HILOGI("InitAndDeinit001 end");
}


} // namespace TEST
} // namespace Nearlink
} // namespace OHOS