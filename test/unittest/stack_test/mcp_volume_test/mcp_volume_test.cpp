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
#include "nlstk_mcp_volume.h"
#include "nlstk_mcp_volume_client.h"

#include "cpfwk_log.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

static uint8_t g_serviceUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x06, 0x16};
static uint8_t g_volumeUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x73};
static uint8_t g_streamVolumeUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x7B};
static uint8_t g_controlUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x75};
static uint8_t g_eventUuid[] = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x10, 0x76};

static void STUB_CacheVolumeServiceWithoutStream();
static void STUB_CacheVolumeServiceWithStream();

class MCP_VOLUME_TEST : public testing::Test {
public:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;

    static void Stub_ResetData();

    static void Stub_VolumeChangeEvent(SLE_Addr_S *addr, uint8_t volume);
    static void Stub_MuteStatusChangeEvent(SLE_Addr_S *addr, uint8_t muteStatus);
    static void Stub_NotifyVolumeChange(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E property, void *value);
    static void Stub_GetVolumeRsp(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E property,
                                  uint8_t errorCode, void *value);
    static void Stub_SetVolumeRsp(SLE_Addr_S *addr, uint8_t errorCode);
    static void Stub_StateChange(SLE_Addr_S *addr, uint8_t state, uint8_t preState);

    static SLE_Addr_S g_recvAddr;
    static bool g_recvCbk;
    static uint8_t g_state;
    static uint8_t g_volume;
    static uint8_t g_mVolume;
    static uint8_t g_cVolume;
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
        
        // 步骤3：使能MCP模块Volume子模块
        McpVolumeEnable();

        Stub_ResetData();
    }

    void TearDown() override
    {
        // 步骤4：去使能MCP模块Volume子模块
        McpVolumeDisable();

        // 步骤5：去初始化SSAP模块
        SSAP_DeInit();

        // 步骤6：清理桩函数
        TEST_StackScheduleDeInit();
        TEST_CM_DeInit();
    }
};

SLE_Addr_S MCP_VOLUME_TEST::g_recvAddr = {0};
bool MCP_VOLUME_TEST::g_recvCbk = false;
uint8_t MCP_VOLUME_TEST::g_state = 0xFF;
uint8_t MCP_VOLUME_TEST::g_volume = 0x00;
uint8_t MCP_VOLUME_TEST::g_mVolume = 0x00;
uint8_t MCP_VOLUME_TEST::g_cVolume = 0x00;

void MCP_VOLUME_TEST::Stub_VolumeChangeEvent(SLE_Addr_S *addr, uint8_t volume)
{
    g_recvCbk = true;
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
}

void MCP_VOLUME_TEST::Stub_MuteStatusChangeEvent(SLE_Addr_S *addr, uint8_t muteStatus)
{
    g_recvCbk = true;
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
}

void MCP_VOLUME_TEST::Stub_NotifyVolumeChange(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E property, void *value)
{
    g_recvCbk = true;
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
}

void MCP_VOLUME_TEST::Stub_GetVolumeRsp(SLE_Addr_S *addr, NLSTK_McpVolumePropertyType_E property,
                                uint8_t errorCode, void *value)
{
    g_recvCbk = true;
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (property == NLSTK_MCP_VOLUME_STATUS) {
        NLSTK_McpVolumeStatus_S *vol = (NLSTK_McpVolumeStatus_S *)value;
        g_volume = vol->volume;
    } else if (property == NLSTK_MCP_STREAM_VOLUME_STATUS) {
        NLSTK_McpStreamVolumeStatus_S *vol = (NLSTK_McpStreamVolumeStatus_S *)value;
        g_mVolume = vol->mediaVolume;
        g_cVolume = vol->callVolume;
    }
}

void MCP_VOLUME_TEST::Stub_SetVolumeRsp(SLE_Addr_S *addr, uint8_t errorCode)
{
    g_recvCbk = true;
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
}

void MCP_VOLUME_TEST::Stub_StateChange(SLE_Addr_S *addr, uint8_t state, uint8_t preState)
{
    g_recvCbk = true;
    g_state = state;
    (void)memcpy_s(&g_recvAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));

}

void MCP_VOLUME_TEST::Stub_ResetData()
{
    g_recvAddr = {0};
    g_recvCbk = false;
    g_state = 0xFF;
    g_volume = 0x00;
    g_mVolume = 0x00;
    g_cVolume = 0x00;
}

static void STUB_CacheVolumeServiceWithoutStream()
{
    SsapCacheServInfo_S serv = {0};
    serv.handle = 0x10;
    serv.endHandle = 0x14;
    serv.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serv.uuid, sizeof(NLSTK_SsapUuid_S), &g_serviceUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheServ(&g_addr, &serv);

    NLSTK_SsapPrty_S volume = {0};
    volume.handle = 0x11;
    memcpy_s(&volume.uuid, sizeof(NLSTK_SsapUuid_S), &g_volumeUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &volume);

    NLSTK_SsapPrty_S control = {0};
    control.handle = 0x12;
    memcpy_s(&control.uuid, sizeof(NLSTK_SsapUuid_S), &g_controlUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheMethod(&g_addr, &control);

    NLSTK_SsapPrty_S event = {0};
    event.handle = 0x13;
    memcpy_s(&event.uuid, sizeof(NLSTK_SsapUuid_S), &g_eventUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheEvent(&g_addr, &event);

    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_PROPERTY, 0x11);
    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_METHOD, 0x12);
    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_EVENT, 0x13);
    SsapcCacheServDiscFinish(&g_addr);
}

static void STUB_CacheVolumeServiceWithStream()
{
    SsapCacheServInfo_S serv = {0};
    serv.handle = 0x10;
    serv.endHandle = 0x14;
    serv.serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    memcpy_s(&serv.uuid, sizeof(NLSTK_SsapUuid_S), &g_serviceUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheServ(&g_addr, &serv);

    NLSTK_SsapPrty_S volume = {0};
    volume.handle = 0x11;
    memcpy_s(&volume.uuid, sizeof(NLSTK_SsapUuid_S), &g_volumeUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &volume);

    NLSTK_SsapPrty_S streamVolume = {0};
    streamVolume.handle = 0x12;
    memcpy_s(&streamVolume.uuid, sizeof(NLSTK_SsapUuid_S), &g_streamVolumeUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCachePrty(&g_addr, &streamVolume);

    NLSTK_SsapPrty_S control = {0};
    control.handle = 0x13;
    memcpy_s(&control.uuid, sizeof(NLSTK_SsapUuid_S), &g_controlUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheMethod(&g_addr, &control);

    NLSTK_SsapPrty_S event = {0};
    event.handle = 0x14;
    memcpy_s(&event.uuid, sizeof(NLSTK_SsapUuid_S), &g_eventUuid, sizeof(NLSTK_SsapUuid_S));
    SsapcCacheEvent(&g_addr, &event);

    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_PROPERTY, 0x12);
    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_METHOD, 0x13);
    SsapcCacheServMemberDiscFinish(&g_addr, FIND_STRUCTURE_TYPE_EVENT, 0x14);
    SsapcCacheServDiscFinish(&g_addr);
}

/**
 * @test MCP_VOLUME_TEST_001
 * @brief 此测试用例用于验证音量控制Profile连接流程，对端无音频流音量属性，并发送设置主音量控制请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpRegVolumeClientCbk注册回调；
 * 2. 调用NLSTK_McpVolumeConnect开启连接；
 * 3. 模拟链路建立成功；
 * 4. 模拟收到连接流程中的SSAP响应报文；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 发送设置主音量控制请求；
 * 7. 模拟收到主音量控制响应；
 * 8. 校验是否收到主音量控制的回调；
 * 9. 模拟收到主音量变化通知；
 * 10. 校验是否收到主音量属性变化的回调；
 * 11. 调用NLSTK_McpVolumeDisconnect断连；
 * 12. 模拟链路断连销毁；
 * 13. 校验是否收到连接状态的回调已断连；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Volume子模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(MCP_VOLUME_TEST, MCP_VOLUME_TEST_001)
{
    // 注册回调
    CP_LOG_INFO("enter MCP_VOLUME_TEST_001");
    NLSTK_McpVolumeClientCallBack_S clientCallbacks;
    clientCallbacks.volumeChangeEvent = &Stub_VolumeChangeEvent;
    clientCallbacks.muteStatusChangeEvent = &Stub_MuteStatusChangeEvent;
    clientCallbacks.notifyVolumeChange = &Stub_NotifyVolumeChange;
    clientCallbacks.getVolumeRsp = &Stub_GetVolumeRsp;
    clientCallbacks.setVolumeRsp = &Stub_SetVolumeRsp;
    clientCallbacks.stateChange = &Stub_StateChange;

    // 模拟链路建立
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    TEST_RunQueueStubSchedule();

    // 模拟服务发现已有服务缓存
    STUB_CacheVolumeServiceWithoutStream();

    // 调用连接
    uint32_t ret = NLSTK_McpRegVolumeClientCbk(&clientCallbacks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ret = NLSTK_McpVolumeConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);
    EXPECT_EQ(g_state, NLSTK_MCP_VOLUME_CONNECTING);

    Stub_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);
    TEST_RunQueueStubSchedule();

    // 模拟收到连接流程的SSAP报文
    uint8_t wrtRsp[] = {0x0E, 0x03, 0x11, 0x00, 0x02, 0x01, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, wrtRsp, sizeof(wrtRsp));
    TEST_RunQueueStubSchedule();
    uint8_t readRsp[] = {0x09, 0x03, 0x00, 0x00, 0x00, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, readRsp, sizeof(readRsp));
    TEST_RunQueueStubSchedule();

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);
    EXPECT_EQ(g_state, NLSTK_MCP_VOLUME_CONNECTED);
    Stub_ResetData();

    bool flag = NLSTK_McpGetStreamVolumeAbility(&g_addr);
    EXPECT_EQ(flag, false);

    ret = NLSTK_McpSetVolume(&g_addr, 0x30);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    uint8_t rsp[] = {0x14, 0x03, 0x07, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);
    Stub_ResetData();

    uint8_t ntf[] = {0x0F, 0x03, 0x11, 0x00, 0x04, 0x00, 0x30, 0x00, 0x02, 0x01};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, ntf, sizeof(ntf));
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);
    Stub_ResetData();

    // 连续设置音量，触发音量控制队列
    ret = NLSTK_McpSetVolume(&g_addr, 0x40);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ret = NLSTK_McpSetVolume(&g_addr, 0x50);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ret = NLSTK_McpSetVolume(&g_addr, 0x60);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    uint8_t ntf1[] = {0x0F, 0x03, 0x11, 0x00, 0x04, 0x00, 0x40, 0x00, 0x02, 0x02};
    uint8_t ntf2[] = {0x0F, 0x03, 0x11, 0x00, 0x04, 0x00, 0x50, 0x00, 0x02, 0x03};
    uint8_t ntf3[] = {0x0F, 0x03, 0x11, 0x00, 0x04, 0x00, 0x60, 0x00, 0x02, 0x04};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, ntf1, sizeof(ntf1));
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, ntf2, sizeof(ntf2));
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, ntf3, sizeof(ntf3));
    TEST_RunQueueStubSchedule();
    uint8_t volReq1[] = {0x13, 0x03, 0x12, 0x00, 0x07, 0x60, 0x03 ,0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(volReq1, sizeof(volReq1)));

    // 模拟设置音量，对端回复变更标记不匹配
    ret = NLSTK_McpSetVolume(&g_addr, 0x70);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    uint8_t volReq2[] = {0x13, 0x03, 0x12, 0x00, 0x07, 0x70, 0x04, 0x00};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(volReq2, sizeof(volReq2)));
    uint8_t errRsp[] = {0x14, 0x03, 0x07, 0x02};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, errRsp, sizeof(errRsp));
    TEST_RunQueueStubSchedule();
    // 变更标记不匹配后会主动读取一次音量
    uint8_t readRsp3[] = {0x09, 0x03, 0x80, 0x00, 0x01, 0x05};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, readRsp3, sizeof(readRsp3));
    TEST_RunQueueStubSchedule();

    // 模拟收到音量同步事件
    uint8_t event1[] = {0x0F, 0x07, 0x13, 0x00, 0x02, 0x00, 0x01, 0x00};
    uint8_t event2[] = {0x0F, 0x07, 0x13, 0x00, 0x02, 0x00, 0x02, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, event1, sizeof(event1));
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, event2, sizeof(event2));
    TEST_RunQueueStubSchedule();

    ret = NLSTK_McpVolumeDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, NLSTK_MCP_VOLUME_DISCONNECTED);
    Stub_ResetData();

    // 运行完队列所有任务，防止内存泄露
    TEST_RunQueueStubSchedule();
}

/**
 * @test MCP_VOLUME_TEST_002
 * @brief 此测试用例用于验证音量控制Profile连接流程，对端有音频流音量属性，并发送设置音频流音量控制请求。
 *
 * @details
 * 此用例将按照如下步骤进行：
 * 1. 调用NLSTK_McpRegVolumeClientCbk注册回调；
 * 2. 调用NLSTK_McpVolumeConnect开启连接；
 * 3. 模拟链路建立成功；
 * 4. 模拟收到连接流程中的SSAP响应报文；
 * 5. 校验是否收到连接状态的回调已连接；
 * 6. 发送设置音频流音量控制请求；
 * 7. 模拟收到音频流音量控制响应；
 * 8. 校验是否收到音频流音量控制的回调；
 * 9. 模拟收到音频流音量变化通知；
 * 10. 校验是否收到音频流音量属性变化的回调；
 * 11. 调用NLSTK_McpVolumeDisconnect断连；
 * 12. 模拟链路断连销毁；
 * 13. 校验是否收到连接状态的回调已断连；
 *
 * @pre
 * - SSAP模块已初始化。
 * - MCP模块Volume子模块已使能。
 *
 * @post
 * - 预期结果：调用返回值成功，对应回调函数触发。
 */
TEST_F(MCP_VOLUME_TEST, MCP_VOLUME_TEST_002)
{
    CP_LOG_INFO("enter MCP_VOLUME_TEST_002");
    // 注册回调
    NLSTK_McpVolumeClientCallBack_S clientCallbacks;
    clientCallbacks.volumeChangeEvent = &Stub_VolumeChangeEvent;
    clientCallbacks.muteStatusChangeEvent = &Stub_MuteStatusChangeEvent;
    clientCallbacks.notifyVolumeChange = &Stub_NotifyVolumeChange;
    clientCallbacks.getVolumeRsp = &Stub_GetVolumeRsp;
    clientCallbacks.setVolumeRsp = &Stub_SetVolumeRsp;
    clientCallbacks.stateChange = &Stub_StateChange;

    // 模拟链路建立
    CM_LogicLinkState_S linkState = {.lcid = g_lcid, .result = 0,};
    (void)memcpy_s(&linkState.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState);
    TEST_RunQueueStubSchedule();

    // 模拟服务发现已有服务缓存
    STUB_CacheVolumeServiceWithStream();

    // 调用连接
    uint32_t ret = NLSTK_McpRegVolumeClientCbk(&clientCallbacks);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    ret = NLSTK_McpVolumeConnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 预期收到回调连接中
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);
    EXPECT_EQ(g_state, NLSTK_MCP_VOLUME_CONNECTING);
    Stub_ResetData();

    // 由于判断设备是否为hisiold，链路建立时会初始化发送一个init find req，此处是模拟对端回复find req
    // 此步骤完成后才会发送下一条报文
    TEST_DTAP_SSAP_RevcInitPkt(g_lcid);
    TEST_RunQueueStubSchedule();

    // 模拟收到连接流程的SSAP报文
    uint8_t wrtRsp1[] = {0x0E, 0x03, 0x11, 0x00, 0x02, 0x01, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, wrtRsp1, sizeof(wrtRsp1));
    TEST_RunQueueStubSchedule();
    uint8_t readRsp1[] = {0x09, 0x03, 0x00, 0x00, 0x00, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, readRsp1, sizeof(readRsp1));
    TEST_RunQueueStubSchedule();

    // 模拟收到连接流程的SSAP报文
    uint8_t wrtRsp2[] = {0x0E, 0x03, 0x12, 0x00, 0x02, 0x01, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, wrtRsp2, sizeof(wrtRsp2));
    TEST_RunQueueStubSchedule();
    uint8_t readRsp2[] = {0x09, 0x03, 0x02, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, readRsp2, sizeof(readRsp2));
    TEST_RunQueueStubSchedule();

    // 预期收到回调已连接
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);
    EXPECT_EQ(g_state, NLSTK_MCP_VOLUME_CONNECTED);
    Stub_ResetData();

    bool flag = NLSTK_McpGetStreamVolumeAbility(&g_addr);
    EXPECT_EQ(flag, true);

    McpVolumeNotifyAccessPoint(&g_addr, 0x01, 0x02);

    NLSTK_McpSetStreamVolume_S volume[2] = {{0x10, NLSTK_MCP_MEDIA_STREAM}, {0x20, NLSTK_MCP_CALL_STREAM}};
    ret = NLSTK_McpSetStreamVolume(&g_addr, volume, 2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    uint8_t rsp[] = {0x14, 0x03, 0x09, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);
    Stub_ResetData();

    uint8_t ntf[] = {0x0F, 0x03, 0x12, 0x00, 0x08, 0x00, 0x02, 0x01, 0x10, 0x02, 0x02, 0x20, 0x02, 0x01};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, ntf, sizeof(ntf));
    TEST_RunQueueStubSchedule();
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(memcmp(&g_recvAddr, &g_addr, sizeof(SLE_Addr_S)), 0);
    Stub_ResetData();

    // 连续设置音量，触发音量控制队列
    ret = NLSTK_McpSetStreamVolume(&g_addr, volume, 2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    ret = NLSTK_McpSetStreamVolume(&g_addr, volume, 2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    ret = NLSTK_McpSetStreamVolume(&g_addr, volume, 2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    uint8_t ntf1[] = {0x0F, 0x03, 0x12, 0x00, 0x08, 0x00, 0x02, 0x01, 0x10, 0x02, 0x02, 0x20, 0x02, 0x02};
    uint8_t ntf2[] = {0x0F, 0x03, 0x12, 0x00, 0x08, 0x00, 0x02, 0x01, 0x10, 0x02, 0x02, 0x20, 0x02, 0x03};
    uint8_t ntf3[] = {0x0F, 0x03, 0x12, 0x00, 0x08, 0x00, 0x02, 0x01, 0x10, 0x02, 0x02, 0x20, 0x02, 0x04};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, ntf1, sizeof(ntf1));
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, ntf2, sizeof(ntf2));
    TEST_RunQueueStubSchedule();
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, rsp, sizeof(rsp));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, ntf3, sizeof(ntf3));
    TEST_RunQueueStubSchedule();
    uint8_t volReq1[] = {0x13, 0x03, 0x13, 0x00, 0x09, 0x02, 0x01, 0x10, 0x02, 0x20, 0x03};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(volReq1, sizeof(volReq1)));

    // 模拟设置音量，对端回复变更标记不匹配
    ret = NLSTK_McpSetStreamVolume(&g_addr, volume, 2);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();
    uint8_t volReq2[] = {0x13, 0x03, 0x13, 0x00, 0x09, 0x02, 0x01, 0x10, 0x02, 0x20, 0x04};
    EXPECT_TRUE(TEST_DTAP_CompareLastPkt(volReq2, sizeof(volReq2)));
    uint8_t errRsp[] = {0x14, 0x03, 0x09, 0x02};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, errRsp, sizeof(errRsp));
    TEST_RunQueueStubSchedule();
    // 变更标记不匹配后会主动读取一次音量
    uint8_t readRsp3[] = {0x09, 0x03, 0x02, 0x01, 0x50, 0x01, 0x02, 0x60, 0x01, 0x05};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, readRsp3, sizeof(readRsp3));

    // 模拟收到音量同步事件
    uint8_t event1[] = {0x0F, 0x07, 0x14, 0x00, 0x02, 0x00, 0x01, 0x00};
    uint8_t event2[] = {0x0F, 0x07, 0x14, 0x00, 0x02, 0x00, 0x02, 0x00};
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, event1, sizeof(event1));
    TEST_DTAP_RecDataWithPkt(g_lcid, TCID_SLE_SMTC, event2, sizeof(event2));
    TEST_RunQueueStubSchedule();

    ret = NLSTK_McpVolumeDisconnect(&g_addr);
    EXPECT_EQ(ret, NLSTK_ERRCODE_SUCCESS);
    TEST_RunQueueStubSchedule();

    // 模拟链路断连
    CM_LogicLinkState_S linkState2 = {.lcid = g_lcid, .result = 2,};
    (void)memcpy_s(&linkState2.addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    TEST_CM_CreateLogicLink(CM_MODULE_SSAP, &linkState2);
    TEST_RunQueueStubSchedule();

    // 预期收到回调断连
    EXPECT_EQ(g_recvCbk, true);
    EXPECT_EQ(g_state, NLSTK_MCP_VOLUME_DISCONNECTED);
    Stub_ResetData();

    // 运行完队列所有任务，防止内存泄露
    TEST_RunQueueStubSchedule();
}