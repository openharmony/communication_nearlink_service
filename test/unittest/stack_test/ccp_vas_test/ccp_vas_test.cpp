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

#include "ccp_type.h"
#include "ccp_utils.h"
#include "ccp_vas_server.h"
#include "nlstk_ccp.h"
#include "nlstk_ccp_vas_server.h"

#include "cpfwk_log.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

class CCP_VAS_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;

    static void Stub_VasResetData(void);
    static void Stub_StartServiceCbk(uint32_t errorCode);
    static void Stub_AuthorizeCbk(SLE_Addr_S *addr, uint32_t requestId);
    static void Stub_ActivateCallback(SLE_Addr_S *addr, uint32_t requestId);
    static void Stub_TerminateCallback(SLE_Addr_S *addr, uint32_t requestId);
    static void Stub_ActivateFailCallback(SLE_Addr_S *addr, uint32_t requestId);
    static void Stub_TerminateFailCallback(SLE_Addr_S *addr, uint32_t requestId);

    static uint32_t g_addResult;
    static SLE_Addr_S g_recvAddr;
    static bool g_recvCbk;
    static bool g_recvAddCbk;
    static bool g_recvAuthCbk;
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
        SSAP_Init();
        
        // 步骤3：使能Ccp模块
        CcpEnable();

        Stub_VasResetData();
    }

    void TearDown() override
    {
        // 步骤4：去使能CCP模块
        CcpDisable();

        // 步骤5：去初始化SSAP模块
        SSAP_DeInit();

        // 步骤6：清理桩函数
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

uint32_t CCP_VAS_TEST::g_addResult = NLSTK_ERRCODE_FAIL;
SLE_Addr_S CCP_VAS_TEST::g_recvAddr = {0};
bool CCP_VAS_TEST::g_recvCbk = false;
bool CCP_VAS_TEST::g_recvAddCbk = false;
bool CCP_VAS_TEST::g_recvAuthCbk = false;

void CCP_VAS_TEST::Stub_VasResetData(void)
{
    g_addResult = NLSTK_ERRCODE_FAIL;
    g_recvAddr = {0};
    g_recvCbk = false;
    g_recvAddCbk = false;
    g_recvAuthCbk = false;
}

void CCP_VAS_TEST::Stub_StartServiceCbk(uint32_t errorCode)
{
    g_recvAddCbk = true;
    g_addResult = errorCode;
}

void CCP_VAS_TEST::Stub_AuthorizeCbk(SLE_Addr_S *addr, uint32_t requestId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvAuthCbk = true;
    NLSTK_CcpVasStateAuthorizeResult(requestId, NLSTK_ERRCODE_SUCCESS);
}

void CCP_VAS_TEST::Stub_ActivateCallback(SLE_Addr_S *addr, uint32_t requestId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_CcpVasControlResult(requestId, NLSTK_CCP_VAS_ACTIVATE, NLSTK_VAS_CONTROL_SUCCESS);
}

void CCP_VAS_TEST::Stub_TerminateCallback(SLE_Addr_S *addr, uint32_t requestId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_CcpVasControlResult(requestId, NLSTK_CCP_VAS_TERMINATE, NLSTK_VAS_CONTROL_SUCCESS);
}

void CCP_VAS_TEST::Stub_ActivateFailCallback(SLE_Addr_S *addr, uint32_t requestId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_CcpVasControlResult(requestId, NLSTK_CCP_VAS_ACTIVATE, NLSTK_VAS_CONTROL_FAIL);
}

void CCP_VAS_TEST::Stub_TerminateFailCallback(SLE_Addr_S *addr, uint32_t requestId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_CcpVasControlResult(requestId, NLSTK_CCP_VAS_TERMINATE, NLSTK_VAS_CONTROL_FAIL);
}

static bool CompUuidFunc(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    SSAP_Property_S *property = (SSAP_Property_S *)ptr;
    NLSTK_SsapUuid_S *uuid = (NLSTK_SsapUuid_S *)args;
    return memcmp(&property->uuid, uuid, sizeof(NLSTK_SsapUuid_S)) == 0;
}

/**
 * @test CCP_VAS_TEST_001
 * @brief 此测试用例用于验证语音助手服务端添加语音助手服务，此场景为正常添加，属性需要授权。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateVoiceAssistantService添加语音助手服务；
 * 2. 校验回调函数触发添加成功；
 * 3. 校验SSAP服务中是否有语音助手服务；
 * 4. 模拟对端读取语音助手状态，校验授权回调；
 * 5. 调用NLSTK_CcpDeleteVoiceAssistantService删除服务；
 * 6. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回结果码成功。
 */
TEST_F(CCP_VAS_TEST, CCP_VAS_TEST_001)
{
    CP_LOG_INFO("[TEST] enter CCP_VAS_TEST_001");
    // 添加语音助手服务
    NLSTK_CcpVasInfo_S vasInfo = {0};
    vasInfo.stateRight = NLSTK_VAS_READ_AUTHOR;
    vasInfo.startService = Stub_StartServiceCbk;
    vasInfo.authorize = Stub_AuthorizeCbk;
    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);
    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_VOICE_ASSISTANT_SERVICE_UUID);
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru((uint16_t)CCP_VOICE_ASSISTANT_STATE_UUID);
    size_t index = 0;
    EXPECT_EQ(SDF_VectorFindFirst(service->properties, CompUuidFunc, &uuidStru, &index), true);
    SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, index);
    uint16_t handle = property->handle;
    size_t handleOffset = 0x02;

    // 模拟收到读取请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    uint8_t readReq[] = {0x08, 0x03, 0x00, 0x00, 0x00};
    (void)memcpy_s(readReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    CP_LOG_INFO("[TEST] enter recv read req");
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, readReq, sizeof(readReq));
    TEST_RunQueueStubSchedule();

    EXPECT_EQ(g_recvAuthCbk, true);
    Stub_VasResetData();

    // 校验发送响应报文是否符合预期
    uint8_t errRsp[] = {0x09, 0x03, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(errRsp, sizeof(errRsp)));

    // 删除语音助手服务
    ret = NLSTK_CcpDeleteVoiceAssistantService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test CCP_VAS_TEST_002
 * @brief 此测试用例用于验证语音助手服务端添加语音助手服务，此场景为正常添加，然后更新语音助手状态。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateVoiceAssistantService添加语音助手服务；
 * 2. 校验回调函数触发添加成功；
 * 3. 校验SSAP服务中是否有语音助手服务；
 * 4. 调用NLSTK_CcpUpdateVasState更新语音助手状态；
 * 5. 校验底层SSAP属性数据是否一致；
 * 6. 调用NLSTK_CcpDeleteVoiceAssistantService删除服务；
 * 7. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回结果码成功。
 */
TEST_F(CCP_VAS_TEST, CCP_VAS_TEST_002)
{
    // 添加语音助手服务
    NLSTK_CcpVasInfo_S vasInfo = {0};
    vasInfo.state = NLSTK_VAS_STATE_IDLE;
    vasInfo.startService = Stub_StartServiceCbk;
    vasInfo.vasControlPoint.activate = Stub_ActivateCallback;
    vasInfo.vasControlPoint.terminate = Stub_TerminateCallback;
    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);
    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_VOICE_ASSISTANT_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 1);
    SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, 0);
    EXPECT_EQ(property->val->len, 1);
    uint8_t data1[] = {(uint8_t)NLSTK_VAS_STATE_IDLE};
    size_t len1 = sizeof(data1) / sizeof(data1[0]);
    EXPECT_EQ(memcmp(property->val->value, data1, len1), 0);

    // 更新语音助手状态
    ret = NLSTK_CcpUpdateVasState(NLSTK_VAS_STATE_ACTIVATED);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(property->val->len, 1);
    uint8_t data2[] = {(uint8_t)NLSTK_VAS_STATE_ACTIVATED};
    size_t len2 = sizeof(data2) / sizeof(data2[0]);
    EXPECT_EQ(memcmp(property->val->value, data2, len2), 0);

    // 删除语音助手服务
    ret = NLSTK_CcpDeleteVoiceAssistantService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test CCP_VAS_TEST_003
 * @brief 此测试用例用于验证语音助手服务端添加语音助手服务，此场景为正常添加，模拟接收语音助手激活请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateVoiceAssistantService添加语音助手服务；
 * 2. 校验回调函数触发添加成功；
 * 3. 校验SSAP服务中是否有语音助手服务；
 * 4. 使用TEST_DTAP_RecData模拟接收到语音助手激活请求；
 * 5. 校验Stub_ActivateCallback回调是否正常上报；
 * 6. 调用NLSTK_CcpDeleteVoiceAssistantService删除服务；
 * 7. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回结果码成功。
 */
TEST_F(CCP_VAS_TEST, CCP_VAS_TEST_003)
{
    // 添加语音助手服务
    NLSTK_CcpVasInfo_S vasInfo = {0};
    vasInfo.state = NLSTK_VAS_STATE_IDLE;
    vasInfo.startService = Stub_StartServiceCbk;
    vasInfo.vasControlPoint.activate = Stub_ActivateCallback;
    vasInfo.vasControlPoint.terminate = Stub_TerminateCallback;
    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);
    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_VOICE_ASSISTANT_SERVICE_UUID);

    // 模拟接收到激活请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t req[] = {0x13, 0x03, 0x00, 0x00, 0x01};
    (void)memcpy_s(req + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req, sizeof(req));
    TEST_RunQueueStubSchedule();

    // 校验语音助手请求回调是否正常上报
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);

    // 校验发送响应报文是否符合预期
    uint8_t rsp[] = {0x14, 0x03, 0x01, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp, sizeof(rsp)));

    // 更新语音助手状态
    ret = NLSTK_CcpUpdateVasState(NLSTK_VAS_STATE_ACTIVATED);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 删除语音助手服务
    ret = NLSTK_CcpDeleteVoiceAssistantService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test CCP_VAS_TEST_004
 * @brief 此测试用例用于验证语音助手服务端添加语音助手服务，此场景为正常添加，状态为激活，模拟接收语音助手结束请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateVoiceAssistantService添加语音助手服务；
 * 2. 校验回调函数触发添加成功；
 * 3. 校验SSAP服务中是否有语音助手服务；
 * 4. 使用TEST_DTAP_RecData模拟接收到语音助手结束请求；
 * 5. 校验Stub_TerminateCallback回调是否正常上报；
 * 6. 调用NLSTK_CcpDeleteVoiceAssistantService删除服务；
 * 7. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回结果码成功。
 */
TEST_F(CCP_VAS_TEST, CCP_VAS_TEST_004)
{
    // 添加语音助手服务
    NLSTK_CcpVasInfo_S vasInfo = {0};
    vasInfo.state = NLSTK_VAS_STATE_ACTIVATED;
    vasInfo.startService = Stub_StartServiceCbk;
    vasInfo.vasControlPoint.activate = Stub_ActivateCallback;
    vasInfo.vasControlPoint.terminate = Stub_TerminateCallback;
    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);
    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_VOICE_ASSISTANT_SERVICE_UUID);

    // 模拟接收到结束请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t req[] = {0x13, 0x03, 0x00, 0x00, 0x02};
    (void)memcpy_s(req + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req, sizeof(req));
    TEST_RunQueueStubSchedule();

    // 校验语音助手请求回调是否正常上报
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);

    // 校验发送响应报文是否符合预期
    uint8_t rsp[] = {0x14, 0x03, 0x02, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp, sizeof(rsp)));

    // 更新语音助手状态
    ret = NLSTK_CcpUpdateVasState(NLSTK_VAS_STATE_IDLE);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 删除语音助手服务
    ret = NLSTK_CcpDeleteVoiceAssistantService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test CCP_VAS_TEST_005
 * @brief 此测试用例用于验证语音助手服务端添加语音助手服务，模拟接收不匹配当前状态的语音助手请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateVoiceAssistantService添加语音助手服务；
 * 2. 校验回调函数触发添加成功；
 * 3. 校验SSAP服务中是否有语音助手服务；
 * 4. 使用TEST_DTAP_RecData模拟接收到语音助手结束请求；
 * 5. 校验Stub_TerminateCallback回调是否正常上报；
 * 6. 调用NLSTK_CcpDeleteVoiceAssistantService删除服务；
 * 7. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回结果码成功。
 */
TEST_F(CCP_VAS_TEST, CCP_VAS_TEST_005)
{
    // 添加语音助手服务
    NLSTK_CcpVasInfo_S vasInfo = {0};
    vasInfo.state = NLSTK_VAS_STATE_IDLE;
    vasInfo.startService = Stub_StartServiceCbk;
    vasInfo.vasControlPoint.activate = Stub_ActivateCallback;
    vasInfo.vasControlPoint.terminate = Stub_TerminateCallback;
    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);
    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_VOICE_ASSISTANT_SERVICE_UUID);

    // 模拟接收到结束请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t req1[] = {0x13, 0x03, 0x00, 0x00, 0x02};
    (void)memcpy_s(req1 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req1, sizeof(req1));
    TEST_RunQueueStubSchedule();

    // 校验语音助手请求回调是否上报, 预期不上报
    EXPECT_EQ(g_recvCbk, false);

    // 校验发送响应报文是否符合预期
    uint8_t rsp1[] = {0x14, 0x03, 0x02, 0xFF};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp1, sizeof(rsp1)));

    // 更新语音助手状态
    ret = NLSTK_CcpUpdateVasState(NLSTK_VAS_STATE_ACTIVATED);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到激活请求
    uint8_t req2[] = {0x13, 0x03, 0x00, 0x00, 0x01};
    (void)memcpy_s(req2 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req2, sizeof(req2));
    TEST_RunQueueStubSchedule();

    // 校验语音助手请求回调是否上报, 预期不上报
    EXPECT_EQ(g_recvCbk, false);

    // 校验发送响应报文是否符合预期
    uint8_t rsp2[] = {0x14, 0x03, 0x01, 0xFF};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp2, sizeof(rsp2)));


    // 删除语音助手服务
    ret = NLSTK_CcpDeleteVoiceAssistantService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

// 此用例覆盖需要授权，但是回调为空
TEST_F(CCP_VAS_TEST, CCP_VAS_TEST_007)
{
    // 添加语音助手服务
    NLSTK_CcpVasInfo_S vasInfo = {0};
    vasInfo.state = NLSTK_VAS_STATE_IDLE;
    vasInfo.startService = Stub_StartServiceCbk;
    vasInfo.vasControlPoint.activate = Stub_ActivateCallback;
    vasInfo.vasControlPoint.terminate = Stub_TerminateCallback;
    vasInfo.stateRight = NLSTK_VAS_READ_AUTHOR;
    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru((uint16_t)CCP_VOICE_ASSISTANT_STATE_UUID);
    size_t index = 0;
    EXPECT_EQ(SDF_VectorFindFirst(service->properties, CompUuidFunc, &uuidStru, &index), true);
    SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, index);
    uint16_t handle = property->handle;
    size_t handleOffset = 0x02;

    // 模拟收到读取请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    uint8_t readReq[] = {0x08, 0x03, 0x00, 0x00, 0x00};
    (void)memcpy_s(readReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, readReq, sizeof(readReq));
    TEST_RunQueueStubSchedule();

    // 校验发送响应报文是否符合预期
    uint8_t errRsp[] = {0x09, 0x0B, 0x09, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(errRsp, sizeof(errRsp)));
    Stub_VasResetData();

    // 删除语音助手服务
    ret = NLSTK_CcpDeleteVoiceAssistantService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
}

// 此用例覆盖收到语音助手请求，但是用户注册的语音助手回调为空
TEST_F(CCP_VAS_TEST, CCP_VAS_TEST_008)
{
    // 添加通用语音助手实例
    NLSTK_CcpVasInfo_S vasInfo = {0};
    vasInfo.state = NLSTK_VAS_STATE_ACTIVATED;
    vasInfo.startService = Stub_StartServiceCbk;
    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;

    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 模拟接收到结束请求
    uint8_t req1[] = {0x13, 0x03, 0x00, 0x00, 0x02};
    (void)memcpy_s(req1 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req1, sizeof(req1));
    TEST_RunQueueStubSchedule();

    // 校验发送响应报文是否符合预期
    uint8_t rsp1[] = {0x14, 0x03, 0x02, 0xFF};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp1, sizeof(rsp1)));

    // 更新语音助手状态
    ret = NLSTK_CcpUpdateVasState(NLSTK_VAS_STATE_IDLE);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到激活请求
    uint8_t req2[] = {0x13, 0x03, 0x00, 0x00, 0x01};
    (void)memcpy_s(req2 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req2, sizeof(req2));
    TEST_RunQueueStubSchedule();

    // 校验发送响应报文是否符合预期
    uint8_t rsp2[] = {0x14, 0x03, 0x01, 0xFF};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp2, sizeof(rsp2)));

    // 删除语音助手服务
    ret = NLSTK_CcpDeleteVoiceAssistantService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
}

// 此用例覆盖收到语音助手请求，用户返回失败
TEST_F(CCP_VAS_TEST, CCP_VAS_TEST_009)
{
    // 添加通用语音助手实例
    NLSTK_CcpVasInfo_S vasInfo = {0};
    vasInfo.state = NLSTK_VAS_STATE_ACTIVATED;
    vasInfo.startService = Stub_StartServiceCbk;
    vasInfo.vasControlPoint.activate = Stub_ActivateFailCallback;
    vasInfo.vasControlPoint.terminate = Stub_TerminateFailCallback;
    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);
    Stub_VasResetData();

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;

    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 模拟接收到结束请求
    uint8_t req1[] = {0x13, 0x03, 0x00, 0x00, 0x02};
    (void)memcpy_s(req1 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req1, sizeof(req1));
    TEST_RunQueueStubSchedule();

    // 校验语音助手请求回调是否上报, 预期上报
    EXPECT_EQ(g_recvCbk, true);

    // 校验发送响应报文是否符合预期
    uint8_t rsp1[] = {0x14, 0x03, 0x02, 0xFF};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp1, sizeof(rsp1)));

    // 更新语音助手状态
    ret = NLSTK_CcpUpdateVasState(NLSTK_VAS_STATE_IDLE);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到激活请求
    uint8_t req2[] = {0x13, 0x03, 0x00, 0x00, 0x01};
    (void)memcpy_s(req2 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req2, sizeof(req2));
    TEST_RunQueueStubSchedule();

    // 校验语音助手请求回调是否上报, 预期上报
    EXPECT_EQ(g_recvCbk, true);

    // 校验发送响应报文是否符合预期
    uint8_t rsp2[] = {0x14, 0x03, 0x01, 0xFF};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp2, sizeof(rsp2)));

    // 删除语音助手服务
    ret = NLSTK_CcpDeleteVoiceAssistantService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
}

// 此用例覆盖收到错误的语音助手请求
TEST_F(CCP_VAS_TEST, CCP_VAS_TEST_010)
{
    // 添加通用语音助手实例
    NLSTK_CcpVasInfo_S vasInfo = {0};
    vasInfo.state = NLSTK_VAS_STATE_ACTIVATED;
    vasInfo.startService = Stub_StartServiceCbk;
    vasInfo.vasControlPoint.activate = Stub_ActivateFailCallback;
    vasInfo.vasControlPoint.terminate = Stub_TerminateFailCallback;
    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);
    Stub_VasResetData();

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;

    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 模拟接收到结束请求
    uint8_t req[] = {0x13, 0x03, 0x00, 0x00, 0xFF};
    (void)memcpy_s(req + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req, sizeof(req));
    TEST_RunQueueStubSchedule();

    // 校验语音助手请求回调是否上报, 预期不上报
    EXPECT_EQ(g_recvCbk, false);

    // 校验发送响应报文是否符合预期
    uint8_t rsp[] = {0x14, 0x03, 0xFF, 0xFF};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp, sizeof(rsp)));

    // 删除语音助手服务
    ret = NLSTK_CcpDeleteVoiceAssistantService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
}

// // 此用例覆盖重复添加场景
TEST_F(CCP_VAS_TEST, CCP_VAS_TEST_011)
{
    // 添加通用语音助手实例
    NLSTK_CcpVasInfo_S vasInfo = {0};
    vasInfo.state = NLSTK_VAS_STATE_ACTIVATED;
    vasInfo.startService = Stub_StartServiceCbk;
    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);
    Stub_VasResetData();

    // 重复添加
    ret = NLSTK_CcpCreateVoiceAssistantService(&vasInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_FAIL);

    // 删除语音助手服务
    ret = NLSTK_CcpDeleteVoiceAssistantService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
}

TEST_F(CCP_VAS_TEST, CCP_VAS_TEST_014)
{
    CcpCreateVoiceAssistantService(NULL);
    TEST_RunQueueStubSchedule();
    CcpVasStateAuthorizeResult(NULL);
    TEST_RunQueueStubSchedule();
    CcpVasControlResult(NULL);
    TEST_RunQueueStubSchedule();
    CcpUpdateVasState(NULL);
    TEST_RunQueueStubSchedule();

    uint32_t ret = NLSTK_CcpCreateVoiceAssistantService(NULL);
    EXPECT_NE(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
}