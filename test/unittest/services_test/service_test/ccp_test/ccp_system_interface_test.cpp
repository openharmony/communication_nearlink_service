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
#include "ThreadUtil.h"
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
    // 单测不启动ccp工作线程：置为同步执行，回调内动作即时生效
    ThreadUtil::GetInstance().InitThreadStateMap();
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_CCP, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
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

/**
 * @tc.number: SystemAbilityChangeCallback001
 * @tc.name: Test SA status callback splits by ability id.
 * @tc.desc: FUNC
 */
HWTEST_F(CcpSystemInterfaceTest, SystemAbilityChangeCallback001, TestSize.Level1)
{
    // 电话服务上下线回调按能力号分流：匹配走客户端初始化，不匹配只告警；初始化后查询通话列表为空
    CcpSystemInterface::SystemAbilityStatusChange listener;
    listener.OnAddSystemAbility(TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID, "");
    EXPECT_TRUE(CcpSystemInterface::GetInstance().GetCurrentCallList().empty());

    listener.OnAddSystemAbility(TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID + 1, "");
    listener.OnRemoveSystemAbility(TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID, "");
    listener.OnRemoveSystemAbility(TELEPHONY_CALL_MANAGER_SYS_ABILITY_ID + 1, "");
    EXPECT_TRUE(CcpSystemInterface::GetInstance().GetCurrentCallList().empty());
}

/**
 * @tc.number: CallOperationNoServiceEnv001
 * @tc.name: Test call operations return error without call manager.
 * @tc.desc: FUNC
 */
HWTEST_F(CcpSystemInterfaceTest, CallOperationNoServiceEnv001, TestSize.Level1)
{
    // 无电话服务环境下接听、挂断、拒接都返回内部错误
    EXPECT_EQ(CcpSystemInterface::GetInstance().AnswerCall(), NL_ERR_INTERNAL_ERROR);
    EXPECT_EQ(CcpSystemInterface::GetInstance().HangUpCall(), NL_ERR_INTERNAL_ERROR);
    EXPECT_EQ(CcpSystemInterface::GetInstance().RejectCall(), NL_ERR_INTERNAL_ERROR);
}

/**
 * @tc.number: IsRingingStates001
 * @tc.name: Test ringing judgement covers incoming and waiting.
 * @tc.desc: FUNC
 */
HWTEST_F(CcpSystemInterfaceTest, IsRingingStates001, TestSize.Level1)
{
    // 来电与等待都算振铃，其余状态不算
    EXPECT_TRUE(CcpSystemInterface::GetInstance().IsRinging(Telephony::TelCallState::CALL_STATUS_INCOMING));
    EXPECT_TRUE(CcpSystemInterface::GetInstance().IsRinging(Telephony::TelCallState::CALL_STATUS_WAITING));
    EXPECT_FALSE(CcpSystemInterface::GetInstance().IsRinging(Telephony::TelCallState::CALL_STATUS_ACTIVE));
    EXPECT_FALSE(CcpSystemInterface::GetInstance().IsRinging(Telephony::TelCallState::CALL_STATUS_IDLE));
}

/**
 * @tc.number: CheckNowHasDeviceConnectedNoDevice001
 * @tc.name: Test connected check with no audio device.
 * @tc.desc: FUNC
 */
HWTEST_F(CcpSystemInterfaceTest, CheckNowHasDeviceConnectedNoDevice001, TestSize.Level1)
{
    // 无音频设备在连时判断为假
    EXPECT_FALSE(CcpSystemInterface::GetInstance().CheckNowHasDeviceConnected());
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS