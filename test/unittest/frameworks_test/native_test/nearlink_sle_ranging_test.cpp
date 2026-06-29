/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "nearlink_sle_ranging.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Nearlink;
const uint8_t DEFAULT_CAPABILITY = 1;
const uint8_t DEFAULT_REFRESH_RATE = 4;
class NearlinkSleRangingTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkSleRangingTest::SetUpTestCase()
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

void NearlinkSleRangingTest::TearDownTestCase()
{}

void NearlinkSleRangingTest::SetUp()
{
}

void NearlinkSleRangingTest::TearDown()
{}

class SleRangingCallbackTest final: public Nearlink::SleRangingCallback {
public:
    SleRangingCallbackTest() {};
    ~SleRangingCallbackTest() {};

    void OnSleRangingResult(const RangingResult &result) override {}
    void OnSleRangingStateChange(const RangingState &state) override {}
};

/**
 * @tc.number: CreateNearlinkSleRanging001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkSleRangingTest, CreateNearlinkSleRanging001, TestSize.Level1)
{
    HILOGI("CreateNearlinkSleRanging001 start");
    std::shared_ptr<SleRangingCallbackTest> callback = std::make_shared<SleRangingCallbackTest>();
    std::shared_ptr<NearlinkSleRanging> sleRanging = NearlinkSleRanging::CreateNearlinkSleRanging(callback);
    EXPECT_NE(nullptr, sleRanging);
    sleRanging = nullptr;
    HILOGI("CreateNearlinkSleRanging001 end");
}

/**
 * @tc.number: StartAndStopSounding001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkSleRangingTest, StartAndStopSounding001, TestSize.Level1)
{
    HILOGI("StartAndStopSounding001 start");
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<NearlinkRemoteDevice> device = std::make_shared<NearlinkRemoteDevice>(
        addr, static_cast<int>(NlTransportType::NL_TRANSPORT_SLE));
    std::shared_ptr<SleRangingCallbackTest> callback = std::make_shared<SleRangingCallbackTest>();
    std::shared_ptr<NearlinkSleRanging> sleRanging = NearlinkSleRanging::CreateNearlinkSleRanging(callback);
    RangingConfig config = RangingConfig(DEFAULT_REFRESH_RATE);
    NlErrCode ret = sleRanging->StartSleRanging(*device, config);
    EXPECT_EQ(NL_ERR_DEVICE_DISCONNECTED, ret);
    sleep(2); // Wait for 2s until the scan is completely started.
    ret = sleRanging->StopSleRanging(*device);
    EXPECT_EQ(NL_ERR_DEVICE_DISCONNECTED, ret);
    sleep(2); // Wait for 2s until the scan is completely stopped.
    HILOGI("StartAndStopSounding001 end");
}

/**
 * @tc.number: GetRangingSupportedCapability001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkSleRangingTest, GetRangingSupportedCapability001, TestSize.Level1)
{
    HILOGI("GetRangingSupportedCapability001 start");
    uint8_t capability =0;
    NlErrCode ret = NearlinkSleRanging::GetRangingSupportedCapability(capability);
    EXPECT_EQ(NL_NO_ERROR, ret);
    EXPECT_EQ(capability, DEFAULT_CAPABILITY);
    HILOGI("GetRangingSupportedCapability001 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS