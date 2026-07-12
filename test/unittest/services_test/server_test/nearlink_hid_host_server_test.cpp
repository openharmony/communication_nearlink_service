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
#include "log.h"
#include "SleInterfaceManager.h"
#include "nearlink_hid_host_server.h"
#include "nearlink_access_token_mock.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
constexpr int DELAY_LITTLE_MS = 100;

class NearlinkHidHostServerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkHidHostServerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkHidHostServerTest");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkHidHostServerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkHidHostServerTest");
}

void NearlinkHidHostServerTest::SetUp()
{
    HILOGI("SetUp NearlinkHidHostServerTest.");
}

void NearlinkHidHostServerTest::TearDown()
{
    HILOGI("TearDown NearlinkHidHostServerTest.");
}

/**
 * @tc.name: NearlinkHidHostServerTest001
 * @tc.desc:
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkHidHostServerTest, NearlinkHidHostServerTest001, TestSize.Level1)
{
    HILOGI("NearlinkHidHostServerTest001 start");
    sptr <NearlinkHidHostServer> hidHostServer = new(std::nothrow) NearlinkHidHostServer();
    ASSERT_NE(hidHostServer, nullptr);
    std::string addr = "00:11:22:33:44:55";
    std::string report = "0000";
    int result;
    EXPECT_EQ(NL_NO_ERROR, hidHostServer->HidHostSetReport(addr, 2, report, result));
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_LITTLE_MS));
    HILOGI("NearlinkHidHostServerTest001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS