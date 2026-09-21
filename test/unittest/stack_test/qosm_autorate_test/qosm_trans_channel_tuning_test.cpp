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
#include "securec.h"

#include "cm_api.h"
#include "cm_def.h"
#include "cp_worker.h"
#include "cm_dyn_trans_channel_api.h"
#include "cm_errno.h"
#include "qosm_errno.h"
#include "qosm_trans_channel_tuning.h"
#include "sdf_map.h"
#include "sdf_addr.h"
#include "sdf_dlist.h"
#include "sdf_traits.h"
#include "sdf_timer.h"
#include "sle_logic_link_mgr.h"

using namespace testing;

#ifdef __cplusplus
extern "C" {
#endif

#define QOSM_TUNING_TEST_LCID 2
#define QOSM_TUNING_TEST_TCID 5

static QOSM_LogicLink_S g_tuningLogicLink = {};
static QOSM_TransChannelRspParams_S g_tuningStatusParams = {};
static uint32_t g_tuningDecreaseChannelSizeCount = 0;
static bool g_tuningLogicLinkFindable = true;
static uint8_t g_execConnUpdateCallbackStep = 0;
static uint8_t g_execConnUpdateCallbackDestStep = 0;
static SDF_TimerParam g_savedConnUpdateTimerParam = {};

// slqi = 1(或者0)三个不同连接参数变更下发步骤
typedef enum {
    TEST_TIMEOUT_STEP_UPDATE_PARAM = 1,
    TEST_TIMEOUT_STEP_SET_PHY = 2,
    TEST_TIMEOUT_STEP_SET_MCS = 3,
} TEST_TIMEOUT_STEP_E;

static void TestTuningStatusCbk(const QOSM_TransChannelRspParams_S *respParams)
{
    if (respParams != NULL) {
        (void)memcpy_s(&g_tuningStatusParams, sizeof(QOSM_TransChannelRspParams_S), respParams,
            sizeof(QOSM_TransChannelRspParams_S));
    }
}

static QOSM_LogicLink_S *TestTuningLogicLinkFind(uint16_t lcid)
{
    (void)lcid;
    return g_tuningLogicLinkFindable ? &g_tuningLogicLink : NULL;
}

static void TestTuningDecreaseChannelSize(uint16_t lcid)
{
    (void)lcid;
    g_tuningDecreaseChannelSizeCount++;
}

static void RegisterTuningCbks(void)
{
    QOSM_TcTuningCbks_S cbk = {0};
    cbk.statusCbk = TestTuningStatusCbk;
    cbk.logicLinkFindCbk = TestTuningLogicLinkFind;
    cbk.decreaseChannelSizeCbk = TestTuningDecreaseChannelSize;
    QOSM_TcTuningCbksRegister(&cbk);
}

static void ResetTuningEnv(void)
{
    (void)memset_s(&g_tuningLogicLink, sizeof(QOSM_LogicLink_S), 0, sizeof(QOSM_LogicLink_S));
    (void)memset_s(&g_tuningStatusParams, sizeof(QOSM_TransChannelRspParams_S), 0,
        sizeof(QOSM_TransChannelRspParams_S));
    g_tuningDecreaseChannelSizeCount = 0;
    g_tuningLogicLinkFindable = true;
    g_execConnUpdateCallbackStep = 0;
    g_execConnUpdateCallbackDestStep = 0;
    QOSM_TcTuningCbksUnregister();
}

static QOSM_TcTuningCtx_S CreateTuningCtx(uint8_t slqi)
{
    QOSM_TcTuningCtx_S ctx = {};
    ctx.rspParams.lcid = QOSM_TUNING_TEST_LCID;
    ctx.rspParams.tcid = QOSM_TRANS_CHANNEL_SLQI_LOW;
    ctx.rspParams.slqi = (QOSM_TransChannelSlqi_E)slqi;
    return ctx;
}

static void InitTuningLogicLink(void)
{
    g_tuningLogicLink.lcid = QOSM_TUNING_TEST_LCID;
    SLE_Addr_S addr = {0, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    g_tuningLogicLink.addr = addr;
    g_tuningLogicLink.slqi = QOSM_TRANS_CHANNEL_SLQI_LOW;
}

static void DriveTuningToComplete(uint16_t lcid, uint8_t testTimeoutStep = 0)
{
    if (testTimeoutStep == TEST_TIMEOUT_STEP_UPDATE_PARAM) {
        if (g_savedConnUpdateTimerParam.callback != NULL) {
            g_savedConnUpdateTimerParam.callback(g_savedConnUpdateTimerParam.args);
        }
        return;
    } else {
        CM_LogicLinkConnUpdateParam_S updParam = {};
        updParam.lcid = lcid;
        updParam.result = CM_SUCCESS;
        QOSM_TcTuningLogicLinkConnUpdateParamCbk(&updParam);
    }

    if (testTimeoutStep == TEST_TIMEOUT_STEP_SET_PHY) {
        if (g_savedConnUpdateTimerParam.callback != NULL) {
            g_savedConnUpdateTimerParam.callback(g_savedConnUpdateTimerParam.args);
        }
        return;
    } else {
        CM_LogicLinkSetPhy_S phyParam = {};
        phyParam.lcid = lcid;
        phyParam.status = CM_SUCCESS;
        QOSM_TcTuningLogicLinkSetPhyCbk(&phyParam);
    }

    if (testTimeoutStep == TEST_TIMEOUT_STEP_SET_MCS) {
        if (g_savedConnUpdateTimerParam.callback != NULL) {
            g_savedConnUpdateTimerParam.callback(g_savedConnUpdateTimerParam.args);
        }
        return;
    } else {
        CM_LogicLinkSetMcs_S mcsParam = {};
        mcsParam.status = CM_SUCCESS;
        mcsParam.lcid = lcid;
        QOSM_TcTuningLogicLinkSetMcsCbk(&mcsParam);
    }
}

#ifdef __cplusplus
}
#endif

class UT_QOSM_TRANS_CHANNEL_TUNING_TEST : public testing::Test {
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        ResetTuningEnv();
    }
    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
    }
};

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningCbksRegisterAndUnRegister_01)
{
    QOSM_TcTuningCbksRegister(NULL);
    QOSM_TcTuningCbksUnregister();
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningCbksRegisterInvalidArgs_02)
{
    RegisterTuningCbks();
    QOSM_TcTuningCbks_S invalidCbk = {0};
    invalidCbk.statusCbk = TestTuningStatusCbk;  // logicLinkFindCbk/decreaseChannelSizeCbk 为NULL，注册不生效
    QOSM_TcTuningCbksRegister(&invalidCbk);

    // 非法注册不覆盖原回调，原有回调仍能正常工作
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
    EXPECT_EQ(g_tuningLogicLink.slqi, QOSM_TRANS_CHANNEL_SLQI_LOW);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningStartWithStmNullCtx_03)
{
    EXPECT_EQ(QOSM_TcTuningStartWithStm(NULL), QOSM_NULL_PTR_ERR);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningStartWithStmCompleteLowSlqi_04)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid);

    EXPECT_EQ(g_tuningStatusParams.lcid, ctx.rspParams.lcid);
    EXPECT_EQ(g_tuningStatusParams.tcid, ctx.rspParams.tcid);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_LOW);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
    EXPECT_EQ(g_tuningLogicLink.slqi, QOSM_TRANS_CHANNEL_SLQI_LOW);
    EXPECT_EQ(g_tuningDecreaseChannelSizeCount, 0);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningStartWithStmCompleteHighSlqi_05_01)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_HIGH);
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid);

    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_HIGH);
    EXPECT_EQ(g_tuningLogicLink.slqi, QOSM_TRANS_CHANNEL_SLQI_HIGH);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningStartWithStmCompleteHighSlqi_05_02)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_HIGH);
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_RELEASED;

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid);

    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_RELEASED);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_HIGH);
    EXPECT_EQ(g_tuningLogicLink.slqi, QOSM_TRANS_CHANNEL_SLQI_HIGH);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningStartWithStmDuplicateStart_06)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    // 状态机已运行，再次启动应失败
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_POST_TASK_ERR);

    // 完成后状态机回收，可再次启动
    DriveTuningToComplete(ctx.rspParams.lcid);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningStartWithStmLogicLinkNotFound_07)
{
    RegisterTuningCbks();
    g_tuningLogicLinkFindable = false;
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
    // 失败时tcid有效，需释放传输通道并回调decreaseChannelSize
    EXPECT_EQ(g_tuningDecreaseChannelSizeCount, 1);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningStartWithStmSlqiOutOfRange_08)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_MAX);

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
    // tcid为无效值(0)，失败时不释放传输通道
    EXPECT_EQ(g_tuningDecreaseChannelSizeCount, 0);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningConnUpdateRspFail_09)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    CM_LogicLinkConnUpdateParam_S updParam = {};
    updParam.lcid = ctx.rspParams.lcid;
    updParam.result = (uint8_t)CM_FAIL;
    QOSM_TcTuningLogicLinkConnUpdateParamCbk(&updParam);

    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
    EXPECT_EQ(g_tuningDecreaseChannelSizeCount, 1);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningConnUpdateRspLogicLinkNotFound_10)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    // 响应阶段逻辑链路查找失败
    g_tuningLogicLinkFindable = false;
    CM_LogicLinkConnUpdateParam_S updParam = {};
    updParam.lcid = ctx.rspParams.lcid;
    updParam.result = CM_SUCCESS;
    QOSM_TcTuningLogicLinkConnUpdateParamCbk(&updParam);

    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
    EXPECT_EQ(g_tuningDecreaseChannelSizeCount, 0);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningSetPhyRspFail_11)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    CM_LogicLinkConnUpdateParam_S updParam = {};
    updParam.lcid = ctx.rspParams.lcid;
    updParam.result = CM_SUCCESS;
    QOSM_TcTuningLogicLinkConnUpdateParamCbk(&updParam);

    CM_LogicLinkSetPhy_S phyParam = {};
    phyParam.lcid = ctx.rspParams.lcid;
    phyParam.status = (uint8_t)CM_FAIL;
    QOSM_TcTuningLogicLinkSetPhyCbk(&phyParam);

    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningSetMcsRspFail_12)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    CM_LogicLinkConnUpdateParam_S updParam = {};
    updParam.lcid = ctx.rspParams.lcid;
    updParam.result = CM_SUCCESS;
    QOSM_TcTuningLogicLinkConnUpdateParamCbk(&updParam);

    CM_LogicLinkSetPhy_S phyParam = {};
    phyParam.lcid = ctx.rspParams.lcid;
    phyParam.status = CM_SUCCESS;
    QOSM_TcTuningLogicLinkSetPhyCbk(&phyParam);

    // 设置mcs失败，进入failed状态
    CM_LogicLinkSetMcs_S mcsParam = {};
    mcsParam.status = (uint8_t)CM_FAIL;
    mcsParam.lcid = ctx.rspParams.lcid;
    QOSM_TcTuningLogicLinkSetMcsCbk(&mcsParam);

    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningRspFilteredNullArg_13_01)
{
    // NULL参数直接返回
    QOSM_TcTuningLogicLinkConnUpdateParamCbk(NULL);
    QOSM_TcTuningLogicLinkSetPhyCbk(NULL);
    QOSM_TcTuningLogicLinkSetMcsCbk(NULL);

    // 空指针参数不影响后续状态机流程
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningRspFilteredNoStm_13_02)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);

    // 状态机未运行时收到响应被过滤，不触发状态回调
    CM_LogicLinkConnUpdateParam_S updParam = {};
    updParam.lcid = ctx.rspParams.lcid;
    updParam.result = CM_SUCCESS;
    QOSM_TcTuningLogicLinkConnUpdateParamCbk(&updParam);

    CM_LogicLinkSetPhy_S phyParam = {};
    phyParam.lcid = ctx.rspParams.lcid;
    phyParam.status = CM_SUCCESS;
    QOSM_TcTuningLogicLinkSetPhyCbk(&phyParam);

    CM_LogicLinkSetMcs_S mcsParam = {};
    mcsParam.status = CM_SUCCESS;
    mcsParam.lcid = ctx.rspParams.lcid;
    QOSM_TcTuningLogicLinkSetMcsCbk(&mcsParam);

    EXPECT_EQ(g_tuningStatusParams.status, 0);
    EXPECT_EQ(g_tuningDecreaseChannelSizeCount, 0);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningRspFilteredLcidMismatch_13_03)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    // 先推进到等待set phy响应状态
    CM_LogicLinkConnUpdateParam_S updParam = {};
    updParam.lcid = ctx.rspParams.lcid;
    updParam.result = CM_SUCCESS;
    QOSM_TcTuningLogicLinkConnUpdateParamCbk(&updParam);

    // lcid不匹配被过滤，状态机不推进
    CM_LogicLinkSetPhy_S phyParam = {};
    phyParam.lcid = ctx.rspParams.lcid + 1;
    phyParam.status = CM_SUCCESS;
    QOSM_TcTuningLogicLinkSetPhyCbk(&phyParam);

    // lcid匹配的响应可正常接收，流程继续推进
    phyParam.lcid = ctx.rspParams.lcid;
    QOSM_TcTuningLogicLinkSetPhyCbk(&phyParam);

    CM_LogicLinkSetMcs_S mcsParam = {};
    mcsParam.status = CM_SUCCESS;
    mcsParam.lcid = ctx.rspParams.lcid;
    QOSM_TcTuningLogicLinkSetMcsCbk(&mcsParam);

    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningRspFilteredStateMismatch_13_04)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);

    // 当前处于wait conn update状态，收到的set phy响应状态不匹配被过滤
    CM_LogicLinkSetPhy_S phyParam = {};
    phyParam.lcid = ctx.rspParams.lcid;
    phyParam.status = CM_SUCCESS;
    QOSM_TcTuningLogicLinkSetPhyCbk(&phyParam);

    // 过滤后状态机未推进，正确的流程仍可正常完成
    DriveTuningToComplete(ctx.rspParams.lcid);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningRspFilteredAfterComplete_13_05)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);

    // 状态机完成后再次收到响应被过滤，不影响已上报的完成状态
    CM_LogicLinkConnUpdateParam_S updParam = {};
    updParam.lcid = ctx.rspParams.lcid;
    updParam.result = CM_SUCCESS;
    QOSM_TcTuningLogicLinkConnUpdateParamCbk(&updParam);

    CM_LogicLinkSetPhy_S phyParam = {};
    phyParam.lcid = ctx.rspParams.lcid;
    phyParam.status = CM_SUCCESS;
    QOSM_TcTuningLogicLinkSetPhyCbk(&phyParam);

    CM_LogicLinkSetMcs_S mcsParam = {};
    mcsParam.status = CM_SUCCESS;
    mcsParam.lcid = ctx.rspParams.lcid;
    QOSM_TcTuningLogicLinkSetMcsCbk(&mcsParam);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningCompleteLogicLinkNotFound_14)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);

    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    CM_LogicLinkConnUpdateParam_S updParam = {};
    updParam.lcid = ctx.rspParams.lcid;
    updParam.result = CM_SUCCESS;
    QOSM_TcTuningLogicLinkConnUpdateParamCbk(&updParam);

    CM_LogicLinkSetPhy_S phyParam = {};
    phyParam.lcid = ctx.rspParams.lcid;
    phyParam.status = CM_SUCCESS;
    QOSM_TcTuningLogicLinkSetPhyCbk(&phyParam);

    // 完成阶段逻辑链路查找失败，不影响上报完成状态
    g_tuningLogicLinkFindable = false;
    CM_LogicLinkSetMcs_S mcsParam = {};
    mcsParam.status = CM_SUCCESS;
    mcsParam.lcid = ctx.rspParams.lcid;
    QOSM_TcTuningLogicLinkSetMcsCbk(&mcsParam);

    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_TcTuningReuseAfterReleaseChannelFail_15)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;

    // 触发失败流程，释放传输通道
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    CM_LogicLinkConnUpdateParam_S updParam = {};
    updParam.lcid = ctx.rspParams.lcid;
    updParam.result = (uint8_t)CM_FAIL;
    QOSM_TcTuningLogicLinkConnUpdateParamCbk(&updParam);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
    EXPECT_EQ(g_tuningDecreaseChannelSizeCount, 1);

    // 失败释放后状态机回收，可再次发起调参
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
    EXPECT_EQ(g_tuningStatusParams.tcid, ctx.rspParams.tcid);
}

// 流程步骤处理超时测试
#ifdef __cplusplus
extern "C" {
#endif

extern "C" uint32_t CP_TimerAdd(int *handle, SDF_TimerParam *param)
{
    *handle = 1;
    g_execConnUpdateCallbackStep++;
    if ((g_execConnUpdateCallbackStep == g_execConnUpdateCallbackDestStep) && param->callback != NULL) {
        (void)memcpy_s(&g_savedConnUpdateTimerParam, sizeof(SDF_TimerParam), param, sizeof(SDF_TimerParam));
        g_execConnUpdateCallbackStep = 0;
    }
    return 0;
}

extern "C" void CP_TimerDel(int handle)
{
    (void)handle;
    return;
}

#ifdef __cplusplus
}
#endif

static void SetConnUpdateExecCallbackAtOnce(void)
{
    g_execConnUpdateCallbackDestStep = TEST_TIMEOUT_STEP_UPDATE_PARAM; // 连接参数更新是第1个步骤
}

static void SetConnSetPhyExecCallbackAtOnce(void)
{
    g_execConnUpdateCallbackDestStep = TEST_TIMEOUT_STEP_SET_PHY; // SetPhy是第2个步骤
}

static void SetConnSetMcsExecCallbackAtOnce(void)
{
    g_execConnUpdateCallbackDestStep = TEST_TIMEOUT_STEP_SET_MCS; // SetMcs是第3个步骤
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_ConnUpdateTimeoutNullArg_16)
{
    SetConnUpdateExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(NULL), QOSM_NULL_PTR_ERR);
    SetConnUpdateExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试连接参数更新QOSM_ConnUpdateTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_ConnUpdateTimeoutTrigTimeout_01_01)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_HIGH);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;

    SetConnUpdateExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_UPDATE_PARAM);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_HIGH);
    SetConnUpdateExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试连接参数更新QOSM_ConnUpdateTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_ConnUpdateTimeoutTrigTimeout_01_02)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_HIGH);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_RELEASED;

    SetConnUpdateExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_UPDATE_PARAM);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_RELEASE_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_HIGH);
    SetConnUpdateExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试连接参数更新QOSM_ConnUpdateTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_ConnUpdateTimeoutTrigTimeout_02_01)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;

    SetConnUpdateExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_UPDATE_PARAM);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_LOW);
    SetConnUpdateExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试连接参数更新QOSM_ConnUpdateTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_ConnUpdateTimeoutTrigTimeout_02_02)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_RELEASED;

    SetConnUpdateExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_UPDATE_PARAM);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_RELEASE_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_LOW);
    SetConnUpdateExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试连接参数更新QOSM_SetPhyTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_SetPhyTimeoutTrigTimeout_01_01)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_HIGH);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;

    SetConnSetPhyExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_SET_PHY);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_HIGH);
    SetConnSetPhyExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试SetPhy超时QOSM_SetPhyTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_SetPhyTimeoutTrigTimeout_01_02)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_HIGH);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_RELEASED;

    SetConnSetPhyExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_SET_PHY);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_RELEASE_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_HIGH);
    SetConnSetPhyExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试SetPhy超时QOSM_SetPhyTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_SetPhyTimeoutTrigTimeout_02_01)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;

    SetConnSetPhyExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_SET_PHY);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_LOW);
    SetConnSetPhyExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试SetPhy超时QOSM_SetPhyTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_SetPhyTimeoutTrigTimeout_02_02)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_RELEASED;

    SetConnSetPhyExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_SET_PHY);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_RELEASE_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_LOW);
    SetConnSetPhyExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试连接参数更新QOSM_SetMcsTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_SetMcsTimeoutTrigTimeout_01_01)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_HIGH);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;

    SetConnSetMcsExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_SET_MCS);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_HIGH);
    SetConnSetMcsExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试SetMcs超时QOSM_SetMcsTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_SetMcsTimeoutTrigTimeout_01_02)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_HIGH);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_RELEASED;

    SetConnSetMcsExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_SET_MCS);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_RELEASE_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_HIGH);
    SetConnSetMcsExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试连接参数更新QOSM_SetMcsTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_SetMcsTimeoutTrigTimeout_02_01)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_ESTABLISHED;

    SetConnSetMcsExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_SET_MCS);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_LOW);
    SetConnSetMcsExecCallbackAtOnce();
}

/*
 * @brief 处理流程超时用例测试: 覆盖测试SetMcs超时QOSM_SetMcsTimeout
 */
TEST_F(UT_QOSM_TRANS_CHANNEL_TUNING_TEST, QOSM_SetMcsTimeoutTrigTimeout_02_02)
{
    RegisterTuningCbks();
    InitTuningLogicLink();
    QOSM_TcTuningCtx_S ctx = CreateTuningCtx(QOSM_TRANS_CHANNEL_SLQI_LOW);
    ctx.rspParams.tcid = QOSM_TUNING_TEST_TCID;
    ctx.rspParams.status = QOSM_TRANS_CHANNEL_RELEASED;

    SetConnSetMcsExecCallbackAtOnce();
    EXPECT_EQ(QOSM_TcTuningStartWithStm(&ctx), QOSM_SUCCESS);
    DriveTuningToComplete(ctx.rspParams.lcid, TEST_TIMEOUT_STEP_SET_MCS);
    EXPECT_EQ(g_tuningStatusParams.status, QOSM_TRANS_CHANNEL_RELEASE_FAIL);
    EXPECT_EQ(g_tuningStatusParams.slqi, QOSM_TRANS_CHANNEL_SLQI_LOW);
    SetConnSetMcsExecCallbackAtOnce();
}