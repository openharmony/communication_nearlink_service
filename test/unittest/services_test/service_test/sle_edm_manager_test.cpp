/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "system_ability_definition.h"
#include "parameters.h"
#include "SleEdmManager.h"
#include "log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Nearlink {
const std::string SET_EDM_SERVICE = "persist.edm.edm_enable";
static std::shared_ptr<SleEdmManager> mgr_ = SleEdmManager::GetInstance();

class SleEdmManagerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {}
    static void TearDownTestCase()
    {}
    void SetUp() override;
    void TearDown() override;
};


static void SetEdmServiceEnable()
{
    system::SetParameter(SET_EDM_SERVICE, "true");
}

void SleEdmManagerTest::SetUp()
{
    SetEdmServiceEnable();
}

void SleEdmManagerTest::TearDown()
{
}

/**
 * @tc.name: Edm_Mgr_test_IsAllowedConnect_001
 * @tc.desc: BluetoothEdmManagerTest:
 * @tc.type: FUNC
 */
HWTEST_F(SleEdmManagerTest, Edm_Mgr_test_IsAllowedConnect_001, TestSize.Level1)
{
    HILOGI("Edm_Mgr_test_IsAllowedConnect_001 enter");
    mgr_->Init();
    bool ret = mgr_->IsAllowedConnect("");
    EXPECT_TRUE(ret);
    ret = mgr_->IsAllowedConnect("SSAP");
    EXPECT_TRUE(ret);
    ret = mgr_->IsAllowedConnect("DATA_TRANSFER");
    EXPECT_TRUE(ret);
    HILOGI("Edm_Mgr_test_IsAllowedConnect_001 end");
}

/**
 * @tc.name: Edm_Mgr_test_GetForbiddenlist_001
 * @tc.desc: BluetoothEdmManagerTest:
 * @tc.type: FUNC
 */
HWTEST_F(SleEdmManagerTest, Edm_Mgr_test_OnEdmListChanged_001, TestSize.Level1)
{
    HILOGI("Edm_Mgr_test_OnEdmListChanged_001 enter");
    mgr_->OnEdmListChanged();
    EXPECT_TRUE(mgr_->accountForbiddenlist_.IsEmpty());
    HILOGI("Edm_Mgr_test_OnEdmListChanged_001 end");
}
}  // namespace bluetooth
}  // namespace OHOS