/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "ccp_ccs_server.h"
#include "nlstk_ccp.h"
#include "nlstk_ccp_ccs_server.h"

#include "cpfwk_log.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

class CCP_CCS_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;

    static void Stub_CcpResetData();

    static void Stub_StartCcsInstCbk(int32_t instanceId, NLSTK_Errcode_E ret);
    static void Stub_AuthorizeCbk(uint32_t requestId, int32_t instanceId, NLSTK_CcpCcsPropertyType_E property,
                                  NLSTK_ServicePropertyOpType_E operation);
    static void Stub_AnswerCallback(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId);
    static void Stub_HangUpCallback(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId);
    static void Stub_AnswerFailCallback(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId);
    static void Stub_HangUpFailCallback(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId);

    static int32_t g_instanceId;
    static uint32_t g_requestId;
    static uint32_t g_addResult;
    static SLE_Addr_S g_recvAddr;
    static bool g_recvCbk;
    static NLSTK_CcpCcsPropertyType_E g_type;
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

        Stub_CcpResetData();
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

int32_t CCP_CCS_TEST::g_instanceId = -1;
uint32_t CCP_CCS_TEST::g_requestId = 0;
uint32_t CCP_CCS_TEST::g_addResult = NLSTK_ERRCODE_FAIL;
SLE_Addr_S CCP_CCS_TEST::g_recvAddr = {0};
bool CCP_CCS_TEST::g_recvCbk = false;
NLSTK_CcpCcsPropertyType_E CCP_CCS_TEST::g_type = NLSTK_CCP_CCS_MAX_PROPERTY;

void CCP_CCS_TEST::Stub_CcpResetData()
{
    g_instanceId = -1;
    g_requestId = 0;
    g_addResult = NLSTK_ERRCODE_FAIL;
    g_recvAddr = {0};
    g_recvCbk = false;
    g_type = NLSTK_CCP_CCS_MAX_PROPERTY;
}


void CCP_CCS_TEST::Stub_StartCcsInstCbk(int32_t instanceId, NLSTK_Errcode_E ret)
{
    g_instanceId = instanceId;
    g_addResult = ret;
}

void CCP_CCS_TEST::Stub_AuthorizeCbk(uint32_t requestId, int32_t instanceId, NLSTK_CcpCcsPropertyType_E property,
                                         NLSTK_ServicePropertyOpType_E operation)
{
    g_requestId = requestId;
    g_instanceId = instanceId;
    g_type = property;
}

void CCP_CCS_TEST::Stub_AnswerCallback(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_CcpCallControlResult(requestId, instanceId, 0);
}

void CCP_CCS_TEST::Stub_HangUpCallback(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_CcpCallControlResult(requestId, instanceId, 0);
}

void CCP_CCS_TEST::Stub_AnswerFailCallback(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_CcpCallControlResult(requestId, instanceId, 0xFF);
}

void CCP_CCS_TEST::Stub_HangUpFailCallback(SLE_Addr_S *addr, int32_t instanceId, uint32_t requestId, uint8_t callId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_CcpCallControlResult(requestId, instanceId, 0xFF);
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
 * @test CCP_CCS_TEST_001
 * @brief 此测试用例用于验证通话控制服务端添加通用通话控制实例，此场景为正常添加。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateCcsInstance添加通用通话控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用通话控制服务；
 * 4. 调用NLSTK_CcpDeleteCcsInstance删除实例；
 * 5. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_001)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);
    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_CALL_CONTROL_COMMON_SERVICE_UUID);

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test CCP_CCS_TEST_002
 * @brief 此测试用例用于验证通话控制服务端添加通用通话控制实例，此场景为异常添加。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateCcsInstance添加通话控制实例；
 * 2. 校验返回结果不为成功；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：调用函数返回结果不为成功。
 */
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_002)
{
    // 异常1：入参为空
    uint32_t ret = NLSTK_CcpCreateCcsInstance(NULL);
    EXPECT_NE(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 异常2：类型为通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo1 = {};
    memset_s(&basicInfo1, sizeof(basicInfo1), 0, sizeof(basicInfo1));
    basicInfo1.type = NLSTK_CCP_CALL_CONTROL_SERVICE;
    basicInfo1.mediaInstanceId = 0xFF;
    basicInfo1.startCcsInst = Stub_StartCcsInstCbk;
    ret = NLSTK_CcpCreateCcsInstance(&basicInfo1);
    EXPECT_NE(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 异常3：通话实例标识为无效值
    NLSTK_CcpCallControlInfo_S basicInfo2 = {};
    memset_s(&basicInfo2, sizeof(basicInfo2), 0, sizeof(basicInfo2));
    basicInfo2.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo2.mediaInstanceId = 0;
    basicInfo2.startCcsInst = Stub_StartCcsInstCbk;
    ret = NLSTK_CcpCreateCcsInstance(&basicInfo2);
    EXPECT_NE(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
}

/**
 * @test CCP_CCS_TEST_003
 * @brief 此测试用例用于验证通话控制服务端添加通用通话控制实例，此场景为正常添加，必选属性正常赋值。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateCcsInstance添加通用通话控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用通话控制服务；
 * 4. 校验通用通话控制服务中属性数量是否为必选属性数；
 * 5. 调用NLSTK_CcpDeleteCcsInstance删除实例；
 * 6. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_003)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    char instanceName[] = "NearlinkCall";
    basicInfo.instanceName.data = reinterpret_cast<uint8_t*>(instanceName);
    basicInfo.instanceName.len = strlen(instanceName);
    basicInfo.featureStatus = NLSTK_CCP_IN_BAND_RING_ON;  // bit0：0: 带内铃音关闭 1: 带内铃音开启
    char supportProtocol[] = "tel:";
    basicInfo.protocolSupport.data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(supportProtocol));
    basicInfo.protocolSupport.len = strlen(supportProtocol);
    basicInfo.callRequestSupport = NLSTK_CCP_CALL_REQUEST_SUPPORT_NOT;
    basicInfo.callControlPoint.answer = &Stub_AnswerCallback;
    basicInfo.callControlPoint.hangUp = &Stub_HangUpCallback;
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    basicInfo.authorize = Stub_AuthorizeCbk;
    basicInfo.propertyRights[NLSTK_CCP_CCS_CALL_REQ_SUPPORT] = NLSTK_CCS_READ_AUTHOR;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_CALL_CONTROL_COMMON_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 8);
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru((uint16_t)CCP_CCS_CALL_REQ_SUPPORT_UUID);
    size_t index = 0;
    SDF_VectorFindFirst(service->properties, CompUuidFunc, &uuidStru, &index);
    SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, index);
    uint16_t handle = property->handle;
    size_t handleOffset = 0x02;

    // 模拟收到读取通话请求支持请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    uint8_t readReq[] = {0x08, 0x03, 0x00, 0x00, 0x00};
    (void)memcpy_s(readReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, readReq, sizeof(readReq));
    TEST_RunQueueStubSchedule();

    // 校验授权回调是否正常上报
    EXPECT_EQ(g_type, NLSTK_CCP_CCS_CALL_REQ_SUPPORT);

    ret = NLSTK_CcpCcsAuthorizeResult(g_requestId, g_instanceId, g_type, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 校验发送响应报文是否符合预期
    uint8_t readRsp[] = {0x09, 0x03, 0x00, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(readRsp, sizeof(readRsp)));
    Stub_CcpResetData();

    // 模拟再次收到读取通话请求支持请求
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, readReq, sizeof(readReq));
    TEST_RunQueueStubSchedule();

    // 校验授权回调是否正常上报
    EXPECT_EQ(g_type, NLSTK_CCP_CCS_CALL_REQ_SUPPORT);

    ret = NLSTK_CcpCcsAuthorizeResult(g_requestId, g_instanceId, g_type, NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 校验发送响应报文是否符合预期
    uint8_t errRsp[] = {0x09, 0x0B, 0x09, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(errRsp, sizeof(errRsp)));
    Stub_CcpResetData();

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test CCP_CCS_TEST_004
 * @brief 此测试用例用于验证通话控制服务端添加通用通话控制实例，此场景为正常添加，并勾选所有可选属性。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateCcsInstance添加通用通话控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用通话控制服务；
 * 4. 校验通用通话控制服务中属性数量是否为所有属性数；
 * 5. 调用NLSTK_CcpDeleteCcsInstance删除实例；
 * 6. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_004)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    basicInfo.instanceIconFlag = true;
    basicInfo.networkSelectionFlag = true;
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_CALL_CONTROL_COMMON_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 10);

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test CCP_CCS_TEST_005
 * @brief 此测试用例用于验证通话控制正常业务交互，首先添加通话实例，然后将通话状态更新为来电，之后模拟接收客户端接听请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateCcsInstance添加通用通话控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用通话控制服务；
 * 4. 校验通用通话控制服务中属性数量是否为必选属性数；
 * 5. 调用NLSTK_CcpUpdateCcsProperty更新通话状态为来电；
 * 6. 使用TEST_DTAP_RecData模拟接收到接听方法调用请求；
 * 7. 校验Stub_AnswerCallback回调是否正常上报；
 * 8. Stub_AnswerCallback中调用NLSTK_CcpCallControlResult返回方法调用结果；
 * 9. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期；
 * 10. 调用NLSTK_CcpDeleteCcsInstance删除实例；
 * 11. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_005)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    char instanceName[] = "NearlinkCall";
    basicInfo.instanceName.data = reinterpret_cast<uint8_t*>(instanceName);
    basicInfo.instanceName.len = strlen(instanceName);
    basicInfo.featureStatus = NLSTK_CCP_IN_BAND_RING_ON;  // bit0：0: 带内铃音关闭 1: 带内铃音开启
    char supportProtocol[] = "tel:";
    basicInfo.protocolSupport.data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(supportProtocol));
    basicInfo.protocolSupport.len = strlen(supportProtocol);
    basicInfo.callRequestSupport = NLSTK_CCP_CALL_REQUEST_SUPPORT_NOT;
    basicInfo.callControlPoint.answer = &Stub_AnswerCallback;
    basicInfo.callControlPoint.hangUp = &Stub_HangUpCallback;
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_CALL_CONTROL_COMMON_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 8);

    // 更新通话状态为来电
    NLSTK_CcpCallStatues_S callStatus = {0};
    callStatus.callCount = 1;
    callStatus.callId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.networkId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.callStatus = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.callFlag = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    *callStatus.callId = 0x01;
    *callStatus.networkId = 0x00;
    *callStatus.callStatus = 0x01; // 来电
    *callStatus.callFlag = 0x00;
    ret = NLSTK_CcpUpdateCcsProperty(basicInfo.mediaInstanceId, NLSTK_CCP_CCS_CALL_STATUS, &callStatus);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到接听请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t answerReq[] = {0x13, 0x03, 0x00, 0x00, 0x02, 0x01};
    (void)memcpy_s(answerReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, answerReq, sizeof(answerReq));
    TEST_RunQueueStubSchedule();

    // 校验通话请求回调是否正常上报
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);

    // 校验发送响应报文是否符合预期
    uint8_t answerRsp[] = {0x14, 0x03, 0x02, 0x00, 0x01};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(answerRsp, sizeof(answerRsp)));

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);

    SDF_MemFree(callStatus.callId);
    SDF_MemFree(callStatus.networkId);
    SDF_MemFree(callStatus.callStatus);
    SDF_MemFree(callStatus.callFlag);
}

/**
 * @test CCP_CCS_TEST_005
 * @brief 此测试用例用于验证通话控制正常业务交互，首先添加通话实例，然后将通话状态更新为一通接通一通来电，之后模拟接收客户端挂断请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateCcsInstance添加通用通话控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用通话控制服务；
 * 4. 校验通用通话控制服务中属性数量是否为必选属性数；
 * 5. 调用NLSTK_CcpUpdateCcsProperty更新通话状态为一通接通一通来电；
 * 6. 使用TEST_DTAP_RecData模拟接收到挂断方法调用请求；
 * 7. 校验Stub_HangUpCallback回调是否正常上报；
 * 8. Stub_HangUpCallback中调用NLSTK_CcpCallControlResult返回方法调用结果；
 * 9. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期；
 * 10. 调用NLSTK_CcpDeleteCcsInstance删除实例；
 * 11. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_006)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    char instanceName[] = "NearlinkCall";
    basicInfo.instanceName.data = reinterpret_cast<uint8_t*>(instanceName);
    basicInfo.instanceName.len = strlen(instanceName);
    basicInfo.featureStatus = NLSTK_CCP_IN_BAND_RING_ON;  // bit0：0: 带内铃音关闭 1: 带内铃音开启
    char supportProtocol[] = "tel:";
    basicInfo.protocolSupport.data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(supportProtocol));
    basicInfo.protocolSupport.len = strlen(supportProtocol);
    basicInfo.callRequestSupport = NLSTK_CCP_CALL_REQUEST_SUPPORT_NOT;
    basicInfo.callControlPoint.answer = &Stub_AnswerCallback;
    basicInfo.callControlPoint.hangUp = &Stub_HangUpCallback;
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_CALL_CONTROL_COMMON_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 8);

    // 更新通话状态为一通接通一通来电
    NLSTK_CcpCallStatues_S callStatus = {0};
    callStatus.callCount = 2;
    callStatus.callId = (uint8_t *)SDF_MemZalloc(sizeof(uint16_t));
    callStatus.networkId = (uint8_t *)SDF_MemZalloc(sizeof(uint16_t));
    callStatus.callStatus = (uint8_t *)SDF_MemZalloc(sizeof(uint16_t));
    callStatus.callFlag = (uint8_t *)SDF_MemZalloc(sizeof(uint16_t));
    *callStatus.callId = 0x01;
    *callStatus.networkId = 0x00;
    *callStatus.callStatus = 0x04; // 接通
    *callStatus.callFlag = 0x00;
    *(callStatus.callId + 1) = 0x02;
    *(callStatus.networkId + 1) = 0x00;
    *(callStatus.callStatus + 1) = 0x01; // 来电
    *(callStatus.callFlag + 1) = 0x00;
    ret = NLSTK_CcpUpdateCcsProperty(basicInfo.mediaInstanceId, NLSTK_CCP_CCS_CALL_STATUS, &callStatus);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 挂断来电电话
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    // 挂断来电电话
    uint8_t req1[] = {0x13, 0x03, 0x00, 0x00, 0x03, 0x02};
    (void)memcpy_s(req1 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req1, sizeof(req1));
    TEST_RunQueueStubSchedule();

    // 校验通话请求回调是否正常上报
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);

    // 校验发送响应报文是否符合预期
    uint8_t rsp1[] = {0x14, 0x03, 0x03, 0x00, 0x02};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp1, sizeof(rsp1)));

    g_recvCbk = false;
    (void)memset_s(&g_recvAddr, sizeof(SLE_Addr_S), 0, sizeof(SLE_Addr_S));

    // 挂断接通电话
    uint8_t req2[] = {0x13, 0x03, 0x00, 0x00, 0x03, 0x01};
    (void)memcpy_s(req2 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req2, sizeof(req2));
    TEST_RunQueueStubSchedule();

    // 校验通话请求回调是否正常上报
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);

    // 校验发送响应报文是否符合预期
    uint8_t rsp2[] = {0x14, 0x03, 0x03, 0x00, 0x01};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp2, sizeof(rsp2)));

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);

    SDF_MemFree(callStatus.callId);
    SDF_MemFree(callStatus.networkId);
    SDF_MemFree(callStatus.callStatus);
    SDF_MemFree(callStatus.callFlag);
}

/**
 * @test CCP_CCS_TEST_007
 * @brief 此测试用例用于验证通话控制异常业务交互，首先添加通话实例，然后将通话状态更新为接通，之后模拟接收客户端接听请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateCcsInstance添加通用通话控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用通话控制服务；
 * 4. 校验通用通话控制服务中属性数量是否为必选属性数；
 * 5. 调用NLSTK_CcpUpdateCcsProperty更新通话状态为接通；
 * 6. 使用TEST_DTAP_RecData模拟接收到接听方法调用请求；
 * 7. 校验Stub_AnswerCallback回调是否上报，预期不上报；
 * 8. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期，预期回复状态机错误0x03错误码；
 * 9. 调用NLSTK_CcpDeleteCcsInstance删除实例；
 * 10. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_007)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    char instanceName[] = "NearlinkCall";
    basicInfo.instanceName.data = reinterpret_cast<uint8_t*>(instanceName);
    basicInfo.instanceName.len = strlen(instanceName);
    basicInfo.featureStatus = NLSTK_CCP_IN_BAND_RING_ON;  // bit0：0: 带内铃音关闭 1: 带内铃音开启
    char supportProtocol[] = "tel:";
    basicInfo.protocolSupport.data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(supportProtocol));
    basicInfo.protocolSupport.len = strlen(supportProtocol);
    basicInfo.callRequestSupport = NLSTK_CCP_CALL_REQUEST_SUPPORT_NOT;
    basicInfo.callControlPoint.answer = &Stub_AnswerCallback;
    basicInfo.callControlPoint.hangUp = &Stub_HangUpCallback;
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_CALL_CONTROL_COMMON_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 8);

    // 更新通话状态为接通
    NLSTK_CcpCallStatues_S callStatus = {0};
    callStatus.callCount = 1;
    callStatus.callId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.networkId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.callStatus = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.callFlag = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    *callStatus.callId = 0x01;
    *callStatus.networkId = 0x00;
    *callStatus.callStatus = 0x04; // 接通
    *callStatus.callFlag = 0x00;
    ret = NLSTK_CcpUpdateCcsProperty(basicInfo.mediaInstanceId, NLSTK_CCP_CCS_CALL_STATUS, &callStatus);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到接听请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t answerReq[] = {0x13, 0x03, 0x00, 0x00, 0x02, 0x01};
    (void)memcpy_s(answerReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, answerReq, sizeof(answerReq));
    TEST_RunQueueStubSchedule();

    // 校验通话请求回调上报，预期不上报
    EXPECT_EQ(g_recvCbk, false);

    // 校验发送响应报文是否符合预期，预期回复状态机错误0x03错误码
    uint8_t answerRsp[] = {0x14, 0x03, 0x02, 0x03, 0x01, 0x04};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(answerRsp, sizeof(answerRsp)));

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);

    SDF_MemFree(callStatus.callId);
    SDF_MemFree(callStatus.networkId);
    SDF_MemFree(callStatus.callStatus);
    SDF_MemFree(callStatus.callFlag);
}

/**
 * @test CCP_CCS_TEST_008
 * @brief 此测试用例用于验证通话控制异常业务交互，添加通话实例，不更新通话状态，当前无通话。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateCcsInstance添加通用通话控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用通话控制服务；
 * 4. 校验通用通话控制服务中属性数量是否为必选属性数；
 * 6. 使用TEST_DTAP_RecData模拟接收到接听方法调用请求；
 * 7. 校验Stub_AnswerCallback回调是否上报，预期不上报；
 * 6. 使用TEST_DTAP_RecData模拟接收到挂断方法调用请求；
 * 7. 校验Stub_HangUpCallback回调是否上报，预期不上报；
 * 8. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期，预期回复通话标识不存在错误码0x02；
 * 9. 调用NLSTK_CcpDeleteCcsInstance删除实例；
 * 10. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_008)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    char instanceName[] = "NearlinkCall";
    basicInfo.instanceName.data = reinterpret_cast<uint8_t*>(instanceName);
    basicInfo.instanceName.len = strlen(instanceName);
    basicInfo.featureStatus = NLSTK_CCP_IN_BAND_RING_ON;  // bit0：0: 带内铃音关闭 1: 带内铃音开启
    char supportProtocol[] = "tel:";
    basicInfo.protocolSupport.data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(supportProtocol));
    basicInfo.protocolSupport.len = strlen(supportProtocol);
    basicInfo.callRequestSupport = NLSTK_CCP_CALL_REQUEST_SUPPORT_NOT;
    basicInfo.callControlPoint.answer = &Stub_AnswerCallback;
    basicInfo.callControlPoint.hangUp = &Stub_HangUpCallback;
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_CALL_CONTROL_COMMON_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 8);

    // 模拟接收到接听请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t req1[] = {0x13, 0x03, 0x00, 0x00, 0x02, 0xFF};
    (void)memcpy_s(req1 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req1, sizeof(req1));
    TEST_RunQueueStubSchedule();

    // 校验通话请求回调是否上报，预期不上报
    EXPECT_EQ(g_recvCbk, false);

    // 校验发送响应报文是否符合预期，预期回复通话标识不存在0x02错误码
    uint8_t rsp1[] = {0x14, 0x03, 0x02, 0x02};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp1, sizeof(rsp1)));

    g_recvCbk = false;

    // 模拟接收到挂断请求
    uint8_t req2[] = {0x13, 0x03, 0x00, 0x00, 0x03, 0x10};
    (void)memcpy_s(req2 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req2, sizeof(req2));
    TEST_RunQueueStubSchedule();

    // 校验通话请求回调是否上报，预期不上报
    EXPECT_EQ(g_recvCbk, false);

    // 校验发送响应报文是否符合预期，预期回复通话标识不存在0x02错误码
    uint8_t rsp2[] = {0x14, 0x03, 0x03, 0x02};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp2, sizeof(rsp2)));

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test CCP_CCS_TEST_009
 * @brief 此测试用例用于验证通话控制异常业务交互，添加通话实例，更新通话状态为来电，之后模拟客户端发送接收不支持的本地保持请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_CcpCreateCcsInstance添加通用通话控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用通话控制服务；
 * 4. 校验通用通话控制服务中属性数量是否为必选属性数；
 * 6. 使用TEST_DTAP_RecData模拟接收到本地保持方法调用请求；
 * 7. 校验Stub_AnswerCallback回调是否上报，预期不上报；
 * 8. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期，预期回复请求类型不支持错误码0x01；
 * 9. 调用NLSTK_CcpDeleteCcsInstance删除实例；
 * 10. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - CCP模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_009)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    char instanceName[] = "NearlinkCall";
    basicInfo.instanceName.data = reinterpret_cast<uint8_t*>(instanceName);
    basicInfo.instanceName.len = strlen(instanceName);
    basicInfo.featureStatus = NLSTK_CCP_IN_BAND_RING_ON;  // bit0：0: 带内铃音关闭 1: 带内铃音开启
    char supportProtocol[] = "tel:";
    basicInfo.protocolSupport.data = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(supportProtocol));
    basicInfo.protocolSupport.len = strlen(supportProtocol);
    basicInfo.callRequestSupport = NLSTK_CCP_CALL_REQUEST_SUPPORT_NOT;
    basicInfo.callControlPoint.answer = &Stub_AnswerCallback;
    basicInfo.callControlPoint.hangUp = &Stub_HangUpCallback;
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_CALL_CONTROL_COMMON_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 8);

    // 更新通话状态为接通
    NLSTK_CcpCallStatues_S callStatus = {0};
    callStatus.callCount = 1;
    callStatus.callId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.networkId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.callStatus = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.callFlag = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    *callStatus.callId = 0x80;
    *callStatus.networkId = 0x00;
    *callStatus.callStatus = 0x04; // 接通
    *callStatus.callFlag = 0x00;
    ret = NLSTK_CcpUpdateCcsProperty(basicInfo.mediaInstanceId, NLSTK_CCP_CCS_CALL_STATUS, &callStatus);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到本地保持请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    ASSERT_TRUE(method != NULL);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t req[] = {0x13, 0x03, 0x00, 0x00, 0x04, 0x80};
    (void)memcpy_s(req + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req, sizeof(req));
    TEST_RunQueueStubSchedule();

    // 校验通话请求回调是否上报，预期不上报
    EXPECT_EQ(g_recvCbk, false);

    // 校验发送响应报文是否符合预期，预期回复请求类型不支持错误码0x01
    uint8_t rsp[] = {0x14, 0x03, 0x04, 0x01};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp, sizeof(rsp)));

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);

    SDF_MemFree(callStatus.callId);
    SDF_MemFree(callStatus.networkId);
    SDF_MemFree(callStatus.callStatus);
    SDF_MemFree(callStatus.callFlag);
}

// 此用例覆盖需要授权，但是回调为空
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_011)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    basicInfo.propertyRights[NLSTK_CCP_CCS_CALL_REQ_SUPPORT] = NLSTK_CCS_READ_AUTHOR;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = CcpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)CCP_CALL_CONTROL_COMMON_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 8);
    NLSTK_SsapUuid_S uuidStru = CcpConvertUuidToStru((uint16_t)CCP_CCS_CALL_REQ_SUPPORT_UUID);
    size_t index = 0;
    SDF_VectorFindFirst(service->properties, CompUuidFunc, &uuidStru, &index);
    SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, index);
    uint16_t handle = property->handle;
    size_t handleOffset = 0x02;

    // 模拟收到读取通话请求支持请求
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
    Stub_CcpResetData();

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

// 此用例覆盖非必选属性正常赋值
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_012)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    basicInfo.instanceIconFlag = true;
    basicInfo.networkSelectionFlag = true;
    // 通话状态
    basicInfo.callStatus.callCount = 1;
    basicInfo.callStatus.callId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    basicInfo.callStatus.networkId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    basicInfo.callStatus.callStatus = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    basicInfo.callStatus.callFlag = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    *basicInfo.callStatus.callId = 0x80;
    *basicInfo.callStatus.networkId = 0x00;
    *basicInfo.callStatus.callStatus = 0x04; // 接通
    *basicInfo.callStatus.callFlag = 0x00;
    // 呼入呼出信息
    basicInfo.callInOutInfo.callId = 0x80;
    basicInfo.callInOutInfo.callFlag = 0x00;
    basicInfo.callInOutInfo.networkId = 0x00;
    char name[] = "Lihua";
    basicInfo.callInOutInfo.userInfo.len = strlen(name);
    basicInfo.callInOutInfo.userInfo.data = reinterpret_cast<uint8_t*>(name);
    basicInfo.callInOutInfo.userAlias.len = strlen(name);
    basicInfo.callInOutInfo.userAlias.data = reinterpret_cast<uint8_t*>(name);
    // 通话实例图标
    char icon[] = "Icon";
    basicInfo.instanceIcon.type = 0;
    basicInfo.instanceIcon.icon = reinterpret_cast<uint8_t*>(icon);
    basicInfo.instanceIcon.len = strlen(icon);
    // 网络选择
    char networkSelection[] = "NetworkSelection";
    basicInfo.networkSelection.data = reinterpret_cast<uint8_t*>(networkSelection);
    basicInfo.networkSelection.len = strlen(networkSelection);
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    SDF_MemFree(basicInfo.callStatus.callId);
    SDF_MemFree(basicInfo.callStatus.networkId);
    SDF_MemFree(basicInfo.callStatus.callStatus);
    SDF_MemFree(basicInfo.callStatus.callFlag);
}

// 此用例覆盖通话控制服务依次更新各属性
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_013)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    basicInfo.instanceIconFlag = true;
    basicInfo.networkSelectionFlag = true;
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    // 通话状态
    basicInfo.callStatus.callCount = 1;
    basicInfo.callStatus.callId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    basicInfo.callStatus.networkId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    basicInfo.callStatus.callStatus = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    basicInfo.callStatus.callFlag = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    *basicInfo.callStatus.callId = 0x80;
    *basicInfo.callStatus.networkId = 0x00;
    *basicInfo.callStatus.callStatus = 0x04; // 接通
    *basicInfo.callStatus.callFlag = 0x00;
    // 呼入呼出信息
    basicInfo.callInOutInfo.callId = 0x80;
    basicInfo.callInOutInfo.callFlag = 0x00;
    basicInfo.callInOutInfo.networkId = 0x00;
    char name[] = "Lihua";
    basicInfo.callInOutInfo.userInfo.len = strlen(name);
    basicInfo.callInOutInfo.userInfo.data = reinterpret_cast<uint8_t*>(name);
    basicInfo.callInOutInfo.userAlias.len = strlen(name);
    basicInfo.callInOutInfo.userAlias.data = reinterpret_cast<uint8_t*>(name);
    // 通话实例图标
    char icon[] = "Icon";
    basicInfo.instanceIcon.type = 0;
    basicInfo.instanceIcon.icon = reinterpret_cast<uint8_t*>(icon);
    basicInfo.instanceIcon.len = strlen(icon);
    // 网络选择
    char networkSelection[] = "NetworkSelection";
    basicInfo.networkSelection.data = reinterpret_cast<uint8_t*>(networkSelection);
    basicInfo.networkSelection.len = strlen(networkSelection);

    ret = NLSTK_CcpUpdateCcsProperty(0xFF, NLSTK_CCP_CCS_CALL_STATUS, &basicInfo.callStatus);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    ret = NLSTK_CcpUpdateCcsProperty(0xFF, NLSTK_CCP_CCS_CALLIN_OUT_INFO, &basicInfo.callInOutInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    ret = NLSTK_CcpUpdateCcsProperty(0xFF, NLSTK_CCP_CCS_INSTANCE_ICON, &basicInfo.instanceIcon);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    ret = NLSTK_CcpUpdateCcsProperty(0xFF, NLSTK_CCP_CCS_NETWORK_SELECTION, &basicInfo.networkSelection);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    ret = NLSTK_CcpUpdateCcsProperty(0xFF, NLSTK_CCP_CCS_CALL_TERMINATION, &basicInfo.callTermination);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    ret = NLSTK_CcpUpdateCcsProperty(0xFF, NLSTK_CCP_CCS_FEATURE_STATUS, &basicInfo.featureStatus);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    SDF_MemFree(basicInfo.callStatus.callId);
    SDF_MemFree(basicInfo.callStatus.networkId);
    SDF_MemFree(basicInfo.callStatus.callStatus);
    SDF_MemFree(basicInfo.callStatus.callFlag);
}

// 此用例覆盖收到通话控制请求，但是用户注册的通话控制回调为空
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_014)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;

    // 更新通话状态为来电
    NLSTK_CcpCallStatues_S callStatus = {0};
    callStatus.callCount = 1;
    callStatus.callId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.networkId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.callStatus = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.callFlag = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    *callStatus.callId = 0x80;
    *callStatus.networkId = 0x00;
    *callStatus.callStatus = 0x01; // 来电
    *callStatus.callFlag = 0x00;
    ret = NLSTK_CcpUpdateCcsProperty(basicInfo.mediaInstanceId, NLSTK_CCP_CCS_CALL_STATUS, &callStatus);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 模拟接收到接听请求
    uint8_t answerReq[] = {0x13, 0x03, 0x00, 0x00, 0x02, 0x80};
    (void)memcpy_s(answerReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, answerReq, sizeof(answerReq));
    TEST_RunQueueStubSchedule();

    // 校验发送响应报文是否符合预期
    uint8_t answerRsp[] = {0x14, 0x03, 0x02, 0xFF};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(answerRsp, sizeof(answerRsp)));

    // 模拟接收到挂断请求
    uint8_t hangUpReq[] = {0x13, 0x03, 0x00, 0x00, 0x03, 0x80};
    (void)memcpy_s(hangUpReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, hangUpReq, sizeof(hangUpReq));
    TEST_RunQueueStubSchedule();

    // 校验发送响应报文是否符合预期
    uint8_t hangUpRsp[] = {0x14, 0x03, 0x03, 0xFF};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(hangUpRsp, sizeof(hangUpRsp)));

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    SDF_MemFree(callStatus.callId);
    SDF_MemFree(callStatus.networkId);
    SDF_MemFree(callStatus.callStatus);
    SDF_MemFree(callStatus.callFlag);
}

// 此用例覆盖收到通话控制请求，用户返回失败
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_015)
{
    // 添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    basicInfo.callControlPoint.answer = &Stub_AnswerFailCallback;
    basicInfo.callControlPoint.hangUp = &Stub_HangUpFailCallback;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;

    // 更新通话状态为来电
    NLSTK_CcpCallStatues_S callStatus = {0};
    callStatus.callCount = 1;
    callStatus.callId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.networkId = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.callStatus = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    callStatus.callFlag = (uint8_t *)SDF_MemZalloc(sizeof(uint8_t));
    *callStatus.callId = 0x80;
    *callStatus.networkId = 0x00;
    *callStatus.callStatus = 0x01; // 来电
    *callStatus.callFlag = 0x00;
    ret = NLSTK_CcpUpdateCcsProperty(basicInfo.mediaInstanceId, NLSTK_CCP_CCS_CALL_STATUS, &callStatus);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);

    // 模拟接收到接听请求
    uint8_t answerReq[] = {0x13, 0x03, 0x00, 0x00, 0x02, 0x80};
    (void)memcpy_s(answerReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, answerReq, sizeof(answerReq));
    TEST_RunQueueStubSchedule();
    // 校验通话请求回调是否上报
    EXPECT_EQ(g_recvCbk, true);
    Stub_CcpResetData();

    // 校验发送响应报文是否符合预期
    uint8_t answerRsp[] = {0x14, 0x03, 0x02, 0xFF};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(answerRsp, sizeof(answerRsp)));

    // 模拟接收到挂断请求
    uint8_t hangUpReq[] = {0x13, 0x03, 0x00, 0x00, 0x03, 0x80};
    (void)memcpy_s(hangUpReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, hangUpReq, sizeof(hangUpReq));
    TEST_RunQueueStubSchedule();
    // 校验通话请求回调是否上报
    EXPECT_EQ(g_recvCbk, true);
    Stub_CcpResetData();

    // 校验发送响应报文是否符合预期
    uint8_t hangUpRsp[] = {0x14, 0x03, 0x03, 0xFF};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(hangUpRsp, sizeof(hangUpRsp)));

    // 删除通用通话控制实例
    ret = NLSTK_CcpDeleteCcsInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    SDF_MemFree(callStatus.callId);
    SDF_MemFree(callStatus.networkId);
    SDF_MemFree(callStatus.callStatus);
    SDF_MemFree(callStatus.callFlag);
}

// 此用例覆盖内部接口调用异常，以及重复添加场景
TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_016)
{
    // 覆盖添加NLSTK_CCP_CALL_CONTROL_SERVICE类型异常
    NLSTK_CcpCallControlInfo_S param = {};
    memset_s(&param, sizeof(param), 0, sizeof(param));
    param.type = NLSTK_CCP_CALL_CONTROL_SERVICE;
    param.mediaInstanceId = 0xFD;
    param.startCcsInst = Stub_StartCcsInstCbk;
    CcpCreateCcsInstance(&param);
    EXPECT_EQ(g_instanceId, param.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_FAIL);
    Stub_CcpResetData();

    // 正常添加通用通话控制实例
    NLSTK_CcpCallControlInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_CCP_CALL_CONTROL_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    basicInfo.startCcsInst = Stub_StartCcsInstCbk;
    uint32_t ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);
    Stub_CcpResetData();

    // 重复添加通用通话控制实例
    basicInfo.mediaInstanceId = 0xFE; // mediaInstanceId的有效值为0x01~0xFF
    ret = NLSTK_CcpCreateCcsInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_FAIL);
    Stub_CcpResetData();
}

TEST_F(CCP_CCS_TEST, CCP_CCS_TEST_021)
{
    uint16_t uuid = 0;
    NLSTK_CcpCcsPropertyType_E type = CcpGetPropertyTypeByUuid(uuid);
    EXPECT_EQ(type, NLSTK_CCP_CCS_MAX_PROPERTY);

    CcpFreeUpdateCcsPropertyParam(NULL);
    NLSTK_VariableData_S *value = CcpCcsPropertyValueConvert(NULL, NLSTK_CCP_CCS_INSTANCE_NAME);
    EXPECT_EQ(value == NULL, 1);
    void *data = SDF_MemZalloc(1);
    value = CcpCcsPropertyValueConvert(data, NLSTK_CCP_CCS_MAX_PROPERTY);
    EXPECT_EQ(value == NULL, 1);
    SDF_MemFree(data);

    CcpCreateCcsInstance(NULL);
    CcpCallControlResult(NULL);
    CcpCcsAuthorizeResult(NULL);
    CcpUpdateCcsProperty(NULL);
    CcpDeleteCcsInstance(NULL);

    uint32_t ret = NLSTK_CcpCreateCcsInstance(NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_POINTER_NULL);
    TEST_RunQueueStubSchedule();
    ret = NLSTK_CcpUpdateCcsProperty(0xFF, NLSTK_CCP_CCS_INSTANCE_NAME, NULL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_POINTER_NULL);
    TEST_RunQueueStubSchedule();
}