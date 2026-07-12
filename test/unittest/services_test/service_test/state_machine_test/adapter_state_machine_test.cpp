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
#include "gmock/gmock.h"

#include "nearlink_errorcode.h"
#include "AdapterStateMachine.h"
#include "mock_profile_service_manager.cpp"
#include "mock_sle_adapter.cpp"
#include "SleServiceManager.h"
#include "ClassCreator.h"
#include "log.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Nearlink;

namespace OHOS {
namespace Nearlink {
namespace {
    std::shared_ptr<AdapterStateMachine> stateMachine = std::make_shared<AdapterStateMachine>();
    std::shared_ptr<SleAdapter> adapter;
}

class AdapterStateMachineTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HILOGI("nearlink AdapterStateMachineTest.");
    }
    static void TearDownTestCase()
    {
        stateMachine = nullptr;
        adapter = nullptr;
    }

    void SetUp()
    {
        adapter = std::make_shared<SleAdapter>();
        ProfileServiceManager::GetInstance().Initialize();
        pAdapterTurningOnState_ = std::make_unique<AdapterTurningOnState>(stateMachine, adapter);
        pAdapterTurnOnState_ = std::make_unique<AdapterTurnOnState>(stateMachine, adapter);
        pAdapterTurningOffState_ = std::make_unique<AdapterTurningOffState>(stateMachine, adapter);
        pAdapterTurnOffState_ = std::make_unique<AdapterTurnOffState>(stateMachine, adapter);
        pAdapterTurningHalfToOffState_ = std::make_unique<AdapterTurningHalfToOffState>(stateMachine, adapter);
        pAdapterTurningOffToHalfState_ = std::make_unique<AdapterTurningOffToHalfState>(stateMachine, adapter);
        pAdapterTurningHalfToOnState_ = std::make_unique<AdapterTurningHalfToOnState>(stateMachine, adapter);
        pAdapterTurnHalfState_ = std::make_unique<AdapterTurnHalfState>(stateMachine, adapter);
    }
    void TearDown()
    {
        pAdapterTurnOffState_.reset();
        pAdapterTurningOnState_.reset();
        pAdapterTurnOnState_.reset();
        pAdapterTurningOffState_.reset();
        adapter = nullptr;
    }

public:
    std::unique_ptr<AdapterTurningOnState> pAdapterTurningOnState_;
    std::unique_ptr<AdapterTurnOnState> pAdapterTurnOnState_;
    std::unique_ptr<AdapterTurningOffState> pAdapterTurningOffState_;
    std::unique_ptr<AdapterTurnOffState> pAdapterTurnOffState_;
    std::unique_ptr<AdapterTurningHalfToOffState> pAdapterTurningHalfToOffState_;
    std::unique_ptr<AdapterTurningOffToHalfState> pAdapterTurningOffToHalfState_;
    std::unique_ptr<AdapterTurningHalfToOnState> pAdapterTurningHalfToOnState_;
    std::unique_ptr<AdapterTurnHalfState> pAdapterTurnHalfState_;
};

/*
 * @tc.number: StateMachineTest_TurnOffTest_001
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_TurnOffTest_001, TestSize.Level1)
{
    HILOGI("StateMachineTest_TurnOffTest_001 start");

    pAdapterTurnOffState_->Entry();
    EXPECT_EQ(SleServiceManager::GetInstance()->GetState(ADAPTER_SLE), SleStateID::STATE_TURN_OFF);

    utility::Message msgEnableReq(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ENABLE_REQ));
    utility::Message msgDisableReq(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_DISABLE_REQ));
    utility::Message msgHalfDisableReq(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_HALF_DISABLE_REQ));

    EXPECT_TRUE(pAdapterTurnOffState_->Dispatch(msgEnableReq));
    EXPECT_FALSE(pAdapterTurnOffState_->Dispatch(msgDisableReq));
    EXPECT_TRUE(pAdapterTurnOffState_->Dispatch(msgHalfDisableReq));

    HILOGI("StateMachineTest_TurnOffTest_001 end");
}

/*
 * @tc.number: StateMachineTest_TurnOnTest_001
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_TurnOnTest_001, TestSize.Level1)
{
    HILOGI("StateMachineTest_TurnOnTest_001 start");

    pAdapterTurnOnState_->Entry();
    EXPECT_EQ(SleServiceManager::GetInstance()->GetState(ADAPTER_SLE), SleStateID::STATE_TURN_ON);

    utility::Message msgEnableReq(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ENABLE_REQ));
    utility::Message msgDisableReq(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_DISABLE_REQ));
    utility::Message msgHalfDisableReq(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_HALF_DISABLE_REQ));

    EXPECT_FALSE(pAdapterTurnOnState_->Dispatch(msgEnableReq));
    EXPECT_TRUE(pAdapterTurnOnState_->Dispatch(msgDisableReq));
    EXPECT_TRUE(pAdapterTurnOnState_->Dispatch(msgHalfDisableReq));

    HILOGI("StateMachineTest_TurnOnTest_001 end");
}

/*
 * @tc.number: StateMachineTest_TurnHalfTest_001
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_TurnHalfTest_001, TestSize.Level1)
{
    HILOGI("StateMachineTest_TurnHalfTest_001 start");

    pAdapterTurnHalfState_->Entry();
    EXPECT_EQ(SleServiceManager::GetInstance()->GetState(ADAPTER_SLE), SleStateID::STATE_TURN_HALF);

    utility::Message msgEnableReq(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ENABLE_REQ));
    utility::Message msgDisableReq(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_DISABLE_REQ));
    utility::Message msgHalfDisableReq(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_HALF_DISABLE_REQ));

    EXPECT_TRUE(pAdapterTurnHalfState_->Dispatch(msgEnableReq));
    EXPECT_TRUE(pAdapterTurnHalfState_->Dispatch(msgDisableReq));
    EXPECT_FALSE(pAdapterTurnHalfState_->Dispatch(msgHalfDisableReq));

    HILOGI("StateMachineTest_TurnHalfTest_001 end");
}

/*
 * @tc.number: StateMachineTest_TurningOnTest_001
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_TurningOnTest_001, TestSize.Level1)
{
    HILOGI("StateMachineTest_TurningOnTest_001 start");

    pAdapterTurningOnState_->Entry();
    EXPECT_EQ(SleServiceManager::GetInstance()->GetState(ADAPTER_SLE), SleStateID::STATE_TURNING_ON);
    EXPECT_EQ(adapter->pimpl->calledCountEnable_, 1);

    HILOGI("StateMachineTest_TurningOnTest_001 end");
}

/*
 * @tc.number: StateMachineTest_TurningOnTest_002
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_TurningOnTest_002, TestSize.Level1)
{
    HILOGI("StateMachineTest_TurningOnTest_002 start");

    utility::Message msgAdapterEnableCmp(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_ENABLE_CMP), true);
    utility::Message msgProfileEnableCmp(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_ENABLE_CMP), true);
    utility::Message msgAdapterEnableTimeOut(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_ENABLE_TIME_OUT), true);
    utility::Message msgProfileEnableTimeOut(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_ENABLE_TIME_OUT), true);
    
    EXPECT_TRUE(pAdapterTurningOnState_->Dispatch(msgAdapterEnableCmp));
    EXPECT_EQ(ProfileServiceManager::GetInstance().pimpl->calledCountEnable_, 1);

    EXPECT_TRUE(pAdapterTurningOnState_->Dispatch(msgProfileEnableCmp));


    stateMachine->nextTargetState_ = SleStateID::STATE_TURN_ON;
    EXPECT_TRUE(pAdapterTurningOnState_->Dispatch(msgAdapterEnableTimeOut));
    EXPECT_EQ(stateMachine->nextTargetState_, SleStateID::STATE_TURN_OFF);

    stateMachine->nextTargetState_ = SleStateID::STATE_TURN_ON;
    EXPECT_TRUE(pAdapterTurningOnState_->Dispatch(msgProfileEnableTimeOut));
    EXPECT_EQ(stateMachine->nextTargetState_, SleStateID::STATE_TURN_OFF);

    HILOGI("StateMachineTest_TurningOnTest_002 end");
}

/*
 * @tc.number: StateMachineTest_TurningOffTest_001
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_TurningOffTest_001, TestSize.Level1)
{
    HILOGI("StateMachineTest_TurningOffTest_001 start");

    pAdapterTurningOffState_->Entry();
    EXPECT_EQ(SleServiceManager::GetInstance()->GetState(ADAPTER_SLE), SleStateID::STATE_TURNING_OFF);
    EXPECT_EQ(ProfileServiceManager::GetInstance().pimpl->calledCountDisable_, 1);

    HILOGI("StateMachineTest_TurningOffTest_001 end");
}

/*
 * @tc.number: StateMachineTest_TurningOffTest_002
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_TurningOffTest_002, TestSize.Level1)
{
    HILOGI("StateMachineTest_TurningOffTest_002 start");

    utility::Message msgAdapterDisableCmp(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_DISABLE_CMP), true);
    utility::Message msgProfileDisableCmp(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_DISABLE_CMP), true);
    utility::Message msgAdapterDisableTimeOut(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_DISABLE_TIME_OUT), true);
    utility::Message msgProfileDisableTimeOut(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_DISABLE_TIME_OUT), true);

    EXPECT_TRUE(pAdapterTurningOffState_->Dispatch(msgProfileDisableCmp));
    EXPECT_EQ(adapter->pimpl->calledCountDisable_, 1);

    EXPECT_TRUE(pAdapterTurningOffState_->Dispatch(msgAdapterDisableCmp));
    EXPECT_TRUE(pAdapterTurningOffState_->Dispatch(msgProfileDisableTimeOut));
    EXPECT_TRUE(pAdapterTurningOffState_->Dispatch(msgAdapterDisableTimeOut));

    HILOGI("StateMachineTest_TurningOffTest_002 end");
}

/*
 * @tc.number: StateMachineTest_TurningOffToHalfTest_001
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_TurningOffToHalfTest_001, TestSize.Level1)
{
    HILOGI("StateMachineTest_TurningOffToHalfTest_001 start");

    pAdapterTurningOffToHalfState_->Entry();
    EXPECT_EQ(SleServiceManager::GetInstance()->GetState(ADAPTER_SLE), SleStateID::STATE_TURNING_OFF_TO_HALF);
    EXPECT_EQ(adapter->pimpl->calledCountEnable_, 1);

    HILOGI("StateMachineTest_TurningOffToHalfTest_001 end");
}

/*
 * @tc.number: StateMachineTest_TurningOffToHalfTest_002
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_TurningOffToHalfTest_002, TestSize.Level1)
{
    HILOGI("StateMachineTest_TurningOffToHalfTest_002 start");

    utility::Message msgAdapterEnableCmp(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_ENABLE_CMP), true);
    utility::Message msgProfileEnableCmp(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_ENABLE_CMP), true);
    utility::Message msgAdapterEnableTimeOut(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_ENABLE_TIME_OUT), true);
    utility::Message msgProfileEnableTimeOut(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_ENABLE_TIME_OUT), true);

    EXPECT_TRUE(pAdapterTurningOffToHalfState_->Dispatch(msgAdapterEnableCmp));
    EXPECT_EQ(ProfileServiceManager::GetInstance().pimpl->calledCountEnable_, 1);

    EXPECT_TRUE(pAdapterTurningOffToHalfState_->Dispatch(msgProfileEnableCmp));

    stateMachine->nextTargetState_ = SleStateID::STATE_TURN_HALF;
    EXPECT_TRUE(pAdapterTurningOffToHalfState_->Dispatch(msgAdapterEnableTimeOut));
    EXPECT_EQ(stateMachine->nextTargetState_, SleStateID::STATE_TURN_OFF);

    stateMachine->nextTargetState_ = SleStateID::STATE_TURN_HALF;
    EXPECT_TRUE(pAdapterTurningOffToHalfState_->Dispatch(msgProfileEnableTimeOut));
    EXPECT_EQ(stateMachine->nextTargetState_, SleStateID::STATE_TURN_OFF);

    HILOGI("StateMachineTest_TurningOffToHalfTest_002 end");
}

/*
 * @tc.number: StateMachineTest_TurningHalfToOffTest_001
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_TurningHalfToOffTest_001, TestSize.Level1)
{
    HILOGI("StateMachineTest_TurningHalfToOffTest_001 start");

    pAdapterTurningHalfToOffState_->Entry();
    EXPECT_EQ(SleServiceManager::GetInstance()->GetState(ADAPTER_SLE), SleStateID::STATE_TURNING_HALF_TO_OFF);
    EXPECT_EQ(ProfileServiceManager::GetInstance().pimpl->calledCountDisable_, 1);

    HILOGI("StateMachineTest_TurningHalfToOffTest_001 end");
}

/*
 * @tc.number: StateMachineTest_TurningHalfToOffTest_002
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_TurningHalfToOffTest_002, TestSize.Level1)
{
    HILOGI("StateMachineTest_TurningHalfToOffTest_002 start");

    utility::Message msgAdapterDisableCmp(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_DISABLE_CMP), true);
    utility::Message msgProfileDisableCmp(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_DISABLE_CMP), true);
    utility::Message msgAdapterDisableTimeOut(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_ADAPTER_DISABLE_TIME_OUT), true);
    utility::Message msgProfileDisableTimeOut(
        static_cast<int>(AdapterStateMachine::AdapterStateMessage::MSG_PROFILE_DISABLE_TIME_OUT), true);

    EXPECT_TRUE(pAdapterTurningHalfToOffState_->Dispatch(msgProfileDisableCmp));
    EXPECT_EQ(adapter->pimpl->calledCountDisable_, 1);

    EXPECT_TRUE(pAdapterTurningHalfToOffState_->Dispatch(msgAdapterDisableCmp));
    EXPECT_TRUE(pAdapterTurningHalfToOffState_->Dispatch(msgProfileDisableTimeOut));
    EXPECT_TRUE(pAdapterTurningHalfToOffState_->Dispatch(msgAdapterDisableTimeOut));

    HILOGI("StateMachineTest_TurningHalfToOffTest_002 end");
}

/*
 * @tc.number: StateMachineTest_SetNextTargetStateTest_001
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_SetNextTargetStateTest_001, TestSize.Level1)
{
    HILOGI("StateMachineTest_SetNextTargetStateTest_001 start");

    stateMachine->SetNextTargetState(SleStateID::STATE_TURN_OFF);
    EXPECT_EQ(stateMachine->GetNextTargetState(), SleStateID::STATE_TURN_OFF);
    stateMachine->SetNextTargetState(SleStateID::STATE_TURN_HALF);
    EXPECT_EQ(stateMachine->GetNextTargetState(), SleStateID::STATE_TURN_HALF);
    stateMachine->SetNextTargetState(SleStateID::STATE_TURN_ON);
    EXPECT_EQ(stateMachine->GetNextTargetState(), SleStateID::STATE_TURN_ON);
    stateMachine->SetNextTargetState(SleStateID::STATE_TURNING_OFF);
    EXPECT_EQ(stateMachine->GetNextTargetState(), SleStateID::STATE_TURN_ON);

    HILOGI("StateMachineTest_SetNextTargetStateTest_001 end");
}

/*
 * @tc.number: StateMachineTest_PostTargetMessageTest_001
 * @tc.name: Dispatch
 * @tc.desc:
 */
HWTEST_F(AdapterStateMachineTest, StateMachineTest_PostTargetMessageTest_001, TestSize.Level1)
{
    HILOGI("StateMachineTest_PostTargetMessageTest_001 start");

    stateMachine->SetNextTargetState(SleStateID::STATE_TURN_HALF);
    pAdapterTurnOffState_->Entry();
    stateMachine->SetNextTargetState(SleStateID::STATE_TURN_ON);
    pAdapterTurnHalfState_->Entry();
    stateMachine->SetNextTargetState(SleStateID::STATE_TURN_OFF);
    pAdapterTurnOnState_->Entry();
    stateMachine->SetNextTargetState(SleStateID::STATE_TURNING_ON);
    EXPECT_EQ(stateMachine->GetNextTargetState(), SleStateID::STATE_TURN_OFF);

    HILOGI("StateMachineTest_PostTargetMessageTest_001 end");
}

}  // namespace Nearlink
}  // namespace OHOS