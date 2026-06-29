/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "log.h"
#include "BasService.h"
#include "BasClientStackAdapter.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceProfileManager.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

constexpr int DELAY_LITTLE_MS = 100;

class BasServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void BasServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

void BasServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
    SleInterfaceManager::GetInstance()->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

void BasServiceTest::SetUp()
{
    HILOGI("SetUp start");
}

void BasServiceTest::TearDown()
{
    HILOGI("TearDown start");
}

/**
 * @tc.number: BasServiceTest001
 * @tc.name: Test BasService GetInstance
 * @tc.desc: Test BasService singleton get instance
 */
HWTEST_F(BasServiceTest, BasServiceTest001, TestSize.Level1)
{
    HILOGI("BasServiceTest001 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);
}

/**
 * @tc.number: BasServiceTest002
 * @tc.name: Test BasService Enable and Disable
 * @tc.desc: Test BasService enable and disable flow
 */
HWTEST_F(BasServiceTest, BasServiceTest002, TestSize.Level1)
{
    HILOGI("BasServiceTest002 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    utility::Context *context = basService->GetContext();
    ASSERT_NE(context, nullptr);

    basService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

/**
 * @tc.number: BasServiceTest003
 * @tc.name: Test BasService Connect
 * @tc.desc: Test BasService connect to device
 */
HWTEST_F(BasServiceTest, BasServiceTest003, TestSize.Level1)
{
    HILOGI("BasServiceTest003 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    std::string addr = "00:11:22:33:44:55";
    RawAddress rawAddress(addr);

    basService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    int ret = basService->Connect(rawAddress);
    EXPECT_EQ(ret, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

/**
 * @tc.number: BasServiceTest004
 * @tc.name: Test BasService Disconnect
 * @tc.desc: Test BasService disconnect from device
 */
HWTEST_F(BasServiceTest, BasServiceTest004, TestSize.Level1)
{
    HILOGI("BasServiceTest004 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    std::string addr = "00:11:22:33:44:55";
    RawAddress rawAddress(addr);

    basService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    int ret = basService->Disconnect(rawAddress);
    EXPECT_EQ(ret, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

/**
 * @tc.number: BasServiceTest005
 * @tc.name: Test BasService GetDeviceBatteryLevel
 * @tc.desc: Test BasService get device battery level
 */
HWTEST_F(BasServiceTest, BasServiceTest005, TestSize.Level1)
{
    HILOGI("BasServiceTest005 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    std::string addr = "00:11:22:33:44:55";
    RawAddress rawAddress(addr);

    basService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->GetDeviceBatteryLevel(rawAddress);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

/**
 * @tc.number: BasServiceTest006
 * @tc.name: Test BasService GetConnectDevices
 * @tc.desc: Test BasService get connected devices list
 */
HWTEST_F(BasServiceTest, BasServiceTest006, TestSize.Level1)
{
    HILOGI("BasServiceTest006 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    basService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    std::list<RawAddress> devices = basService->GetConnectDevices();
    EXPECT_GE(devices.size(), 0);

    basService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

/**
 * @tc.number: BasServiceTest007
 * @tc.name: Test BasService GetConnectState
 * @tc.desc: Test BasService get connect state
 */
HWTEST_F(BasServiceTest, BasServiceTest007, TestSize.Level1)
{
    HILOGI("BasServiceTest007 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    basService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    int state = basService->GetConnectState();
    EXPECT_EQ(state, static_cast<int>(SleConnectState::DISCONNECTED));

    basService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

/**
 * @tc.number: BasServiceTest008
 * @tc.name: Test BasObserver Register and Deregister
 * @tc.desc: Test BasObserver registration
 */
class TestBasObserver : public BasObserver {
public:
    TestBasObserver() {}
    virtual ~TestBasObserver() {}
    void OnConnectionStateChanged(
        const RawAddress &addr, int32_t state, int32_t preState) override
    {
        HILOGI("OnConnectionStateChanged called");
    }
};

HWTEST_F(BasServiceTest, BasServiceTest008, TestSize.Level1)
{
    HILOGI("BasServiceTest008 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    TestBasObserver observer;
    basService->RegisterObserver(observer);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->DeregisterObserver(observer);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

/**
 * @tc.number: BasServiceTest009
 * @tc.name: Test IDeviceBatteryCallback Register and Deregister
 * @tc.desc: Test IDeviceBatteryCallback registration
 */
class TestDeviceBatteryObserver : public IDeviceBatteryCallback {
public:
    TestDeviceBatteryObserver() {}
    virtual ~TestDeviceBatteryObserver() {}
    void OnGetBatteryLevelEvent(const RawAddress &addr, int8_t batteryLevel) override
    {
        HILOGI("OnGetBatteryLevelEvent called");
    }
    void OnBatteryLevelChanged(const RawAddress &addr, int8_t batteryLevel) override
    {
        HILOGI("OnBatteryLevelChanged called");
    }
};

HWTEST_F(BasServiceTest, BasServiceTest009, TestSize.Level1)
{
    HILOGI("BasServiceTest009 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    TestDeviceBatteryObserver observer;
    basService->RegisterDeviceObserver(observer);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->DeregisterDeviceObserver(observer);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

/**
 * @tc.number: BasServiceTest010
 * @tc.name: Test BasService NotifyStateChanged
 * @tc.desc: Test BasService notify state changed
 */
HWTEST_F(BasServiceTest, BasServiceTest010, TestSize.Level1)
{
    HILOGI("BasServiceTest010 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    std::string addr = "00:11:22:33:44:55";
    RawAddress rawAddress(addr);

    TestBasObserver observer;
    basService->RegisterObserver(observer);

    basService->NotifyStateChanged(
        rawAddress, SleConnectState::CONNECTED, SleConnectState::CONNECTING);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->DeregisterObserver(observer);
}

/**
 * @tc.number: BasServiceTest011
 * @tc.name: Test BasService NotifyBatteryLevelEvent
 * @tc.desc: Test BasService notify battery level event
 */
HWTEST_F(BasServiceTest, BasServiceTest011, TestSize.Level1)
{
    HILOGI("BasServiceTest011 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    std::string addr = "00:11:22:33:44:55";
    RawAddress rawAddress(addr);

    TestDeviceBatteryObserver observer;
    basService->RegisterDeviceObserver(observer);

    basService->NotifyBatteryLevelEvent(rawAddress, 80);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->DeregisterDeviceObserver(observer);
}

/**
 * @tc.number: BasServiceTest012
 * @tc.name: Test BasService NotifyBatteryLevelChanged
 * @tc.desc: Test BasService notify battery level changed
 */
HWTEST_F(BasServiceTest, BasServiceTest012, TestSize.Level1)
{
    HILOGI("BasServiceTest012 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    std::string addr = "00:11:22:33:44:55";
    RawAddress rawAddress(addr);

    TestDeviceBatteryObserver observer;
    basService->RegisterDeviceObserver(observer);

    basService->NotifyBatteryLevelChanged(rawAddress, 60);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->DeregisterDeviceObserver(observer);
}

/**
 * @tc.number: BasServiceTest013
 * @tc.name: Test BasClientStackAdapter GetInstance
 * @tc.desc: Test BasClientStackAdapter basic function
 */
HWTEST_F(BasServiceTest, BasServiceTest013, TestSize.Level1)
{
    HILOGI("BasServiceTest013 start");
    BasClientStackAdapter adapter;
    int ret = adapter.RegisterCallBackToStack();
    EXPECT_GE(ret, 0);
}

/**
 * @tc.number: BasServiceTest016
 * @tc.name: Test ProfileBas::GetInstance
 * @tc.desc: Test ProfileBas singleton get instance
 */
HWTEST_F(BasServiceTest, BasServiceTest016, TestSize.Level1)
{
    HILOGI("BasServiceTest016 start");
    ProfileBas *profileBas = ProfileBas::GetInstance();
    ASSERT_NE(profileBas, nullptr);
}

/**
 * @tc.number: BasServiceTest017
 * @tc.name: Test BasService Enable twice
 * @tc.desc: Test BasService enable twice should not crash
 */
HWTEST_F(BasServiceTest, BasServiceTest017, TestSize.Level1)
{
    HILOGI("BasServiceTest017 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    basService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}

/**
 * @tc.number: BasServiceTest018
 * @tc.name: Test BasService Disable twice
 * @tc.desc: Test BasService disable twice should not crash
 */
HWTEST_F(BasServiceTest, BasServiceTest018, TestSize.Level1)
{
    HILOGI("BasServiceTest018 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    basService->Enable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));

    basService->Disable();
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
}
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS