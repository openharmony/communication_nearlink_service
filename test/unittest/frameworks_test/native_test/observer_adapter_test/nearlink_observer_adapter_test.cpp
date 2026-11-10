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
#include <memory>
#include <string>
#include <vector>

#include "log.h"
#include "nearlink_base_host.h"
#include "nearlink_host.h"
#include "nearlink_observer_adapter.h"
#include "nearlink_remote_device.h"
#include "nearlink_uuid.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string TEST_DEVICE_ADDR = "AA:BB:CC:DD:EE:11";
const std::string TEST_DEVICE_ADDR2 = "AA:BB:CC:DD:EE:22";
} // namespace

// 记录型 Fake：实现上层 NearlinkHostObserver 全部回调，仅记录调用计数与参数
class FakeHostObserver : public NearlinkHostObserver {
public:
    void OnStateChanged(const int transport, const int status) override
    {
        stateChangedCount_++;
        lastTransport_ = transport;
        lastStatus_ = status;
    }
    void OnFullStateChanged(const int transport, const int status) override
    {
        fullStateChangedCount_++;
        lastFullTransport_ = transport;
        lastFullStatus_ = status;
    }
    void OnPairConfirmed(const NearlinkRemoteDevice &device, int reqType, int number) override
    {
        pairConfirmedCount_++;
        lastPairAddr_ = device.GetDeviceAddr();
        lastReqType_ = reqType;
        lastNumber_ = number;
    }
    void OnDeviceNameChanged(const std::string &deviceName) override
    {
        deviceNameCount_++;
        lastDeviceName_ = deviceName;
    }
    void OnDeviceAddrChanged(const std::string &address) override
    {
        deviceAddrCount_++;
        lastAddress_ = address;
    }

    int stateChangedCount_ = 0;
    int fullStateChangedCount_ = 0;
    int pairConfirmedCount_ = 0;
    int deviceNameCount_ = 0;
    int deviceAddrCount_ = 0;
    int lastTransport_ = 0;
    int lastStatus_ = 0;
    int lastFullTransport_ = 0;
    int lastFullStatus_ = 0;
    int lastReqType_ = 0;
    int lastNumber_ = 0;
    std::string lastPairAddr_;
    std::string lastDeviceName_;
    std::string lastAddress_;
};

// 记录型 Fake：实现上层 NearlinkRemoteDeviceObserver 全部 9 个回调
class FakeRemoteDeviceObserver : public NearlinkRemoteDeviceObserver {
public:
    void OnAcbStateChanged(const NearlinkRemoteDevice &device, int state, int reason) override
    {
        acbCount_++;
        lastAcbAddr_ = device.GetDeviceAddr();
        lastAcbState_ = state;
        lastAcbReason_ = reason;
    }
    void OnPairingRequest(const NearlinkRemoteDevice &device, const std::string &passkey, int type) override
    {
        pairingReqCount_++;
        lastPairingAddr_ = device.GetDeviceAddr();
        lastPasskey_ = passkey;
        lastPairingType_ = type;
    }
    void OnPairStatusChanged(const NearlinkRemoteDevice &device, int preState, int state, int reason) override
    {
        pairStatusCount_++;
        lastPairStatusAddr_ = device.GetDeviceAddr();
        lastPairPreState_ = preState;
        lastPairState_ = state;
        lastPairReason_ = reason;
    }
    void OnConnectionStateChanged(const NearlinkRemoteDevice &device, int preState, int state, int reason) override
    {
        connStateCount_++;
        lastConnAddr_ = device.GetDeviceAddr();
        lastConnPreState_ = preState;
        lastConnState_ = state;
        lastConnReason_ = reason;
    }
    void OnRemoteUuidChanged(const NearlinkRemoteDevice &device, const std::vector<UUID> &uuids) override
    {
        uuidCount_++;
        lastUuidAddr_ = device.GetDeviceAddr();
        lastUuids_ = uuids;
    }
    void OnRemoteNameChanged(const NearlinkRemoteDevice &device, const std::string &deviceName) override
    {
        remoteNameCount_++;
        lastRemoteNameAddr_ = device.GetDeviceAddr();
        lastRemoteName_ = deviceName;
    }
    void OnRemoteAliasChanged(const NearlinkRemoteDevice &device, const std::string &alias) override
    {
        remoteAliasCount_++;
        lastAliasAddr_ = device.GetDeviceAddr();
        lastAlias_ = alias;
    }
    void OnReadRemoteRssiEvent(const NearlinkRemoteDevice &device, int rssi, int status) override
    {
        rssiCount_++;
        lastRssiAddr_ = device.GetDeviceAddr();
        lastRssi_ = rssi;
        lastRssiStatus_ = status;
    }
    void OnLinkFreqBandChanged(const NearlinkRemoteDevice &device, int32_t freqBand) override
    {
        freqBandCount_++;
        lastFreqAddr_ = device.GetDeviceAddr();
        lastFreqBand_ = freqBand;
    }

    int acbCount_ = 0;
    int pairingReqCount_ = 0;
    int pairStatusCount_ = 0;
    int connStateCount_ = 0;
    int uuidCount_ = 0;
    int remoteNameCount_ = 0;
    int remoteAliasCount_ = 0;
    int rssiCount_ = 0;
    int freqBandCount_ = 0;
    int lastAcbState_ = 0;
    int lastAcbReason_ = 0;
    int lastPairingType_ = 0;
    int lastPairPreState_ = 0;
    int lastPairState_ = 0;
    int lastPairReason_ = 0;
    int lastConnPreState_ = 0;
    int lastConnState_ = 0;
    int lastConnReason_ = 0;
    int lastRssi_ = 0;
    int lastRssiStatus_ = 0;
    int32_t lastFreqBand_ = 0;
    std::string lastAcbAddr_;
    std::string lastPairingAddr_;
    std::string lastPairStatusAddr_;
    std::string lastConnAddr_;
    std::string lastUuidAddr_;
    std::string lastRemoteNameAddr_;
    std::string lastAliasAddr_;
    std::string lastRssiAddr_;
    std::string lastFreqAddr_;
    std::string lastPasskey_;
    std::string lastRemoteName_;
    std::string lastAlias_;
    std::vector<UUID> lastUuids_;
};

// 记录型 Fake：实现上层 NearlinkRemoteDeviceBatteryObserver 全部 2 个回调
// 注：基类未声明虚析构，此处补虚析构避免 -Wdelete-non-abstract-non-virtual-dtor
class FakeBatteryObserver : public NearlinkRemoteDeviceBatteryObserver {
public:
    virtual ~FakeBatteryObserver() = default;
    void OnGetBatteryLevelEvent(const NearlinkRemoteDevice &device, int32_t batteryLevel) override
    {
        getLevelCount_++;
        lastGetAddr_ = device.GetDeviceAddr();
        lastGetLevel_ = batteryLevel;
    }
    void OnBatteryLevelChanged(const NearlinkRemoteDevice &device, int32_t batteryLevel) override
    {
        levelChangedCount_++;
        lastChangedAddr_ = device.GetDeviceAddr();
        lastChangedLevel_ = batteryLevel;
    }

    int getLevelCount_ = 0;
    int levelChangedCount_ = 0;
    int32_t lastGetLevel_ = 0;
    int32_t lastChangedLevel_ = 0;
    std::string lastGetAddr_;
    std::string lastChangedAddr_;
};

// 记录型 Fake：实现上层 NearlinkRemoteDeviceRssiObserver 的 1 个回调
class FakeRssiObserver : public NearlinkRemoteDeviceRssiObserver {
public:
    void OnReadRemoteRssiEvent(const NearlinkRemoteDevice &device, int rssi, int status) override
    {
        rssiCount_++;
        lastAddr_ = device.GetDeviceAddr();
        lastRssi_ = rssi;
        lastStatus_ = status;
    }

    int rssiCount_ = 0;
    int lastRssi_ = 0;
    int lastStatus_ = 0;
    std::string lastAddr_;
};

class NearlinkObserverAdapterTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HILOGI("SetUpTestCase NearlinkObserverAdapterTest.");
    }
    static void TearDownTestCase()
    {
        HILOGI("TearDownTestCase NearlinkObserverAdapterTest");
    }
    void SetUp()
    {
        HILOGI("SetUp NearlinkObserverAdapterTest.");
    }
    void TearDown()
    {
        HILOGI("TearDown NearlinkObserverAdapterTest.");
    }
};

/**
 * @tc.name: HostAdapterForwardCallbacks001
 * @tc.desc: 观察者存活时适配器 5 个回调全部转发且参数一致
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, HostAdapterForwardCallbacks001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: HostAdapterForwardCallbacks001 start");
    auto fake = std::make_shared<FakeHostObserver>();
    auto adapter = WrapHostObserver(fake);
    ASSERT_NE(nullptr, adapter);

    adapter->OnStateChanged(1, 2);
    adapter->OnFullStateChanged(3, 4);
    NearlinkBaseRemoteDevice device(TEST_DEVICE_ADDR, 1);
    adapter->OnPairConfirmed(device, 5, 6);
    adapter->OnDeviceNameChanged("name-1");
    adapter->OnDeviceAddrChanged(TEST_DEVICE_ADDR2);

    EXPECT_EQ(1, fake->stateChangedCount_);
    EXPECT_EQ(1, fake->lastTransport_);
    EXPECT_EQ(2, fake->lastStatus_);
    EXPECT_EQ(1, fake->fullStateChangedCount_);
    EXPECT_EQ(3, fake->lastFullTransport_);
    EXPECT_EQ(4, fake->lastFullStatus_);
    EXPECT_EQ(1, fake->pairConfirmedCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastPairAddr_);
    EXPECT_EQ(5, fake->lastReqType_);
    EXPECT_EQ(6, fake->lastNumber_);
    EXPECT_EQ(1, fake->deviceNameCount_);
    EXPECT_EQ("name-1", fake->lastDeviceName_);
    EXPECT_EQ(1, fake->deviceAddrCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR2, fake->lastAddress_);
    HILOGI("NearlinkObserverAdapterTest: HostAdapterForwardCallbacks001 end");
}

/**
 * @tc.name: HostAdapterNullObserver001
 * @tc.desc: 观察者已销毁时 5 个回调走空腿不崩溃（顺带触发内部 Erase）
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, HostAdapterNullObserver001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: HostAdapterNullObserver001 start");
    std::shared_ptr<NearlinkBaseHostObserver> adapter;
    std::weak_ptr<NearlinkHostObserver> weakFake;
    {
        auto fake = std::make_shared<FakeHostObserver>();
        adapter = WrapHostObserver(fake);
        ASSERT_NE(nullptr, adapter);
        weakFake = fake;
    }
    EXPECT_TRUE(weakFake.expired());
    adapter->OnStateChanged(1, 2);
    adapter->OnFullStateChanged(3, 4);
    NearlinkBaseRemoteDevice device(TEST_DEVICE_ADDR, 1);
    adapter->OnPairConfirmed(device, 5, 6);
    adapter->OnDeviceNameChanged("name-1");
    adapter->OnDeviceAddrChanged(TEST_DEVICE_ADDR2);
    HILOGI("NearlinkObserverAdapterTest: HostAdapterNullObserver001 end");
}

/**
 * @tc.name: HostAdapterUnwrap001
 * @tc.desc: 已注册观察者 Unwrap 成功取回适配器
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, HostAdapterUnwrap001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: HostAdapterUnwrap001 start");
    auto fake = std::make_shared<FakeHostObserver>();
    auto adapter = WrapHostObserver(fake);
    ASSERT_NE(nullptr, adapter);
    std::shared_ptr<NearlinkBaseHostObserver> found;
    EXPECT_TRUE(UnwrapHostObserver(fake, found));
    EXPECT_EQ(adapter.get(), found.get());
    HILOGI("NearlinkObserverAdapterTest: HostAdapterUnwrap001 end");
}

/**
 * @tc.name: HostAdapterUnwrapNotRegistered001
 * @tc.desc: 未注册观察者 Unwrap 返回失败
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, HostAdapterUnwrapNotRegistered001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: HostAdapterUnwrapNotRegistered001 start");
    auto fake = std::make_shared<FakeHostObserver>();
    std::shared_ptr<NearlinkBaseHostObserver> found;
    EXPECT_FALSE(UnwrapHostObserver(fake, found));
    HILOGI("NearlinkObserverAdapterTest: HostAdapterUnwrapNotRegistered001 end");
}

/**
 * @tc.name: HostAdapterWrapDuplicate001
 * @tc.desc: 同一观察者重复 Wrap 返回空指针
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, HostAdapterWrapDuplicate001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: HostAdapterWrapDuplicate001 start");
    auto fake = std::make_shared<FakeHostObserver>();
    EXPECT_NE(nullptr, WrapHostObserver(fake));
    EXPECT_EQ(nullptr, WrapHostObserver(fake));
    HILOGI("NearlinkObserverAdapterTest: HostAdapterWrapDuplicate001 end");
}

/**
 * @tc.name: HostAdapterErase001
 * @tc.desc: Erase 后 Unwrap 失败，重复 Erase 不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, HostAdapterErase001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: HostAdapterErase001 start");
    auto fake = std::make_shared<FakeHostObserver>();
    EXPECT_NE(nullptr, WrapHostObserver(fake));
    EraseHostObserver(fake);
    std::shared_ptr<NearlinkBaseHostObserver> found;
    EXPECT_FALSE(UnwrapHostObserver(fake, found));
    EraseHostObserver(fake); // 二次 Erase 应静默通过
    HILOGI("NearlinkObserverAdapterTest: HostAdapterErase001 end");
}

/**
 * @tc.name: RemoteDeviceAdapterForwardStateCallbacks001
 * @tc.desc: 状态类 4 个回调转发且设备地址经 base 转换后保持一致
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RemoteDeviceAdapterForwardStateCallbacks001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterForwardStateCallbacks001 start");
    auto fake = std::make_shared<FakeRemoteDeviceObserver>();
    auto adapter = WrapRemoteDeviceObserver(fake);
    ASSERT_NE(nullptr, adapter);

    NearlinkBaseRemoteDevice device(TEST_DEVICE_ADDR, 1);
    adapter->OnAcbStateChanged(device, 1, 2);
    adapter->OnPairingRequest(device, "123456", 3);
    adapter->OnPairStatusChanged(device, 4, 5, 6);
    adapter->OnConnectionStateChanged(device, 7, 8, 9);

    EXPECT_EQ(1, fake->acbCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastAcbAddr_);
    EXPECT_EQ(1, fake->lastAcbState_);
    EXPECT_EQ(2, fake->lastAcbReason_);
    EXPECT_EQ(1, fake->pairingReqCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastPairingAddr_);
    EXPECT_EQ("123456", fake->lastPasskey_);
    EXPECT_EQ(3, fake->lastPairingType_);
    EXPECT_EQ(1, fake->pairStatusCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastPairStatusAddr_);
    EXPECT_EQ(4, fake->lastPairPreState_);
    EXPECT_EQ(5, fake->lastPairState_);
    EXPECT_EQ(6, fake->lastPairReason_);
    EXPECT_EQ(1, fake->connStateCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastConnAddr_);
    EXPECT_EQ(7, fake->lastConnPreState_);
    EXPECT_EQ(8, fake->lastConnState_);
    EXPECT_EQ(9, fake->lastConnReason_);
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterForwardStateCallbacks001 end");
}

/**
 * @tc.name: RemoteDeviceAdapterForwardInfoCallbacks001
 * @tc.desc: 信息类 5 个回调（uuid/name/alias/rssi/freqBand）转发且参数一致
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RemoteDeviceAdapterForwardInfoCallbacks001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterForwardInfoCallbacks001 start");
    auto fake = std::make_shared<FakeRemoteDeviceObserver>();
    auto adapter = WrapRemoteDeviceObserver(fake);
    ASSERT_NE(nullptr, adapter);

    NearlinkBaseRemoteDevice device(TEST_DEVICE_ADDR, 1);
    std::vector<UUID> uuids = {UUID(0x1122), UUID(0x3344)};
    adapter->OnRemoteUuidChanged(device, uuids);
    adapter->OnRemoteNameChanged(device, "dev-name");
    adapter->OnRemoteAliasChanged(device, "dev-alias");
    adapter->OnReadRemoteRssiEvent(device, -60, 0);
    adapter->OnLinkFreqBandChanged(device, 2);

    EXPECT_EQ(1, fake->uuidCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastUuidAddr_);
    ASSERT_EQ(2U, fake->lastUuids_.size());
    EXPECT_EQ(0x1122, fake->lastUuids_[0].GetLeastSig16Bits());
    EXPECT_EQ(0x3344, fake->lastUuids_[1].GetLeastSig16Bits());
    EXPECT_EQ(1, fake->remoteNameCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastRemoteNameAddr_);
    EXPECT_EQ("dev-name", fake->lastRemoteName_);
    EXPECT_EQ(1, fake->remoteAliasCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastAliasAddr_);
    EXPECT_EQ("dev-alias", fake->lastAlias_);
    EXPECT_EQ(1, fake->rssiCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastRssiAddr_);
    EXPECT_EQ(-60, fake->lastRssi_);
    EXPECT_EQ(0, fake->lastRssiStatus_);
    EXPECT_EQ(1, fake->freqBandCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastFreqAddr_);
    EXPECT_EQ(2, fake->lastFreqBand_);
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterForwardInfoCallbacks001 end");
}

/**
 * @tc.name: RemoteDeviceAdapterNullObserver001
 * @tc.desc: 观察者已销毁时 9 个回调走空腿不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RemoteDeviceAdapterNullObserver001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterNullObserver001 start");
    std::shared_ptr<NearlinkBaseRemoteDeviceObserver> adapter;
    {
        auto fake = std::make_shared<FakeRemoteDeviceObserver>();
        adapter = WrapRemoteDeviceObserver(fake);
        ASSERT_NE(nullptr, adapter);
    }
    NearlinkBaseRemoteDevice device(TEST_DEVICE_ADDR, 1);
    std::vector<UUID> uuids = {UUID(0x1122)};
    adapter->OnAcbStateChanged(device, 1, 2);
    adapter->OnPairingRequest(device, "123456", 3);
    adapter->OnPairStatusChanged(device, 4, 5, 6);
    adapter->OnConnectionStateChanged(device, 7, 8, 9);
    adapter->OnRemoteUuidChanged(device, uuids);
    adapter->OnRemoteNameChanged(device, "dev-name");
    adapter->OnRemoteAliasChanged(device, "dev-alias");
    adapter->OnReadRemoteRssiEvent(device, -60, 0);
    adapter->OnLinkFreqBandChanged(device, 2);
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterNullObserver001 end");
}

/**
 * @tc.name: RemoteDeviceAdapterUnwrap001
 * @tc.desc: 已注册观察者 Unwrap 成功取回适配器
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RemoteDeviceAdapterUnwrap001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterUnwrap001 start");
    auto fake = std::make_shared<FakeRemoteDeviceObserver>();
    auto adapter = WrapRemoteDeviceObserver(fake);
    ASSERT_NE(nullptr, adapter);
    std::shared_ptr<NearlinkBaseRemoteDeviceObserver> found;
    EXPECT_TRUE(UnwrapRemoteDeviceObserver(fake, found));
    EXPECT_EQ(adapter.get(), found.get());
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterUnwrap001 end");
}

/**
 * @tc.name: RemoteDeviceAdapterUnwrapNotRegistered001
 * @tc.desc: 未注册观察者 Unwrap 返回失败
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RemoteDeviceAdapterUnwrapNotRegistered001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterUnwrapNotRegistered001 start");
    auto fake = std::make_shared<FakeRemoteDeviceObserver>();
    std::shared_ptr<NearlinkBaseRemoteDeviceObserver> found;
    EXPECT_FALSE(UnwrapRemoteDeviceObserver(fake, found));
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterUnwrapNotRegistered001 end");
}

/**
 * @tc.name: RemoteDeviceAdapterWrapDuplicate001
 * @tc.desc: 同一观察者重复 Wrap 返回空指针
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RemoteDeviceAdapterWrapDuplicate001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterWrapDuplicate001 start");
    auto fake = std::make_shared<FakeRemoteDeviceObserver>();
    EXPECT_NE(nullptr, WrapRemoteDeviceObserver(fake));
    EXPECT_EQ(nullptr, WrapRemoteDeviceObserver(fake));
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterWrapDuplicate001 end");
}

/**
 * @tc.name: RemoteDeviceAdapterErase001
 * @tc.desc: Erase 后 Unwrap 失败，重复 Erase 不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RemoteDeviceAdapterErase001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterErase001 start");
    auto fake = std::make_shared<FakeRemoteDeviceObserver>();
    EXPECT_NE(nullptr, WrapRemoteDeviceObserver(fake));
    EraseRemoteDeviceObserver(fake);
    std::shared_ptr<NearlinkBaseRemoteDeviceObserver> found;
    EXPECT_FALSE(UnwrapRemoteDeviceObserver(fake, found));
    EraseRemoteDeviceObserver(fake); // 二次 Erase 应静默通过
    HILOGI("NearlinkObserverAdapterTest: RemoteDeviceAdapterErase001 end");
}

/**
 * @tc.name: BatteryAdapterForwardCallbacks001
 * @tc.desc: 电池 2 个回调转发且设备地址与电量一致
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, BatteryAdapterForwardCallbacks001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterForwardCallbacks001 start");
    auto fake = std::make_shared<FakeBatteryObserver>();
    auto adapter = WrapBatteryObserver(fake);
    ASSERT_NE(nullptr, adapter);

    NearlinkBaseRemoteDevice device(TEST_DEVICE_ADDR, 1);
    adapter->OnGetBatteryLevelEvent(device, 80);
    adapter->OnBatteryLevelChanged(device, 90);

    EXPECT_EQ(1, fake->getLevelCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastGetAddr_);
    EXPECT_EQ(80, fake->lastGetLevel_);
    EXPECT_EQ(1, fake->levelChangedCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastChangedAddr_);
    EXPECT_EQ(90, fake->lastChangedLevel_);
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterForwardCallbacks001 end");
}

/**
 * @tc.name: BatteryAdapterNullObserver001
 * @tc.desc: 观察者已销毁时 2 个回调走空腿不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, BatteryAdapterNullObserver001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterNullObserver001 start");
    std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> adapter;
    {
        auto fake = std::make_shared<FakeBatteryObserver>();
        adapter = WrapBatteryObserver(fake);
        ASSERT_NE(nullptr, adapter);
    }
    NearlinkBaseRemoteDevice device(TEST_DEVICE_ADDR, 1);
    adapter->OnGetBatteryLevelEvent(device, 80);
    adapter->OnBatteryLevelChanged(device, 90);
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterNullObserver001 end");
}

/**
 * @tc.name: BatteryAdapterUnwrap001
 * @tc.desc: 已注册观察者 Unwrap 成功取回适配器
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, BatteryAdapterUnwrap001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterUnwrap001 start");
    auto fake = std::make_shared<FakeBatteryObserver>();
    auto adapter = WrapBatteryObserver(fake);
    ASSERT_NE(nullptr, adapter);
    std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> found;
    EXPECT_TRUE(UnwrapBatteryObserver(fake, found));
    EXPECT_EQ(adapter.get(), found.get());
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterUnwrap001 end");
}

/**
 * @tc.name: BatteryAdapterUnwrapNotRegistered001
 * @tc.desc: 未注册观察者 Unwrap 返回失败
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, BatteryAdapterUnwrapNotRegistered001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterUnwrapNotRegistered001 start");
    auto fake = std::make_shared<FakeBatteryObserver>();
    std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> found;
    EXPECT_FALSE(UnwrapBatteryObserver(fake, found));
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterUnwrapNotRegistered001 end");
}

/**
 * @tc.name: BatteryAdapterWrapDuplicate001
 * @tc.desc: 同一观察者重复 Wrap 返回空指针
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, BatteryAdapterWrapDuplicate001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterWrapDuplicate001 start");
    auto fake = std::make_shared<FakeBatteryObserver>();
    EXPECT_NE(nullptr, WrapBatteryObserver(fake));
    EXPECT_EQ(nullptr, WrapBatteryObserver(fake));
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterWrapDuplicate001 end");
}

/**
 * @tc.name: BatteryAdapterErase001
 * @tc.desc: Erase 后 Unwrap 失败，重复 Erase 不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, BatteryAdapterErase001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterErase001 start");
    auto fake = std::make_shared<FakeBatteryObserver>();
    EXPECT_NE(nullptr, WrapBatteryObserver(fake));
    EraseBatteryObserver(fake);
    std::shared_ptr<NearlinkBaseRemoteDeviceBatteryObserver> found;
    EXPECT_FALSE(UnwrapBatteryObserver(fake, found));
    EraseBatteryObserver(fake); // 二次 Erase 应静默通过
    HILOGI("NearlinkObserverAdapterTest: BatteryAdapterErase001 end");
}

/**
 * @tc.name: RssiAdapterForwardCallback001
 * @tc.desc: Rssi 回调转发且设备地址、rssi、status 一致
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RssiAdapterForwardCallback001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterForwardCallback001 start");
    auto fake = std::make_shared<FakeRssiObserver>();
    auto adapter = WrapRssiObserver(fake);
    ASSERT_NE(nullptr, adapter);

    NearlinkBaseRemoteDevice device(TEST_DEVICE_ADDR, 1);
    adapter->OnReadRemoteRssiEvent(device, -70, 1);

    EXPECT_EQ(1, fake->rssiCount_);
    EXPECT_EQ(TEST_DEVICE_ADDR, fake->lastAddr_);
    EXPECT_EQ(-70, fake->lastRssi_);
    EXPECT_EQ(1, fake->lastStatus_);
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterForwardCallback001 end");
}

/**
 * @tc.name: RssiAdapterNullObserver001
 * @tc.desc: 观察者已销毁时回调走空腿不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RssiAdapterNullObserver001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterNullObserver001 start");
    std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> adapter;
    {
        auto fake = std::make_shared<FakeRssiObserver>();
        adapter = WrapRssiObserver(fake);
        ASSERT_NE(nullptr, adapter);
    }
    NearlinkBaseRemoteDevice device(TEST_DEVICE_ADDR, 1);
    adapter->OnReadRemoteRssiEvent(device, -70, 1);
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterNullObserver001 end");
}

/**
 * @tc.name: RssiAdapterUnwrap001
 * @tc.desc: 已注册观察者 Unwrap 成功取回适配器
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RssiAdapterUnwrap001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterUnwrap001 start");
    auto fake = std::make_shared<FakeRssiObserver>();
    auto adapter = WrapRssiObserver(fake);
    ASSERT_NE(nullptr, adapter);
    std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> found;
    EXPECT_TRUE(UnwrapRssiObserver(fake, found));
    EXPECT_EQ(adapter.get(), found.get());
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterUnwrap001 end");
}

/**
 * @tc.name: RssiAdapterUnwrapNotRegistered001
 * @tc.desc: 未注册观察者 Unwrap 返回失败
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RssiAdapterUnwrapNotRegistered001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterUnwrapNotRegistered001 start");
    auto fake = std::make_shared<FakeRssiObserver>();
    std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> found;
    EXPECT_FALSE(UnwrapRssiObserver(fake, found));
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterUnwrapNotRegistered001 end");
}

/**
 * @tc.name: RssiAdapterWrapDuplicate001
 * @tc.desc: 同一观察者重复 Wrap 返回空指针
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RssiAdapterWrapDuplicate001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterWrapDuplicate001 start");
    auto fake = std::make_shared<FakeRssiObserver>();
    EXPECT_NE(nullptr, WrapRssiObserver(fake));
    EXPECT_EQ(nullptr, WrapRssiObserver(fake));
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterWrapDuplicate001 end");
}

/**
 * @tc.name: RssiAdapterErase001
 * @tc.desc: Erase 后 Unwrap 失败，重复 Erase 不崩溃
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkObserverAdapterTest, RssiAdapterErase001, TestSize.Level1)
{
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterErase001 start");
    auto fake = std::make_shared<FakeRssiObserver>();
    EXPECT_NE(nullptr, WrapRssiObserver(fake));
    EraseRssiObserver(fake);
    std::shared_ptr<NearlinkBaseRemoteDeviceRssiObserver> found;
    EXPECT_FALSE(UnwrapRssiObserver(fake, found));
    EraseRssiObserver(fake); // 二次 Erase 应静默通过
    HILOGI("NearlinkObserverAdapterTest: RssiAdapterErase001 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
