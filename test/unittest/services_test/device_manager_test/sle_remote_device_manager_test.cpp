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

/*
 * 远端设备管理器测试：注入自构造的远端设备后，验证配对/连接清单过滤、
 * LCID 与地址互查、星闪与蓝牙地址双向映射、型号信息条件落盘、
 * 自动回连白名单的各条排除规则以及别名同名短路。
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "SleRemoteDeviceManager.h"
#include "SleConfig.h"

using namespace OHOS::Nearlink;

namespace {
constexpr const char *ADDR_A = "11:11:11:11:11:11";
constexpr const char *ADDR_B = "22:22:22:22:22:22";
constexpr const char *ADDR_C = "33:33:33:33:33:33";
constexpr const char *BT_ADDR_A = "44:44:44:44:44:41";
constexpr const char *BT_ADDR_B = "44:44:44:44:44:42";
constexpr uint16_t LCID_A = 0x0102;
} // namespace

class SleRemoteDeviceManagerTest : public ::testing::Test {
protected:
    /* 配置单例首次使用前必须加载文档，别名/型号/地址映射的落盘才生效 */
    static void SetUpTestCase()
    {
        SleConfig::GetInstance().LoadConfigInfo();
    }

    static void TearDownTestCase() {}

    void SetUp() override
    {
        manager_ = SleRemoteDeviceManager::GetInstance();
    }

    void TearDown() override
    {
        /* 清空注入的设备并移除落盘的设备段落，单例不留跨用例状态 */
        manager_->RemoveAllPeripheralDevices();
        (void)SleConfig::GetInstance().RemovePairedDevice(ADDR_A);
        (void)SleConfig::GetInstance().RemovePairedDevice(ADDR_B);
        (void)SleConfig::GetInstance().RemovePairedDevice("66:66:66:66:66:66");
        (void)SleConfig::GetInstance().Save();
    }

    /* 构造一个指定地址的远端设备并注入管理器 */
    std::shared_ptr<SlePeripheralDevice> AddDevice(const std::string &addr)
    {
        auto device = std::make_shared<SlePeripheralDevice>();
        device->SetAddress(RawAddress(addr));
        manager_->AddPeripheralDevice(addr, device);
        return device;
    }

    SleRemoteDeviceManager *manager_ = nullptr;
};

/* 配对清单只收配对成功的设备 */
TEST_F(SleRemoteDeviceManagerTest, PairedDevicesFilter)
{
    auto deviceA = AddDevice(ADDR_A);
    auto deviceB = AddDevice(ADDR_B);
    ASSERT_TRUE(deviceA->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED)));
    ASSERT_TRUE(deviceB->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRING)));

    auto paired = manager_->GetPairedDevices();
    ASSERT_EQ(paired.size(), 1U);
    EXPECT_EQ(paired[0].GetAddress(), ADDR_A);
}

/* 连接中清单只收正在建链的设备 */
TEST_F(SleRemoteDeviceManagerTest, ConnectingDevicesFilter)
{
    auto deviceA = AddDevice(ADDR_A);
    auto deviceB = AddDevice(ADDR_B);
    deviceA->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTING));
    deviceB->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));

    auto connecting = manager_->GetConnectingDevices();
    ASSERT_EQ(connecting.size(), 1U);
    EXPECT_EQ(connecting[0].GetAddress(), ADDR_A);
}

/* 已连接清单同时收已连接与已加密两态，其余状态排除 */
TEST_F(SleRemoteDeviceManagerTest, AcbConnectedDevicesIncludesEncrypted)
{
    auto deviceA = AddDevice(ADDR_A);
    auto deviceB = AddDevice(ADDR_B);
    auto deviceC = AddDevice(ADDR_C);
    deviceA->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));
    deviceB->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
    deviceC->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));

    auto connected = manager_->GetAcbConnectedDevices();
    ASSERT_EQ(connected.size(), 2U);
}

/* 是否存在已连接设备：无连接、已连接、仅加密三种情形 */
TEST_F(SleRemoteDeviceManagerTest, HasConnectedDeviceStates)
{
    EXPECT_FALSE(manager_->HasConnectedDevice());

    auto deviceA = AddDevice(ADDR_A);
    EXPECT_FALSE(manager_->HasConnectedDevice());

    deviceA->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_ENCRYPTED));
    EXPECT_TRUE(manager_->HasConnectedDevice());
}

/* LCID 查地址命中返回地址，未命中返回无效地址 */
TEST_F(SleRemoteDeviceManagerTest, AddressByLcidLookup)
{
    auto deviceA = AddDevice(ADDR_A);
    deviceA->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    EXPECT_TRUE(manager_->SetLcid(ADDR_A, LCID_A));

    EXPECT_EQ(manager_->GetAddressByLcid(LCID_A), ADDR_A);
    EXPECT_EQ(manager_->GetAddressByLcid(LCID_A + 1), INVALID_MAC_ADDRESS);
}

/* 断连原因：设备不存在时保持默认值 0，存在时读回设置值 */
TEST_F(SleRemoteDeviceManagerTest, DisConnReasonMissingDeviceKeepsDefault)
{
    EXPECT_EQ(manager_->GetAcbDisConnReason("55:55:55:55:55:55"), 0);

    AddDevice(ADDR_A);
    EXPECT_TRUE(manager_->SetAcbDisConnReason(ADDR_A, 0x13));
    EXPECT_EQ(manager_->GetAcbDisConnReason(ADDR_A), 0x13);
}

/* 记录主动发起方向只在未建链时生效，已建链设备保持原方向 */
TEST_F(SleRemoteDeviceManagerTest, ConnDirectActiveOnlyWhenDisconnected)
{
    auto deviceA = AddDevice(ADDR_A);
    auto deviceB = AddDevice(ADDR_B);
    deviceA->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    deviceB->SetAcbConnectState(static_cast<int>(SleConnState::SLE_CONNECTION_STATE_CONNECTED));

    manager_->SetConnDirectActive(RawAddress(ADDR_A));
    manager_->SetConnDirectActive(RawAddress(ADDR_B));

    EXPECT_EQ(manager_->GetConnDirect(RawAddress(ADDR_A)), static_cast<int>(SleConnDirect::SLE_CONNECTION_ACTIVE));
    EXPECT_EQ(manager_->GetConnDirect(RawAddress(ADDR_B)), static_cast<int>(SleConnDirect::SLE_CONNECTION_PASSIVE));
}

/* 设备不可用时禁止业务连接；可用时允许 */
TEST_F(SleRemoteDeviceManagerTest, ServiceSupportedDependsOnAvailability)
{
    auto deviceA = AddDevice(ADDR_A);
    deviceA->SetIsDeviceAvailable(false);
    EXPECT_FALSE(manager_->IsServiceSupportedConn(RawAddress(ADDR_A)));

    deviceA->SetIsDeviceAvailable(true);
    EXPECT_TRUE(manager_->IsServiceSupportedConn(RawAddress(ADDR_A)));
}

/* 星闪地址与蓝牙地址双向映射：新地址写入、相同地址不重写、未登记返回失败 */
TEST_F(SleRemoteDeviceManagerTest, BtAddrMappingBothDirections)
{
    auto deviceA = AddDevice(ADDR_A);
    AddDevice(ADDR_B);

    EXPECT_TRUE(manager_->SetBtAddrBySleAddr(ADDR_A, BT_ADDR_A));
    std::string btAddr;
    EXPECT_TRUE(manager_->GetBtAddrBySleAddr(ADDR_A, btAddr));
    EXPECT_EQ(btAddr, BT_ADDR_A);
    std::string sleAddr;
    EXPECT_TRUE(manager_->GetSleAddrByBtAddr(BT_ADDR_A, sleAddr));
    EXPECT_EQ(sleAddr, ADDR_A);

    /* 相同蓝牙地址再次设置不触发重写；不同地址则更新 */
    EXPECT_TRUE(manager_->SetBtAddrBySleAddr(ADDR_A, BT_ADDR_A));
    EXPECT_TRUE(manager_->SetBtAddrBySleAddr(ADDR_A, BT_ADDR_B));
    EXPECT_TRUE(manager_->GetBtAddrBySleAddr(ADDR_A, btAddr));
    EXPECT_EQ(btAddr, BT_ADDR_B);

    /* 未登记的地址双向查询都失败 */
    EXPECT_FALSE(manager_->SetBtAddrBySleAddr("66:66:66:66:66:66", BT_ADDR_A));
    EXPECT_FALSE(manager_->GetBtAddrBySleAddr("66:66:66:66:66:66", btAddr));
    EXPECT_FALSE(manager_->GetSleAddrByBtAddr("77:77:77:77:77:77", sleAddr));
}

/* 映射表中允许存在空设备引用：反查跳过空条目，写入空条目不解引用 */
TEST_F(SleRemoteDeviceManagerTest, BtAddrLookupSkipsNullDeviceEntry)
{
    std::shared_ptr<SlePeripheralDevice> nullDevice = nullptr;
    manager_->AddPeripheralDevice(ADDR_A, nullDevice);
    auto deviceB = AddDevice(ADDR_B);
    deviceB->SetBtAddr(BT_ADDR_B);

    /* 空引用条目命中时不解引用，仍返回条目存在 */
    EXPECT_TRUE(manager_->SetBtAddrBySleAddr(ADDR_A, BT_ADDR_A));

    std::string sleAddr;
    EXPECT_TRUE(manager_->GetSleAddrByBtAddr(BT_ADDR_B, sleAddr));
    EXPECT_EQ(sleAddr, ADDR_B);

    manager_->RemovePeripheralDevice(ADDR_A);
}

/* 设备不存在时的状态查询与可用性开关：走日志分支并返回默认值 */
TEST_F(SleRemoteDeviceManagerTest, MissingDeviceQueriesKeepDefaults)
{
    const std::string unknown = "66:66:66:66:66:66";
    EXPECT_EQ(manager_->GetAcbState(unknown),
        static_cast<int>(SleConnState::SLE_CONNECTION_STATE_DISCONNECTED));
    EXPECT_EQ(manager_->GetAcbDisConnReason(unknown), 0);

    manager_->SetDeviceIsAvailable(RawAddress(unknown), true);
    EXPECT_TRUE(manager_->GetIsDeviceAvailable(RawAddress(unknown)));
}

/* 型号信息：四个字段与新型号按空/非空条件落盘，读回与查询存在性一致 */
TEST_F(SleRemoteDeviceManagerTest, DeviceModelInfoSaveMatrix)
{
    auto deviceA = AddDevice(ADDR_A);

    /* 全空：不落任何字段，读回仍可查询 */
    DeviceModel empty;
    manager_->SaveDeviceModelInfo(ADDR_A, empty, "");
    DeviceModel loadedEmpty;
    std::string newModelEmpty;
    EXPECT_TRUE(manager_->GetDeviceModelInfo(RawAddress(ADDR_A), loadedEmpty, newModelEmpty));
    EXPECT_EQ(loadedEmpty.GetModelId(), "");

    /* 逐字段填入：型号、新型号、图标、形态各自落盘 */
    DeviceModel model;
    model.SetModelId("model001");
    manager_->SaveDeviceModelInfo(ADDR_A, model, "new001");

    DeviceModel loaded;
    std::string newModelId;
    ASSERT_TRUE(manager_->GetDeviceModelInfo(RawAddress(ADDR_A), loaded, newModelId));
    EXPECT_EQ(loaded.GetModelId(), "model001");
    EXPECT_EQ(newModelId, "new001");

    model.SetIconId("icon002");
    model.SetDevType("watch");
    manager_->SaveDeviceModelInfo(ADDR_A, model, "");
    ASSERT_TRUE(manager_->GetDeviceModelInfo(RawAddress(ADDR_A), loaded, newModelId));
    EXPECT_EQ(loaded.GetIconId(), "icon002");
    EXPECT_EQ(loaded.GetDevType(), "watch");
    EXPECT_EQ(newModelId, "new001");

    /* 未登记设备查询失败 */
    DeviceModel absent;
    EXPECT_FALSE(manager_->GetDeviceModelInfo(RawAddress("66:66:66:66:66:66"), absent, newModelId));
}

/* 自动回连白名单：未配对、被动配对、车锁、路由器、不可用、非音频设备均被排除 */
TEST_F(SleRemoteDeviceManagerTest, DirectConnDevicesExclusionMatrix)
{
    /* 完全符合条件的设备：主动配对、已配对、普通外观、可用、音频 */
    auto eligible = AddDevice(ADDR_A);
    ASSERT_TRUE(eligible->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED)));
    eligible->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));
    eligible->SetAppearance(0x0100);
    eligible->SetIsDeviceAvailable(true);
    eligible->SetIsAudioDeviceFlag(true);

    /* 未配对 */
    auto notPaired = AddDevice("A0:00:00:00:00:01");
    notPaired->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));
    notPaired->SetIsAudioDeviceFlag(true);
    /* 被动配对 */
    auto passive = AddDevice("A0:00:00:00:00:02");
    ASSERT_TRUE(passive->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED)));
    passive->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_PASSIVE));
    passive->SetIsAudioDeviceFlag(true);
    /* 车锁 */
    auto vehicleLock = AddDevice("A0:00:00:00:00:03");
    ASSERT_TRUE(vehicleLock->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED)));
    vehicleLock->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));
    vehicleLock->SetAppearance(static_cast<int>(DeviceClassForService::DEVICE_CLASS_VEHICLE_LOCK));
    vehicleLock->SetIsAudioDeviceFlag(true);
    /* 路由器 */
    auto networking = AddDevice("A0:00:00:00:00:04");
    ASSERT_TRUE(networking->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED)));
    networking->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));
    networking->SetAppearance(static_cast<int>(DeviceClassForService::DEVICE_NETWORKING));
    networking->SetIsAudioDeviceFlag(true);
    /* 不可用 */
    auto unavailable = AddDevice("A0:00:00:00:00:05");
    ASSERT_TRUE(unavailable->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED)));
    unavailable->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));
    unavailable->SetIsDeviceAvailable(false);
    unavailable->SetIsAudioDeviceFlag(true);
    /* 非音频（默认走后台回连） */
    auto nonAudio = AddDevice("A0:00:00:00:00:06");
    ASSERT_TRUE(nonAudio->SetPairedStatus(static_cast<int>(SlePairState::SLE_PAIR_PAIRED)));
    nonAudio->SetPairDirection(static_cast<int>(SlePairDirect::SLE_PAIR_ACTIVE));
    nonAudio->SetIsDeviceAvailable(true);
    nonAudio->SetIsAudioDeviceFlag(false);

    auto directConn = manager_->GetDirectConnDevices();
    ASSERT_EQ(directConn.size(), 1U);
    EXPECT_EQ(directConn[0].GetAddress(), ADDR_A);
}

/* 别名设置：新别名生效并落盘，同名重复设置短路不落盘，未登记设备失败 */
TEST_F(SleRemoteDeviceManagerTest, AliasNameSetAndSameNameShortCircuit)
{
    auto deviceA = AddDevice(ADDR_A);

    EXPECT_TRUE(manager_->SetAliasName(RawAddress(ADDR_A), "left-earbud"));
    EXPECT_EQ(manager_->GetAliasName(RawAddress(ADDR_A)), "left-earbud");

    EXPECT_TRUE(manager_->SetAliasName(RawAddress(ADDR_A), "left-earbud"));
    EXPECT_EQ(manager_->GetAliasName(RawAddress(ADDR_A)), "left-earbud");

    EXPECT_FALSE(manager_->SetAliasName(RawAddress("66:66:66:66:66:66"), "ghost"));
    EXPECT_EQ(manager_->GetAliasName(RawAddress("66:66:66:66:66:66")), "");
}
