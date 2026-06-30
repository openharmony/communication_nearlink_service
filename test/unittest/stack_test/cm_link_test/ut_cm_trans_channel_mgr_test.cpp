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

#include "cm_trans_channel_mgr.h"
#include "cm_errno.h"
#include "securec.h"
#include "sle_logic_link_mgr.h"

class UT_CM_TRANS_CHANNEL_MGR_TEST : public testing::Test {
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

static void TransChannelTestFunc(CM_TransChannelStateList_S *state)
{
    return;
}

TEST_F(UT_CM_TRANS_CHANNEL_MGR_TEST, CM_FixedTransChannelChangeCbk)
{
    SleLogicLinkInit();
    uint32_t ret = CM_RegTransChannelCbk(TransChannelTestFunc);
    EXPECT_EQ(ret, CM_SUCCESS);
    SLE_Addr_S *addr = (SLE_Addr_S *)malloc(sizeof(SLE_Addr_S));
    memset_s(addr, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    SleLogicLink_S *node = SleLogicLinkAdd(addr);
    SleTransLcidParam_S *params = (SleTransLcidParam_S *)malloc(sizeof(SleTransLcidParam_S));
    memset_s(params, sizeof(SleTransLcidParam_S), 0x00, sizeof(SleTransLcidParam_S));
    EXPECT_EQ(CM_ActivateFixedTransChannel(CM_INVALID_LCID, params), CM_SUCCESS);
    CM_UnRegTransChannelCbk();
    SleLogicLinkRemove(node);
    free(params);
    free(addr);
}