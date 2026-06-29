/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "nearlink_sle_controller.h"
#include "nearlink_def.h"
#include "log.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;

class NearlinkSleControllerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void NearlinkSleControllerTest::SetUpTestCase()
{
    HILOGI("SetUpTestCase start NearlinkSleControllerTest.");
}

void NearlinkSleControllerTest::TearDownTestCase()
{
    HILOGI("TearDownTestCase start NearlinkSleControllerTest.");
}

void NearlinkSleControllerTest::SetUp()
{
    HILOGI("SetUp start NearlinkSleControllerTest.");
}

void NearlinkSleControllerTest::TearDown()
{
    HILOGI("TearDown start NearlinkSleControllerTest.");
}

/**
 * @tc.name: SetSleCoexParam001
 * @tc.type: FUNC
 * @tc.desc: 测试 SetSleCoexParam 权限拦截
 */
HWTEST_F(NearlinkSleControllerTest, SetSleCoexParam001, TestSize.Level1)
{
    HILOGI("NearlinkSleControllerTest:SetSleCoexParam001 start");
    SLEBitRate maxBitRate = SLEBitRate::BITRATE_32;
    SledutyCycle dutyCycle = SledutyCycle::DUTY_CYCLE_100P;
    NlErrCode status = NearlinkSleController::GetInstance().SetSleCoexParam(maxBitRate, dutyCycle);
    ASSERT_TRUE(status == NL_ERR_INTERNAL_ERROR);  // 校验 calling name 拦截
    HILOGI("NearlinkSleControllerTest:SetSleCoexParam001 end");
}

/**
 * @tc.name: UpdateConnectInterval001
 * @tc.desc: 测试正常参数下的 UpdateConnectInterval 函数
 * @tc.type: 功能测试
 */
HWTEST_F(NearlinkSleControllerTest, UpdateConnectInterval001, TestSize.Level1)
{
    HILOGI("NearlinkSleControllerTest:UpdateConnectInterval001 start");
    char addr[] = "00:00:00:00:00:00";
    std::string deviceId(addr);
    ConnectionInterval intervalType = HIGH_SPEED_INTERVAL_4_5;
    NlErrCode status = NearlinkSleController::GetInstance().UpdateConnectInterval(deviceId, intervalType);
    ASSERT_TRUE(status == NL_ERR_INTERNAL_ERROR);  // 校验 calling name 拦截
    HILOGI("NearlinkSleControllerTest:UpdateConnectInterval001 end");
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS
