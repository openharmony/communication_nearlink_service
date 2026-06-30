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

#include "nearlink_datashare_helper.h"
#include "nearlink_access_token_mock.h"
#include "SleInterfaceManager.h"
#include "ClassCreator.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class NearlinkDataShareHelperTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkDataShareHelperTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    OHOS::Nearlink::NearlinkAccessTokenMock::SetNativeTokenInfo();
    HILOGI("SetUpTestCase end");
}

void NearlinkDataShareHelperTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
}

void NearlinkDataShareHelperTest::SetUp()
{
    HILOGI("SetUp start");
}

void NearlinkDataShareHelperTest::TearDown()
{
    HILOGI("TearDown start");
}

/**
 * @tc.number: GetValue001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDataShareHelperTest, GetValue001, TestSize.Level1)
{
    HILOGI("GetValue001 start");

    int32_t state = NearlinkDataShareHelper::GetInstance().GetSwitchState();
    bool isValidState = state >= 0 && state <= 2; // nearlink state: 0-off, 1-on, 2-half
    EXPECT_EQ(isValidState, true);
}

/**
 * @tc.number: GetValue002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkDataShareHelperTest, GetValue002, TestSize.Level1)
{
    HILOGI("GetValue002 start");

    std::unique_ptr<SleInterfaceAdapter> adapter(
        ClassCreator<SleInterfaceAdapter>::NewInstance(ADAPTER_NAME_SLE));
    ASSERT_NE(adapter, nullptr);
    std::string name = adapter->GetLocalName();
    std::cout << name << std::endl;
    EXPECT_NE(name, "");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS