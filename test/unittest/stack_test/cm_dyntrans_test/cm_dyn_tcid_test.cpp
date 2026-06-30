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
#include "securec.h"
#include "cm_api.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "cm_dyn_tcid.h"
#include "cm_trans_channel_api.h"

#define UT_CM_LCID_A 0x2041
#define UT_CM_LCID_B 0x2042
#define UT_CM_LCID_C 0x2043
#define UT_CM_SRC_TCID ((CM_TCID_UC_BEGIN))
#define UT_CM_DST_TCID ((CM_TCID_UC_BEGIN) + 10)
#define UT_CM_DYN_TCID_MAX_SIZE 200
#define CM_DYN_UNICAST_TCID_SIZE_MAX ((CM_TCID_UC_END) - (CM_TCID_UC_BEGIN) + 1)

class UT_CM_DYN_TCID : public testing::Test {
protected:
  protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
    }

    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
    }

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

TEST_F(UT_CM_DYN_TCID, CM_DynTcidTest)
{
    CM_DynTcidInit();
    for (uint32_t i = 0; i < 2; i++) {
        uint32_t lcid = UT_CM_LCID_A + i;
        // 未激活，直接分配失败
        CM_DynTcidDestroyPool(lcid);
        uint8_t tcid = CM_DynTcidAllocate(lcid, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(tcid, CM_TRANS_INVALID_TCID);
        EXPECT_EQ(CM_DynTcidActivatePool(lcid), CM_SUCCESS);
        // 重复激活
        EXPECT_EQ(CM_DynTcidActivatePool(lcid), CM_SUCCESS);
        // 归还不存在的TCID
        uint32_t ret = CM_DynTcidRelease(lcid, 0, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(ret, CM_FAIL);
        // 归还不存在的LCID
        ret = CM_DynTcidRelease(0, 0, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(ret, CM_FAIL);
        // 分配第1个
        tcid = CM_DynTcidAllocate(lcid, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(tcid, CM_TCID_UC_BEGIN);
        // 归还第1个
        ret = CM_DynTcidRelease(lcid, tcid, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(ret, CM_SUCCESS);
        // 分配第1个
        tcid = CM_DynTcidAllocate(lcid, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(tcid, CM_TCID_UC_BEGIN);
        // 再分配第2个
        tcid = CM_DynTcidAllocate(lcid, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(tcid, (CM_TCID_UC_BEGIN + 1));
        // 归还第2个
        ret = CM_DynTcidRelease(lcid, tcid, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(ret, CM_SUCCESS);
        // 再分配第2个
        tcid = CM_DynTcidAllocate(lcid, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(tcid, CM_TCID_UC_BEGIN + 1);
        // 归还存在的TCID, 不存在的传输模式，归还失败
        ret = CM_DynTcidRelease(lcid, tcid, CM_ACCESS_TRANS_MODE_MAX);
        EXPECT_EQ(ret, CM_FAIL);
        // 再分配第3个
        tcid = CM_DynTcidAllocate(lcid, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(tcid, CM_TCID_UC_BEGIN + 2);
        // 归还第3个成功
        ret = CM_DynTcidRelease(lcid, tcid, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(ret, CM_SUCCESS);
        // 归还第2个成功
        ret = CM_DynTcidRelease(lcid, tcid - 1, CM_ACCESS_TRANS_MODE_UNICAST);
        // 归还第1个成功
        ret = CM_DynTcidRelease(lcid, tcid - 2, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(ret, CM_SUCCESS);
        for (uint32_t i = 0; i < CM_DYN_UNICAST_TCID_SIZE_MAX; i++) {
            tcid = CM_DynTcidAllocate(lcid, CM_ACCESS_TRANS_MODE_UNICAST);
        }
        EXPECT_EQ(tcid, CM_TCID_UC_END);
        tcid = CM_DynTcidAllocate(lcid, CM_ACCESS_TRANS_MODE_UNICAST);
        EXPECT_EQ(tcid, CM_TRANS_INVALID_TCID);
        CM_DynTcidDestroyPool(lcid);
    }
    CM_DynTcidDeInit();
}

TEST_F(UT_CM_DYN_TCID, CM_DynTcidTest_01)
{
    CM_DynTcidInit();
    for (uint32_t i = 0; i < 2; i++) {
        uint8_t tcid;
        for (uint32_t i = 0; i < CM_DYN_UNICAST_TCID_SIZE_MAX; i++) {
            uint32_t lcid = UT_CM_LCID_A + i;
            EXPECT_EQ(CM_DynTcidActivatePool(lcid), CM_SUCCESS);
            tcid = CM_DynTcidAllocate(lcid, CM_ACCESS_TRANS_MODE_UNICAST);
        }
        // 不做CM_DynTcidDestroyPool，由CM_DynTcidDeInit释放
    }
    CM_DynTcidDeInit();
}

TEST_F(UT_CM_DYN_TCID, CM_DynTcidTest_02)
{
    CM_DynTcidInit();
    for (uint32_t i = 0; i < 2; i++) {
        uint8_t tcid;
        for (uint32_t i = 0; i < CM_DYN_UNICAST_TCID_SIZE_MAX; i++) {
            uint32_t lcid = UT_CM_LCID_A + i;
            EXPECT_EQ(CM_DynTcidActivatePool(lcid), CM_SUCCESS);
            // transmode 不合法
            tcid = CM_DynTcidAllocate(lcid, CM_ACCESS_TRANS_MODE_DATA_MCST);
            EXPECT_EQ(tcid, CM_TRANS_INVALID_TCID);
            CM_DynTcidDestroyPool(lcid);
        }
    }
    CM_DynTcidDeInit();
}

TEST_F(UT_CM_DYN_TCID, CM_DynTcidTest_03)
{
    CM_DynTcidInit();
    for (uint32_t i = 0; i < 2; i++) {
        uint8_t tcid;
        for (uint32_t i = 0; i < CM_DYN_UNICAST_TCID_SIZE_MAX; i++) {
            uint32_t lcid = UT_CM_LCID_A + i;
            EXPECT_EQ(CM_DynTcidActivatePool(lcid), CM_SUCCESS);
            // transmode 不合法
            tcid = CM_DynTcidAllocate(lcid, CM_ACCESS_TRANS_MODE_MAX);
            EXPECT_EQ(tcid, CM_TRANS_INVALID_TCID);
            CM_DynTcidDestroyPool(lcid);
        }
    }
    CM_DynTcidDeInit();
}