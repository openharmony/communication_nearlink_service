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
#include <stddef.h>
#include "securec.h"
#include "cp_worker.h"
#include "sdf_thread.h"
#include "sdf_evc.h"
#include "sdf_dlist.h"
#include "sdf_map.h"
#include "log.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "dli.h"
#include "dli_errno.h"
#include "dli_thread.h"
#include "dli_event_struct.h"
#include "cm_exter_cbks_mgr.h"
#include "cm_common.h"
#include "cm_log.h"
#include "cm_api.h"
#include "cm_icb_api.h"
#include "cm_def.h"
#include "cm_trans_channel_api.h"
#include "cm_util.h"
#include "cm_logic_link_listener_mgr.h"
#include "cm_signaling_manage.h"
#include "cm_inner_api.h"
#include "cm_concurrent_conn.h"
#include "cm_util_test.h"
#include "cm_dli_adapter.h"
#include "CP_Timer_mocker.h"

/* 音频场景默认subrate */
#define UT_NLSTK_DEFAULT_SUBRATE 0x06

/* 帧4场景默认subrate值 */
#define UT_GLE_FRAME_TYPE_4_SUBRATE_DEFAULT 0x02
#define UT_GLE_FRAME_TYPE_4_MAX_LATENCY 0
#define UT_GLE_FRAME_TYPE_4_CONTINUATION_NUM 1
#define UT_GLE_FRAME_TYPE_4_SUBRATE_SUPERVISION_TIMEOUT 2000

static constexpr int UT_SLE_FREQMAP_LEN = 10;
static uint8_t g_testConnectState = 0;
static const uint16_t g_activeHandleInitValue = 0x2051;
static const uint16_t g_passiveHandleInitValue = 0x2061;

// DLI未定义事件（少用，建议在设计时初期需要考虑定义DLI返回事件）
typedef struct {
} DLI_ConnUndefinedCmpEvt;

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

static void prepareForTest()
{
    SDF_ThreadInit(2e2);
    SDF_EvcInit();
    EXPECT_EQ(CM_SignalingCacheInit(), CM_SUCCESS);
    EXPECT_EQ(CM_ConcurrentConnInit(), CM_SUCCESS);
    EXPECT_EQ(CM_Init(), CM_SUCCESS);
    DLI_ThreadInit();
}

static void finishTest()
{
    CM_SignalingCacheDeinit();
    SDF_EvcDeinit();
    SDF_ThreadDeinit();
    CM_DeInit();
    DLI_ThreadDeinit();
}

class UT_CM_CONCURRENT_CONN_API : public testing::Test {
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
        CM_LOGI("SetUpTestCase");
        prepareForTest();
    }

    // TearDownTestCase 在所有 TEST_F 测试完成后执行一次
    static void TearDownTestCase()
    {
        finishTest();
        CM_LOGI("TearDownTestCase");
    }
};

static void TEST_FreqBandSwitchCbk(CM_FreqBandSwitchParam *param)
{
    CM_LOGI("enter");
    if (param == nullptr) {
        CM_LOGE("cmp is nullptr");
        return;
    }
}

static void TEST_ConnectCancelCbk(uint8_t *param)
{
    CM_LOGI("TEST_ConnectCancelCbk");
}

static void TEST_ConnectReadRemoteFetureVersionCbk(CM_ReadRemoteFeatureVersionRsp_S *param)
{
    CM_LOGI("TEST_ConnectReadRemoteFetureVersionCbk");
}

static void TEST_ConnectUpdatePramCbk(CM_ConnectUpdateParamRsp_S *param)
{
    CM_LOGI("TEST_ConnectUpdatePramCbk");
}

static void TEST_ConnectRemoteUpdateParamReqCbk(CM_ConnectRemoteUpdateParamReq_S *param)
{
    CM_LOGI("TEST_ConnectRemoteUpdateParamReqCbk");
}

static void TEST_SetPhyCbk(CM_SetPhyRsp_S *param)
{
    CM_LOGI("TEST_SetPhyCbk");
}

static void UapiCmLinkInitTest(void)
{
    uint32_t ret = CM_Init();
    EXPECT_EQ(ret, CM_SUCCESS);
    ret = CM_ListenFreqBandSwitchEvent(TEST_FreqBandSwitchCbk);
    EXPECT_EQ(ret, CM_SUCCESS);

    CM_ConnectCbks_S cbks = { 0 };
    cbks.readRemoteFeatureVersionCbk = TEST_ConnectReadRemoteFetureVersionCbk;
    cbks.connUpdateParamCbk = TEST_ConnectUpdatePramCbk;
    cbks.connRemoteUpdateParamReqCbk = TEST_ConnectRemoteUpdateParamReqCbk;
    cbks.setPhyCbk = TEST_SetPhyCbk;
    EXPECT_EQ(CM_RegConnectCbks(&cbks), CM_SUCCESS);
}

// 测试场景：非必填回调设置为空，测试其回调有没有判空
static void UapiCmLinkInitOnlyRequiredTest(void)
{
    UapiCmLinkInitTest();
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UapiCmLinkInit)
{
    UapiCmLinkInitTest();
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

static void UT_CM_ApiTestBgConnect(uint16_t handle)
{
    CM_LOGI("UT_CM_ApiTestBgConnect start, handle:%hu", handle);
    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    CM_BgConnAddrParam_S addrArr = {};
    (void)memcpy_s(&addrArr.addr, sizeof(CM_BgConnAddrParam_S), &addr, sizeof(CM_BgConnAddrParam_S));
    uint32_t ret = CM_BackgroundConnectAdd(CM_MODULE_ADPT, 1, &addrArr);
    EXPECT_EQ(ret, CM_SUCCESS);
    CM_LOGI("UT_CM_ApiTestBgConnect end, handle:%u", handle);
}

static void UT_CM_ApiTestBgRemove(uint16_t handle)
{
    CM_LOGI("UT_CM_ApiTestBgRemove start, handle:%u", handle);
    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    EXPECT_EQ(CM_BackgroundConnectRemove(CM_MODULE_ADPT, &addr), CM_SUCCESS);
    CM_LOGI("UT_CM_ApiTestBgRemove end, handle:%u", handle);
}

static void UT_CM_ApiTestDirectConnect(uint16_t handle)
{
    CM_LOGI("UT_CM_ApiTestDirectConnect start, handle:%hu", handle);
    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    CM_DirectConnAddrParam_S param = {};
    (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
    param.frameType = CM_CONN_PARAM_FRAME_TYPE_1;
    uint32_t ret = CM_DirectConnectAdd(CM_MODULE_ADPT, &param);
    EXPECT_EQ(ret, CM_SUCCESS);
    CM_LOGI("UT_CM_ApiTestDirectConnect end, handle:%u", handle);
}

static void UT_CM_ApiTestDirectDisconnect(uint16_t handle)
{
    CM_LOGI("UT_CM_ApiTestDirectDisconnect start, handle:%u", handle);
    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    EXPECT_EQ(CM_DirectConnectRemove(CM_MODULE_ADPT, &addr, CM_DISC_REASON_REMOTE_USER_TERMINATED), CM_SUCCESS);
    CM_LOGI("UT_CM_ApiTestDirectDisconnect end, handle:%u", handle);
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

static void UT_CM_RegTransChannelListener(void)
{
    CM_TransChannelCbk cbk = UT_CM_DtapFixedTransChannelStateCbk;
    EXPECT_EQ(CM_RegTransChannelListener(cbk), CM_SUCCESS);
}

static void CM_ApiTestCheckLogicLinkAndTransChannel(uint16_t handle)
{
    CM_LogicLink_S logicLink = { 0 };
    EXPECT_EQ(CM_GetLogicLinkByLcid(handle, &logicLink), CM_SUCCESS);
    EXPECT_EQ(logicLink.role, UT_CM_GetTestNodeRole());
    EXPECT_NE(UT_CM_GetTestFixedTransChannelSize(), 0);
}

static void UT_CM_CheckConnectComplete(uint16_t handle)
{
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
    SLE_Addr_S emptyAddr = { 0 };
    EXPECT_EQ(CM_GetLogicLinkByAddr(&emptyAddr, &logicLink), CM_NOT_FOUND);
}

static void UT_CM_CheckDisconnectComplete(uint16_t handle)
{
    CM_LogicLink_S logicLink = { 0 };
    EXPECT_EQ(CM_GetLogicLinkByLcid(handle, &logicLink), CM_NOT_FOUND);
    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    EXPECT_EQ(CM_GetLogicLinkByAddr(&addr, &logicLink), CM_NOT_FOUND);
    EXPECT_EQ(CM_SetLogicLinkDeviceType(handle, CM_DEVTYPE_OLD), CM_NOT_FOUND);
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_BgConnectAddAndRemoveSucccess)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();
    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestBgConnect(handle);
    // 2) 连接成功
    UT_CM_SleConnectCompleteEvt(handle, DLI_SUCCESS, 0);
    UT_CM_CheckConnectComplete(handle);
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 1);

    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    // 3) 断开1个连接，并去初始化环境
    UT_CM_ApiTestBgRemove(handle);
    // 4) 断连成功
    UT_CM_SleDisconnectCompleteEvt(handle, DLI_SUCCESS);
    UT_CM_CheckDisconnectComplete(handle);
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);
    CM_DeInit();
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_BgConnectAddAndRemoveButNoConnectEvent)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();
    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestBgConnect(handle);

    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    // 2) 断开1个连接，并去初始化环境
    UT_CM_ApiTestBgRemove(handle);
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);
    CM_DeInit();
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_DirectConnectAddAndRemoveSucccess)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();
    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestDirectConnect(handle);
    // 2) 连接成功
    UT_CM_SleConnectCompleteEvt(handle, DLI_SUCCESS, 0);
    UT_CM_CheckConnectComplete(handle);
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 1);

    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    // 3) 断开1个连接，并去初始化环境
    UT_CM_ApiTestDirectDisconnect(handle);
    // 4) 断连成功
    UT_CM_SleDisconnectCompleteEvt(handle, DLI_SUCCESS);
    UT_CM_CheckDisconnectComplete(handle);
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);
    CM_DeInit();
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_DirectConnectAddAndRemoveButNoConnectEvent)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();
    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestDirectConnect(handle);

    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    // 2) 断开1个连接，并去初始化环境
    UT_CM_ApiTestDirectDisconnect(handle);
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);
    CM_DeInit();
}

static uint16_t UT_CM_PrepareCreateConnectTest(void)
{
    // 1) 初始化注册环境，并创建1个连接
    EXPECT_EQ(CM_Init(), CM_SUCCESS);
    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();
    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestDirectConnect(handle);
    // 2) 连接成功
    UT_CM_SleConnectCompleteEvt(handle, DLI_SUCCESS, 0);
    UT_CM_CheckConnectComplete(handle);
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 1);
    return handle;
}

static void UT_CM_AfterDisconnectTest(uint16_t handle)
{
    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    // 1) 断开1个连接，并去初始化环境
    UT_CM_ApiTestDirectDisconnect(handle);
    // 2) 断连成功
    UT_CM_SleDisconnectCompleteEvt(handle, DLI_SUCCESS);
    UT_CM_CheckDisconnectComplete(handle);
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);
    CM_DeInit();
}

class CM_UT_TestcaseFactory {
public:
    void RegisterTestcaseFunc(const std::function<void(uint16_t handle)>& func)
    {
        func_ = func;
    }

    void ExecuteFunc(void)
    {
        // 创建1个连接
        uint16_t handle = UT_CM_PrepareCreateConnectTest();
        // 执行测试用例函数
        func_(handle);
        // 断开1个连接
        UT_CM_AfterDisconnectTest(handle);
    }
private:
    std::function<void(uint16_t)> func_;
};

static void UT_CM_ExecuteTestcaseFunc(const std::function<void(uint16_t handle)>& func)
{
    CM_UT_TestcaseFactory factory;
    factory.RegisterTestcaseFunc(func);
    factory.ExecuteFunc();
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_BackgroundConnectAdd_1)
{
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        // 测试场景：已添加设备，进行背景连接
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        CM_BgConnAddrParam_S addrArr = {};
        (void)memcpy_s(&addrArr.addr, sizeof(CM_BgConnAddrParam_S), &addr, sizeof(CM_BgConnAddrParam_S));
        EXPECT_EQ(CM_BackgroundConnectAdd(CM_MODULE_ADPT, 1, &addrArr), CM_SUCCESS);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_DirectConnectAdd_1)
{
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        // 测试场景：已添加设备，进行主动连接
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        CM_DirectConnAddrParam_S addParam = {};
        (void)memcpy_s(&addParam.addr, sizeof(SLE_Addr_S), &addParam, sizeof(SLE_Addr_S));
        addParam.frameType = CM_CONN_PARAM_FRAME_TYPE_1;
        EXPECT_EQ(CM_DirectConnectAdd(CM_MODULE_ADPT, &addParam), CM_SUCCESS);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_DirectConnectRemove_1)
{
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        // 测试场景：重复断开连接
        UT_CM_ApiTestDirectDisconnect(handle);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_BackgroundConnectClear)
{
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        // 测试场景：取消已连接的链路，预期无法取消
        EXPECT_EQ(CM_BackgroundConnectClear(CM_MODULE_ADPT), CM_SUCCESS);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_BackgroundConnectClear_1)
{
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        // 测试场景：取消已连接的链路，预期无法取消
        EXPECT_EQ(CM_BackgroundConnectClear(CM_MODULE_ADPT), CM_SUCCESS);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_BackgroundConnectClear_2)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();
    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestDirectConnect(handle);
    // 2) 测试场景：还未连接成功，重复发起连接
    // 测试场景：取消已连接的链路，预期无法取消
    EXPECT_EQ(CM_BackgroundConnectClear(CM_MODULE_ADPT), CM_SUCCESS);
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);
    CM_DeInit();
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_BackgroundConnectAddAndRemove_1)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();
    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestBgConnect(handle);
    // 2) 测试场景：还未连接成功，重复发起连接
    UT_CM_ApiTestBgConnect(handle);

    // 3) 测试场景：还未连接成功，重复移除连接
    UT_CM_ApiTestBgRemove(handle);
    UT_CM_ApiTestBgRemove(handle);
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);
    CM_DeInit();
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_DirectConnectAdd_2)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();
    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestDirectConnect(handle);
    // 2) 测试场景：还未连接成功，重复发起连接
    UT_CM_ApiTestDirectConnect(handle);
    // 3) 连接成功
    UT_CM_SleConnectCompleteEvt(handle, DLI_SUCCESS, 0);
    UT_CM_CheckConnectComplete(handle);
    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 1);

    UT_CM_AfterDisconnectTest(handle);
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_BgConnectRemove_1)
{
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        // 测试场景：移除已连接，无效
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        EXPECT_EQ(CM_BackgroundConnectRemove(CM_MODULE_ADPT, &addr), CM_SUCCESS);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_DirectConnectAdd_AddTimer_1)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();

    // 2) 设置连接（立即）超时开关
    CP_TimerSetExecCallbackAtOnce(true);

    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestDirectConnect(handle);

    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);

    // 3) 恢复连接（立即）超时开关关闭
    CP_TimerSetExecCallbackAtOnce(false);
    CM_DeInit();
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_DirectConnectAdd_AddTimer_2)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    // 1) 初始化注册环境，并创建1个连接
    UT_CM_ADPT_RegLogicLinkListener();
    UT_CM_RegTransChannelListener();

    // 2) 设置连接定时器添加开关
    CP_TimerSetAddFailed(true);

    uint16_t handle = g_activeHandleInitValue;
    UT_CM_ApiTestDirectConnect(handle);

    EXPECT_EQ(UT_CM_GetTestConnectListSize(), 0);

    // 3) 恢复连接定时器添加开关关闭
    CP_TimerSetAddFailed(false);
    CM_DeInit();
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_ConnectParamUpdateReq)
{
    UapiCmLinkInitTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);

        // UAPI 测试
        CM_ConnectUpdateParamReq_S gleParam = {};
        (void)memcpy_s(&gleParam.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        EXPECT_EQ(CM_ConnectUpdateParamReq(&gleParam), CM_SUCCESS);
        EXPECT_EQ(CM_ConnectUpdateParamReq(NULL), CM_INVALID_PARAM_ERR);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_ConnectReleaseReq_01)
{
    EXPECT_EQ(CM_Init(), CM_SUCCESS);

    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        // 测试目标指令API功能，并测试其回调参数
        // 测试场景：非法路径覆盖，地址不存在
        CM_DisconnectParamReq_S disParam_1 = {};
        disParam_1.discReason = CM_DISC_REASON_REMOTE_USER_TERMINATED;
        EXPECT_EQ(CM_ConnectReleaseReq(&disParam_1), CM_FAIL);

        disParam_1.discReason = CM_DISC_REASON_CANCEL_PAIR;
        EXPECT_EQ(CM_ConnectReleaseReq(&disParam_1), CM_FAIL);

        disParam_1.discReason = CM_DISC_REASON_COMMAND_TIMEOUT;
        EXPECT_EQ(CM_ConnectReleaseReq(&disParam_1), CM_FAIL);
        //    测试场景：非法路径覆盖，断连原因为空
        CM_DisconnectParamReq_S disParam_2 = {};
        EXPECT_EQ(CM_ConnectReleaseReq(&disParam_2), CM_INVALID_PARAM_ERR);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_ConnectEstablishReq_01)
{
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        // 测试目标指令API功能，并测试其回调参数
        // 测试场景：连接已经存在
        CM_ConnectParamReq_S param = { 0 };
        (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        EXPECT_EQ(CM_ConnectEstablishReq(&param), CM_SUCCESS);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_SetPowerMode_01)
{
    UapiCmLinkInitTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        // 测试目标指令API功能，并测试其回调参数
        // 测试场景：覆盖其回调
        UT_CM_SetPowerModeCbkEvt(handle);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_SetPhyReq_02)
{
    UapiCmLinkInitTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        // 测试目标指令API功能，并测试其回调参数
        // 测试场景：连接不存在
        CM_SetPhyReq_S param = { 0 };
        param.lcid = CM_INVALID_LCID;
        EXPECT_EQ(CM_SetPhy(&param), CM_SUCCESS);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_SetPhyReq_03)
{
    UapiCmLinkInitTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        // 测试目标指令API功能，并测试其回调参数
        CM_SetPhyReq_S setPhyReqParam = { 0 };
        setPhyReqParam.lcid = handle;
        EXPECT_EQ(CM_SetPhy(&setPhyReqParam), CM_SUCCESS);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, CM_InnerSetACBSubrate_1)
{
    UapiCmLinkInitTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        CM_SetACBSubrateInnerParam param = {0};
        param.lcid = handle;
        param.subrate = UT_NLSTK_DEFAULT_SUBRATE;
        EXPECT_EQ(CM_InnerSetACBSubrate(&param), CM_SUCCESS);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, CM_InnerSetACBSubrate_2)
{
    UapiCmLinkInitTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        CM_SetACBSubrateInnerParam param = {0};
        param.lcid = CM_INVALID_LCID;
        param.subrate = UT_NLSTK_DEFAULT_SUBRATE;
        EXPECT_EQ(CM_InnerSetACBSubrate(&param), CM_FAIL);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, CM_InnerSetACBSubrate_3)
{
    UapiCmLinkInitTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        CM_SetACBSubrateInnerParam param = {0};
        param.lcid = handle;
        param.subrate = UT_NLSTK_DEFAULT_SUBRATE;
        EXPECT_EQ(CM_InnerSetACBSubrate(&param), CM_SUCCESS);
        // 重复调用API
        EXPECT_EQ(CM_InnerSetACBSubrate(&param), CM_SUCCESS);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, CM_InnerSetACBSubrate_4)
{
    UapiCmLinkInitOnlyRequiredTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        CM_SetACBSubrateInnerParam param = {0};
        param.lcid = CM_INVALID_LCID;
        param.subrate = UT_NLSTK_DEFAULT_SUBRATE;
        EXPECT_EQ(CM_InnerSetACBSubrate(&param), CM_FAIL);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_SleReqAcbSubrateCbkEvt_1)
{
    UapiCmLinkInitTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        CM_SetACBSubrateParam param = {};
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        param.subrate = UT_GLE_FRAME_TYPE_4_SUBRATE_DEFAULT;
        param.subrateMax = UT_GLE_FRAME_TYPE_4_SUBRATE_DEFAULT;
        param.maxLatency = UT_GLE_FRAME_TYPE_4_MAX_LATENCY;
        param.continuationNum = UT_GLE_FRAME_TYPE_4_CONTINUATION_NUM;
        param.supervisionTimeout = UT_GLE_FRAME_TYPE_4_SUBRATE_SUPERVISION_TIMEOUT;
        UT_CM_SleReqAcbSubrateCbkEvt(handle, &param);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_SleReqAcbSubrateCbkEvt_2)
{
    UapiCmLinkInitOnlyRequiredTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        CM_SetACBSubrateParam param = {};
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &addr, sizeof(SLE_Addr_S));
        param.subrate = UT_GLE_FRAME_TYPE_4_SUBRATE_DEFAULT;
        param.subrateMax = UT_GLE_FRAME_TYPE_4_SUBRATE_DEFAULT;
        param.maxLatency = UT_GLE_FRAME_TYPE_4_MAX_LATENCY;
        param.continuationNum = UT_GLE_FRAME_TYPE_4_CONTINUATION_NUM;
        param.supervisionTimeout = UT_GLE_FRAME_TYPE_4_SUBRATE_SUPERVISION_TIMEOUT;
        UT_CM_SleReqAcbSubrateCbkEvt(handle, &param);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_SleSwitchFreqBandCbkEvt_1)
{
    UapiCmLinkInitTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        CM_FreqBandSwitchParam param = {};
        param.status = DLI_SUCCESS;
        param.lcid = handle;
        UT_CM_SleFreqBandSwitchCbkEvt(handle, &param);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, UT_CM_SleSwitchFreqBandCbkEvt_2)
{
    UapiCmLinkInitOnlyRequiredTest();
    UT_CM_ExecuteTestcaseFunc([](uint16_t handle) {
        CM_FreqBandSwitchParam param = {};
        param.status = DLI_SUCCESS;
        param.lcid = handle;
        UT_CM_SleFreqBandSwitchCbkEvt(handle, &param);
    });
}

TEST_F(UT_CM_CONCURRENT_CONN_API, CM_DliAdapterInit_1)
{
    EXPECT_EQ(CM_DliAdapterInit(), CM_SUCCESS);
    EXPECT_EQ(CM_DliAdapterInit(), CM_SUCCESS);
    EXPECT_EQ(CM_DliAdapterDeinit(), CM_SUCCESS);
    EXPECT_EQ(CM_DliAdapterDeinit(), CM_SUCCESS);
}

static void DliAdapterCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    return;
}

static void DliAdapterCbk_1(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    return;
}

static void DliAdapterCbk_2(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    return;
}

TEST_F(UT_CM_CONCURRENT_CONN_API, CM_RegisterDliAdapterCbk_1)
{
    EXPECT_EQ(CM_DliAdapterInit(), CM_SUCCESS);
    EXPECT_EQ(CM_DliAdapterDeinit(), CM_SUCCESS);
    EXPECT_EQ(CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_DISCONNECT, DliAdapterCbk), CM_NOT_INITED);
    EXPECT_EQ(CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_DISCONNECT), CM_NOT_INITED);
    EXPECT_EQ(CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_CONNECT, DliAdapterCbk), CM_NOT_INITED);
    EXPECT_EQ(CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_CONNECT), CM_NOT_INITED);
}

TEST_F(UT_CM_CONCURRENT_CONN_API, CM_RegisterDliAdapterCbk_2)
{
    EXPECT_EQ(CM_DliAdapterInit(), CM_SUCCESS);
    EXPECT_EQ(CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_DISCONNECT, NULL), CM_INVALID_PARAM_ERR);
    EXPECT_EQ(CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE_MAX, CM_DLI_ADAPTER_DISCONNECT, NULL), CM_INVALID_PARAM_ERR);
    EXPECT_EQ(CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE_MAX, CM_DLI_ADAPTER_DISCONNECT), CM_INVALID_PARAM_ERR);
    EXPECT_EQ(CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_CONNECT, NULL), CM_INVALID_PARAM_ERR);
    EXPECT_EQ(CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE_MAX, CM_DLI_ADAPTER_CONNECT, NULL), CM_INVALID_PARAM_ERR);
    EXPECT_EQ(CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_MODULE_MAX, CM_DLI_ADAPTER_CONNECT), CM_INVALID_PARAM_ERR);
    EXPECT_EQ(CM_DliAdapterDeinit(), CM_SUCCESS);
}

TEST_F(UT_CM_CONCURRENT_CONN_API, CM_RegisterDliAdapterCbk_3)
{
    EXPECT_EQ(CM_DliAdapterInit(), CM_SUCCESS);
    EXPECT_EQ(CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_DISCONNECT, DliAdapterCbk_1), CM_SUCCESS);
    EXPECT_EQ(CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_CONNECT, DliAdapterCbk_2), CM_SUCCESS);
    EXPECT_EQ(CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_DISCONNECT), CM_SUCCESS);
    EXPECT_EQ(CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_CONNECT), CM_SUCCESS);
    EXPECT_EQ(CM_DliAdapterDeinit(), CM_SUCCESS);
}