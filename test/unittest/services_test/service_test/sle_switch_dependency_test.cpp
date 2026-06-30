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

#include "nearlink_errorcode.h"
#include "nearlink_access_token_mock.h"
#include "SleSwitchDependency.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
using namespace testing;
using namespace testing::ext;
namespace {
    int g_callbackTriggeredCount = 0;
    bool g_isDataShareReady = false;
}

bool SleDataShareCheckUtils::IsDataShareReady()
{
    return g_isDataShareReady;
}

class SleSwitchDependencyTest : public testing::Test {
public:
    SleSwitchDependencyTest() {}
    ~SleSwitchDependencyTest() {}

    static void SetUpTestCase(void)
    {
        HILOGI("SetUpTestCase SleSwitchDependencyTest.");
        NearlinkAccessTokenMock::SetNativeTokenInfo();
    }
    static void TearDownTestCase(void)
    {
        HILOGI("TearDownTestCase SleSwitchDependencyTest");
    }
    void SetUp()
    {
        g_isDataShareReady = false;
    }
    void TearDown() {}
};

void SleCommonEventSubscriber::OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data)
{}

/*
 * @tc.number: UnitTest_SleSwitchDependencyTest_001
 * @tc.name: data share is not ready
 * @tc.desc: name :
 */
HWTEST_F(SleSwitchDependencyTest, UnitTest_SleSwitchDependencyTest_001, TestSize.Level1)
{
    HILOGI("UnitTest_SleSwitchDependencyTest_001 start");

    g_isDataShareReady = false;
    g_callbackTriggeredCount = 0;
    auto func = []() -> void { ++g_callbackTriggeredCount; };
    auto dependency = std::make_shared<SleSwitchDependency>(func);
    int ret = dependency->Init();
    EXPECT_EQ(ret, 0);

    // Wait system ability callback
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(g_callbackTriggeredCount, 0);
    HILOGI("UnitTest_SleSwitchDependencyTest_001 end");
}

/*
 * @tc.number: UnitTest_SleSwitchDependencyTest_002
 * @tc.name: data share is already ready
 * @tc.desc: name :
 */
HWTEST_F(SleSwitchDependencyTest, UnitTest_SleSwitchDependencyTest_002, TestSize.Level1)
{
    HILOGI("UnitTest_SleSwitchDependencyTest_002 start");

    g_isDataShareReady = true;
    g_callbackTriggeredCount = 0;
    auto func = []() -> void { ++g_callbackTriggeredCount; };
    auto dependency = std::make_shared<SleSwitchDependency>(func);
    int ret = dependency->Init();
    EXPECT_EQ(ret, 0);

    // Wait system ability callback
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(g_callbackTriggeredCount, 1);
    HILOGI("UnitTest_SleSwitchDependencyTest_002 end");
}

/*
 * @tc.number: UnitTest_SleSwitchDependencyTest_003
 * @tc.name: Check dependency callback can't double trigger
 * @tc.desc: name :
 */
HWTEST_F(SleSwitchDependencyTest, UnitTest_SleSwitchDependencyTest_003, TestSize.Level1)
{
    HILOGI("UnitTest_SleSwitchDependencyTest_003 start");

    g_isDataShareReady = true;
    g_callbackTriggeredCount = 0;
    auto func = []() -> void { ++g_callbackTriggeredCount; };
    auto dependency = std::make_shared<SleSwitchDependency>(func);
    int ret = dependency->Init();
    EXPECT_EQ(ret, 0);
    // Wait system ability callback
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    dependency->CheckAllDependencySatisfied();
    dependency->CheckAllDependencySatisfied();
    EXPECT_EQ(g_callbackTriggeredCount, 1);
    HILOGI("UnitTest_SleSwitchDependencyTest_003 end");
}

/*
 * @tc.number: UnitTest_SleSwitchDependencyTest_004
 * @tc.name: Dependency callback is nullptr
 * @tc.desc: name :
 */
HWTEST_F(SleSwitchDependencyTest, UnitTest_SleSwitchDependencyTest_004, TestSize.Level1)
{
    HILOGI("UnitTest_SleSwitchDependencyTest_004 start");
    g_isDataShareReady = true;
    auto dependency = std::make_shared<SleSwitchDependency>(nullptr);
    int ret = dependency->Init();
    EXPECT_EQ(ret, 0);
    // Wait system ability callback
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    HILOGI("UnitTest_SleSwitchDependencyTest_004 end");
}

} // namespace Nearlink
} // namespace OHOS