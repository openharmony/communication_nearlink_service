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

#include "McpServerService.cpp"
#include "ThreadUtil.h"
#include "ProfileServiceManager.cpp"
#include "ASCService.h"
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

// 测试目的：跨 TU 注册表注入辅助——本 TU 唯一编译 ProfileServiceManager.cpp（见 BUILD 排除块），
// 经此函数向同进程其他测试 TU（manager 套件）暴露注册表写入能力。
void SetStartedProfile(SleTransport transport, const std::string &name, SleInterfaceProfile *profile)
{
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(transport, name, profile);
}

static std::shared_ptr<MockAVSessionController> g_mockAVSessionController = std::make_shared<MockAVSessionController>();

namespace {
sptr<NearlinkSleDataTransferServer> g_sleDataTransfer = new (std::nothrow) NearlinkSleDataTransferServer();
}

class McpServerServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    // 测试目的：用例自持真实 McpServerService 实例（不依赖 vendor 栈启动链），随用例生命周期创建/销毁。
    std::unique_ptr<McpServerService> service_;
};


void McpServerServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase McpServerServiceTest start");
    // MCP server service tests own their service; vendor stack startup is an integration dependency.
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
    // 测试目的：MCP 任务切到当前线程同步执行（去等待）；自建服务并直接写入注册表绕过 Start 链
    //（裸进程 slem_initialize 必败），最后用致命断言锁定"GetService 命中本实例"前提。
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_MCP, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    service_ = std::make_unique<McpServerService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_MCP_SERVER, service_.get());
    // 测试目的：同步注入 mock ASC 服务——MCP 按键/ IsActiveDevice 产品链经注册表取 PROFILE_NAME_ASC；
    // mock ASC 以测试地址 00:11:22:33:44:55 为激活设备，使按键链前提成立。
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_ASC, ASCService::GetService());
    ASSERT_EQ(service_.get(), McpServerService::GetService());
}

void McpServerServiceTest::TearDown()
{
    HILOGI("TearDown McpServerServiceTest.");
    // 测试目的：先解绑注册表再销毁服务，避免 GetService 命中悬垂指针；最后恢复线程调度默认态。
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_MCP_SERVER, nullptr);
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_ASC, nullptr);
    service_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

/**
 * @tc.number: GetService001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：fixture 注册表注入生效——GetService 必须返回非空服务实例（原 EXPECT 升级为致命断言，
// 原 sleep(100ms) 等待已无意义：服务由本用例持有，无需轮询）。
HWTEST_F(McpServerServiceTest, GetService001, TestSize.Level1)
{
    HILOGI("GetService001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    HILOGI("GetService001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空再 Disable；NOT_SWITCH_THREAD 下事件同步处理，
// 原 sleep(100ms) 等待不再需要。
HWTEST_F(McpServerServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->Disable();
    mcpServerService->Disable(); // repeat
    HILOGI("Disable001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空再 Enable；NLSTK 媒体实例创建已由 mock_nlstk_mcp_media_server
// 垫掉，Enable 同步完成，原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->Enable();
    mcpServerService->Enable(); // repeat
    HILOGI("Enable001 end");
}

/**
 * @tc.number: McpInit
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空再直调 pimpl->McpInit()（mock 栈返回成功）；
// NOT_SWITCH_THREAD 下无异步收尾，原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, McpInit001, TestSize.Level1)
{
    HILOGI("McpInit001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->McpInit();

    HILOGI("McpInit001 end");
}


/**
 * @tc.number: InitMediaListener001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：AVSession 桩生效前提下 InitMediaListener 完成媒体监听器初始化不崩溃；
// 前提断言前置（服务指针先判空再用），原 sleep(100ms) 不再需要。
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
    ASSERT_NE(nullptr, mcpServerService);

    mcpServerService->LoadMediaSo();
    mcpServerService->InitMediaListener();

    HILOGI("InitMediaListener001 end");
}

/**
 * @tc.number: SendKeyEventByWearDetection001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：AVSession 桩生效前提下佩戴检测按键事件送出不崩溃；服务改用例自持后先装载媒体库
// （原依赖套件级共享服务的加载残留），再送出按键；原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, SendKeyEventByWearDetection001, TestSize.Level1)
{
    HILOGI("SendKeyEventByWearDetection001 start");

    Stub stub;
    stub.set(ADDR(AVSessionManager, GetInstance), MockAVSessionManager::StubGetInstance);
    EXPECT_CALL(MockAVSessionManager::StubGetInstance(), SendSystemAVKeyEvent(_))
        .Times(1).WillOnce(Return(AVSESSION_SUCCESS));

    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);

    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    uint8_t key = static_cast<uint8_t>(MCP_ID_PLAY);
    mcpServerService->LoadMediaSo();
    mcpServerService->SendKeyEventByWearDetection(*device, key);

    HILOGI("SendKeyEventByWearDetection001 end");
}

/**
 * @tc.number: IsAVPlaybackStatePlay001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空再查媒体加载器；原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, IsAVPlaybackStatePlay001, TestSize.Level1)
{
    HILOGI("GetAVPlaybackState001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    bool ret = mcpServerService->GetMediaLoader()->IsAVPlaybackStatePlay();
    EXPECT_NE(true, ret);
    HILOGI("GetAVPlaybackState001 end");
}

/*
 * @tc.number: StartMediaInstCallback001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再直调启动实例回调；原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, StartMediaInstCallback001, TestSize.Level1)
{
    HILOGI("StartMediaInstCallback001 start");
    int32_t instanceId = 1;
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_SUCCESS;
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->StartMediaInstCallback(instanceId, ret);

    HILOGI("StartMediaInstCallback001 end");
}

/*
 * @tc.number: PlayCallback001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再直调播放控制点回调（事件同步投递，无栈依赖）；
// 原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, PlayCallback001, TestSize.Level1)
{
    HILOGI("PlayCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->PlayCallback(&sleAddr, requestId, instanceId);

    HILOGI("PlayCallback001 end");
}

/*
 * @tc.number: StopCallback001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再直调停止控制点回调；原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, StopCallback001, TestSize.Level1)
{
    HILOGI("StopCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->StopCallback(&sleAddr, requestId, instanceId);

    HILOGI("StopCallback001 end");
}

/*
 * @tc.number: PauseCallback001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再直调暂停控制点回调；原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, PauseCallback001, TestSize.Level1)
{
    HILOGI("PauseCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->PauseCallback(&sleAddr, requestId, instanceId);

    HILOGI("PauseCallback001 end");
}

/*
 * @tc.number: FastForwardCallback001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再直调快进控制点回调；原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, FastForwardCallback001, TestSize.Level1)
{
    HILOGI("FastForwardCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->FastForwardCallback(&sleAddr, requestId, instanceId);

    HILOGI("FastForwardCallback001 end");
}

/*
 * @tc.number: PreviousMediaCallback001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再直调上一曲控制点回调；原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, PreviousMediaCallback001, TestSize.Level1)
{
    HILOGI("PreviousMediaCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->PreviousMediaCallback(&sleAddr, requestId, instanceId);

    HILOGI("PreviousMediaCallback001 end");
}

/*
 * @tc.number: NextMediaCallback001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再直调下一曲控制点回调；原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, NextMediaCallback001, TestSize.Level1)
{
    HILOGI("NextMediaCallback001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->NextMediaCallback(&sleAddr, requestId, instanceId);

    HILOGI("NextMediaCallback001 end");
}

/*
 * @tc.number: AuthorizeCallback001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再直调授权回调；原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, AuthorizeCallback001, TestSize.Level1)
{
    HILOGI("AuthorizeCallback001 start");
    uint16_t requestId = 1;
    int32_t instanceId = 1;
    NLSTK_McpPropertyType_E mcpProperty = NLSTK_McpPropertyType_E::NLSTK_MCP_MEDIA_INSTANCE_NAME;
    NLSTK_ServicePropertyOpType_E serviceProperty = NLSTK_ServicePropertyOpType_E::NLSTK_SSAP_PROPERTY_READ;
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->pimpl->AuthorizeCallback(requestId, instanceId, mcpProperty, serviceProperty);

    HILOGI("AuthorizeCallback001 end");
}

/*
 * @tc.number: RendererStreamStateChange001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：前提断言前置——服务指针先判空再通知渲染流状态变化；原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, RendererStreamStateChange001, TestSize.Level1)
{
    HILOGI("RendererStreamStopState001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    int state = 1;
    mcpServerService->GetMediaLoader()->RendererStreamStateChange(state);
    HILOGI("RendererStreamStopState001 end");
}

/*
 * @tc.number: InitAndDeinit001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：媒体库就绪前提下 Init/DeInit 全流程可用——本用例先显式 LoadMediaSo 装载媒体库
//（原用例依赖套件级共享服务的加载残留；服务改为用例自持后，装载前提由本用例自建），
// 再断言库已加载并完成 Init/DeInit 往返；原 sleep(100ms) 不再需要。
HWTEST_F(McpServerServiceTest, InitAndDeinit001, TestSize.Level1)
{
    HILOGI("InitAndDeinit001 start");
    McpServerService* mcpServerService = McpServerService::GetService();
    ASSERT_NE(nullptr, mcpServerService);
    mcpServerService->LoadMediaSo();
    auto mediaLoader = mcpServerService->GetMediaLoader();
    bool state = mediaLoader->IsLibraryLoaded();
    EXPECT_EQ(true, state);
    mediaLoader->Init();
    mediaLoader->DeInit();
    HILOGI("InitAndDeinit001 end");
}


} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
