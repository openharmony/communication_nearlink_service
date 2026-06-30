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

#include "cm_logic_link_api.h"

#include "cdsm_api.h"
#include "cdsm_encp.h"
#include "cdsm.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static uint16_t g_lcid = 0;

static SLE_Addr_S g_addr1 = {.type = PUBLIC_ADDRESS, .addr = {0x01, 0x01, 0x02, 0x02, 0x03, 0x03}};
static SLE_Addr_S g_addr2 = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint8_t g_addrConnState = CDSM_EVENT_DISCONNECT;
static NLSTK_SsapUuid_S g_cdsmUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00};
static NLSTK_SsapUuid_S g_keyInfoUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00};
static NLSTK_SsapUuid_S g_memNumUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01};
static NLSTK_SsapUuid_S g_coopsetRoleUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02};
static NLSTK_SsapUuid_S g_discoverModUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x03};
static NLSTK_SsapUuid_S g_serviceTableUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x04};
static NLSTK_SsapUuid_S g_memberAddrUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x7A};
static NLSTK_SsapUuid_S g_keyUuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x79};

static void Stub_CdsmEventCbk(NLSTK_CdsmEvent_S *event)
{
    for (int i = 0; i < event->num; i++) {
        if (memcmp(&event->memInfo[i].addr, &g_addr1, sizeof(SLE_Addr_S)) == 0) {
            g_addrConnState = event->memInfo[i].state;
        }
    }
}

class CDSM_TEST : public testing::Test {
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


        // 步骤2：初始化SSAP模块
        SSAP_Init();
        
        // 步骤3：使能cdsm模块
        CdsmInit();
        DevdLocalDeviceInit();
        NLSTK_CdsmRegisterEventCbk(Stub_CdsmEventCbk);
    }

    void TearDown() override
    {
        // 步骤4：去使能cdsm模块
        DevdLocalDeviceDeInit();
        CdsmDeInit();

        // 步骤5：去初始化SSAP模块
        SSAP_DeInit();

        // 步骤6：清理桩函数
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

/**
 * @test CDSM_CREATE_001
 * @brief 此测试用例用于验证创建合作集与添加设备。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 创建合作集
 * 2. 合作集添加设备
 * 3. 删除合作集
 *
 * @pre
 * - SSAP模块已初始化。
 * - CDSM子模块已初始化。
 *
 * @post
 * - 预期结果：合作集设备成员数量查询正确。
 */
TEST_F(CDSM_TEST, CDSM_CREATE_001)
{
    uint32_t gid = NLSTK_CdsmCreateSet(&g_addr1);
    EXPECT_TRUE(gid != 0xFFFFFFFF);

    SDF_Vector_S *vector = NLSTK_CdsmFindAllAddrByGid(gid);
    ASSERT_TRUE(vector != NULL);
    EXPECT_EQ(vector->size, 1);
    SDF_DestroyVector(vector);

    NLSTK_CdsmRecoverMeb(gid, 1, &g_addr2);
    vector = NLSTK_CdsmFindAllAddrByGid(gid);
    EXPECT_EQ(vector->size, 2);
    SDF_DestroyVector(vector);

    uint32_t findGid = NLSTK_CdsmFindGidByAddr(&g_addr1);
    EXPECT_EQ(findGid, gid);

    NLSTK_CdsmRemoveSet(gid);
    vector = NLSTK_CdsmFindAllAddrByGid(gid);
    EXPECT_EQ(vector->size, 0);
    SDF_DestroyVector(vector);
}

static void STUB_CacheCdsmService()
{
    SsapCacheServInfo_S serv = {0};
    serv.handle = 0x10;
    serv.endHandle = 0x17;
    serv.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serv.uuid, sizeof(NLSTK_SsapUuid_S), &g_cdsmUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheServ(&g_addr1, &serv);

    NLSTK_SsapPrty_S keyInfoProp = {0};
    keyInfoProp.handle = 0x11;
    memcpy_s(&keyInfoProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_keyInfoUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &keyInfoProp);

    NLSTK_SsapPrty_S memNumProp = {0};
    memNumProp.handle = 0x12;
    memcpy_s(&memNumProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_memNumUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &memNumProp);

    NLSTK_SsapPrty_S coopsetRoleProp = {0};
    coopsetRoleProp.handle = 0x13;
    memcpy_s(&coopsetRoleProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_coopsetRoleUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &coopsetRoleProp);

    NLSTK_SsapPrty_S discoverModProp = {0};
    discoverModProp.handle = 0x14;
    memcpy_s(&discoverModProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_discoverModUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &discoverModProp);

    NLSTK_SsapPrty_S serviceTableProp = {0};
    serviceTableProp.handle = 0x15;
    memcpy_s(&serviceTableProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_serviceTableUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &serviceTableProp);

    NLSTK_SsapPrty_S memberAddrProp = {0};
    memberAddrProp.handle = 0x16;
    memcpy_s(&memberAddrProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_memberAddrUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &memberAddrProp);

    NLSTK_SsapPrty_S keyProp = {0};
    keyProp.handle = 0x17;
    memcpy_s(&keyProp.uuid, sizeof(NLSTK_SsapUuid_S), &g_keyUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr1, &keyProp);

    SsapcCacheServMemberDiscFinish(&g_addr1, FIND_STRUCTURE_TYPE_PROPERTY, 0x17);
    SsapcCacheServDiscFinish(&g_addr1);
}

static uint8_t g_readKeyInfoRsp[] = {0x09, 0x03, 0x01, 0x02};
static uint8_t g_readMemNumRsp[] = {0x09, 0x03, 0x02};
static uint8_t g_readAddrRsp[] = {0x09, 0x03, 0x01, 0x00, 0x7C, 0x50, 0x22, 0x19, 0x01, 0x00};
static uint8_t g_readKeyRsp[] = {0x09, 0x03, 0xA3, 0xD8, 0x21, 0x9E, 0xD8, 0x5C, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};

static void STUB_AdvEvent(uint8_t handle, uint8_t event, uint8_t result)
{
    DevdAdvNode_S *node = DevdGetAdvNode(handle, DEVD_ADV_LIST);
    if (node != NULL && node->cbk != NULL) {
        NLSTK_DevdAdvCbkParam_S cbkParam = {0};
        cbkParam.advHandle = handle;
        cbkParam.event = event;
        cbkParam.result = result;
        node->cbk(&cbkParam);
    }
}

/**
 * @test CDSM_CONNECT_001
 * @brief 此测试用例用于验证创建合作集与添加设备。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 创建合作集
 * 2. 合作集添加设备
 * 3. 删除合作集
 *
 * @pre
 * - SSAP模块已初始化。
 * - CDSM子模块已初始化。
 *
 * @post
 * - 预期结果：合作集设备成员数量查询正确。
 */
TEST_F(CDSM_TEST, CDSM_CONNECT_001)
{
    // 完成建链
    CM_LogicLinkState_S linkState = {.lcid = 0, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr1, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    uint32_t gid = NLSTK_CdsmCreateSet(&g_addr1);
    NLSTK_CdsmRecoverMeb(gid, 1, &g_addr2);
    STUB_CacheCdsmService();

    NLSTK_CdsmConnect(&g_addr1);

    // 模拟收到init初始报文
    TEST_DTAP_SSAP_RevcInitPkt(0);
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readKeyInfoRsp, sizeof(g_readKeyInfoRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readMemNumRsp, sizeof(g_readMemNumRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readAddrRsp, sizeof(g_readAddrRsp));
    TEST_DTAP_RecDataWithPkt(0, TCID_SLE_SMTC, g_readKeyRsp, sizeof(g_readKeyRsp));
    EXPECT_EQ(g_addrConnState, CDSM_EVENT_CONNECT);

    NLSTK_CdsmStartAdv(&g_addr1);
    STUB_AdvEvent(0, DEVD_CBK_EVENT_ENABLE_ADV, 0);
    NLSTK_CdsmStopAdv(&g_addr1);
    STUB_AdvEvent(0, DEVD_CBK_EVENT_DISABLE_ADV, 0);
    STUB_AdvEvent(0, DEVD_CBK_EVENT_REMOVE_ADV, 0);

    NLSTK_CdsmDisconnect(&g_addr1);
    CM_LogicLinkState_S linkState2 = {.lcid = 0, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr1, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    EXPECT_EQ(g_addrConnState, CDSM_EVENT_DISCONNECT);
}