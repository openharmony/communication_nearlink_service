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
#include "sle_logic_link_mgr.h"
#include "common_ext_func_wrapper.h"
#include "common_reg_ext_func.h"

using namespace testing;
using namespace testing::ext;

#ifdef __cplusplus
extern "C" {
#endif

static SleLogicLink_S g_link = {};
static CM_DynTransChannelCbks_S g_dynTransCbk = {};
static CM_LogicLinkCbks_S g_LogicLinkCbk = {};
static CM_DynTransChannelEstablishParamReq_S g_establishParams = {};
static CM_DynTransChannelReleaseParamReq_S g_releaseParams = {};
static QOSM_TransChannelRspParams_S g_channelStatusParams = {};

static void TestQOSM_TransChannelStatusCbk(const QOSM_TransChannelRspParams_S *respParams)
{
    (void)memcpy_s(&g_channelStatusParams, sizeof(QOSM_TransChannelRspParams_S), respParams,
        sizeof(QOSM_TransChannelRspParams_S));
};

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
    }
    return CM_SUCCESS;
}

uint32_t CM_UnRegLogicLinkListener(uint16_t moduleId)
{
    if (moduleId == CM_MODULE_QOSM) {
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

uint32_t CM_GetLogicLinkConnectedSize(void)
{
    return 0;
}

uint32_t CM_DynTransChannelReleaseReq(const CM_DynTransChannelReleaseParamReq_S *param)
{
    if (memcpy_s(&g_releaseParams, sizeof(CM_DynTransChannelReleaseParamReq_S), param,
        sizeof(CM_DynTransChannelReleaseParamReq_S)) != EOK) {
        return CM_FAIL;
    }
    return CM_SUCCESS;
}

uint32_t CM_GetLogicLinkCapInfo(CM_CapInfo_S *capInfo, const SLE_Addr_S *addr)
{
    capInfo->rxWnd = 100;
    capInfo->supportTransMode = 0X07;
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
        EXPECT_EQ(QOSM_TransChannelInit(), QOSM_SUCCESS);
        EXPECT_NE(g_dynTransCbk.establishRspCbk, nullptr);
        EXPECT_NE(g_dynTransCbk.releaseRspCbk, nullptr);
        EXPECT_NE(g_dynTransCbk.statusIndicationCbk, nullptr);

        QOSM_TransChannelCbks_S cbk = {0};
        cbk.statusCbk = TestQOSM_TransChannelStatusCbk;
        EXPECT_EQ(QOSM_TransChannelCbksRegister(&cbk), QOSM_SUCCESS);
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        EXPECT_EQ(QOSM_TransChannelCbksUnregister(), QOSM_SUCCESS);
        QOSM_TransChannelDeInit();
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
    rsp.lcid = 3;
    rsp.srcTcid = CM_TCID_SLE_CMTC;
    rsp.srcPort = params.srcPort;
    rsp.dstPort = params.dstPort;
    rsp.mtu = 1500 - DEFAULT_MAX_PACKET_HEADER_LEN;
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
    rsp.lcid = 3;
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
    indication.mtu = 1500 - DEFAULT_MAX_PACKET_HEADER_LEN;
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
    indication.mtu = 1500 - DEFAULT_MAX_PACKET_HEADER_LEN;
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
