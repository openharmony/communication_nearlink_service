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
#include "bnl_reg_ext_func.h"
#include "collab_reg_ext_func.h"
#include "common_reg_ext_func.h"
#include "devd_reg_ext_func.h"
#include "dli_reg_ext_func.h"
#include "hadm_reg_ext_func.h"
#include "qosm_reg_ext_func.h"

namespace {
uint32_t ProbeDli(DLI_FreqBandExtParam *param)
{
    (void)param;
    return 0;
}

uint32_t ProbeCollab(const COLLAB_StmDevdScanCbk_S *cbk, const COLLAB_DevdCollabStmFunc_S *func)
{
    (void)cbk;
    (void)func;
    return 0;
}

void ProbeBnl(const NLSTK_BnlLogicLinkState_S *state)
{
    (void)state;
}

bool ProbeCommon(void)
{
    return true;
}

uint32_t ProbeDevd(const NLSTK_DevdCollabScanFilterFunc_S *filterFunc)
{
    (void)filterFunc;
    return 0;
}

bool ProbeHadm(uint16_t companyid, uint16_t subversion, uint16_t lcid)
{
    (void)companyid;
    (void)subversion;
    (void)lcid;
    return true;
}

void ProbeQosm(void)
{
}
} // namespace

class UT_ADAPTER_REG_EXT_FUNC_TEST : public testing::Test {
protected:
    void SetUp() override
    {
        DLI_DeregisterExtFunc();
        COLLAB_DeregisterExtFunc();
        BNL_DeregisterExtFunc();
        COMMON_DeregisterExtFunc();
        Devd_DeregisterExtFunc();
        HADM_DeregisterExtFunc();
        QOSM_DeregisterExtFunc();
    }

    void TearDown() override {}
};

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, DLI_RegisterExtFunc_NullHandleKeepsTableUntouched)
{
    // 空动态库句柄注册直接返回，既不清空已有表项也不写入任何新表项
    DLI_ExtFuncList *list = DLI_GetExtFuncList();
    ASSERT_NE(list, nullptr);
    list->enableFreqBandExt = ProbeDli;
    DLI_RegisterExtFunc(nullptr);
    EXPECT_EQ(list->enableFreqBandExt, ProbeDli);
    EXPECT_EQ(list->isSupportAccessFilterListExt, nullptr);
    EXPECT_EQ(list->setImgAcbInfo, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, DLI_DeregisterExtFunc_ClearsAllMembers)
{
    // 注销后函数表全成员清空
    DLI_ExtFuncList *list = DLI_GetExtFuncList();
    list->enableFreqBandExt = ProbeDli;
    DLI_DeregisterExtFunc();
    EXPECT_EQ(list->enableFreqBandExt, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, COLLAB_RegisterExtFunc_NullHandleKeepsTableUntouched)
{
    // 空动态库句柄注册直接返回，不触碰已有表项
    COLLAB_ExtFuncList *list = COLLAB_GetExtFuncList();
    ASSERT_NE(list, nullptr);
    list->stmDevdScanInit = ProbeCollab;
    COLLAB_RegisterExtFunc(nullptr);
    EXPECT_EQ(list->stmDevdScanInit, ProbeCollab);
    EXPECT_EQ(list->transFuncRegister, nullptr);
    EXPECT_EQ(list->advInit, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, COLLAB_DeregisterExtFunc_ClearsAllMembers)
{
    // 注销后函数表全成员清空
    COLLAB_ExtFuncList *list = COLLAB_GetExtFuncList();
    list->stmDevdScanInit = ProbeCollab;
    COLLAB_DeregisterExtFunc();
    EXPECT_EQ(list->stmDevdScanInit, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, BNL_RegisterExtFunc_NullHandleKeepsTableUntouched)
{
    // 空动态库句柄注册直接返回，不触碰已有表项
    BNL_ExtFuncList *list = BNL_GetExtFuncList();
    ASSERT_NE(list, nullptr);
    list->linkStateChange = ProbeBnl;
    BNL_RegisterExtFunc(nullptr);
    EXPECT_EQ(list->linkStateChange, ProbeBnl);
    EXPECT_EQ(list->init, nullptr);
    EXPECT_EQ(list->deInit, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, BNL_DeregisterExtFunc_ClearsAllMembers)
{
    // 注销后函数表全成员清空
    BNL_ExtFuncList *list = BNL_GetExtFuncList();
    list->linkStateChange = ProbeBnl;
    BNL_DeregisterExtFunc();
    EXPECT_EQ(list->linkStateChange, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, COMMON_RegisterExtFunc_NullHandleKeepsTableUntouched)
{
    // 空动态库句柄注册直接返回，不触碰已有表项
    COMMON_ExtFuncList *list = COMMON_GetExtFuncList();
    ASSERT_NE(list, nullptr);
    list->isSupportConnHighSpeed = ProbeCommon;
    COMMON_RegisterExtFunc(nullptr);
    EXPECT_EQ(list->isSupportConnHighSpeed, ProbeCommon);
    EXPECT_EQ(list->isSupportScanFilter, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, COMMON_DeregisterExtFunc_ClearsAllMembers)
{
    // 注销后函数表全成员清空
    COMMON_ExtFuncList *list = COMMON_GetExtFuncList();
    list->isSupportConnHighSpeed = ProbeCommon;
    COMMON_DeregisterExtFunc();
    EXPECT_EQ(list->isSupportConnHighSpeed, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, Devd_RegisterExtFunc_NullHandleKeepsTableUntouched)
{
    // 空动态库句柄注册直接返回，不触碰已有表项
    Devd_ExtFuncList *list = Devd_GetExtFuncList();
    ASSERT_NE(list, nullptr);
    list->devdScanFilterInit = ProbeDevd;
    Devd_RegisterExtFunc(nullptr);
    EXPECT_EQ(list->devdScanFilterInit, ProbeDevd);
    EXPECT_EQ(list->devdAddChipFilters, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, Devd_DeregisterExtFunc_ClearsAllMembers)
{
    // 注销后函数表全成员清空
    Devd_ExtFuncList *list = Devd_GetExtFuncList();
    list->devdScanFilterInit = ProbeDevd;
    Devd_DeregisterExtFunc();
    EXPECT_EQ(list->devdScanFilterInit, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, HADM_RegisterExtFunc_NullHandleKeepsTableUntouched)
{
    // 空动态库句柄注册直接返回，不触碰已有表项
    HADM_ExtFuncList *list = HADM_GetExtFuncList();
    ASSERT_NE(list, nullptr);
    list->readRemoteExtFeatures = ProbeHadm;
    HADM_RegisterExtFunc(nullptr);
    EXPECT_EQ(list->readRemoteExtFeatures, ProbeHadm);
    EXPECT_EQ(list->clearRemoteCsCaps, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, HADM_DeregisterExtFunc_ClearsAllMembers)
{
    // 注销后函数表全成员清空
    HADM_ExtFuncList *list = HADM_GetExtFuncList();
    list->readRemoteExtFeatures = ProbeHadm;
    HADM_DeregisterExtFunc();
    EXPECT_EQ(list->readRemoteExtFeatures, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, QOSM_RegisterExtFunc_NullHandleKeepsTableUntouched)
{
    // 空动态库句柄注册直接返回，不触碰已有表项
    QOSM_ExtFuncList *list = QOSM_GetExtFuncList();
    ASSERT_NE(list, nullptr);
    list->antennaDfxSendQueryCmd = ProbeQosm;
    QOSM_RegisterExtFunc(nullptr);
    EXPECT_EQ(list->antennaDfxSendQueryCmd, ProbeQosm);
    EXPECT_EQ(list->isNeedRecoverFreqBandAbility, nullptr);
}

TEST_F(UT_ADAPTER_REG_EXT_FUNC_TEST, QOSM_DeregisterExtFunc_ClearsAllMembers)
{
    // 注销后函数表全成员清空
    QOSM_ExtFuncList *list = QOSM_GetExtFuncList();
    list->antennaDfxSendQueryCmd = ProbeQosm;
    QOSM_DeregisterExtFunc();
    EXPECT_EQ(list->antennaDfxSendQueryCmd, nullptr);
}
