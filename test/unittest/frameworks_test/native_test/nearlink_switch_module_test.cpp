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
#include <condition_variable>
#include <future>
#include <mutex>
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
    MOCK_METHOD(NlErrCode, EnableNearlink,
        (SleAutoConnectPolicy, int32_t, const NearlinkSwitchActionValidChecker &), (override));
    MOCK_METHOD(NlErrCode, DisableNearlink, (), (override));
    MOCK_METHOD(NlErrCode, DisableNearlinkToOff, (), (override));
    MOCK_METHOD(NlErrCode, EnableNearlinkToHalf,
        (int32_t, const NearlinkSwitchActionValidChecker &), (override));
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
    EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_NO_ERROR));
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
    EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_ERR_INTERNAL_ERROR));
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
    EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_ERR_INVALID_SWITCH_OPERATION));
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
    EXPECT_CALL(*switchAction_, EnableNearlinkToHalf(_, _)).WillOnce(Return(NL_NO_ERROR));
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
    EXPECT_CALL(*switchAction_, EnableNearlinkToHalf(_, _)).WillOnce(Return(NL_ERR_INTERNAL_ERROR));
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
    EXPECT_CALL(*switchAction_, EnableNearlinkToHalf(_, _)).WillOnce(Return(NL_ERR_INVALID_SWITCH_OPERATION));
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
        EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_NO_ERROR));
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
        EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_NO_ERROR));
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
        EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_NO_ERROR));
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
        EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_NO_ERROR));
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
        EXPECT_CALL(*switchAction_, EnableNearlinkToHalf(_, _)).WillOnce(Return(NL_NO_ERROR));
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
        EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_NO_ERROR));
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
        EXPECT_CALL(*switchAction_, EnableNearlinkToHalf(_, _)).WillOnce(Return(NL_NO_ERROR));
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
 * @tc.desc: 开关动作超时后下发最近一次缓存事件（队尾），其余缓存事件清除；
 *           无缓存事件后流程结束
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_024, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_024 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_NO_ERROR));
        // 超时后队尾事件 DISABLE_NEARLINK 被下发处理
        EXPECT_CALL(*switchAction_, DisableNearlink()).WillOnce(Return(NL_NO_ERROR));
    }

    switchModule_->taskTimeout_ = 10000;  // 10ms
    // 首个开启动作指定 10ms 加载窗口：动作返回后超时窗口重挂为 10ms(自身)，随后触发超时
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK,
        SleAutoConnectPolicy::AUTO_CONN_GENERAL, 10), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 4);

    // 第一次超时(重挂 10ms)后下发队尾 DISABLE_NEARLINK；第二次超时无缓存事件后流程结束
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 0);

    HILOGI("NearlinkSwitchModuleTest_024 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_026
 * @tc.desc: 开关动作超时后仅下发最近一次缓存事件（队尾），其余缓存事件清除，
 *           开关动作正常完成后连续超时次数清零
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_026, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_026 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_NO_ERROR));
        // 超时后仅队尾事件 ENABLE_NEARLINK 被下发，DISABLE_NEARLINK 与 ENABLE_NEARLINK_TO_HALF 被清除
        EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 3);

    // 取消真实定时任务后手动触发一次超时
    switchModule_->ffrtQueue_.cancel(switchModule_->taskTimeoutHandle_);
    switchModule_->OnTaskTimeout(switchModule_->actionGeneration_);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 1);

    WAIT_CACHED_EVENT_COMPLETE;
    // 队尾 ENABLE_NEARLINK 被下发处理
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);

    // 开关动作正常完成后连续超时次数清零
    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_ON);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 0);

    HILOGI("NearlinkSwitchModuleTest_026 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_027
 * @tc.desc: 开关动作连续超时达到3次后清空缓存队列，不再下发缓存事件
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_027, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_027 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_NO_ERROR));
        // 第一次超时后下发队尾 ENABLE_NEARLINK_TO_HALF
        EXPECT_CALL(*switchAction_, EnableNearlinkToHalf(_, _)).WillOnce(Return(NL_NO_ERROR));
        // 第二次超时后下发队尾 DISABLE_NEARLINK
        EXPECT_CALL(*switchAction_, DisableNearlink()).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF), NL_NO_ERROR);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 2);

    // 第一次超时：下发队尾 ENABLE_NEARLINK_TO_HALF
    switchModule_->ffrtQueue_.cancel(switchModule_->taskTimeoutHandle_);
    switchModule_->OnTaskTimeout(switchModule_->actionGeneration_);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 1);
    WAIT_CACHED_EVENT_COMPLETE;
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    // 处理期间再次缓存 DISABLE_NEARLINK
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 1);

    // 第二次超时：下发队尾 DISABLE_NEARLINK
    switchModule_->ffrtQueue_.cancel(switchModule_->taskTimeoutHandle_);
    switchModule_->OnTaskTimeout(switchModule_->actionGeneration_);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 2);
    WAIT_CACHED_EVENT_COMPLETE;
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);

    // 处理期间再次缓存 ENABLE_NEARLINK_TO_HALF
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK_TO_HALF), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 1);

    // 第三次超时：达到连续超时上限，清空缓存队列，不再下发
    switchModule_->ffrtQueue_.cancel(switchModule_->taskTimeoutHandle_);
    switchModule_->OnTaskTimeout(switchModule_->actionGeneration_);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 0);

    HILOGI("NearlinkSwitchModuleTest_027 end");
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

/**
 * @tc.name: NearlinkSwitchModuleTest_028
 * @tc.desc: 耗时动作在锁外执行：不阻塞其它事件与超时补救；被超时判死的旧动作返回
 *           不再覆盖重放新动作的状态
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_028, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_028 start");
    switchModule_->taskTimeout_ = 5000000;  // 5s，避免自动超时干扰

    std::mutex mtx;
    std::condition_variable cv;
    bool actionStarted = false;
    std::promise<void> releaseAction;
    std::shared_future<void> releaseFut = releaseAction.get_future().share();
    EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Invoke(
        [&](SleAutoConnectPolicy, int32_t, const NearlinkSwitchActionValidChecker &) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            actionStarted = true;
        }
        cv.notify_all();
        releaseFut.wait();      // 模拟耗时动作（如 SA 慢加载）长时间未返回
        return NL_ERR_INTERNAL_ERROR;   // 旧动作最终失败返回
    }));
    EXPECT_CALL(*switchAction_, DisableNearlink()).WillOnce(Return(NL_NO_ERROR));

    // 动作在独立线程执行，阻塞在耗时动作内（不持开关锁）
    std::thread actionThread([this]() {
        switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK);
    });
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&actionStarted] { return actionStarted; });
    }
    switchModule_->ffrtQueue_.cancel(switchModule_->taskTimeoutHandle_);

    // 耗时动作不持锁：其它开关事件可正常进入并缓存
    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::DISABLE_NEARLINK), NL_NO_ERROR);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 1);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);

    // 超时任务不被耗时动作阻塞，可立即执行补救：下发队尾事件
    switchModule_->OnTaskTimeout(switchModule_->actionGeneration_);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 1);
    WAIT_CACHED_EVENT_COMPLETE;
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);  // 队尾 DISABLE 动作已启动
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    // 旧动作失败返回（已被代次作废）：不得覆盖新动作状态、不得清零计数
    uint32_t genBeforeRelease = switchModule_->actionGeneration_;
    releaseAction.set_value();
    actionThread.join();
    WAIT_CACHED_EVENT_COMPLETE;
    EXPECT_EQ(switchModule_->actionGeneration_, genBeforeRelease);
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 1);

    switchModule_->ffrtQueue_.cancel(switchModule_->taskTimeoutHandle_);
    HILOGI("NearlinkSwitchModuleTest_028 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_029
 * @tc.desc: 动作终结后到达的陈旧超时任务被跳过，无副作用
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_029, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_029 start");
    {
        InSequence seq;
        EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Return(NL_NO_ERROR));
    }

    EXPECT_EQ(switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK), NL_NO_ERROR);
    uint32_t genAtStart = switchModule_->actionGeneration_;
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);

    // 动作正常完成（动作终结时递增代次）
    switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::NEARLINK_ON);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 0);
    EXPECT_EQ(switchModule_->actionGeneration_, genAtStart + 1);

    // 携带旧代次的陈旧超时任务：直接跳过，无副作用
    switchModule_->OnTaskTimeout(genAtStart);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 0);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    HILOGI("NearlinkSwitchModuleTest_029 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_030
 * @tc.desc: 动作超时窗口分两段：动作执行期 = SA 加载窗口 + 动作自身窗口（合法加载等待不被误判），
 *           动作返回后重挂为固定的动作自身窗口（等待状态变化阶段不继承加载窗口）
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_030, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_030 start");
    switchModule_->taskTimeout_ = 50000;  // 动作自身窗口 50ms

    std::mutex mtx;
    std::condition_variable cv;
    bool actionStarted = false;
    std::promise<void> releaseAction;
    std::shared_future<void> releaseFut = releaseAction.get_future().share();
    EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Invoke(
        [&](SleAutoConnectPolicy, int32_t, const NearlinkSwitchActionValidChecker &) {
            {
                std::lock_guard<std::mutex> lock(mtx);
                actionStarted = true;
            }
            cv.notify_all();
            releaseFut.wait();      // 模拟 SA 加载等待（在 200ms 加载窗口内）
            return NL_NO_ERROR;
        }));

    // 加载窗口 200ms：动作执行期窗口 = 50ms + 200ms = 250ms
    std::thread actionThread([this]() {
        switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK,
            SleAutoConnectPolicy::AUTO_CONN_GENERAL, 200);
    });
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&actionStarted] { return actionStarted; });
    }
    uint32_t genAtStart = switchModule_->actionGeneration_;

    // 动作执行期：等待 100ms 已超过动作自身窗口(50ms)但仍在执行期窗口(250ms)内，不被误判
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->actionGeneration_, genAtStart);

    // 放行动作返回：超时窗口应重挂为固定的动作自身窗口(50ms)
    releaseAction.set_value();
    actionThread.join();

    // 返回后短时间内不判死（重挂生效，未残留执行期长窗口）
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(switchModule_->isNlSwitchProcessing_);

    // 超过重挂窗口(50ms)后判死；无缓存事件，流程结束且计数清零、代次仅递增一次
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->actionGeneration_, genAtStart + 1);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 0);

    HILOGI("NearlinkSwitchModuleTest_030 end");
}

/**
 * @tc.name: NearlinkSwitchModuleTest_031
 * @tc.desc: 在途代次校验器：动作在途时校验为真；被超时判死后校验为假，动作不得再下发
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkSwitchModuleTest, NearlinkSwitchModuleTest_031, TestSize.Level1)
{
    HILOGI("NearlinkSwitchModuleTest_031 start");
    switchModule_->taskTimeout_ = 5000000;  // 5s，避免自动超时干扰

    std::mutex mtx;
    std::condition_variable cv;
    bool actionStarted = false;
    bool validBeforeTimeout = false;
    bool validAfterTimeout = true;
    std::promise<void> releaseAction;
    std::shared_future<void> releaseFut = releaseAction.get_future().share();
    EXPECT_CALL(*switchAction_, EnableNearlink(_, _, _)).WillOnce(Invoke(
        [&](SleAutoConnectPolicy, int32_t, const NearlinkSwitchActionValidChecker &actionValidChecker) {
            validBeforeTimeout = actionValidChecker != nullptr && actionValidChecker();
            {
                std::lock_guard<std::mutex> lock(mtx);
                actionStarted = true;
            }
            cv.notify_all();
            releaseFut.wait();      // 模拟 SA 加载等待，期间动作被超时判死
            validAfterTimeout = actionValidChecker != nullptr && actionValidChecker();
            return NL_ERR_INVALID_SWITCH_OPERATION;   // 复核失败不下发，动作直接返回
        }));

    // 动作在独立线程执行，阻塞在耗时动作内（不持开关锁）
    std::thread actionThread([this]() {
        switchModule_->ProcessNearlinkSwitchEvent(NearlinkSwitchEvent::ENABLE_NEARLINK);
    });
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&actionStarted] { return actionStarted; });
    }

    // 动作在途：校验器为真
    EXPECT_TRUE(validBeforeTimeout);

    // 模拟超时判死（如动作内下发前的等待超过动作超时窗口）
    switchModule_->ffrtQueue_.cancel(switchModule_->taskTimeoutHandle_);
    switchModule_->OnTaskTimeout(switchModule_->actionGeneration_);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);

    // 判死后释放动作：校验器为假，动作不得再下发
    releaseAction.set_value();
    actionThread.join();
    EXPECT_FALSE(validAfterTimeout);
    EXPECT_FALSE(switchModule_->isNlSwitchProcessing_);
    EXPECT_EQ(switchModule_->consecutiveTimeoutCnt_, 0);
    EXPECT_EQ(switchModule_->cachedEventVec_.size(), 0);

    HILOGI("NearlinkSwitchModuleTest_031 end");
}
} // Nearlink
} // OHOS
