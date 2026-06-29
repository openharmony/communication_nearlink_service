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
#include "nearlink_def.h"
#include "log_util.h"
#include "nearlink_utils.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class NearlinkUtilsTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkUtilsTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkUtilsTest.");
}

void NearlinkUtilsTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkUtilsTest.");
}

void NearlinkUtilsTest::SetUp()
{
    HILOGI("SetUp NearlinkUtilsTest.");
}

void NearlinkUtilsTest::TearDown()
{
    HILOGI("TearDown NearlinkUtilsTest.");
}

/**
 * @tc.name: TestGetNearlinkStateName001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkUtilsTest, TestGetNearlinkStateName001, TestSize.Level1)
{
    HILOGI("TestGetNearlinkStateName001 start.");
    std::string stateName = GetStateString(SleStateID::STATE_TURNING_ON);
    EXPECT_EQ(stateName, "STATE_TURNING_ON(0)");
    stateName = GetStateString(SleStateID::STATE_TURN_ON);
    EXPECT_EQ(stateName, "STATE_TURN_ON(1)");
    stateName = GetStateString(SleStateID::STATE_TURNING_OFF);
    EXPECT_EQ(stateName, "STATE_TURNING_OFF(2)");
    stateName = GetStateString(SleStateID::STATE_TURN_OFF);
    EXPECT_EQ(stateName, "STATE_TURN_OFF(3)");
    stateName = GetStateString(SleStateID::STATE_TURNING_HALF_TO_OFF);
    EXPECT_EQ(stateName, "STATE_TURNING_HALF_TO_OFF(4)");
    stateName = GetStateString(SleStateID::STATE_TURNING_OFF_TO_HALF);
    EXPECT_EQ(stateName, "STATE_TURNING_OFF_TO_HALF(5)");
    stateName = GetStateString(SleStateID::STATE_TURNING_HALF_TO_ON);
    EXPECT_EQ(stateName, "STATE_TURNING_HALF_TO_ON(6)");
    stateName = GetStateString(SleStateID::STATE_TURNING_ON_TO_HALF);
    EXPECT_EQ(stateName, "STATE_TURNING_ON_TO_HALF(7)");
    stateName = GetStateString(SleStateID::STATE_TURN_HALF);
    EXPECT_EQ(stateName, "STATE_TURN_HALF(8)");
    stateName = GetStateString(100);
    EXPECT_EQ(stateName, "Unknown");
    HILOGI("TestGetNearlinkStateName001 end.");
}

/**
 * @tc.name: TestGetTransportString001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkUtilsTest, TestGetTransportString001, TestSize.Level1)
{
    HILOGI("TestGetTransportString001 start.");
    std::string TransportString = GetTransportString(SleTransport::ADAPTER_SLB);
    EXPECT_EQ(TransportString, "ADAPTER_SLB(0)");
    TransportString = GetTransportString(SleTransport::ADAPTER_SLE);
    EXPECT_EQ(TransportString, "ADAPTER_SLE(1)");
    TransportString = GetTransportString(100);
    EXPECT_EQ(TransportString, "Unknown");
    HILOGI("TestGetTransportString001 end.");
}

/**
 * @tc.name: TestGetReasonString001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkUtilsTest, TestGetReasonString001, TestSize.Level1)
{
    HILOGI("TestGetReasonString001 start.");
    std::string ReasonString = GetReasonString(static_cast<int>(SleEventType::INTERFACE_TRIGGERED));
    EXPECT_EQ(ReasonString, "INTERFACE_TRIGGERED(0)");
    ReasonString = GetReasonString(static_cast<int>(SleEventType::AIRPLANE_TRIGGERED));
    EXPECT_EQ(ReasonString, "AIRPLANE_TRIGGERED(1)");
    ReasonString = GetReasonString(static_cast<int>(SleEventType::COLLABORATION_TRIGGERED));
    EXPECT_EQ(ReasonString, "COLLABORATION_TRIGGERED(2)");
    ReasonString = GetReasonString(static_cast<int>(SleEventType::RESTORE_TRIGGERED));
    EXPECT_EQ(ReasonString, "RESTORE_TRIGGERED(3)");
    ReasonString = GetReasonString(static_cast<int>(SleEventType::CHIP_RESET_TRIGGERED));
    EXPECT_EQ(ReasonString, "CHIP_RESET_TRIGGERED(4)");
    ReasonString = GetReasonString(static_cast<int>(SleEventType::SELF_HEALING_RETRY));
    EXPECT_EQ(ReasonString, "SELF_HEALING_RETRY(5)");
    ReasonString = GetReasonString(static_cast<int>(SleEventType::SVC_TRIGGERED));
    EXPECT_EQ(ReasonString, "SVC_TRIGGERED(6)");
    ReasonString = GetReasonString(static_cast<int>(SleEventType::SYS_STOP_TRIGGERED));
    EXPECT_EQ(ReasonString, "SYS_STOP_TRIGGERED(7)");
    ReasonString = GetReasonString(static_cast<int>(SleEventType::SYS_RESET_TRIGGERED));
    EXPECT_EQ(ReasonString, "SYS_RESET_TRIGGERED(8)");
    ReasonString = GetReasonString(static_cast<int>(SleEventType::SYS_FACTORY_RESET_TRIGGERED));
    EXPECT_EQ(ReasonString, "SYS_FACTORY_RESET_TRIGGERED(9)");
    ReasonString = GetReasonString(static_cast<int>(100));
    EXPECT_EQ(ReasonString, "Unknown");
    HILOGI("TestGetReasonString001 end.");
}

/**
 * @tc.name: TestGetConnStateString001  
 * @tc.desc: 测试 GetConnStateString 函数对不同连接状态的字符串转换  
 * @tc.type: FUNC  
 */  
HWTEST_F(NearlinkUtilsTest, TestGetConnStateString001, TestSize.Level1)  
{
    HILOGI("TestGetConnStateString001 start.");  
    std::string ConnStateString = GetConnStateString(static_cast<int>(SleConnectState::CONNECTING));  
    EXPECT_EQ(ConnStateString, "CONNECTING(0)");  
    ConnStateString = GetConnStateString(static_cast<int>(SleConnectState::CONNECTED));  
    EXPECT_EQ(ConnStateString, "CONNECTED(1)");  
    ConnStateString = GetConnStateString(static_cast<int>(SleConnectState::DISCONNECTING));  
    EXPECT_EQ(ConnStateString, "DISCONNECTING(2)");  
    ConnStateString = GetConnStateString(static_cast<int>(SleConnectState::DISCONNECTED));  
    EXPECT_EQ(ConnStateString, "DISCONNECTED(3)");   
    ConnStateString = GetConnStateString(100);  
    EXPECT_EQ(ConnStateString, "Unknown");  
    HILOGI("TestGetConnStateString001 end.");  
}

/**
 * @tc.name: TestIsValidAddress001
 * @tc.desc: 测试地址格式校验函数，覆盖所有分支
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkUtilsTest, TestIsValidAddress001, TestSize.Level1) {
    // 1. 测试空地址（返回 false）
    EXPECT_FALSE(IsValidAddress(""));
    // 2. 测试长度不符合要求（返回 false）
    EXPECT_FALSE(IsValidAddress("12:34:56:78:90")); // 长度不足
    EXPECT_FALSE(IsValidAddress("12:34:56:78:90:AB:CD")); // 长度过长
    // 3. 测试有效地址（返回 true）
    EXPECT_TRUE(IsValidAddress("12:34:56:78:90:AB"));
    // 4. 测试非法字符（返回 false）
    EXPECT_FALSE(IsValidAddress("12:34:56:78:90:AG")); // 'G' 不是合法十六进制字符
    EXPECT_FALSE(IsValidAddress("12:34:56:78:90:"));   // 缺少最后一位
    // 5. 测试分隔符错误（返回 false）
    EXPECT_FALSE(IsValidAddress("12-34-56-78-90-AB")); // 使用 '-' 而非 ':'
    EXPECT_FALSE(IsValidAddress("12:34:56:78:90AB"));  // 缺少 ':'
}

/**
 * @tc.name: TestIsValidUuid001
 * @tc.desc: 测试 UUID 格式校验函数，覆盖所有分支
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkUtilsTest, TestIsValidUuid001, TestSize.Level1) {
    // 1. 测试长度不符合要求（返回 false）
    EXPECT_FALSE(IsValidUuid("12345678-1234-5678")); // 长度不足
    EXPECT_FALSE(IsValidUuid("12345678-1234-5678-1234-5678901234567")); // 长度过长
    // 2. 测试连字符位置错误（返回 false）
    EXPECT_FALSE(IsValidUuid("123456781-2345-6789-0ABC-DEF123456789")); // 第8位错误
    EXPECT_FALSE(IsValidUuid("12345678-12345-6789-0ABC-DEF123456789")); // 第13位错误
    // 3. 测试非法字符（返回 false）
    EXPECT_FALSE(IsValidUuid("12345678-1234-5678-0ABC-DEF12345678G")); // 'G' 不是合法十六进制字符
    // 4. 测试有效 UUID（返回 true）
    EXPECT_TRUE(IsValidUuid("12345678-1234-5678-0ABC-DEF123456789"));
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
