/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"

#include "sdf_addr.h"
#include "sdf_mem.h"
#include "sdf_string.h"
#include "nlstk_log.h"

#include "cm_def.h"
#include "dtap_tcid.h"
#include "ssap_link.h"
#include "ssapc_cache.h"
#include "ssap_manager.h"
#include "ssaps_service.h"
#include "nlstk_ssap_app_link.h"
#include "ssapc_app.h"

#include "nlstk_micp_client.h"
#include "micp_dev.h"
#include "cpfwk_log.h"
using namespace testing;
using namespace testing::ext;
using namespace OHOS;

extern SsapcAppRegParam_S *g_regList;

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x04, 0x04, 0x06, 0x06, 0x08, 0x08}};
static uint16_t g_lcid = 1;

static uint8_t g_serviceUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x19};
static uint8_t g_propertyUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x7e};

static bool g_recvCbk = false;
static uint8_t g_micState = -1;
static NLSTK_MicpConnectState_E g_conState = NLSTK_MICP_STATE_DISCONNECTED;

static void STUB_CacheMicpService();
static void STUB_ConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_MicpConnectState_E curState,
    NLSTK_MicpConnectState_E preState, uint8_t errorCode);
static void STUB_MicStateCbk(SLE_Addr_S *addr, uint8_t micState);

class IT_MICP_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
protected:
    void SetUp() override
    {
        // 步骤1：初始化桩函数
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(TEST_ScheduleTimerAddStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(TEST_ScheduleTimerDelStub);

        TEST_CM_Init();
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);

        EXPECT_CALL(dtapMock, DTAP_RegisterDataRecvCb).WillRepeatedly(TEST_DTAP_RegisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_UnregisterDataRecvCb).WillRepeatedly(TEST_DTAP_UnregisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_DataSend).WillRepeatedly(TEST_DTAP_DataSend);

        // 步骤2：初始化SSAP模块客户端
        SSAP_LinkInit();
        SSAP_Init();

        // 步骤3：使能MICP
        MicpDevInit();
        TEST_RunQueueStubSchedule();

        // 注册回调
        NLSTK_MicpCbk_S cbk = {0};
        cbk.eventCbk = STUB_ConnectStateChangeCbk;
        cbk.micStateCbk = STUB_MicStateCbk;
        NLSTK_MicpRegisterCallback(&cbk);
    }

    void TearDown() override
    {
        // 步骤4：去使能MICP
        MicpDevDisable();
        MicpDevDeinit();

        // 步骤5：去初始化SSAP模块
        SSAP_DeInit();
        SSAP_LinkDeInit();

        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

static void STUB_ConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_MicpConnectState_E curState,
    NLSTK_MicpConnectState_E preState, uint8_t errorCode)
{
    g_recvCbk = true;
    g_conState = curState;
}

static void STUB_MicStateCbk(SLE_Addr_S *addr, uint8_t micState)
{
    g_recvCbk = true;
    g_micState = micState;
}

static void STUB_ResetData()
{
    g_recvCbk = false;
    g_conState = NLSTK_MICP_STATE_DISCONNECTED;
    g_micState = -1;
}

static void STUB_CacheMicpService()
{
    SsapCacheServInfo_S serv = {0};
    serv.handle = 0x10;
    serv.endHandle = 0x11;
    serv.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serv.uuid, sizeof(NLSTK_SsapUuid_S), &g_serviceUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheServ(&g_addr, &serv);

    NLSTK_SsapPrty_S prty = {0};
    prty.handle = 0x11;

    memcpy_s(&prty.uuid, sizeof(NLSTK_SsapUuid_S), &g_propertyUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &prty);

    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_PROPERTY, 0x11);
    SsapcCacheServDiscFinish(&g_addr);
}

static uint8_t g_readRsp[] = {0x09, 0x03, 0x01, 0x00, 0x00, 0x00};
static uint8_t g_readRsp2[] = {0x09, 0x03, 0x01, 0x00, 0x01, 0x00};
static uint8_t g_readRspError[] = {0x09, 0x0B, 0x04, 0x00};
static uint8_t g_writeRsp[] = {0x0E, 0x03, 0x11, 0x00, 0x02, 0x01, 0x00};
static uint8_t g_writeErrorRsp[] = {0x0E, 0x03, 0x12, 0x00, 0x02, 0x01, 0x00};
static uint8_t g_valueNtfOff[] = {0x0F, 0x03, 0x11, 0x00, 0x02, 0x00, 0x00, 0x00};
static uint8_t g_valueNtfOn[] = {0x0F, 0x03, 0x11, 0x00, 0x02, 0x00, 0x01, 0x00};
/**
 * @test MICP_TEST_001
 * @brief 此测试用例用于验证MICP Profile连接流程，SSAP已有服务结构缓存。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_MicpRegisterCallbackCbk注册回调；
 * 2. 模拟链路建立成功；
 * 3. 调用NLSTK_MicpConnect开启连接；
 * 4. 模拟收到连接流程中的SSAP响应报文；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 模拟ntf报文校验是否收到mic状态变更；
 * 7. 调用NLSTK_MicpDisconnect断连；
 * 8. 模拟链路断连销毁；
 * 9. 校验是否收到连接状态的回调已断连；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_001)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_001");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeRsp, sizeof(g_writeRsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_readRsp, sizeof(g_readRsp));

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_CONNECTED);
    EXPECT_EQ(g_micState, 1);
    STUB_ResetData();

    // 模拟收到value ntf
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_valueNtfOff, sizeof(g_valueNtfOff));
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_micState, 0);
    STUB_ResetData();

    // 模拟收到value ntf
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_valueNtfOn, sizeof(g_valueNtfOn));
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_micState, 1);
    STUB_ResetData();

    // 调用NLSTK_MicpDisconnect断开连接
    ret = NLSTK_MicpDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2, };
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
}

/**
 * @test MICP_TEST_002
 * @brief 此测试用例用于验证MICP Profile连接流程，SSAP已有服务结构缓存。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_MicpRegisterCallbackCbk注册回调；
 * 2. 模拟链路建立成功；
 * 3. 调用NLSTK_MicpConnect开启连接；
 * 4. 模拟收到连接流程中的SSAP响应报文；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 模拟ntf报文校验是否收到mic状态变更；
 * 7. 调用NLSTK_MicpDisconnect断连；
 * 8. 模拟链路断连销毁；
 * 9. 校验是否收到连接状态的回调已断连；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_002)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_002");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeRsp, sizeof(g_writeRsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_readRsp2, sizeof(g_readRsp2));

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_CONNECTED);
    EXPECT_EQ(g_micState, 1);
    STUB_ResetData();

    // 模拟收到value ntf
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_valueNtfOff, sizeof(g_valueNtfOff));
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_micState, 0);
    STUB_ResetData();

    // 模拟收到value ntf
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_valueNtfOn, sizeof(g_valueNtfOn));
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_micState, 1);
    STUB_ResetData();

    // 调用NLSTK_MicpDisconnect断开连接
    ret = NLSTK_MicpDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2, };
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
}

/**
 * @test MICP_TEST_003
 * @brief 此测试用例用于验证MICP Profile连接异常流程, write rsp错误
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_003)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_003");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeErrorRsp, sizeof(g_writeErrorRsp));
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
    STUB_ResetData();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2, };
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
}

/**
 * @test MICP_TEST_004
 * @brief 此测试用例用于验证MICP Profile连接流程，读属性错误
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_004)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_004");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeRsp, sizeof(g_writeRsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_readRspError, sizeof(g_readRspError));

    // 预期收到回调连接失败
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
    STUB_ResetData();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2, };
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
}
/**
 * @test MICP_TEST_005
 * @brief 此测试用例通过直接调用注册的回调构造MICP连接异常场景
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_005)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_005");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeRsp, sizeof(g_writeRsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_readRsp, sizeof(g_readRsp));
    STUB_ResetData();

    // 调用MicpRegAppCb
    g_regList[0].cb->onRegisterApp(0, &g_addr, NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
    STUB_ResetData();
}

/**
 * @test MICP_TEST_006
 * @brief 此测试用例通过直接调用注册的回调构造MICP连接异常场景
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_006)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_006");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeRsp, sizeof(g_writeRsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_readRsp, sizeof(g_readRsp));
    STUB_ResetData();

    // MicpConnStateChangedCb
    g_regList[0].cb->onConnectionStateChanged(0, SSAP_CONNECT_STATE_CONNECTED, NLSTK_ERRCODE_FAIL, 0);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
    STUB_ResetData();
}

/**
 * @test MICP_TEST_007
 * @brief 此测试用例通过直接调用注册的回调构造MICP连接异常场景
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_007)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_007");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeRsp, sizeof(g_writeRsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_readRsp, sizeof(g_readRsp));
    STUB_ResetData();

    // MicpGetServicesCb
    NLSTK_SsapServ_S service = {0};
    g_regList[0].cb->onGetServices(0, (NLSTK_SsapUuid_S *)&g_serviceUuid, &service, 0, NULL);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
    STUB_ResetData();
}

/**
 * @test MICP_TEST_008
 * @brief 此测试用例通过直接调用注册的回调构造MICP连接异常场景
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_008)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_008");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeRsp, sizeof(g_writeRsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_readRsp, sizeof(g_readRsp));
    STUB_ResetData();

    // MicpGetServicesCb
    NLSTK_SsapServ_S service = {0};
    service.propertyNum = 1;
    NLSTK_SsapPrty_S property = {0};
    property.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x19};
    service.properties = &property;
    g_regList[0].cb->onGetServices(0, (NLSTK_SsapUuid_S *)&g_serviceUuid, &service, 1, NULL);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
    STUB_ResetData();
}

/**
 * @test MICP_TEST_009
 * @brief 此测试用例通过直接调用注册的回调构造MICP连接异常场景
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_009)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_009");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeRsp, sizeof(g_writeRsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_readRsp, sizeof(g_readRsp));
    STUB_ResetData();

    // MicpSetPropertyNtfCb
    g_regList[0].cb->onSetPropertyNtf(0, (NLSTK_SsapUuid_S *)g_serviceUuid, 0, true, NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
    STUB_ResetData();
}
/**
 * @test MICP_TEST_010
 * @brief 此测试用例通过直接调用注册的回调构造MICP连接异常场景
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_010)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_010");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeRsp, sizeof(g_writeRsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_readRsp, sizeof(g_readRsp));
    STUB_ResetData();

    // MicpSetPropertyNtfCb
    g_regList[0].cb->onSetPropertyNtf(0, (NLSTK_SsapUuid_S *)g_serviceUuid, 0, true, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
    STUB_ResetData();
}
/**
 * @test MICP_TEST_011
 * @brief 此测试用例通过直接调用注册的回调构造MICP连接异常场景
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_011)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_011");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeRsp, sizeof(g_writeRsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_readRsp, sizeof(g_readRsp));
    STUB_ResetData();

    // MicpReadPropertyCb
    NLSTK_SsapClientReadPropertyInfo_S property = {0};
    property.handle = 0x11;
    g_regList[0].cb->onReadProperty(0, &property, NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
    STUB_ResetData();
}

/**
 * @test MICP_TEST_012
 * @brief 此测试用例通过直接调用注册的回调构造MICP连接异常场景
 *
 * @pre
 * - SSAP模块已初始化。
 * - MICP模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_MICP_TEST, MICP_TEST_012)
{
    // 链路建立
    CP_LOG_INFO("enter MICP_TEST_012");
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0, };
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheMicpService();

    // 调用NLSTK_MicpConnect开启连接
    uint32_t ret = NLSTK_MicpConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 连接中ssap报文,write rsp, read rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_writeRsp, sizeof(g_writeRsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, g_readRsp, sizeof(g_readRsp));
    STUB_ResetData();

    // MicpReadPropertyCb
    NLSTK_SsapClientReadPropertyInfo_S property = {0};
    property.handle = 0x11;
    property.errorCode = SSAP_ERRCODE_INVALID_PDU;
    g_regList[0].cb->onReadProperty(0, &property, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_conState, NLSTK_MICP_STATE_DISCONNECTED);
    STUB_ResetData();
}