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

#include "nearlink_access_token_mock.h"
#include "nearlink_def.h"
#include "nearlink_host.h"
#include "nearlink_sle_advertiser.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class SleAdvertiserTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    void CreateAdvertiseSettings()
    {
        advertiserSettings_.SetConnectable(true);
        advertiserSettings_.SetLegacyMode(true);
        advertiserSettings_.SetInterval(static_cast<int>(AdvInterval::ADV_INTERVAL_DEFAULT));
        advertiserSettings_.SetTxPower(static_cast<uint8_t>(SleAdvertiserTxPowerLevel::SLE_ADV_TX_POWER_LOW));
        advertiserSettings_.SetPrimaryPhy(static_cast<uint8_t>(SleAdvPhy::SLE_ADVERTISEMENT_PHY_1M));
        advertiserSettings_.SetLinkRole(static_cast<uint8_t>(SleLinkRole::T_CAN_NEGO));
    }

    void CreateAdvdata()
    {
        Nearlink::UUID serviceDataUuid = Nearlink::UUID::FromString("00000000-0000-1000-8000-00805F9B34FB");
        advertiserData_.AddManufacturerData(0x1, "1234");
        advertiserData_.AddServiceData(serviceDataUuid, "5");
    }

    Nearlink::SleAdvertiserSettings &GetAdvertiserSettings()
    {
        return advertiserSettings_;
    }

    Nearlink::SleAdvertiserData &GetAdvertiserData()
    {
        return advertiserData_;
    }

    int GetAdvHandle()
    {
        return advHandle_;
    }

    void SetAdvHandle(int advHandle)
    {
        advHandle_ = advHandle;
    }

private:
    Nearlink::SleAdvertiserSettings advertiserSettings_;
    Nearlink::SleAdvertiserData advertiserData_;
    int advHandle_ = -1;
};

void SleAdvertiserTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    if (!NearlinkHost::GetInstance().IsSleEnabled()) {
        HILOGI("start enable nearlink");
        NearlinkHost::GetInstance().EnableNl();
        sleep(2); // sleep 2s
    }
    HILOGI("SetUpTestCase end");
}

void SleAdvertiserTest::TearDownTestCase()
{}

void SleAdvertiserTest::SetUp()
{
    CreateAdvertiseSettings();
    CreateAdvdata();
}

void SleAdvertiserTest::TearDown()
{}

class SleAdvertiseCallbackTest final: public Nearlink::SleAdvertiseCallback {
public:
    explicit SleAdvertiseCallbackTest(SleAdvertiserTest &test) : test_(test) {};
    ~SleAdvertiseCallbackTest() {};

private:
    void OnStartResultEvent(int result, int advHandle)
    {
        HILOGI("result(%{public}d), advHandle(%{public}d)", result, advHandle);
        test_.SetAdvHandle(advHandle);
    }
    void OnEnableResultEvent(int result, int advHandle) {}
    void OnDisableResultEvent(int result, int advHandle) {}
    void OnStopResultEvent(int result, int advHandle)
    {
        HILOGI("result(%{public}d), advHandle(%{public}d)", result, advHandle);
    }
    void OnSetAdvDataEvent(int result) {}
    void OnGetAdvHandleEvent(int result, int advHandle) {}

private:
    SleAdvertiserTest &test_;
};

/**
 * @tc.number: CreateSleAdvertiser001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAdvertiserTest, CreateSleAdvertiser001, TestSize.Level1)
{
    HILOGI("CreateSleAdvertiser001 start");
    std::shared_ptr<SleAdvertiser> advertiserInst = SleAdvertiser::CreateSleAdvertiser();
    EXPECT_NE(nullptr, advertiserInst);
    HILOGI("CreateSleAdvertiser001 end");
}

/**
 * @tc.number: StartAndStopAdvertising001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAdvertiserTest, StartAndStopAdvertising001, TestSize.Level1)
{
    HILOGI("StartAndStopAdvertising001 start");
    std::shared_ptr<SleAdvertiser> advertiserInst = SleAdvertiser::CreateSleAdvertiser();
    std::shared_ptr<SleAdvertiseCallbackTest> callback = std::make_shared<SleAdvertiseCallbackTest>(*this);
    NlErrCode ret = advertiserInst->StartAdvertising(
        GetAdvertiserSettings(), GetAdvertiserData(), GetAdvertiserData(), 0, callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely started.
    ret = advertiserInst->StopAdvertising(callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely stopped.
    HILOGI("StartAndStopAdvertising001 end");
}

/**
 * @tc.number: EnableAndDisableAdvertising001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAdvertiserTest, EnableAndDisableAdvertising001, TestSize.Level1)
{
    HILOGI("EnableAndDisableAdvertising001 start");
    std::shared_ptr<SleAdvertiser> advertiserInst = SleAdvertiser::CreateSleAdvertiser();
    std::shared_ptr<SleAdvertiseCallbackTest> callback = std::make_shared<SleAdvertiseCallbackTest>(*this);
    NlErrCode ret = advertiserInst->StartAdvertising(
        GetAdvertiserSettings(), GetAdvertiserData(), GetAdvertiserData(), 0, callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely started.
    ret = advertiserInst->DisableAdvertising(GetAdvHandle());
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely Disable.
    ret = advertiserInst->EnableAdvertising(GetAdvHandle());
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely enable.
    ret = advertiserInst->StopAdvertising(callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely stopped.
    HILOGI("EnableAndDisableAdvertising001 end");
}

/**
 * @tc.number: EnableAndDisableAdvertising002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAdvertiserTest, EnableAndDisableAdvertising002, TestSize.Level1)
{
    HILOGI("EnableAndDisableAdvertising002 start");
    std::shared_ptr<SleAdvertiser> advertiserInst = SleAdvertiser::CreateSleAdvertiser();
    std::shared_ptr<SleAdvertiseCallbackTest> callback = std::make_shared<SleAdvertiseCallbackTest>(*this);
    NlErrCode ret = advertiserInst->StartAdvertising(
        GetAdvertiserSettings(), GetAdvertiserData(), GetAdvertiserData(), 0, callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely started.
    ret = advertiserInst->DisableAdvertising(GetAdvHandle());
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely Disable.
    ret = advertiserInst->EnableAdvertising(GetAdvHandle());
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely enable.
    ret = advertiserInst->DisableAdvertising(GetAdvHandle());
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely Disable.
    ret = advertiserInst->StopAdvertising(callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely stopped.
    HILOGI("EnableAndDisableAdvertising002 end");
}

/**
 * @tc.number: EnableAndDisableAdvertising003
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAdvertiserTest, EnableAndDisableAdvertising003, TestSize.Level1)
{
    HILOGI("EnableAndDisableAdvertising003 start");
    std::shared_ptr<SleAdvertiser> advertiserInst = SleAdvertiser::CreateSleAdvertiser();
    std::shared_ptr<SleAdvertiseCallbackTest> callback = std::make_shared<SleAdvertiseCallbackTest>(*this);
    NlErrCode ret = advertiserInst->StartAdvertising(
        GetAdvertiserSettings(), GetAdvertiserData(), GetAdvertiserData(), 0, callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely started.
    ret = advertiserInst->DisableAdvertising(GetAdvHandle());
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely Disable.
    ret = advertiserInst->StopAdvertising(callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely stopped.
    HILOGI("EnableAndDisableAdvertising003 end");
}

/**
 * @tc.number: EnableAndDisableAdvertising004
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(SleAdvertiserTest, EnableAndDisableAdvertising004, TestSize.Level1)
{
    HILOGI("EnableAndDisableAdvertising004 start");
    std::shared_ptr<SleAdvertiser> advertiserInst = SleAdvertiser::CreateSleAdvertiser();
    std::shared_ptr<SleAdvertiseCallbackTest> callback = std::make_shared<SleAdvertiseCallbackTest>(*this);
    NlErrCode ret = advertiserInst->StartAdvertising(
        GetAdvertiserSettings(), GetAdvertiserData(), GetAdvertiserData(), 0, callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely started.
    ret = advertiserInst->EnableAdvertising(GetAdvHandle());
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely Enable.
    ret = advertiserInst->StopAdvertising(callback);
    EXPECT_EQ(NL_NO_ERROR, ret);
    sleep(2); // Wait for 2s until the adv is completely stopped.
    HILOGI("EnableAndDisableAdvertising004 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
