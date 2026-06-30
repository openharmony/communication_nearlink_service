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

#include "nearlink_timer.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

static std::shared_ptr<NearlinkTimer> g_testTimer = nullptr;
static int g_testNum = 0;

class NearlinkTimerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkTimerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
}

void NearlinkTimerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
}

void NearlinkTimerTest::SetUp()
{
    HILOGI("SetUp start");
}

void NearlinkTimerTest::TearDown()
{
    HILOGI("TearDown start");
}

/**
 * @tc.number: NearlinkTimerTest001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkTimerTest, NearlinkTimerTest001, TestSize.Level1)
{
    HILOGI("NearlinkTimerTest001 start");
    auto timeoutFunc = []() -> void {
        g_testNum++;
    };
    g_testTimer = std::make_shared<NearlinkTimer>(timeoutFunc);
    ASSERT_EQ(g_testTimer->Start(300, false), true); // 300ms
    ASSERT_EQ(g_testTimer->Start(500, false), true); // 500ms
    ASSERT_EQ(g_testTimer->Start(700, false), true); // 700ms
    ASSERT_EQ(g_testTimer->Start(1000, false), true); // 1000ms
    ASSERT_EQ(g_testTimer->Start(5000, false), true); // 5000ms
    ASSERT_EQ(g_testTimer->Start(99999, false), true); // 99999ms
    ASSERT_EQ(g_testTimer->Stop(), true);

    ASSERT_EQ(g_testTimer->Start(99999, true), true); // 99999ms
    g_testTimer = nullptr;

    g_testTimer = std::make_shared<NearlinkTimer>(timeoutFunc);
    uint32_t uintMaxNum = 4294967295;
    ASSERT_EQ(g_testTimer->Start(static_cast<int>(uintMaxNum), false), false);  // 4294967295ms
    ASSERT_EQ(g_testTimer->Start(-1, false), false);  // -1ms
    ASSERT_EQ(g_testTimer->Start(2147483647, false), true);  // int的最大值
    ASSERT_EQ(g_testTimer->Start(0, false), false);  // 0ms
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS