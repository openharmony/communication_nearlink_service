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
 
#include "SleKiaManager.h"
#include "log.h"
#include "syspara/parameters.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
constexpr const char SYS_PARAM_SERVICE_FORCE_ENABLE[] = "const.pc_security.fileguard_force_enable";
class SleKiaManagerTest : public testing::Test {
public:
    SleKiaManagerTest() {}
    ~SleKiaManagerTest() {}
 
    static void SetUpTestSuite(void);
    static void TearDownTestSuite(void);
    void SetUp(void);
    void TearDown(void);
};

void SleKiaManagerTest::SetUpTestSuite(void)
{
    system::SetParameter(SYS_PARAM_SERVICE_FORCE_ENABLE, "true");
}

void SleKiaManagerTest::TearDownTestSuite(void)
{
}

void SleKiaManagerTest::SetUp(void) {}

void SleKiaManagerTest::TearDown(void) {}

static int64_t GetBootTime()
{
    constexpr int64_t msPerSecond = 1000;
    constexpr int64_t nsPerMs = 1000000;
    struct timespec times;
    if (clock_gettime(CLOCK_MONOTONIC, &times) < 0) {
        HILOGE("Failed clock_gettime:%{public}s, ShouldRefuseConnect:false", strerror(errno));
        return INT64_MAX;
    }
    int64_t bootTime = ((times.tv_sec * msPerSecond) + (times.tv_nsec / nsPerMs));
    HILOGI("bootTime:%{public}lu", bootTime);
    return bootTime;
}

/*
 * @tc.number:sle_kia_manager_test_001
 * @tc.name: state
 * @tc.desc:not add policy
 */
HWTEST_F(SleKiaManagerTest, sle_kia_manager_test_001, TestSize.Level0)
{
    HILOGI("sle_kia_manager_manager_test_001 enter");
    const int32_t pidA = 1111;
    const int32_t pidB = 2222;
    EXPECT_FALSE(SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_SSAP, pidA));
    EXPECT_FALSE(SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_DATA_TRANSFER, pidB));
    EXPECT_FALSE(SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_DATA_TRANSFER, pidA));
    HILOGI("sle_kia_manager_test_001 end");
}

/*
 * @tc.number:sle_kia_manager_test_002
 * @tc.name: state
 * @tc.desc:add(REFUSE_PROTOCOL_TYPE_SSAP, pidA, GetBootTime() + 10)
 */
HWTEST_F(SleKiaManagerTest, sle_kia_manager_test_002, TestSize.Level0)
{
    HILOGI("sle_kia_manager_test_002 enter");
    const int32_t pidA = 1111;
    const int32_t pidB = 2222;
    const int64_t prohibitedSecondsTime = 1000;
    EXPECT_TRUE(SleKiaManager::GetInstance().UpdateRefusePolicy(REFUSE_PROTOCOL_TYPE_SSAP,
        pidA, prohibitedSecondsTime) == 0);
    EXPECT_TRUE(SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_SSAP, pidA));
    EXPECT_FALSE(SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_SSAP, pidB));
    sleep(10);
    EXPECT_FALSE(SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_SSAP, pidA));
    HILOGI("sle_kia_manager_test_002 end");
}

/*
 * @tc.number:sle_kia_manager_test_003
 * @tc.name: state
 * @tc.desc:add(REFUSE_PROTOCOL_TYPE_SSAP, pidA, 10)
 * add(REFUSE_PROTOCOL_TYPE_DATA_TRANSFER, pidB, 10)
 */
HWTEST_F(SleKiaManagerTest, sle_kia_manager_test_003, TestSize.Level0)
{
    HILOGI("sle_kia_manager_test_003 enter");
    const int32_t pidA = 3333;
    const int32_t pidB = 4444;
    const int64_t prohibitedSecondsTimeA= 4999; // 5s
    EXPECT_TRUE(SleKiaManager::GetInstance().UpdateRefusePolicy(REFUSE_PROTOCOL_TYPE_SSAP,
        pidA, prohibitedSecondsTimeA) == 0);
    const int64_t prohibitedSecondsTimeB= 9999; // 10s
    EXPECT_TRUE(SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_SSAP, pidA));
    EXPECT_TRUE(SleKiaManager::GetInstance().UpdateRefusePolicy(REFUSE_PROTOCOL_TYPE_DATA_TRANSFER,
        pidB, prohibitedSecondsTimeB) == 0);
    EXPECT_TRUE(SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_DATA_TRANSFER, pidB));
    sleep(5);
    EXPECT_FALSE(SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_SSAP, pidA));
    EXPECT_TRUE(SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_DATA_TRANSFER, pidB));
    sleep(5);
    EXPECT_FALSE(SleKiaManager::GetInstance().ShouldRefuseConnect(REFUSE_PROTOCOL_TYPE_DATA_TRANSFER, pidB));
    HILOGI("sle_kia_manager_test_003 end");
}


}  // namespace bluetooth
}  // namespace OHOS