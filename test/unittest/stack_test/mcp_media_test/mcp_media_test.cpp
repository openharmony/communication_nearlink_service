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

#include "mcp_type.h"
#include "mcp_utils.h"
#include "mcp_media_server.h"
#include "nlstk_mcp_media.h"
#include "nlstk_mcp_media_server.h"

#include "cpfwk_log.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

class MCP_MEDIA_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;

    static void Stub_McpResetData(void);
    static uint32_t AddMediaService(void);

    static void Stub_StartMediaInstCbk(int32_t instanceId, NLSTK_Errcode_E ret);
    static void Stub_MediaAuthorizeCbk(uint16_t requestId, int32_t instanceId, NLSTK_McpPropertyType_E property,
                                       NLSTK_ServicePropertyOpType_E operation);
    static void Stub_PlayCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
    static void Stub_PauseCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
    static void Stub_StopCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
    static void Stub_FastForwardCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
    static void Stub_PreviousMediaCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);
    static void Stub_NextMediaCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId);

    static int32_t g_instanceId;
    static uint32_t g_addResult;
    static SLE_Addr_S g_recvAddr;
    static bool g_recvCbk;
    static uint16_t g_requestId;
    static NLSTK_McpPropertyType_E g_type;
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
        
        // 步骤3：使能MCP模块Media子模块
        McpMediaEnable();

        Stub_McpResetData();
    }

    void TearDown() override
    {
        // 步骤4：去使能MCP模块Media子模块
        McpMediaDisable();

        // 步骤5：去初始化SSAP模块
        SSAP_DeInit();

        // 步骤6：清理桩函数
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

int32_t MCP_MEDIA_TEST::g_instanceId = -1;
uint32_t MCP_MEDIA_TEST::g_addResult = NLSTK_ERRCODE_FAIL;
SLE_Addr_S MCP_MEDIA_TEST::g_recvAddr = {0};
bool MCP_MEDIA_TEST::g_recvCbk = false;
uint16_t MCP_MEDIA_TEST::g_requestId = 0;
NLSTK_McpPropertyType_E MCP_MEDIA_TEST::g_type = NLSTK_MCP_MEDIA_MAX_PROPERTY;

void MCP_MEDIA_TEST::Stub_McpResetData(void)
{
    g_instanceId = -1;
    g_addResult = NLSTK_ERRCODE_FAIL;
    g_recvAddr = {0};
    g_recvCbk = false;
    g_requestId = 0;
    g_type = NLSTK_MCP_MEDIA_MAX_PROPERTY;
}

void MCP_MEDIA_TEST::Stub_StartMediaInstCbk(int32_t instanceId, NLSTK_Errcode_E ret)
{
    g_instanceId = instanceId;
    g_addResult = ret;
}

void MCP_MEDIA_TEST::Stub_MediaAuthorizeCbk(uint16_t requestId, int32_t instanceId, NLSTK_McpPropertyType_E property,
                                               NLSTK_ServicePropertyOpType_E operation)
{
    g_requestId = requestId;
    g_instanceId = instanceId;
    g_type = property;
}

void MCP_MEDIA_TEST::Stub_PlayCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_McpPlayControlResult(requestId, instanceId, 0);
    uint8_t state = NLSTK_MCP_STATE_PLAYING;
    NLSTK_McpUpdateMediaProperty(instanceId, NLSTK_MCP_PLAYBACK_STATE, &state);

}

void MCP_MEDIA_TEST::Stub_PauseCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_McpPlayControlResult(requestId, instanceId, 0);
    uint8_t state = NLSTK_MCP_STATE_READY;
    NLSTK_McpUpdateMediaProperty(instanceId, NLSTK_MCP_PLAYBACK_STATE, &state);
}

void MCP_MEDIA_TEST::Stub_StopCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_McpPlayControlResult(requestId, instanceId, 0);
    uint8_t state = NLSTK_MCP_STATE_READY;
    NLSTK_McpUpdateMediaProperty(instanceId, NLSTK_MCP_PLAYBACK_STATE, &state);
}

void MCP_MEDIA_TEST::Stub_FastForwardCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_McpPlayControlResult(requestId, instanceId, 0);
    uint8_t state = NLSTK_MCP_STATE_SEEKING;
    NLSTK_McpUpdateMediaProperty(instanceId, NLSTK_MCP_PLAYBACK_STATE, &state);
}

void MCP_MEDIA_TEST::Stub_PreviousMediaCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_McpPlayControlResult(requestId, instanceId, 0);
}

void MCP_MEDIA_TEST::Stub_NextMediaCallback(SLE_Addr_S *addr, uint16_t requestId, int32_t instanceId)
{
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_recvCbk = true;
    NLSTK_McpPlayControlResult(requestId, instanceId, 0);
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

uint32_t MCP_MEDIA_TEST::AddMediaService()
{
    NLSTK_McpMediaInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_MCP_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF;
    char instanceName[] = "NearlinkAudio";
    basicInfo.instanceName.data = reinterpret_cast<uint8_t*>(instanceName);
    basicInfo.instanceName.len = strlen(instanceName);
    basicInfo.mediaBaseInfo.mediaType = NLSTK_MEDIA_UNSPECIFIED;
    basicInfo.playbackState = NLSTK_MCP_STATE_UNINITIALIZED;
    basicInfo.featuresSupported.FeatureType = NLSTK_MCP_FEATURE_TYPE_PLAY_CTL | NLSTK_MCP_FEATURE_TYPE_PLAY_MODE;
    basicInfo.featuresSupported.playMode = NLSTK_MCP_SINGLE_PLAY | NLSTK_MCP_SINGLE_LOOP | NLSTK_MCP_SINGLE_LIST
                        | NLSTK_MCP_LOOP_LIST | NLSTK_MCP_RANDOM_PLAY;
    basicInfo.featuresSupported.playCtl = NLSTK_MCP_PLAY | NLSTK_MCP_STOP | NLSTK_MCP_PAUSE | NLSTK_MCP_FAST_FORWARD
                        | NLSTK_MCP_PREVIOUS_MEDIA | NLSTK_MCP_NEXT_MEDIA;
    basicInfo.startMediaInst = Stub_StartMediaInstCbk;
    basicInfo.playbackControlPoint.play = Stub_PlayCallback;
    basicInfo.playbackControlPoint.pause = Stub_PauseCallback;
    basicInfo.playbackControlPoint.stop = Stub_StopCallback;
    basicInfo.playbackControlPoint.fastForward = Stub_FastForwardCallback;
    basicInfo.playbackControlPoint.previousMedia = Stub_PreviousMediaCallback;
    basicInfo.playbackControlPoint.nextMedia = Stub_NextMediaCallback;
    return NLSTK_McpCreateMediaInstance(&basicInfo);
}

/**
 * @test MCP_MEDIA_TEST_001
 * @brief 此测试用例用于验证媒体播放控制服务端添加通用媒体播放控制实例，此场景为正常添加。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 5. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_001)
{
    // 添加通用媒体播放控制实例
    NLSTK_McpMediaInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_MCP_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    basicInfo.startMediaInst = Stub_StartMediaInstCbk;
    uint32_t ret = NLSTK_McpCreateMediaInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);
    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test MCP_MEDIA_TEST_002
 * @brief 此测试用例用于验证媒体播放控制服务端添加通用媒体播放控制实例，此场景为异常添加。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加媒体播放控制实例；
 * 2. 校验返回结果不为成功；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：调用函数返回结果不为成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_002)
{
    // 异常1：入参为空
    uint32_t ret = NLSTK_McpCreateMediaInstance(NULL);
    EXPECT_NE(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 异常2：类型为媒体播放控制实例
    NLSTK_McpMediaInfo_S basicInfo1 = {};
    memset_s(&basicInfo1, sizeof(basicInfo1), 0, sizeof(basicInfo1));
    basicInfo1.type = NLSTK_MCP_SERVICE;
    basicInfo1.mediaInstanceId = 0xFF;
    basicInfo1.startMediaInst = Stub_StartMediaInstCbk;
    ret = NLSTK_McpCreateMediaInstance(&basicInfo1);
    EXPECT_NE(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 异常3：媒体实例标识为无效值
    NLSTK_McpMediaInfo_S basicInfo2 = {};
    memset_s(&basicInfo2, sizeof(basicInfo2), 0, sizeof(basicInfo2));
    basicInfo2.type = NLSTK_MCP_COMMON_SERVICE;
    basicInfo2.mediaInstanceId = 0;
    basicInfo2.startMediaInst = Stub_StartMediaInstCbk;
    ret = NLSTK_McpCreateMediaInstance(&basicInfo2);
    EXPECT_NE(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
}

/**
 * @test MCP_MEDIA_TEST_003
 * @brief 此测试用例用于验证媒体播放控制服务端添加通用媒体播放控制实例，此场景为正常添加，必选属性正常赋值，播放状态需要授权。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 校验通用媒体播放控制服务中属性数量是否为必选属性数；
 * 5. 模拟收到读取播放状态请求，验证授权功能；
 * 6. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 7. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_003)
{
    CP_LOG_INFO("[TEST] enter MCP_MEDIA_TEST_003");
    // 添加通用媒体播放控制实例
    NLSTK_McpMediaInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_MCP_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    char instanceName[] = "NearlinkAudio";
    basicInfo.instanceName.data = reinterpret_cast<uint8_t*>(instanceName);
    basicInfo.instanceName.len = strlen(instanceName);
    basicInfo.mediaBaseInfo.mediaType = NLSTK_MEDIA_UNSPECIFIED;
    basicInfo.playbackState = NLSTK_MCP_STATE_UNINITIALIZED;
    basicInfo.featuresSupported.FeatureType = NLSTK_MCP_FEATURE_TYPE_PLAY_CTL | NLSTK_MCP_FEATURE_TYPE_PLAY_MODE;
    basicInfo.featuresSupported.playMode = NLSTK_MCP_SINGLE_PLAY | NLSTK_MCP_SINGLE_LOOP | NLSTK_MCP_SINGLE_LIST
                        | NLSTK_MCP_LOOP_LIST | NLSTK_MCP_RANDOM_PLAY;
    basicInfo.featuresSupported.playCtl = 0;
    basicInfo.propertyRights[NLSTK_MCP_PLAYBACK_STATE] = NLSTK_MCP_READ_AUTHOR;
    basicInfo.startMediaInst = Stub_StartMediaInstCbk;
    basicInfo.authorize = Stub_MediaAuthorizeCbk;
    uint32_t ret = NLSTK_McpCreateMediaInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 6);
    NLSTK_SsapUuid_S uuidStru = McpConvertUuidToStru((uint16_t)MCP_PLAYBACK_STATE_UUID);
    size_t index = 0;
    SDF_VectorFindFirst(service->properties, CompUuidFunc, &uuidStru, &index);
    SSAP_Property_S *property = (SSAP_Property_S *)SDF_VectorElementAt(service->properties, index);
    uint16_t handle = property->handle;
    size_t handleOffset = 0x02;

    // 模拟收到读取媒体播放状态请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    TEST_RunQueueStubSchedule();
    uint8_t readReq[] = {0x08, 0x03, 0x00, 0x00, 0x00};
    (void)memcpy_s(readReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    CP_LOG_INFO("[TEST] MCP_MEDIA_TEST_003 recv readReq: 0x08, 0x03, 0x%u, 0x%u, 0x00", readReq + 2, readReq + 3);
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, readReq, sizeof(readReq));
    TEST_RunQueueStubSchedule();

    // 校验授权回调是否正常上报
    EXPECT_EQ(g_type, NLSTK_MCP_PLAYBACK_STATE);
    (void)g_addr;

    ret = NLSTK_McpMediaAuthorizeResult(g_requestId, g_instanceId, g_type, NLSTK_ERRCODE_SUCCESS);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 校验发送响应报文是否符合预期
    uint8_t readRsp[] = {0x09, 0x03, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(readRsp, sizeof(readRsp)));
    Stub_McpResetData();

    // 模拟再次收到读取媒体播放状态请求
    (void)memcpy_s(readReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    CP_LOG_INFO("[TEST] MCP_MEDIA_TEST_003 recv readReq: 0x08, 0x03, 0x%u, 0x%u, 0x00", readReq + 2, readReq + 3);
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, readReq, sizeof(readReq));
    TEST_RunQueueStubSchedule();

    // 校验授权回调是否正常上报
    EXPECT_EQ(g_type, NLSTK_MCP_PLAYBACK_STATE);

    ret = NLSTK_McpMediaAuthorizeResult(g_requestId, g_instanceId, g_type, NLSTK_ERRCODE_FAIL);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 校验发送响应报文是否符合预期
    uint8_t errRsp[] = {0x09, 0x0B, 0x09, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(errRsp, sizeof(errRsp)));

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test MCP_MEDIA_TEST_004
 * @brief 此测试用例用于验证媒体播放控制服务端添加通用媒体播放控制实例，此场景为正常添加，并勾选所有可选属性。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 校验通用媒体播放控制服务中属性数量是否为所有属性数；
 * 5. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 6. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_004)
{
    // 添加通用媒体播放控制实例
    NLSTK_McpMediaInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_MCP_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    basicInfo.optionalItem.flags.flagByte = 0xFF; // 可选属性均勾选
    basicInfo.startMediaInst = Stub_StartMediaInstCbk;
    uint32_t ret = NLSTK_McpCreateMediaInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 13);

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test MCP_MEDIA_TEST_005
 * @brief 此测试用例用于验证媒体播放控制正常业务交互，首先添加媒体实例，然后将播放状态更新为就绪，之后模拟接收客户端播放请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 校验通用媒体播放控制服务中属性数量是否为必选属性数；
 * 5. 调用NLSTK_McpUpdateMediaProperty更新播放状态为就绪；
 * 6. 使用TEST_DTAP_RecData模拟接收到播放方法调用请求；
 * 7. 校验Stub_PlayCallback回调是否正常上报；
 * 8. Stub_PlayCallback中调用NLSTK_McpPlayControlResult返回方法调用结果并更新播放状态；
 * 9. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期；
 * 10. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 11. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_005)
{
    // 添加通用媒体播放控制实例
    uint32_t ret = AddMediaService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, 0xFF);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 6);

    // 更新播放状态为就绪
    uint8_t state = NLSTK_MCP_STATE_READY;
    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_PLAYBACK_STATE, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到播放请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    TEST_RunQueueStubSchedule();
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t playReq[] = {0x13, 0x03, 0x00, 0x00, 0x01};
    (void)memcpy_s(playReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, playReq, sizeof(playReq));
    TEST_RunQueueStubSchedule();

    // 校验播放请求回调是否正常上报
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);

    // 校验发送响应报文是否符合预期
    uint8_t playRsp[] = {0x14, 0x03, 0x01, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(playRsp, sizeof(playRsp)));

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(0xFF);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test MCP_MEDIA_TEST_006
 * @brief 此测试用例用于验证媒体播放控制正常业务交互，首先添加媒体实例，然后将播放状态更新为播放，之后模拟接收客户端停止请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 校验通用媒体播放控制服务中属性数量是否为必选属性数；
 * 5. 调用NLSTK_McpUpdateMediaProperty更新播放状态为播放；
 * 6. 使用TEST_DTAP_RecData模拟接收到停止方法调用请求；
 * 7. 校验Stub_PauseCallback回调是否正常上报；
 * 8. Stub_PauseCallback中调用NLSTK_McpPlayControlResult返回方法调用结果并更新播放状态；
 * 9. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期；
 * 10. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 11. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_006)
{
    // 添加通用媒体播放控制实例
    uint32_t ret = AddMediaService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, 0xFF);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 6);

    // 更新播放状态为播放
    uint8_t state = NLSTK_MCP_STATE_PLAYING;
    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_PLAYBACK_STATE, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到停止请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    TEST_RunQueueStubSchedule();
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t playReq[] = {0x13, 0x03, 0x00, 0x00, 0x02};
    (void)memcpy_s(playReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, playReq, sizeof(playReq));
    TEST_RunQueueStubSchedule();

    // 校验播放请求回调是否正常上报
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);

    // 校验发送响应报文是否符合预期
    uint8_t playRsp[] = {0x14, 0x03, 0x02, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(playRsp, sizeof(playRsp)));

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(0xFF);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test MCP_MEDIA_TEST_007
 * @brief 此测试用例用于验证媒体播放控制正常业务交互，首先添加媒体实例，然后将播放状态更新为播放，之后模拟接收客户端暂停请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 校验通用媒体播放控制服务中属性数量是否为必选属性数；
 * 5. 调用NLSTK_McpUpdateMediaProperty更新播放状态为播放；
 * 6. 使用TEST_DTAP_RecData模拟接收到暂停方法调用请求；
 * 7. 校验Stub_StopCallback回调是否正常上报；
 * 8. Stub_StopCallback中调用NLSTK_McpPlayControlResult返回方法调用结果并更新播放状态；
 * 9. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期；
 * 10. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 11. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_007)
{
    // 添加通用媒体播放控制实例
    uint32_t ret = AddMediaService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, 0xFF);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 6);

    // 更新播放状态为播放
    uint8_t state = NLSTK_MCP_STATE_PLAYING;
    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_PLAYBACK_STATE, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到暂停请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    TEST_RunQueueStubSchedule();
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t playReq[] = {0x13, 0x03, 0x00, 0x00, 0x03};
    (void)memcpy_s(playReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, playReq, sizeof(playReq));
    TEST_RunQueueStubSchedule();

    // 校验播放请求回调是否正常上报
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);

    // 校验发送响应报文是否符合预期
    uint8_t playRsp[] = {0x14, 0x03, 0x03, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(playRsp, sizeof(playRsp)));

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(0xFF);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test MCP_MEDIA_TEST_008
 * @brief 此测试用例用于验证媒体播放控制正常业务交互，首先添加媒体实例，然后将播放状态更新为播放，之后模拟接收客户端快进请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 校验通用媒体播放控制服务中属性数量是否为必选属性数；
 * 5. 调用NLSTK_McpUpdateMediaProperty更新播放状态为播放；
 * 6. 使用TEST_DTAP_RecData模拟接收到快进方法调用请求；
 * 7. 校验Stub_FastForwardCallback回调是否正常上报；
 * 8. Stub_FastForwardCallback中调用NLSTK_McpPlayControlResult返回方法调用结果并更新播放状态；
 * 9. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期；
 * 10. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 11. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_008)
{
    // 添加通用媒体播放控制实例
    uint32_t ret = AddMediaService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, 0xFF);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 6);

    // 更新播放状态为播放
    uint8_t state = NLSTK_MCP_STATE_PLAYING;
    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_PLAYBACK_STATE, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到暂停请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    TEST_RunQueueStubSchedule();
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t playReq[] = {0x13, 0x03, 0x00, 0x00, 0x04};
    (void)memcpy_s(playReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, playReq, sizeof(playReq));
    TEST_RunQueueStubSchedule();

    // 校验播放请求回调是否正常上报
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);

    // 校验发送响应报文是否符合预期
    uint8_t playRsp[] = {0x14, 0x03, 0x04, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(playRsp, sizeof(playRsp)));

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(0xFF);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test MCP_MEDIA_TEST_009
 * @brief 此测试用例用于验证媒体播放控制正常业务交互，首先添加媒体实例，然后将播放状态更新为播放，之后模拟接收客户端上一个媒体请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 校验通用媒体播放控制服务中属性数量是否为必选属性数；
 * 5. 调用NLSTK_McpUpdateMediaProperty更新播放状态为播放；
 * 6. 使用TEST_DTAP_RecData模拟接收到上一个媒体方法调用请求；
 * 7. 校验Stub_PreviousMediaCallback回调是否正常上报；
 * 8. Stub_PreviousMediaCallback中调用NLSTK_McpPlayControlResult返回方法调用结果并更新播放状态；
 * 9. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期；
 * 10. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 11. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_009)
{
    // 添加通用媒体播放控制实例
    uint32_t ret = AddMediaService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, 0xFF);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 6);

    // 更新播放状态为播放
    uint8_t state = NLSTK_MCP_STATE_PLAYING;
    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_PLAYBACK_STATE, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到暂停请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    TEST_RunQueueStubSchedule();
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t playReq[] = {0x13, 0x03, 0x00, 0x00, 0x30};
    (void)memcpy_s(playReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, playReq, sizeof(playReq));
    TEST_RunQueueStubSchedule();

    // 校验播放请求回调是否正常上报
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);

    // 校验发送响应报文是否符合预期
    uint8_t playRsp[] = {0x14, 0x03, 0x30, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(playRsp, sizeof(playRsp)));

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(0xFF);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test MCP_MEDIA_TEST_009
 * @brief 此测试用例用于验证媒体播放控制正常业务交互，首先添加媒体实例，然后将播放状态更新为播放，之后模拟接收客户端下一个媒体请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 校验通用媒体播放控制服务中属性数量是否为必选属性数；
 * 5. 调用NLSTK_McpUpdateMediaProperty更新播放状态为播放；
 * 6. 使用TEST_DTAP_RecData模拟接收到下一个媒体方法调用请求；
 * 7. 校验Stub_NextMediaCallback回调是否正常上报；
 * 8. Stub_NextMediaCallback中调用NLSTK_McpPlayControlResult返回方法调用结果并更新播放状态；
 * 9. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期；
 * 10. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 11. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_010)
{
    // 添加通用媒体播放控制实例
    uint32_t ret = AddMediaService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, 0xFF);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 6);

    // 更新播放状态为播放
    uint8_t state = NLSTK_MCP_STATE_PLAYING;
    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_PLAYBACK_STATE, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到暂停请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    TEST_RunQueueStubSchedule();
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t playReq[] = {0x13, 0x03, 0x00, 0x00, 0x31};
    (void)memcpy_s(playReq + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, playReq, sizeof(playReq));
    TEST_RunQueueStubSchedule();

    // 校验播放请求回调是否正常上报
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);

    // 校验发送响应报文是否符合预期
    uint8_t playRsp[] = {0x14, 0x03, 0x31, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(playRsp, sizeof(playRsp)));

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(0xFF);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test MCP_MEDIA_TEST_011
 * @brief 此测试用例用于验证媒体播放控制异常业务交互，首先添加媒体实例，之后模拟接收客户端播放请求，然后将播放状态更新为播放，之后模拟接收客户端快退请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 校验通用媒体播放控制服务中属性数量是否为必选属性数；
 * 5. 使用TEST_DTAP_RecData模拟接收到播放方法调用请求；
 * 6. 校验Stub_PlayCallback回调是否上报，预期不上报；
 * 7. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期，预期回复媒体播放实例未就绪错误码0x03；
 * 8. 调用NLSTK_McpUpdateMediaProperty更新播放状态为播放；
 * 9. 使用TEST_DTAP_RecData模拟接收到快退方法调用请求；
 * 10. 使用TEST_DTAP_CompareLastPkt校验发送报文是否符合预期，预期回复请求类型不支持错误码0x01；
 * 11. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 12. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_011)
{
    // 添加通用媒体播放控制实例
    uint32_t ret = AddMediaService();
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, 0xFF);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 6);

    // 模拟接收到播放请求
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    TEST_RunQueueStubSchedule();
    SSAP_Method_S *method = (SSAP_Method_S *)SDF_VectorElementAt(service->methods, 0);
    ASSERT_TRUE(method != NULL);
    uint16_t handle = method->handle;
    size_t handleOffset = 0x02;
    uint8_t req1[] = {0x13, 0x03, 0x00, 0x00, 0x01};
    (void)memcpy_s(req1 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req1, sizeof(req1));
    TEST_RunQueueStubSchedule();

    // 校验播放请求回调是否上报，预期不上报
    EXPECT_EQ(g_recvCbk, false);

    // 校验发送响应报文是否符合预期，预期回复媒体播放实例未就绪错误码0x03
    uint8_t rsp1[] = {0x14, 0x03, 0x01, 0x03};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp1, sizeof(rsp1)));

    g_recvCbk = false;

    // 更新播放状态为播放
    uint8_t state = NLSTK_MCP_STATE_PLAYING;
    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_PLAYBACK_STATE, &state);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟接收到快退请求
    uint8_t req2[] = {0x13, 0x03, 0x00, 0x00, 0x05};
    (void)memcpy_s(req2 + handleOffset, sizeof(handle), &handle, sizeof(handle));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, req2, sizeof(req2));
    TEST_RunQueueStubSchedule();

    // 校验发送响应报文是否符合预期，请求类型不支持错误码0x01
    uint8_t rsp2[] = {0x14, 0x03, 0x05, 0x01};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(rsp2, sizeof(rsp2)));

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(0xFF);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test MCP_MEDIA_TEST_012
 * @brief 此测试用例用于验证媒体播放控制服务端添加通用媒体播放控制实例，此场景为正常添加，并勾选所有可选属性，可选属性正常赋值。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 校验通用媒体播放控制服务中属性数量是否为所有属性数；
 * 5. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 6. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_012)
{
    // 添加通用媒体播放控制实例
    NLSTK_McpMediaInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_MCP_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    char mediaBaseInfo[] = "mediaBaseInfo";
    basicInfo.mediaBaseInfo.mediaName = reinterpret_cast<uint8_t*>(mediaBaseInfo);
    basicInfo.mediaBaseInfo.mediaNameLen = strlen(mediaBaseInfo);
    char instanceIcon[] = "NearlinkIcon";
    basicInfo.optionalItem.instanceIcon.iconValue = reinterpret_cast<uint8_t*>(instanceIcon);
    basicInfo.optionalItem.instanceIcon.iconLen = strlen(instanceIcon);
    char mediaExtendedInfo[] = "mediaExtendedInfo";
    basicInfo.optionalItem.mediaExtendedInfo.data = reinterpret_cast<uint8_t*>(mediaExtendedInfo);
    basicInfo.optionalItem.mediaExtendedInfo.len = strlen(mediaExtendedInfo);
    char segmentInfo[] = "segmentInfo";
    basicInfo.optionalItem.segmentInfo.data = reinterpret_cast<uint8_t*>(segmentInfo);
    basicInfo.optionalItem.segmentInfo.len = strlen(segmentInfo);
    basicInfo.optionalItem.flags.flagByte = 0xFF; // 可选属性均勾选
    basicInfo.startMediaInst = Stub_StartMediaInstCbk;
    uint32_t ret = NLSTK_McpCreateMediaInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 13);

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(basicInfo.mediaInstanceId);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}

/**
 * @test MCP_MEDIA_TEST_013
 * @brief 此测试用例用于验证媒体播放控制服务端添加通用媒体播放控制实例，此场景为正常添加，并勾选所有可选属性，依次更新各属性。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpCreateMediaInstance添加通用媒体播放控制实例；
 * 2. 校验回调函数触发对应的实例标识添加成功；
 * 3. 校验SSAP服务中是否有通用媒体播放控制服务；
 * 4. 校验通用媒体播放控制服务中属性数量是否为所有属性数；
 * 5. 依次更新各属性值
 * 6. 调用NLSTK_McpDeleteMediaInstance删除实例；
 * 7. 校验SSAP服务数量是否为0；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Media子模块已使能。
 *
 * @post
 * - 预期结果：回调函数触发，返回对应的实例标识以及结果码成功。
 */
TEST_F(MCP_MEDIA_TEST, MCP_MEDIA_TEST_013)
{
    // 添加通用媒体播放控制实例
    NLSTK_McpMediaInfo_S basicInfo = {};
    memset_s(&basicInfo, sizeof(basicInfo), 0, sizeof(basicInfo));
    basicInfo.type = NLSTK_MCP_COMMON_SERVICE;
    basicInfo.mediaInstanceId = 0xFF; // mediaInstanceId的有效值为0x01~0xFF
    basicInfo.mediaBaseInfo.mediaType = NLSTK_MEDIA_UNSPECIFIED;
    basicInfo.optionalItem.flags.flagByte = 0xFF; // 可选属性均勾选
    basicInfo.startMediaInst = Stub_StartMediaInstCbk;
    uint32_t ret = NLSTK_McpCreateMediaInstance(&basicInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    // 校验回调上来的结果
    EXPECT_EQ(g_instanceId, basicInfo.mediaInstanceId);
    EXPECT_EQ(g_addResult, NLSTK_ERRCODE_SUCCESS);

    SDF_Vector_S *services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 1);
    SSAP_Service_S *service = (SSAP_Service_S *)SDF_VectorElementAt(services, 0);
    uint16_t uuid = McpConvertUuidTo16Bits(service->uuid);
    EXPECT_EQ(uuid, (uint16_t)MCP_COMMON_MEDIA_SERVICE_UUID);
    EXPECT_EQ(service->properties->size, 13);

    basicInfo.mediaBaseInfo.mediaType = NLSTK_MEDIA_UNSPECIFIED;
    basicInfo.featuresSupported.FeatureType = NLSTK_MCP_FEATURE_TYPE_PLAY_CTL | NLSTK_MCP_FEATURE_TYPE_PLAY_MODE;
    basicInfo.featuresSupported.playMode = NLSTK_MCP_SINGLE_PLAY | NLSTK_MCP_SINGLE_LOOP | NLSTK_MCP_SINGLE_LIST
                        | NLSTK_MCP_LOOP_LIST | NLSTK_MCP_RANDOM_PLAY;
    basicInfo.featuresSupported.playCtl = NLSTK_MCP_PLAY | NLSTK_MCP_STOP | NLSTK_MCP_PAUSE | NLSTK_MCP_FAST_FORWARD
                        | NLSTK_MCP_PREVIOUS_MEDIA | NLSTK_MCP_NEXT_MEDIA;
    basicInfo.playbackLocation = 0x10;
    basicInfo.optionalItem.playbackSpeed = 0x01;
    char mediaBaseInfo[] = "mediaBaseInfo";
    basicInfo.mediaBaseInfo.mediaName = reinterpret_cast<uint8_t*>(mediaBaseInfo);
    basicInfo.mediaBaseInfo.mediaNameLen = strlen(mediaBaseInfo);
    char instanceIcon[] = "NearlinkIcon";
    basicInfo.optionalItem.instanceIcon.iconValue = reinterpret_cast<uint8_t*>(instanceIcon);
    basicInfo.optionalItem.instanceIcon.iconLen = strlen(instanceIcon);
    char mediaExtendedInfo[] = "mediaExtendedInfo";
    basicInfo.optionalItem.mediaExtendedInfo.data = reinterpret_cast<uint8_t*>(mediaExtendedInfo);
    basicInfo.optionalItem.mediaExtendedInfo.len = strlen(mediaExtendedInfo);

    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_MEDIA_BASIC_INFO, &basicInfo.mediaBaseInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_MEDIA_EXTENDED_INFO, &basicInfo.optionalItem.mediaExtendedInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_MEDIA_INSTANCE_ICON, &basicInfo.optionalItem.instanceIcon);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_MEDIA_IDENTIFIER_INFO, &basicInfo.optionalItem.mediaIdInfo);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_MEDIA_PLAYBACK_POSITION, &basicInfo.playbackLocation);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_PLAYBACK_SPEED, &basicInfo.optionalItem.playbackSpeed);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    ret = NLSTK_McpUpdateMediaProperty(0xFF, NLSTK_MCP_FEATURE_SUPPORT, &basicInfo.featuresSupported);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 删除通用媒体播放控制实例
    ret = NLSTK_McpDeleteMediaInstance(0xFF);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    services = SSAPS_GetServices();
    EXPECT_EQ(services->size, 0);
}
