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

#include "CcpSystemInterface.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Telephony;

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
}

void CcpSystemInterfaceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase CcpSystemInterfaceTest");
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
    NlErrCode ret = CcpSystemInterface::GetInstance().HangUpCall();
    EXPECT_TRUE(ret == NL_NO_ERROR || ret == NL_ERR_INTERNAL_ERROR);
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
    NlErrCode ret = CcpSystemInterface::GetInstance().RejectCall();
    EXPECT_TRUE(ret == NL_NO_ERROR || ret == NL_ERR_INTERNAL_ERROR);
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
    EXPECT_NO_THROW(CcpSystemInterface::GetInstance().CheckNowHasDeviceConnected());
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
    EXPECT_TRUE(CcpSystemInterface::GetInstance().IsRinging(Telephony::TelCallState::CALL_STATUS_INCOMING));
    EXPECT_TRUE(CcpSystemInterface::GetInstance().IsRinging(Telephony::TelCallState::CALL_STATUS_WAITING));
    EXPECT_FALSE(CcpSystemInterface::GetInstance().IsRinging(Telephony::TelCallState::CALL_STATUS_ACTIVE));
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("IsRinging001 end");
}

/*
 * @tc.number: DeInitNearlinkCallClient001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(CcpSystemInterfaceTest, DeInitNearlinkCallClient001, TestSize.Level1)
{
    HILOGI("DeInitNearlinkCallClient001 start");
    EXPECT_NO_THROW(CcpSystemInterface::GetInstance().DeInitNearlinkCallClient());
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
    EXPECT_NO_THROW(CcpSystemInterface::GetInstance().Stop());
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::CCP_SERVICE_UT_DELAY_50_MS));
    HILOGI("Stop001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS