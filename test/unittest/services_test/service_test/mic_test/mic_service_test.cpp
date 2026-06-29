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
#include "nearlink_access_token_mock.h"
#include "MicService.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "log.h"
#include "MicClientStackAdapter.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
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
    constexpr int MIC_SERVICE_UT_DELAY_50_MS = 50;
    constexpr int MIC_SERVICE_UT_DELAY_1000_MS = 1000;
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
};

void NearlinkMicServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkMicServiceTest start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_1000_MS));
    HILOGI("SetUpTestCase start NearlinkMicServiceTest end");
}

void NearlinkMicServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkMicServiceTest");
}

void NearlinkMicServiceTest::SetUp()
{
    HILOGI("SetUp NearlinkMicServiceTest.");
}

void NearlinkMicServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkMicServiceTest.");
}

/**
 * @tc.number: GetService001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, GetService001, TestSize.Level1)
{
    HILOGI("GetService001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    HILOGI("GetService001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    MicService* micService = MicService::GetService();
    micService->Enable();
    EXPECT_NE(nullptr, micService);

    micService->Enable();
    EXPECT_NE(nullptr, micService);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    HILOGI("Enable001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    MicService* micService = MicService::GetService();
    micService->Disable();
    EXPECT_NE(nullptr, micService);

    micService->Disable();
    EXPECT_NE(nullptr, micService);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    HILOGI("Disable001 end");
}

/*
 * @tc.number: RegisterObserver001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, RegisterObserver001, TestSize.Level1)
{
    HILOGI("RegisterObserver001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    micService->RegisterObserver(g_micServiceObserver_);
    EXPECT_NE(nullptr, micService);
    HILOGI("RegisterObserver001 end");
}

/*
 * @tc.number: DeregisterObserver001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, DeregisterObserver001, TestSize.Level1)
{
    HILOGI("DeregisterObserver001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    micService->DeregisterObserver(g_micServiceObserver_);
    EXPECT_NE(nullptr, micService);
    HILOGI("DeregisterObserver001 end");
}

/*
 * @tc.number: RegisterMicStateObserver001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, RegisterMicStateObserver001, TestSize.Level1)
{
    HILOGI("RegisterMicStateObserver001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    micService->RegisterMicStateObserver(g_micStateObserver_);
    EXPECT_NE(nullptr, micService);
    HILOGI("RegisterMicStateObserver001 end");
}

/*
 * @tc.number: DeregisterMicStateObserver001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, DeregisterMicStateObserver001, TestSize.Level1)
{
    HILOGI("DeregisterMicStateObserver001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    micService->DeregisterMicStateObserver(g_micStateObserver_);
    EXPECT_NE(nullptr, micService);
    HILOGI("DeregisterMicStateObserver001 end");
}

/*
 * @tc.number: GetConnectState001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, GetConnectState001, TestSize.Level1)
{
    HILOGI("GetConnectState001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    int connectState = micService->GetConnectState();
    EXPECT_EQ(3, connectState);
    HILOGI("GetConnectState001 end");
}

/*
 * @tc.number: GetConnectDevices001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, GetConnectDevices001, TestSize.Level1)
{
    HILOGI("GetConnectDevices001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    std::list<RawAddress> connectedDevices = micService->GetConnectDevices();
    EXPECT_EQ(0, connectedDevices.size());
    HILOGI("GetConnectDevices001 end");
}

/*
 * @tc.number: Connect001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, Connect001, TestSize.Level1)
{
    HILOGI("Connect001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    int ret = micService->Connect(g_mockDevice);
    EXPECT_EQ(MIC_SUCCESS, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_1000_MS));
    HILOGI("Connect001 end");
}

/*
 * @tc.number: Disconnect001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, Disconnect001, TestSize.Level1)
{
    HILOGI("Disconnect001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    int ret = micService->Disconnect(g_mockDevice);
    EXPECT_EQ(MIC_SUCCESS, ret);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_1000_MS));
    HILOGI("Disconnect001 end");
}

/**
 * @tc.number: UpdateMicState001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, UpdateMicState001, TestSize.Level1)
{
    HILOGI("UpdateMicState001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::CONNECTED, SleConnectState::CONNECTING);
    micService->UpdateMicState(g_mockDevice, MIC_ON);
    // 校验状态相同时的逻辑
    micService->UpdateMicState(g_mockDevice, MIC_ON);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::DISCONNECTED, SleConnectState::DISCONNECTING);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    HILOGI("UpdateMicState001 end");
}

/**
 * @tc.number: IsMicOpen001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, IsMicOpen001, TestSize.Level1)
{
    HILOGI("IsMicOpen001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::CONNECTED, SleConnectState::CONNECTING);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    micService->UpdateMicState(g_mockDevice, MIC_ON);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    bool isMicOpen = micService->IsMicOpen(g_mockDevice);
    EXPECT_EQ(true, isMicOpen);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::DISCONNECTED, SleConnectState::DISCONNECTING);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    HILOGI("IsMicOpen001 end");
}

/*
 * @tc.number: NotifyStateChanged001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkMicServiceTest, NotifyStateChanged001, TestSize.Level1)
{
    HILOGI("NotifyStateChanged001 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::CONNECTED, SleConnectState::CONNECTING);
    bool isMicOpen = micService->IsMicOpen(g_mockDevice);
    EXPECT_EQ(false, isMicOpen);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::DISCONNECTED, SleConnectState::DISCONNECTING);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    HILOGI("NotifyStateChanged001 end");
}

/*
 * @tc.number: NotifyStateChanged002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkMicServiceTest, NotifyStateChanged002, TestSize.Level1)
{
    HILOGI("NotifyStateChanged002 start");
    MicService* micService = MicService::GetService();
    EXPECT_NE(nullptr, micService);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::CONNECTED, SleConnectState::CONNECTING);
    micService->NotifyStateChanged(g_mockDevice, SleConnectState::DISCONNECTED, SleConnectState::DISCONNECTING);
    bool isMicOpen = micService->IsMicOpen(g_mockDevice);
    EXPECT_EQ(false, isMicOpen);
    std::this_thread::sleep_for(std::chrono::milliseconds(MIC_SERVICE_UT_DELAY_50_MS));
    HILOGI("NotifyStateChanged002 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
