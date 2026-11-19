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
#include <thread>

#include "log.h"
#include "BasService.h"
#include "BasClientStackAdapter.h"
#include "SleInterfaceManager.h"
#include "SleInterfaceProfileManager.h"
#include "ThreadUtil.h"
#include "ProfileServiceManager.cpp"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const std::string BAS_UT_ADDR = "00:11:22:33:44:55";
} // namespace

/**
 * @tc.number: BasServiceTest008
 * @tc.name: Test BasObserver Register and Deregister
 * @tc.desc: Test BasObserver registration
 */
class CountingBasObserver : public BasObserver {
public:
    int stateChangeCount = 0;
    int lastState = -1;
    int lastPreState = -1;
    void OnConnectionStateChanged(
        const RawAddress &addr, int32_t state, int32_t preState) override
    {
        stateChangeCount++;
        lastState = state;
        lastPreState = preState;
    }
};

/**
 * @tc.number: BasServiceTest009
 * @tc.name: Test IDeviceBatteryCallback Register and Deregister
 * @tc.desc: Test IDeviceBatteryCallback registration
 */
class CountingDeviceBatteryObserver : public IDeviceBatteryCallback {
public:
    int levelEventCount = 0;
    int8_t lastEventLevel = -1;
    int levelChangedCount = 0;
    int8_t lastChangedLevel = -1;
    void OnGetBatteryLevelEvent(const RawAddress &addr, int8_t batteryLevel) override
    {
        levelEventCount++;
        lastEventLevel = batteryLevel;
    }
    void OnBatteryLevelChanged(const RawAddress &addr, int8_t batteryLevel) override
    {
        levelChangedCount++;
        lastChangedLevel = batteryLevel;
    }
};

class BasServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    // 测试目的：fixture 持有自有 BasService 实例（cdsm_test 样板），
    // 不依赖厂商栈启动链（裸进程协议栈初始化必败，Start 后服务实例为空）。
    std::unique_ptr<BasService> service_;
};

void BasServiceTest::SetUpTestCase()
{
    // 测试目的：删除 SleInterfaceManager::Start()/Stop()（裸进程必败且返回值被忽略）与固定 sleep；
    // 服务实例改由每例 SetUp 自建并注入注册表。
    HILOGI("SetUpTestCase BasServiceTest start");
}

void BasServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase BasServiceTest");
}

void BasServiceTest::SetUp()
{
    // 测试目的：先让 BAS 任务在测试线程同步执行（漏插键会让异步用例假绿），再自建服务注入注册表。
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_BAS, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    service_ = std::make_unique<BasService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_BAS, service_.get());
    ASSERT_EQ(service_.get(), BasService::GetInstance());
}

void BasServiceTest::TearDown()
{
    // 测试目的：先解绑注册表再销毁服务，并恢复线程调度默认态，防单例状态跨例残留。
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_BAS, nullptr);
    service_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
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
    basService->Disable();
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

    RawAddress rawAddress(BAS_UT_ADDR);

    basService->Enable();
    int ret = basService->Connect(rawAddress);
    EXPECT_EQ(ret, 0);

    basService->Disable();
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

    RawAddress rawAddress(BAS_UT_ADDR);

    basService->Enable();
    int ret = basService->Disconnect(rawAddress);
    EXPECT_EQ(ret, 0);

    basService->Disable();
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

    RawAddress rawAddress(BAS_UT_ADDR);

    basService->Enable();
    basService->GetDeviceBatteryLevel(rawAddress);

    basService->Disable();
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
    std::list<RawAddress> devices = basService->GetConnectDevices();
    EXPECT_GE(devices.size(), 0);

    basService->Disable();
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
    int state = basService->GetConnectState();
    EXPECT_EQ(state, static_cast<int>(SleConnectState::DISCONNECTED));

    basService->Disable();
}

/**
 * @tc.number: BasServiceTest008
 * @tc.name: Test BasObserver Register and Deregister
 * @tc.desc: Test BasObserver registration
 */
HWTEST_F(BasServiceTest, BasServiceTest008, TestSize.Level1)
{
    HILOGI("BasServiceTest008 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    CountingBasObserver observer;
    basService->RegisterObserver(observer);
    basService->DeregisterObserver(observer);
}

/**
 * @tc.number: BasServiceTest009
 * @tc.name: Test IDeviceBatteryCallback Register and Deregister
 * @tc.desc: Test IDeviceBatteryCallback registration
 */
HWTEST_F(BasServiceTest, BasServiceTest009, TestSize.Level1)
{
    HILOGI("BasServiceTest009 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    CountingDeviceBatteryObserver observer;
    basService->RegisterDeviceObserver(observer);
    basService->DeregisterDeviceObserver(observer);
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

    RawAddress rawAddress(BAS_UT_ADDR);

    CountingBasObserver observer;
    basService->RegisterObserver(observer);

    basService->NotifyStateChanged(
        rawAddress, SleConnectState::CONNECTED, SleConnectState::CONNECTING);
    EXPECT_EQ(1, observer.stateChangeCount);
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTED), observer.lastState);

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

    RawAddress rawAddress(BAS_UT_ADDR);

    CountingDeviceBatteryObserver observer;
    basService->RegisterDeviceObserver(observer);

    basService->NotifyBatteryLevelEvent(rawAddress, 80);
    EXPECT_EQ(1, observer.levelEventCount);
    EXPECT_EQ(80, observer.lastEventLevel);

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

    RawAddress rawAddress(BAS_UT_ADDR);

    CountingDeviceBatteryObserver observer;
    basService->RegisterDeviceObserver(observer);

    basService->NotifyBatteryLevelChanged(rawAddress, 60);
    EXPECT_EQ(1, observer.levelChangedCount);
    EXPECT_EQ(60, observer.lastChangedLevel);

    basService->DeregisterDeviceObserver(observer);
}

/**
 * @tc.number: BasServiceTest013
 * @tc.name: Test BasClientStackAdapter RegisterCallBackToStack
 * @tc.desc: Test BasClientStackAdapter callback registration against stack
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
    basService->Enable();
    basService->Disable();
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
    basService->Disable();
    basService->Disable();
}

/**
 * @tc.number: BasEnableDisableMatrix001
 * @tc.name: 电量服务开关状态矩阵
 * @tc.desc: 未使能先去使能、反复开关切换均按已处理返回不崩溃.
 */
HWTEST_F(BasServiceTest, BasEnableDisableMatrix001, TestSize.Level1)
{
    HILOGI("BasEnableDisableMatrix001 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    basService->Disable();
    basService->Enable();
    basService->Enable();
    basService->Disable();
    basService->Disable();
    HILOGI("BasEnableDisableMatrix001 end");
}

/**
 * @tc.number: BasNotifyStateChanged002
 * @tc.name: 非连接建立的状态迁移仅做观察者分发
 * @tc.desc: 已断开到连接中迁移不进入连接建立分支，观察者仍收到状态回调.
 */
HWTEST_F(BasServiceTest, BasNotifyStateChanged002, TestSize.Level1)
{
    HILOGI("BasNotifyStateChanged002 start");
    BasService *basService = BasService::GetInstance();
    ASSERT_NE(basService, nullptr);

    RawAddress rawAddress(BAS_UT_ADDR);
    CountingBasObserver observer;
    basService->RegisterObserver(observer);

    basService->NotifyStateChanged(
        rawAddress, SleConnectState::CONNECTING, SleConnectState::DISCONNECTED);
    EXPECT_EQ(1, observer.stateChangeCount);
    EXPECT_EQ(static_cast<int>(SleConnectState::CONNECTING), observer.lastState);
    EXPECT_EQ(static_cast<int>(SleConnectState::DISCONNECTED), observer.lastPreState);

    basService->DeregisterObserver(observer);
    HILOGI("BasNotifyStateChanged002 end");
}

/**
 * @tc.number: BasAdapterOnConnectStateChange001
 * @tc.name: 栈侧断链回调触发配对上报与服务状态分发
 * @tc.desc: 连接中到已断开迁移通知服务观察者，普通迁移仅做状态分发.
 */
HWTEST_F(BasServiceTest, BasAdapterOnConnectStateChange001, TestSize.Level1)
{
    HILOGI("BasAdapterOnConnectStateChange001 start");
    SLE_Addr_S addr = {};
    RawAddress device(BAS_UT_ADDR);
    device.ConvertToUint8(addr.addr);

    CountingBasObserver observer;
    BasService::GetInstance()->RegisterObserver(observer);

    BasClientStackAdapter::OnBasConnectStateChangeCbk(&addr, BAS_DISCONNECTED, BAS_CONNECTING,
        NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(1, observer.stateChangeCount);

    BasClientStackAdapter::OnBasConnectStateChangeCbk(&addr, BAS_CONNECTING, BAS_DISCONNECTED,
        NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(2, observer.stateChangeCount);

    BasClientStackAdapter::OnBasConnectStateChangeCbk(nullptr, BAS_DISCONNECTED, BAS_CONNECTING,
        NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(2, observer.stateChangeCount);

    BasService::GetInstance()->DeregisterObserver(observer);
    HILOGI("BasAdapterOnConnectStateChange001 end");
}

/**
 * @tc.number: BasAdapterOnGetPropertyCbk001
 * @tc.name: 栈侧电量读取回调对空值与有效值的分发
 * @tc.desc: 空电量指针按无效值上报，有效电量百分比按栈值上报.
 */
HWTEST_F(BasServiceTest, BasAdapterOnGetPropertyCbk001, TestSize.Level1)
{
    HILOGI("BasAdapterOnGetPropertyCbk001 start");
    SLE_Addr_S addr = {};
    RawAddress device(BAS_UT_ADDR);
    device.ConvertToUint8(addr.addr);

    CountingDeviceBatteryObserver observer;
    BasService::GetInstance()->RegisterDeviceObserver(observer);

    BasClientStackAdapter::OnBasGetPropertyCbk(&addr, BAS_BATTERY_PERCENTAGE, nullptr,
        NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(1, observer.levelEventCount);
    EXPECT_EQ(-1, observer.lastEventLevel);

    uint8_t battery = 88;
    NLSTK_VariableData_S batteryData {};
    batteryData.data = &battery;
    batteryData.len = 1;
    BasClientStackAdapter::OnBasGetPropertyCbk(&addr, BAS_BATTERY_PERCENTAGE, &batteryData,
        NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(2, observer.levelEventCount);
    EXPECT_EQ(88, observer.lastEventLevel);

    BasService::GetInstance()->DeregisterDeviceObserver(observer);
    HILOGI("BasAdapterOnGetPropertyCbk001 end");
}

/**
 * @tc.number: BasAdapterOnGetPropertyCbk002
 * @tc.name: 栈侧电量读取回调的防御分支
 * @tc.desc: 空地址直接返回，非电量属性与失败结果不上报电量事件.
 */
HWTEST_F(BasServiceTest, BasAdapterOnGetPropertyCbk002, TestSize.Level1)
{
    HILOGI("BasAdapterOnGetPropertyCbk002 start");
    SLE_Addr_S addr = {};
    RawAddress device(BAS_UT_ADDR);
    device.ConvertToUint8(addr.addr);

    CountingDeviceBatteryObserver observer;
    BasService::GetInstance()->RegisterDeviceObserver(observer);

    uint8_t battery = 66;
    NLSTK_VariableData_S batteryData {};
    batteryData.data = &battery;
    batteryData.len = 1;

    BasClientStackAdapter::OnBasGetPropertyCbk(nullptr, BAS_BATTERY_PERCENTAGE, &batteryData,
        NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(0, observer.levelEventCount);

    BasClientStackAdapter::OnBasGetPropertyCbk(&addr, BAS_BATTERY_CAPACITY, &batteryData,
        NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(0, observer.levelEventCount);

    BasClientStackAdapter::OnBasGetPropertyCbk(&addr, BAS_BATTERY_PERCENTAGE, &batteryData,
        NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(0, observer.levelEventCount);

    BasService::GetInstance()->DeregisterDeviceObserver(observer);
    HILOGI("BasAdapterOnGetPropertyCbk002 end");
}

/**
 * @tc.number: BasAdapterOnPropertyChangedCbk001
 * @tc.name: 栈侧电量变化回调的电量百分比分发
 * @tc.desc: 电量百分比属性按栈值上报变化事件，空值与空数据被拒绝.
 */
HWTEST_F(BasServiceTest, BasAdapterOnPropertyChangedCbk001, TestSize.Level1)
{
    HILOGI("BasAdapterOnPropertyChangedCbk001 start");
    SLE_Addr_S addr = {};
    RawAddress device(BAS_UT_ADDR);
    device.ConvertToUint8(addr.addr);

    CountingDeviceBatteryObserver observer;
    BasService::GetInstance()->RegisterDeviceObserver(observer);

    uint8_t battery = 55;
    NLSTK_VariableData_S batteryData {};
    batteryData.data = &battery;
    batteryData.len = 1;

    BasClientStackAdapter::OnBasPropertyChangedCbk(&addr, BAS_BATTERY_PERCENTAGE, &batteryData);
    EXPECT_EQ(1, observer.levelChangedCount);
    EXPECT_EQ(55, observer.lastChangedLevel);

    BasClientStackAdapter::OnBasPropertyChangedCbk(&addr, BAS_BATTERY_PERCENTAGE, nullptr);
    EXPECT_EQ(1, observer.levelChangedCount);

    BasClientStackAdapter::OnBasPropertyChangedCbk(&addr, BAS_BATTERY_CAPACITY, &batteryData);
    EXPECT_EQ(1, observer.levelChangedCount);

    BasService::GetInstance()->DeregisterDeviceObserver(observer);
    HILOGI("BasAdapterOnPropertyChangedCbk001 end");
}

}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS
