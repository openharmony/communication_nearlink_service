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

#include "cm_api.h"
#include "cm_inner_api.h"
#include "gtest/gtest.h"
#include "securec.h"
#include "sdf_worker.h"
#include "cp_worker.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "dli_opcode.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "dli_errno.h"
#include "dli.h"
#include "dli_cmd.h"
#include "sle_connect_param.h"
#include "sle_access_dli.h"
#include "cm_logic_link_api.h"
#include "cm_trans_channel_api.h"
#include "cm_util.h"
#include "cm_util_test.h"

namespace OHOS {
namespace Nearlink {
namespace TEST {
using namespace testing::ext;
using namespace OHOS::Nearlink;

/* 音频场景默认subrate */
#define UT_NLSTK_DEFAULT_SUBRATE 0x06

/* 帧4场景默认subrate值 */
#define UT_GLE_FRAME_TYPE_4_SUBRATE_DEFAULT 0x02
#define UT_GLE_FRAME_TYPE_4_MAX_LATENCY 0
#define UT_GLE_FRAME_TYPE_4_CONTINUATION_NUM 1
#define UT_GLE_FRAME_TYPE_4_SUBRATE_SUPERVISION_TIMEOUT 2000

static uint8_t g_testConnectState = 0;
static const uint16_t g_activeHandleInitValue = 0x2042;
static const uint16_t g_passiveHandleInitValue = 0x2047;

static void UT_CM_RegConnectCbks(void);
static void UT_CM_RegTransChannelListener(void);

template <typename T>
inline void UT_CM_SleCompleteEvt(uint16_t cmdOpcode, uint16_t cbkCmdOpcode, T &eventParameter,
    uint8_t status = DLI_SUCCESS)
{
    uint8_t size = (std::is_same<decltype(eventParameter), std::nullptr_t>::value) ? 0 : sizeof(T);
    void *eventP = (size == 0) ? NULL : &eventParameter;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = cbkCmdOpcode,
        .size = size,
        .eventParameter = eventP,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, status);
}

template <typename T>
inline void UT_CM_SetRemoteCbkEvt(uint16_t cbkCmdOpcode, T &eventParameter)
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

class UT_CM_API : public testing::Test {
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
        EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);
        CM_LOGI("TearDownTestCase");
        CM_DeInit();
    }
};

static void CM_ConnectSetNormalParam(CM_ConnectSetParamReq_S &setParam)
{
    // 合法参数
    setParam.enableFilterPolicy = true;
    setParam.scanInterval = SLE_SCAN_INTERVAL;
    setParam.scanWindow = SLE_SCAN_WINDOW;
    setParam.initiatingPhys = SLE_INITIATE_PHYS;
    setParam.gtNegotiate = SLE_NEGOTIATE;
    setParam.minInterval = SLE_CONNECTION_INTERVAL_MIN;
    setParam.maxInterval = SLE_CONNECTION_INTERVAL_MAX;
    setParam.timeout = SLE_SUPERVISION_TIMEOUT;
}

static void UT_CM_ApiTestConnectUpdateParam(uint16_t connHandle, CM_ConnectParamReq_S &connParam, SLE_Addr_S &addr)
{
    // 测试场景: 正常参数更新
    CM_LOGI("UT_CM_ApiTestConnectUpdateParam case1:");
    CM_ConnectUpdateParamReq_S param = { 0 };
    param.version = connParam.version;
    param.localIndex = connParam.localIndex;
    (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    param.intervalMin = UT_CM_CONN_INTERAL;
    param.intervalMax = UT_CM_CONN_INTERAL;
    param.txRxInterval = UT_CM_CONN_EVENT_IFS;
    param.eventInterval = UT_CM_CONN_EVENT_IFS;
    param.maxLatency = UT_CM_CONN_MAX_LATENCY;
    param.supervisionTimeout = UT_CM_CONN_SUPERVISION_TIMEOUT;
    param.systemTimeUnit = UT_CM_CONN_TIME_UNIT;
    param.txRxFlag = UT_CM_CONN_T_TX_RX_FLAG;
    int32_t ret = CM_ConnectUpdateParamReq(&param);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_ConnectUpdateParamReq failed, ret:0x%08x", ret);
        return;
    }

    // 测试场景: 地址不存在
    CM_LOGI("UT_CM_ApiTestConnectUpdateParam case2:");
    uint16_t tmpLcid = UT_CM_GetTestTcid();
    UT_CM_SetTestTcid(CM_INVALID_LCID);
    (void)memset_s(&param.addr, sizeof(SLE_Addr_S), 0x00, sizeof(SLE_Addr_S));
    ret = CM_ConnectUpdateParamReq(&param);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_ConnectUpdateParamReq failed, ret:0x%08x", ret);
        return;
    }
    EXPECT_EQ(UT_CM_GetTestTcid(), CM_INVALID_LCID);
    // 恢复地址
    UT_CM_SetTestTcid(tmpLcid);
    (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));

    // 测试场景: 回调返回状态值异常
    CM_LOGI("UT_CM_ApiTestConnectUpdateParam case3:");
    ret = CM_ConnectUpdateParamReq(&param);
    if (ret != CM_SUCCESS) {
        CM_LOGE("CM_ConnectUpdateParamReq failed, ret:0x%08x", ret);
        return;
    }
    DLI_ConnectionUpdateCmpEvt evt = { 0 };
    evt.connHandle = connHandle;
    evt.status = DLI_UNKNOWN_COMMAND;
    UT_CM_SleCompleteEvt(DLI_CONNECTION_UPDATE, DLI_CBK_CONNECT_UPDATE, evt, evt.status);
}

static void UT_CM_ApiTestConnectSetOtherParam(uint16_t handle)
{
    std::nullptr_t evt = nullptr;
    CM_SetPhyReq_S setPhyReqParam = { 0 };
    setPhyReqParam.lcid = handle;
    setPhyReqParam.txFormat = UT_CM_CONN_TX_FORMAT;
    EXPECT_EQ(CM_SetPhy(&setPhyReqParam), CM_SUCCESS);
    DLI_SetPhyEvt setPhyEvt = { 0 };
    setPhyEvt.connHandle = handle;
    setPhyEvt.txFormat = setPhyReqParam.txFormat;
    UT_CM_SetRemoteCbkEvt(DLI_CBK_SET_PHY, setPhyEvt);

    CM_SetChannelMapReq_S mapParam = { 0 };
    EXPECT_EQ(CM_SetHostChannelClassification(&mapParam), CM_SUCCESS);
    UT_CM_SleCompleteEvt(DLI_SET_HOST_CHANNEL_CLASSIFICATION, DLI_CBK_SET_HOST_CHANNEL_CLASSIFICATION, evt);
}

static void UT_CM_ApiTestAcceptFilterList(SLE_Addr_S &addr)
{
    std::nullptr_t evt = nullptr;
    CM_ReadAcceptFilterListSize();
    UT_CM_SleCompleteEvt(DLI_READ_ACCESS_FILTER_LIST_SIZE, DLI_CBK_READ_ACCESS_FLT_LIST_SIZE, evt);
    CM_ClearAcceptFilterList();
    UT_CM_SleCompleteEvt(DLI_CLEAR_ACCESS_FILTER_LIST, DLI_CBK_CLEAR_ACCESS_FLT_LIST, evt);
    EXPECT_EQ(CM_RemoveDeviceFromAcceptFilterList(&addr), CM_SUCCESS);
    UT_CM_SleCompleteEvt(DLI_REMOVE_DEVICE_FROM_ACCESS_FILTER_LIST, DLI_CBK_RMV_DEV_FROM_ACCESS_FLT_LIST, evt);
    EXPECT_EQ(CM_AddDeviceToAcceptFilterList(&addr, false), CM_SUCCESS);
    UT_CM_SleCompleteEvt(DLI_ADD_DEVICE_TO_ACCESS_FILTER_LIST, DLI_CBK_ADD_DEV_TO_ACCESS_FLT_LIST, evt);
}

static void UT_CM_DtapFixedTransChannelStateCbk(CM_TransChannelStateList_S *param)
{
    if (param == NULL || param->channelVector == NULL) {
        return;
    }
    CM_LOGI("UT_CM_DtapFixedTransChannelStateCbk state:%d, channel size:%u, connectState:0x%02x",
        param->result, param->channelVector->size, g_testConnectState);
    EXPECT_EQ(param->channelVector->size, UT_CM_GetTestFixedTransChannelSize());
    for (size_t i = 0; i < param->channelVector->size; i++) {
        CM_TransChan_S *state = (CM_TransChan_S *)SDF_VectorElementAt(param->channelVector, i);
        EXPECT_EQ(state != NULL, true);
        CM_LOGI("UT_CM_DtapFixedTransChannelStateCbk, lcid:0x%04x, srcTcid:0x%02x, dstTcid:0x%02x",
            state->lcid, state->srcTcid, state->dstTcid);
    }
}

static void UT_CM_ApiTestConnect(uint16_t handle)
{
    CM_LOGI("UT_CM_ApiTestConnect start, handle:%hu", handle);
    // 连接未发起，直接取消
    UT_CM_CancelConnectResult(DLI_UNKNOWN_CONNECTION_IDENTIFIER);
    std::nullptr_t evt = nullptr;
    CM_ConnectCancelReq();

    // 第一次发起连接
    CM_LOGI("UT_CM_ApiTestConnect first connect start");
    UT_CM_CancelConnectResult(DLI_SUCCESS);
    CM_ConnectSetParamReq_S setParam = { 0 };
    CM_ConnectSetNormalParam(setParam);

    EXPECT_EQ(CM_ConnectSetParamReq(&setParam), CM_SUCCESS);

    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    CM_ConnectParamReq_S connParam = { 0 };
    connParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    connParam.version = CM_CONNECT_VERSION_1_0;
    if (!setParam.enableFilterPolicy) {
        (void)memcpy_s(&connParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    }
    EXPECT_EQ(CM_ConnectEstablishReq(&connParam), CM_SUCCESS);
    CM_LOGI("UT_CM_ApiTestConnect first connect end");
    // 取消连接
    UT_CM_SleCompleteEvt(DLI_CREATE_CONNECTION_CANCEL, DLI_CBK_CONNECT_CANCEL, evt);
    UT_CM_TestNormalReset();
    UT_CM_SleConnectCompleteCommandErrorEvt(handle, DLI_COMMAND_TIMEOUT);
    UT_CM_ApiTestAcceptFilterList(addr);

    // 第二次连接成功
    CM_LOGI("UT_CM_ApiTestConnect second connect start");
    UT_CM_CancelConnectResult(DLI_SUCCESS);
    EXPECT_EQ(CM_ConnectEstablishReq(&connParam), CM_SUCCESS);
    if ((handle % 2) == 0) {
        // G/T节点交叉测试覆盖
        UT_CM_SetNodeRole(CM_G_NODE);
    } else {
        UT_CM_SetNodeRole(CM_T_NODE);
    }
    UT_CM_SleConnectCompleteEvt(handle, DLI_SUCCESS, CM_CONN_COMPLETE_SCAN);
    CM_LOGI("UT_CM_ApiTestConnect second connect success end");
    CM_LogicLinkState_S link = UT_CM_GetLogicLinkState(handle);
    EXPECT_EQ(link.lcid, handle);
    EXPECT_EQ(link.connCompleteType, CM_CONN_COMPLETE_SCAN);
    EXPECT_EQ(link.advHandle, UT_CM_CONN_ADV_HANDLE);
    int ret = SDF_CompareSleAddr(&link.addr, &addr);
    EXPECT_EQ(ret, 0);

    UT_CM_ApiTestConnectUpdateParam(handle, connParam, addr);
    UT_CM_ApiTestConnectSetOtherParam(handle);

    setParam.scanInterval = 0; //参数非法，由芯片做检测
    EXPECT_EQ(CM_ConnectSetParamReq(&setParam), CM_SUCCESS);
    UT_CM_SleConnectCompleteCommandErrorEvt(handle, DLI_INVALID_PARAMETERS);

    // 该地址建链完成后，再次添加到白名单列表中，日志将会告警提示，发起建链时，将会超时提示
    CM_LOGI("UT_CM_ApiTestConnect allow list connection repeate connect connected addr start");
    UT_CM_ApiTestAcceptFilterList(addr);
    CM_LOGI("UT_CM_ApiTestConnect allow list connection repeate connect connected addr end");
    EXPECT_EQ(CM_ConnectEstablishReq(&connParam), CM_SUCCESS);
    UT_CM_SleConnectCompleteCommandErrorEvt(handle, DLI_COMMAND_TIMEOUT);
    // 最终状态仍然为已经连接
    g_testConnectState = CM_LINK_STATE_CONNECTED;
    CM_LOGI("UT_CM_ApiTestConnect end");
}

static void UT_CM_ApiTestPassiveConnect(uint16_t handle)
{
    CM_LOGI("UT_CM_ApiTestPassiveConnect start, handle:%hu", handle);

    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    const uint8_t connCompleteType = CM_CONN_COMPLETE_ADV;
    UT_CM_SleConnectCompleteEvt(handle, DLI_SUCCESS, connCompleteType);
    CM_LogicLinkState_S link = UT_CM_GetLogicLinkState(handle);
    EXPECT_EQ(link.lcid, handle);
    EXPECT_EQ(link.connCompleteType, connCompleteType);
    EXPECT_EQ(link.advHandle, UT_CM_CONN_ADV_HANDLE);
    int ret = SDF_CompareSleAddr(&link.addr, &addr);
    EXPECT_EQ(ret, 0);
    // 最终状态仍然为已经连接
    g_testConnectState = CM_LINK_STATE_CONNECTED;
    CM_LOGI("UT_CM_ApiTestPassiveConnect end");
}

static void CM_ApiTestDisconnect(uint16_t handle)
{
    CM_LOGI("CM_ApiTestDisconnect start, handle:%hu", handle);
    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    CM_DisconnectParamReq_S disParam = {};
    (void)memcpy_s(&disParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    disParam.discReason = CM_DISC_REASON_REMOTE_USER_TERMINATED;
    EXPECT_EQ(CM_ConnectReleaseReq(&disParam), CM_SUCCESS);
    CM_LOGI("CM_ApiTestDisconnect end");
}

static void CM_ApiTestCheckLogicLinkAndTransChannel(uint16_t handle)
{
    CM_LogicLink_S logicLink = { 0 };
    EXPECT_EQ(CM_GetLogicLinkByLcid(handle, &logicLink), CM_SUCCESS);
    EXPECT_EQ(logicLink.role, UT_CM_GetTestNodeRole());
}

static void UT_CM_ApiTestConnectAndDisconnect(uint16_t testCount)
{
    CM_LOGI("UT_CM_ApiTestConnectAndDisconnect start");

    uint16_t handle = g_activeHandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        UT_CM_ApiTestConnect(handle + i);
    }
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), testCount);
    EXPECT_EQ(UT_CM_GetDtapTestConnectListSize(), testCount);
    CM_LogicLinkState_S dtapState = UT_CM_GetDtapTestConnectListFirst();
    EXPECT_EQ(dtapState.result, CM_LINK_STATE_CONNECTED);

    handle = g_activeHandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        CM_LOGI("UT_CM_ApiTestConnectAndDisconnect CM_ApiTestCheckLogicLinkAndTransChannel, i:%hu", i);
        CM_ApiTestCheckLogicLinkAndTransChannel(handle);
        DLI_RemoteConnParamReqEvt revt = { 0 };
        revt.connHandle = handle;
        revt.connIntervalMin = UT_CM_CONN_INTERAL;
        revt.connIntervalMax = UT_CM_CONN_INTERAL;
        revt.maxLatency = UT_CM_CONN_MAX_LATENCY;
        revt.supervisionTimeout = UT_CM_CONN_SUPERVISION_TIMEOUT;
        UT_CM_SleCompleteEvt(DLI_REMOTE_CONNECTION_PARAMETER_REQUEST_EVT, DLI_CBK_REMOTE_CONNECT_PARAM_REQ, revt);
        CM_TestDLI_UndefinedEvt uevt = { 0 };
        UT_CM_SleCompleteEvt(DLI_CMD_COMPLETE_EVT, DLI_CBK_REMOTE_CONNECT_PARAM_REQ_REPLY, uevt);
        EXPECT_EQ(CM_SetLogicLinkDeviceType(handle, CM_DEVTYPE_OLD), CM_SUCCESS);
        EXPECT_EQ(CM_SetLogicLinkDeviceType(handle, CM_DEVTIYPE_MAX), CM_INVALID_PARAM_ERR);
        CM_LogicLink_S logicLink = { 0 };
        EXPECT_EQ(CM_GetLogicLinkByLcid(handle, &logicLink), CM_SUCCESS);
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        EXPECT_EQ(CM_GetLogicLinkByAddr(&addr, &logicLink), CM_SUCCESS);
        // 连接完成后，不应该存在一条空地址
        CM_LOGI("UT_CM_ApiTestConnectAndDisconnect CM_GetLogicLinkByAddr");
        SLE_Addr_S emptyAddr = { 0 };
        EXPECT_EQ(CM_GetLogicLinkByAddr(&emptyAddr, &logicLink), CM_NOT_FOUND);
    }
    CM_LOGI("UT_CM_ApiTestConnectAndDisconnect CM_ApiTestDisconnect");
    handle = g_activeHandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        CM_ApiTestDisconnect(handle + i);
    }
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);
    EXPECT_EQ(UT_CM_GetDtapTestConnectListSize(), 0);

    // 断开完成后，也不应该存在一条空地址
    CM_LogicLink_S logicLink = { 0 };
    SLE_Addr_S emptyAddr = { 0 };
    EXPECT_EQ(CM_GetLogicLinkByAddr(&emptyAddr, &logicLink), CM_NOT_FOUND);
    UT_CM_TestNormalReset();
    CM_LOGI("UT_CM_ApiTestConnectAndDisconnect end");
}

static void UT_CM_ApiTestPassiveConnectedAndDisconnect(uint16_t testCount)
{
    CM_LOGI("UT_CM_ApiTestPassiveConnectedAndDisconnect start");
    uint16_t handle = g_passiveHandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        UT_CM_ApiTestPassiveConnect(handle + i);
    }
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), testCount);
    EXPECT_EQ(UT_CM_GetDtapTestConnectListSize(), testCount);
    CM_LogicLinkState_S dtapState = UT_CM_GetDtapTestConnectListFirst();
    EXPECT_EQ(dtapState.result, CM_LINK_STATE_CONNECTED);

    handle = g_passiveHandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        CM_LOGI("UT_CM_ApiTestConnectAndDisconnect CM_ApiTestCheckLogicLinkAndTransChannel, i:%hu", i);
        CM_ApiTestCheckLogicLinkAndTransChannel(handle);
        DLI_RemoteConnParamReqEvt revt = { 0 };
        revt.connHandle = handle;
        revt.connIntervalMin = UT_CM_CONN_INTERAL;
        revt.connIntervalMax = UT_CM_CONN_INTERAL;
        revt.maxLatency = UT_CM_CONN_MAX_LATENCY;
        revt.supervisionTimeout = UT_CM_CONN_SUPERVISION_TIMEOUT;
        UT_CM_SleCompleteEvt(DLI_REMOTE_CONNECTION_PARAMETER_REQUEST_EVT, DLI_CBK_REMOTE_CONNECT_PARAM_REQ, revt);
        EXPECT_EQ(CM_SetLogicLinkDeviceType(handle, CM_DEVTYPE_OLD), CM_SUCCESS);
        EXPECT_EQ(CM_SetLogicLinkDeviceType(handle, CM_DEVTIYPE_MAX), CM_INVALID_PARAM_ERR);
        CM_LogicLink_S logicLink = { 0 };
        EXPECT_EQ(CM_GetLogicLinkByLcid(handle, &logicLink), CM_SUCCESS);
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        EXPECT_EQ(CM_GetLogicLinkByAddr(&addr, &logicLink), CM_SUCCESS);
        // 连接完成后，不应该存在一条空地址
        CM_LOGI("UT_CM_ApiTestConnectAndDisconnect CM_GetLogicLinkByAddr");
        SLE_Addr_S emptyAddr = { 0 };
        EXPECT_EQ(CM_GetLogicLinkByAddr(&emptyAddr, &logicLink), CM_NOT_FOUND);
    }
    CM_LOGI("UT_CM_ApiTestConnectAndDisconnect CM_ApiTestDisconnect");
    handle = g_passiveHandleInitValue;
    for (uint16_t i = 0; i < testCount; i++) {
        CM_ApiTestDisconnect(handle + i);
    }
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);
    EXPECT_EQ(UT_CM_GetDtapTestConnectListSize(), 0);

    // 断开完成后，也不应该存在一条空地址
    CM_LogicLink_S logicLink = { 0 };
    SLE_Addr_S emptyAddr = { 0 };
    EXPECT_EQ(CM_GetLogicLinkByAddr(&emptyAddr, &logicLink), CM_NOT_FOUND);
    UT_CM_TestNormalReset();
    CM_LOGI("UT_CM_ApiTestConnectAndDisconnect end");
}

static void UT_CM_ApiTestConnectAndDeInit(void)
{
    CM_LOGI("UT_CM_ApiTestConnectAndDeInit start");
    uint16_t handle = g_activeHandleInitValue;
    // 第一次发起连接
    CM_LOGI("UT_CM_ApiTestConnect first connect start");
    UT_CM_CancelConnectResult(DLI_SUCCESS);
    CM_ConnectSetParamReq_S setParam = { 0 };
    CM_ConnectSetNormalParam(setParam);
    EXPECT_EQ(CM_ConnectSetParamReq(&setParam), CM_SUCCESS);

    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    CM_ConnectParamReq_S connParam = { 0 };
    connParam.localIndex = CM_CONNECT_LOCAL_INDEX_0;
    connParam.version = CM_CONNECT_VERSION_1_0;
    if (!setParam.enableFilterPolicy) {
        (void)memcpy_s(&connParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    }
    UT_CM_ApiTestAcceptFilterList(addr);

    // 连接成功
    CM_LOGI("UT_CM_ApiTestConnectAndDeInit connect start");
    UT_CM_CancelConnectResult(DLI_SUCCESS);
    EXPECT_EQ(CM_ConnectEstablishReq(&connParam), CM_SUCCESS);
    UT_CM_TestNormalReset();
    UT_CM_SleConnectCompleteEvt(handle, DLI_SUCCESS, 0);
    CM_LOGI("UT_CM_ApiTestConnectAndDeInit connect success end");

    CM_DeInit();
    // 业务系统需要自身销毁自己的内存
    UT_CM_GetTestConnectListClear();
    CM_Init();
    UT_CM_RegConnectCbks();
    UT_CM_RegTransChannelListener();
}

static void UT_CM_ApiTestConnectAndDisconnectMultiSameAddress(uint16_t testCount)
{
    // 重复同一个地址，进行建链和拆链
    const uint16_t multiSameAddressCount = 3;
    for (uint32_t i = 0; i < multiSameAddressCount; i ++) {
        CM_LOGI("UT_CM_ApiTestConnectAndDisconnectMultiSameAddress count:%u start", i);
        UT_CM_ApiTestConnectAndDisconnect(testCount);
        CM_LOGI("UT_CM_ApiTestConnectAndDisconnectMultiSameAddress count:%u end", i);
    }
}

static void UT_CM_RegRequiredConnectCbks(void)
{
    CM_ConnectCbks_S cbks = { 0 };
    cbks.readRemoteFeatureVersionCbk = UT_CM_SsapConnectReadRemoteFetureVersionCbk;
    cbks.connUpdateParamCbk = UT_CM_SsapConnectUpdatePramCbk;
    cbks.connRemoteUpdateParamReqCbk = UT_CM_ConnectRemoteUpdateParamReqCbk;
    cbks.setPhyCbk = UT_CM_SetPhyCbk;
    EXPECT_EQ(CM_RegConnectCbks(&cbks), CM_SUCCESS);
}

static void UT_CM_ADPT_RegLogicLinkListener(void)
{
    CM_LogicLinkCbks_S cmCbk = {};
    cmCbk.moduleId = CM_MODULE_ADPT;
    cmCbk.logicLinkCbk = UT_CM_ServiceConnectStateCbk;
    cmCbk.remoteFeaturesCbk = UT_CM_ReadRemoteFeatureCbk;
    cmCbk.connUpdateParamCbk = UT_CM_ConnUpdateParamCbk;
    EXPECT_EQ(CM_RegLogicLinkListener(&cmCbk), CM_SUCCESS);
}

static CM_ConnectCbks_S UT_CM_GetRequiredConnectCbks(void)
{
    CM_ConnectCbks_S cbks = { 0 };
    cbks.readRemoteFeatureVersionCbk = UT_CM_SsapConnectReadRemoteFetureVersionCbk;
    cbks.connUpdateParamCbk = UT_CM_SsapConnectUpdatePramCbk;
    cbks.connRemoteUpdateParamReqCbk = UT_CM_ConnectRemoteUpdateParamReqCbk;
    cbks.setPhyCbk = UT_CM_SetPhyCbk;
    return cbks;
}

static void UT_CM_DTAP_RegLogicLinkListener(void)
{
    CM_LogicLinkCbks_S cmCbkDtap = {};
    cmCbkDtap.moduleId = CM_MODULE_DTAP;
    cmCbkDtap.logicLinkCbk = UT_CM_DtapConnectStateCbk;
    cmCbkDtap.remoteFeaturesCbk = NULL;
    cmCbkDtap.connUpdateParamCbk = UT_CM_SsapLogicLinkUpdatePramCbk;
    EXPECT_EQ(CM_RegLogicLinkListener(&cmCbkDtap), CM_SUCCESS);
}

static void UT_CM_RegConnectCbks(void)
{
    UT_CM_RegRequiredConnectCbks();
    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_DTAP_RegLogicLinkListener();
}

static void UT_CM_RegTransChannelListener(void)
{
    CM_TransChannelCbk cbk = UT_CM_DtapFixedTransChannelStateCbk;
    EXPECT_EQ(CM_RegTransChannelListener(cbk), CM_SUCCESS);
}

TEST_F(UT_CM_API, CM_ApiConnectAndDisconnectTest)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    UT_CM_RegConnectCbks();
    UT_CM_RegTransChannelListener();
    const uint16_t testCount = 3;
    UT_CM_ApiTestConnectAndDisconnectMultiSameAddress(testCount);
    CM_UnRegTransChannelListener();
    CM_DeInit();
}

TEST_F(UT_CM_API, CM_ApiConnectAndDeInitTest)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    UT_CM_RegConnectCbks();
    UT_CM_RegTransChannelListener();
    UT_CM_ApiTestConnectAndDeInit();
    const uint16_t testCount = 1;
    UT_CM_ApiTestConnectAndDisconnect(testCount);
    CM_DeInit();
}

/*
 * @brief 被动连接测试
 */
TEST_F(UT_CM_API, CM_ApiPassiveConnectedTest)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    UT_CM_RegConnectCbks();
    UT_CM_RegTransChannelListener();
    UT_CM_ApiTestConnectAndDeInit();
    const uint16_t testCount = 1;
    UT_CM_ApiTestPassiveConnectedAndDisconnect(testCount);
    CM_DeInit();
}

TEST_F(UT_CM_API, CM_Enable)
{
    EXPECT_EQ(CM_Enable(), CM_SUCCESS);
    CM_Disable();
}

TEST_F(UT_CM_API, CM_SetSubrateParam_01)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    CM_ConnectCbks_S cbks = UT_CM_GetRequiredConnectCbks();
    cbks.setAcbSubrateCbk = UT_CM_SetAcbSubrateCbk;
    EXPECT_EQ(CM_RegConnectCbks(&cbks), CM_SUCCESS);

    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();
    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestConnect(handle);

    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);

    // 2) 断开1个连接，并去初始化环境
    CM_ApiTestDisconnect(handle);
    CM_DeInit();
}

TEST_F(UT_CM_API, CM_SetSubrateParam_02)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    CM_ConnectCbks_S cbks = UT_CM_GetRequiredConnectCbks();
    cbks.setAcbSubrateCbk = UT_CM_SetAcbSubrateCbk;
    EXPECT_EQ(CM_RegConnectCbks(&cbks), CM_SUCCESS);

    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();
    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestConnect(handle);

    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);

    // 2) 断开1个连接，并去初始化环境
    CM_ApiTestDisconnect(handle);
    CM_DeInit();
}

TEST_F(UT_CM_API, CM_ReqSubrateParam_01)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    CM_ConnectCbks_S cbks = UT_CM_GetRequiredConnectCbks();
    cbks.reqAcbSubrateCbk = UT_CM_ReqAcbSubrateCbk;
    EXPECT_EQ(CM_RegConnectCbks(&cbks), CM_SUCCESS);

    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();
    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestConnect(handle);

    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);

    // 2) 断开1个连接，并去初始化环境
    CM_ApiTestDisconnect(handle);
    CM_DeInit();
}

} // namespace TEST
} // namespace Nearlink
} // namespace OHOS