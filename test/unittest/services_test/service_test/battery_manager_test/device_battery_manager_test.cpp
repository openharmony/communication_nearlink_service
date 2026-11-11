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
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "nearlink_native_token_mock.h"
#include "nearlink_access_token_mock.h"
#include "DeviceBatteryManager.h"
#include "SleRemoteDeviceManager.h"
#include "sle_service_data.h"
#include "IRemoteDeviceQuery.h"
#include "SleUtils.h"
#include "log.h"
#include "TwsMessage.h"
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

/**
 * @tc.name: PublishBatteryLevel_TwsNull_001
 * @tc.desc: PublishBatteryLevel with non-audio device, should return early without crash.
 *           Note: IsAudioDevice returns false for unknown addresses, so the TWS null path
 *           (line 53 NL_CHECK_RETURN) is not reached. This test covers the early-return path.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, PublishBatteryLevel_TwsNull_001, TestSize.Level1)
{
    HILOGI("NearlinkBattaryManagerTest:PublishBatteryLevel_TwsNull_001 start");
    std::string atCmdParam = "8,4,84,5,0,2,81,3,1,6,1,7,0,9,11,12,256";
    RawAddress peerAddr("11:22:33:44:55:66");
    BatteryInfo batteryInfo {};
    bool ret = TwsHiBoxParser::ParserAtCmdVendorBattery(peerAddr, atCmdParam, batteryInfo);
    EXPECT_EQ(ret, true);
    EXPECT_FALSE(IRemoteDeviceQuery::GetInstance()->IsAudioDevice(peerAddr.GetAddress()));
    DeviceBatteryManager::GetInstance().PublishBatteryLevel(peerAddr, batteryInfo);

    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkBattaryManagerTest:PublishBatteryLevel_TwsNull_001 end");
}

/**
 * @tc.name: BatteryInfo_DefaultValueCompat_001
 * @tc.desc: Verify BatteryInfo migrated to TwsDefines.h has default member values (= 0)
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, BatteryInfo_DefaultValueCompat_001, TestSize.Level1)
{
    HILOGI("NearlinkBattaryManagerTest:BatteryInfo_DefaultValueCompat_001 start");
    BatteryInfo batteryInfo {};
    EXPECT_EQ(batteryInfo.devBattery_, 0);
    EXPECT_EQ(batteryInfo.leftBattery_, 0);
    EXPECT_EQ(batteryInfo.leftCharge_, 0);
    EXPECT_EQ(batteryInfo.rightBattery_, 0);
    EXPECT_EQ(batteryInfo.rightCharge_, 0);
    EXPECT_EQ(batteryInfo.boxBattery_, 0);
    EXPECT_EQ(batteryInfo.boxCharge_, 0);
    EXPECT_EQ(batteryInfo.earErrCode_, 0);
    EXPECT_EQ(batteryInfo.boxOpen_, 0);
    EXPECT_EQ(batteryInfo.leftModel_, 0);
    EXPECT_EQ(batteryInfo.rightModel_, 0);
    EXPECT_EQ(batteryInfo.dialogState_, 0);
    EXPECT_EQ(batteryInfo.earStatus_, 0);
    HILOGI("NearlinkBattaryManagerTest:BatteryInfo_DefaultValueCompat_001 end");
}

/**
 * @tc.name: BatteryInfo_OffsetCompat_001
 * @tc.desc: Verify BatteryInfo member offsets are unchanged after migration out of #pragma pack(1)
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, BatteryInfo_OffsetCompat_001, TestSize.Level1)
{
    HILOGI("NearlinkBattaryManagerTest:BatteryInfo_OffsetCompat_001 start");
    BatteryInfo batteryInfo {};
    EXPECT_EQ(offsetof(BatteryInfo, devBattery_), 0);
    EXPECT_EQ(offsetof(BatteryInfo, leftBattery_), 1);
    EXPECT_EQ(offsetof(BatteryInfo, leftCharge_), 2);
    EXPECT_EQ(offsetof(BatteryInfo, rightBattery_), 3);
    EXPECT_EQ(offsetof(BatteryInfo, rightCharge_), 4);
    EXPECT_EQ(offsetof(BatteryInfo, boxBattery_), 5);
    EXPECT_EQ(offsetof(BatteryInfo, boxCharge_), 6);
    EXPECT_EQ(offsetof(BatteryInfo, earErrCode_), 7);
    EXPECT_EQ(offsetof(BatteryInfo, boxOpen_), 8);
    EXPECT_EQ(offsetof(BatteryInfo, leftModel_), 9);
    EXPECT_EQ(offsetof(BatteryInfo, rightModel_), 10);
    EXPECT_EQ(offsetof(BatteryInfo, dialogState_), 11);
    EXPECT_EQ(offsetof(BatteryInfo, earStatus_), 12);
    HILOGI("NearlinkBattaryManagerTest:BatteryInfo_OffsetCompat_001 end");
}

/**
 * @tc.name: BatteryInfo_SizeofCompat_001
 * @tc.desc: Verify sizeof(BatteryInfo) is 13 (13 x uint8_t) after migration out of #pragma pack(1)
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, BatteryInfo_SizeofCompat_001, TestSize.Level1)
{
    HILOGI("NearlinkBattaryManagerTest:BatteryInfo_SizeofCompat_001 start");
    EXPECT_EQ(sizeof(BatteryInfo), 13);
    HILOGI("NearlinkBattaryManagerTest:BatteryInfo_SizeofCompat_001 end");
}


/**
 * @tc.name: BatteryCalcAndCurrent_001
 * @tc.desc: 电量上报值取双耳较低值，仅一只耳有电量时取该值，双耳无电量取常规电量，全无效为 -1；
 *           当前电量设备优先取支持电量的激活设备，激活无效时取最近连接的支持电量设备
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, BatteryCalcAndCurrent_001, TestSize.Level1)
{
    auto &mgr = DeviceBatteryManager::GetInstance();
    BatteryInfo bothSides = {};
    bothSides.leftBattery_ = 60;
    bothSides.rightBattery_ = 80;
    EXPECT_EQ(60, mgr.CalculateBatteryLevel(bothSides));
    BatteryInfo singleSide = {};
    singleSide.leftBattery_ = 90;
    EXPECT_EQ(90, mgr.CalculateBatteryLevel(singleSide));
    BatteryInfo devOnly = {};
    devOnly.devBattery_ = 70;
    EXPECT_EQ(70, mgr.CalculateBatteryLevel(devOnly));
    BatteryInfo nothing = {};
    EXPECT_EQ(-1, mgr.CalculateBatteryLevel(nothing));

    mgr.connectedDevices_ = {{"A1:B2:C3:D4:E5:21", false}, {"A1:B2:C3:D4:E5:22", true}};
    mgr.activeDevice_ = "A1:B2:C3:D4:E5:22";
    EXPECT_EQ("A1:B2:C3:D4:E5:22", mgr.GetCurrentBatteryDevice(true));
    mgr.activeDevice_ = "A1:B2:C3:D4:E5:21";
    EXPECT_EQ("A1:B2:C3:D4:E5:22", mgr.GetCurrentBatteryDevice(true));
    EXPECT_EQ("A1:B2:C3:D4:E5:21", mgr.GetLastConnectedDevice(false));
    mgr.activeDevice_ = "";
    EXPECT_EQ("A1:B2:C3:D4:E5:22", mgr.GetCurrentBatteryDevice(true));
    mgr.connectedDevices_.clear();
    EXPECT_EQ("", mgr.GetCurrentBatteryDevice(true));
}

/**
 * @tc.name: BatteryNotify_001
 * @tc.desc: 电量通知只对当前电量设备生效且电量变化才上报；设备变更通知跳过
 *           非电量设备与未变化设备，双无效电量只切换当前设备不上报
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, BatteryNotify_001, TestSize.Level1)
{
    auto &mgr = DeviceBatteryManager::GetInstance();
    mgr.connectedDevices_ = {{"A1:B2:C3:D4:E5:21", true}, {"A1:B2:C3:D4:E5:22", true}};
    mgr.currentBatteryDevice_ = "A1:B2:C3:D4:E5:21";
    mgr.reportBatteryLevel_ = 50;
    BatteryInfo info = {};
    info.leftBattery_ = 60;
    mgr.batteryDeviceMap_["A1:B2:C3:D4:E5:21"] = info;

    mgr.NotifyForBatteryInfoChanged("A1:B2:C3:D4:E5:22", 60);
    EXPECT_EQ(50, mgr.reportBatteryLevel_);
    mgr.NotifyForBatteryInfoChanged("A1:B2:C3:D4:E5:21", 50);
    EXPECT_EQ(50, mgr.reportBatteryLevel_);
    mgr.NotifyForBatteryInfoChanged("A1:B2:C3:D4:E5:21", 60);
    EXPECT_EQ(60, mgr.reportBatteryLevel_);

    mgr.NotifyForDeviceChanged("");
    EXPECT_EQ("A1:B2:C3:D4:E5:21", mgr.currentBatteryDevice_);
    mgr.NotifyForDeviceChanged("A1:B2:C3:D4:E5:31");
    EXPECT_EQ("A1:B2:C3:D4:E5:21", mgr.currentBatteryDevice_);
    mgr.NotifyForDeviceChanged("A1:B2:C3:D4:E5:21");
    EXPECT_EQ("A1:B2:C3:D4:E5:21", mgr.currentBatteryDevice_);
    mgr.reportBatteryLevel_ = -1;
    mgr.NotifyForDeviceChanged("A1:B2:C3:D4:E5:22");
    EXPECT_EQ("A1:B2:C3:D4:E5:22", mgr.currentBatteryDevice_);
    EXPECT_EQ(-1, mgr.reportBatteryLevel_);
}

/**
 * @tc.name: BatteryAddUpdate_001
 * @tc.desc: 已连接设备重复添加时移到队尾并重置电量标记；已有电量记录的设备
 *           重新添加时立即成为当前电量设备
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, BatteryAddUpdate_001, TestSize.Level1)
{
    auto &mgr = DeviceBatteryManager::GetInstance();
    mgr.connectedDevices_.clear();
    mgr.batteryDeviceMap_.clear();
    mgr.currentBatteryDevice_ = "";
    mgr.reportBatteryLevel_ = -1;

    mgr.AddConnectedDevice("A1:B2:C3:D4:E5:21");
    mgr.AddConnectedDevice("A1:B2:C3:D4:E5:22");
    ASSERT_EQ(2U, mgr.connectedDevices_.size());
    mgr.UpdateBatteryStatus("A1:B2:C3:D4:E5:21", true);
    EXPECT_TRUE(mgr.FindConnectedDevice("A1:B2:C3:D4:E5:21").second);

    mgr.AddConnectedDevice("A1:B2:C3:D4:E5:21");
    ASSERT_EQ(2U, mgr.connectedDevices_.size());
    EXPECT_EQ("A1:B2:C3:D4:E5:21", mgr.connectedDevices_.back().first);
    EXPECT_FALSE(mgr.connectedDevices_.back().second);

    mgr.UpdateBatteryStatus("A1:B2:C3:D4:E5:21", true);
    mgr.currentBatteryDevice_ = "";
    BatteryInfo info = {};
    info.leftBattery_ = 40;
    mgr.batteryDeviceMap_["A1:B2:C3:D4:E5:21"] = info;
    mgr.AddConnectedDevice("A1:B2:C3:D4:E5:21");
    EXPECT_EQ("A1:B2:C3:D4:E5:21", mgr.currentBatteryDevice_);
}

/**
 * @tc.name: BatteryDisconnect_001
 * @tc.desc: 档案断开清理电量记录与连接表项并清空激活设备；非法地址直接忽略
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, BatteryDisconnect_001, TestSize.Level1)
{
    auto &mgr = DeviceBatteryManager::GetInstance();
    mgr.connectedDevices_ = {{"A1:B2:C3:D4:E5:21", true}};
    mgr.batteryDeviceMap_ = {{"A1:B2:C3:D4:E5:21", BatteryInfo()}};
    mgr.activeDevice_ = "A1:B2:C3:D4:E5:21";

    mgr.ProcessProfileDisconnected("not-an-addr");
    EXPECT_EQ(1U, mgr.connectedDevices_.size());

    mgr.ProcessProfileDisconnected("A1:B2:C3:D4:E5:21");
    EXPECT_TRUE(mgr.connectedDevices_.empty());
    EXPECT_TRUE(mgr.batteryDeviceMap_.empty());
    EXPECT_EQ("", mgr.activeDevice_);
}

/**
 * @tc.name: PublishBatteryLevel003
 * @tc.desc: 音频标记设备因 TWS 档案未启动不落电量记录；非音频设备直接忽略
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkBattaryManagerTest, PublishBatteryLevel003, TestSize.Level1)
{
    auto &mgr = DeviceBatteryManager::GetInstance();
    mgr.batteryDeviceMap_.clear();

    auto audioDev = std::make_shared<SlePeripheralDevice>();
    audioDev->SetAddress(RawAddress("A1:B2:C3:D4:E5:21"));
    audioDev->SetIsAudioDeviceFlag(true);
    SleRemoteDeviceManager::GetInstance()->AddPeripheralDevice("A1:B2:C3:D4:E5:21", audioDev);
    mgr.PublishBatteryLevel(RawAddress("A1:B2:C3:D4:E5:21"), BatteryInfo());
    EXPECT_TRUE(mgr.batteryDeviceMap_.empty());

    mgr.PublishBatteryLevel(RawAddress("B2:C3:D4:E5:F6:32"), BatteryInfo());
    EXPECT_TRUE(mgr.batteryDeviceMap_.empty());

    SleRemoteDeviceManager::GetInstance()->RemovePeripheralDevice("A1:B2:C3:D4:E5:21");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
