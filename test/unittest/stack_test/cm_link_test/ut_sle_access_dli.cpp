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
#include "cm_event_core.h"
#include "cm_concurrent_conn.h"
#include "dli_event.h"
#include "dli_errno.h"
#include "sle_logic_link_mgr.h"
#include "sle_access_dli.h"
#include "cp_worker.h"
#include "securec.h"

static uint32_t CP_PostTaskStub(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    cb(arg);
    if (freeCb != NULL) {
        freeCb(arg);
    }
    return CP_OK;
}

class UT_SLE_ACCESS_DLI : public testing::Test {
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
        EXPECT_EQ(CM_Init(), CM_SUCCESS);
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        CM_DeInit();
    }
};

TEST_F(UT_SLE_ACCESS_DLI, SleAccessReadRemoteFeatures)
{
    uint16_t req = 0x00;
    EXPECT_EQ(SleAccessReadRemoteFeatures(req), DLI_SUCCESS);
}

TEST_F(UT_SLE_ACCESS_DLI, SleAccessReadRemoteVersion)
{
    uint16_t req = 0x00;
    EXPECT_EQ(SleAccessReadRemoteVersion(req), DLI_SUCCESS);
}

TEST_F(UT_SLE_ACCESS_DLI, SleAccessSetPhy)
{
    DLI_SetPhyParam *param = (DLI_SetPhyParam *)malloc(sizeof(DLI_SetPhyParam));
    EXPECT_NE(param, nullptr);
    memset_s(param, sizeof(DLI_SetPhyParam), 0x00, sizeof(DLI_SetPhyParam));
    SleAccessSetPhy(param);
    SDF_MemFree(param);
}