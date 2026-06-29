/*
 * Copyright (C) 2025-2025 Huawei Device Co., Ltd.
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

#include "cm_errno.h"
#include "cm_signaling_internal.h"
#include "sle_logic_link_mgr.h"
#include "securec.h"

class UT_SIGNALING_INTERNAL : public testing::Test {
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {}

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {}

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {}

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {}
};

TEST_F(UT_SIGNALING_INTERNAL, CM_GetLogicLinkCapInfo_1)
{
    CM_CapInfo_S *capInfo = (CM_CapInfo_S *)malloc(sizeof(CM_CapInfo_S));
    SLE_Addr_S *addr = (SLE_Addr_S *)malloc(sizeof(SLE_Addr_S));
    memset_s(capInfo, sizeof(CM_CapInfo_S), 0x00, sizeof(CM_CapInfo_S));
    memset_s(addr, sizeof(SLE_Addr_S), 0x01, sizeof(SLE_Addr_S));
    uint32_t ret = CM_GetLogicLinkCapInfo(nullptr, nullptr);
    EXPECT_EQ(ret, CM_INVALID_PARAM_ERR);
    ret = CM_GetLogicLinkCapInfo(capInfo, addr);
    EXPECT_EQ(ret, CM_NOT_FOUND);
    free(capInfo);
    free(addr);
}

TEST_F(UT_SIGNALING_INTERNAL, CM_GetLogicLinkCapInfo_2)
{
    SLE_Addr_S addr = {0};
    SleLogicLink_S *link = SleLogicLinkAdd(&addr);
    EXPECT_NE(&addr, nullptr);
    EXPECT_NE(link, nullptr);

    CM_CapInfo_S *capInfo = (CM_CapInfo_S *)malloc(sizeof(CM_CapInfo_S));
    memset_s(capInfo, sizeof(CM_CapInfo_S), 0x00, sizeof(CM_CapInfo_S));
    uint32_t ret = CM_GetLogicLinkCapInfo(capInfo, &addr);
    EXPECT_EQ(ret, CM_SUCCESS);
    free(capInfo);

    SleLogicLinkRemove(link);
}