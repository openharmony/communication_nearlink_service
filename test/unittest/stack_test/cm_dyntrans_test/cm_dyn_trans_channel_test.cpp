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
#include <vector>
#include <algorithm>
#include "securec.h"
#include "sdf_worker.h"
#include "cp_worker.h"
#include "dli_opcode.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "dli_errno.h"
#include "dli.h"
#include "dli_cmd.h"
#include "sle_connect_param.h"
#include "sle_access_dli.h"
#include "cm_api.h"
#include "cm_def.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "cm_logic_link_api.h"
#include "cm_trans_channel_api.h"
#include "cm_signaling_internal.h"
#include "cm_signaling_trans_channel.h"
#include "cm_dyn_trans_channel_api.h"
#include "cm_trans_channel_mgr.h"
#include "cm_util.h"
#include "cm_util_test.h"
#include "cm_signaling_trans_channel_mocker.h"

// 宏定义测试值，与生产实际值不一定一致
#define UT_CM_SRC_PORT_0 0x1010
#define UT_CM_DST_PORT_0 0x1020
#define UT_CM_AID 999
#define UT_CM_MTU 1500
#define UT_CM_SQLI CM_TRANS_CHANNEL_SLQI_HIGH
#define UT_CM_TRANS_CHAN_SLQI_SURPPORT_NUM 1U

#define UT_CM_INVALID_TCID 0
#define UT_CM_SRC_TCID ((CM_TCID_UC_BEGIN))
#define UT_CM_DST_TCID ((CM_TCID_UC_BEGIN) + 10)

#define UT_CM_DYN_TC_ACTIVE_REQ_ID 0x01
#define UT_CM_DYN_TC_PASSIVE_REQ_ID 0x02
#define UT_CM_DYN_TC_ONE_SIZE 1

typedef struct {
    uint16_t lcid;
    uint8_t srcTcidResult;
    uint8_t dstTcidResult;
    uint16_t srcPortResult;
    uint16_t dstPortResult;
    uint8_t slqiResult;
    bool transChanAdded;
    SLE_Addr_S sleAddr;
} CM_TestDynChannelParamResult;

static bool g_testTransChanAdded = false;
static uint32_t g_testDynChannelStateChangeSize = 0;
static CM_TestDynChannelParamResult g_testDynChannelParamResult = { 0 };
static std::vector<CM_TestDynChannelParamResult> g_testDynChannelParamResultList;
static const uint16_t g_passivehandleInitValue = 0x2045;
static SLE_Addr_S g_emptyTestAddr = {};

template <typename T>
inline void UT_CM_SleCompleteEvt(uint16_t cmdOpcode, uint16_t cbkCmdOpcode, T &eventParameter)
{
    uint8_t size = (std::is_same<decltype(eventParameter), std::nullptr_t>::value) ? 0 : sizeof(T);
    void *eventP = (size == 0) ? NULL : &eventParameter;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = cbkCmdOpcode,
        .size = size,
        .eventParameter = eventP,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}

static size_t UT_CM_DynChanGetParamResultListSize(void)
{
    return g_testDynChannelParamResultList.size();
}

static size_t UT_CM_DynChanGetParamResultListSizeByLcid(uint32_t lcid)
{
    size_t count = 0;
    for (auto item : g_testDynChannelParamResultList) {
        if (item.lcid == lcid) {
            count++;
        }
    }
    return count;
}

class UT_CM_DYN_TRANS_CHANN : public testing::Test {
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
        EXPECT_EQ(CM_Init(), CM_SUCCESS);
        EXPECT_EQ(UT_CM_DynChanGetParamResultListSize(), 0);
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        CM_LOGI("TearDownTestCase stub reset");
        g_testDynChannelParamResultList.clear();
        EXPECT_EQ(UT_CM_DynChanGetParamResultListSize(), 0);
        // Don't call STUB_Reset
        CM_DeInit();
        CM_LOGI("TearDownTestCase");
    }
};

static void UT_CM_ApiTestAcceptFilterList(SLE_Addr_S &addr)
{
    std::nullptr_t evt = nullptr;
    CM_ClearAcceptFilterList();
    UT_CM_SleCompleteEvt(DLI_CLEAR_ACCESS_FILTER_LIST, DLI_CBK_CLEAR_ACCESS_FLT_LIST, evt);
    EXPECT_EQ(CM_AddDeviceToAcceptFilterList(&addr, false), CM_SUCCESS);
    UT_CM_SleCompleteEvt(DLI_ADD_DEVICE_TO_ACCESS_FILTER_LIST, DLI_CBK_ADD_DEV_TO_ACCESS_FLT_LIST, evt);
}

static void UT_CM_DYN_TC_TestIdReset(void)
{
    g_testDynChannelParamResult.srcTcidResult = 0;
    g_testDynChannelParamResult.srcPortResult = 0;
    g_testDynChannelParamResult.dstPortResult = 0;
    g_testDynChannelParamResult.dstTcidResult = 0;
    (void)memset_s(&g_testDynChannelParamResult.sleAddr, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
}

static void UT_CM_DYN_TC_TestIdIsReset(void)
{
    EXPECT_EQ(g_testDynChannelParamResult.srcTcidResult, 0);
    EXPECT_EQ(g_testDynChannelParamResult.srcPortResult, 0);
    EXPECT_EQ(g_testDynChannelParamResult.dstPortResult, 0);
    EXPECT_EQ(g_testDynChannelParamResult.dstTcidResult, 0);
    EXPECT_EQ(SDF_CompareSleAddr(&g_emptyTestAddr, &g_testDynChannelParamResult.sleAddr), 0);
}

// 测试场景：被动创建传输通道
static void UT_CM_DYN_TC_TestEstablishAndReleaseActiveEstablished(uint16_t lcid,
    CM_DynTransChannelEstablishParamReq_S &reqParam)
{
    uint32_t ret = CM_DynTransChannelEstablishReq(&reqParam);
    EXPECT_EQ(ret, CM_SUCCESS);
    CM_SignalingTransChanEstablishRsp_S establishRsp = { 0 };
    establishRsp.srcTcid = UT_CM_SRC_TCID;
    establishRsp.dstTcid = UT_CM_DST_TCID;
    establishRsp.result = CM_RESULT_ESTABLISH_SUCCESS;
    CM_GetSignalingTransChanCbks()->establishRspCbk(lcid, &establishRsp);
    EXPECT_EQ(g_testDynChannelParamResult.srcTcidResult, establishRsp.srcTcid);
    EXPECT_EQ(g_testDynChannelParamResult.dstTcidResult, establishRsp.dstTcid);
    EXPECT_EQ(g_testDynChannelParamResult.srcPortResult, UT_CM_SRC_PORT_0);
    EXPECT_EQ(g_testDynChannelParamResult.dstPortResult, UT_CM_DST_PORT_0);
    EXPECT_EQ(SDF_CompareSleAddr(&reqParam.addr, &g_testDynChannelParamResult.sleAddr), 0);
    EXPECT_EQ(g_testDynChannelParamResult.slqiResult, UT_CM_SQLI);
}

// 测试场景：被动创建传输通道
static void UT_CM_DYN_TC_TestEstablishAndReleaseActiveReleased(uint16_t lcid,
    CM_DynTransChannelEstablishParamReq_S &reqParam, SLE_Addr_S& addr)
{
    CM_DynTransChannelReleaseParamReq_S releaseParam = { 0 };
    releaseParam.version = CM_CONNECT_VERSION_1_0;
    releaseParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    releaseParam.srcTcid = UT_CM_SRC_TCID;
    (void)memcpy_s(&releaseParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    releaseParam.srcTcid = UT_CM_SRC_TCID;
    uint32_t ret = CM_DynTransChannelReleaseReq(&releaseParam);
    EXPECT_EQ(ret, CM_SUCCESS);
    CM_SignalingTransChanReleaseRsp_S releaseRsp = { 0 };
    releaseRsp.srcTcid = releaseParam.srcTcid;
    // 主动创建传输通道标识
    releaseRsp.dstTcid = g_testDynChannelParamResult.dstTcidResult;
    releaseRsp.result = CM_RESULT_ESTABLISH_SUCCESS;
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 2 releaseRspCbk");
    UT_CM_DYN_TC_TestIdReset();
    CM_GetSignalingTransChanCbks()->releaseRspCbk(lcid, &releaseRsp);
    EXPECT_EQ(g_testDynChannelParamResult.srcTcidResult, releaseParam.srcTcid);
    EXPECT_EQ(g_testDynChannelParamResult.dstTcidResult, releaseRsp.dstTcid);
    EXPECT_EQ(g_testDynChannelParamResult.srcPortResult, UT_CM_SRC_PORT_0);
    EXPECT_EQ(g_testDynChannelParamResult.dstPortResult, UT_CM_DST_PORT_0);
    EXPECT_EQ(SDF_CompareSleAddr(&reqParam.addr, &g_testDynChannelParamResult.sleAddr), 0);
    EXPECT_EQ(g_testDynChannelParamResult.slqiResult, UT_CM_SQLI);
}

// 测试场景：被动创建传输通道后，主动释放失败
static void UT_CM_DYN_TC_TestEstablishAndReleaseActiveReleaseFailed(uint16_t lcid,
    CM_DynTransChannelEstablishParamReq_S &reqParam, SLE_Addr_S& addr)
{
    CM_DynTransChannelReleaseParamReq_S releaseParam = { 0 };
    releaseParam.version = CM_CONNECT_VERSION_1_0;
    releaseParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    releaseParam.srcTcid = UT_CM_SRC_TCID;
    (void)memcpy_s(&releaseParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    releaseParam.srcTcid = UT_CM_SRC_TCID;
    uint32_t ret = CM_DynTransChannelReleaseReq(&releaseParam);
    EXPECT_EQ(ret, CM_FAIL);
}

// 测试场景：被动创建传输通道，释放通道超时
static void UT_CM_DYN_TC_TestEstablishAndReleaseActiveReleasedTimeout(uint16_t lcid,
    CM_DynTransChannelEstablishParamReq_S &reqParam, SLE_Addr_S& addr)
{
    CM_DynTransChannelReleaseParamReq_S releaseParam = { 0 };
    releaseParam.version = CM_CONNECT_VERSION_1_0;
    releaseParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    releaseParam.srcTcid = UT_CM_SRC_TCID;
    (void)memcpy_s(&releaseParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    releaseParam.srcTcid = UT_CM_SRC_TCID;
    uint32_t ret = CM_DynTransChannelReleaseReq(&releaseParam);
    EXPECT_EQ(ret, CM_SUCCESS);
    CM_SignalingTransChanReleaseRsp_S releaseRsp = { 0 };
    releaseRsp.srcTcid = releaseParam.srcTcid;
    // 主动创建传输通道标识
    releaseRsp.dstTcid = g_testDynChannelParamResult.dstTcidResult;
    releaseRsp.result = CM_RESULT_ESTABLISH_TIMEOUT;
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 2 releaseRspCbk");
    UT_CM_DYN_TC_TestIdReset();
    CM_GetSignalingTransChanCbks()->releaseRspCbk(lcid, &releaseRsp);
    EXPECT_EQ(g_testDynChannelParamResult.srcTcidResult, releaseParam.srcTcid);
    EXPECT_EQ(g_testDynChannelParamResult.dstTcidResult, releaseRsp.dstTcid);
    EXPECT_EQ(g_testDynChannelParamResult.srcPortResult, UT_CM_SRC_PORT_0);
    EXPECT_EQ(g_testDynChannelParamResult.dstPortResult, UT_CM_DST_PORT_0);
    EXPECT_EQ(SDF_CompareSleAddr(&reqParam.addr, &g_testDynChannelParamResult.sleAddr), 0);
    EXPECT_EQ(g_testDynChannelParamResult.slqiResult, UT_CM_SQLI);
}

// 测试场景：被动创建传输通道
static void UT_CM_DYN_TC_TestEstablishAndReleaseActiveEstablishRspFailed(uint16_t lcid,
    CM_DynTransChannelEstablishParamReq_S &reqParam, SLE_Addr_S& addr, uint8_t failedResult)
{
    UT_CM_DYN_TC_TestIdReset();
    reqParam.version = CM_CONNECT_VERSION_1_0;
    reqParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    reqParam.expectedTransportMode = CM_ACCESS_TRANS_MODE_UNICAST;
    reqParam.srcPort = UT_CM_SRC_PORT_0;
    reqParam.dstPort = UT_CM_DST_PORT_0;
    (void)memcpy_s(&reqParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    uint32_t ret = CM_DynTransChannelEstablishReq(&reqParam);
    EXPECT_EQ(ret, CM_SUCCESS);
    CM_SignalingTransChanEstablishRsp_S timeoutEstablishRsp = { 0 };
    timeoutEstablishRsp.srcTcid = UT_CM_SRC_TCID;
    timeoutEstablishRsp.dstTcid = UT_CM_INVALID_TCID;
    timeoutEstablishRsp.result = failedResult;
    CM_GetSignalingTransChanCbks()->establishRspCbk(lcid, &timeoutEstablishRsp);
    EXPECT_EQ(g_testDynChannelParamResult.srcPortResult, UT_CM_SRC_PORT_0);
    EXPECT_EQ(g_testDynChannelParamResult.dstPortResult, UT_CM_DST_PORT_0);
    EXPECT_EQ(g_testDynChannelParamResult.slqiResult, UT_CM_SQLI);
    EXPECT_EQ(SDF_CompareSleAddr(&reqParam.addr, &g_testDynChannelParamResult.sleAddr), 0);
}

// 测试场景：被动创建传输通道
static void UT_CM_DYN_TC_TestEstablishAndReleasePassiveEstablished(uint16_t lcid,
    CM_DynTransChannelEstablishParamReq_S &reqParam)
{
    uint8_t reqId = UT_CM_DYN_TC_ACTIVE_REQ_ID;
    CM_SignalingTransChanEstablishReq_S establishReq = { 0 };
    CM_SignalingPortConfig_S portConfig = { .srcPort = UT_CM_DST_PORT_0, .dstPort = UT_CM_SRC_PORT_0,
        .aid = UT_CM_AID };
    establishReq.extension.portConfig = &portConfig;
    UT_CM_DYN_TC_TestIdReset();
    // 场景3.1 正常测试：被动创建传输通道
    establishReq.srcTcid = UT_CM_DST_TCID;
    establishReq.transModeConfig.commonConfig.mtu = UT_CM_MTU;
    CM_GetSignalingTransChanCbks()->establishReqCbk(lcid, reqId, &establishReq);
    EXPECT_EQ(g_testDynChannelParamResult.srcTcidResult, UT_CM_SRC_TCID); // 前面用例已归还srcTcid,
                                                                          // 本端继续从UT_CM_SRC_TCID开始分配
    EXPECT_EQ(g_testDynChannelParamResult.dstTcidResult, establishReq.srcTcid);
    EXPECT_EQ(g_testDynChannelParamResult.srcPortResult, UT_CM_SRC_PORT_0);
    EXPECT_EQ(g_testDynChannelParamResult.dstPortResult, UT_CM_DST_PORT_0);
    EXPECT_TRUE(g_testTransChanAdded);
    EXPECT_EQ(SDF_CompareSleAddr(&reqParam.addr, &g_testDynChannelParamResult.sleAddr), 0);
}

// 测试场景：被动创建传输通道
static void UT_CM_DYN_TC_TestEstablishAndReleasePassiveEstablishedReqCheckFailed(uint16_t lcid,
    CM_DynTransChannelEstablishParamReq_S &reqParam)
{
    uint8_t reqId = UT_CM_DYN_TC_ACTIVE_REQ_ID;
    CM_SignalingTransChanEstablishReq_S establishReq = { 0 };
    CM_SignalingPortConfig_S portConfig = { .srcPort = UT_CM_DST_PORT_0, .dstPort = UT_CM_SRC_PORT_0,
        .aid = UT_CM_AID };
    establishReq.extension.portConfig = &portConfig;
    UT_CM_DYN_TC_TestIdReset();
    // 设置检查本端tcid即DestTcid为无效
    CM_SetSignalingTransChanDestTcidInvalid(true);
    CM_SetSignalingTransChanEstablishRspSendResult(CM_RESULT_INSUFFICIENT_RESOURCE);
    // 场景1.1 正常测试：被动创建传输通道
    establishReq.srcTcid = UT_CM_DST_TCID;
    establishReq.transModeConfig.commonConfig.mtu = UT_CM_MTU;
    CM_GetSignalingTransChanCbks()->establishReqCbk(lcid, reqId, &establishReq);
    EXPECT_EQ(g_testDynChannelParamResult.srcTcidResult, 0); // 前面用例已归还srcTcid,
                                                             // 本端继续从UT_CM_SRC_TCID开始分配
    EXPECT_EQ(g_testDynChannelParamResult.dstTcidResult, 0);
    EXPECT_EQ(g_testDynChannelParamResult.srcPortResult, 0);
    EXPECT_EQ(g_testDynChannelParamResult.dstPortResult, 0);
    EXPECT_FALSE(g_testTransChanAdded);
    EXPECT_EQ(SDF_CompareSleAddr(&g_emptyTestAddr, &g_testDynChannelParamResult.sleAddr), 0);
    // 恢复检查设置
    CM_SetSignalingTransChanDestTcidInvalid(false);
    CM_SetSignalingTransChanEstablishRspSendResult(CM_RESULT_ESTABLISH_SUCCESS);
}

// 测试场景：被动创建传输通道失败
static void UT_CM_DYN_TC_TestEstablishAndReleasePassiveEstablishFailed(uint16_t lcid,
    CM_DynTransChannelEstablishParamReq_S &reqParam)
{
    uint8_t reqId = UT_CM_DYN_TC_ACTIVE_REQ_ID;
    CM_SignalingTransChanEstablishReq_S establishReq = { 0 };
    CM_SignalingPortConfig_S portConfig = { .srcPort = UT_CM_DST_PORT_0, .dstPort = UT_CM_SRC_PORT_0,
        .aid = UT_CM_AID };
    establishReq.extension.portConfig = &portConfig;
    UT_CM_DYN_TC_TestIdReset();
    // 场景3.1 异常测试：被动创建传输通道失败
    establishReq.srcTcid = UT_CM_DST_TCID;
    establishReq.transModeConfig.commonConfig.mtu = UT_CM_MTU;
    CM_GetSignalingTransChanCbks()->establishReqCbk(lcid, reqId, &establishReq);
}

// 测试场景：被动释放传输通道
static void UT_CM_DYN_TC_TestEstablishAndReleasePassiveReleased(uint16_t lcid,
    CM_DynTransChannelEstablishParamReq_S &reqParam)
{
    UT_CM_DYN_TC_TestIdReset();
    CM_SignalingTransChanReleaseReq_S releaseReq = { 0 };
    uint8_t reqId = UT_CM_DYN_TC_PASSIVE_REQ_ID;
    // 场景4.1 正常测试：被动创建传输通道
    UT_CM_DYN_TC_TestIdReset();
    releaseReq.srcTcid = UT_CM_DST_TCID; // 对端TCID
    releaseReq.dstTcid = UT_CM_SRC_TCID; // 本端TCID
    CM_GetSignalingTransChanCbks()->releaseReqCbk(lcid, reqId, &releaseReq);
    EXPECT_EQ(g_testDynChannelParamResult.srcTcidResult, releaseReq.dstTcid); // 需要回调转换TCID
    EXPECT_EQ(g_testDynChannelParamResult.dstTcidResult, releaseReq.srcTcid);
    EXPECT_EQ(g_testDynChannelParamResult.srcPortResult, UT_CM_SRC_PORT_0);
    EXPECT_EQ(g_testDynChannelParamResult.dstPortResult, UT_CM_DST_PORT_0);
    EXPECT_TRUE(!g_testTransChanAdded);
    EXPECT_EQ(SDF_CompareSleAddr(&reqParam.addr, &g_testDynChannelParamResult.sleAddr), 0);
}

static void UT_CM_DYN_TC_TestEstablishAndRelease(uint16_t lcid, SLE_Addr_S& addr)
{
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case start");
    UT_CM_DYN_TC_TestIdReset();
    g_testDynChannelStateChangeSize = UT_CM_DYN_TC_ONE_SIZE;
    CM_DynTransChannelEstablishParamReq_S reqParam = { 0 };
    reqParam.version = CM_CONNECT_VERSION_1_0;
    reqParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    reqParam.expectedTransportMode = CM_ACCESS_TRANS_MODE_UNICAST;
    reqParam.srcPort = UT_CM_SRC_PORT_0;
    reqParam.dstPort = UT_CM_DST_PORT_0;
    reqParam.slqi = UT_CM_SQLI;
    (void)memcpy_s(&reqParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 1 start");
    // 场景1：主动创建传输通道和响应成功测试
    UT_CM_DYN_TC_TestEstablishAndReleaseActiveEstablished(lcid, reqParam);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 1);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 1 end");
    // 场景2：主动释放传输通道和响应成功测试
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 2 start");
    UT_CM_DYN_TC_TestEstablishAndReleaseActiveReleased(lcid, reqParam, addr);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 0);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 2 end");
    // 场景3：被动创建传输通道
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 3 start");
    UT_CM_DYN_TC_TestEstablishAndReleasePassiveEstablished(lcid, reqParam);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 1);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 3 end");
    // 场景4：被动释放传输通道
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 4 start");
    UT_CM_DYN_TC_TestEstablishAndReleasePassiveReleased(lcid, reqParam);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 0);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 4 end");
    // 场景5：主动创建传输通道和请求响应超时测试
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 5 start");
    UT_CM_DYN_TC_TestEstablishAndReleaseActiveEstablishRspFailed(lcid, reqParam, addr, CM_RESULT_ESTABLISH_TIMEOUT);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 0);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 5 end");
    // 场景6：主动创建传输通道和释放请求响应超时测试
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 6 start");
    UT_CM_DYN_TC_TestEstablishAndReleaseActiveEstablished(lcid, reqParam);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 1);
    UT_CM_DYN_TC_TestEstablishAndReleaseActiveReleasedTimeout(lcid, reqParam, addr);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 0);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 6 end");

    // 场景13：主动创建传输通道，主动释放失败
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 13 start");
    CM_SetSignalingTransChanReleaseReqSend(true);
    UT_CM_DYN_TC_TestEstablishAndReleaseActiveEstablished(lcid, reqParam);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 1);
    UT_CM_DYN_TC_TestEstablishAndReleaseActiveReleaseFailed(lcid, reqParam, addr);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 1);
    CM_SetSignalingTransChanReleaseReqSend(false);
    // 再次正常释放
    UT_CM_DYN_TC_TestEstablishAndReleaseActiveReleased(lcid, reqParam, addr);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 13 end");

    // 场景14：被动创建传输通道失败
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 14 start");
    CM_SetSignalingTransChanEstablishRspSend(true);
    UT_CM_DYN_TC_TestEstablishAndReleasePassiveEstablishFailed(lcid, reqParam);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 0);
    CM_SetSignalingTransChanEstablishRspSend(false);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 14 end");

    // 场景15：主动创建传输通道请求发送失败
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 15 start");
    CM_SetSignalingTransChanEstablishReqSend(true);
    uint32_t ret = CM_DynTransChannelEstablishReq(&reqParam);
    EXPECT_EQ(ret, CM_FAIL);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 0);
    CM_SetSignalingTransChanEstablishReqSend(false);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 15 end");

    // 场景16：主动创建传输通道和被动释放传输通道响应发送失败
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 16 start");
    CM_SetSignalingTransChanReleaseRspSend(true);
    UT_CM_DYN_TC_TestEstablishAndReleaseActiveEstablished(lcid, reqParam);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 1);
    UT_CM_DYN_TC_TestEstablishAndReleasePassiveReleased(lcid, reqParam);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 0);
    CM_SetSignalingTransChanReleaseRspSend(false);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 16 end");
    UT_CM_DYN_TC_TestIdReset();
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case end");
}

static void UT_CM_DYN_TC_TestEstablishAndNoRelease(uint16_t lcid, SLE_Addr_S& addr)
{
    // 场景1：主动创建传输通道和不做主动或者被动测试，由链路断开来触发释放
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndNoRelease]case 1 start");
    CM_DynTransChannelEstablishParamReq_S reqParam = { 0 };
    reqParam.version = CM_CONNECT_VERSION_1_0;
    reqParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    reqParam.expectedTransportMode = CM_ACCESS_TRANS_MODE_UNICAST;
    reqParam.srcPort = UT_CM_SRC_PORT_0;
    reqParam.dstPort = UT_CM_DST_PORT_0;
    reqParam.slqi = UT_CM_SQLI;
    (void)memcpy_s(&reqParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    uint32_t ret = CM_DynTransChannelEstablishReq(&reqParam);
    EXPECT_EQ(ret, CM_SUCCESS);
    // 正常测试, 连接超时
    UT_CM_DYN_TC_TestIdReset();
    UT_CM_DYN_TC_TestIdIsReset();
    CM_SignalingTransChanEstablishRsp_S establishRsp = { 0 };
    establishRsp.result = CM_RESULT_ESTABLISH_TIMEOUT;
    establishRsp.srcTcid = UT_CM_SRC_TCID;
    establishRsp.dstTcid = 0;
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndNoRelease]case 4 establishRspCbk");
    CM_GetSignalingTransChanCbks()->establishRspCbk(lcid, &establishRsp);
    EXPECT_EQ(g_testDynChannelParamResult.srcTcidResult, establishRsp.srcTcid);
    EXPECT_EQ(g_testDynChannelParamResult.dstTcidResult, 0); // 超时后，该字段值为0，即无效
    EXPECT_EQ(g_testDynChannelParamResult.srcPortResult, UT_CM_SRC_PORT_0);
    EXPECT_EQ(g_testDynChannelParamResult.dstPortResult, UT_CM_DST_PORT_0);
    EXPECT_EQ(SDF_CompareSleAddr(&reqParam.addr, &g_testDynChannelParamResult.sleAddr), 0);
    EXPECT_EQ(g_testDynChannelParamResult.slqiResult, UT_CM_SQLI);

    // 正常测试：连接超时时, 会删除通道节点, 需要再次发起
    ret = CM_DynTransChannelEstablishReq(&reqParam);
    EXPECT_EQ(ret, CM_SUCCESS);
    establishRsp.result = CM_RESULT_ESTABLISH_SUCCESS;
    establishRsp.srcTcid = UT_CM_SRC_TCID;
    establishRsp.dstTcid = UT_CM_DST_TCID;
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndNoRelease]case 5 establishRspCbk");
    CM_GetSignalingTransChanCbks()->establishRspCbk(lcid, &establishRsp);
    EXPECT_EQ(g_testDynChannelParamResult.srcTcidResult, establishRsp.srcTcid);
    EXPECT_EQ(g_testDynChannelParamResult.dstTcidResult, establishRsp.dstTcid);
    EXPECT_EQ(g_testDynChannelParamResult.srcPortResult, UT_CM_SRC_PORT_0);
    EXPECT_EQ(g_testDynChannelParamResult.dstPortResult, UT_CM_DST_PORT_0);
    EXPECT_EQ(SDF_CompareSleAddr(&reqParam.addr, &g_testDynChannelParamResult.sleAddr), 0);
    EXPECT_EQ(g_testDynChannelParamResult.slqiResult, UT_CM_SQLI);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndNoRelease]case 1 end");
}

static void UT_CM_DYN_TC_TestConnect(uint16_t handle)
{
    CM_LOGI("============ UT_CM_DYN_TC_TestConnect start, handle:%u", handle);

    CM_ConnectSetParamReq_S setParam = { 0 };
    // 合法参数
    setParam.enableFilterPolicy = true;
    setParam.scanInterval = SLE_SCAN_INTERVAL;
    setParam.scanWindow = SLE_SCAN_WINDOW;
    setParam.initiatingPhys = SLE_INITIATE_PHYS;
    setParam.gtNegotiate = SLE_NEGOTIATE;
    setParam.minInterval = SLE_CONNECTION_INTERVAL_MIN;
    setParam.maxInterval = SLE_CONNECTION_INTERVAL_MAX;
    setParam.timeout = SLE_SUPERVISION_TIMEOUT;

    EXPECT_EQ(CM_ConnectSetParamReq(&setParam), CM_SUCCESS);
    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    CM_ConnectParamReq_S connParam = { 0 };
    connParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    connParam.version = CM_CONNECT_VERSION_1_0;
    (void)memcpy_s(&connParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    UT_CM_ApiTestAcceptFilterList(addr);
    EXPECT_EQ(CM_ConnectEstablishReq(&connParam), CM_SUCCESS);
    UT_CM_TestNormalReset();
    UT_CM_SleConnectCompleteEvt(handle, DLI_SUCCESS, 0);
    CM_LOGI("============ UT_CM_DYN_TC_TestConnect end, handle:%u", handle);
}

static void CM_ApiTestDisconnect(uint16_t handle)
{
    CM_LOGI("============ CM_ApiTestDisconnect start, handle:%u", handle);
    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    CM_DisconnectParamReq_S disParam = {};
    (void)memcpy_s(&disParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    disParam.discReason = CM_DISC_REASON_REMOTE_USER_TERMINATED;
    EXPECT_EQ(CM_ConnectReleaseReq(&disParam), CM_SUCCESS);
    CM_LOGI("============ CM_ApiTestDisconnect end, handle:%u", handle);
}

static void CM_ApiTestCheckLogicLinkAndTransChannel(uint16_t handle)
{
    CM_LogicLink_S logicLink = { 0 };
    EXPECT_EQ(CM_GetLogicLinkByLcid(handle, &logicLink), CM_SUCCESS);
    EXPECT_EQ(logicLink.role, UT_CM_GetTestNodeRole());
}

static void UT_CM_DYN_TC_TestConnectAndDisconnect(uint16_t testCount)
{
    // 创建链路
    uint16_t handle = g_passivehandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        handle = handle + i;
        UT_CM_DYN_TC_TestConnect(handle);
    }

    handle = g_passivehandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        handle = handle + i;
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        UT_CM_DYN_TC_TestEstablishAndRelease(handle, addr);
        UT_CM_DYN_TC_TestEstablishAndNoRelease(handle, addr);
    }

    UT_CM_TestNormalReset();

    handle = g_passivehandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        CM_ApiTestCheckLogicLinkAndTransChannel(handle);
        DLI_RemoteConnParamReqEvt revt = { 0 };
        revt.connHandle = handle;
        revt.connIntervalMin = UT_CM_CONN_INTERAL;
        revt.connIntervalMax = UT_CM_CONN_INTERAL;
        revt.maxLatency = UT_CM_CONN_MAX_LATENCY;
        revt.supervisionTimeout = UT_CM_CONN_SUPERVISION_TIMEOUT;
        UT_CM_SleCompleteEvt(DLI_REMOTE_CONNECTION_PARAMETER_REQUEST_EVT, DLI_CBK_REMOTE_CONNECT_PARAM_REQ, revt);
        EXPECT_EQ(CM_SetLogicLinkDeviceType(handle, CM_DEVTYPE_OLD), CM_SUCCESS);
        CM_LogicLink_S logicLink = { 0 };
        EXPECT_EQ(CM_GetLogicLinkByLcid(handle, &logicLink), CM_SUCCESS);
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        EXPECT_EQ(CM_GetLogicLinkByAddr(&addr, &logicLink), CM_SUCCESS);
    }

    // 断开链路
    handle = g_passivehandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        CM_ApiTestDisconnect(handle + i);
    }
}

static void UT_CM_RegConnectCbks(void)
{
    CM_ConnectCbks_S cbks = { 0 };
    cbks.connCancelCbk = UT_CM_ConnectCancelCbk;
    cbks.readRemoteFeatureVersionCbk = UT_CM_SsapConnectReadRemoteFetureVersionCbk;
    cbks.connUpdateParamCbk = UT_CM_SsapConnectUpdatePramCbk;
    cbks.connRemoteUpdateParamReqCbk = UT_CM_ConnectRemoteUpdateParamReqCbk;
    cbks.setPhyCbk = UT_CM_SetPhyCbk;
    EXPECT_EQ(CM_RegConnectCbks(&cbks), CM_SUCCESS);

    CM_LogicLinkCbks_S cmCbk = {};
    cmCbk.moduleId = CM_MODULE_ADPT;
    cmCbk.logicLinkCbk = UT_CM_ServiceConnectStateCbk;
    CM_RegLogicLinkListener(&cmCbk);
}

static void UT_CM_UnRegConnectCbks(void)
{
    CM_UnRegConnectCbks();
}

void UT_CM_DtapTransChannelStateCbk(CM_TransChannelStateList_S *param)
{
    if (param == NULL || param->channelVector == NULL) {
        return;
    }
    CM_LOGI("UT_CM_DtapTransChannelStateCbk state:%d, channel size:%u, connectState:0x%02x",
        param->result, param->channelVector->size, g_testDynChannelStateChangeSize);
    CM_TransChan_S *state = (CM_TransChan_S *)SDF_VectorElementAt(param->channelVector, 0);
    EXPECT_EQ(state != NULL, true);
    if (state->srcTcid == CM_TCID_SLE_CMTC) {
        // 固定传输通道创建或者释放通知
        EXPECT_EQ(param->channelVector->size, UT_CM_GetTestFixedTransChannelSize());
    } else {
        // 动态传输通道创建或者释放通知
        EXPECT_EQ(param->channelVector->size, g_testDynChannelStateChangeSize);
    }
    for (size_t i = 0; i < param->channelVector->size; i++) {
        CM_TransChan_S *state = (CM_TransChan_S *)SDF_VectorElementAt(param->channelVector, i);
        EXPECT_EQ(state != NULL, true);
        CM_LOGI("UT_CM_DtapTransChannelStateCbk, lcid:0x%04x, srcTcid:0x%02x, dstTcid:0x%02x",
            state->lcid, state->srcTcid, state->dstTcid);
    }
}

static void UT_CM_RegTransChannelListener(void)
{
    CM_TransChannelCbk cbk = UT_CM_DtapTransChannelStateCbk;
    EXPECT_EQ(CM_RegTransChannelListener(cbk), CM_SUCCESS);
}

template <typename T>
static void UT_CM_SetDynChannelParamResult(const T *param)
{
    g_testDynChannelParamResult.lcid = param->lcid;
    g_testDynChannelParamResult.srcTcidResult = param->srcTcid;
    g_testDynChannelParamResult.dstTcidResult = param->dstTcid;
    g_testDynChannelParamResult.srcPortResult = param->srcPort;
    g_testDynChannelParamResult.dstPortResult = param->dstPort;
    g_testDynChannelParamResult.slqiResult = param->slqiList.slqi[0];
    (void)memcpy_s(&g_testDynChannelParamResult.sleAddr, sizeof(SLE_Addr_S), &param->addr, sizeof(SLE_Addr_S));
    g_testDynChannelParamResult.slqiResult = param->slqiList.slqi[0];
}

static bool UT_CM_DynTransChannIsSame(const CM_TestDynChannelParamResult& p)
{
    if ((p.lcid == g_testDynChannelParamResult.lcid) &&
        (p.srcTcidResult == g_testDynChannelParamResult.srcTcidResult) &&
        (p.dstTcidResult == g_testDynChannelParamResult.dstTcidResult) &&
        (p.srcPortResult == g_testDynChannelParamResult.srcPortResult) &&
        (p.dstPortResult == g_testDynChannelParamResult.dstPortResult) &&
        (memcmp(&p.sleAddr, &g_testDynChannelParamResult.sleAddr, sizeof(p.sleAddr)) == 0)) {
        return true;
    }
    return false;
}

static void UT_CM_DynTransChannEstablishRspCbk(const CM_DynTransChanEstablishParamRsp_S *param)
{
    EXPECT_TRUE(param != NULL);
    CM_LOGI("UT_CM_DynTransChannEstablishRspCbk lcid: 0x%04x, srcTcid: 0x%02x, dstTcid: 0x%02x, srcPort: 0x%04x, "
        "dstPort: 0x%04x, result:0x%02x, slqi:%hhu",
        param->lcid, param->srcTcid, param->dstTcid, param->srcPort, param->dstPort, param->result,
        param->slqiList.slqi[0]);
    EXPECT_EQ(param->srcTcid, UT_CM_SRC_TCID);
    EXPECT_EQ(param->slqiList.slqiNum, UT_CM_TRANS_CHAN_SLQI_SURPPORT_NUM);
    g_testDynChannelParamResult.slqiResult = param->slqiList.slqi[0];
    UT_CM_SetDynChannelParamResult(param);
    if (param->result == CM_DYN_TRANS_CHAN_ESTABLISH_SUCCESS) {
        CM_LOGI("dyn channel list add a dyn channel, tcid:0x%02x", param->srcTcid);
        g_testDynChannelParamResultList.push_back(g_testDynChannelParamResult);
    }
    CM_LOGI("dyn channel list size:%zu", g_testDynChannelParamResultList.size());
}

static void UT_CM_DynTransChannReleaseRspCbk(const CM_DynTransChanReleaseParamRsp_S *param)
{
    EXPECT_TRUE(param != NULL);
    CM_LOGI("UT_CM_DynTransChannReleaseRspCbk lcid: 0x%04x, srcTcid: 0x%02x, dstTcid: 0x%02x, srcPort: 0x%04x"
        ", dstPort: 0x%04x", param->lcid, param->srcTcid, param->dstTcid, param->srcPort, param->dstPort);
    EXPECT_EQ(param->srcTcid, UT_CM_SRC_TCID);
    g_testDynChannelParamResult.srcTcidResult = param->srcTcid;
    g_testDynChannelParamResult.dstTcidResult = param->dstTcid;
    g_testDynChannelParamResult.srcPortResult = param->srcPort;
    g_testDynChannelParamResult.dstPortResult = param->dstPort;
    (void)memcpy_s(&g_testDynChannelParamResult.sleAddr, sizeof(SLE_Addr_S), &param->addr, sizeof(SLE_Addr_S));
    UT_CM_SetDynChannelParamResult(param);
    CM_LOGI("dyn channel list remove a dyn channel, tcid:0x%02x", param->srcTcid);
    g_testDynChannelParamResultList.erase(std::remove_if(g_testDynChannelParamResultList.begin(),
        g_testDynChannelParamResultList.end(), UT_CM_DynTransChannIsSame), g_testDynChannelParamResultList.end());
    CM_LOGI("dyn channel list size:%zu", g_testDynChannelParamResultList.size());
}

static void UT_CM_DynTransChannStatusIndicationCbk(const CM_DynTransChanStatusIndicationRsp_S *param)
{
    EXPECT_TRUE(param != NULL);
    CM_LOGI("UT_CM_DynTransChannStatusIndicationCbk lcid: 0x%04x, srcTcid: 0x%02x, dstTcid: 0x%02x, srcPort: 0x%04x"
        ", dstPort: 0x%04x", param->lcid, param->srcTcid, param->dstTcid, param->srcPort, param->dstPort);
    g_testDynChannelParamResult.srcTcidResult = param->srcTcid;
    g_testDynChannelParamResult.dstTcidResult = param->dstTcid;
    g_testDynChannelParamResult.srcPortResult = param->srcPort;
    g_testDynChannelParamResult.dstPortResult = param->dstPort;
    g_testTransChanAdded = param->added;
    (void)memcpy_s(&g_testDynChannelParamResult.sleAddr, sizeof(SLE_Addr_S), &param->addr, sizeof(SLE_Addr_S));
    UT_CM_SetDynChannelParamResult(param);
    if (param->added) {
        CM_LOGI("dyn channel list add a dyn channel, tcid:0x%02x", param->srcTcid);
        g_testDynChannelParamResultList.push_back(g_testDynChannelParamResult);
    } else {
        CM_LOGI("dyn channel list remove a dyn channel, tcid:0x%02x", param->srcTcid);
        g_testDynChannelParamResultList.erase(std::remove_if(g_testDynChannelParamResultList.begin(),
        g_testDynChannelParamResultList.end(), UT_CM_DynTransChannIsSame), g_testDynChannelParamResultList.end());
    }
    CM_LOGI("dyn channel list size:%zu", g_testDynChannelParamResultList.size());
}

TEST_F(UT_CM_DYN_TRANS_CHANN, CM_ApiConnectAndDisconnectTest)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    UT_CM_RegConnectCbks();
    CM_DynTransChannelCbks_S cbks = { 0 };
    cbks.establishRspCbk = UT_CM_DynTransChannEstablishRspCbk;
    cbks.releaseRspCbk = UT_CM_DynTransChannReleaseRspCbk;
    cbks.statusIndicationCbk = UT_CM_DynTransChannStatusIndicationCbk;
    CM_RegDynTransChannelCbks(&cbks);
    UT_CM_RegTransChannelListener();
    const uint16_t testCount = 1;
    UT_CM_DYN_TC_TestConnectAndDisconnect(testCount);
    CM_UnRegTransChannelListener();
    CM_UnRegDynTransChannelCbks();
    UT_CM_UnRegConnectCbks();
    CM_DeInit();
}

static bool UT_CM_DynTransChannEstablishedCheckSuccess(const CM_DynTransChanEstablishedCheckParam_S *param)
{
    EXPECT_TRUE(param != NULL);
    CM_LOGI("UT_CM_DynTransChannEstablishedCheckCbk srcPort: 0x%04x, dstPort: 0x%04x", param->srcPort, param->dstPort);
    return true;
}

TEST_F(UT_CM_DYN_TRANS_CHANN, CM_ApiConnectAndDisconnectTestEstablishedCheckSuccess)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    UT_CM_RegConnectCbks();
    CM_DynTransChannelCbks_S cbks = { 0 };
    cbks.establishRspCbk = UT_CM_DynTransChannEstablishRspCbk;
    cbks.releaseRspCbk = UT_CM_DynTransChannReleaseRspCbk;
    cbks.statusIndicationCbk = UT_CM_DynTransChannStatusIndicationCbk;
    cbks.establishedCheckCbk = UT_CM_DynTransChannEstablishedCheckSuccess,
    CM_RegDynTransChannelCbks(&cbks);
    UT_CM_RegTransChannelListener();
    const uint16_t testCount = 1;
    UT_CM_DYN_TC_TestConnectAndDisconnect(testCount);
    CM_UnRegTransChannelListener();
    CM_UnRegDynTransChannelCbks();
    UT_CM_UnRegConnectCbks();
    CM_DeInit();
}

static bool UT_CM_DynTransChannEstablishedCheckFailed(const CM_DynTransChanEstablishedCheckParam_S *param)
{
    EXPECT_TRUE(param != NULL);
    CM_LOGI("UT_CM_DynTransChannEstablishedCheckCbk srcPort: 0x%04x, dstPort: 0x%04x", param->srcPort, param->dstPort);
    return false;
}

static void UT_CM_DYN_TC_TestEstablishAndReleaseChannelReqCheckFailed(uint16_t lcid, SLE_Addr_S& addr)
{
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndReleaseChannelReqCheckFailed]case start");
    UT_CM_DYN_TC_TestIdReset();
    g_testDynChannelStateChangeSize = UT_CM_DYN_TC_ONE_SIZE;
    CM_DynTransChannelEstablishParamReq_S reqParam = { 0 };
    reqParam.version = CM_CONNECT_VERSION_1_0;
    reqParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    reqParam.expectedTransportMode = CM_ACCESS_TRANS_MODE_UNICAST;
    reqParam.srcPort = UT_CM_SRC_PORT_0;
    reqParam.dstPort = UT_CM_DST_PORT_0;
    reqParam.slqi = UT_CM_SQLI;
    (void)memcpy_s(&reqParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndReleaseChannelReqCheckFailed]case 1 start");
    // 场景1：被动创建传输通道
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 1 start");
    UT_CM_DYN_TC_TestEstablishAndReleasePassiveEstablishedReqCheckFailed(lcid, reqParam);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 0);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 1 end");

    // 场景2：主动创建传输通道和请求响应错误码为"INSUFFICIENT_RESOURCE"测试
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 2 start");
    UT_CM_DYN_TC_TestEstablishAndReleaseActiveEstablishRspFailed(lcid, reqParam, addr, CM_RESULT_INSUFFICIENT_RESOURCE);
    EXPECT_EQ(UT_CM_DynChanGetParamResultListSizeByLcid(lcid), 0);
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndRelease]case 2 end");

    UT_CM_DYN_TC_TestIdReset();
    CM_LOGI("[UT_CM_DYN_TC_TestEstablishAndReleaseChannelReqCheckFailed]case end");
}

static void UT_CM_DYN_TC_TestConnectAndDisconnectChannelReqCheckFailed(uint16_t testCount)
{
    // 创建链路
    uint16_t handle = g_passivehandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        handle = handle + i;
        UT_CM_DYN_TC_TestConnect(handle);
    }
    // 通道被动创建校验失败测试
    handle = g_passivehandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        handle = handle + i;
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        UT_CM_DYN_TC_TestEstablishAndReleaseChannelReqCheckFailed(handle, addr);
    }

    // 断开链路
    UT_CM_TestNormalReset();
    handle = g_passivehandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        CM_ApiTestDisconnect(handle + i);
    }
}

TEST_F(UT_CM_DYN_TRANS_CHANN, CM_ApiConnectAndDisconnectTestEstablishedCheckFailed)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    UT_CM_RegConnectCbks();
    CM_DynTransChannelCbks_S cbks = { 0 };
    cbks.establishRspCbk = UT_CM_DynTransChannEstablishRspCbk;
    cbks.releaseRspCbk = UT_CM_DynTransChannReleaseRspCbk;
    cbks.statusIndicationCbk = UT_CM_DynTransChannStatusIndicationCbk;
    cbks.establishedCheckCbk = UT_CM_DynTransChannEstablishedCheckFailed,
    CM_RegDynTransChannelCbks(&cbks);
    UT_CM_RegTransChannelListener();
    const uint16_t testCount = 1;
    UT_CM_DYN_TC_TestConnectAndDisconnectChannelReqCheckFailed(testCount);
    CM_UnRegTransChannelListener();
    CM_UnRegDynTransChannelCbks();
    UT_CM_UnRegConnectCbks();
    CM_DeInit();
}

TEST_F(UT_CM_DYN_TRANS_CHANN, CM_DynTransChannEstablishReqInvalidMtu)
{
    UT_CM_DYN_TC_TestIdReset();
    uint8_t reqId = UT_CM_DYN_TC_ACTIVE_REQ_ID;
    CM_SignalingTransChanEstablishReq_S establishReq = { 0 };
    establishReq.srcTcid = CM_TCID_BC_BEGIN;
    establishReq.transModeConfig.commonConfig.mtu = CM_CAP_MIN_MTU - 1;
    // 设置检查本端tcid即DestTcid为无效
    CM_SetSignalingTransChanDestTcidInvalid(true);
    CM_SetSignalingTransChanEstablishRspSendResult(CM_RESULT_UNSUPPORTED_MTU_SIZE);
    CM_GetSignalingTransChanCbks()->establishReqCbk(0, reqId, &establishReq);
    EXPECT_EQ(g_testDynChannelParamResult.srcTcidResult, 0);
    EXPECT_EQ(g_testDynChannelParamResult.dstTcidResult, 0);
    EXPECT_EQ(g_testDynChannelParamResult.srcPortResult, 0);
    EXPECT_EQ(g_testDynChannelParamResult.dstPortResult, 0);
    EXPECT_FALSE(g_testTransChanAdded);
    EXPECT_EQ(SDF_CompareSleAddr(&g_emptyTestAddr, &g_testDynChannelParamResult.sleAddr), 0);
    // 恢复检查设置
    CM_SetSignalingTransChanDestTcidInvalid(false);
    CM_SetSignalingTransChanEstablishRspSendResult(CM_RESULT_ESTABLISH_SUCCESS);
}