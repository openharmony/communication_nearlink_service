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
#include "gmock/gmock.h"
#include "securec.h"
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <iostream>

#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"
#include "stack_sdf_mem_mock.h"
#include "stack_sdf_mem_stub.h"

#include "ssaps_server.h"
#include "ssapc_client.h"
#include "ssaps_server_api.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "ssaps_service.h"
#include "ssap_manager.h"
#include "ssap_utils.h"
#include "ssaps_service_param.h"
#include "cpfwk_log.h"
#include "sdf_string.h"

#define TEST_SERVICE_SIZE 2

#define TEST_MAX_BUF_CACHE 1024

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;
static bool isSendRsp = false;
static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;
static DTAP_Data_Info_S g_dtapDataInfo = {.lcid = g_lcid};

static NLSTK_SsapUuid_S g_uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
static NLSTK_SsapUuid_S g_uuid2 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03}};
static NLSTK_SsapUuid_S g_uuid3 = {.uuid = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    0x0E, 0x0F, 0x10, 0x11, 0x12}};
static NLSTK_SsapUuid_S g_uuid4 = {.uuid = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    0x0E, 0x0F, 0x10, 0x12, 0x13}}; // custome service
static NLSTK_SsapUuid_S g_uuid5 = {.uuid = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    0x0E, 0x0F, 0x10, 0x13, 0x14}}; // custome property

class UT_SSAP_SERVER_SDF_FAIL_FIND : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
    NiceMock<SdfMemMock> sdfMemMock;

protected:
    void SetUp() override
    {
        // 设置桩函数默认调用行为，不检查调用次数
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

        SSAP_ServerInit();
        SSAP_LinkInit();
    }

    void TearDown() override
    {
        SSAP_LinkDeInit();
        SSAP_ServerDeInit();
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

static void MockSendCb(SSAP_Link *link, SDF_Buff_S *buff, uint8_t opcode)
{
    CP_LOG_INFO("[TEST] enter MockSendCb");
    isSendRsp = true;
    g_buffLen = SDF_DataLenGet(buff);
    (void)memcpy_s(g_buffCache, TEST_MAX_BUF_CACHE, SDF_DataOffset(buff), SDF_DataLenGet(buff));
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    } else {
        SSAP_LinkSetTask(link, buff, opcode);
    }
    CP_LOG_INFO("[UT_SSAP_SERVER_SDF_FAIL_FIND] MockSendCb test dtap data send: %s.", SDF_GET_UINT8_STR(g_buffCache, g_buffLen));
}

static void Test_SSAP_ServiceRegisterCallback(void *context, uint8_t evt, void *arg)
{

}

static SSAP_Link* CreateLink()
{
    return SSAP_CreateSsapLink(&g_addr, g_lcid, MockSendCb);
}

static void DeleteLink()
{
    SSAP_DeleteSsapLinkByAddr(&g_addr);
}

TEST_F(UT_SSAP_SERVER_SDF_FAIL_FIND, SSAP_SERVER_MEMZALLOC_FAIL)
{
    CP_LOG_INFO("[UT_SSAP_SERVER_SDF_FAIL_FIND] enter SSAP_SERVER_MEMZALLOC_FAIL");
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)TEST_SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    serviceParam->callback = Test_SSAP_ServiceRegisterCallback;
    (void)memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_ParamAddProperty_S *propertyParam =
        (SSAP_ParamAddProperty_S *)TEST_SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    (void)memcpy_s(&propertyParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam->val.len = 1;
    propertyParam->val.value[0] = 0xFF;
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)TEST_SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;

    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test cache service fail");
    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test properties SDF_CreateVector fail");
    // 注意：EXPECT_CALL以倒序搜索预期，并且要使用RetiresOnSaturation在匹配指定次数后退休，以匹配下一个
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc)
        .WillRepeatedly(TEST_SDF_MemZalloc);    // call step3, 下面的桩调用次数达到后结束后再调用会走到这里，兜底
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc)
        .WillOnce(TEST_SDF_MemZallocFail)
        .RetiresOnSaturation();                 // call step2
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc)
        .Times(1)
        .WillOnce(TEST_SDF_MemZalloc)
        .RetiresOnSaturation();                 // call step1

    SSAP_CacheService(serviceParam);    // properties SDF_CreateVector fail
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    SSAP_StartService(NULL);
    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_NE(services, NULL);
    EXPECT_EQ(services->size, 0);
    services = NULL;

    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test references SDF_CreateVector fail");
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(3).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    SSAP_CacheService(serviceParam);    // references SDF_CreateVector fail
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    SSAP_StartService(NULL);
    services = SSAPS_GetServices();
    EXPECT_NE(services, NULL);
    EXPECT_EQ(services->size, 0);
    services = NULL;

    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test methods SDF_CreateVector fail");
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);    // 下面的桩调用都结束后再调用会走到这里，兜底
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(5).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    SSAP_CacheService(serviceParam);    // methods SDF_CreateVector fail
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    SSAP_StartService(NULL);
    services = SSAPS_GetServices();
    EXPECT_NE(services, NULL);
    EXPECT_EQ(services->size, 0);
    services = NULL;

    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test events SDF_CreateVector fail");
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(7).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    SSAP_CacheService(serviceParam);    // events SDF_CreateVector fail
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    SSAP_StartService(NULL);
    services = SSAPS_GetServices();
    EXPECT_NE(services, NULL);
    EXPECT_EQ(services->size, 0);
    services = NULL;

    SSAP_ServerDeInit();
    SSAP_ServerInit();

    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test cache property fail");
    // cache service succ
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(9).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    SSAP_CacheService(serviceParam);

    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test property val SDF_MemZalloc fail");
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(1).WillOnce(TEST_SDF_MemZalloc).RetiresOnSaturation();
    SSAP_CacheProperty(propertyParam);  // val SDF_MemZalloc fail

    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test property->descriptors SDF_CreateVector fail");
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(2).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    SSAP_CacheProperty(propertyParam);  // property->descriptors SDF_CreateVector fail

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    SSAP_StartService(NULL);
    services = SSAPS_GetServices();
    EXPECT_NE(services, NULL);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    EXPECT_NE(service, NULL);
    EXPECT_EQ(service->properties->size, 0);    // property为0，说明上面正常的缓存属性失败了
    services = NULL;
    service = NULL;

    SSAP_ServerDeInit();
    SSAP_ServerInit();

    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test cache descriptor fail");
    // cache service and property succ
    SSAP_CacheService(serviceParam);
    SSAP_CacheProperty(propertyParam);

    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test desrc val SDF_MemZalloc fail");
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(1).WillOnce(TEST_SDF_MemZalloc).RetiresOnSaturation();
    SSAP_CacheDescriptor(descParam);    // val SDF_MemZalloc fail

    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test descriptor->clientConfigs SDF_CreateVector fail");
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(2).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    SSAP_CacheDescriptor(descParam);    // descriptor->clientConfigs SDF_CreateVector fail

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    SSAP_StartService(NULL);
    services = SSAPS_GetServices();
    EXPECT_NE(services, NULL);
    service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    EXPECT_NE(service, NULL);
    SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 0);
    EXPECT_NE(property, NULL);
    EXPECT_EQ(property->descriptors->size, 0);    // descriptors为0，说明上面正常的缓存属性描述符失败了
    services = NULL;
    service = NULL;
    property = NULL;

    SDF_MemFree(descParam);
    SDF_MemFree(propertyParam);
    SDF_MemFree(serviceParam);
    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] test run success");
}

TEST_F(UT_SSAP_SERVER_SDF_FAIL_FIND, SSAP_SERVER_MEMZALLOC_FAIL_002)
{
    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test SSAP_SERVER_MEMZALLOC_FAIL_002");
    uint32_t ret = SSAP_STACK_SUCCESS;
    SSAP_ServerDeInit();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    ret = SSAP_ServerInit();  // g_services SDF_CreateVector fail
    EXPECT_EQ(ret, SSAP_STACK_FAILED);

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(2).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    ret = SSAP_ServerInit();  // SSAP_InitHandleAllocator fail
    EXPECT_EQ(ret, SSAP_STACK_FAILED);


    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    ret = SSAP_ServerInit();
    EXPECT_EQ(ret, SSAP_STACK_SUCCESS);

    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)TEST_SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    serviceParam->callback = Test_SSAP_ServiceRegisterCallback;
    (void)memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SSAP_ServerDeInit();  // g_tempService != NULL

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    ret = SSAP_ServerInit();
    EXPECT_EQ(ret, SSAP_STACK_SUCCESS);

    SDF_MemFree(serviceParam);
}

TEST_F(UT_SSAP_SERVER_SDF_FAIL_FIND, SSAP_SERVER_MEMZALLOC_FAIL_003)
{
    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter test SSAP_SERVER_MEMZALLOC_FAIL_003");
    SSAP_Service_S *ssapService = NULL;
    NLSTK_ServiceParam_S inputService = {};
    NLSTK_SsapServiceDescriptorParam_S descriptor = {};
    uint8_t data = 0;
    descriptor.val.data = &data;
    inputService.serviceStatement.descriptorNum = 1;
    inputService.serviceStatement.descriptors = &descriptor;
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    ssapService = SsapAllocServiceParam(&inputService); // ssapService SDF_MemZalloc fail
    EXPECT_EQ(ssapService, NULL);

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(1).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    ssapService = SsapAllocServiceParam(&inputService); // SsapAllocServiceParamProfile fail
    EXPECT_EQ(ssapService, NULL);

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(3).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    ssapService = SsapAllocServiceParam(&inputService); // ssapService->references SDF_CreateVector fail
    EXPECT_EQ(ssapService, NULL);

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(5).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    ssapService = SsapAllocServiceParam(&inputService); // ssapService->methods SDF_CreateVector fail
    EXPECT_EQ(ssapService, NULL);

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(7).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    ssapService = SsapAllocServiceParam(&inputService); // ssapService->events SDF_CreateVector fail
    EXPECT_EQ(ssapService, NULL);

    CP_LOG_INFO("[SSAP_SERVER_MEMZALLOC_FAIL] enter SsapAllocServiceCpyDescriptorParm fail");
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    // to enter SsapAllocServiceCpyDescriptorParm
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(11).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    ssapService = SsapAllocServiceParam(&inputService);
    // SsapAllocServiceCpyDescriptorParm in SsapAllocServiceCpyServiceParam fail
    EXPECT_EQ(ssapService, NULL);

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    // to enter SsapAllocServiceCpyDescriptorParm
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(12).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    ssapService = SsapAllocServiceParam(&inputService); // descriptor->clientConfigs SDF_CreateVector fail
    EXPECT_EQ(ssapService, NULL);

    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillRepeatedly(TEST_SDF_MemZalloc);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail).RetiresOnSaturation();
    // to enter SsapAllocServiceCpyDescriptorParm
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).Times(14).WillRepeatedly(TEST_SDF_MemZalloc).RetiresOnSaturation();
    ssapService = SsapAllocServiceParam(&inputService); // descriptor->val SDF_MemZalloc fail
    EXPECT_EQ(ssapService, NULL);
}