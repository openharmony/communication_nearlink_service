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

#include "cm_def.h"
#include "dtap_tcid.h"
#include "ssap_link.h"
#include "ssapc_cache.h"
#include "ssap_manager.h"
#include "ssaps_service.h"
#include "ssapc_app.h"

#include "port_type.h"
#include "port_stm.h"
#include "port_client.h"
#include "port_server.h"
#include "port_client_init.h"
#include "port_server_init.h"
#include "nlstk_port_client.h"
#include "nlstk_port_server.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

extern SsapcAppRegParam_S *g_regList;
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
static uint16_t g_lcid = 1;

static uint8_t g_serviceUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x60};
static uint8_t g_propertyUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x11, 0x01};

static void STUB_CacheHidService();

class IT_PORT_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
    static void STUB_ResetData();

    static void STUB_ConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_PortConnectState_E curState,
                                           NLSTK_PortConnectState_E preState, NLSTK_Errcode_E ret);

    static bool g_recvCbk;
    static NLSTK_PortConnectState_E g_state;
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

        // 步骤3：使能PORT模块
        PortClientEnable();
        STUB_ResetData();

        NLSTK_PortClientCallBack_S clientCallbacks = {0};
        clientCallbacks.connectStateChangeCbk = &STUB_ConnectStateChangeCbk;

        NLSTK_Errcode_E ret = NLSTK_PortClientRegCbk(&clientCallbacks);
        EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
        TEST_RunQueueStubSchedule();
    }

    void TearDown() override
    {
        NLSTK_PortClientDeregCbk();
        TEST_RunQueueStubSchedule();
        // 步骤4：去使能PORT模块客户端
        PortClientDisable();

        // 步骤5：去初始化SSAP模块
        SSAP_DeInit();
        SSAP_LinkDeInit();

        // 步骤6：清理桩函数
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

bool IT_PORT_TEST::g_recvCbk = false;
NLSTK_PortConnectState_E IT_PORT_TEST::g_state = PORT_DISCONNECTED;

void IT_PORT_TEST::STUB_ResetData()
{
    g_recvCbk = false;
    g_state = PORT_DISCONNECTED;
}

void IT_PORT_TEST::STUB_ConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_PortConnectState_E curState,
                                              NLSTK_PortConnectState_E preState, NLSTK_Errcode_E ret)
{
    g_recvCbk = true;
    g_state = curState;
}

static void STUB_CachePortService()
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

/**
 * @test PORT_TEST_001
 * @brief 此测试用例用于验证PORT Profile连接流程，SSAP已有服务结构缓存。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_PortClientRegCbk注册回调；
 * 2. 调用NLSTK_PortConnect开启连接；
 * 3. 模拟链路建立成功；
 * 4. 模拟收到连接流程中的SSAP响应报文；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 调用NLSTK_PortDisconnect断连；
 * 7. 模拟链路断连销毁；
 * 8. 校验是否收到连接状态的回调已断连；
 * 9. 调用NLSTK_PortClientDeregCbk解注册回调；
 *
 * @pre
 * - SSAP模块已初始化。
 * - PORT模块客户端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_PORT_TEST, PORT_TEST_001)
{
    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CachePortService();

    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟收到连接流程的SSAP报文
    uint8_t rsp[] = {0x09, 0x03, 0x01, 0x00, 0x10, 0x00, 0x20, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();

    // cpcd流程
    uint8_t cpcd[] = {0x0E, 0x03, 0x11, 0x00, 0x02, 0x01, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, cpcd, sizeof(cpcd));
    TEST_RunQueueStubSchedule();

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTED);
    STUB_ResetData();

    int state = PORT_DISCONNECTED;
    ret = NLSTK_PortGetConnectState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, PORT_CONNECTED);

    uint16_t portId = 0;
    NLSTK_SsapUuid_S uuid = { .uuid = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10} };
    ret = NLSTK_PortGetDevicePortIdByUuid(&g_addr, &uuid, &portId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(portId, 0x10);

    int num = 0;
    ret = NLSTK_PortGetConnectDeviceNum(&g_addr, &num);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(num, 1);

    ret = NLSTK_PortDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

/**
 * @test PORT_TEST_002
 * @brief 此测试用例用于验证PORT Profile连接流程，SSAP无服务结构缓存。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_PortClientRegCbk注册回调；
 * 2. 调用NLSTK_PortConnect开启连接；
 * 3. 模拟链路建立成功；
 * 4. 模拟收到连接流程中的SSAP响应报文（包含服务发现）；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 调用NLSTK_PortDisconnect断连；
 * 7. 模拟链路断连销毁；
 * 8. 校验是否收到连接状态的回调已断连；
 * 9. 调用NLSTK_PortClientDeregCbk解注册回调；
 *
 * @pre
 * - SSAP模块已初始化。
 * - PORT模块客户端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(IT_PORT_TEST, PORT_TEST_002)
{
    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 调用连接
    NLSTK_ConnParam_S connParam = {0};
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟收到服务发现流程的SSAP报文
    uint8_t findService[] = {0x07, 0x0B, /* 服务信息指示 */0x01, /* 起始句柄 */0x10, 0x00, /* 结束句柄 */0x11, 0x00,
    /* 服务标识 */0x60, 0x06, /* 服务成员类型指示 */0x02};
    uint8_t inexist[] = {0x01, 0x00, 0x06, 0x12, 0x00, 0x0B};
    uint8_t findProperty[] = {0x05, 0x0B, /* 服务信息指示 */0x01, /* 属性句柄 */0x11, 0x00,
    /* 属性标识 */0x01, 0x11, /* 操作指示 */0x01, 0x00, 0x00, 0x00, /* 描述符类型列表 */0x01, 0x02};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, findService, sizeof(findService));
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, inexist, sizeof(inexist));
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, findProperty, sizeof(findProperty));
    TEST_RunQueueStubSchedule();

    // 模拟收到连接流程的SSAP报文
    uint8_t rsp[] = {0x09, 0x03, 0x01, 0x00, 0x10, 0x00, 0x20, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();

    // cpcd流程
    uint8_t cpcd[] = {0x0E, 0x03, 0x11, 0x00, 0x02, 0x01, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, cpcd, sizeof(cpcd));
    TEST_RunQueueStubSchedule();

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTED);
    STUB_ResetData();

    int state = PORT_DISCONNECTED;
    ret = NLSTK_PortGetConnectState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, PORT_CONNECTED);

    uint16_t portId = 0;
    NLSTK_SsapUuid_S uuid = { .uuid = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10} };
    ret = NLSTK_PortGetDevicePortIdByUuid(&g_addr, &uuid, &portId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(portId, 0x10);

    int num = 0;
    ret = NLSTK_PortGetConnectDeviceNum(&g_addr, &num);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(num, 1);

    ret = NLSTK_PortDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

/**
 * @test PORT_TEST_003
 * @brief 此测试用例用于验证PORT服务端管理端口流程。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用PortServerEnable使能服务端；
 * 2. 校验服务属性、属性数量、属性值是否符合预期；
 * 3. 调用NLSTK_PortAddByUuid添加端口号；
 * 4. 校验属性值是否符合预期；
 * 5. 调用NLSTK_PortDeleteByUuid删除端口号；
 * 6. 校验属性值是否符合预期；
 * 7. 调用PortServerDisable去使能服务端；
 * 8. 校验服务是否被删除；
 *
 * @pre
 * - SSAP模块已初始化。
 * - PORT模块服务端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功。
 */
TEST_F(IT_PORT_TEST, PORT_TEST_003)
{
    NLSTK_Errcode_E ret = PortServerEnable();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    EXPECT_EQ(memcmp(&service->uuid, &g_portServiceStdUuid, sizeof(NLSTK_SsapUuid_S)), 0);
    EXPECT_EQ(service->properties->size, 1);
    SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 0);
    uint8_t value0[] = {0x00, 0x00};
    EXPECT_EQ(memcmp(property->val->value, value0, property->val->len), 0);

    NLSTK_SsapUuid_S uuid1 = { .uuid = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10} };
    uint16_t manufactureId1 = 0x01;
    uint16_t portId1 = 0x02;
    ret = NLSTK_PortAddByUuid(&uuid1, manufactureId1, portId1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    EXPECT_EQ(service->properties->size, 1);
    property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 0);
    uint8_t value1[] = {0x01, 0x00,
        0x02, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    EXPECT_EQ(memcmp(property->val->value, value1, property->val->len), 0);

    NLSTK_SsapUuid_S uuid2 = { .uuid = { 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
        0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01} };
    uint16_t manufactureId2 = 0x03;
    uint16_t portId2 = 0x04;
    ret = NLSTK_PortAddByUuid(&uuid2, manufactureId2, portId2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    EXPECT_EQ(service->properties->size, 1);
    property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 0);
    uint8_t value2[] = {0x02, 0x00,
        0x02, 0x00, 0x01, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
        0x04, 0x00, 0x03, 0x00, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
        0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
    EXPECT_EQ(memcmp(property->val->value, value2, property->val->len), 0);

    uint16_t manufactureId3 = 0x05;
    uint16_t portId3 = 0x06;
    ret = NLSTK_PortAddByUuid(&uuid1, manufactureId3, portId3);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    EXPECT_EQ(service->properties->size, 1);
    property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 0);
    uint8_t value3[] = {0x02, 0x00,
        0x06, 0x00, 0x05, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
        0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
        0x04, 0x00, 0x03, 0x00, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
        0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
    EXPECT_EQ(memcmp(property->val->value, value3, property->val->len), 0);

    ret = NLSTK_PortDeleteByUuid(&uuid1);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    EXPECT_EQ(service->properties->size, 1);
    property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 0);
    uint8_t value4[] = {0x01, 0x00,
        0x04, 0x00, 0x03, 0x00, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
        0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01};
    EXPECT_EQ(memcmp(property->val->value, value4, property->val->len), 0);

    ret = NLSTK_PortDeleteByUuid(&uuid2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    EXPECT_EQ(service->properties->size, 1);
    property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 0);
    EXPECT_EQ(memcmp(property->val->value, value0, property->val->len), 0);

    PortServerDisable();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test PORT_TEST_004
 * @brief 此测试用例用于验证PORT重复调用客户端使能。
 *
 * @pre
 * - SSAP模块已初始化。
 * - PORT模块客户端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功。
 */
TEST_F(IT_PORT_TEST, PORT_TEST_004)
{
    NLSTK_Errcode_E ret = PortClientEnable();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
}

/**
 * @test PORT_TEST_005
 * @brief 此测试用例用于验证PORT客户端未注册状态变化回调，发生状态变化。
 *
 * @pre
 * - SSAP模块已初始化。
 * - PORT模块客户端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功。
 */
TEST_F(IT_PORT_TEST, PORT_TEST_005)
{
    NLSTK_PortClientDeregCbk();
    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CachePortService();

    STUB_ResetData();
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期不会收到回调
    EXPECT_EQ(g_recvCbk, false);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();

    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();
}

/**
 * @test PORT_TEST_006
 * @brief 此测试用例用于验证PORT重复调用客户端连接。
 *
 * @pre
 * - SSAP模块已初始化。
 * - PORT模块客户端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功。
 */
TEST_F(IT_PORT_TEST, PORT_TEST_006)
{
    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CachePortService();

    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    // 模拟收到连接流程的SSAP报文
    uint8_t rsp[] = {0x09, 0x03, 0x01, 0x00, 0x10, 0x00, 0x20, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();

    // cpcd流程
    uint8_t cpcd[] = {0x0E, 0x03, 0x11, 0x00, 0x02, 0x01, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, cpcd, sizeof(cpcd));
    TEST_RunQueueStubSchedule();

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTED);
    STUB_ResetData();

    // 再次调用连接
    ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调已断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

/**
 * @test PORT_TEST_007
 * @brief 此测试用例用于验证PORT未建立连接，获取连接状态
 *
 * @pre
 * - SSAP模块已初始化。
 * - PORT模块客户端已使能。
 *
 * @post
 * - 预期结果：调用返回值成功。
 */
TEST_F(IT_PORT_TEST, PORT_TEST_007)
{
    NLSTK_Errcode_E ret = PortClientEnable();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    int state = PORT_CONNECTED;
    ret = NLSTK_PortGetConnectState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中在PORT_STATE_CREATE_LINK状态收到用户断连
TEST_F(IT_PORT_TEST, PORT_TEST_009)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 在PortCreateLinkStateDispatch状态时调用断连
    ret = NLSTK_PortDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();

    int state = PORT_DISCONNECTED;
    ret = NLSTK_PortGetConnectState(&g_addr, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, PORT_DISCONNECTED);

    int num = 0;
    ret = NLSTK_PortGetConnectDeviceNum(&g_addr, &num);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(num, 0);
}

// 模拟在连接过程中在PORT_STATE_CREATE_LINK状态收到底层链路断连
TEST_F(IT_PORT_TEST, PORT_TEST_010)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    int state = -1;
    ret = NLSTK_PortGetConnectState(&g_addr, &state);
    EXPECT_EQ(state, PORT_CONNECTING);

    // 模拟链路断连
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中在PORT_STATE_GET_SERVICE状态收到底层链路断连
TEST_F(IT_PORT_TEST, PORT_TEST_011)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟链路断连
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中在PORT_STATE_GET_SERVICE状态收到用户断连
TEST_F(IT_PORT_TEST, PORT_TEST_012)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 在PortGetServiceStateDispatch状态时调用断连
    ret = NLSTK_PortDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);
    // NLSTK_PortDisconnect先进队列，NLSTK_SsapClientGetServicesByUuidAsyn后进队列
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中在PORT_STATE_READ_PROPERTY状态收到底层链路断连
TEST_F(IT_PORT_TEST, PORT_TEST_013)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CachePortService();
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中在PORT_STATE_READ_PROPERTY状态收到用户断连
TEST_F(IT_PORT_TEST, PORT_TEST_014)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CachePortService();
    TEST_RunQueueStubSchedule();

    // 在PortReadPropertyStateDispatch状态时调用断连
    ret = NLSTK_PortDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中无服务缓存，进入PORT_STATE_FIND_SERVICE收到底层链路断连
TEST_F(IT_PORT_TEST, PORT_TEST_015)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);
    // 无服务缓存
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中无服务缓存，进入PORT_STATE_FIND_SERVICE收到用户断连
TEST_F(IT_PORT_TEST, PORT_TEST_016)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);
    // 无服务缓存
    TEST_RunQueueStubSchedule();

    // 在PortFindServiceStateDispatch状态时调用断连
    ret = NLSTK_PortDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中在PORT_STATE_CREATE_LINK状态收到OnPortRegisterApp
TEST_F(IT_PORT_TEST, PORT_TEST_017)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    g_regList[0].cb->onRegisterApp(0, &g_addr, NLSTK_ERRCODE_SUCCESS);

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中在PORT_STATE_GET_SERVICE状态收到OnPortRegisterApp
TEST_F(IT_PORT_TEST, PORT_TEST_018)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    g_regList[0].cb->onRegisterApp(0, &g_addr, NLSTK_ERRCODE_SUCCESS);

    // 模拟链路断连
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中无服务缓存，进入PORT_STATE_FIND_SERVICE收到OnPortRegisterApp
TEST_F(IT_PORT_TEST, PORT_TEST_019)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);
    // 无服务缓存
    TEST_RunQueueStubSchedule();

    g_regList[0].cb->onRegisterApp(0, &g_addr, NLSTK_ERRCODE_SUCCESS);

    // 模拟链路断连
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中在PORT_STATE_READ_PROPERTY状态收到OnPortRegisterApp
TEST_F(IT_PORT_TEST, PORT_TEST_020)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    // 模拟服务发现已有服务缓存
    STUB_CachePortService();
    TEST_RunQueueStubSchedule();

    g_regList[0].cb->onRegisterApp(0, &g_addr, NLSTK_ERRCODE_SUCCESS);

    // 模拟链路断连
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中在PORT_STATE_GET_SERVICE状态收到onGetServices后退出
TEST_F(IT_PORT_TEST, PORT_TEST_021)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    NLSTK_SsapServ_S service = {0};
    uint16_t serviceNum = 0;
    g_regList[0].cb->onGetServices(0, NULL, &service, serviceNum, NULL);

    // 模拟链路断连
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中在PORT_STATE_GET_SERVICE状态收到onGetServices，property为空
TEST_F(IT_PORT_TEST, PORT_TEST_022)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    NLSTK_SsapServ_S service = {0};
    uint16_t serviceNum = 1;
    g_regList[0].cb->onGetServices(0, NULL, &service, serviceNum, NULL);

    // 模拟链路断连
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}

// 模拟在连接过程中在PORT_STATE_GET_SERVICE状态收到onGetServices，进入PORT_STATE_READ_PROPERTY
TEST_F(IT_PORT_TEST, PORT_TEST_023)
{
    NLSTK_ConnParam_S connParam = {0};
    // 调用连接
    NLSTK_Errcode_E ret = NLSTK_PortConnect(&g_addr, &connParam);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_CONNECTING);
    STUB_ResetData();

    // 模拟链路建立
    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    NLSTK_SsapServ_S service = {0};
    NLSTK_SsapPrty_S property = {0};
    service.properties = &property;
    uint16_t serviceNum = 1;
    g_regList[0].cb->onGetServices(0, NULL, &service, serviceNum, NULL);

    // 模拟链路断连
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, PORT_DISCONNECTED);
    STUB_ResetData();
}