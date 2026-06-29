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
#include <thread>

#include "CcpSystemInterface.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Telephony;

static std::unique_ptr<CcpSystemInterface::CallManagerCallbackImpl> callManagerCallbackImpl_ {nullptr};
constexpr int CCP_SERVICE_UT_DELAY_50_MS = 50;
constexpr int32_t TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID = 4005;

class CcpSystemInterfaceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void CcpSystemInterfaceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase CcpSystemInterfaceTest");
    callManagerCallbackImpl_ = std::make_unique<CcpSystemInterface::CallManagerCallbackImpl>();
}

void CcpSystemInterfaceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase CcpSystemInterfaceTest");
    callManagerCallbackImpl_ = nullptr;
}

void CcpSystemInterfaceTest::SetUp()
{
    HILOGI("SetUp CcpSystemInterfaceTest.");
}

void CcpSystemInterfaceTest::TearDown()
{
    HILOGI("TearDown CcpSystemInterfaceTest.");
}

/*
 * @tc.number: HangUpCall001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(CcpSystemInterfaceTest, HangUpCall001, TestSize.Level1)
{
    HILOGI("HangUpCall001 start");
    CcpSystemInterface::GetInstance().HangUpCall();
    EXPECT_NE(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("HangUpCall001 end");
}

/*
 * @tc.number: RejectCall001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(CcpSystemInterfaceTest, RejectCall001, TestSize.Level1)
{
    HILOGI("RejectCall001 start");
    CcpSystemInterface::GetInstance().RejectCall();
    EXPECT_NE(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("RejectCall001 end");
}

/*
 * @tc.number: CheckNowHasDeviceConnected001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(CcpSystemInterfaceTest, CheckNowHasDeviceConnected001, TestSize.Level1)
{
    HILOGI("CheckNowHasDeviceConnected001 start");
    CcpSystemInterface::GetInstance().CheckNowHasDeviceConnected();
    EXPECT_NE(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("CheckNowHasDeviceConnected001 end");
}

/*
 * @tc.number: IsRinging001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(CcpSystemInterfaceTest, IsRinging001, TestSize.Level1)
{
    HILOGI("IsRinging001 start");
    CcpSystemInterface::GetInstance().IsRinging(Telephony::TelCallState::CALL_STATUS_INCOMING);
    EXPECT_NE(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("IsRinging001 end");
}

/*
 * @tc.number: OnCallDetailsChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(CcpSystemInterfaceTest, OnCallDetailsChange001, TestSize.Level1)
{
    HILOGI("OnCallDetailsChange001 start");
    CallAttributeInfo callAttributeInfo;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    callManagerCallbackImpl_->OnCallDetailsChange(callAttributeInfo);
    EXPECT_NE(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnCallDetailsChange001 end");
}

/*
 * @tc.number: OnMeeTimeDetailsChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(CcpSystemInterfaceTest, OnMeeTimeDetailsChange001, TestSize.Level1)
{
    HILOGI("OnMeeTimeDetailsChange001 start");
    CallAttributeInfo callAttributeInfo;
    callAttributeInfo.callState = TelCallState::CALL_STATUS_INCOMING;
    callManagerCallbackImpl_->OnMeeTimeDetailsChange(callAttributeInfo);
    EXPECT_NE(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnMeeTimeDetailsChange001 end");
}

/*
 * @tc.number: OnPhoneStateChange001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(CcpSystemInterfaceTest, OnPhoneStateChange001, TestSize.Level1)
{
    HILOGI("OnPhoneStateChange001 start");
    int32_t numActive = 1;
    int32_t numHeld = 0;
    int32_t callState = static_cast<int32_t>(TelCallState::CALL_STATUS_ACTIVE);
    std::string number = "11111111111";
    callManagerCallbackImpl_->OnPhoneStateChange(numActive, numHeld, callState, number);
    EXPECT_NE(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("OnPhoneStateChange001 end");
}

/*
 * @tc.number: DeInitNearlinkCallClient001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(CcpSystemInterfaceTest, DeInitNearlinkCallClient001, TestSize.Level1)
{
    HILOGI("DeInitNearlinkCallClient001 start");
    CcpSystemInterface::GetInstance().DeInitNearlinkCallClient();
    EXPECT_NE(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("DeInitNearlinkCallClient001 end");
}

/*
 * @tc.number: Stop001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(CcpSystemInterfaceTest, Stop001, TestSize.Level1)
{
    HILOGI("Stop001 start");
    CcpSystemInterface::GetInstance().Stop();
    EXPECT_NE(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("Stop001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS