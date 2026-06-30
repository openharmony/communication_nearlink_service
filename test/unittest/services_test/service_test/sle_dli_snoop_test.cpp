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

#include "nearlink_access_token_mock.h"
#include "SleDliSnoop.h"
#include "log.h"

using namespace OHOS::Nearlink;
using namespace testing;
using namespace testing::ext;

namespace {
    constexpr int INVALID_FD = -1;
    std::unique_ptr<SleDliSnoop> g_dliSnoopPtr = nullptr;
}

namespace OHOS {
namespace Nearlink {
namespace TEST {
class SleDliSnoopTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void SleDliSnoopTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase SleDliSnoopTest.");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void SleDliSnoopTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase SleDliSnoopTest");
}

void SleDliSnoopTest::SetUp()
{
    g_dliSnoopPtr = std::make_unique<SleDliSnoop>();
    HILOGI("SetUp SleDliSnoopTest.");
}

void SleDliSnoopTest::TearDown()
{
    g_dliSnoopPtr->SnoopShutDown();
    g_dliSnoopPtr = nullptr;
    HILOGI("TearDown SleDliSnoopTest.");
}

/**
 * @tc.name: Dli_Snoop_test_001
 * @tc.desc: SnoopStartUp test
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_001, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_001 enter");
    g_dliSnoopPtr->SnoopStartUp();
    g_dliSnoopPtr->CreateSnoopFile(true);
    sleep(1); // sleep 1s

    EXPECT_EQ(true, g_dliSnoopPtr->isLogging_);
    EXPECT_EQ(true, g_dliSnoopPtr->isModuleStarted_);
    EXPECT_EQ(false, g_dliSnoopPtr->files_.empty());
    EXPECT_NE(INVALID_FD, g_dliSnoopPtr->logFileFd_);

    HILOGI("Dli_Snoop_test_001 end");
}

/**
 * @tc.name: Dli_Snoop_test_002
 * @tc.desc: SnoopShutDown test
 * @tc.type: FUNC
 */
HWTEST_F(SleDliSnoopTest, Dli_Snoop_test_002, TestSize.Level1)
{
    HILOGI("Dli_Snoop_test_002 enter");
    g_dliSnoopPtr->SnoopShutDown();

    EXPECT_EQ(false, g_dliSnoopPtr->isLogging_);
    EXPECT_EQ(false,  g_dliSnoopPtr->isModuleStarted_);
    EXPECT_EQ(INVALID_FD, g_dliSnoopPtr->logFileFd_);

    HILOGI("Dli_Snoop_test_002 end");
}
}  // namespace TEST
}  // namespace Nearlink
}  // namespace OHOS