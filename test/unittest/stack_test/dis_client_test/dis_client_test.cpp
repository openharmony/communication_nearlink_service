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

#include "cm_def.h"
#include "dtap_tcid.h"
#include "ssap_link.h"
#include "ssap_manager.h"
#include "ssaps_service.h"
#include "ssapc_cache.h"
#include "ssapc_app.h"
#include "nlstk_dis_client.h"
#include "dis_common.h"
// #include "dis_init.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x01, 0x02, 0x02, 0x03, 0x03}};
static NLSTK_DisConnectState_E g_addrConnState = DIS_DISCONNECTED;

static NLSTK_SsapUuid_S g_disUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x09};
static NLSTK_SsapUuid_S g_manufactureUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x2E};
static NLSTK_SsapUuid_S g_modelUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x2F};
static NLSTK_SsapUuid_S g_serialNumberUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x30};
static NLSTK_SsapUuid_S g_hardwareVersionUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x31};
static NLSTK_SsapUuid_S g_firmwareVersionUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x32};
static NLSTK_SsapUuid_S g_softwareVersionUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x33};
static NLSTK_SsapUuid_S g_aliasUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x3F};
static NLSTK_SsapUuid_S g_appearanceUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x41};

static void STUB_DisConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_DisConnectState_E curState,
    NLSTK_DisConnectState_E prevState, NLSTK_Errcode_E errorCode);

class IT_DIS_CLIENT_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
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

        // 步骤2：初始化SSAP模块
        SSAP_LinkInit();
        SSAP_Init();

        // 步骤3：初始化DIS
        // DisInit();

        g_addrConnState = DIS_DISCONNECTED;
    }

    void TearDown() override
    {
        DisDisable();
        // DisDeInit();

        SSAP_DeInit();
        SSAP_LinkDeInit();

        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

static void STUB_DisConnectStateChangeCbk(SLE_Addr_S *addr, NLSTK_DisConnectState_E curState,
    NLSTK_DisConnectState_E prevState, NLSTK_Errcode_E errorCode)
{
    g_addrConnState = curState;
    return;
}

static void STUB_CacheDisService()
{
    SsapCacheServInfo_S serv = {0};
    serv.handle = 0x10;
    serv.endHandle = 0x18;
    serv.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serv.uuid, sizeof(NLSTK_SsapUuid_S), &g_disUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheServ(&g_addr, &serv);

    NLSTK_SsapPrty_S manufactureProp = {0};
    manufactureProp.handle = 0x11;
    memcpy_s(&manufactureProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_manufactureUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &manufactureProp);

    NLSTK_SsapPrty_S modelProp = {0};
    modelProp.handle = 0x12;
    memcpy_s(&modelProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_modelUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &modelProp);

    NLSTK_SsapPrty_S serialNumberProp = {0};
    serialNumberProp.handle = 0x13;
    memcpy_s(&serialNumberProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_serialNumberUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &serialNumberProp);

    NLSTK_SsapPrty_S hardwareVersionProp = {0};
    hardwareVersionProp.handle = 0x14;
    memcpy_s(&hardwareVersionProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_hardwareVersionUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &hardwareVersionProp);

    NLSTK_SsapPrty_S firmwareVersionProp = {0};
    firmwareVersionProp.handle = 0x15;
    memcpy_s(&firmwareVersionProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_firmwareVersionUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &firmwareVersionProp);

    NLSTK_SsapPrty_S softVersionProp = {0};
    softVersionProp.handle = 0x16;
    memcpy_s(&softVersionProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_softwareVersionUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &softVersionProp);

    NLSTK_SsapPrty_S aliasProp = {0};
    aliasProp.handle = 0x17;
    memcpy_s(&aliasProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_aliasUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &aliasProp);

    NLSTK_SsapPrty_S appearanceProp = {0};
    appearanceProp.handle = 0x18;
    memcpy_s(&appearanceProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_appearanceUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &appearanceProp);

    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_PROPERTY, 0x18);
    SsapcCacheServDiscFinish(&g_addr);
}

static uint8_t g_readAppearanceInfoRsp[] = {0x09, 0x03, 0x01, 0x02, 0x00};
static uint8_t g_readManufacturerInfoRsp[] = {0x09, 0x03, 0x01};
static uint8_t g_readAliasInfoRsp[] = {0x09, 0x03, 0x01, 0x02};
static uint8_t g_readModelInfoRsp[] = {0x09, 0x03, 0x01, 0x03};

/**
 * @test DIS_CONNECT_001
 * @brief 此测试用例用于验证DIS特性连接。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. DIS连接
 *
 * @pre
 * - SSAP模块已初始化。
 * - DIS子模块已初始化。
 *
 * @post
 * - 预期结果：DIS特性连接成功。
 */
TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_001)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);

    // 模拟收到init初始报文
    TEST_DTAP_SSAP_RevcInitPkt(0);

    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readAppearanceInfoRsp, sizeof(g_readAppearanceInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readManufacturerInfoRsp, sizeof(g_readManufacturerInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readModelInfoRsp, sizeof(g_readModelInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readAliasInfoRsp, sizeof(g_readAliasInfoRsp));
    EXPECT_EQ(g_addrConnState, DIS_CONNECTED);

    uint8_t value[DIS_MAX_VAR_LEN] = {0};
    NLSTK_VariableData_S lenVal = {0};
    lenVal.data = value;
    lenVal.len = DIS_MAX_VAR_LEN;
    ret = NLSTK_DisReadInfo(&g_addr, DIS_MANUFACTURER_INFO, &lenVal);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(lenVal.len, 1);

    uint8_t value2[DIS_MAX_VAR_LEN] = {0};
    NLSTK_VariableData_S lenVal2 = {0};
    lenVal2.data = value2;
    lenVal2.len = DIS_MAX_VAR_LEN;
    ret = NLSTK_DisReadInfo(&g_addr, DIS_LOCAL_ALIAS_INFO, &lenVal2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(lenVal2.len, 2);

    uint32_t appearance = 0;
    ret = NLSTK_DisReadAppearanceInfo(&g_addr, &appearance);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(appearance, 513);

    uint8_t num = 0;
    ret = NLSTK_GetConnectedDeviceNum(&num);
    EXPECT_EQ(num, 1);

    NLSTK_DisProfileDisconnect(&g_addr);
    TEST_RunQueueStubSchedule();
    // EXPECT_EQ(g_addrConnState, DIS_DISCONNECTING);

    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_002)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);

    // 模拟收到init初始报文
    TEST_DTAP_SSAP_RevcInitPkt(0);

    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readAppearanceInfoRsp, sizeof(g_readAppearanceInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readManufacturerInfoRsp, sizeof(g_readManufacturerInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readModelInfoRsp, sizeof(g_readModelInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readAliasInfoRsp, sizeof(g_readAliasInfoRsp));
    EXPECT_EQ(g_addrConnState, DIS_CONNECTED);

    // 未调用disconnect收到断连
    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

// 模拟重复调用NLSTK_DisProfileConnect
TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_003)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);

    // 模拟收到init初始报文
    TEST_DTAP_SSAP_RevcInitPkt(0);

    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readAppearanceInfoRsp, sizeof(g_readAppearanceInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readManufacturerInfoRsp, sizeof(g_readManufacturerInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readModelInfoRsp, sizeof(g_readModelInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readAliasInfoRsp, sizeof(g_readAliasInfoRsp));
    EXPECT_EQ(g_addrConnState, DIS_CONNECTED);

    ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

// 模拟DIS_SERVICE_FOUND状态收到NLSTK_DisProfileDisconnect
TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_004)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);

    NLSTK_DisProfileDisconnect(&g_addr);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

// 模拟未注册回调进行NLSTK_DisProfileConnect
TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_006)
{
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
}

// 模拟未连接获取设备数目
TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_007)
{
    uint8_t num = -1;
    NLSTK_Errcode_E ret = NLSTK_GetConnectedDeviceNum(&num);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(num, 0);
}

// 模拟已连接后readinfo
TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_008)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);

    // 模拟收到init初始报文
    TEST_DTAP_SSAP_RevcInitPkt(0);

    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readAppearanceInfoRsp, sizeof(g_readAppearanceInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readManufacturerInfoRsp, sizeof(g_readManufacturerInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readModelInfoRsp, sizeof(g_readModelInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readAliasInfoRsp, sizeof(g_readAliasInfoRsp));
    EXPECT_EQ(g_addrConnState, DIS_CONNECTED);

    uint8_t value[DIS_MAX_VAR_LEN] = {0};
    NLSTK_VariableData_S lenVal = {0};
    lenVal.data = value;
    lenVal.len = DIS_MAX_VAR_LEN;
    ret = NLSTK_DisReadInfo(&g_addr, DIS_MODEL_INFO, &lenVal);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(lenVal.len, 2);

    uint8_t value2[DIS_MAX_VAR_LEN] = {0};
    NLSTK_VariableData_S lenVal2 = {0};
    lenVal2.data = value2;
    lenVal2.len = DIS_MAX_VAR_LEN;
    ret = NLSTK_DisReadInfo(&g_addr, DIS_SERIAL_INFO, &lenVal2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(lenVal2.len, DIS_MAX_VAR_LEN);

    uint8_t value3[DIS_MAX_VAR_LEN] = {0};
    NLSTK_VariableData_S lenVal3 = {0};
    lenVal3.data = value3;
    lenVal3.len = DIS_MAX_VAR_LEN;
    ret = NLSTK_DisReadInfo(&g_addr, DIS_HARDWARE_VERSION_INFO, &lenVal3);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(lenVal3.len, DIS_MAX_VAR_LEN);

    uint8_t value4[DIS_MAX_VAR_LEN] = {0};
    NLSTK_VariableData_S lenVal4 = {0};
    lenVal4.data = value4;
    lenVal4.len = DIS_MAX_VAR_LEN;
    ret = NLSTK_DisReadInfo(&g_addr, DIS_FIRMWARE_VERSION_INFO, &lenVal4);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(lenVal4.len, DIS_MAX_VAR_LEN);

    uint8_t value5[DIS_MAX_VAR_LEN] = {0};
    NLSTK_VariableData_S lenVal5 = {0};
    lenVal5.data = value5;
    lenVal5.len = DIS_MAX_VAR_LEN;
    ret = NLSTK_DisReadInfo(&g_addr, DIS_SOFTWARE_VERSION_INFO, &lenVal5);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(lenVal5.len, DIS_MAX_VAR_LEN);

    uint8_t value6[DIS_MAX_VAR_LEN] = {0};
    NLSTK_VariableData_S lenVal6 = {0};
    lenVal6.data = value6;
    lenVal6.len = DIS_MAX_VAR_LEN;
    ret = NLSTK_DisReadInfo(&g_addr, DIS_APPEARANCE_INFO, &lenVal6);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(lenVal6.len, DIS_MAX_VAR_LEN);

    NLSTK_DisProfileDisconnect(&g_addr);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_FAIL_001)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    // profile connect
    TEST_RunQueueStubScheduleOnce();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);
    // reg app
    TEST_RunQueueStubScheduleOnce();
    // connect
    TEST_RunQueueStubScheduleOnce();

    // 收到断连
    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_FAIL_002)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    // profile connect
    TEST_RunQueueStubScheduleOnce();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);
    // reg app
    TEST_RunQueueStubScheduleOnce();
    // connect
    TEST_RunQueueStubScheduleOnce();
    // getservice
    TEST_RunQueueStubScheduleOnce();

    // 收到断连
    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_FAIL_003)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);

    // 收到断连
    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_FAIL_004)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    // profile connect
    TEST_RunQueueStubScheduleOnce();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);
    // reg app
    TEST_RunQueueStubScheduleOnce();
    // connect
    TEST_RunQueueStubScheduleOnce();

    // 收到断连
    NLSTK_DisProfileDisconnect(&g_addr);
    TEST_RunQueueStubSchedule();
    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_FAIL_005)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    // profile connect
    TEST_RunQueueStubScheduleOnce();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);
    // reg app
    TEST_RunQueueStubScheduleOnce();
    // connect
    TEST_RunQueueStubScheduleOnce();
    // getservice
    TEST_RunQueueStubScheduleOnce();

    // 收到断连
    NLSTK_DisProfileDisconnect(&g_addr);
    TEST_RunQueueStubSchedule();
    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_FAIL_006)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);

    // 收到断连
    NLSTK_DisProfileDisconnect(&g_addr);
    TEST_RunQueueStubSchedule();
    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

// 模拟DIS_LINK_DISCONNECTED收到链路断链
TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_FAIL_007)
{
    NLSTK_DisClientCbk_S cbk = {.stateChangeCbk = STUB_DisConnectStateChangeCbk};
    (void)NLSTK_DisRegisterCallbBack(&cbk);

    STUB_CacheDisService();

    NLSTK_Errcode_E ret = NLSTK_DisProfileConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_addrConnState, DIS_CONNECTING);

    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    EXPECT_EQ(g_addrConnState, DIS_DISCONNECTED);
}

// 模拟获取连接数目添加任务失败
TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_FAIL_008)
{
    EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStubFail);

    uint8_t num = 255;
    NLSTK_Errcode_E ret = NLSTK_GetConnectedDeviceNum(&num);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
    EXPECT_EQ(num, 255);
}

// 读取信息添加任务失败
TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_FAIL_009)
{
    EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStubFail);

    uint32_t appearance = 0;
    NLSTK_Errcode_E ret = NLSTK_DisReadAppearanceInfo(&g_addr, &appearance);
    EXPECT_EQ(ret, NLSTK_ERRCODE_BASE);
    EXPECT_EQ(appearance, 0);
}

// 读取缓存设备信息添加任务失败
TEST_F(IT_DIS_CLIENT_TEST, DIS_CONNECT_FAIL_010)
{
    EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStubFail);

    uint8_t value[DIS_MAX_VAR_LEN] = {0};
    NLSTK_VariableData_S lenVal = {0};
    lenVal.data = value;
    lenVal.len = DIS_MAX_VAR_LEN;
    NLSTK_Errcode_E ret = NLSTK_DisReadInfo(&g_addr, DIS_MODEL_INFO, &lenVal);
    EXPECT_EQ(ret, 1);
}
