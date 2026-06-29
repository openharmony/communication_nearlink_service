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
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "DeviceBatteryManager.h"
#include "SleUtils.h"
#include "log.h"
#include "TwsHiBoxParser.h"


namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing;
using namespace testing::ext;

constexpr int DELAY_LITTLE_MS = 100;

class NearlinkBattaryManagerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkBattaryManagerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkBattaryManagerTest");
}

void NearlinkBattaryManagerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkBattaryManagerTest");
}

void NearlinkBattaryManagerTest::SetUp()
{
    HILOGI("SetUp NearlinkBattaryManagerTest.");
}

void NearlinkBattaryManagerTest::TearDown()
{
    HILOGI("TearDown NearlinkBattaryManagerTest.");
}

/**
 * @tc.name: PublishBatteryLevel001
 * @tc.desc: Test the PublishBatteryLevel function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, PublishBatteryLevel001, TestSize.Level1)
{
    HILOGI("NearlinkBattaryManagerTest:PublishBatteryLevel001 start");
    std::string atCmdParam = "8,4,84,5,0,2,81,3,1,6,1,7,0,9,11,12,256";
    RawAddress peerAddr("11:22:33:44:55:66");
    BatteryInfo batteryInfo {};
    bool ret = TwsHiBoxParser::ParserAtCmdVendorBattery(peerAddr, atCmdParam, batteryInfo);
    EXPECT_EQ(ret, true);
    DeviceBatteryManager::GetInstance().PublishBatteryLevel(peerAddr, batteryInfo);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkBattaryManagerTest:PublishBatteryLevel001 end");
}

/**
 * @tc.name: PublishBatteryLevel002
 * @tc.desc: Test the PublishBatteryLevel function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, PublishBatteryLevel002, TestSize.Level1)
{
    HILOGI("NearlinkBattaryManagerTest:PublishBatteryLevel002 start");
    std::string atCmdParam = "7,4,84,5,0,2,81,3,1,6,1,7,0,9,11";
    RawAddress peerAddr("11:22:33:44:55:66");
    BatteryInfo batteryInfo {};
    bool ret = TwsHiBoxParser::ParserAtCmdVendorBattery(peerAddr, atCmdParam, batteryInfo);
    EXPECT_EQ(ret, true);
    DeviceBatteryManager::GetInstance().PublishBatteryLevel(peerAddr, batteryInfo);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkBattaryManagerTest:PublishBatteryLevel002 end");
}

/**
 * @tc.name: ProcessProfileStateChanged001
 * @tc.desc: Test the ProcessProfileStateChanged function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, ProcessProfileStateChanged001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:Disconnect001 start");
    RawAddress reportAddr("11:22:33:44:55:66");
    int newConnState = static_cast<int>(SleConnectState::CONNECTED);
    DeviceBatteryManager::GetInstance().ProcessProfileStateChanged(reportAddr.GetAddress(), newConnState);
    DeviceBatteryManager::GetInstance().ProcessProfileStateChanged(reportAddr.GetAddress(), newConnState);

    std::string emptyAddr = "";
    DeviceBatteryManager::GetInstance().ProcessProfileStateChanged(emptyAddr, newConnState);

    RawAddress otherAddr("11:22:33:44:55:77");
    newConnState = static_cast<int>(SleConnectState::CONNECTING);
    DeviceBatteryManager::GetInstance().ProcessProfileStateChanged(otherAddr.GetAddress(), newConnState);
    newConnState = static_cast<int>(SleConnectState::CONNECTED);
    DeviceBatteryManager::GetInstance().ProcessProfileStateChanged(otherAddr.GetAddress(), newConnState);

    DeviceBatteryManager::GetInstance().ProcessActiveDeviceChanged(reportAddr.GetAddress());
    newConnState = static_cast<int>(SleConnectState::DISCONNECTED);
    DeviceBatteryManager::GetInstance().ProcessProfileStateChanged(reportAddr.GetAddress(), newConnState);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:Disconnect001 end");
}

/**
 * @tc.name: ProcessActiveDeviceChanged001
 * @tc.desc: Test the ProcessActiveDeviceChanged function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, ProcessActiveDeviceChanged001, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:Enable001 start");

    RawAddress reportAddr("11:22:33:44:55:66");
    DeviceBatteryManager::GetInstance().ProcessActiveDeviceChanged(reportAddr.GetAddress());

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:Enable001 end");
}

/**
 * @tc.name: ProcessActiveDeviceChanged002
 * @tc.desc: Test the ProcessActiveDeviceChanged function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, ProcessActiveDeviceChanged002, TestSize.Level1)
{
    HILOGI("NearlinkCdsmTest:Enable001 start");

    RawAddress reportAddr("");
    DeviceBatteryManager::GetInstance().ProcessActiveDeviceChanged(reportAddr.GetAddress());

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkCdsmTest:Enable001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
