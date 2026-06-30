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
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <chrono>
#include <thread>
#include "nearlink_switch_module.h"
#include "log.h"
#include "nearlink_errorcode.h"

using namespace testing;
using namespace testing::ext;
using namespace std::chrono_literals;

namespace OHOS {
namespace Nearlink {
#define WAIT_CACHED_EVENT_COMPLETE std::this_thread::sleep_for(std::chrono::milliseconds(10))

class MockNearlinkSwitchAction : public INearlinkSwitchAction {
public:
    MOCK_METHOD(NlErrCode, EnableNearlink, (SleAutoConnectPolicy), (override));
    MOCK_METHOD(NlErrCode, DisableNearlink, (), (override));
    MOCK_METHOD(NlErrCode, DisableNearlinkToOff, (), (override));
    MOCK_METHOD(NlErrCode, EnableNearlinkToHalf, (), (override));
};

class NearlinkSwitchModuleTest : public testing::Test {
public:
    NearlinkSwitchModuleTest() = default;
    ~NearlinkSwitchModuleTest() = default;

    void SetUp() override
    {
        auto switchAction = std::make_unique<MockNearlinkSwitchAction>();
        switchAction_ = switchAction.get();
        switchModule_ = std::make_shared<NearlinkSwitchModule>(std::move(switchAction));
        switchModule_->taskTimeout_ = 20000;  // 20000 is 20ms
    }

    void TearDown() override
    {
        switchModule_= nullptr;
        switchAction_ = nullptr;
    }

    MockNearlinkSwitchAction *switchAction_;
    std::shared_ptr<NearlinkSwitchModule> switchModule_;
};

/**
 * @tc.name: NearlinkSwitchModuleTest_001
 * @tc.desc: 开星闪
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_001, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_001 start");
    EXPECT_CALL(*switchAction_, EnableNearlink(_)).WillOnce(Return(NL_NO_ERROR));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_ON);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_001 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_002
 * @tc.desc: 开星闪失败
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_002, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_002 start");
    EXPECT_CALL(*switchAction_, EnableNearlink(_)).WillOnce(Return(NL_ERR_INTERNAL_ERROR));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK),
        NL_ERR_INTERNAL_ERROR);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_002 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_003
 * @tc.desc: 开星闪操作无效
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_003, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_003 start");
    EXPECT_CALL(*switchAction_, EnableNearlink(_)).WillOnce(Return(NL_ERR_INVALID_SWITCH_OPERATION));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK),
        NL_NO_ERROR);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_003 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_004
 * @tc.desc: 关星闪，目标状态为OFF
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_004, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_004 start");
    EXPECT_CALL(*switchAction_, DisableNearlink()).WillOnce(Return(NL_NO_ERROR));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);

    switchModule_->disableStatus_ = NearlinkDisableStatus::DISABLING_TO_OFF;
    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_OFF);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->disableStatus_, NearlinkDisableStatus::STANDING_BY);
    HILOGI("NearlinkSwitchModuleTest_004 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_005
 * @tc.desc: 关星闪，目标状态为HALF
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_005, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_005 start");
    EXPECT_CALL(*switchAction_, DisableNearlink()).WillOnce(Return(NL_NO_ERROR));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);

    switchModule_->disableStatus_ = NearlinkDisableStatus::DISABLING_TO_HALF;
    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_OFF);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_HALF);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->disableStatus_, NearlinkDisableStatus::STANDING_BY);
    HILOGI("NearlinkSwitchModuleTest_005 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_006
 * @tc.desc: 关星闪失败
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_006, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_006 start");
    EXPECT_CALL(*switchAction_, DisableNearlink()).WillOnce(Return(NL_ERR_INTERNAL_ERROR));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK),
        NL_ERR_INTERNAL_ERROR);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_006 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_007
 * @tc.desc: 关星闪操作无效
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_007, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_007 start");
    EXPECT_CALL(*switchAction_, DisableNearlink()).WillOnce(Return(NL_ERR_INVALID_SWITCH_OPERATION));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK),
        NL_NO_ERROR);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_007 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_008
 * @tc.desc: 全关星闪
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_008, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_008 start");
    EXPECT_CALL(*switchAction_, DisableNearlinkToOff()).WillOnce(Return(NL_NO_ERROR));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_OFF);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_008 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_009
 * @tc.desc: 全关星闪失败
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_009, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_009 start");
    EXPECT_CALL(*switchAction_, DisableNearlinkToOff()).WillOnce(Return(NL_ERR_INTERNAL_ERROR));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF),
        NL_ERR_INTERNAL_ERROR);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_009 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_010
 * @tc.desc: 全关星闪操作无效
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_010, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_010 start");
    EXPECT_CALL(*switchAction_, DisableNearlinkToOff()).WillOnce(Return(NL_ERR_INVALID_SWITCH_OPERATION));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF),
        NL_NO_ERROR);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_010 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_011
 * @tc.desc: 半开星闪
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_011, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_011 start");
    EXPECT_CALL(*switchAction_, EnableNearlinkToHalf()).WillOnce(Return(NL_NO_ERROR));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF),
        NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_HALF);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_011 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_012
 * @tc.desc: 半开星闪失败
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_012, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_012 start");
    EXPECT_CALL(*switchAction_, EnableNearlinkToHalf()).WillOnce(Return(NL_ERR_INTERNAL_ERROR));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF),
        NL_ERR_INTERNAL_ERROR);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_012 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_013
 * @tc.desc: 半开星闪操作无效
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_013, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_013 start");
    EXPECT_CALL(*switchAction_, EnableNearlinkToHalf()).WillOnce(Return(NL_ERR_INVALID_SWITCH_OPERATION));
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF),
        NL_NO_ERROR);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_013 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_014
 * @tc.desc: 反复开星闪、关星闪（1），最后一次操作为关星闪，目标状态为全关
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_014, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_014 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlink(_)).WillOnce(Return(NL_NO_ERROR));
        EXPECT_CALL(*switchAction_, DisableNearlink()).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 3);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_ON);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);
    WAIT_CACHED_EVENT_COMPLETE;

    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    switchModule_->disableStatus_ = NearlinkDisableStatus::DISABLING_TO_OFF;
    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_OFF);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->disableStatus_, NearlinkDisableStatus::STANDING_BY);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    HILOGI("NearlinkSwitchModuleTest_014 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_015
 * @tc.desc: 反复开星闪、关星闪（2），最后一次操作为关星闪，目标状态为半关
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_015, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_015 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlink(_)).WillOnce(Return(NL_NO_ERROR));
        EXPECT_CALL(*switchAction_, DisableNearlink()).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 3);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_ON);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);
    WAIT_CACHED_EVENT_COMPLETE;

    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    switchModule_->disableStatus_ = NearlinkDisableStatus::DISABLING_TO_HALF;
    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_OFF);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->disableStatus_, NearlinkDisableStatus::DISABLING_TO_HALF);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_HALF);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->disableStatus_, NearlinkDisableStatus::STANDING_BY);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    HILOGI("NearlinkSwitchModuleTest_015 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_016
 * @tc.desc: 反复开星闪、关星闪（3），最后一次操作为开星闪
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_016, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_016 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlink(_)).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 4);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_ON);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);
    WAIT_CACHED_EVENT_COMPLETE;

    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_016 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_017
 * @tc.desc: 开星闪动作去重
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_017, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_017 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlink(_)).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);

    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 4);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_ON);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    WAIT_CACHED_EVENT_COMPLETE;
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_017 start");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_018
 * @tc.desc: 全关星闪动作去重
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_018, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_018 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, DisableNearlinkToOff()).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF), NL_NO_ERROR);

    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 4);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_OFF);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    WAIT_CACHED_EVENT_COMPLETE;
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_018 start");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_019
 * @tc.desc: 半开星闪动作去重
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_019, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_019 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlinkToHalf()).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 4);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_HALF);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    WAIT_CACHED_EVENT_COMPLETE;
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_019 start");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_020
 * @tc.desc: 关闭星闪动作去重
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_020, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_020 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, DisableNearlink()).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 4);

    switchModule_->disableStatus_ = NearlinkDisableStatus::DISABLING_TO_HALF;
    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_HALF);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->disableStatus_, NearlinkDisableStatus::STANDING_BY);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    WAIT_CACHED_EVENT_COMPLETE;
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_020 start");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_021
 * @tc.desc: 忽略非预期事件（1）开启后的半开
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_021, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_021 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlink(_)).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 1);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_ON);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    WAIT_CACHED_EVENT_COMPLETE;
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_021 start");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_022
 * @tc.desc: 忽略非预期事件（2）半关后的关闭
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_022, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_022 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlinkToHalf()).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 1);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_HALF);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    WAIT_CACHED_EVENT_COMPLETE;
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_022 start");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_023
 * @tc.desc: 忽略非预期事件（3）全关后的关闭
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_023, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_023 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, DisableNearlinkToOff()).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK_TO_OFF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 1);

    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_OFF);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    WAIT_CACHED_EVENT_COMPLETE;
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    HILOGI("NearlinkSwitchModuleTest_023 start");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_024
 * @tc.desc: Test switch module timer dfr
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_024, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_024 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlink(_)).WillOnce(Return(NL_NO_ERROR));
    }

    switchModule_->taskTimeout_ = 10000;  // 10ms
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 4);

    // 10 ms timeout, clear all state and cache event.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    HILOGI("NearlinkSwitchModuleTest_024 start");
}

 /**
 * @tc.name: NearlinkSwitchModuleTest_025
 * @tc.desc: Test DeduplicateCachedEvent function
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_025, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_025 start");

    switchModule_->cachedEventVec_.push_back(NearlinkSwitchEvent::ENABLE_NEARLINK);
    switchModule_->cachedEventVec_.push_back(NearlinkSwitchEvent::DISABLE_NEARLINK);
    switchModule_->cachedEventVec_.push_back(NearlinkSwitchEvent::ENABLE_NEARLINK);
    switchModule_->cachedEventVec_.push_back(NearlinkSwitchEvent::ENABLE_NEARLINK);
    switchModule_->cachedEventVec_.push_back(NearlinkSwitchEvent::DISABLE_NEARLINK);
    switchModule_->DeduplicateCachedEvent(NearlinkSwitchEvent::ENABLE_NEARLINK);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 1);
    EXPECT_EQ(switchModule_->cachedEventVec_.back(), NearlinkSwitchEvent::DISABLE_NEARLINK);

    HILOGI("NearlinkSwitchModuleTest_025 start");
}
} // Nearlink
} // OHOS
