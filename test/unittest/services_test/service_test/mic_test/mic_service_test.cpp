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
#include <memory>
#include "ThreadUtil.h"
#include "ProfileServiceManager.cpp"
#include "MicService.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "log.h"
#include "MicClientStackAdapter.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
// 测试目的：NLSTK micp 栈桩计数器接口（实现见 mock_mic_stack.cpp），用于断言服务确实触达栈层。
void ResetMicStackMock();
int GetMicpConnectCalls();
int GetMicpDisconnectCalls();

// 测试目的：跨 TU 注册表注入辅助——本 TU 唯一编译 ProfileServiceManager.cpp（见 BUILD 排除块），
// 经此函数向同进程 adapter 套件 TU 暴露 MIC 注册表写入能力。
void SetMicStartedProfile(MicService *profile)
{
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_MIC, profile);
}
using namespace testing;
using namespace testing::ext;

class MicClientObserverMock : public MicObserver {
public:
    MicClientObserverMock() = default;
    ~MicClientObserverMock() override = default;

    void OnConnectionStateChanged(const RawAddress &device, int state, int oldState) override {}
};

class MicStateObserverMock : public MicStateObserver {
public:
    MicStateObserverMock() = default;
    ~MicStateObserverMock() override = default;

    void OnMicStateChanged(const RawAddress &device, uint8_t micState) override {}
};

namespace {
    MicClientObserverMock g_micServiceObserver_;
    MicStateObserverMock g_micStateObserver_;
    RawAddress g_mockDevice = RawAddress("00:11:22:33:44:55");
}

class NearlinkMicServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    // 测试目的：用例自持真实 MicService 实例（不依赖 vendor 栈启动链），随用例生命周期创建/销毁。
    std::unique_ptr<MicService> service_;
};

void NearlinkMicServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkMicServiceTest start");
    // MIC service tests own their service; vendor stack startup is an integration dependency.
    HILOGI("SetUpTestCase start NearlinkMicServiceTest end");
}

void NearlinkMicServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkMicServiceTest");
}

void NearlinkMicServiceTest::SetUp()
{
    HILOGI("SetUp NearlinkMicServiceTest.");
    // 测试目的：MIC 任务切到当前线程同步执行（去等待）；自建服务并直接写入注册表绕过 Start 链
    //（裸进程 slem_initialize 必败），最后用致命断言锁定"GetService 命中本实例"前提。
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_MIC, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    ResetMicStackMock();
    service_ = std::make_unique<MicService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_MIC, service_.get());
    ASSERT_EQ(service_.get(), MicService::GetService());
}

void NearlinkMicServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkMicServiceTest.");
    // 测试目的：先解绑注册表再销毁服务，避免 GetService 命中悬垂指针；最后恢复线程调度默认态。
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_MIC, nullptr);
    service_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
}

/**
 * @tc.number: GetService001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：fixture 注册表注入生效——GetService 必须返回非空服务实例（原 EXPECT 升级为致命断言）。
HWTEST_F(NearlinkMicServiceTest, GetService001, TestSize.Level1)
{
    HILOGI("GetService001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    HILOGI("GetService001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空再 Enable；NOT_SWITCH_THREAD 下使能同步完成，
// 原 sleep(50ms) 等待不再需要。
HWTEST_F(NearlinkMicServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    micService->Enable();
    micService->Enable();
    HILOGI("Enable001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空再 Disable；NOT_SWITCH_THREAD 下去使能同步完成，
// 原 sleep(50ms) 等待不再需要。
HWTEST_F(NearlinkMicServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    micService->Disable();
    micService->Disable();
    HILOGI("Disable001 end");
}

/*
 * @tc.number: RegisterObserver001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空（原 EXPECT 升级为致命）再注册观察者。
HWTEST_F(NearlinkMicServiceTest, RegisterObserver001, TestSize.Level1)
{
    HILOGI("RegisterObserver001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    micService->RegisterObserver(g_micServiceObserver_);
    HILOGI("RegisterObserver001 end");
}

/*
 * @tc.number: DeregisterObserver001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空再解注册观察者。
HWTEST_F(NearlinkMicServiceTest, DeregisterObserver001, TestSize.Level1)
{
    HILOGI("DeregisterObserver001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    micService->DeregisterObserver(g_micServiceObserver_);
    HILOGI("DeregisterObserver001 end");
}

/*
 * @tc.number: RegisterMicStateObserver001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空再注册麦克风状态观察者。
HWTEST_F(NearlinkMicServiceTest, RegisterMicStateObserver001, TestSize.Level1)
{
    HILOGI("RegisterMicStateObserver001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    micService->RegisterMicStateObserver(g_micStateObserver_);
    HILOGI("RegisterMicStateObserver001 end");
}

/*
 * @tc.number: DeregisterMicStateObserver001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空再解注册麦克风状态观察者。
HWTEST_F(NearlinkMicServiceTest, DeregisterMicStateObserver001, TestSize.Level1)
{
    HILOGI("DeregisterMicStateObserver001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    micService->DeregisterMicStateObserver(g_micStateObserver_);
    HILOGI("DeregisterMicStateObserver001 end");
}

/*
 * @tc.number: GetConnectState001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空再查连接态；未连接态服务固定返回 DISCONNECTED(3)。
HWTEST_F(NearlinkMicServiceTest, GetConnectState001, TestSize.Level1)
{
    HILOGI("GetConnectState001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    int connectState = micService->GetConnectState();
    EXPECT_EQ(3, connectState);
    HILOGI("GetConnectState001 end");
}

/*
 * @tc.number: GetConnectDevices001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：前提断言前置——服务指针先判空再查已连接设备列表；无连接返回空列表。
HWTEST_F(NearlinkMicServiceTest, GetConnectDevices001, TestSize.Level1)
{
    HILOGI("GetConnectDevices001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    std::list<RawAddress> connectedDevices = micService->GetConnectDevices();
    EXPECT_EQ(0, connectedDevices.size());
    HILOGI("GetConnectDevices001 end");
}

/*
 * @tc.number: Connect001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：Connect 返回成功且同步触达 NLSTK micp 连接桩一次（NOT_SWITCH_THREAD 下任务同步执行，
// 原 sleep(1000ms) 等待由栈桩计数断言替代）。
HWTEST_F(NearlinkMicServiceTest, Connect001, TestSize.Level1)
{
    HILOGI("Connect001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    int ret = micService->Connect(g_mockDevice);
    EXPECT_EQ(MIC_SUCCESS, ret);
    EXPECT_EQ(1, GetMicpConnectCalls());
    HILOGI("Connect001 end");
}

/*
 * @tc.number: Disconnect001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：Disconnect 返回成功且同步触达 NLSTK micp 断连桩一次，计数断言替代原固定 sleep 等待。
HWTEST_F(NearlinkMicServiceTest, Disconnect001, TestSize.Level1)
{
    HILOGI("Disconnect001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    int ret = micService->Disconnect(g_mockDevice);
    EXPECT_EQ(MIC_SUCCESS, ret);
    EXPECT_EQ(1, GetMicpDisconnectCalls());
    HILOGI("Disconnect001 end");
}

/**
 * @tc.number: UpdateMicState001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：连接后更新麦克风状态（含重复同状态分支）不崩溃；NOT_SWITCH_THREAD 下状态任务同步执行，
// 原 sleep(50ms) 等待不再需要。
HWTEST_F(NearlinkMicServiceTest, UpdateMicState001, TestSize.Level1)
{
    HILOGI("UpdateMicState001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::CONNECTED, SleConnectState::CONNECTING);
    micService->UpdateMicState(g_mockDevice, MIC_ON);
    // 校验状态相同时的逻辑
    micService->UpdateMicState(g_mockDevice, MIC_ON);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::DISCONNECTED, SleConnectState::DISCONNECTING);
    HILOGI("UpdateMicState001 end");
}

/**
 * @tc.number: IsMicOpen001
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：连接并把麦克风状态置为 MIC_ON 后 IsMicOpen 返回 true；断连后状态清除。
// NOT_SWITCH_THREAD 下通知与查询全部同步，原 sleep(50ms) 等待不再需要。
HWTEST_F(NearlinkMicServiceTest, IsMicOpen001, TestSize.Level1)
{
    HILOGI("IsMicOpen001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::CONNECTED, SleConnectState::CONNECTING);
    micService->UpdateMicState(g_mockDevice, MIC_ON);
    bool isMicOpen = micService->IsMicOpen(g_mockDevice);
    EXPECT_EQ(true, isMicOpen);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::DISCONNECTED, SleConnectState::DISCONNECTING);
    HILOGI("IsMicOpen001 end");
}

/*
 * @tc.number: NotifyStateChanged001
 * @tc.name:
 * @tc.desc:
*/
// 测试目的：仅连接（未更新麦克风状态）时 IsMicOpen 为 false（初始 MIC_UNKNOWN）；
// NOT_SWITCH_THREAD 下通知同步生效，原 sleep(50ms) 等待不再需要。
HWTEST_F(NearlinkMicServiceTest, NotifyStateChanged001, TestSize.Level1)
{
    HILOGI("NotifyStateChanged001 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::CONNECTED, SleConnectState::CONNECTING);
    bool isMicOpen = micService->IsMicOpen(g_mockDevice);
    EXPECT_EQ(false, isMicOpen);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::DISCONNECTED, SleConnectState::DISCONNECTING);
    HILOGI("NotifyStateChanged001 end");
}

/*
 * @tc.number: NotifyStateChanged002
 * @tc.name:
 * @tc.desc:
 */
// 测试目的：连接后断连，状态表被清除，IsMicOpen 为 false；同步执行下无需等待。
HWTEST_F(NearlinkMicServiceTest, NotifyStateChanged002, TestSize.Level1)
{
    HILOGI("NotifyStateChanged002 start");
    MicService* micService = MicService::GetService();
    ASSERT_NE(nullptr, micService);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::CONNECTED, SleConnectState::CONNECTING);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::DISCONNECTED, SleConnectState::DISCONNECTING);
    bool isMicOpen = micService->IsMicOpen(g_mockDevice);
    EXPECT_EQ(false, isMicOpen);
    HILOGI("NotifyStateChanged002 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
