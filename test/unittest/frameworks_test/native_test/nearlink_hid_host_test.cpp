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

#include "nearlink_access_token_mock.h"
#include "nearlink_hid_host.h"
#include "nearlink_host.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class NearlinkHidHostTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkHidHostTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    if (!NearlinkHost::GetInstance().IsSleEnabled()) {
        HILOGI("start enable nearlink");
        NearlinkHost::GetInstance().EnableNl();
        sleep(2);
    }
}

void NearlinkHidHostTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
}

void NearlinkHidHostTest::SetUp()
{
    HILOGI("SetUp start");
}

void NearlinkHidHostTest::TearDown()
{
    HILOGI("TearDown start");
}


/**
 * @tc.number: NearlinkHidHostTest001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkHidHostTest, NearlinkHidHostTest001, TestSize.Level1)
{
    HILOGI("NearlinkHidHostTest001 start");
    std::string addr = "00:11:22:33:44:55";
    std::string report = "0000";
    NearlinkHidHost *nearlinkHidHost =NearlinkHidHost::GetProfile();
    nearlinkHidHost->HidHostSetReport(addr,2,report);
    HILOGI("NearlinkHidHostTest001 end");
}

} // namespace TEST
}// namespace Nearlink
} // namespace OHOS