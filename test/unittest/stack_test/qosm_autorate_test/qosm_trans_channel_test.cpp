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
#include "cm_signaling_internal.h"
#include "qosm.h"
#include "qosm_errno.h"
#include "qosm_trans_channel.h"
#include "sdf_map.h"
#include "sdf_addr.h"
#include "sdf_dlist.h"
#include "sdf_traits.h"
#include "sdf_timer.h"
#include "sle_logic_link_mgr.h"
#include "qosm_log.h"

using namespace testing;

#ifdef __cplusplus
extern "C" {
#endif

#define QOSM_DYNC_TRANS_CHANNLE_TEST_LCID_3 3

static SleLogicLink_S g_link = {};
static CM_DynTransChannelCbks_S g_dynTransCbk = {};
static CM_LogicLinkCbks_S g_LogicLinkCbk = {};
static CM_DynTransChannelEstablishParamReq_S g_establishParams = {};
static CM_DynTransChannelReleaseParamReq_S g_releaseParams = {};
static QOSM_TransChannelRspParams_S g_channelStatusParams = {};
static CM_CapInfo_S g_capInfo = {.mtu = CM_CAP_MIN_MTU, .rxWnd = 100, .supportTransMode = 0X07};

static void TestQOSM_TransChannelStatusCbk(const QOSM_TransChannelRspParams_S *respParams)
{
    (void)memcpy_s(&g_channelStatusParams, sizeof(QOSM_TransChannelRspParams_S), respParams,
        sizeof(QOSM_TransChannelRspParams_S));
};

bool TestQOSM_ChannelEstablishedCheckCbk(uint16_t srcPort)
{
    return true;
}

uint32_t CP_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (cb != nullptr) {
        cb(arg);
    }
    if (freeCb != nullptr) {
        freeCb(arg);
    }
    return 0;
}

SleLogicLink_S *SleLogicLinkGetByLcid(uint16_t lcid)
{
    return &g_link;
}

SleLogicLink_S *SleLogicLinkGetByAddr(const SLE_Addr_S *addr)
{
    return &g_link;
}

uint32_t CM_RegDynTransChannelCbks(const CM_DynTransChannelCbks_S *cbks)
{
    if (cbks == nullptr) {
        return CM_FAIL;
    }
    if (memcpy_s(&g_dynTransCbk, sizeof(CM_DynTransChannelCbks_S), cbks, sizeof(CM_DynTransChannelCbks_S)) != EOK) {
        return CM_FAIL;
    }
    QOSM_LOGI("mock CM_RegDynTransChannelCbks");
    return CM_SUCCESS;
}

void CM_UnRegDynTransChannelCbks(void)
{
    (void)memset_s(&g_dynTransCbk, sizeof(CM_DynTransChannelCbks_S), 0x00, sizeof(CM_DynTransChannelCbks_S));
}

uint32_t CM_RegLogicLinkListener(CM_LogicLinkCbks_S *cbks)
{
    if (cbks == nullptr) {
        return CM_FAIL;
    }
    if (cbks->moduleId == CM_MODULE_QOSM) {
        if (memcpy_s(&g_LogicLinkCbk, sizeof(CM_LogicLinkCbks_S), cbks, sizeof(CM_LogicLinkCbks_S)) != EOK) {
            return CM_FAIL;
        }
        if (g_LogicLinkCbk.logicLinkCbk != nullptr) {
            CM_LogicLinkState_S state = { 0 };
            state.lcid = QOSM_DYNC_TRANS_CHANNLE_TEST_LCID_3;
            state.result = CM_LINK_STATE_CONNECTED;
            g_LogicLinkCbk.logicLinkCbk(&state);
        }
    }
    QOSM_LOGI("mock CM_RegLogicLinkListener");
    return CM_SUCCESS;
}

uint32_t CM_UnRegLogicLinkListener(uint8_t moduleId)
{
    if (moduleId == CM_MODULE_QOSM) {
        if (g_LogicLinkCbk.logicLinkCbk != nullptr) {
            CM_LogicLinkState_S state = { 0 };
            state.lcid = QOSM_DYNC_TRANS_CHANNLE_TEST_LCID_3;
            state.result = CM_LINK_STATE_DISCONNECTED;
            g_LogicLinkCbk.logicLinkCbk(&state);
        }
        memset_s(&g_LogicLinkCbk, sizeof(CM_LogicLinkCbks_S), 0x00, sizeof(CM_LogicLinkCbks_S));
    }
    return CM_SUCCESS;
}

uint32_t CM_DynTransChannelEstablishReq(const CM_DynTransChannelEstablishParamReq_S *param)
{
    if (memcpy_s(&g_establishParams, sizeof(CM_DynTransChannelEstablishParamReq_S), param,
        sizeof(CM_DynTransChannelEstablishParamReq_S)) != EOK) {
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_DynTransChannelReleaseReq(const CM_DynTransChannelReleaseParamReq_S *param)
{
    if (memcpy_s(&g_releaseParams, sizeof(CM_DynTransChannelReleaseParamReq_S), param,
        sizeof(CM_DynTransChannelReleaseParamReq_S)) != EOK) {
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_ConnectUpdateParamReq(CM_ConnectUpdateParamReq_S *param)
{
    return CM_SUCCESS;
}

uint32_t CM_SetPhy(CM_SetPhyReq_S *param)
{
    return CM_SUCCESS;
}

uint32_t CM_SetMcs(CM_SetMcsReq_S *param)
{
    return CM_SUCCESS;
}

uint32_t CM_GetLogicLinkCapInfo(CM_CapInfo_S *capInfo, const SLE_Addr_S *addr)
{
    if (capInfo == NULL) {
        return CM_FAIL;
    }
    *capInfo = g_capInfo;
    return CM_SUCCESS;
}

void QOSM_ICGMgrEnable(void)
{}

void QOSM_ICGMgrDisable(void)
{}

#ifdef __cplusplus
}
#endif

class UT_QOSM_TRANS_CHANNEL_TEST : public testing::Test {
protected:
    // SetUP 在每一个 TEST_F 测试开始前执行一次
    virtual void SetUp()
    {
        memset_s(&g_establishParams, sizeof(g_establishParams), 0, sizeof(g_establishParams));
        memset_s(&g_releaseParams, sizeof(g_releaseParams), 0, sizeof(g_releaseParams));
        memset_s(&g_channelStatusParams, sizeof(g_channelStatusParams), 0, sizeof(g_channelStatusParams));
    }
    // TearDown 在每一个 TEST_F 测试完成后执行一次
    virtual void TearDown()
    {
    }

    // SetUpTestCase 在所有 TEST_F 测试开始前执行一次
    static void SetUpTestCase()
    {
        QOSM_Init();
        QOSM_Enable();
        EXPECT_EQ(QOSM_TransChannelInit(), QOSM_SUCCESS);
        EXPECT_NE(g_dynTransCbk.establishRspCbk, nullptr);
        EXPECT_NE(g_dynTransCbk.releaseRspCbk, nullptr);
        EXPECT_NE(g_dynTransCbk.statusIndicationCbk, nullptr);
        EXPECT_NE(g_dynTransCbk.establishedCheckCbk, nullptr);

        QOSM_TransChannelCbks_S cbk = {0};
        cbk.statusCbk = TestQOSM_TransChannelStatusCbk;
        cbk.establishedCheck = TestQOSM_ChannelEstablishedCheckCbk;
        EXPECT_EQ(QOSM_TransChannelCbksRegister(&cbk), QOSM_SUCCESS);
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        EXPECT_EQ(QOSM_TransChannelCbksUnregister(), QOSM_SUCCESS);
        QOSM_TransChannelDeInit();
        QOSM_Disable();
        QOSM_DeInit();
    }
};

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelCreate)
{
    QOSM_TransChannelConf_S conf = {0};
    SLE_Addr_S addr = {0, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    QOSM_TransChannelParams_S params = {};
    params.addr = addr;
    params.linkMode = SLE_MODE_ACB;
    params.accessTransMode = 0; // 单播，CM_AccessTransportMode_E
    params.srcPort = 1000;
    params.dstPort = 2000;
    params.slqi = QOSM_TRANS_CHANNEL_SLQI_LOW;
    conf.mode = CM_TRANS_MODE_BASIC;
    params.tcConf = conf;
    params.frameType = QOSM_SLE_RADIO_FRAME_TYPE_1;
    EXPECT_EQ(QOSM_TransChannelCreate(&params), QOSM_SUCCESS);
    EXPECT_EQ(g_establishParams.addr.addr[0], params.addr.addr[0]);
    EXPECT_EQ(g_establishParams.addr.addr[1], params.addr.addr[1]);
    EXPECT_EQ(g_establishParams.addr.addr[2], params.addr.addr[2]);
    EXPECT_EQ(g_establishParams.addr.addr[3], params.addr.addr[3]);
    EXPECT_EQ(g_establishParams.addr.addr[4], params.addr.addr[4]);
    EXPECT_EQ(g_establishParams.addr.addr[5], params.addr.addr[5]);
    EXPECT_EQ(g_establishParams.srcPort, params.srcPort);
    EXPECT_EQ(g_establishParams.dstPort, params.dstPort);
    EXPECT_EQ(g_establishParams.slqi, params.slqi);
    EXPECT_EQ(g_establishParams.frameType, (CM_TransConnFrameType_E)params.frameType);

    // trans channel create callback
    CM_DynTransChanEstablishParamRsp_S rsp = {};
    rsp.addr = params.addr;
    rsp.transMode = conf.mode;
    rsp.lcid = QOSM_DYNC_TRANS_CHANNLE_TEST_LCID_3;
    rsp.srcTcid = CM_TCID_SLE_CMTC;
    rsp.srcPort = params.srcPort;
    rsp.dstPort = params.dstPort;
    rsp.mtu = 1500 - CM_CAP_MIN_MTU;
    rsp.slqiList.slqiNum = 1;
    rsp.slqiList.slqi[0] = params.slqi;
    rsp.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    rsp.result = CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS;
    g_dynTransCbk.establishRspCbk(&rsp);
    EXPECT_EQ(g_channelStatusParams.addr.addr[0], rsp.addr.addr[0]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[1], rsp.addr.addr[1]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[2], rsp.addr.addr[2]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[3], rsp.addr.addr[3]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[4], rsp.addr.addr[4]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[5], rsp.addr.addr[5]);
    EXPECT_EQ(g_channelStatusParams.transMode, rsp.transMode);
    EXPECT_EQ(g_channelStatusParams.tcid, rsp.srcTcid);
    EXPECT_EQ(g_channelStatusParams.lcid, rsp.lcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, rsp.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, rsp.dstPort);
    EXPECT_EQ(g_channelStatusParams.slqi, rsp.slqiList.slqi[0]);
    EXPECT_EQ(g_channelStatusParams.frameType, (QOSM_TransConnFrameType_E)rsp.frameType);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);

    params.srcPort = 1001;
    params.dstPort = 2001;
    conf.mode = CM_TRANS_MODE_RELIABLE;
    params.tcConf = conf;
    EXPECT_EQ(QOSM_TransChannelCreate(&params), QOSM_SUCCESS);
    EXPECT_EQ(g_establishParams.srcPort, params.srcPort);
    EXPECT_EQ(g_establishParams.dstPort, params.dstPort);
    rsp.srcTcid = CM_TCID_BC_BEGIN;
    rsp.srcPort = params.srcPort;
    rsp.dstPort = params.dstPort;
    g_dynTransCbk.establishRspCbk(&rsp);
    EXPECT_EQ(g_channelStatusParams.transMode, rsp.transMode);
    EXPECT_EQ(g_channelStatusParams.tcid, rsp.srcTcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, rsp.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, rsp.dstPort);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);

    params.srcPort = 1002;
    params.dstPort = 2002;
    conf.mode = CM_TRANS_MODE_STREAM;
    params.tcConf = conf;
    EXPECT_EQ(QOSM_TransChannelCreate(&params), QOSM_SUCCESS);
    EXPECT_EQ(g_establishParams.srcPort, params.srcPort);
    EXPECT_EQ(g_establishParams.dstPort, params.dstPort);
    rsp.srcTcid = CM_TCID_BC_BEGIN + 1;
    rsp.srcPort = params.srcPort;
    rsp.dstPort = params.dstPort;
    g_dynTransCbk.establishRspCbk(&rsp);
    EXPECT_EQ(g_channelStatusParams.transMode, rsp.transMode);
    EXPECT_EQ(g_channelStatusParams.tcid, rsp.srcTcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, rsp.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, rsp.dstPort);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelDestroy)
{
    SLE_Addr_S addr = {0, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    QOSM_TransChannelReleaseParams_S params = {};
    params.addr = addr;
    params.tcid = CM_TCID_SLE_CMTC;
    EXPECT_EQ(QOSM_TransChannelDestroy(&params), QOSM_SUCCESS);
    EXPECT_EQ(g_releaseParams.addr.addr[0], params.addr.addr[0]);
    EXPECT_EQ(g_releaseParams.addr.addr[1], params.addr.addr[1]);
    EXPECT_EQ(g_releaseParams.addr.addr[2], params.addr.addr[2]);
    EXPECT_EQ(g_releaseParams.addr.addr[3], params.addr.addr[3]);
    EXPECT_EQ(g_releaseParams.addr.addr[4], params.addr.addr[4]);
    EXPECT_EQ(g_releaseParams.addr.addr[5], params.addr.addr[5]);
    EXPECT_EQ(g_releaseParams.srcTcid, params.tcid);

    // trans channel destroy callback
    CM_DynTransChanReleaseParamRsp_S rsp = {};
    rsp.result = CM_DYN_TRANS_CHAN_RELEASE_SUCCESS;
    rsp.srcPort = 1000;
    rsp.dstPort = 2000;
    rsp.srcTcid = params.tcid;
    rsp.dstTcid = params.tcid;
    rsp.lcid = QOSM_DYNC_TRANS_CHANNLE_TEST_LCID_3;
    rsp.slqiList.slqiNum = 1;
    rsp.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_LOW;
    rsp.addr = addr;
    rsp.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    g_dynTransCbk.releaseRspCbk(&rsp);
    EXPECT_EQ(g_channelStatusParams.addr.addr[0], rsp.addr.addr[0]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[1], rsp.addr.addr[1]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[2], rsp.addr.addr[2]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[3], rsp.addr.addr[3]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[4], rsp.addr.addr[4]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[5], rsp.addr.addr[5]);
    EXPECT_EQ(g_channelStatusParams.tcid, rsp.srcTcid);
    EXPECT_EQ(g_channelStatusParams.lcid, rsp.lcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, rsp.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, rsp.dstPort);
    EXPECT_EQ(g_channelStatusParams.slqi, rsp.slqiList.slqi[0]);
    EXPECT_EQ(g_channelStatusParams.frameType, (QOSM_TransConnFrameType_E)rsp.frameType);

    params.tcid = CM_TCID_BC_BEGIN;
    EXPECT_EQ(QOSM_TransChannelDestroy(&params), QOSM_SUCCESS);
    EXPECT_EQ(g_releaseParams.srcTcid, params.tcid);
    rsp.srcPort = 1001;
    rsp.dstPort = 2001;
    g_dynTransCbk.releaseRspCbk(&rsp);
    EXPECT_EQ(g_channelStatusParams.tcid, rsp.srcTcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, rsp.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, rsp.dstPort);

    params.tcid = CM_TCID_BC_BEGIN + 1;
    EXPECT_EQ(QOSM_TransChannelDestroy(&params), QOSM_SUCCESS);
    EXPECT_EQ(g_releaseParams.srcTcid, params.tcid);
    rsp.srcPort = 1002;
    rsp.dstPort = 2002;
    g_dynTransCbk.releaseRspCbk(&rsp);
    EXPECT_EQ(g_channelStatusParams.tcid, rsp.srcTcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, rsp.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, rsp.dstPort);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelIsCreated)
{
    CM_DynTransChanStatusIndicationRsp_S indication = {};
    indication.result = CM_DYN_TRANS_CHAN_STATUS_INDICATION_NORMAL;
    indication.srcPort = 3000;
    indication.dstPort = 4000;
    indication.added = 1;
    indication.transMode = CM_TRANS_MODE_BASIC;
    indication.srcTcid = CM_TCID_UC_BEGIN;
    indication.dstTcid = CM_TCID_UC_BEGIN;
    indication.slqiList.slqiNum = 1;
    indication.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_LOW;
    indication.lcid = 4;
    indication.addr = {0, {0x11, 0x12, 0x13, 0x14, 0x15, 0x16}};
    indication.mtu = 1500 - CM_CAP_MIN_MTU;
    g_dynTransCbk.statusIndicationCbk(&indication);
    EXPECT_EQ(g_channelStatusParams.addr.addr[0], indication.addr.addr[0]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[1], indication.addr.addr[1]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[2], indication.addr.addr[2]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[3], indication.addr.addr[3]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[4], indication.addr.addr[4]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[5], indication.addr.addr[5]);
    EXPECT_EQ(g_channelStatusParams.transMode, indication.transMode);
    EXPECT_EQ(g_channelStatusParams.tcid, indication.srcTcid);
    EXPECT_EQ(g_channelStatusParams.lcid, indication.lcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, indication.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, indication.dstPort);
    EXPECT_EQ(g_channelStatusParams.slqi, indication.slqiList.slqi[0]);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);

    indication.srcTcid = CM_TCID_UC_BEGIN;
    indication.dstTcid = CM_TCID_UC_BEGIN;
    indication.srcPort = 3001;
    indication.dstPort = 4001;
    indication.transMode = CM_TRANS_MODE_RELIABLE;
    g_dynTransCbk.statusIndicationCbk(&indication);
    EXPECT_EQ(g_channelStatusParams.transMode, indication.transMode);
    EXPECT_EQ(g_channelStatusParams.tcid, indication.srcTcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, indication.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, indication.dstPort);

    indication.srcTcid = CM_TCID_UC_BEGIN + 1;
    indication.dstTcid = CM_TCID_UC_BEGIN + 1;
    indication.srcPort = 3002;
    indication.dstPort = 4002;
    indication.transMode = CM_TRANS_MODE_STREAM;
    g_dynTransCbk.statusIndicationCbk(&indication);
    EXPECT_EQ(g_channelStatusParams.transMode, indication.transMode);
    EXPECT_EQ(g_channelStatusParams.tcid, indication.srcTcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, indication.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, indication.dstPort);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelIsDestroyed)
{
    CM_DynTransChanStatusIndicationRsp_S indication = {};
    indication.result = CM_DYN_TRANS_CHAN_STATUS_INDICATION_DICONNECTED;
    indication.srcPort = 3000;
    indication.dstPort = 4000;
    indication.added = 1;
    indication.transMode = CM_TRANS_MODE_BASIC;
    indication.srcTcid = CM_TCID_SLE_CMTC;
    indication.dstTcid = CM_TCID_SLE_CMTC;
    indication.slqiList.slqiNum = 1;
    indication.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_LOW;
    indication.lcid = 4;
    indication.addr = {0, {0x11, 0x12, 0x13, 0x14, 0x15, 0x16}};
    indication.mtu = 1500 - CM_CAP_MIN_MTU;
    g_dynTransCbk.statusIndicationCbk(&indication);
    EXPECT_EQ(g_channelStatusParams.addr.addr[0], indication.addr.addr[0]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[1], indication.addr.addr[1]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[2], indication.addr.addr[2]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[3], indication.addr.addr[3]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[4], indication.addr.addr[4]);
    EXPECT_EQ(g_channelStatusParams.addr.addr[5], indication.addr.addr[5]);
    EXPECT_EQ(g_channelStatusParams.transMode, indication.transMode);
    EXPECT_EQ(g_channelStatusParams.tcid, indication.srcTcid);
    EXPECT_EQ(g_channelStatusParams.lcid, indication.lcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, indication.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, indication.dstPort);
    EXPECT_EQ(g_channelStatusParams.slqi, indication.slqiList.slqi[0]);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_RELEASED);

    indication.srcTcid = CM_TCID_UC_BEGIN;
    indication.dstTcid = CM_TCID_UC_BEGIN;
    indication.srcPort = 3001;
    indication.dstPort = 4001;
    indication.transMode = CM_TRANS_MODE_RELIABLE;
    g_dynTransCbk.statusIndicationCbk(&indication);
    EXPECT_EQ(g_channelStatusParams.transMode, indication.transMode);
    EXPECT_EQ(g_channelStatusParams.tcid, indication.srcTcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, indication.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, indication.dstPort);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_RELEASED);

    indication.srcTcid = CM_TCID_UC_BEGIN + 1;
    indication.dstTcid = CM_TCID_UC_BEGIN + 1;
    indication.srcPort = 3002;
    indication.dstPort = 4002;
    indication.transMode = CM_TRANS_MODE_STREAM;
    g_dynTransCbk.statusIndicationCbk(&indication);
    EXPECT_EQ(g_channelStatusParams.transMode, indication.transMode);
    EXPECT_EQ(g_channelStatusParams.tcid, indication.srcTcid);
    EXPECT_EQ(g_channelStatusParams.srcPort, indication.srcPort);
    EXPECT_EQ(g_channelStatusParams.dstPort, indication.dstPort);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_RELEASED);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelEstablishedCheckCbk_NullUserCbk)
{
    EXPECT_EQ(g_dynTransCbk.establishedCheckCbk(NULL), false);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelEstablishedCheckCbk_CallbackRegistered)
{
    ASSERT_NE(g_dynTransCbk.establishedCheckCbk, nullptr);

    CM_DynTransChanEstablishedCheckParam_S checkParam = {};
    checkParam.srcPort = 9002;
    checkParam.dstPort = 9003;

    bool result = g_dynTransCbk.establishedCheckCbk(&checkParam);
    EXPECT_TRUE(result);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelInvalidMtu)
{
    QOSM_TransChannelParams_S params = {};
    params.linkMode = SLE_MODE_ACB;
    params.accessTransMode = ACCESS_TRANS_MODE_UNICAST;
    params.slqi = QOSM_TRANS_CHANNEL_SLQI_LOW;
    params.tcConf.mode = CM_TRANS_MODE_BASIC;
    g_capInfo.mtu = CM_CAP_MIN_MTU - 1;

    EXPECT_EQ(QOSM_TransChannelCreate(&params), QOSM_SUCCESS);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelUpdateConnParamsSlqiSame)
{
    QOSM_TransChannelParams_S params = {};
    SLE_Addr_S addr = {0, {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
    params.addr = addr;
    params.linkMode = SLE_MODE_ACB;
    params.accessTransMode = 0;
    params.srcPort = 5000;
    params.dstPort = 6000;
    params.slqi = QOSM_TRANS_CHANNEL_SLQI_LOW;
    params.tcConf.mode = CM_TRANS_MODE_BASIC;
    params.frameType = QOSM_SLE_RADIO_FRAME_TYPE_1;
    EXPECT_EQ(QOSM_TransChannelCreate(&params), QOSM_SUCCESS);

    CM_DynTransChanEstablishParamRsp_S rsp = {};
    rsp.addr = addr;
    rsp.transMode = CM_TRANS_MODE_BASIC;
    rsp.lcid = 5;
    rsp.srcTcid = CM_TCID_SLE_CMTC;
    rsp.srcPort = params.srcPort;
    rsp.dstPort = params.dstPort;
    rsp.mtu = 1500 - CM_CAP_MIN_MTU;
    rsp.slqiList.slqiNum = 1;
    rsp.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_LOW;
    rsp.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    rsp.result = CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS;
    g_dynTransCbk.establishRspCbk(&rsp);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelUpdateConnParamsSlqiDifferent)
{
    g_channelStatusParams.status = QOSM_TRANS_CHANNEL_ESTABLISH_FAIL; // 动态传输通道初始化未创建成功
    QOSM_TransChannelParams_S params = {};
    SLE_Addr_S addr = {0, {0x11, 0x12, 0x13, 0x14, 0x15, 0x16}};
    params.addr = addr;
    params.linkMode = SLE_MODE_ACB;
    params.accessTransMode = 0;
    params.srcPort = 5001;
    params.dstPort = 6001;
    params.slqi = QOSM_TRANS_CHANNEL_SLQI_HIGH;
    params.tcConf.mode = CM_TRANS_MODE_RELIABLE;
    params.frameType = QOSM_SLE_RADIO_FRAME_TYPE_1;
    EXPECT_EQ(QOSM_TransChannelCreate(&params), QOSM_SUCCESS);

    CM_DynTransChanEstablishParamRsp_S rsp = {};
    rsp.addr = addr;
    rsp.transMode = CM_TRANS_MODE_RELIABLE;
    rsp.lcid = QOSM_DYNC_TRANS_CHANNLE_TEST_LCID_3;
    rsp.srcTcid = CM_TCID_SLE_CMTC;
    rsp.srcPort = params.srcPort;
    rsp.dstPort = params.dstPort;
    rsp.mtu = 1500 - CM_CAP_MIN_MTU;
    rsp.slqiList.slqiNum = 1;
    rsp.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_HIGH;
    rsp.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    rsp.result = CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS;
    g_dynTransCbk.establishRspCbk(&rsp);
    // 不同的slqi 需要进行参数等流程更新，此处因为未打桩回调，所以实际未完全创建完成，即创建不成功
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);

    params.srcPort = 5002;
    params.dstPort = 6002;
    params.slqi = QOSM_TRANS_CHANNEL_SLQI_LOW;
    EXPECT_EQ(QOSM_TransChannelCreate(&params), QOSM_SUCCESS);
    rsp.srcPort = params.srcPort;
    rsp.dstPort = params.dstPort;
    rsp.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_LOW;
    g_dynTransCbk.establishRspCbk(&rsp);
    // 未等待上一个通道创建完成，则不允许立即创建下一个通道传输
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISH_FAIL);

    QOSM_TransChannelReleaseParams_S releaseParams = {};
    releaseParams.addr = addr;
    releaseParams.tcid = CM_TCID_SLE_CMTC;
    EXPECT_EQ(QOSM_TransChannelDestroy(&releaseParams), QOSM_SUCCESS);

    CM_DynTransChanReleaseParamRsp_S releaseRsp = {};
    releaseRsp.result = CM_DYN_TRANS_CHAN_RELEASE_SUCCESS;
    releaseRsp.srcPort = params.srcPort;
    releaseRsp.dstPort = params.dstPort;
    releaseRsp.srcTcid = CM_TCID_SLE_CMTC;
    releaseRsp.dstTcid = CM_TCID_SLE_CMTC;
    releaseRsp.lcid = QOSM_DYNC_TRANS_CHANNLE_TEST_LCID_3;
    releaseRsp.slqiList.slqiNum = 0;
    releaseRsp.slqiList.slqi[0] = 0;
    releaseRsp.addr = addr;
    releaseRsp.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    g_dynTransCbk.releaseRspCbk(&releaseRsp);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_RELEASED);

    EXPECT_EQ(QOSM_TransChannelCreate(&params), QOSM_SUCCESS);
    rsp.srcPort = params.srcPort;
    rsp.dstPort = params.dstPort;
    rsp.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_LOW;
    g_dynTransCbk.establishRspCbk(&rsp);
    // slqi 值仍然为QOSM_TRANS_CHANNEL_SLQI_LOW，不需要进行等流程更新，将返回动态传输通道创建成功流程
    // 上一个通道已经销毁，则允许创建下一个通道传输
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelUpdateConnParamsAfterRelease)
{
    QOSM_TransChannelParams_S params = {};
    SLE_Addr_S addr = {0, {0x21, 0x22, 0x23, 0x24, 0x25, 0x26}};
    params.addr = addr;
    params.linkMode = SLE_MODE_ACB;
    params.accessTransMode = 0;
    params.srcPort = 5003;
    params.dstPort = 6003;
    params.slqi = QOSM_TRANS_CHANNEL_SLQI_HIGH;
    params.tcConf.mode = CM_TRANS_MODE_STREAM;
    params.frameType = QOSM_SLE_RADIO_FRAME_TYPE_1;
    EXPECT_EQ(QOSM_TransChannelCreate(&params), QOSM_SUCCESS);

    CM_DynTransChanEstablishParamRsp_S rsp = {};
    rsp.addr = addr;
    rsp.transMode = CM_TRANS_MODE_STREAM;
    rsp.lcid = 7;
    rsp.srcTcid = CM_TCID_SLE_CMTC;
    rsp.srcPort = params.srcPort;
    rsp.dstPort = params.dstPort;
    rsp.mtu = 1500 - CM_CAP_MIN_MTU;
    rsp.slqiList.slqiNum = 1;
    rsp.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_HIGH;
    rsp.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    rsp.result = CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS;
    g_dynTransCbk.establishRspCbk(&rsp);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);

    QOSM_TransChannelReleaseParams_S releaseParams = {};
    releaseParams.addr = addr;
    releaseParams.tcid = CM_TCID_SLE_CMTC;
    EXPECT_EQ(QOSM_TransChannelDestroy(&releaseParams), QOSM_SUCCESS);

    CM_DynTransChanReleaseParamRsp_S releaseRsp = {};
    releaseRsp.result = CM_DYN_TRANS_CHAN_RELEASE_SUCCESS;
    releaseRsp.srcPort = params.srcPort;
    releaseRsp.dstPort = params.dstPort;
    releaseRsp.srcTcid = CM_TCID_SLE_CMTC;
    releaseRsp.dstTcid = CM_TCID_SLE_CMTC;
    releaseRsp.lcid = 7;
    releaseRsp.slqiList.slqiNum = 0;
    releaseRsp.slqiList.slqi[0] = 0;
    releaseRsp.addr = addr;
    releaseRsp.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    g_dynTransCbk.releaseRspCbk(&releaseRsp);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_RELEASED);
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelFreeTransChannelNode)
{
    SLE_Addr_S addr1 = {0, {0x31, 0x32, 0x33, 0x34, 0x35, 0x36}};
    SLE_Addr_S addr2 = {0, {0x41, 0x42, 0x43, 0x44, 0x45, 0x46}};
    SLE_Addr_S addr3 = {0, {0x51, 0x52, 0x53, 0x54, 0x55, 0x56}};

    QOSM_TransChannelParams_S params1 = {};
    params1.addr = addr1;
    params1.linkMode = SLE_MODE_ACB;
    params1.accessTransMode = 0;
    params1.srcPort = 6001;
    params1.dstPort = 7001;
    params1.slqi = QOSM_TRANS_CHANNEL_SLQI_LOW;
    params1.tcConf.mode = CM_TRANS_MODE_BASIC;
    params1.frameType = QOSM_SLE_RADIO_FRAME_TYPE_1;
    EXPECT_EQ(QOSM_TransChannelCreate(&params1), QOSM_SUCCESS);

    CM_DynTransChanEstablishParamRsp_S rsp1 = {};
    rsp1.addr = addr1;
    rsp1.transMode = CM_TRANS_MODE_BASIC;
    rsp1.lcid = 10;
    rsp1.srcTcid = CM_TCID_SLE_CMTC;
    rsp1.srcPort = params1.srcPort;
    rsp1.dstPort = params1.dstPort;
    rsp1.mtu = 1500 - CM_CAP_MIN_MTU;
    rsp1.slqiList.slqiNum = 1;
    rsp1.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_LOW;
    rsp1.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    rsp1.result = CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS;
    g_dynTransCbk.establishRspCbk(&rsp1);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);

    QOSM_TransChannelParams_S params2 = {};
    params2.addr = addr2;
    params2.linkMode = SLE_MODE_ACB;
    params2.accessTransMode = 0;
    params2.srcPort = 6002;
    params2.dstPort = 7002;
    params2.slqi = QOSM_TRANS_CHANNEL_SLQI_LOW;
    params2.tcConf.mode = CM_TRANS_MODE_RELIABLE;
    params2.frameType = QOSM_SLE_RADIO_FRAME_TYPE_1;
    EXPECT_EQ(QOSM_TransChannelCreate(&params2), QOSM_SUCCESS);

    CM_DynTransChanEstablishParamRsp_S rsp2 = {};
    rsp2.addr = addr2;
    rsp2.transMode = CM_TRANS_MODE_RELIABLE;
    rsp2.lcid = 11;
    rsp2.srcTcid = CM_TCID_SLE_CMTC;
    rsp2.srcPort = params2.srcPort;
    rsp2.dstPort = params2.dstPort;
    rsp2.mtu = 1500 - CM_CAP_MIN_MTU;
    rsp2.slqiList.slqiNum = 1;
    rsp2.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_LOW;
    rsp2.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    rsp2.result = CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS;
    g_dynTransCbk.establishRspCbk(&rsp2);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);

    QOSM_TransChannelParams_S params3 = {};
    params3.addr = addr3;
    params3.linkMode = SLE_MODE_ACB;
    params3.accessTransMode = 0;
    params3.srcPort = 6003;
    params3.dstPort = 7003;
    params3.slqi = QOSM_TRANS_CHANNEL_SLQI_HIGH;
    params3.tcConf.mode = CM_TRANS_MODE_STREAM;
    params3.frameType = QOSM_SLE_RADIO_FRAME_TYPE_1;
    EXPECT_EQ(QOSM_TransChannelCreate(&params3), QOSM_SUCCESS);

    CM_DynTransChanEstablishParamRsp_S rsp3 = {};
    rsp3.addr = addr3;
    rsp3.transMode = CM_TRANS_MODE_STREAM;
    rsp3.lcid = 12;
    rsp3.srcTcid = CM_TCID_SLE_CMTC;
    rsp3.srcPort = params3.srcPort;
    rsp3.dstPort = params3.dstPort;
    rsp3.mtu = 1500 - CM_CAP_MIN_MTU;
    rsp3.slqiList.slqiNum = 1;
    rsp3.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_HIGH;
    rsp3.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    rsp3.result = CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS;
    g_dynTransCbk.establishRspCbk(&rsp3);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);

    QOSM_TransChannelReleaseParams_S releaseParams1 = {};
    releaseParams1.addr = addr1;
    releaseParams1.tcid = CM_TCID_SLE_CMTC;
    EXPECT_EQ(QOSM_TransChannelDestroy(&releaseParams1), QOSM_SUCCESS);

    CM_DynTransChanReleaseParamRsp_S releaseRsp1 = {};
    releaseRsp1.result = CM_DYN_TRANS_CHAN_RELEASE_SUCCESS;
    releaseRsp1.srcPort = params1.srcPort;
    releaseRsp1.dstPort = params1.dstPort;
    releaseRsp1.srcTcid = CM_TCID_SLE_CMTC;
    releaseRsp1.dstTcid = CM_TCID_SLE_CMTC;
    releaseRsp1.lcid = 10;
    releaseRsp1.slqiList.slqiNum = 0;
    releaseRsp1.slqiList.slqi[0] = 0;
    releaseRsp1.addr = addr1;
    releaseRsp1.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    g_dynTransCbk.releaseRspCbk(&releaseRsp1);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_RELEASED);

    QOSM_TransChannelReleaseParams_S releaseParams2 = {};
    releaseParams2.addr = addr2;
    releaseParams2.tcid = CM_TCID_SLE_CMTC;
    EXPECT_EQ(QOSM_TransChannelDestroy(&releaseParams2), QOSM_SUCCESS);

    CM_DynTransChanReleaseParamRsp_S releaseRsp2 = {};
    releaseRsp2.result = CM_DYN_TRANS_CHAN_RELEASE_SUCCESS;
    releaseRsp2.srcPort = params2.srcPort;
    releaseRsp2.dstPort = params2.dstPort;
    releaseRsp2.srcTcid = CM_TCID_SLE_CMTC;
    releaseRsp2.dstTcid = CM_TCID_SLE_CMTC;
    releaseRsp2.lcid = 11;
    releaseRsp2.slqiList.slqiNum = 0;
    releaseRsp2.slqiList.slqi[0] = 0;
    releaseRsp2.addr = addr2;
    releaseRsp2.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    g_dynTransCbk.releaseRspCbk(&releaseRsp2);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_RELEASED);

    QOSM_TransChannelReleaseParams_S releaseParams3 = {};
    releaseParams3.addr = addr3;
    releaseParams3.tcid = CM_TCID_SLE_CMTC;
    EXPECT_EQ(QOSM_TransChannelDestroy(&releaseParams3), QOSM_SUCCESS);

    CM_DynTransChanReleaseParamRsp_S releaseRsp3 = {};
    releaseRsp3.result = CM_DYN_TRANS_CHAN_RELEASE_SUCCESS;
    releaseRsp3.srcPort = params3.srcPort;
    releaseRsp3.dstPort = params3.dstPort;
    releaseRsp3.srcTcid = CM_TCID_SLE_CMTC;
    releaseRsp3.dstTcid = CM_TCID_SLE_CMTC;
    releaseRsp3.lcid = 12;
    releaseRsp3.slqiList.slqiNum = 0;
    releaseRsp3.slqiList.slqi[0] = 0;
    releaseRsp3.addr = addr3;
    releaseRsp3.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    g_dynTransCbk.releaseRspCbk(&releaseRsp3);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_RELEASED);

    QOSM_TransChannelDeInit();
    QOSM_TransChannelInit();
}

TEST_F(UT_QOSM_TRANS_CHANNEL_TEST, TestCaseTransChannelDeInitFreesNodes)
{
    SLE_Addr_S addr1 = {0, {0x61, 0x62, 0x63, 0x64, 0x65, 0x66}};
    SLE_Addr_S addr2 = {0, {0x71, 0x72, 0x73, 0x74, 0x75, 0x76}};

    QOSM_TransChannelParams_S params1 = {};
    params1.addr = addr1;
    params1.linkMode = SLE_MODE_ACB;
    params1.accessTransMode = 0;
    params1.srcPort = 8001;
    params1.dstPort = 9001;
    params1.slqi = QOSM_TRANS_CHANNEL_SLQI_LOW;
    params1.tcConf.mode = CM_TRANS_MODE_BASIC;
    params1.frameType = QOSM_SLE_RADIO_FRAME_TYPE_1;
    EXPECT_EQ(QOSM_TransChannelCreate(&params1), QOSM_SUCCESS);

    CM_DynTransChanEstablishParamRsp_S rsp1 = {};
    rsp1.addr = addr1;
    rsp1.transMode = CM_TRANS_MODE_BASIC;
    rsp1.lcid = 20;
    rsp1.srcTcid = CM_TCID_SLE_CMTC;
    rsp1.srcPort = params1.srcPort;
    rsp1.dstPort = params1.dstPort;
    rsp1.mtu = 1500 - CM_CAP_MIN_MTU;
    rsp1.slqiList.slqiNum = 1;
    rsp1.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_LOW;
    rsp1.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    rsp1.result = CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS;
    g_dynTransCbk.establishRspCbk(&rsp1);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);

    QOSM_TransChannelParams_S params2 = {};
    params2.addr = addr2;
    params2.linkMode = SLE_MODE_ACB;
    params2.accessTransMode = 0;
    params2.srcPort = 8002;
    params2.dstPort = 9002;
    params2.slqi = QOSM_TRANS_CHANNEL_SLQI_HIGH;
    params2.tcConf.mode = CM_TRANS_MODE_RELIABLE;
    params2.frameType = QOSM_SLE_RADIO_FRAME_TYPE_1;
    EXPECT_EQ(QOSM_TransChannelCreate(&params2), QOSM_SUCCESS);

    CM_DynTransChanEstablishParamRsp_S rsp2 = {};
    rsp2.addr = addr2;
    rsp2.transMode = CM_TRANS_MODE_RELIABLE;
    rsp2.lcid = 21;
    rsp2.srcTcid = CM_TCID_SLE_CMTC;
    rsp2.srcPort = params2.srcPort;
    rsp2.dstPort = params2.dstPort;
    rsp2.mtu = 1500 - CM_CAP_MIN_MTU;
    rsp2.slqiList.slqiNum = 1;
    rsp2.slqiList.slqi[0] = QOSM_TRANS_CHANNEL_SLQI_HIGH;
    rsp2.frameType = (CM_TransConnFrameType_E)QOSM_SLE_RADIO_FRAME_TYPE_1;
    rsp2.result = CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS;
    g_dynTransCbk.establishRspCbk(&rsp2);
    EXPECT_EQ(g_channelStatusParams.status, QOSM_TRANS_CHANNEL_ESTABLISHED);

    QOSM_TransChannelDeInit();
    QOSM_TransChannelInit();
}