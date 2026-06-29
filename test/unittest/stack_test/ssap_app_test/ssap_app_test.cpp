/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "securec.h"

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"
#include "stack_sdf_mem_mock.h"
#include "stack_sdf_mem_stub.h"

#include "sdf_addr.h"
#include "nlstk_ssap_app_client.h"
#include "nlstk_public_define.h"
#include "nlstk_ssap_app_link.h"
#include "ssap_manager.h"
#include "nlstk_ssap_app_server.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static int32_t g_appId;
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static NLSTK_SsapUuid_S g_uuid = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};

class SSAP_APP_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
    NiceMock<SdfMemMock> sdfMemMock;

protected:
    void SetUp() override
    {
        // 步骤1：初始化桩函数
        ON_CALL(sdfMemMock, SDF_MemZalloc).WillByDefault(TEST_SDF_MemZalloc);

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
    }
    void TearDown() override
    {
        // 步骤3：去初始化SSAP模块
        SSAP_DeInit();
        // 步骤4：清理桩函数
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

static void Stub_SsapClientFindServiceCb(int32_t appId, NLSTK_Errcode_E ret)
{
}

static void Stub_SsapServerMtuChangedCb(int32_t appId, SLE_Addr_S *addr, uint16_t mtu)
{
}


static void Stub_ResetData()
{
    g_appId = -1;
}
// 调用NLSTK_SsapClientRegApp时内存分配失败
TEST_F(SSAP_APP_TEST, CLIENT_TEST_001)
{
    Stub_ResetData();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZallocFail);

    NLSTK_SsapAppClientCb_S cb = {0};
    cb.onFindService = Stub_SsapClientFindServiceCb;
    NLSTK_Errcode_E ret = NLSTK_SsapClientRegApp(&g_appId, &cb, &g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_MALLOC_FAIL);
}

// 调用NLSTK_SsapClientRegAppAsyn时内存分配失败
TEST_F(SSAP_APP_TEST, CLIENT_TEST_002)
{
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZallocFail);

    NLSTK_ConnParam_S param = {0};
    NLSTK_SsapAppClientCb_S cb = {0};
    cb.onFindService = Stub_SsapClientFindServiceCb;
    NLSTK_Errcode_E ret = NLSTK_SsapClientRegAppAsyn(&g_addr, &param, &cb);
    EXPECT_EQ(ret, NLSTK_ERRCODE_MALLOC_FAIL);
}

// 调用NLSTK_SsapClientGetServicesByUuidAsyn时内存分配失败
TEST_F(SSAP_APP_TEST, CLIENT_TEST_003)
{
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZallocFail);

    NLSTK_Errcode_E ret = NLSTK_SsapClientGetServicesByUuidAsyn(g_appId, &g_uuid);
    EXPECT_EQ(ret, NLSTK_ERRCODE_MALLOC_FAIL);
}

// 调用NLSTK_SsapClientWriteProperty时内存分配失败
TEST_F(SSAP_APP_TEST, CLIENT_TEST_004)
{
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZallocFail);
    NLSTK_VariableData_S value = {0};
    uint8_t data = 0;
    value.len = 1;
    value.data = &data;
    uint16_t handle = 0x01;
    NLSTK_Errcode_E ret = NLSTK_SsapClientWriteProperty(g_appId, handle, &value, false);
    EXPECT_EQ(ret, NLSTK_ERRCODE_MALLOC_FAIL);
}

// 调用NLSTK_SsapClientWriteProperty时内存分配失败
TEST_F(SSAP_APP_TEST, CLIENT_TEST_005)
{
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZallocFail);
    NLSTK_VariableData_S value = {0};
    uint8_t data = 0;
    value.len = 1;
    value.data = &data;
    uint16_t handle = 0x01;
    NLSTK_Errcode_E ret = NLSTK_SsapClientWriteProperty(g_appId, handle, &value, false);
    EXPECT_EQ(ret, NLSTK_ERRCODE_MALLOC_FAIL);
}

// 调用NLSTK_SsapClientConnect时添加队列失败
TEST_F(SSAP_APP_TEST, LINK_TEST_001)
{
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);

    NLSTK_SsapAppClientCb_S cb = {0};
    cb.onFindService = Stub_SsapClientFindServiceCb;
    NLSTK_Errcode_E ret = NLSTK_SsapClientRegApp(&g_appId, &cb, &g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    ret = NLSTK_SsapClientConnect(g_appId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
    Stub_ResetData();
}

// 调用NLSTK_SsapClientDisconnect时添加队列失败
TEST_F(SSAP_APP_TEST, LINK_TEST_002)
{
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);

    NLSTK_SsapAppClientCb_S cb = {0};
    cb.onFindService = Stub_SsapClientFindServiceCb;
    NLSTK_Errcode_E ret = NLSTK_SsapClientRegApp(&g_appId, &cb, &g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);

    ret = NLSTK_SsapClientDisconnect(g_appId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
    Stub_ResetData();
}

// 调用NLSTK_SsapServerRegApp时添加队列失败
TEST_F(SSAP_APP_TEST, SERVER_TEST_001)
{
    EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStubFail);

    NLSTK_SsapAppServerCb_S cb = {0};
    cb.onMtuChanged = Stub_SsapServerMtuChangedCb;
    NLSTK_Errcode_E ret = NLSTK_SsapServerRegApp(&cb, &g_appId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
    Stub_ResetData();
}

// 调用NLSTK_SsapServerSetMtu时添加队列失败
TEST_F(SSAP_APP_TEST, SERVER_TEST_002)
{
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);

    NLSTK_Errcode_E ret = NLSTK_SsapServerSetMtu(600);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
    Stub_ResetData();
}

// 调用NLSTK_SsapServerRemoveService时添加队列失败
TEST_F(SSAP_APP_TEST, SERVER_TEST_003)
{
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);

    g_appId = 1;
    uint16_t handle = 1;
    NLSTK_Errcode_E ret = NLSTK_SsapServerRemoveService(g_appId, handle);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
    EXPECT_EQ(g_appId, 1);
    Stub_ResetData();
}

// 调用NLSTK_SsapServerClearServices时添加队列失败
TEST_F(SSAP_APP_TEST, SERVER_TEST_005)
{
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);

    g_appId = 1;
    NLSTK_Errcode_E ret = NLSTK_SsapServerClearServices(g_appId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
    EXPECT_EQ(g_appId, 1);
    Stub_ResetData();
}

// 调用NLSTK_SsapServerAuthorizeResult时添加队列失败
TEST_F(SSAP_APP_TEST, SERVER_TEST_006)
{
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);

    g_appId = 1;
    uint16_t requestId = 1;
    bool allow = true;
    NLSTK_Errcode_E ret = NLSTK_SsapServerAuthorizeResult(g_appId, requestId, allow);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
    EXPECT_EQ(g_appId, 1);
    Stub_ResetData();
}

// 调用NLSTK_SsapServerUpdateAndNotifyProperty
TEST_F(SSAP_APP_TEST, SERVER_TEST_007)
{
    g_appId = 1;
    uint16_t handle = 1;
    NLSTK_VariableData_S value = {0};
    value.len = 1;
    uint8_t data = 2;
    value.data = &data;
    NLSTK_Errcode_E ret = NLSTK_SsapServerUpdateAndNotifyProperty(g_appId, handle, &g_addr, &value);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(g_appId, 1);
    Stub_ResetData();
}

// 调用NLSTK_SsapServerUpdateAndNotifyProperty添加队列失败
TEST_F(SSAP_APP_TEST, SERVER_TEST_008)
{
    EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStubFail);

    g_appId = 1;
    uint16_t handle = 1;
    NLSTK_VariableData_S value = {0};
    value.len = 1;
    uint8_t data = 2;
    value.data = &data;
    NLSTK_Errcode_E ret = NLSTK_SsapServerUpdateAndNotifyProperty(g_appId, handle, &g_addr, &value);
    EXPECT_EQ(ret, NLSTK_ERRCODE_TASK_FAIL);
    EXPECT_EQ(g_appId, 1);
    Stub_ResetData();
}

// 调用NLSTK_SsapFreeServiceParam传入空指针
TEST_F(SSAP_APP_TEST, SERVER_TEST_009)
{
    NLSTK_SsapFreeServiceParam(NULL);
}

// 调用NLSTK_SsapFreeServiceParam
TEST_F(SSAP_APP_TEST, SERVER_TEST_010)
{
    // param非空时会在NLSTK_SsapFreeServiceParam中free
    NLSTK_ServiceParam_S *param = (NLSTK_ServiceParam_S*)TEST_SDF_MemZalloc(sizeof(NLSTK_ServiceParam_S));
    param->serviceStatement.descriptors = NULL;
    param->serviceStatement.descriptorNum = 0;
    NLSTK_SsapServiceReferenceParam_S *serviceReference = (NLSTK_SsapServiceReferenceParam_S*)TEST_SDF_MemZalloc
                                                            (sizeof(NLSTK_SsapServiceReferenceParam_S));
    param->property = NULL;
    param->servicePropertyNum = 0;
    param->method = NULL;
    param->serviceMethodNum = 0;
    NLSTK_SsapServiceEventParam_S *event = (NLSTK_SsapServiceEventParam_S*)TEST_SDF_MemZalloc
                                            (sizeof(NLSTK_SsapServiceEventParam_S));
    param->serviceEventNum = 0;
    NLSTK_SsapFreeServiceParam((void*)param);
}