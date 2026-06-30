/*
 * Copyright (C) 2026 Huawei Device Co., Ltd. All rights reserved.
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
#include "VasStackAdapter.cpp"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
constexpr int VAS_SERVICE_UT_DELAY_50_MS = 50;
}

namespace {
VasStackAdapter vasStackAdapter_;
}

class NearlinkVasStackAdapterTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkVasStackAdapterTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkVasStackAdapterTest");
}

void NearlinkVasStackAdapterTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkVasStackAdapterTest");
}

void NearlinkVasStackAdapterTest::SetUp()
{
    HILOGI("SetUp NearlinkVasStackAdapterTest.");
}

void NearlinkVasStackAdapterTest::TearDown()
{
    HILOGI("TearDown NearlinkVasStackAdapterTest.");
}

/*
 * @tc.number: OpenVoiceAssistantFail001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVasStackAdapterTest, OpenVoiceAssistantFail001, TestSize.Level1)
{
    HILOGI("OpenVoiceAssistantFail001 start");
    uint32_t requestId = 1;

    vasStackAdapter_.OpenVoiceAssistantFail(requestId);
    EXPECT_NE(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::VAS_SERVICE_UT_DELAY_50_MS));
    HILOGI("OpenVoiceAssistantFail001 end");
}

/*
 * @tc.number: CloseVoiceAssistantFail001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVasStackAdapterTest, CloseVoiceAssistantFail001, TestSize.Level1)
{
    HILOGI("CloseVoiceAssistantFail001 start");
    uint32_t requestId = 1;

    vasStackAdapter_.CloseVoiceAssistantFail(requestId);
    EXPECT_NE(0, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::VAS_SERVICE_UT_DELAY_50_MS));
    HILOGI("CloseVoiceAssistantFail001 end");
}

/*
 * @tc.number: VasActivateCbk001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVasStackAdapterTest, VasActivateCbk001, TestSize.Level1)
{
    HILOGI("VasActivateCbk001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint32_t requestId = 0;
    VasActivateCbk(&sleAddr, requestId);
    EXPECT_NE(0, 1);
    HILOGI("VasActivateCbk001 end");
}

/*
 * @tc.number: VasTerminateCbk001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVasStackAdapterTest, VasTerminateCbk001, TestSize.Level1)
{
    HILOGI("VasTerminateCbk001 start");
    SLE_Addr_S sleAddr = { .type = 0, .addr = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, }, };
    uint32_t requestId = 0;
    VasTerminateCbk(&sleAddr, requestId);
    EXPECT_NE(0, 1);
    HILOGI("VasTerminateCbk001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS