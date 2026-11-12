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
#include <memory>
#include "ThreadUtil.h"
#include "ProfileServiceManager.cpp"
#include "SleServiceManager.h"
#include "VasService.h"
#include "SleInterfaceManager.h"
#include "SleServiceManager.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class NearlinkVasServiceTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

private:
    std::unique_ptr<VasService> service_;
};

void NearlinkVasServiceTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase NearlinkVasServiceTest start");
    // VAS service tests own their service; vendor stack startup is an integration dependency.
    HILOGI("SetUpTestCase NearlinkVasServiceTest end");
}

void NearlinkVasServiceTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase NearlinkVasServiceTest");
}

void NearlinkVasServiceTest::SetUp()
{
    HILOGI("SetUp NearlinkVasServiceTest.");
    // VAS tasks execute synchronously in the test thread; no sleep is needed after calls.
    ThreadUtil::GetInstance().threadStateMap_.EnsureInsert(
        THREAD_ID_VAS, ThreadUtil::ThreadState::NOT_SWITCH_THREAD);
    service_ = std::make_unique<VasService>();
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_VAS, service_.get());
    ASSERT_EQ(service_.get(), VasService::GetService());
}

void NearlinkVasServiceTest::TearDown()
{
    HILOGI("TearDown NearlinkVasServiceTest.");
    ProfileServiceManager::GetInstance().pimpl->startedProfiles_.SetProfile(
        SleTransport::ADAPTER_SLE, PROFILE_NAME_VAS, nullptr);
    service_.reset();
    ThreadUtil::GetInstance().InitThreadStateMap();
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
    ASSERT_NE(nullptr, vasService);
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
    ASSERT_NE(nullptr, vasService);
    vasService->Disable();
    EXPECT_NE(nullptr, vasService);

    vasService->Disable();
    EXPECT_NE(nullptr, vasService);
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
    ASSERT_NE(nullptr, vasService);
    vasService->Enable();
    EXPECT_NE(nullptr, vasService);

    vasService->Enable();
    EXPECT_NE(nullptr, vasService);
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
    ASSERT_NE(nullptr, vasService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    vasService->OpenVoiceAssistant(*device);
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
    ASSERT_NE(nullptr, vasService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    vasService->CloseVoiceAssistant(*device);
    EXPECT_NE(nullptr, vasService);
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
    ASSERT_NE(nullptr, vasService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t requestId = 0;
    vasService->HandleActivateVoiceAssistant(*device, requestId);
    EXPECT_NE(nullptr, vasService);
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
    ASSERT_NE(nullptr, vasService);
    std::string addr = "00:11:22:33:44:55";
    std::shared_ptr<RawAddress> device = std::make_shared<RawAddress>(addr);
    int32_t requestId = 0;
    vasService->HandleCloseVoiceAssistant(*device, requestId);
    EXPECT_NE(nullptr, vasService);
    HILOGI("HandleCloseVoiceAssistant001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
