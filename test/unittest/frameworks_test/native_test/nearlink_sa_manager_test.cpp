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
#include "nearlink_def.h"
#include "nearlink_host.h"
#include "nearlink_sa_manager.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

namespace {
const char *NEARLINK_HOST = "NearlinkHost";
}

class NearlinkSaManagerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkSaManagerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start");
    NearlinkAccessTokenMock::SetNativeTokenInfo();
}

void NearlinkSaManagerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start");
}

void NearlinkSaManagerTest::SetUp()
{
    HILOGI("SetUp start");
}

void NearlinkSaManagerTest::TearDown()
{
    HILOGI("TearDown start");
}

/**
 * @tc.number: GetRemoteProfile001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkSaManagerTest, GetRemoteProfile001, TestSize.Level1)
{
    HILOGI("GetRemoteProfile001 start");
    sptr<IRemoteObject> remote = NearlinkSaManager::GetInstance().GetRemoteProfile(NEARLINK_HOST);
    EXPECT_NE(nullptr, remote); // nearlink service resident after enable.
    HILOGI("GetRemoteProfile001 end");
}

/**
 * @tc.number: RegisterFunc001
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkSaManagerTest, RegisterFunc001, TestSize.Level1)
{
    HILOGI("RegisterFunc001 start");
    // DisableNl
    NearlinkHost::GetInstance().DisableNl();
    sleep(16); // 16s

    static bool g_serviceStarted = false;
    static bool g_sleHalfDisable = false;

    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(NEARLINK_HOST);
    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        g_serviceStarted = true;
    };
    info->serviceStoppedFunc_ = [this]() -> void {
        g_serviceStarted = false;
    };
    // nearlink is off
    int32_t profileRegisterId = NearlinkSaManager::GetInstance().RegisterFunc(info);
    g_sleHalfDisable = NearlinkHost::GetInstance().IsSleHalfDisabled();
    EXPECT_EQ(3, profileRegisterId);
    if (g_sleHalfDisable) {
        EXPECT_EQ(true, g_serviceStarted);
    } else {
        EXPECT_EQ(false, g_serviceStarted);
    }

    NearlinkHost::GetInstance().EnableNl();
    sleep(3); // 3s
    EXPECT_EQ(true, g_serviceStarted);

    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId);
    HILOGI("RegisterFunc001 end");
}

/**
 * @tc.number: RegisterFunc002
 * @tc.name:
 * @tc.desc:
 */
HWTEST_F(NearlinkSaManagerTest, RegisterFunc002, TestSize.Level1)
{
    HILOGI("RegisterFunc002 start");
    NearlinkHost::GetInstance().EnableNl();
    sleep(3); // 3s

    static bool g_serviceStarted = false;

    std::shared_ptr<NearlinkRegisterInfo> info = std::make_shared<NearlinkRegisterInfo>(NEARLINK_HOST);
    info->serviceStartedFunc_ = [this](sptr<IRemoteObject> remote) -> void {
        g_serviceStarted = true;
    };
    info->serviceStoppedFunc_ = [this]() -> void {
        g_serviceStarted = false;
    };

    // nearlink is on
    int32_t profileRegisterId = NearlinkSaManager::GetInstance().RegisterFunc(info);
    EXPECT_EQ(3, profileRegisterId);
    EXPECT_EQ(true, g_serviceStarted);

    NearlinkSaManager::GetInstance().DeregisterFunc(profileRegisterId);
    HILOGI("RegisterFunc002 end");
}
} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
