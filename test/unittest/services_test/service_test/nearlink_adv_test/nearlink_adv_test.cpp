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
#include "SleAdvertiserAdapter.h"
#include <tuple>
#include <thread>
#include "nlstk_devd_def.h"
#include "nlstk_devd_api.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class NearlinkAdvTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

enum class AdvStatusTest : int {
    ADV_START_EVENT_TEST = 0,
    ADV_STOP_EVENT_TEST = 1,
    ADV_ENABLE_EVENT_TEST = 2,
    ADV_DISABLE_EVENT_TEST = 3,
    ADV_SETDATA_EVENT_TEST = 4,
    ADV_EVENT_TEST_MAX = 5,
};

static std::tuple<int, int, uint8_t> advEventTestTuple_(static_cast<int>(AdvStatusTest::ADV_EVENT_TEST_MAX), -1, 0xFF);
static void SetAdvEventTestTuple (int status, int result, uint8_t advHandle)
{
    HILOGI("status = %{public}d, result = %{public}d, advHandle = %{public}d", status, result, advHandle);
    std::get<0>(advEventTestTuple_) = status;
    std::get<1>(advEventTestTuple_) = result;
    std::get<2>(advEventTestTuple_) = advHandle;
}
static void ResetAdvEventTestTuple()
{
    std::get<0>(advEventTestTuple_) = static_cast<int>(AdvStatusTest::ADV_EVENT_TEST_MAX);
    std::get<1>(advEventTestTuple_) = -1;
    std::get<2>(advEventTestTuple_) = 0xFF;
}

static std::tuple<int, int, uint8_t> connectableEventTestTuple_(
    static_cast<int>(AdvStatusTest::ADV_EVENT_TEST_MAX), -1, 0xFF);
static void SetConnectableEventTestTuple (int status, int result, uint8_t advHandle)
{
    HILOGI("status = %{public}d, result = %{public}d, advHandle = %{public}d", status, result, advHandle);
    std::get<0>(connectableEventTestTuple_) = status;
    std::get<1>(connectableEventTestTuple_) = result;
    std::get<2>(connectableEventTestTuple_) = advHandle;
}
static void ResetConnectableEventTestTuple()
{
    std::get<0>(connectableEventTestTuple_) = static_cast<int>(AdvStatusTest::ADV_EVENT_TEST_MAX);
    std::get<1>(connectableEventTestTuple_) = -1;
    std::get<2>(connectableEventTestTuple_) = 0xFF;
}

class MockAdvertiserCallbackTest : public ISleAdvertiserCallback {
public:
    void OnStartResultEvent(int result, uint8_t advHandle) override
    {
        SetAdvEventTestTuple(static_cast<int>(AdvStatusTest::ADV_START_EVENT_TEST), result, advHandle);
    }
    void OnStopResultEvent(int result, uint8_t advHandle)
    {
        SetAdvEventTestTuple(static_cast<int>(AdvStatusTest::ADV_STOP_EVENT_TEST), result, advHandle);
    }
    void OnEnableResultEvent(int result, uint8_t advHandle)
    {
        SetAdvEventTestTuple(static_cast<int>(AdvStatusTest::ADV_ENABLE_EVENT_TEST), result, advHandle);
    }
    void OnDisableResultEvent(int result, uint8_t advHandle)
    {
        SetAdvEventTestTuple(static_cast<int>(AdvStatusTest::ADV_DISABLE_EVENT_TEST), result, advHandle);
    }
    void OnSetAdvDataEvent(int result, uint8_t advHandle)
    {
        SetAdvEventTestTuple(static_cast<int>(AdvStatusTest::ADV_SETDATA_EVENT_TEST), result, advHandle);
    }
    void OnAutoStopAdvEvent(uint8_t advHandle) override {};
};

class MockConnectableCallbackTest : public ISleAdvertiserCallback {
public:
    void OnStartResultEvent(int result, uint8_t advHandle) override
    {
        SetConnectableEventTestTuple(static_cast<int>(AdvStatusTest::ADV_START_EVENT_TEST), result, advHandle);
    }
    void OnStopResultEvent(int result, uint8_t advHandle)
    {
        SetConnectableEventTestTuple(static_cast<int>(AdvStatusTest::ADV_STOP_EVENT_TEST), result, advHandle);
    }
    void OnEnableResultEvent(int result, uint8_t advHandle)
    {
        SetConnectableEventTestTuple(static_cast<int>(AdvStatusTest::ADV_ENABLE_EVENT_TEST), result, advHandle);
    }
    void OnDisableResultEvent(int result, uint8_t advHandle)
    {
        SetConnectableEventTestTuple(static_cast<int>(AdvStatusTest::ADV_DISABLE_EVENT_TEST), result, advHandle);
    }
    void OnSetAdvDataEvent(int result, uint8_t advHandle)
    {
        SetConnectableEventTestTuple(static_cast<int>(AdvStatusTest::ADV_SETDATA_EVENT_TEST), result, advHandle);
    }
    void OnAutoStopAdvEvent(uint8_t advHandle) override {};
};

static std::shared_ptr<MockAdvertiserCallbackTest> advEventTestImp_ = nullptr;
static std::shared_ptr<MockConnectableCallbackTest> connectableEventTestImp_ = nullptr;
static std::unique_ptr<SleAdvertiserImpl> sleAdvertiserImp_ = nullptr;
static std::shared_ptr<MockAdvertiserCallbackTest> advEventTestImpForAdvImp_ = nullptr;

void NearlinkAdvTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkAdvTest.");
    advEventTestImp_ = std::make_shared<MockAdvertiserCallbackTest>();
    connectableEventTestImp_ = std::make_shared<MockConnectableCallbackTest>();
    sleAdvertiserImp_ = std::make_unique<SleAdvertiserImpl>();
    advEventTestImpForAdvImp_ = std::make_shared<MockAdvertiserCallbackTest>();
    sleAdvertiserImp_->RegisterSleAdvertiserCallback(advEventTestImpForAdvImp_);
}

void NearlinkAdvTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkAdvTest");
    uint8_t handle = 0xFF;
    (void)NLSTK_DevdRemoveAdv(&handle);
}

void NearlinkAdvTest::SetUp()
{
    HILOGI("SetUp NearlinkAdvTest.");
}

void NearlinkAdvTest::TearDown()
{
    HILOGI("TearDown NearlinkAdvTest.");
}

/**
 * @tc.name: Sle_AdvertiserAdapter_Test001
 * @tc.desc: 广播初始化
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvTest, Sle_AdvertiserAdapter_Test001, TestSize.Level1)
{
    HILOGI("Sle_AdvertiserAdapter_Test001 start");
    InterfaceAdvertiserService::GetInstance().RegisterSleAdvertiserCallback(advEventTestImp_);
    InterfaceAdvertiserService::GetInstance().RegisterSleConnectableAdvertiserCallback(connectableEventTestImp_);
    InterfaceAdvertiserService::GetInstance().DeregisterSleAdvertiserCallback();
    uint8_t advHandle = InterfaceAdvertiserService::GetInstance().GetAdvertiserHandle();
    uint8_t connectableHandle = InterfaceAdvertiserService::GetInstance().GetConnectableAdvertiserHandle();
    EXPECT_EQ(advHandle, 0);
    EXPECT_EQ(connectableHandle, 1);
    int advStatus = InterfaceAdvertiserService::GetInstance().GetAdvertisingStatus();
    EXPECT_EQ(advStatus, static_cast<int>(SleAdvState::SLE_ADV_STATE_IDLE));
    HILOGI("Sle_AdvertiserAdapter_Test001 end");
}

/**
 * @tc.name: Sle_AdvertiserAdapter_Test002
 * @tc.desc: 起广播
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvTest, Sle_AdvertiserAdapter_Test002, TestSize.Level1)
{
    HILOGI("Sle_AdvertiserAdapter_Test002 start");
    SleAdvertiserSettingsImpl advSettings;
    SleAdvertiserDataImpl advData;
    SleAdvertiserDataImpl advResponseData;
    uint8_t advHandle = 0;
    ResetAdvEventTestTuple();
    // 起广播，advHandleSettingDatas_存入handle为0，启动中
    InterfaceAdvertiserService::GetInstance().StartAdvertising(advSettings, advData, advResponseData, advHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<0>(advEventTestTuple_), static_cast<int>(AdvStatusTest::ADV_EVENT_TEST_MAX));
    // 重复起广播
    InterfaceAdvertiserService::GetInstance().StartAdvertising(advSettings, advData, advResponseData, advHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_ALREADY_STARTED));
    // 异常起广播，advHandleSettingDatas_存入handle为2，启动中
    advHandle = 2;
    advSettings.SetInterval(static_cast<int>(AdvInterval::ADV_SLE_CONNECTABLE_ADV_INTERBAL));
    InterfaceAdvertiserService::GetInstance().StartAdvertising(advSettings, advData, advResponseData, advHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR));
    advSettings.SetLegacyMode(false);
    std::string str(0x100, ' ');
    advData.SetPayload(str);
    advHandle = 3;
    InterfaceAdvertiserService::GetInstance().StartAdvertising(advSettings, advData, advResponseData, advHandle);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_CHECK_PARA_FAIL));
    HILOGI("Sle_AdvertiserAdapter_Test002 end");
}

/**
 * @tc.name: Sle_AdvertiserAdapter_Test003
 * @tc.desc: 更新广播数据
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvTest, Sle_AdvertiserAdapter_Test003, TestSize.Level1)
{
    HILOGI("Sle_AdvertiserAdapter_Test003 start");
    SleAdvertiserDataImpl advData;
    SleAdvertiserDataImpl advResponseData;
    // SetAdvertisingData
    InterfaceAdvertiserService::GetInstance().SetAdvertisingData(advData, advResponseData, 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_INVALID_HANDLE));
    EXPECT_EQ(std::get<2>(advEventTestTuple_), 3);
    InterfaceAdvertiserService::GetInstance().SetAdvertisingData(advData, advResponseData, 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_NOT_STARTED));
    // 构造handle = 0广播已启动成功
    NLSTK_DevdAdvCbkParam_S cbkParam = {SLE_DD_ADV_ENABLE_COMPLETE_EVT, 0, NLSTK_ERRCODE_SUCCESS};
    SleAdvertiserImpl::AdvEventResult(&cbkParam);
    std::string str(0x0F, ' ');
    advData.SetPayload(str);
    InterfaceAdvertiserService::GetInstance().SetAdvertisingData(advData, advResponseData, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_SUCCESS));
    str = " ";
    advData.SetPayload(str);
    InterfaceAdvertiserService::GetInstance().SetAdvertisingData(advData, advResponseData, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_NOT_STARTED));
    HILOGI("Sle_AdvertiserAdapter_Test003 end");
}

/**
 * @tc.name: Sle_AdvertiserAdapter_Test004
 * @tc.desc: 停广播
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvTest, Sle_AdvertiserAdapter_Test004, TestSize.Level1)
{
    HILOGI("Sle_AdvertiserAdapter_Test004 start");
    InterfaceAdvertiserService::GetInstance().StopAdvertising(3);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<0>(advEventTestTuple_), static_cast<int>(AdvStatusTest::ADV_STOP_EVENT_TEST));
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_SUCCESS));
    InterfaceAdvertiserService::GetInstance().StopAdvertising(2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_NOT_STARTED));
    // handle = 0广播停止中
    InterfaceAdvertiserService::GetInstance().StopAdvertising(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_NOT_STARTED));
    int advStatus = InterfaceAdvertiserService::GetInstance().GetAdvertisingStatus();
    EXPECT_EQ(advStatus, static_cast<int>(SleAdvState::SLE_ADV_STATE_ADVERTISING));
    InterfaceAdvertiserService::GetInstance().StopAdvertisingAll();
    // 构造handle = 0和handle = 2广播已停止，handle = 3停止的构造为跑异常分支
    NLSTK_DevdAdvCbkParam_S cbkParam0 = {SLE_DD_ADV_REMOVE_COMPLETE_EVT, 0, NLSTK_ERRCODE_SUCCESS};
    NLSTK_DevdAdvCbkParam_S cbkParam2 = {SLE_DD_ADV_REMOVE_COMPLETE_EVT, 2, NLSTK_ERRCODE_SUCCESS};
    NLSTK_DevdAdvCbkParam_S cbkParam3 = {SLE_DD_ADV_REMOVE_COMPLETE_EVT, 3, NLSTK_ERRCODE_SUCCESS};
    SleAdvertiserImpl::AdvEventResult(&cbkParam0);
    SleAdvertiserImpl::AdvEventResult(&cbkParam2);
    SleAdvertiserImpl::AdvEventResult(&cbkParam3);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<0>(advEventTestTuple_), static_cast<int>(AdvStatusTest::ADV_STOP_EVENT_TEST));
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_SUCCESS));
    EXPECT_EQ(std::get<2>(advEventTestTuple_), 2); 
    HILOGI("Sle_AdvertiserAdapter_Test004 end");
}

/**
 * @tc.name: Sle_AdvertiserImpl_Test001
 * @tc.desc: HandleDdEvent回调
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvTest, Sle_AdvertiserImpl_Test001, TestSize.Level1)
{
    HILOGI("Sle_AdvertiserImpl_Test001 start");
    ResetAdvEventTestTuple();
    SleAdvertiserSettingsImpl advSettings;
    SleAdvertiserDataImpl advData;
    SleAdvertiserDataImpl advResponseData;
    // 起广播，advHandleSettingDatas_存入handle = 0的数据，状态STARTTING
    InterfaceAdvertiserService::GetInstance().StartAdvertising(advSettings, advData, advResponseData, 0);
    // 不会回调，invalid handle
    NLSTK_DevdAdvCbkParam_S cbkParam2 = {SLE_DD_ADV_DATA_SET_COMPLETE_EVT, 2, NLSTK_ERRCODE_SUCCESS};
    SleAdvertiserImpl::AdvEventResult(&cbkParam2);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), -1);

    // 不会回调，因为当前状态不是UPDATING
    NLSTK_DevdAdvCbkParam_S cbkParam0 = {SLE_DD_ADV_DATA_SET_COMPLETE_EVT, 0, NLSTK_ERRCODE_SUCCESS};
    SleAdvertiserImpl::AdvEventResult(&cbkParam0);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), -1);
    ResetAdvEventTestTuple();
    // 不会回调，invalid handle
    cbkParam2.event = SLE_DD_ADV_SCAN_RSP_DATA_SET_COMPLETE_EVT;
    SleAdvertiserImpl::AdvEventResult(&cbkParam2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), -1);
    // 不会回调，invalid handle
    cbkParam2.event = SLE_DD_ADV_DISABLE_COMPLETE_EVT;
    SleAdvertiserImpl::AdvEventResult(&cbkParam2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), -1);
    cbkParam0.result = NLSTK_ERRCODE_FAIL;
    // DISABLE失败，状态STARTED
    cbkParam0.event = SLE_DD_ADV_DISABLE_COMPLETE_EVT;
    SleAdvertiserImpl::AdvEventResult(&cbkParam0);
    cbkParam0.result = NLSTK_ERRCODE_SUCCESS;
    // 目标状态不对，RemoveAdvHandle，状态REMOVING
    cbkParam0.event = SLE_DD_ADV_DISABLE_COMPLETE_EVT;
    SleAdvertiserImpl::AdvEventResult(&cbkParam0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR));
    ResetAdvEventTestTuple();
    cbkParam2.event = SLE_DD_ADV_CLEAR_COMPLETE_EVT;
    SleAdvertiserImpl::AdvEventResult(&cbkParam2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), -1);
    cbkParam2.event = SLE_DD_ADV_TERMINATED_COMPLETE_EVT;
    SleAdvertiserImpl::AdvEventResult(&cbkParam2);
    // RemoveAdvHandle，状态REMOVING
    cbkParam0.event = SLE_DD_ADV_TERMINATED_COMPLETE_EVT;
    SleAdvertiserImpl::AdvEventResult(&cbkParam0);
    cbkParam2.event = 0xFF;
    SleAdvertiserImpl::AdvEventResult(&cbkParam2);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), -1);
    HILOGI("Sle_AdvertiserImpl_Test001 end");
}

/**
 * @tc.name: Sle_AdvertiserImpl_Test002
 * @tc.desc: on回调connectable分支验证
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvTest, Sle_AdvertiserImpl_Test002, TestSize.Level1)
{
    HILOGI("Sle_AdvertiserImpl_Test002 start");
    ResetAdvEventTestTuple();
    ResetConnectableEventTestTuple();
    SleAdvertiserSettingsImpl advSettings;
    SleAdvertiserDataImpl advData;
    SleAdvertiserDataImpl advResponseData;
    // invalid handle使能广播
    InterfaceAdvertiserService::GetInstance().EnableAdvertising(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<0>(connectableEventTestTuple_), static_cast<int>(AdvStatusTest::ADV_ENABLE_EVENT_TEST));
    // invalid handle去使能广播
    InterfaceAdvertiserService::GetInstance().DisableAdvertising(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<0>(connectableEventTestTuple_), static_cast<int>(AdvStatusTest::ADV_DISABLE_EVENT_TEST));
    // invalid handle更新广播数据
    InterfaceAdvertiserService::GetInstance().SetAdvertisingData(advData, advResponseData, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<0>(connectableEventTestTuple_), static_cast<int>(AdvStatusTest::ADV_SETDATA_EVENT_TEST));
    // 起广播，advHandleSettingDatas_存入handle = 1的数据，状态启动中
    InterfaceAdvertiserService::GetInstance().StartAdvertising(advSettings, advData, advResponseData, 1);
    // 重复起广播
    InterfaceAdvertiserService::GetInstance().StartAdvertising(advSettings, advData, advResponseData, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<0>(connectableEventTestTuple_), static_cast<int>(AdvStatusTest::ADV_START_EVENT_TEST));
    // 停广播
    InterfaceAdvertiserService::GetInstance().StopAdvertising(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<0>(connectableEventTestTuple_), static_cast<int>(AdvStatusTest::ADV_STOP_EVENT_TEST));
    HILOGI("Sle_AdvertiserImpl_Test002 end");
}

/**
 * @tc.name: Sle_AdvertiserImpl_Test003
 * @tc.desc: CheckAdvertiserPara分支测试
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvTest, Sle_AdvertiserImpl_Test003, TestSize.Level1)
{
    HILOGI("Sle_AdvertiserImpl_Test003 start");
    SleAdvertiserSettingsImpl advSettings;
    SleAdvertiserDataImpl advData;
    SleAdvertiserDataImpl advResponseData;
    std::string shortDataStr(0xEE, 'a');
    std::string longDataStr(0x100, 'a');

    advSettings.SetLegacyMode(true);
    advData.SetPayload(shortDataStr);
    advResponseData.SetPayload(longDataStr);

    int res = sleAdvertiserImp_->CheckAdvertiserPara(advSettings, advData, advResponseData);
    EXPECT_EQ(res, NLSTK_ERRCODE_PARAM_ERR);

    advData.SetPayload(longDataStr);
    res = sleAdvertiserImp_->CheckAdvertiserPara(advSettings, advData, advResponseData);
    EXPECT_EQ(res, NLSTK_ERRCODE_PARAM_ERR);

    advSettings.SetLegacyMode(false);
    advSettings.SetPrimaryPhy(static_cast<uint8_t>(SlePhyType::PHY_LE_CODED));
    res = sleAdvertiserImp_->CheckAdvertiserPara(advSettings, advData, advResponseData);
    EXPECT_EQ(res, NLSTK_ERRCODE_PARAM_ERR);

    advSettings.SetPrimaryPhy(static_cast<uint8_t>(SlePhyType::PHY_LE_1M));
    advSettings.SetSecondaryPhy(static_cast<uint8_t>(SlePhyType::PHY_LE_CODED));
    res = sleAdvertiserImp_->CheckAdvertiserPara(advSettings, advData, advResponseData);
    EXPECT_EQ(res, NLSTK_ERRCODE_PARAM_ERR);

    advSettings.SetPrimaryPhy(static_cast<uint8_t>(SlePhyType::PHY_LE_1M));
    advSettings.SetSecondaryPhy(static_cast<uint8_t>(SlePhyType::PHY_LE_1M));
    advData.SetPayload(shortDataStr);
    res = sleAdvertiserImp_->CheckAdvertiserPara(advSettings, advData, advResponseData);
    EXPECT_EQ(res, NLSTK_ERRCODE_PARAM_ERR);

    advResponseData.SetPayload(shortDataStr);
    res = sleAdvertiserImp_->CheckAdvertiserPara(advSettings, advData, advResponseData);
    EXPECT_EQ(res, NLSTK_ERRCODE_SUCCESS);
    HILOGI("Sle_AdvertiserImpl_Test003 end");
}

/**
 * @tc.name: Sle_AdvertiserImpl_Test004
 * @tc.desc: SetAdvertisingData StopAdvertising分支测试
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvTest, Sle_AdvertiserImpl_Test004, TestSize.Level1)
{
    HILOGI("Sle_AdvertiserImpl_Test004 start");
    // 确保handle = 0广播移除
    sleAdvertiserImp_->DdAdvRemoveCompleteEvt(0, NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    SleAdvertiserSettingsImpl advSettings;
    SleAdvertiserDataImpl advData;
    SleAdvertiserDataImpl advResponseData;
    sleAdvertiserImp_->StartAdvertising(advSettings, advData, advResponseData, 0);
    // 构造handle = 0广播状态STARTED
    sleAdvertiserImp_->DdAdvEnableCompleteEvt(0, NLSTK_ERRCODE_SUCCESS);
    // UpdateAdvDataToDd失败分支
    ResetAdvEventTestTuple();
    std::string str(0x01, ' ');
    advData.SetPayload(str);
    sleAdvertiserImp_->SetAdvertisingData(advData, advResponseData, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR));
    // StopAdvertising分支
    sleAdvertiserImp_->StopAdvertising(0);
    // 构造handle = 0广播移除
    sleAdvertiserImp_->DdAdvRemoveCompleteEvt(0, NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<0>(advEventTestTuple_), static_cast<int>(AdvStatusTest::ADV_STOP_EVENT_TEST));
    HILOGI("Sle_AdvertiserImpl_Test004 end");
}

/**
 * @tc.name: Sle_AdvertiserImpl_Test005
 * @tc.desc: EnableAdvertising DisableAdvertising SetAdvParam FillAdvParam FillScanRspData IsScanableAdv分支测试
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvTest, Sle_AdvertiserImpl_Test005, TestSize.Level1)
{
    HILOGI("Sle_AdvertiserImpl_Test005 start");
    ResetAdvEventTestTuple();
    SleAdvertiserSettingsImpl advSettings;
    SleAdvertiserDataImpl advData;
    SleAdvertiserDataImpl advResponseData;
    advSettings.SetConnectable(false);
    sleAdvertiserImp_->StartAdvertising(advSettings, advData, advResponseData, 0);
    // handle = 0广播状态DISABLING
    sleAdvertiserImp_->DisableAdvertising(0);
    sleAdvertiserImp_->EnableAdvertising(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_ALREADY_STARTED));
    // handle = 0广播状态DISABLED
    sleAdvertiserImp_->DdAdvDisableCompleteEvt(0, NLSTK_ERRCODE_SUCCESS);
    sleAdvertiserImp_->DisableAdvertising(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR));
    // handle = 0广播状态ENABLING
    sleAdvertiserImp_->EnableAdvertising(0);
    // 构造handle = 0广播移除
    sleAdvertiserImp_->DdAdvRemoveCompleteEvt(0, NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<0>(advEventTestTuple_), static_cast<int>(AdvStatusTest::ADV_STOP_EVENT_TEST));
    // SetAdvParam分支测试
    NLSTK_ERRCODE res = sleAdvertiserImp_->SetAdvParam(0, advSettings, true);
    EXPECT_EQ(res, NLSTK_ERRCODE_PARAM_ERR);
    // FillAdvParam分支测试
    NLSTK_DevdSetAdvParams_S advParams = {};
    res = sleAdvertiserImp_->FillAdvParam(0, advParams, advSettings);
    EXPECT_EQ(res, NLSTK_ERRCODE_PARAM_ERR);
    // FillScanRspData分支测试
    NLSTK_DevdAdvData_S data = {};
    std::string dataStr(0x01, 'a');
    advResponseData.SetPayload(dataStr);
    res = sleAdvertiserImp_->FillScanRspData(data, advResponseData);
    EXPECT_EQ(res, NLSTK_ERRCODE_SUCCESS);
    // IsScanableAdv分支测试
    bool result = sleAdvertiserImp_->IsScanableAdv(0);
    EXPECT_EQ(result, false);
    HILOGI("Sle_AdvertiserImpl_Test005 end");
}

/**
 * @tc.name: Sle_AdvertiserImpl_Test006
 * @tc.desc: SetFilter SetLinkRole SetPrimaryFrameType GetMaxAdvertisingDataLength分支测试
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvTest, Sle_AdvertiserImpl_Test006, TestSize.Level1)
{
    HILOGI("Sle_AdvertiserImpl_Test006 start");
    SleAdvertiserSettingsImpl settings;
    SleAdvertiserDataImpl advData;
    SleAdvertiserDataImpl scanResponse;
    struct SleAdvertiserImplData data(settings, advData, scanResponse, static_cast<int>(ADVERTISE_STATUS::NOT_STARTED));
    
    sleAdvertiserImp_->SetFilter(data, true, false);
    EXPECT_EQ(data.advParams.advFilterPolicy, ADV_FILTER_ALLOW_SCAN_WLST_CON_ANY);
    sleAdvertiserImp_->SetFilter(data, false, true);
    EXPECT_EQ(data.advParams.advFilterPolicy, ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST);
    sleAdvertiserImp_->SetFilter(data, true, true);
    EXPECT_EQ(data.advParams.advFilterPolicy, ADV_FILTER_ALLOW_SCAN_WLST_CON_WLST);

    sleAdvertiserImp_->SetLinkRole(data, 4);
    EXPECT_EQ(data.advParams.linkRole, static_cast<uint8_t>(SleLinkRole::T_CAN_NEGO));

    sleAdvertiserImp_->SetPrimaryFrameType(data, 2);
    EXPECT_EQ(data.advParams.primaryFrameType,
        static_cast<uint8_t>(SleAdvertiserPrimaryFrameType::SLE_ADV_PRI_FRAME_TYPE_1));

    uint32_t res = sleAdvertiserImp_->GetMaxAdvertisingDataLength(settings);
    EXPECT_EQ(res, SLE_LEGACY_ADV_DATA_LEN_MAX);
    HILOGI("Sle_AdvertiserImpl_Test006 end");
}

/**
 * @tc.name: Sle_AdvertiserImpl_Test007
 * @tc.desc: Evt分支测试
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkAdvTest, Sle_AdvertiserImpl_Test007, TestSize.Level1)
{
    HILOGI("Sle_AdvertiserImpl_Test007 start");
    ResetAdvEventTestTuple();
    int advStatus = static_cast<int>(ADVERTISE_STATUS::STARTED);
    sleAdvertiserImp_->DdAdvDataUpdateComplete(0, NLSTK_ERRCODE_FAIL, advStatus);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR));
    sleAdvertiserImp_->DdAdvScanRspDataUpdateComplete(0, NLSTK_ERRCODE_FAIL, advStatus);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), static_cast<int>(ADV_RESULT_FAILED_INTERNAL_ERROR));
    sleAdvertiserImp_->DdAdvScanRspDataUpdateComplete(0, NLSTK_ERRCODE_SUCCESS, advStatus);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), NLSTK_ERRCODE_SUCCESS);
    ResetAdvEventTestTuple();
    sleAdvertiserImp_->DdAdvEnableCompleteEvt(0, NLSTK_ERRCODE_SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 涉及切线程异步操作与回调，延迟1s
    EXPECT_EQ(std::get<1>(advEventTestTuple_), -1);
    HILOGI("Sle_AdvertiserImpl_Test007 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS