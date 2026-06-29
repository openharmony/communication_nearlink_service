/*
 * Copyright (C) 2024 Huawei Device Co., Ltd. All rights reserved.
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

#include <vector>
#include "nearlink_dft_utils.h"
#include "nearlink_dft_manager_c.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Nearlink;

HWTEST(NearlinkDftManagerTest, StartTest_001, TestSize.Level1)
{
    DftManagerStart();
    DftManagerStop();
}

HWTEST(NearlinkDftManagerTest, StopTest_001, TestSize.Level1)
{
    DftManagerStart();
    DftManagerStop();
}

HWTEST(NearlinkDftManagerTest, CacheTest_001, TestSize.Level1)
{
    DftManagerStart();
    std::string app = "com.hxxwxx.app";
    std::string time = "10:36:32 145";
    std::vector<DftParamC> p1;
    p1.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    p1.emplace_back(CreateStrParamC(SWITCH_START_TIME, time));
    DftManagerCache(DFT_SWITCH_EXCEP, p1.data(), p1.size());
    DftManagerStop();
}

HWTEST(NearlinkDftManagerTest, ReportTest_001, TestSize.Level1)
{
    DftManagerStart();
    std::string app = "com.hxxwxx.app";
    std::string time = "10:36:32 145";
    std::vector<DftParamC> p1;
    p1.emplace_back(CreateStrParamC(SWITCH_USED_APP, app));
    p1.emplace_back(CreateStrParamC(SWITCH_START_TIME, time));
    DftManagerCache(DFT_SWITCH_EXCEP, p1.data(), p1.size());

    std::vector<DftParamC> p2;
    p2.emplace_back(CreateStrParamC(SWITCH_END_TIME, time));
    DftManagerCache(DFT_SWITCH_EXCEP, p2.data(), p2.size());

    std::string state = "1->2->3->4";
    std::vector<DftParamC> p3;
    p3.emplace_back(CreateStrParamC(STATE_FLOW, state));
    DftManagerCache(DFT_FLOW_STATE, p3.data(), p3.size());

    DftSubEventRefC ref1 = {DFT_FLOW_STATE, nullptr, 0};

    std::vector<DftParamC> p4;
    p4.emplace_back(CreateUi16ParamC(SWITCH_ERR_CODE, 1));
    p4.emplace_back(CreateRefParamC(SUB_STATE_FLOW, ref1));
    DftManagerReport(DFT_SWITCH_EXCEP, p4.data(), p4.size());

    sleep(3);
    DftManagerStop();
}