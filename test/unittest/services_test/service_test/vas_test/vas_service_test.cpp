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
#include "SleServiceManager.h"
#include "nearlink_access_token_mock.h"
#include "VasService.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
constexpr int VAS_SERVICE_UT_DELAY_50_MS = 50;
constexpr int VAS_SERVICE_UT_DELAY_1000_MS = 1000;
}

class NearlinkVasServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkVasServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkVasServiceTest start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
    SleInterfaceManager::GetInstance()->Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::VAS_SERVICE_UT_DELAY_1000_MS));
    HILOGI("SetUpTestCase NearlinkVasServiceTest end");
}

void NearlinkVasServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkVasServiceTest");
}

void NearlinkVasServiceTest::SetUp()
{
    HILOGI("SetUp NearlinkVasServiceTest.");
}

void NearlinkVasServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkVasServiceTest.");
}

/**
 * @tc.number: GetService001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVasServiceTest, GetService001, TestSize.Level1)
{
    HILOGI("GetService001 start");
    VasService* vasService = VasService::GetService();
    EXPECT_NE(nullptr, vasService);
    HILOGI("GetService001 end");
}

/**
 * @tc.number: Disable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVasServiceTest, Disable001, TestSize.Level1)
{
    HILOGI("Disable001 start");
    VasService* vasService = VasService::GetService();
    vasService->Disable();
    EXPECT_NE(nullptr, vasService);

    vasService->Disable();
    EXPECT_NE(nullptr, vasService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::VAS_SERVICE_UT_DELAY_50_MS));
    HILOGI("Disable001 end");
}

/**
 * @tc.number: Enable001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVasServiceTest, Enable001, TestSize.Level1)
{
    HILOGI("Enable001 start");
    VasService* vasService = VasService::GetService();
    vasService->Enable();
    EXPECT_NE(nullptr, vasService);

    vasService->Enable();
    EXPECT_NE(nullptr, vasService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::VAS_SERVICE_UT_DELAY_50_MS));
    HILOGI("Enable001 end");
}

/**
 * @tc.number: OpenVoiceAssistant001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVasServiceTest, OpenVoiceAssistant001, TestSize.Level1)
{
    HILOGI("OpenVoiceAssistant001 start");
    VasService* vasService = VasService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    vasService->OpenVoiceAssistant(*device);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::VAS_SERVICE_UT_DELAY_50_MS));
    EXPECT_NE(nullptr, vasService);
    HILOGI("OpenVoiceAssistant001 end");
}

/**
 * @tc.number: CloseVoiceAssistant001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkVasServiceTest, CloseVoiceAssistant001, TestSize.Level1)
{
    HILOGI("CloseVoiceAssistant001 start");
    VasService* vasService = VasService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    vasService->CloseVoiceAssistant(*device);
    EXPECT_NE(nullptr, vasService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::VAS_SERVICE_UT_DELAY_50_MS));
    HILOGI("CloseVoiceAssistant001 end");
}

/*
 * @tc.number: HandleActivateVoiceAssistant001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVasServiceTest, HandleActivateVoiceAssistant001, TestSize.Level1)
{
    HILOGI("HandleActivateVoiceAssistant001 start");
    VasService* vasService = VasService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t requestId = 0;
    vasService->HandleActivateVoiceAssistant(*device, requestId);
    EXPECT_NE(nullptr, vasService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::VAS_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandleActivateVoiceAssistant001 end");
}

/*
 * @tc.number: HandleCloseVoiceAssistant001
 * @tc.name:
 * @tc.desc:
*/
HWTEST_F(NearlinkVasServiceTest, HandleCloseVoiceAssistant001, TestSize.Level1)
{
    HILOGI("HandleCloseVoiceAssistant001 start");
    VasService* vasService = VasService::GetService();
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t requestId = 0;
    vasService->HandleCloseVoiceAssistant(*device, requestId);
    EXPECT_NE(nullptr, vasService);
    std::this_thread::sleep_for(std::chrono::milliseconds(OHOS::Nearlink::TEST::VAS_SERVICE_UT_DELAY_50_MS));
    HILOGI("HandleCloseVoiceAssistant001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS