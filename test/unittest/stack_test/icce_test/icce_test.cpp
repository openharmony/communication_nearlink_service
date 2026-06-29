/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "ssapc_app.h"
#include "cm_def.h"
#include "dtap_tcid.h"
#include "ssap_link.h"
#include "ssapc_cache.h"
#include "ssap_manager.h"
#include "ssaps_service.h"
#include "nlstk_ssap_app_link.h"

#include "icce_type.h"
#include "icce_init.h"
#include "nlstk_icce_client.h"
#include "nlstk_icce_server.h"
#include "nlstk_public_define.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

extern SsapcAppRegParam_S *g_regList;
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x04, 0x04, 0x06, 0x06, 0x08, 0x08}};
static uint16_t g_lcid = 1;

static uint8_t g_serviceUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0D};
static uint8_t g_propertyUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x4C};

static void STUB_CacheIcceService();

class IT_ICCE_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;

    static void STUB_ResetData();
    static void STUB_ConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_IcceConnectState_E curstate,
        NLSTK_IcceConnectState_E prestate, NLSTK_Errcode_E errCode);
    static void STUB_IcceRegCbk();
    static void STUB_IcceConnect();
protected:
    void SetUp() override
    {
        // 步骤1：初始化桩函数
        TEST_ScheduleInit();
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskQueueStub);
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

        // 步骤3：使能ICCE
        IcceClientEnable();
        IcceServerEnable();
        TEST_RunQueueStubSchedule();
    }

    void TearDown() override
    {
        // 步骤4：去使能ICCE
        IcceClientDisable();

        // 步骤5：去初始化SSAP模块
        SSAP_DeInit();
        SSAP_LinkDeInit();

        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

bool g_recvCbk = false;
NLSTK_IcceConnectState_E g_state = ICCE_DISCONNECTED;

void IT_ICCE_TEST::STUB_ResetData()
{
    g_recvCbk = false;
    g_state = ICCE_DISCONNECTED;
}

void IT_ICCE_TEST::STUB_ConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_IcceConnectState_E curstate,
    NLSTK_IcceConnectState_E prestate, NLSTK_Errcode_E errCode)
{
    g_recvCbk = true;
    g_state = curstate;
}

static void STUB_CacheIcceService()
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

void IT_ICCE_TEST::STUB_IcceRegCbk()
{
    // 注册回调
    NLSTK_IcceClientCallBack_S clientCallbacks = {0};
    clientCallbacks.connectStateChangeCbk = &STUB_ConnectStateChangeCbk;

    uint32_t ret = NLSTK_IcceRegisterReadInfoCallBack(&clientCallbacks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
}

void IT_ICCE_TEST::STUB_IcceConnect()
{
    STUB_IcceRegCbk();
    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheIcceService();

    // 调用连接
    uint32_t ret = NLSTK_IcceConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟收到连接流程的SSAP报文
    uint8_t rsp[] = {0x09, 0x03, 0x15, 0x00, 0x00, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_CONNECTED);
    STUB_ResetData();
}

/**
 * @test ICCE_TEST_001
 * @brief 此测试用例用于验证ICCE Profile连接流程，SSAP已有服务结构缓存，未注册状态变化回调函数的情况
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_IcceClientRegCbk注册回调；
 * 2. 调用NLSTK_IcceConnect开启连接；
 * 3. 模拟链路建立成功；
 * 4. 模拟收到连接流程中的SSAP响应报文；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 调用NLSTK_IcceDisconnect断连；
 * 7. 模拟链路断连销毁；
 * 8. 校验是否收到连接状态的回调已断连；
 *
 * @pre
 * - SSAP模块已初始化。
 * - ICCE模块客户端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功
 */
TEST_F(IT_ICCE_TEST, ICCE_TEST_001)
{
    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheIcceService();

    // 调用连接
    uint32_t ret = NLSTK_IcceConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟收到连接流程的SSAP报文
    uint8_t rsp[] = {0x09, 0x03, 0x15, 0x00, 0x00, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();

    ret = NLSTK_IcceDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    TEST_RunQueueStubSchedule();
}

// 此用例覆盖在ICCE_SERVICE_FOUND状态收到链路断连，未注册回调
TEST_F(IT_ICCE_TEST, ICCE_TEST_002)
{
    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheIcceService();

    // 调用连接
    uint32_t ret = NLSTK_IcceConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();
}

// 此用例覆盖在ICCE_DEV_CONNECTED状态收到链路断连，未注册回调
TEST_F(IT_ICCE_TEST, ICCE_TEST_003)
{
    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheIcceService();

    // 调用连接
    uint32_t ret = NLSTK_IcceConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟收到连接流程的SSAP报文
    uint8_t rsp[] = {0x09, 0x03, 0x15, 0x00, 0x00, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();
}

// 此用例覆盖在ICCE_LINK_CONNECTED状态收到链路断连，未注册回调
TEST_F(IT_ICCE_TEST, ICCE_TEST_004)
{
    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheIcceService();

    // 调用连接
    uint32_t ret = NLSTK_IcceConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubScheduleOnce();
    TEST_RunQueueStubScheduleOnce();
    TEST_RunQueueStubScheduleOnce();
    // 运行到ICCE_LINK_CONNECTED状态等待调用NLSTK_SsapClientGetServicesByUuidAsyn进入队列运行

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();
}

/**
 * @test ICCE_TEST_005
 * @brief 此测试用例用于验证ICCE Profile连接流程，SSAP已有服务结构缓存。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_IcceClientRegCbk注册回调；
 * 2. 调用NLSTK_IcceConnect开启连接；
 * 3. 模拟链路建立成功；
 * 4. 模拟收到连接流程中的SSAP响应报文；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 调用NLSTK_IcceDisconnect断连；
 * 7. 模拟链路断连销毁；
 * 8. 校验是否收到连接状态的回调已断连；
 *
 * @pre
 * - SSAP模块已初始化。
 * - ICCE模块客户端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_ICCE_TEST, ICCE_TEST_005)
{
    // 注册回调
    STUB_IcceRegCbk();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheIcceService();

    // 调用连接
    uint32_t ret = NLSTK_IcceConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟收到连接流程的SSAP报文
    uint8_t rsp[] = {0x09, 0x03, 0x15, 0x00, 0x00, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_CONNECTED);
    STUB_ResetData();

    int32_t port = 0;
    ret = NLSTK_IcceGetPort(&g_addr, &port);
    EXPECT_EQ(port, 0x15);

    uint8_t num = 0;
    num = NLSTK_GetConnectionsDeviceNum();
    EXPECT_EQ(num, 0x01);

    ret = NLSTK_IcceDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_DISCONNECTED);
    STUB_ResetData();

    TEST_RunQueueStubSchedule();
}

/**
 * @test ICCE_TEST_006
 * @brief 此测试用例用于验证ICCE服务端管理端口流程。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_IcceCreateIcceInstance添加服务；
 * 2. 校验服务属性、属性数量、属性值；
 * 3. 调用NLSTK_IcceDestroyIcceInstance删除服务；
 * 4. 校验服务是否被删除；
 *
 * @pre
 * - SSAP模块已初始化。
 * - ICCE模块服务端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功。
 */
TEST_F(IT_ICCE_TEST, ICCE_TEST_006)
{
    // 注册回调
    STUB_IcceRegCbk();

    NLSTK_IcceServiceInfo_S info = {};
    info.iccePort = 0x12;
    info.iccePortRight = 0;
    uint32_t ret = NLSTK_IcceCreateIcceInstance(&info);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    EXPECT_EQ(memcmp(&service->uuid, &g_serviceUuid, sizeof(NLSTK_SsapUuid_S)), 0);
    EXPECT_EQ(service->properties->size, 1);
    SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 0);
    uint8_t value0[] = {0x12, 0x00, 0x00, 0x00};
    EXPECT_EQ(memcmp(property->val->value, value0, property->val->len), 0);

    ret = NLSTK_IcceDestroyIcceInstance();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

// 此用例覆盖在ICCE_SERVICE_FOUND状态收到链路断连
TEST_F(IT_ICCE_TEST, ICCE_TEST_007)
{
    // 注册回调
    STUB_IcceRegCbk();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheIcceService();

    // 调用连接
    uint32_t ret = NLSTK_IcceConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_DISCONNECTED);
    STUB_ResetData();

    TEST_RunQueueStubSchedule();
}

// 此用例覆盖在ICCE_DEV_CONNECTED状态收到链路断连
TEST_F(IT_ICCE_TEST, ICCE_TEST_008)
{
    // 注册回调
    STUB_IcceRegCbk();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheIcceService();

    // 调用连接
    uint32_t ret = NLSTK_IcceConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟收到连接流程的SSAP报文
    uint8_t rsp[] = {0x09, 0x03, 0x15, 0x00, 0x00, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_CONNECTED);
    STUB_ResetData();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_DISCONNECTED);
    STUB_ResetData();

    TEST_RunQueueStubSchedule();
}

// 此用例覆盖在ICCE_LINK_CONNECTED状态收到链路断连
TEST_F(IT_ICCE_TEST, ICCE_TEST_009)
{
    // 注册回调
    STUB_IcceRegCbk();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheIcceService();

    // 调用连接
    uint32_t ret = NLSTK_IcceConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubScheduleOnce();
    TEST_RunQueueStubScheduleOnce();
    TEST_RunQueueStubScheduleOnce();
    // 运行到ICCE_LINK_CONNECTED状态等待调用NLSTK_SsapClientGetServicesByUuidAsyn进入队列运行

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_CONNECTING);
    STUB_ResetData();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_DISCONNECTED);
    STUB_ResetData();

    TEST_RunQueueStubSchedule();
}

/**
 * @test ICCE_TEST_010
 * @brief 此测试用例通过直接调用注册的回调构造ICCE连接异常场景
 * @pre
 * - SSAP模块已初始化。
 * - ICCE模块客户端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_ICCE_TEST, ICCE_TEST_010)
{
    // Icce连接流程
    STUB_IcceConnect();
    NLSTK_SsapServ_S service = {0};
    g_regList[0].cb->onGetServices(0, (NLSTK_SsapUuid_S *)&g_serviceUuid, &service, 0, NULL);
}

/**
 * @test ICCE_TEST_011
 * @brief 此测试用例通过直接调用注册的回调构造ICCE连接异常场景
 * @pre
 * - SSAP模块已初始化。
 * - ICCE模块客户端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_ICCE_TEST, ICCE_TEST_011)
{
    // Icce连接流程
    STUB_IcceConnect();
    NLSTK_SsapClientReadPropertyInfo_S property = {0};
    property.value.len = ICCE_PORT_LEN + 1;
    g_regList[0].cb->onReadProperty(0, &property, NLSTK_ERRCODE_SUCCESS);
}

/**
 * @test ICCE_TEST_012
 * @brief 此测试用例覆盖ICCE异常场景，重复使能
 */
TEST_F(IT_ICCE_TEST, ICCE_TEST_012)
{
    IcceClientEnable();
}

/**
 * @test ICCE_TEST_013
 * @brief 此测试用例覆盖ICCE异常场景，去使能后获取连接设备数
 */
TEST_F(IT_ICCE_TEST, ICCE_TEST_013)
{
    IcceClientDisable();
    uint8_t num = NLSTK_GetConnectionsDeviceNum();
    EXPECT_EQ(num, 0);
}

// 此用例覆盖在ICCE_LINK_CONNECTED状态收到用户路断连
TEST_F(IT_ICCE_TEST, ICCE_TEST_014)
{
    // 注册回调
    STUB_IcceRegCbk();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheIcceService();

    // 调用连接
    uint32_t ret = NLSTK_IcceConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubScheduleOnce();
    TEST_RunQueueStubScheduleOnce();
    TEST_RunQueueStubScheduleOnce();
    // 运行到ICCE_LINK_CONNECTED状态等待调用NLSTK_SsapClientGetServicesByUuidAsyn进入队列运行

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_CONNECTING);
    STUB_ResetData();

    // 模拟用户断连
    ret = NLSTK_IcceDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_DISCONNECTED);
    STUB_ResetData();

    TEST_RunQueueStubSchedule();
}

// 此用例覆盖在ICCE_SERVICE_FOUND状态收到用户断连
TEST_F(IT_ICCE_TEST, ICCE_TEST_015)
{
    // 注册回调
    STUB_IcceRegCbk();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CacheIcceService();

    // 调用连接
    uint32_t ret = NLSTK_IcceConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟用户断连
    ret = NLSTK_IcceDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, ICCE_DISCONNECTED);
    STUB_ResetData();

    TEST_RunQueueStubSchedule();
}

/**
 * @test ICCE_TEST_016
 * @brief 此测试用例覆盖ICCE异常场景，获取连接设备数目添加任务失败
 */
TEST_F(IT_ICCE_TEST, ICCE_TEST_016)
{
    EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStubFail);
    uint8_t num = NLSTK_GetConnectionsDeviceNum();
    EXPECT_EQ(num, 0);
}

/**
 * @test ICCE_TEST_017
 * @brief 此测试用例覆盖ICCE异常场景，销毁实例添加任务失败
 */
TEST_F(IT_ICCE_TEST, ICCE_TEST_017)
{
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);
    uint32_t ret = NLSTK_IcceDestroyIcceInstance();
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
}