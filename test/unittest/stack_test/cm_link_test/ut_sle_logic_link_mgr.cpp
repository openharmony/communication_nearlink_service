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

#include "gtest/gtest.h"

#include "sle_logic_link_mgr.h"
#include "cm_log.h"
#include "securec.h"
#include "sdf_mem.h"

class UT_SLE_LOGIC_LINK_MGR : public testing::Test {
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {}

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {}

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {
        CM_LOGI("SetUpTestCase");
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        CM_LOGI("TearDownTestCase stub reset");
        // Don't call STUB_Reset
        CM_LOGI("TearDownTestCase");
    }
};

TEST_F(UT_SLE_LOGIC_LINK_MGR, logic_link_test)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)malloc(sizeof(SLE_Addr_S));
    EXPECT_NE(addr, nullptr);
    memset_s(addr, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    SleLogicLinkInit();
    SleLogicLink_S *link = SleLogicLinkAdd(addr);
    SleLogicLinkRemove(link);

    link = SleLogicLinkAdd(addr);
    EXPECT_NE(link, nullptr);
    link = SleLogicLinkGetByLcid(CM_INVALID_LCID);
    EXPECT_NE(link, nullptr);
    link = SleLogicLinkGetByStatus(0x00);
    EXPECT_NE(link, nullptr);
    SDF_MemFree(addr);
    SleLogicLinkDeInit();
}