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

#include <cstring>
#include <stdlib.h>
#include "securec.h"
#include "gtest/gtest.h"
#include "sdf_mem.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"

#include "dtap_tcid.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssaps_service.h"
#include "ssapc_cache.h"
#include "devd_local.h"
#include "devd_tbl.h"

#include "nlstk_ssap_app_client.h"
#include "nlstk_ssap_app_link.h"
#include "nlstk_cfgdb_api.h"
#include "nlstk_cfgdb.h"
#include "dli_callback.h"
#include "cp_worker.h"

#include "cm_logic_link_api.h"

#include "cpfwk_log.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;
static void Stub_SsapClientFindServiceCb(int32_t appId, NLSTK_Errcode_E ret);
static void Stub_SsapClientFindServicesByUuidCb(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_Errcode_E ret);

static void TEST_SsapRecvAcbHandler(uint16_t lcid, SDF_Buff_S *buf)
{
}

static void TestDliCallbackInit(void)
{
    DLI_Callback dliCallback = {0};
    dliCallback.postOtherThread = CP_PostTask;
    dliCallback.postOtherBlockedThread = CP_PostTaskBlocked;
    dliCallback.dftReportKill = NULL;
    dliCallback.recvAcbHandler = TEST_SsapRecvAcbHandler;
    DLI_SetCallback(&dliCallback);
}

class SSAP_CLIENT_FIND_API_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;

    static int32_t g_appId;
    static NLSTK_Errcode_E g_discResult;
    static NLSTK_Errcode_E g_discByUuidResult;
    static NLSTK_SsapUuid_S g_uuid;

    void Stub_SsapResetData(void);

protected:
    void SetUp() override
    {
        // 步骤1：初始化桩函数
        TEST_ScheduleInit();
        TestDliCallbackInit();
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


        // 步骤2：初始化SSAP模块
        SSAP_Init();

        // 步骤3：注册AppId
        Stub_SsapResetData();
        NLSTK_SsapAppClientCb_S cb = {0};
        cb.onFindService = Stub_SsapClientFindServiceCb;
        cb.onFindServiceByUuid = Stub_SsapClientFindServicesByUuidCb;
        int32_t ret = NLSTK_SsapClientRegApp(&g_appId, &cb, &g_addr);
        EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    }

    void TearDown() override
    {

        // 步骤3：去初始化SSAP模块
        NLSTK_SsapClientDeregApp(g_appId);
        SSAP_DeInit();

        // 步骤4：清理桩函数
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

// 静态变量初始化
int32_t SSAP_CLIENT_FIND_API_TEST::g_appId = -1;
NLSTK_Errcode_E SSAP_CLIENT_FIND_API_TEST::g_discResult = NLSTK_ERRCODE_FAIL;
NLSTK_Errcode_E SSAP_CLIENT_FIND_API_TEST::g_discByUuidResult = NLSTK_ERRCODE_FAIL;
NLSTK_SsapUuid_S SSAP_CLIENT_FIND_API_TEST::g_uuid = {0};


void SSAP_CLIENT_FIND_API_TEST::Stub_SsapResetData(void)
{
    g_appId = -1;
    g_discResult = NLSTK_ERRCODE_FAIL;
    g_discByUuidResult = NLSTK_ERRCODE_FAIL;
}

static void Stub_SsapClientFindServiceCb(int32_t appId, NLSTK_Errcode_E ret)
{
    if (appId != SSAP_CLIENT_FIND_API_TEST::g_appId) {
        return;
    }
    SSAP_CLIENT_FIND_API_TEST::g_discResult = ret;
}

static void Stub_SsapClientFindServicesByUuidCb(int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_Errcode_E ret)
{
    if (appId != SSAP_CLIENT_FIND_API_TEST::g_appId) {
        return;
    }
    SSAP_CLIENT_FIND_API_TEST::g_discByUuidResult = ret;
    memcpy_s(&SSAP_CLIENT_FIND_API_TEST::g_uuid, sizeof(NLSTK_SsapUuid_S), uuid, sizeof(NLSTK_SsapUuid_S));
}

static uint8_t reqPkt1[] = {0x04, 0x11, 0x01, 0x00, 0xFF, 0xFF};
static uint8_t reqPkt2[] = {0x04, 0x11, 0x13, 0x00, 0xFF, 0xFF};
static uint8_t reqPkt3[] = {0x04, 0x13, 0x11, 0x00, 0x11, 0x00};

// find回复,op-1,ctrl混合模式-1,{指示-1,{开始句柄-2,结束句柄-2,uuid-2,member-1},...},
// {指示-1,{开始句柄-2,结束句柄-2,uuid-16,member-1},...}
static uint8_t rspPkt1[] = {0x05, 0x0B, 0x01, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02,
    0x81, 0x12, 0x00, 0x12, 0x00, 0x12, 0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A,
    0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x00};

static uint8_t rspPkt2[] = {0x01, 0x00, 0x04, 0x00, 0x00, 0x0B};

// find回复,op-1,ctrl标准模式-1,{开始句柄-2,uuid-2,operation-4,count-1,descriptor-x}
static uint8_t rspPkt3[] = {0x05, 0x03, 0x11, 0x00, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};

/**
 * @test FIND_REQ_001
 * @brief 此测试用例用于验证客户端发送find报文与解析服务端回复报文。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 初始化SSAP模块并注册AppId；
 * 2. 调用NLSTK_SsapClientConnect函数完成建链动作；
 * 3. 模拟链路建立成功；
 * 4. 调用NLSTK_SsapClientDiscoverServices，向服务端请求服务
 * 5. 模拟收包，校验回调
 *
 * @pre
 * - SSAP模块已初始化。
 * - 回调函数已注册。
 *
 * @post
 * - 预期结果：非法AppId应返回错误码。
 */
TEST_F(SSAP_CLIENT_FIND_API_TEST, FIND_REQ_001)
{
    // 完成建链
    NLSTK_Errcode_E ret = NLSTK_SsapClientConnect(g_appId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 调用NLSTK_SsapClientDiscoverServices，向服务端发现服务
    ret = NLSTK_SsapClientDiscoverServices(g_appId, 0x01, 0xFFFF, FIND_STRUCTURE_TYPE_PRIMARY_SERVICE);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);
    // 校验发送报文是否符合预期
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(reqPkt1, sizeof(reqPkt1)));
    // 模拟对端server发送报文，发送find service rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rspPkt1, sizeof(rspPkt1));

    // 下层收到service后，会继续发送find service，直到查不到
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(reqPkt2, sizeof(reqPkt2)));
    // 模拟对端server发送报文，发送error rsp，找不到条目
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rspPkt2, sizeof(rspPkt2));

    // 模拟对端server发送报文，发送find property rsp
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(reqPkt3, sizeof(reqPkt3)));
    // 回发现属性，这个时候cache判断发现完了，会回调上去
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rspPkt3, sizeof(rspPkt3));

    // 校验回调上来的结果
    EXPECT_EQ(g_discResult, NLSTK_ERRCODE_SUCCESS);
    
    NLSTK_SsapServ_S *services = NULL;
    uint16_t serviceNum = 0;
    NLSTK_SsapClientFreeFunc freeFunc = NULL;
    ret = NLSTK_SsapClientGetServices(g_appId, &services, &serviceNum, &freeFunc);
    ASSERT_TRUE(services != NULL);
    ASSERT_EQ(serviceNum, 2);
    EXPECT_TRUE(freeFunc != NULL);
    if (freeFunc != NULL) {
        freeFunc(services, serviceNum);
    }

    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
}

static NLSTK_SsapUuid_S g_uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};

static uint8_t reqPkt4[] = {0x06, 0x11, 0x01, 0x00, 0xFF, 0xFF, 0x02, 0x01};
static uint8_t rspPkt4[] = {0x07, 0x03, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02};

static uint8_t reqPkt5[] = {0x06, 0x11, 0x12, 0x00, 0xFF, 0xFF, 0x02, 0x01};
static uint8_t rspPkt5[] = {0x01, 0x00, 0x06, 0x00, 0x00, 0x0B};

static uint8_t reqPkt6[] = {0x04, 0x13, 0x11, 0x00, 0x11, 0x00};
static uint8_t rspPkt6[] = {0x05, 0x03, 0x11, 0x00, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};

/**
 * @test FIND_BY_UUID_REQ_001
 * @brief 此测试用例用于验证客户端发送find报文与解析服务端回复报文。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 初始化SSAP模块并注册AppId；
 * 2. 调用NLSTK_SsapClientConnect函数完成建链动作；
 * 3. 模拟链路建立成功；
 * 4. 调用NLSTK_SsapClientDiscoverServicesByUuid，向服务端请求服务
 * 5. 模拟收包，校验回调
 *
 * @pre
 * - SSAP模块已初始化。
 * - 回调函数已注册。
 *
 * @post
 * - 预期结果：非法AppId应返回错误码。
 */
TEST_F(SSAP_CLIENT_FIND_API_TEST, FIND_BY_UUID_REQ_001)
{
    // 完成建链
    NLSTK_Errcode_E ret = NLSTK_SsapClientConnect(g_appId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 调用NLSTK_SsapClientDiscoverServicesByUuid，向服务端发现服务
    ret = NLSTK_SsapClientDiscoverServicesByUuid(g_appId, &g_uuid1, 0x01, 0xFFFF, FIND_STRUCTURE_TYPE_PRIMARY_SERVICE);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);
    // 校验发送报文是否符合预期
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(reqPkt4, sizeof(reqPkt4)));
    // 模拟对端server发送报文，发送find service rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rspPkt4, sizeof(rspPkt4));

    // 下层收到service后，会继续发送find service by uuid，直到查不到
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(reqPkt5, sizeof(reqPkt5)));
    // 模拟对端server发送报文，发送error rsp，找不到条目
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rspPkt5, sizeof(rspPkt5));

    // 模拟对端server发送报文，发送find property rsp
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(reqPkt6, sizeof(reqPkt6)));
    // 回发现属性，这个时候cache判断发现完了，会回调上去
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rspPkt6, sizeof(rspPkt6));

    // 校验回调上来的结果
    EXPECT_EQ(g_discByUuidResult, NLSTK_ERRCODE_SUCCESS);

    NLSTK_SsapServ_S *services = NULL;
    uint16_t serviceNum = 0;
    NLSTK_SsapClientFreeFunc freeFunc = NULL;
    ret = NLSTK_SsapClientGetServicesByUuid(g_appId, &g_uuid1, &services, &serviceNum, &freeFunc);
    ASSERT_TRUE(services != NULL);
    EXPECT_EQ(serviceNum, 1);
    EXPECT_TRUE(freeFunc != NULL);
    if (freeFunc != NULL) {
        freeFunc(services, serviceNum);
    }

    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
}

static uint8_t reqPkt7[] = {0x04, 0x10, 0x01, 0x00, 0xFF, 0xFF};
static uint8_t rspPkt7[] = {0x05, 0x0B,
0x10, 0x00, 0x00, 0x12, 0x23, 0x00,
0x12, 0x00, 0x02, 0x23, 0x34, 0x09, 0x02, 0x00, 0x00, 0x01, 0x02,
0x13, 0x00, 0x04, 0x34, 0x45, 0x09, 0x02, 0x00, 0x00, 0x00,
0x11, 0x00, 0x02, 0x45, 0x56, 0x09, 0x02, 0x00, 0x00, 0x01, 0x01,
0x20, 0x00, 0x08, 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x12, 0x23, 0x00,
0x25, 0x00, 0x0A, 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x23, 0x34,
    0x09, 0x02, 0x00, 0x00, 0x01, 0x02,
0x23, 0x00, 0x0B, 0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA, 0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x34, 0x45,
    0x09, 0x02, 0x00, 0x00, 0x00};

static uint8_t errPkt7[] = {0x01, 0x00, 0x04, 0x00, 0x00, 0x02};

/**
 * @test FIND_SERVICE_STRUCTURE
 * @brief 此测试用例用于验证客户端发送find报文与解析服务端回复报文。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 初始化SSAP模块并注册AppId；
 * 2. 调用NLSTK_SsapClientConnect函数完成建链动作；
 * 3. 模拟链路建立成功；
 * 4. 调用NLSTK_SsapClientDiscoverServices，向服务端请求服务
 * 5. 模拟收包，校验回调
 *
 * @pre
 * - SSAP模块已初始化。
 * - 回调函数已注册。
 *
 * @post
 * - 预期结果：非法AppId应返回错误码。
 */
TEST_F(SSAP_CLIENT_FIND_API_TEST, FIND_SERVICE_STRUCTURE)
{
    // 完成建链
    NLSTK_Errcode_E ret1 = NLSTK_SsapClientConnect(g_appId);
    EXPECT_EQ(ret1, NLSTK_ERRCODE_SUCCESS);
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    (void)CfgdbInit();

    // 设置设备支持服务结构发现能力位图，CFGDB_FIND_SERVICE_STRUCTURE
    NLSTK_ManufacturerAbility_S ability = {.ability[0] = 1};
    uint32_t ret2 = NLSTK_CfgdbSetManufacturerAbility(&g_addr, &ability);

    NLSTK_Errcode_E ret3 = NLSTK_SsapClientDiscoverServices(g_appId, 0x01, 0xFFFF, FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE);
    EXPECT_EQ(ret3, NLSTK_ERRCODE_SUCCESS);
    // 模拟收到init初始报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);
    // 校验发送报文是否符合预期
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(reqPkt7, sizeof(reqPkt7)));
    // 模拟对端server发送报文，发送find service rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rspPkt7, sizeof(rspPkt7));

    // 校验回调上来的结果
    EXPECT_EQ(g_discResult, NLSTK_ERRCODE_SUCCESS);

    NLSTK_SsapServ_S *services = NULL;
    uint16_t serviceNum = 0;
    NLSTK_SsapClientFreeFunc freeFunc = NULL;
    NLSTK_Errcode_E ret4 = NLSTK_SsapClientGetServices(g_appId, &services, &serviceNum, &freeFunc);
    EXPECT_EQ(ret4, NLSTK_ERRCODE_SUCCESS);
    ASSERT_TRUE(services != NULL);
    ASSERT_EQ(serviceNum, 2);
    EXPECT_EQ(services[0].endHandle, 0x0013);
    EXPECT_EQ(services[0].propertyNum, 2);
    EXPECT_EQ(services[0].methodNum, 0);
    EXPECT_EQ(services[0].eventNum, 1);
 
    EXPECT_EQ(services[1].endHandle, 0x0025);
    EXPECT_EQ(services[1].propertyNum, 1);
    EXPECT_EQ(services[1].methodNum, 1);
    EXPECT_EQ(services[1].eventNum, 0);
    freeFunc(services, serviceNum);
    (void)CfgdbDeinit();
}

TEST_F(SSAP_CLIENT_FIND_API_TEST, FIND_SERVICE_STRUCTURE_ERROR)
{
    // 完成建链
    NLSTK_Errcode_E ret1 = NLSTK_SsapClientConnect(g_appId);
    EXPECT_EQ(ret1, NLSTK_ERRCODE_SUCCESS);
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    (void)CfgdbInit();

    // 设置设备支持服务结构发现能力位图，CFGDB_FIND_SERVICE_STRUCTURE
    NLSTK_ManufacturerAbility_S ability = {.ability[0] = 1};
    uint32_t ret2 = NLSTK_CfgdbSetManufacturerAbility(&g_addr, &ability);

    NLSTK_Errcode_E ret3 = NLSTK_SsapClientDiscoverServices(g_appId, 0x01, 0xFFFF, FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE);
    EXPECT_EQ(ret3, NLSTK_ERRCODE_SUCCESS);
    // 模拟收到init初始报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);
    // 校验发送报文是否符合预期
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(reqPkt7, sizeof(reqPkt7)));
    // 模拟对端server发送报文，发送find service rsp
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, errPkt7, sizeof(errPkt7));

    // 校验回调上来的结果
    EXPECT_NE(g_discResult, NLSTK_ERRCODE_SUCCESS);

    NLSTK_SsapServ_S *services = NULL;
    uint16_t serviceNum = 0;
    NLSTK_SsapClientFreeFunc freeFunc = NULL;
    NLSTK_Errcode_E ret4 = NLSTK_SsapClientGetServices(g_appId, &services, &serviceNum, &freeFunc);
    EXPECT_EQ(ret4, NLSTK_ERRCODE_SUCCESS);
    EXPECT_TRUE(services == NULL);
    EXPECT_EQ(serviceNum, 0);
    if (freeFunc != NULL) {
        freeFunc(services, serviceNum);
    }
    (void)CfgdbDeinit();
}