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
#include "securec.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"

#include "sdf_addr.h"
#include "sdf_mem.h"
#include "nlstk_log.h"

#include "cm_def.h"
#include "dtap_tcid.h"
#include "ssap_link.h"
#include "ssapc_cache.h"
#include "ssap_manager.h"
#include "ssaps_service.h"
#include "nlstk_ssap_app_link.h"

#include "bas_def.h"
#include "bas_type.h"
#include "bas_client.h"
#include "bas_common.h"
#include "nlstk_bas_client.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
static uint16_t g_lcid = 1;

static uint8_t g_basServiceUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x0A};
static uint8_t g_batteryPercentageUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x34};
static uint8_t g_batteryCapacityUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x36};
static uint8_t g_batteryRateCapacityUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x37};
static uint8_t g_remainWorkingTimeUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38};

static uint8_t g_basServiceUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x0A, 0x06, 0x00};
static uint8_t g_batteryPercentageUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x34, 0x10, 0x00};
static uint8_t g_batteryCapacityUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x36, 0x10, 0x00};
static uint8_t g_batteryRateCapacityUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x37, 0x10, 0x00};
static uint8_t g_remainWorkingTimeUuidPen[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x38, 0x10, 0x00};

static void STUB_CacheBasService();

class IT_BAS_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;

    static void STUB_ResetData();

    static void STUB_ReadPropertyCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value, NLSTK_Errcode_E ret);
    static void STUB_ConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_BasConnectState_E curState,
                                           NLSTK_BasConnectState_E prevState, NLSTK_Errcode_E errNumb);
    static void STUB_PropertyChangedCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value);

    static SLE_Addr_S g_recvAddr;
    static bool g_recvCbk;
    static NLSTK_BasConnectState_E g_state;
protected:
    void SetUp() override
    {
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

        SSAP_LinkInit();
        SSAP_Init();

        BasEnable();
        STUB_ResetData();
    }

    void TearDown() override
    {
        BasDisable();

        SSAP_DeInit();
        SSAP_LinkDeInit();

        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

bool IT_BAS_TEST::g_recvCbk = false;
NLSTK_BasConnectState_E IT_BAS_TEST::g_state = BAS_DISCONNECTED;

void IT_BAS_TEST::STUB_ResetData()
{
    g_recvCbk = false;
    g_state = BAS_DISCONNECTED;
}

void IT_BAS_TEST::STUB_ReadPropertyCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value, NLSTK_Errcode_E ret)
{
    g_recvCbk = true;
    (void)addr;
    (void)type;
    (void)value;
    (void)ret;
}

void IT_BAS_TEST::STUB_ConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_BasConnectState_E curState,
                                             NLSTK_BasConnectState_E prevState, NLSTK_Errcode_E errNumb)
{
    g_recvCbk = true;
    g_state = curState;
    (void)addr;
    (void)prevState;
    (void)errNumb;
}

void IT_BAS_TEST::STUB_PropertyChangedCbk(SLE_Addr_S *addr, BasPropertyType_E type, void *value)
{
    g_recvCbk = true;
    (void)addr;
    (void)type;
    (void)value;
}

static void STUB_CacheBasService()
{
    SsapCacheServInfo_S serv = {0};
    serv.handle = 0x10;
    serv.endHandle = 0x18;
    serv.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serv.uuid, sizeof(NLSTK_SsapUuid_S), &g_basServiceUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheServ(&g_addr, &serv);

    NLSTK_SsapPrty_S batteryPct = {0};
    batteryPct.handle = 0x11;
    memcpy_s(&batteryPct.uuid, sizeof(NLSTK_SsapUuid_S), &g_batteryPercentageUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &batteryPct);

    NLSTK_SsapPrty_S batteryCapacity = {0};
    batteryCapacity.handle = 0x12;
    memcpy_s(&batteryCapacity.uuid, sizeof(NLSTK_SsapUuid_S), &g_batteryCapacityUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &batteryCapacity);

    NLSTK_SsapPrty_S batteryRateCapacity = {0};
    batteryRateCapacity.handle = 0x13;
    memcpy_s(&batteryRateCapacity.uuid, sizeof(NLSTK_SsapUuid_S), &g_batteryRateCapacityUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &batteryRateCapacity);

    NLSTK_SsapPrty_S remainWorkingTime = {0};
    remainWorkingTime.handle = 0x14;
    memcpy_s(&remainWorkingTime.uuid, sizeof(NLSTK_SsapUuid_S), &g_remainWorkingTimeUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &remainWorkingTime);

    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_PROPERTY, 0x14);
    SsapcCacheServDiscFinish(&g_addr);
}

static void STUB_CacheBasServicePen()
{
    SsapCacheServInfo_S serv = {0};
    serv.handle = 0x10;
    serv.endHandle = 0x18;
    serv.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serv.uuid, sizeof(NLSTK_SsapUuid_S), &g_basServiceUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheServ(&g_addr, &serv);

    NLSTK_SsapPrty_S batteryPct = {0};
    batteryPct.handle = 0x11;
    memcpy_s(&batteryPct.uuid, sizeof(NLSTK_SsapUuid_S), &g_batteryPercentageUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &batteryPct);

    NLSTK_SsapPrty_S batteryCapacity = {0};
    batteryCapacity.handle = 0x12;
    memcpy_s(&batteryCapacity.uuid, sizeof(NLSTK_SsapUuid_S), &g_batteryCapacityUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &batteryCapacity);

    NLSTK_SsapPrty_S batteryRateCapacity = {0};
    batteryRateCapacity.handle = 0x13;
    memcpy_s(&batteryRateCapacity.uuid, sizeof(NLSTK_SsapUuid_S), &g_batteryRateCapacityUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &batteryRateCapacity);

    NLSTK_SsapPrty_S remainWorkingTime = {0};
    remainWorkingTime.handle = 0x14;
    memcpy_s(&remainWorkingTime.uuid, sizeof(NLSTK_SsapUuid_S), &g_remainWorkingTimeUuidPen, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &remainWorkingTime);

    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_PROPERTY, 0x14);
    SsapcCacheServDiscFinish(&g_addr);
}

/**
 * @test BAS_TEST_001
 * @brief 此测试用例用于验证BAS Profile连接流程对端设备UUID后两字节为大端序。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_BasRegisterCallBack注册回调；
 * 2. 调用NLSTK_BasProfileConnect开启连接；
 * 3. 模拟链路建立成功；
 * 4. 模拟收到连接流程中的SSAP响应报文；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 调用NLSTK_BasProfileDisconnect断连；
 * 7. 模拟链路断连销毁；
 * 8. 校验是否收到连接状态的回调已断连；
 *
 * @pre
 * - SSAP模块已初始化。
 * - BAS模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功对应回调函数触发。
 */
TEST_F(IT_BAS_TEST, BAS_TEST_001)
{
    BasClientCallBack_S clientCallbacks = {0};
    clientCallbacks.readPropertyCbk = &STUB_ReadPropertyCbk;
    clientCallbacks.connectStateChangeCbk = &STUB_ConnectStateChangeCbk;
    clientCallbacks.propertyChangedCbk = &STUB_PropertyChangedCbk;

    NLSTK_Errcode_E ret = NLSTK_BasRegisterCallBack(&clientCallbacks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState1 = {.lcid = g_lcid, .result = 0};
    (void)memcpy_s(&linkState1.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState1);

    STUB_CacheBasService();

    ret = NLSTK_BasProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, BAS_CONNECTING);
    STUB_ResetData();

    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    uint8_t desc[] = {0x09, 0x03, 0x01};
    uint8_t index1[] = {0x09, 0x03, 0x01, 0x01, 0x16, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t index2[] = {0x09, 0x03, 0x01, 0x02, 0x17, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t index3[] = {0x09, 0x03, 0x01, 0x03, 0x18, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, desc, sizeof(desc));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, index1, sizeof(index1));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, index2, sizeof(index2));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, index3, sizeof(index3));
    TEST_RunQueueStubSchedule();

    uint8_t cpcd[] = {0x0E, 0x03, 0x16, 0x00, 0x02, 0x01, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, cpcd, sizeof(cpcd));
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, BAS_CONNECTED);
    STUB_ResetData();

    uint8_t num = 0;
    ret = NLSTK_GetConnectedBasDeviceNum(&num);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(num, 1);

    ret = NLSTK_GetBatteryLevel(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    uint8_t notifyData[] = {0x0F, 0x03, 0x16, 0x00, 0x04, 0x00, 0x04, 0x64};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, notifyData, sizeof(notifyData));
    EXPECT_EQ(g_recvCbk, true);
    STUB_ResetData();

    ret = NLSTK_BasProfileDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, BAS_DISCONNECTED);
    STUB_ResetData();
}

/**
 * @test BAS_TEST_002
 * @brief 此测试用例用于验证BAS Profile连接流程对端设备UUID后两字节为小端序。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_BasRegisterCallBack注册回调；
 * 2. 调用NLSTK_BasProfileConnect开启连接；
 * 3. 模拟链路建立成功；
 * 4. 模拟收到连接流程中的SSAP响应报文；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 调用NLSTK_BasProfileDisconnect断连；
 * 7. 模拟链路断连销毁；
 * 8. 校验是否收到连接状态的回调已断连；
 *
 * @pre
 * - SSAP模块已初始化。
 * - BAS模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功对应回调函数触发。
 */
TEST_F(IT_BAS_TEST, BAS_TEST_002)
{
    BasClientCallBack_S clientCallbacks = {0};
    clientCallbacks.readPropertyCbk = &STUB_ReadPropertyCbk;
    clientCallbacks.connectStateChangeCbk = &STUB_ConnectStateChangeCbk;
    clientCallbacks.propertyChangedCbk = &STUB_PropertyChangedCbk;

    NLSTK_Errcode_E ret = NLSTK_BasRegisterCallBack(&clientCallbacks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    uint8_t state = BAS_CONNECTED;
    ret = NLSTK_GetConnectedBasDeviceNum(&state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(state, 0);

    SLE_Addr_S *addrs = nullptr;
    size_t num = 0;

    STUB_CacheBasServicePen();

    ret = NLSTK_BasProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, BAS_CONNECTING);
    STUB_ResetData();

    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);

    uint8_t desc[] = {0x09, 0x03, 0x01};
    uint8_t index1[] = {0x09, 0x03, 0x01, 0x01, 0x16, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, desc, sizeof(desc));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, index1, sizeof(index1));
    TEST_RunQueueStubSchedule();

    uint8_t cpcd[] = {0x0E, 0x03, 0x16, 0x00, 0x02, 0x01, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, cpcd, sizeof(cpcd));
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, BAS_CONNECTED);
    STUB_ResetData();

    ret = NLSTK_BasProfileDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, BAS_DISCONNECTED);
    STUB_ResetData();
}

/**
 * @test BAS_TEST_003
 * @brief 此测试用例用于验证BAS注册回调函数为空时的错误处理。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_BasRegisterCallBack传入NULL指针；
 * 2. 校验返回值是否为NLSTK_ERRCODE_POINTER_NULL；
 *
 * @pre
 * - SSAP模块已初始化。
 * - BAS模块已使能。
 *
 * @post
 * - 预期结果：返回错误码NLSTK_ERRCODE_POINTER_NULL。
 */
TEST_F(IT_BAS_TEST, BAS_TEST_003)
{
    NLSTK_Errcode_E ret = NLSTK_BasRegisterCallBack(NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_POINTER_NULL);
}

/**
 * @test BAS_TEST_004
 * @brief 此测试用例用于验证BAS连接时传入空地址的错误处理。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_BasProfileConnect传入NULL指针；
 * 2. 校验返回值是否为NLSTK_ERRCODE_POINTER_NULL；
 *
 * @pre
 * - SSAP模块已初始化。
 * - BAS模块已使能。
 *
 * @post
 * - 预期结果：返回错误码NLSTK_ERRCODE_POINTER_NULL。
 */
TEST_F(IT_BAS_TEST, BAS_TEST_004)
{
    NLSTK_Errcode_E ret = NLSTK_BasProfileConnect(NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_POINTER_NULL);
}

/**
 * @test BAS_TEST_005
 * @brief 此测试用例用于验证BAS断开连接时传入空地址的错误处理。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_BasProfileDisconnect传入NULL指针；
 * 2. 校验返回值是否为NLSTK_ERRCODE_POINTER_NULL；
 *
 * @pre
 * - SSAP模块已初始化。
 * - BAS模块已使能。
 *
 * @post
 * - 预期结果：返回错误码NLSTK_ERRCODE_POINTER_NULL。
 */
TEST_F(IT_BAS_TEST, BAS_TEST_005)
{
    NLSTK_Errcode_E ret = NLSTK_BasProfileDisconnect(NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_POINTER_NULL);
}

/**
 * @test BAS_TEST_006
 * @brief 此测试用例用于验证BAS获取已连接设备数量时传入空指针的错误处理。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_GetConnectedBasDeviceNum传入NULL指针；
 * 2. 校验返回值是否为NLSTK_ERRCODE_POINTER_NULL；
 *
 * @pre
 * - SSAP模块已初始化。
 * - BAS模块已使能。
 *
 * @post
 * - 预期结果：返回错误码NLSTK_ERRCODE_POINTER_NULL。
 */
TEST_F(IT_BAS_TEST, BAS_TEST_006)
{
    NLSTK_Errcode_E ret = NLSTK_GetConnectedBasDeviceNum(NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_POINTER_NULL);
}

/**
 * @test BAS_TEST_007
 * @brief 此测试用例用于验证BAS获取电池电量时传入空地址的错误处理。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_GetBatteryLevel传入NULL指针；
 * 2. 校验返回值是否为错误码；
 *
 * @pre
 * - SSAP模块已初始化。
 * - BAS模块已使能。
 *
 * @post
 * - 预期结果：返回错误码。
 */
TEST_F(IT_BAS_TEST, BAS_TEST_007)
{
    NLSTK_Errcode_E ret = NLSTK_GetBatteryLevel(NULL);
    EXPECT_NE(ret, NLSTK_ERRCODE_SUCCESS);
}

/**
 * @test BAS_TEST_008
 * @brief 此测试用例用于验证BAS使能功能。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用BasEnable使能BAS模块；
 * 2. 校验返回值是否为NLSTK_ERRCODE_SUCCESS；
 *
 * @pre
 * - SSAP模块已初始化。
 *
 * @post
 * - 预期结果：返回成功。
 */
TEST_F(IT_BAS_TEST, BAS_TEST_008)
{
    NLSTK_Errcode_E ret = NLSTK_ERRCODE_FAIL;
    ret = BasEnable();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
}

/**
 * @test BAS_TEST_009
 * @brief 此测试用例用于验证BAS模块去使能功能。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 先调用BasEnable使能BAS模块；
 * 2. 再调用BasDisable去使能BAS模块；
 *
 * @pre
 * - SSAP模块已初始化。
 *
 * @post
 * - 预期结果：功能正常执行，不崩溃。
 */
TEST_F(IT_BAS_TEST, BAS_TEST_009)
{
    (void)BasEnable();
    BasDisable();
}