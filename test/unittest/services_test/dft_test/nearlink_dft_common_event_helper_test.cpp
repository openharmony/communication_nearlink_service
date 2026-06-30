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
#include "common_event.h"
#include "common_event_data.h"
#include "nearlink_dft_common_event_helper.h"


using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
class NearlinkDftCommonEventHelperTest : public testing::Test {
public:
    NearlinkDftCommonEventHelperTest() = default;
    ~NearlinkDftCommonEventHelperTest() = default;

    static void SetUpTestCase()
    {}
    static void TearDownTestCase()
    {}

    void SetUp(){};
    void TearDown(){};
};

/**
 * @tc.name: Test_PublishEvent_001
 * @tc.desc: Verify the PublishEvent function.
 * @tc.type: FUNC
 */
HWTEST_F(NearlinkDftCommonEventHelperTest, Test_PublishEvent_001, TestSize.Level1)
{
    OHOS::AAFwk::Want want;
    want.SetAction("usual.event.nearlink.host.AUDIO_EXCEP");
    std::vector<std::string> permissions {"ohos.permission.MANAGE_NEARLINK"};
    EXPECT_TRUE(NearlinkHelper::NearlinkDftCommonEventHelper::PublishEvent(
        want, -1, false, false, permissions));
}

}  // namespace Nearlink
}  // namespace OHOS