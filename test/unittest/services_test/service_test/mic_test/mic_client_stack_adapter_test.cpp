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

#include <chrono>
#include <thread>
#include <gtest/gtest.h>
#include <memory>
#include "ThreadUtil.h"
#include "MicClientStackAdapter.h"
#include "MicService.h"
#include "log.h"
#include "nlstk_public_define.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
void OnMicpConnectEventCbk(
    SLE_Addr_S *addr, NLSTK_MicpConnectState_E curState, NLSTK_MicpConnectState_E preState, uint8_t errorCode);
void OnMicpMicStateCbk(SLE_Addr_S *addr, uint8_t micState);

// 测试目的：跨 TU 注册表注入辅助——ProfileServiceManager.cpp 仅由 mic_service_test.cpp 编译，
// 本套件经此函数（定义在该 TU）注入/解绑 MIC 服务实例。
void SetMicStartedProfile(MicService *profile);

namespace {
    constexpr int MIC_SERVICE_UT_DELAY_50_MS = 50;
    RawAddress g_mockDevice = RawAddress("00:11:22:33:44:55");
    MicClientStackAdapter g_MicStackAdapter_;
}

class NearlinkMicClientStackAdapterTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    // 测试目的：用例自持真实 MicService 实例——adapter 回调内部经 GetService() 找服务，
    // 用例也直接对其断言/调用；不依赖 vendor 栈启动链。
    std::unique_ptr<MicService> service_;
};

void NearlinkMicClientStackAdapterTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkMicClientStackAdapterTest");
}

void NearlinkMicClientStackAdapterTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkMicClientStackAdapterTest");
}

void NearlinkMicClientStackAdapterTest::SetUp()
{
    HILOGI("SetUp NearlinkMicClientStackAdapterTest.");
    // 测试目的：MIC 任务同步执行 + 注册表注入服务实例（绕过 Start 链），
    // 使 adapter 回调内部的 GetService() 与用例断言都能命中真实服务。
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_MIC, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    service_ = std::make_unique<MicService>();
    SetMicStartedProfile(service_.get());
    ASSERT_EQ(service_.get(), MicService::GetService());
}

void NearlinkMicClientStackAdapterTest::TearDown()
{
    HILOGI("TearDown NearlinkMicClientStackAdapterTest.");
    // 测试目的：先解绑注册表再销毁服务，最后恢复线程调度默认态。
    SetMicStartedProfile(nullptr);
    service_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

/*
 * @tc.number: RegisterCallBackToStack001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkMicClientStackAdapterTest, RegisterCallBackToStack001, TestSize.Level1)
{
    HILOGI("RegisterCallBackToStack001 start");
    int ret = g_MicStackAdapter_.RegisterCallBackToStack();
    EXPECT_EQ(MIC_SUCCESS, ret);
    HILOGI("RegisterCallBackToStack001 end");
}

/*
 * @tc.number: Connect001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkMicClientStackAdapterTest, Connect001, TestSize.Level1)
{
    HILOGI("Connect001 start");
    int ret = g_MicStackAdapter_.Connect(g_mockDevice);
    EXPECT_EQ(MIC_SUCCESS, ret);
    HILOGI("Connect001 end");
}

/*
 * @tc.number: Disconnect001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkMicClientStackAdapterTest, Disconnect001, TestSize.Level1)
{
    HILOGI("Disconnect001 start");
    int ret = g_MicStackAdapter_.Disconnect(g_mockDevice);
    EXPECT_EQ(MIC_SUCCESS, ret);
    HILOGI("Disconnect001 end");
}

/*
 * @tc.number: OnMicpConnectEventCbk001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkMicClientStackAdapterTest, OnMicpConnectEventCbk001, TestSize.Level1)
{
    HILOGI("OnMicpConnectEventCbk001 start");
    g_MicStackAdapter_.OnMicpConnectEventCbk(
        nullptr, NLSTK_MICP_STATE_CONNECTED, NLSTK_MICP_STATE_CONNECTING, NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    bool isMicOpen = micService->IsMicOpen(g_mockDevice);
    EXPECT_EQ(false, isMicOpen);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnMicpConnectEventCbk001 end");
}

/*
 * @tc.number: OnMicpConnectEventCbk002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicClientStackAdapterTest, OnMicpConnectEventCbk002, TestSize.Level1)
{
    HILOGI("OnMicpConnectEventCbk002 start");
    SLE_Addr_S addr = {
        .type = 0,
        .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 }
    };
    g_MicStackAdapter_.OnMicpConnectEventCbk(
        &addr, NLSTK_MICP_STATE_CONNECTED, NLSTK_MICP_STATE_CONNECTING, NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    bool isMicOpen = micService->IsMicOpen(g_mockDevice);
    EXPECT_EQ(false, isMicOpen);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnMicpConnectEventCbk002 end");
}

/*
 * @tc.number: OnMicpMicStateCbk001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicClientStackAdapterTest, OnMicpMicStateCbk001, TestSize.Level1)
{
    HILOGI("OnMicpMicStateCbk001 start");
    g_MicStackAdapter_.OnMicpMicStateCbk(nullptr, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    bool isMicOpen = micService->IsMicOpen(g_mockDevice);
    EXPECT_EQ(false, isMicOpen);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnMicpMicStateCbk001 end");
}

/*
 * @tc.number: OnMicpMicStateCbk002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicClientStackAdapterTest, OnMicpMicStateCbk002, TestSize.Level1)
{
    HILOGI("OnMicpMicStateCbk002 start");
    SLE_Addr_S addr = {
        .type = 0,
        .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 }
    };
    g_MicStackAdapter_.OnMicpMicStateCbk(&addr, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    bool isMicOpen = micService->IsMicOpen(g_mockDevice);
    EXPECT_EQ(false, isMicOpen);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnMicpMicStateCbk002 end");
}

/*
 * @tc.number: OnMicpMicStateCbk003
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicClientStackAdapterTest, OnMicpMicStateCbk003, TestSize.Level1)
{
    HILOGI("OnMicpMicStateCbk003 start");
    SLE_Addr_S addr = {
        .type = 0,
        .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 }
    };
    g_MicStackAdapter_.OnMicpConnectEventCbk(
        &addr, NLSTK_MICP_STATE_CONNECTED, NLSTK_MICP_STATE_CONNECTING, NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    g_MicStackAdapter_.OnMicpMicStateCbk(&addr, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    bool isMicOpen = micService->IsMicOpen(g_mockDevice);
    EXPECT_EQ(true, isMicOpen);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnMicpMicStateCbk003 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS