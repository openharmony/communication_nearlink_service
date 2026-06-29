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

#include <cstdint>
#include <gtest/gtest.h>
#include "collab_ext_func_wrapper.h"
#include "collab_reg_ext_func.h"
#include "nlstk_public_define.h"

using namespace testing;
using namespace testing::Ext;

namespace {
// Global mock control variables
uint32_t g_transInitRet = NLSTK_ERRCODE_SUCCESS;
uint32_t g_transInitAfterRegRet = NLSTK_ERRCODE_SUCCESS;
bool g_isFuncListNull = false;
bool g_isTransInitNull = false;
bool g_isTransInitAfterRegNull = false;
bool g_isTransDeinitNull = false;
} // namespace

extern "C" {
static COLLAB_ExtFuncList *g_mockFuncList = nullptr;

void COLLAB_RegisterExtFunc(void *soHandle)
{
    (void)soHandle;
}

void COLLAB_DeregisterExtFunc(void)
{
}

COLLAB_ExtFuncList *COLLAB_GetExtFuncList(void)
{
    if (g_isFuncListNull) {
        return nullptr;
    }
    return g_mockFuncList;
}

static uint32_t MockTransInit(COLLAB_TransExposerExt *exposer)
{
    (void)exposer;
    return g_transInitRet;
}

static uint32_t MockTransInitAfterReg(const COLLAB_TransInitAfterCbkExt cbk)
{
    (void)cbk;
    return g_transInitAfterRegRet;
}

static void MockTransDeinit(void)
{
}

void SetMockFuncList(void)
{
    static COLLAB_ExtFuncList funcList = {
        .stmDevdScanInit = NULL,
        .stmDevdScanInitAfterReg = NULL,
        .stmDevdScanDeInit = NULL,
        .transFuncRegister = g_isTransInitNull ? NULL : MockTransInit,
        .transInitAfterReg = g_isTransInitAfterRegNull ? NULL : MockTransInitAfterReg,
        .transDeinit = g_isTransDeinitNull ? NULL : MockTransDeinit,
    };
    g_mockFuncList = &funcList;
}
} // extern "C"

class UT_COLLAB_EXT_FUNC_WRAPPER_TEST : public testing::Test {
protected:
    virtual void SetUp()
    {
        g_isFuncListNull = false;
        g_isTransInitNull = false;
        g_isTransInitAfterRegNull = false;
        g_isTransDeinitNull = false;
        g_transInitRet = NLSTK_ERRCODE_SUCCESS;
        g_transInitAfterRegRet = NLSTK_ERRCODE_SUCCESS;
        SetMockFuncList();
    }

    virtual void TearDown()
    {
    }

    static void SetUpTestCase()
    {
    }

    static void TearDownTestCase()
    {
    }
};

TEST_F(UT_COLLAB_EXT_FUNC_WRAPPER_TEST, COLLAB_TransInit_Success)
{
    COLLAB_TransExposerExt exposer = {0};
    uint32_t ret = COLLAB_TransFuncRegister(&exposer);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
}

TEST_F(UT_COLLAB_EXT_FUNC_WRAPPER_TEST, COLLAB_TransInit_Fail)
{
    g_transInitRet = NLSTK_ERRCODE_FAIL;
    COLLAB_TransExposerExt exposer = {0};
    uint32_t ret = COLLAB_TransFuncRegister(&exposer);
    EXPECT_EQ(ret, NLSTK_ERRCODE_FAIL);
}

TEST_F(UT_COLLAB_EXT_FUNC_WRAPPER_TEST, COLLAB_TransInit_NullFuncList)
{
    g_isFuncListNull = true;
    COLLAB_TransExposerExt exposer = {0};
    uint32_t ret = COLLAB_TransFuncRegister(&exposer);
    EXPECT_EQ(ret, NLSTK_ERRCODE_FAIL);
}

TEST_F(UT_COLLAB_EXT_FUNC_WRAPPER_TEST, COLLAB_TransInit_NullTransInit)
{
    g_isTransInitNull = true;
    COLLAB_TransExposerExt exposer = {0};
    uint32_t ret = COLLAB_TransFuncRegister(&exposer);
    EXPECT_EQ(ret, NLSTK_ERRCODE_FAIL);
}

TEST_F(UT_COLLAB_EXT_FUNC_WRAPPER_TEST, COLLAB_TransInitAfterReg_Success)
{
    uint32_t ret = COLLAB_TransInitAfterReg(NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
}

TEST_F(UT_COLLAB_EXT_FUNC_WRAPPER_TEST, COLLAB_TransInitAfterReg_Fail)
{
    g_transInitAfterRegRet = NLSTK_ERRCODE_FAIL;
    uint32_t ret = COLLAB_TransInitAfterReg(NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_FAIL);
}

TEST_F(UT_COLLAB_EXT_FUNC_WRAPPER_TEST, COLLAB_TransInitAfterReg_NullFuncList)
{
    g_isFuncListNull = true;
    uint32_t ret = COLLAB_TransInitAfterReg(NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_FAIL);
}

TEST_F(UT_COLLAB_EXT_FUNC_WRAPPER_TEST, COLLAB_TransInitAfterReg_NullTransInitAfterReg)
{
    g_isTransInitAfterRegNull = true;
    uint32_t ret = COLLAB_TransInitAfterReg(NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_FAIL);
}

TEST_F(UT_COLLAB_EXT_FUNC_WRAPPER_TEST, COLLAB_TransDeInit_Success)
{
    COLLAB_TransDeInit();
}

TEST_F(UT_COLLAB_EXT_FUNC_WRAPPER_TEST, COLLAB_TransDeInit_NullFuncList)
{
    g_isFuncListNull = true;
    COLLAB_TransDeInit();
}

TEST_F(UT_COLLAB_EXT_FUNC_WRAPPER_TEST, COLLAB_TransDeInit_NullTransDeinit)
{
    g_isTransDeinitNull = true;
    COLLAB_TransDeInit();
}