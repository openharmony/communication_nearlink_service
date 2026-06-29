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

#include "SleCloudPairService.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceAdapterSub.h"
#include "nearlink_verification_manager.h"
#include "SleRemoteDeviceAdapter.h"

#include "SleConfig.h"
#include "log.h"
#include <thread>

namespace OHOS {
namespace Nearlink {
namespace TEST {

using namespace testing::ext;

const char* DEVICE_SLE_REPORT_ADDR = "11:22:33:44:55:66";
const char* DEVICE_SLE_MEMBER_ADDR = "AA:BB:CC:DD:EE:FF";
const char* DEVICE_BT_ADDR = "12:34:56:78:9A:BC";
const char* DEVICE_NAME = "cloudDevice";
const char* DEVICE_MODEL = "cloudDeviceModel";
const char* DEVICE_SUBMODEL_ID = "cloudDeviceSubModel";
const char* DEVICE_ICON_ID = "1234";
const std::vector<uint8_t> DEVICE_TOKEN =
    {0x90, 0x4D, 0xA3, 0x60, 0x73, 0x0A, 0x3B, 0x27, 0x70, 0xDA, 0xC4, 0x07, 0x94, 0xD7, 0xF7, 0x79,
     0x58, 0x85, 0x17, 0x4D, 0x63, 0x33, 0xF5, 0xA2, 0xFA, 0xF6, 0x7F, 0x40, 0x5C, 0xEB, 0xFE, 0x75};

class SleCloudPairServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    static void InitConf();
    void ProcDownCloudDevice(const std::string deviceName, const std::vector<uint8_t> deviceToken);
};

void SleCloudPairServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase SleCloudPairServiceTest.");
    SleConfig::GetInstance().LoadConfigInfo();
    InitConf();
    NearlinkVerificationManager::GetInstance().LoadStrategies();
}

void SleCloudPairServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase SleCloudPairServiceTest");
}

void SleCloudPairServiceTest::SetUp()
{
    HILOGI("SetUp SleCloudPairServiceTest.");
    SleCloudPairService::GetInstance().Init();
}

void SleCloudPairServiceTest::TearDown()
{
    HILOGI("TearDown SleCloudPairServiceTest.");
    SleConfig::GetInstance().RemoveAllCloudDevice();
    SleConfig::GetInstance().RemoveCdsmGroup(DEVICE_SLE_REPORT_ADDR);
    SleConfig::GetInstance().Save();
    SleCloudPairService::GetInstance().cloudDevicesMap_.Clear();
}

void SleCloudPairServiceTest::InitConf()
{
    HILOGI("SleCloudPairServiceTest Write Cloud Device Info to SleConfig.");
    SleConfig::GetInstance().SetCloudDeviceBtAddr(DEVICE_SLE_REPORT_ADDR, DEVICE_BT_ADDR);
    SleConfig::GetInstance().SetCloudDeviceName(DEVICE_SLE_REPORT_ADDR, DEVICE_NAME);
    SleConfig::GetInstance().SetCloudDeviceToken(DEVICE_SLE_REPORT_ADDR, DEVICE_TOKEN);
    SleConfig::GetInstance().SetCloudDeviceReportAddr(DEVICE_SLE_REPORT_ADDR, DEVICE_SLE_REPORT_ADDR);
    SleConfig::GetInstance().SetCloudDeviceMembersAddrList(DEVICE_SLE_REPORT_ADDR,
        {DEVICE_SLE_REPORT_ADDR, DEVICE_SLE_MEMBER_ADDR});
    SleConfig::GetInstance().SetCloudDeviceModel(DEVICE_SLE_REPORT_ADDR, DEVICE_MODEL);
    SleConfig::GetInstance().SetCloudDeviceSubModelId(DEVICE_SLE_REPORT_ADDR, DEVICE_SUBMODEL_ID);
    SleConfig::GetInstance().SetCloudDeviceIconId(DEVICE_SLE_REPORT_ADDR, DEVICE_ICON_ID);
    SleConfig::GetInstance().SetCloudDeviceState(DEVICE_SLE_REPORT_ADDR, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
    SleConfig::GetInstance().Save();
}

void SleCloudPairServiceTest::ProcDownCloudDevice(const std::string deviceName, const std::vector<uint8_t> deviceToken)
{
    HILOGI("SleCloudPairServiceTest Down Cloud Device.");
    NearlinkCloudPairDevice dev;
    dev.SetBtAddr(DEVICE_BT_ADDR);
    dev.SetDeviceName(deviceName);
    dev.SetToken(deviceToken);
    dev.SetReportAddr(DEVICE_SLE_REPORT_ADDR);
    dev.SetMembersAddr({DEVICE_SLE_REPORT_ADDR, DEVICE_SLE_MEMBER_ADDR});
    dev.SetModel(DEVICE_MODEL);
    dev.SetSubModelId(DEVICE_SUBMODEL_ID);
    dev.SetDeviceIconId(DEVICE_ICON_ID);
    std::vector<NearlinkCloudPairDevice> cloudDeviceInfos = {dev};
    SleCloudPairService::GetInstance().UpdateCloudDeviceInfoList(cloudDeviceInfos);
}

/**
 * @tc.name: SleCloudPairServiceTest001
 * @tc.desc: 从 xml 读取云配设备信息
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest001, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest001 start");
    EXPECT_EQ(SleCloudPairService::GetInstance().cloudDevicesMap_.Size(), 1);
    std::string deviceName = "";
    SleCloudPairService::GetInstance().cloudDevicesMap_.GetValueAndOpt(DEVICE_SLE_REPORT_ADDR,
        [&deviceName] (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        deviceName = value->GetDeviceName();
    });
    EXPECT_EQ(deviceName, DEVICE_NAME);
    HILOGI("SleCloudPairServiceTest001 end");
}

/**
 * @tc.name: SleCloudPairServiceTest002
 * @tc.desc: 设备下云未连接
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest002, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest002 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    int32_t cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);

    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
    EXPECT_EQ(SleCloudPairService::GetInstance().cloudDevicesMap_.Size(), 1);
    HILOGI("SleCloudPairServiceTest002 end");
}

/**
 * @tc.name: SleCloudPairServiceTest003
 * @tc.desc: 设备下云未连接，获取已配对列表，设备名，星闪蓝牙地址映射
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest003, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest003 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    auto sleAdapter = static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    std::string remoteName = sleAdapter->GetDeviceName(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(remoteName, DEVICE_NAME);

    std::string aliasName = sleAdapter->GetAliasName(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(aliasName, DEVICE_NAME);

    std::vector<RawAddress> pairedList = sleAdapter->GetPairedDevices();
    EXPECT_EQ(pairedList.size(), 0);

    std::string sleAddr = "";
    sleAdapter->GetSleAddrByBtAddr(DEVICE_BT_ADDR, sleAddr);
    EXPECT_EQ(sleAddr, DEVICE_SLE_REPORT_ADDR);

    std::string btAddr = "";
    sleAdapter->GetBtAddrBySleAddr(DEVICE_SLE_REPORT_ADDR, btAddr);
    EXPECT_EQ(btAddr, DEVICE_BT_ADDR);
    HILOGI("SleCloudPairServiceTest003 end");
}

/**
 * @tc.name: SleCloudPairServiceTest004
 * @tc.desc: 设备下云未连接，acb连接成功，token校验失败
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest004, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest004 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    auto sleAdapter = static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    sleAdapter->ConnectAllProfile(RawAddress(DEVICE_SLE_REPORT_ADDR));
    int32_t cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(SleCloudPairService::GetInstance().IsCloudDeviceConnecting(RawAddress(DEVICE_SLE_REPORT_ADDR)), true);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);

    std::this_thread::sleep_for(std::chrono::milliseconds(11000)); // 延迟11s 触发定时器超时
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);

    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
    EXPECT_EQ(SleCloudPairService::GetInstance().IsCloudDeviceConnecting(RawAddress(DEVICE_SLE_REPORT_ADDR)), false);
    HILOGI("SleCloudPairServiceTest004 end");
}

/**
 * @tc.name: SleCloudPairServiceTest005
 * @tc.desc: 设备下云未连接，acb连接成功后，token校验成功，调可信配对，配对失败
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest005, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest005 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    auto sleAdapter = static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    sleAdapter->ConnectAllProfile(RawAddress(DEVICE_SLE_REPORT_ADDR));
    int32_t cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);

    sleAdapter->StartCrediblePair(RawAddress(DEVICE_SLE_REPORT_ADDR));
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);
    EXPECT_EQ(SleCloudPairService::GetInstance().IsCloudDeviceConnecting(RawAddress(DEVICE_SLE_REPORT_ADDR)), true);

    // 获取主耳地址
    RawAddress realAddr = SleCloudPairService::GetInstance().GetCloudDeviceRealAddress(
        RawAddress(DEVICE_SLE_MEMBER_ADDR));
    EXPECT_EQ(DEVICE_SLE_REPORT_ADDR, realAddr.GetAddress());

    // 配对失败
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    sleAdapter->NotifyPairStatusChanged(RawAddress(DEVICE_SLE_REPORT_ADDR),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRING), static_cast<int>(SlePairState::SLE_PAIR_NONE),
        static_cast<uint8_t>(PairingStateChangeReason::PAIRING_LOCAL_CANCELED));
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
    HILOGI("SleCloudPairServiceTest005 end");
}

/**
 * @tc.name: SleCloudPairServiceTest006
 * @tc.desc: 设备下云未连接，acb连接成功后，token校验成功，调可信配对，配对成功，配对成功后再调ConnectAllProfile
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest006, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest006 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    auto sleAdapter = static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    sleAdapter->ConnectAllProfile(RawAddress(DEVICE_SLE_REPORT_ADDR));
    int32_t cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);

    sleAdapter->StartCrediblePair(RawAddress(DEVICE_SLE_REPORT_ADDR));
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);
    EXPECT_EQ(SleCloudPairService::GetInstance().IsCloudDeviceConnecting(RawAddress(DEVICE_SLE_REPORT_ADDR)), true);

    // 配对成功
    sleAdapter->NotifyPairStatusChanged(RawAddress(DEVICE_SLE_REPORT_ADDR),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRING), static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<uint8_t>(PairingStateChangeReason::PAIRING_SUCCESS));
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);
    HILOGI("SleCloudPairServiceTest006 end");
}

/**
 * @tc.name: SleCloudPairServiceTest007
 * @tc.desc: 设备下云实体连接后，设备改名后重新下云
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest007, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest007 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    auto sleAdapter = static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    sleAdapter->ConnectAllProfile(RawAddress(DEVICE_SLE_REPORT_ADDR));
    int32_t cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);

    sleAdapter->StartCrediblePair(RawAddress(DEVICE_SLE_REPORT_ADDR));
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);
    EXPECT_EQ(SleCloudPairService::GetInstance().IsCloudDeviceConnecting(RawAddress(DEVICE_SLE_REPORT_ADDR)), true);

    // 配对成功
    sleAdapter->NotifyPairStatusChanged(RawAddress(DEVICE_SLE_REPORT_ADDR),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRING), static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<uint8_t>(PairingStateChangeReason::PAIRING_SUCCESS));
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);

    ProcDownCloudDevice("cloudDevice_", DEVICE_TOKEN);
    cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);
    EXPECT_EQ(SleCloudPairService::GetInstance().cloudDevicesMap_.Size(), 1);
    std::string deviceName = "";
    SleCloudPairService::GetInstance().cloudDevicesMap_.GetValueAndOpt(DEVICE_SLE_REPORT_ADDR,
        [&deviceName] (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        deviceName = value->GetDeviceName();
    });
    EXPECT_NE(deviceName, DEVICE_NAME);
    HILOGI("SleCloudPairServiceTest007 end");
}

/**
 * @tc.name: SleCloudPairServiceTest008
 * @tc.desc: 设备下云实体连接后，设备token变化后重新下云
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest008, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest008 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    auto sleAdapter = static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    sleAdapter->ConnectAllProfile(RawAddress(DEVICE_SLE_REPORT_ADDR));
    int32_t cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);

    sleAdapter->StartCrediblePair(RawAddress(DEVICE_SLE_REPORT_ADDR));
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);
    EXPECT_EQ(SleCloudPairService::GetInstance().IsCloudDeviceConnecting(RawAddress(DEVICE_SLE_REPORT_ADDR)), true);

    // 配对成功
    sleAdapter->NotifyPairStatusChanged(RawAddress(DEVICE_SLE_REPORT_ADDR),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRING), static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<uint8_t>(PairingStateChangeReason::PAIRING_SUCCESS));
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);

    std::vector<uint8_t> newToken =
        {0xFD, 0x0C, 0xF3, 0xAC, 0x96, 0x3C, 0x3D, 0x6E, 0x84, 0x3C, 0x85, 0x0B, 0x36, 0x17, 0xFC, 0x99,
         0xE7, 0xFD, 0xDF, 0x9A, 0xE0, 0xD8, 0x21, 0xDF, 0x9F, 0x99, 0x76, 0x98, 0x4C, 0x0F, 0x61, 0x51};
    ProcDownCloudDevice(DEVICE_NAME, newToken);
    cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
    EXPECT_EQ(SleCloudPairService::GetInstance().cloudDevicesMap_.Size(), 1);
    std::vector<uint8_t> deviceToken;
    SleCloudPairService::GetInstance().cloudDevicesMap_.GetValueAndOpt(DEVICE_SLE_REPORT_ADDR,
        [&deviceToken] (std::string key, std::shared_ptr<DownCloudPairDevice> value) -> void {
        deviceToken = value->GetToken();
    });
    EXPECT_NE(deviceToken, DEVICE_TOKEN);
    HILOGI("SleCloudPairServiceTest008 end");
}

/**
 * @tc.name: SleCloudPairServiceTest009
 * @tc.desc: 设备下云实体连接后，点击删除配对记录
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest009, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest009 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    auto sleAdapter = static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    sleAdapter->ConnectAllProfile(RawAddress(DEVICE_SLE_REPORT_ADDR));
    int32_t cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);

    sleAdapter->StartCrediblePair(RawAddress(DEVICE_SLE_REPORT_ADDR));
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);
    EXPECT_EQ(SleCloudPairService::GetInstance().IsCloudDeviceConnecting(RawAddress(DEVICE_SLE_REPORT_ADDR)), true);

    // 配对成功
    sleAdapter->NotifyPairStatusChanged(RawAddress(DEVICE_SLE_REPORT_ADDR),
        static_cast<int>(SlePairState::SLE_PAIR_PAIRING), static_cast<int>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<uint8_t>(PairingStateChangeReason::PAIRING_SUCCESS));
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);

    // 删配对记录
    sleAdapter->CancelPairing(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(SleCloudPairService::GetInstance().cloudDevicesMap_.Size(), 0);
    HILOGI("SleCloudPairServiceTest009 end");
}

/**
 * @tc.name: SleCloudPairServiceTest010
 * @tc.desc: 设备下云未实体连接，点击删除配对记录
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest010, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest010 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    EXPECT_EQ(SleCloudPairService::GetInstance().cloudDevicesMap_.Size(), 1);

    auto sleAdapter = (SleInterfaceAdapter *)(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    sleAdapter->CancelPairing(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(SleCloudPairService::GetInstance().cloudDevicesMap_.Size(), 0);

    HILOGI("SleCloudPairServiceTest010 end");
}

/**
 * @tc.name: SleCloudPairServiceTest011
 * @tc.desc: 测试HandlePairStatusChanged - 替换旧云设备场景
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest011, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest011 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    auto sleAdapter = static_cast<SleInterfaceAdapterSub*>(SleInterfaceManager::GetInstance()->GetAdapter(ADAPTER_SLE));
    SleCloudPairService::GetInstance().replacedDevices_.Insert(DEVICE_SLE_REPORT_ADDR);

    sleAdapter->NotifyPairStatusChanged(RawAddress(DEVICE_SLE_REPORT_ADDR),
        static_cast<int32_t>(SlePairState::SLE_PAIR_PAIRED),
        static_cast<int32_t>(SlePairState::SLE_PAIR_NONE), 0);

    EXPECT_EQ(SleCloudPairService::GetInstance().cloudDevicesMap_.Size(), 1);
    HILOGI("SleCloudPairServiceTest011 end");
}

/**
 * @tc.name: SleCloudPairServiceTest012
 * @tc.desc: 测试GetBtAddrByReportAddr和GetReportAddrByBtAddr
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest012, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest012 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);

    std::string btAddr = SleCloudPairService::GetInstance().GetBtAddrByReportAddr(DEVICE_SLE_REPORT_ADDR);
    EXPECT_EQ(btAddr, DEVICE_BT_ADDR);

    std::string sleReportAddr = SleCloudPairService::GetInstance().GetReportAddrByBtAddr(DEVICE_BT_ADDR);
    EXPECT_EQ(sleReportAddr, DEVICE_SLE_REPORT_ADDR);
    HILOGI("SleCloudPairServiceTest012 end");
}

/**
 * @tc.name: SleCloudPairServiceTest013
 * @tc.desc: 测试IsCloudDevice
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest013, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest013 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    bool result = SleCloudPairService::GetInstance().IsCloudDevice(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, true);

    const char* NON_CLOUD_ADDR = "AA:BB:CC:DD:EE:11";
    result = SleCloudPairService::GetInstance().IsCloudDevice(RawAddress(NON_CLOUD_ADDR));
    EXPECT_EQ(result, false);
    HILOGI("SleCloudPairServiceTest013 end");
}

/**
 * @tc.name: SleCloudPairServiceTest014
 * @tc.desc: 测试CancelCloudPairing - 不同配对状态
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest014, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest014 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);

    bool result = SleCloudPairService::GetInstance().CancelCloudPairing(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, true);
    EXPECT_EQ(SleCloudPairService::GetInstance().cloudDevicesMap_.Size(), 0);
    HILOGI("SleCloudPairServiceTest014 end");
}

/**
 * @tc.name: SleCloudPairServiceTest015
 * @tc.desc: 测试SetKeyMissingPairState
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest015, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest015 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);

    SleCloudPairService::GetInstance().SetKeyMissingPairState(RawAddress(DEVICE_SLE_REPORT_ADDR));

    int32_t cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_TOKEN_CHANGING);
    HILOGI("SleCloudPairServiceTest015 end");
}

/**
 * @tc.name: SleCloudPairServiceTest016
 * @tc.desc: 测试GetCloudDeviceRealAddress
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest016, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest016 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);

    SleCloudPairService::GetInstance().HandleAcbStateChanged(RawAddress(DEVICE_SLE_MEMBER_ADDR),
         static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED), 0);
    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);
    RawAddress realAddr = SleCloudPairService::GetInstance().GetCloudDeviceRealAddress(
        RawAddress(DEVICE_SLE_MEMBER_ADDR));
    EXPECT_EQ(realAddr.GetAddress(), DEVICE_SLE_MEMBER_ADDR);
    HILOGI("SleCloudPairServiceTest016 end");
}

/**
 * @tc.name: SleCloudPairServiceTest017
 * @tc.desc: 测试IsInRepairing和IsInReplacing
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest017, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest017 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    SleCloudPairService::GetInstance().needRepairDevices_.Insert(DEVICE_SLE_REPORT_ADDR);
    bool result = SleCloudPairService::GetInstance().IsInRepairing(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, true);

    SleCloudPairService::GetInstance().replacedDevices_.Insert(DEVICE_SLE_REPORT_ADDR);
    result = SleCloudPairService::GetInstance().IsInReplacing(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, true);

    const char* NON_CLOUD_ADDR = "AA:BB:CC:DD:EE:33";
    result = SleCloudPairService::GetInstance().IsInRepairing(RawAddress(NON_CLOUD_ADDR));
    EXPECT_EQ(result, false);

    result = SleCloudPairService::GetInstance().IsInReplacing(RawAddress(NON_CLOUD_ADDR));
    EXPECT_EQ(result, false);
    HILOGI("SleCloudPairServiceTest017 end");
}

/**
 * @tc.name: SleCloudPairServiceTest018
 * @tc.desc: 测试IsPreparingRepair
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest018, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest018 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR, 
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_TOKEN_CHANGING);
    SleCloudPairService::GetInstance().needRepairDevices_.Insert(DEVICE_SLE_REPORT_ADDR);

    bool result = SleCloudPairService::GetInstance().IsPreparingRepair(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, true);

    SleCloudPairService::GetInstance().needRepairDevices_.Erase(DEVICE_SLE_REPORT_ADDR);
    result = SleCloudPairService::GetInstance().IsPreparingRepair(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, false);
    HILOGI("SleCloudPairServiceTest018 end");
}

/**
 * @tc.name: SleCloudPairServiceTest019
 * @tc.desc: 测试ClearToken
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest019, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest019 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    std::vector<uint8_t> token = DEVICE_TOKEN;
    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_TOKEN_CHANGING);

    bool result = SleCloudPairService::GetInstance().ClearToken(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, true);

    std::vector<uint8_t> emptyToken;
    SleCloudPairService::GetInstance().cloudDevicesMap_.GetValueAndOpt(DEVICE_SLE_REPORT_ADDR,
        [&emptyToken] (std::string key, std::shared_ptr<DownCloudPairDevice> value) {
            emptyToken = value->GetToken();
        });
    EXPECT_EQ(emptyToken.size(), 0);
    HILOGI("SleCloudPairServiceTest019 end");
}

/**
 * @tc.name: SleCloudPairServiceTest020
 * @tc.desc: IsCloudDeviceCreatePair
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest020, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest020 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    bool result = SleCloudPairService::GetInstance().IsCloudDeviceCreatePair(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, false);

    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);

    result = SleCloudPairService::GetInstance().IsCloudDeviceCreatePair(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, true);
    HILOGI("SleCloudPairServiceTest020 end");
}

/**
 * @tc.name: SleCloudPairServiceTest021
 * @tc.desc: 测试GetCloudDeviceAliasName和SetCloudDeviceAliasName
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest021, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest021 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    std::string aliasName = SleCloudPairService::GetInstance().GetCloudDeviceAliasName(
        RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(aliasName, DEVICE_NAME);

    const char* NEW_NAME = "NewDeviceName";
    bool result = SleCloudPairService::GetInstance().SetCloudDeviceAliasName(
        RawAddress(DEVICE_SLE_REPORT_ADDR), NEW_NAME);
    EXPECT_EQ(result, true);

    aliasName = SleCloudPairService::GetInstance().GetCloudDeviceAliasName(
        RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(aliasName, NEW_NAME);
    HILOGI("SleCloudPairServiceTest021 end");
}

/**
 * @tc.name: SleCloudPairServiceTest022
 * @tc.desc: 测试GetCloudDeviceIcondId
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest022, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest022 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    std::string iconId = SleCloudPairService::GetInstance().GetCloudDeviceIcondId(
        RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_NE(iconId, "");
    HILOGI("SleCloudPairServiceTest022 end");
}

/**
 * @tc.name: SleCloudPairServiceTest023
 * @tc.desc: 测试GetCloudDeviceSubModelId
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest023, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest023 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    std::string subModelId = SleCloudPairService::GetInstance().GetCloudDeviceSubModelId(
        RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(subModelId, DEVICE_SUBMODEL_ID);
    HILOGI("SleCloudPairServiceTest023 end");
}

/**
 * @tc.name: SleCloudPairServiceTest024
 * @tc.desc: 测试GetCloudDeviceManufacturerBusinessType
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest024, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest024 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    int businessType = SleCloudPairService::GetInstance().GetCloudDeviceManufacturerBusinessType(
        RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(businessType, Nearlink::SLE_PRIVATE_AUDIO_BUSINESS_TYPE);

    const char* NON_CLOUD_ADDR = "AA:BB:CC:DD:EE:33";
    businessType = SleCloudPairService::GetInstance().GetCloudDeviceManufacturerBusinessType(
        RawAddress(NON_CLOUD_ADDR));
    EXPECT_EQ(businessType, 0);
    HILOGI("SleCloudPairServiceTest024 end");
}

/**
 * @tc.name: SleCloudPairServiceTest025
 * @tc.desc: 测试DownCloudPairDevice
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest025, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest025 start");
    auto device = std::make_shared<DownCloudPairDevice>();
    device->SetReportAddr(DEVICE_SLE_REPORT_ADDR);
    device->SetMembersAddr({DEVICE_SLE_REPORT_ADDR, DEVICE_SLE_MEMBER_ADDR});

    device->SetCloudPairState(NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);
    EXPECT_EQ(device->GetCloudPairState(), NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);

    device->SetConnectedState(DEVICE_SLE_MEMBER_ADDR, true);
    auto connectedMaps = device->GetConnectedMaps();
    EXPECT_EQ(connectedMaps[DEVICE_SLE_MEMBER_ADDR], true);

    EXPECT_EQ(device->IsAllMembersDisconnected(), false);

    device->SetConnectedState(DEVICE_SLE_MEMBER_ADDR, false);
    EXPECT_EQ(device->IsAllMembersDisconnected(), true);

    device->SetConnectedState(DEVICE_SLE_REPORT_ADDR, false);
    EXPECT_EQ(device->IsAllMembersDisconnected(), true);
    HILOGI("SleCloudPairServiceTest025 end");
}

/**
 * @tc.name: SleCloudPairServiceTest026
 * @tc.desc: 测试CancelCloudPairComplete - Token变化场景
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest026, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest026 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_TOKEN_CHANGING);

    bool result = SleCloudPairService::GetInstance().CancelCloudPairComplete(RawAddress(DEVICE_SLE_REPORT_ADDR),
        static_cast<int>(SlePairState::SLE_PAIR_NONE), 0, true,
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));

    EXPECT_EQ(result, false);
    int32_t cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID);
    HILOGI("SleCloudPairServiceTest026 end");
}

/**
 * @tc.name: SleCloudPairServiceTest027
 * @tc.desc: 测试CancelCloudPairing
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest027, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest027 start");

    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);

    bool result = SleCloudPairService::GetInstance().CancelCloudPairing(RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, true);
    EXPECT_EQ(SleCloudPairService::GetInstance().cloudDevicesMap_.Size(), 0);
    HILOGI("SleCloudPairServiceTest027 end");
}

/**
 * @tc.name: SleCloudPairServiceTest029
 * @tc.desc: 测试GetCollabAddrByReportAddr
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest029, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest029 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    std::string collabAddr = SleCloudPairService::GetInstance().GetCollabAddrByReportAddr(
        RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(collabAddr, DEVICE_SLE_MEMBER_ADDR);
    HILOGI("SleCloudPairServiceTest029 end");
}

/**
 * @tc.name: SleCloudPairServiceTest030
 * @tc.desc: 测试ConnectCloudDeviceAllProfile - 重新配对流程
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest030, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest030 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);

    bool result = SleCloudPairService::GetInstance().ConnectCloudDeviceAllProfile(
        RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, true);

    int32_t cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);
    HILOGI("SleCloudPairServiceTest030 end");
}

/**
 * @tc.name: SleCloudPairServiceTest031
 * @tc.desc: 测试IsCloudDeviceConnecting
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest031, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest031 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);
    EXPECT_EQ(SleCloudPairService::GetInstance().IsCloudDeviceConnecting(RawAddress(DEVICE_SLE_REPORT_ADDR)), true);

    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);
    EXPECT_EQ(SleCloudPairService::GetInstance().IsCloudDeviceConnecting(RawAddress(DEVICE_SLE_REPORT_ADDR)), true);

    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);
    EXPECT_EQ(SleCloudPairService::GetInstance().IsCloudDeviceConnecting(RawAddress(DEVICE_SLE_REPORT_ADDR)), false);
    HILOGI("SleCloudPairServiceTest031 end");
}

/**
 * @tc.name: SleCloudPairServiceTest032
 * @tc.desc: 测试CloudDeviceConnectionComplete
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest032, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest032 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);

    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_NONE);
    bool result = SleCloudPairService::GetInstance().CloudDeviceConnectionComplete(
        RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, true);

    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);
    result = SleCloudPairService::GetInstance().CloudDeviceConnectionComplete(
        RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, false);
    HILOGI("SleCloudPairServiceTest032 end");
}

/**
 * @tc.name: SleCloudPairServiceTest033
 * @tc.desc: 测试SetCrediblePairState
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest033, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest033 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRING);
    SleCloudPairService::GetInstance().needRepairDevices_.Insert(DEVICE_SLE_REPORT_ADDR);

    SleCloudPairService::GetInstance().SetCrediblePairState(RawAddress(DEVICE_SLE_REPORT_ADDR));

    int32_t cloudPairState = NL_CLOUD_PAIR_STATE::CLOUD_PAIR_INVALID;
    SleCloudPairService::GetInstance().GetCloudPairState(DEVICE_SLE_REPORT_ADDR, cloudPairState);
    EXPECT_EQ(cloudPairState, NL_CLOUD_PAIR_STATE::CLOUD_PAIR_CREATE_PAIR);
    EXPECT_EQ(SleCloudPairService::GetInstance().needRepairDevices_.Find([](std::string addr) {
        return addr == DEVICE_SLE_REPORT_ADDR;
    }), false);
    HILOGI("SleCloudPairServiceTest033 end");
}

/**
 * @tc.name: SleCloudPairServiceTest034
 * @tc.desc: 测试ConnectCloudDeviceAllProfile - 已配对状态
 * @tc.type: FUNC
 */
HWTEST_F(SleCloudPairServiceTest, SleCloudPairServiceTest034, TestSize.Level1)
{
    HILOGI("SleCloudPairServiceTest034 start");
    ProcDownCloudDevice(DEVICE_NAME, DEVICE_TOKEN);
    SleCloudPairService::GetInstance().UpdateCloudState(DEVICE_SLE_REPORT_ADDR,
        NL_CLOUD_PAIR_STATE::CLOUD_PAIR_PAIRED);

    bool result = SleCloudPairService::GetInstance().ConnectCloudDeviceAllProfile(
        RawAddress(DEVICE_SLE_REPORT_ADDR));
    EXPECT_EQ(result, false);
    HILOGI("SleCloudPairServiceTest034 end");
}


} // namespace TEST
} // namespace Nearlink
} // namespace OHOS