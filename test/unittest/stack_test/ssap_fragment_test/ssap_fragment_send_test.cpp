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
#include "gmock/gmock.h"
#include "securec.h"

#include "ssapc_client.h"
#include "ssaps_server.h"
#include "ssaps_server_api.h"
#include "ssaps_service.h"
#include "ssap_manager.h"
#include "ssap_link.h"
#include "ssap_utils.h"
#include "ssap_pkt.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_addr.h"
#include "sdf_timer.h"
#include "cpfwk_log.h"
#include "sdf_string.h"
#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_cm_mock.h"
#include "stack_cm_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

#define TEST_MAX_BUF_CACHE 4096
#define TEST_MAX_FRAG_NUM 16

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;
static DTAP_Data_Info_S g_dtapDataInfo = {.lcid = g_lcid};

static uint8_t g_sentFrags[TEST_MAX_FRAG_NUM][TEST_MAX_BUF_CACHE] = {{0}};
static uint32_t g_sentFragLens[TEST_MAX_FRAG_NUM] = {0};
static uint32_t g_sentFragCount = 0;

static SDF_TimerParam g_savedTimerParam = {0};
static int g_nextTimerHandle = 100;

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

// 定时器mock：记录handle并保存定时器参数
static uint32_t TEST_ScheduleTimerAddStubEx(int *handle, SDF_TimerParam *param)
{
    if (handle == NULL || param == NULL) {
        return 1;
    }
    *handle = g_nextTimerHandle++;
    g_savedTimerParam = *param;
    return 0;
}

// DTAP发送mock捕获：记录每个分片内容
static uint32_t CaptureFragSend(DTAP_Data_S *data)
{
    if (data == NULL || data->buff == NULL) {
        return 1;
    }
    if (g_sentFragCount < TEST_MAX_FRAG_NUM) {
        g_sentFragLens[g_sentFragCount] = SDF_DataLenGet(data->buff);
        (void)memcpy_s(g_sentFrags[g_sentFragCount], sizeof(g_sentFrags[g_sentFragCount]),
            SDF_DataOffset(data->buff), g_sentFragLens[g_sentFragCount]);
        g_sentFragCount++;
    }
    SDF_BuffFree(data->buff);
    return 0;
}

static SSAP_Link* CreateLink(uint16_t mtu, bool fragment)
{
    // 使用真实发送函数SSAP_Send：分包判断收敛在发送入口内部（单包与分片均经DTAP下发，由mock捕获）
    SSAP_Link *link = SSAP_CreateSsapLink(&g_addr, g_lcid, SSAP_Send);
    if (link != NULL) {
        link->mtu = mtu;
        link->fragCtx.fragment = fragment;
    }
    return link;
}

static void DeleteLink()
{
    SSAP_DeleteSsapLinkByAddr(&g_addr);
}

static void ResetCapture()
{
    g_sentFragCount = 0;
    (void)memset_s(g_sentFrags, sizeof(g_sentFrags), 0, sizeof(g_sentFrags));
    (void)memset_s(g_sentFragLens, sizeof(g_sentFragLens), 0, sizeof(g_sentFragLens));
}

// 注册服务端服务：标准服务(含属性) + 自定义服务
static void AddService()
{
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_STD_PRIMARY_SERVICE;
    (void)memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);
    SSAP_ParamAddProperty_S *propertyParam =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    (void)memcpy_s(&propertyParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid2, sizeof(NLSTK_SsapUuid_S));
    propertyParam->val.len = 1;
    propertyParam->val.value[0] = 0xFF;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;
    SSAP_CacheDescriptor(descParam);
    SDF_MemFree(descParam);
    SSAP_StartService(NULL);

    SSAP_ParamAddService_S *serviceParam2 = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam2->serviceType = ITEM_TYPE_VENDOR_PRIMARY_SERVICE;
    (void)memcpy_s(&serviceParam2->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid3, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam2);
    SDF_MemFree(serviceParam2);
    SSAP_StartService(NULL);
}

// 注册自定义服务（含自定义属性）：用于混合模式FIND_RSP载荷超MTU的分包场景
static void AddCusServiceWithCusProp()
{
    SSAP_ParamAddService_S *serviceParam = (SSAP_ParamAddService_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddService_S));
    serviceParam->serviceType = ITEM_TYPE_VENDOR_PRIMARY_SERVICE;
    (void)memcpy_s(&serviceParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid4, sizeof(NLSTK_SsapUuid_S));
    SSAP_CacheService(serviceParam);
    SDF_MemFree(serviceParam);
    SSAP_ParamAddProperty_S *propertyParam =
        (SSAP_ParamAddProperty_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddProperty_S) + 1);
    (void)memcpy_s(&propertyParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid5, sizeof(NLSTK_SsapUuid_S));
    propertyParam->val.len = 1;
    propertyParam->val.value[0] = 0xFF;
    SSAP_CacheProperty(propertyParam);
    SDF_MemFree(propertyParam);
    SSAP_ParamAddDescriptor_S *descParam =
        (SSAP_ParamAddDescriptor_S *)SDF_MemZalloc(sizeof(SSAP_ParamAddDescriptor_S) + 1);
    descParam->type = DESC_TYPE_PROPERTY_INSTRUCTION;
    descParam->val.len = 1;
    descParam->val.value[0] = 0xEE;
    SSAP_CacheDescriptor(descParam);
    SDF_MemFree(descParam);
    SSAP_StartService(NULL);
}

class UT_SSAP_FRAGMENT_SEND : public testing::Test {
protected:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<CmMock> cmMock;
    NiceMock<DtapMock> dtapMock;
    void SetUp() override
    {
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(TEST_ScheduleTimerAddStubEx);
        EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(TEST_ScheduleTimerDelStub);
        EXPECT_CALL(cmMock, CM_RegLogicLinkListener).WillRepeatedly(TEST_CM_RegLogicLinkListener);
        EXPECT_CALL(cmMock, CM_UnRegLogicLinkListener).WillRepeatedly(TEST_CM_UnRegLogicLinkListener);
        EXPECT_CALL(dtapMock, DTAP_RegisterDataRecvCb).WillRepeatedly(TEST_DTAP_RegisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_UnregisterDataRecvCb).WillRepeatedly(TEST_DTAP_UnregisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_DataSend).WillRepeatedly(Invoke(CaptureFragSend));
        SSAP_ServerInit();
        SSAP_LinkInit();
        ResetCapture();
    }

    void TearDown() override
    {
        SSAP_LinkDeInit();
        SSAP_ServerDeInit();
    }
};

/* ---------- 发送侧分包入口 ---------- */

TEST_F(UT_SSAP_FRAGMENT_SEND, ValueAck_FragSend)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: ValueAck_FragSend");
    SSAP_Link *link = CreateLink(128, true);
    uint16_t len = 300;
    SSAP_ValueAckInfo_S *valueAck = (SSAP_ValueAckInfo_S *)SDF_MemZalloc(sizeof(SSAP_ValueAckInfo_S) + len);
    ASSERT_NE(valueAck, nullptr);
    valueAck->value.len = len;
    for (int i = 0; i < len; i++) {
        valueAck->value.value[i] = (uint8_t)(i & 0xFF);
    }
    (void)memcpy_s(&valueAck->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_ValueAck, valueAck, SDF_MemFree);

    // 对端支持分包且超MTU：分包发送，3片BEGIN/MID/END
    EXPECT_EQ(g_sentFragCount, 3u);
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    uint8_t fragTypes[] = {SSAP_CTRL_FRAG_BEGIN, SSAP_CTRL_FRAG_MID, SSAP_CTRL_FRAG_END};
    for (uint32_t i = 0; i < g_sentFragCount; i++) {
        EXPECT_EQ(g_sentFrags[i][0], SSAP_VALUE_ACK);
        EXPECT_EQ(g_sentFrags[i][1] & 0x03, fragTypes[i]);
        uint32_t offset = i * maxPayload;
        uint32_t expectLen = (len - offset > maxPayload) ? maxPayload : (len - offset);
        EXPECT_EQ(g_sentFragLens[i], SSAP_PDU_BASE_LEN + expectLen);
    }
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: ValueAck_FragSend");
}

TEST_F(UT_SSAP_FRAGMENT_SEND, ValueAck_SingleSend_NoFragSupport)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: ValueAck_SingleSend_NoFragSupport");
    SSAP_Link *link = CreateLink(128, false);
    uint16_t len = 300;
    uint8_t expectValue[300] = {0};
    for (int i = 0; i < len; i++) {
        expectValue[i] = (uint8_t)(i & 0xFF);
    }
    SSAP_ValueAckInfo_S *valueAck = (SSAP_ValueAckInfo_S *)SDF_MemZalloc(sizeof(SSAP_ValueAckInfo_S) + len);
    ASSERT_NE(valueAck, nullptr);
    valueAck->value.len = len;
    (void)memcpy_s(valueAck->value.value, len, expectValue, len);
    (void)memcpy_s(&valueAck->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_ValueAck, valueAck, SDF_MemFree);

    // 对端不支持分包：SSAP_Send单包发送完整报文（经DTAP下发，mock捕获）
    EXPECT_EQ(g_sentFragCount, 1u);
    EXPECT_EQ(g_sentFragLens[0], sizeof(SSAP_PduValueAck_S) + len);
    EXPECT_EQ(g_sentFrags[0][0], SSAP_VALUE_ACK);
    EXPECT_EQ(g_sentFrags[0][1] & 0x03, SSAP_CTRL_NO_FRAG);
    EXPECT_EQ(memcmp(g_sentFrags[0] + SSAP_PDU_BASE_LEN, expectValue, len), 0);  // 载荷完整
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: ValueAck_SingleSend_NoFragSupport");
}

TEST_F(UT_SSAP_FRAGMENT_SEND, ValueAck_SingleSend_UnderMtu)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: ValueAck_SingleSend_UnderMtu");
    SSAP_Link *link = CreateLink(128, true);
    uint16_t len = 100;
    SSAP_ValueAckInfo_S *valueAck = (SSAP_ValueAckInfo_S *)SDF_MemZalloc(sizeof(SSAP_ValueAckInfo_S) + len);
    ASSERT_NE(valueAck, nullptr);
    valueAck->value.len = len;
    (void)memcpy_s(&valueAck->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_ValueAck, valueAck, SDF_MemFree);

    // 未超MTU：单包发送（经DTAP下发，mock捕获）
    EXPECT_EQ(g_sentFragCount, 1u);
    EXPECT_EQ(g_sentFragLens[0], sizeof(SSAP_PduValueAck_S) + len);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: ValueAck_SingleSend_UnderMtu");
}

TEST_F(UT_SSAP_FRAGMENT_SEND, WriteReq_FragSend)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: WriteReq_FragSend");
    SSAP_Link *link = CreateLink(128, true);
    uint16_t len = 300;
    SSAP_WriteReqInfo_S *writeReq = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteReqInfo_S) + len);
    ASSERT_NE(writeReq, nullptr);
    writeReq->handle = 0x0011;
    writeReq->type = 0;
    writeReq->value.len = len;
    for (int i = 0; i < len; i++) {
        writeReq->value.value[i] = (uint8_t)(i & 0xFF);
    }
    (void)memcpy_s(&writeReq->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_WriteReq, writeReq, SDF_MemFree);

    EXPECT_GT(g_sentFragCount, 1u);
    for (uint32_t i = 0; i < g_sentFragCount; i++) {
        EXPECT_EQ(g_sentFrags[i][0], SSAP_WRITE_REQ);
        uint8_t oper = (g_sentFrags[i][1] >> 3) & 0x03;
        if (i == g_sentFragCount - 1) {
            EXPECT_EQ(oper, SSAP_CTRL_WRITE_INSTANT);
        } else {
            EXPECT_EQ(oper, SSAP_CTRL_WRITE_PART);
        }
    }
    // WRITE_REQ为REPLY类：分包后设置任务
    EXPECT_EQ(link->curTask.opcode, SSAP_WRITE_REQ);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: WriteReq_FragSend");
}

TEST_F(UT_SSAP_FRAGMENT_SEND, WriteReq_SingleSend_NoFragSupport)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: WriteReq_SingleSend_NoFragSupport");
    SSAP_Link *link = CreateLink(128, false);
    uint16_t len = 300;
    SSAP_WriteReqInfo_S *writeReq = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteReqInfo_S) + len);
    ASSERT_NE(writeReq, nullptr);
    writeReq->handle = 0x0011;
    writeReq->type = 0;
    writeReq->value.len = len;
    (void)memcpy_s(&writeReq->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_WriteReq, writeReq, SDF_MemFree);

    EXPECT_EQ(g_sentFragCount, 1u);
    EXPECT_EQ(g_sentFrags[0][0], SSAP_WRITE_REQ);
    EXPECT_EQ(link->curTask.opcode, SSAP_WRITE_REQ);  // SSAP_Send对REPLY类注册任务
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: WriteReq_SingleSend_NoFragSupport");
}

TEST_F(UT_SSAP_FRAGMENT_SEND, WriteCmd_FragSend)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: WriteCmd_FragSend");
    SSAP_Link *link = CreateLink(128, true);
    uint16_t len = 300;
    SSAP_WriteCmdInfo_S *writeCmd = (SSAP_WriteCmdInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteCmdInfo_S) + len);
    ASSERT_NE(writeCmd, nullptr);
    writeCmd->handle = 0x0011;
    writeCmd->type = 0;
    writeCmd->value.len = len;
    (void)memcpy_s(&writeCmd->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_WriteCmd, writeCmd, SDF_MemFree);

    EXPECT_GT(g_sentFragCount, 1u);
    EXPECT_EQ(g_sentFrags[0][0], SSAP_WRITE_CMD);
    // WRITE_CMD非REPLY类：不设置任务
    EXPECT_EQ(link->curTask.opcode, 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: WriteCmd_FragSend");
}

TEST_F(UT_SSAP_FRAGMENT_SEND, ReadProps_FragSend)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: ReadProps_FragSend");
    SSAP_Link *link = CreateLink(20, true);
    uint8_t num = 30;
    SSAP_ReadPropsInfo_S *readProps =
        (SSAP_ReadPropsInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadPropsInfo_S) + num * sizeof(uint16_t));
    ASSERT_NE(readProps, nullptr);
    readProps->num = num;
    readProps->type = 0;
    for (int i = 0; i < num; i++) {
        readProps->handles[i] = (uint16_t)(0x10 + i);
    }
    (void)memcpy_s(&readProps->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_ReadProps, readProps, SDF_MemFree);

    // 多句柄读取超MTU且对端支持分包：分包发送
    EXPECT_GT(g_sentFragCount, 1u);
    EXPECT_EQ(g_sentFrags[0][0], SSAP_READ_REQ);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: ReadProps_FragSend");
}

TEST_F(UT_SSAP_FRAGMENT_SEND, ReadProps_SingleSend_NoFragSupport)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: ReadProps_SingleSend_NoFragSupport");
    SSAP_Link *link = CreateLink(128, false);
    uint8_t num = 30;
    SSAP_ReadPropsInfo_S *readProps =
        (SSAP_ReadPropsInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadPropsInfo_S) + num * sizeof(uint16_t));
    ASSERT_NE(readProps, nullptr);
    readProps->num = num;
    readProps->type = 0;
    for (int i = 0; i < num; i++) {
        readProps->handles[i] = (uint16_t)(0x10 + i);
    }
    (void)memcpy_s(&readProps->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_ReadProps, readProps, SDF_MemFree);

    EXPECT_EQ(g_sentFragCount, 1u);
    EXPECT_EQ(g_sentFrags[0][0], SSAP_READ_REQ);
    EXPECT_EQ(g_sentFragLens[0], sizeof(SSAP_PduReadReq_S) + num * sizeof(Ssap_PduReadReqItem_S));
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: ReadProps_SingleSend_NoFragSupport");
}

TEST_F(UT_SSAP_FRAGMENT_SEND, CallMethodReq_FragSend)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: CallMethodReq_FragSend");
    SSAP_Link *link = CreateLink(128, true);
    uint16_t len = 300;
    SSAP_CallMethodReqInfo_S *info = (SSAP_CallMethodReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_CallMethodReqInfo_S) + len);
    ASSERT_NE(info, nullptr);
    info->handle = 0x0011;
    info->value.len = len;
    for (int i = 0; i < len; i++) {
        info->value.value[i] = (uint8_t)(i & 0xFF);
    }
    (void)memcpy_s(&info->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_CallMethodReq, info, SDF_MemFree);

    EXPECT_GT(g_sentFragCount, 1u);
    EXPECT_EQ(g_sentFrags[0][0], SSAP_CALL_METHOD_REQ);
    EXPECT_EQ(link->curTask.opcode, SSAP_CALL_METHOD_REQ);  // REPLY类设置任务
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: CallMethodReq_FragSend");
}

TEST_F(UT_SSAP_FRAGMENT_SEND, CallMethodCmd_FragSend)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: CallMethodCmd_FragSend");
    SSAP_Link *link = CreateLink(128, true);
    uint16_t len = 300;
    SSAP_CallMethodCmdInfo_S *info = (SSAP_CallMethodCmdInfo_S *)SDF_MemZalloc(sizeof(SSAP_CallMethodCmdInfo_S) + len);
    ASSERT_NE(info, nullptr);
    info->handle = 0x0011;
    info->value.len = len;
    (void)memcpy_s(&info->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_ProcessNormalTask(link, SSAPC_CallMethodCmd, info, SDF_MemFree);

    EXPECT_GT(g_sentFragCount, 1u);
    EXPECT_EQ(g_sentFrags[0][0], SSAP_CALL_METHOD_CMD);
    EXPECT_EQ(link->curTask.opcode, 0);  // 命令类不设置任务
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: CallMethodCmd_FragSend");
}

// FIND_RSP服务端分包：对端支持分包且响应超MTU时按BEGIN/MID/END分片发送
TEST_F(UT_SSAP_FRAGMENT_SEND, FindRsp_FragSend_Server)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: FindRsp_FragSend_Server");
    AddService();
    SSAP_Link *link = CreateLink(30, true);

    // FIND_STRUCTURE_REQ：SERVICE_STRUCTURE + MIX
    static uint8_t req[] = {0x04, 0x10, 0x01, 0x00, 0xFF, 0xFF};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(req));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(req));
    (void)memcpy_s(tmpBuf, sizeof(req), req, sizeof(req));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    // 载荷37B（服务1 6B + 属性1 11B + 服务2 20B），maxPayload=28 → 2片
    EXPECT_EQ(g_sentFragCount, 2u);
    EXPECT_EQ(g_sentFrags[0][0], SSAP_FIND_STRUCTURE_RSP);
    EXPECT_EQ(g_sentFrags[0][1] & 0x03, SSAP_CTRL_FRAG_BEGIN);
    EXPECT_EQ(g_sentFrags[1][1] & 0x03, SSAP_CTRL_FRAG_END);
    EXPECT_EQ(g_sentFragLens[0], 30u);
    EXPECT_EQ(g_sentFragLens[1], 11u);
    // 拼接全部分片与期望报文逐字节比对（载荷37B：服务1 6B + 属性1 11B + 服务2 20B）
    uint8_t concat[41] = {0};
    uint32_t off = 0;
    for (uint32_t i = 0; i < g_sentFragCount; i++) {
        (void)memcpy_s(concat + off, sizeof(concat) - off, g_sentFrags[i], g_sentFragLens[i]);
        off += g_sentFragLens[i];
    }
    static uint8_t rspFragment[] = {
        // 片1: BEGIN（ctrl=0x0B&~0x03=0x08）
        0x05, 0x08, 0x10, 0x00, 0x00, 0x02, 0x01, 0x00, 0x11, 0x00, 0x02, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x01, 0x12, 0x00, 0x08, 0x12, 0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B,
        // 片2: END
        0x05, 0x0A, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x00,
    };
    EXPECT_EQ(off, sizeof(rspFragment));
    EXPECT_EQ(memcmp(concat, rspFragment, sizeof(rspFragment)), 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: FindRsp_FragSend_Server");
}

// FIND_RSP服务端分包（标准+自定义混合服务）：对端支持分包且响应超MTU时按BEGIN/MID/END分片发送
TEST_F(UT_SSAP_FRAGMENT_SEND, FindRsp_FragSend_Server_CusMix)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: FindRsp_FragSend_Server_CusMix");
    AddService();
    AddCusServiceWithCusProp();
    SSAP_Link *link = CreateLink(30, true);

    // FIND_STRUCTURE_REQ：SERVICE_STRUCTURE + MIX
    static uint8_t req[] = {0x04, 0x10, 0x01, 0x00, 0xFF, 0xFF};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(req));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(req));
    (void)memcpy_s(tmpBuf, sizeof(req), req, sizeof(req));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    // 载荷82B（服务1 6B + 属性1 11B + 服务2 20B + 服务3 20B + 属性2 25B），maxPayload=28 → 3片：
    // 片1 BEGIN(ctrl=0x08): payload[0:28]，片2 MID(ctrl=0x09): payload[28:56]，片3 END(ctrl=0x0A): payload[56:82]
    EXPECT_EQ(g_sentFragCount, 3u);
    EXPECT_EQ(g_sentFrags[0][0], SSAP_FIND_STRUCTURE_RSP);
    EXPECT_EQ(g_sentFrags[0][1] & 0x03, SSAP_CTRL_FRAG_BEGIN);
    EXPECT_EQ(g_sentFrags[1][1] & 0x03, SSAP_CTRL_FRAG_MID);
    EXPECT_EQ(g_sentFrags[2][1] & 0x03, SSAP_CTRL_FRAG_END);
    EXPECT_EQ(g_sentFragLens[0], 30u);
    EXPECT_EQ(g_sentFragLens[1], 30u);
    EXPECT_EQ(g_sentFragLens[2], 28u);

    // 拼接全部分片与期望报文逐字节比对
    uint8_t concat[88] = {0};
    uint32_t off = 0;
    for (uint32_t i = 0; i < g_sentFragCount; i++) {
        (void)memcpy_s(concat + off, sizeof(concat) - off, g_sentFrags[i], g_sentFragLens[i]);
        off += g_sentFragLens[i];
    }
    static uint8_t rspFragment[] = {
        // 片1: BEGIN
        0x05, 0x08, 0x10, 0x00, 0x00, 0x02, 0x01, 0x00, 0x11, 0x00, 0x02, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x01, 0x12, 0x00, 0x08, 0x12, 0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B,
        // 片2: MID
        0x05, 0x09, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x00, 0x13, 0x00, 0x08, 0x13, 0x12, 0x10,
        0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
        // 片3: END
        0x05, 0x0A, 0x00, 0x14, 0x00, 0x0A, 0x14, 0x13, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
        0x07, 0x06, 0x05, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
    };
    EXPECT_EQ(off, sizeof(rspFragment));
    EXPECT_EQ(memcmp(concat, rspFragment, sizeof(rspFragment)), 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: FindRsp_FragSend_Server_CusMix");
}

// FIND_RSP对照：对端不支持分包时按MTU截断单包发送
TEST_F(UT_SSAP_FRAGMENT_SEND, FindRsp_SingleSend_NoFragSupport)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] begin: FindRsp_SingleSend_NoFragSupport");
    AddService();
    SSAP_Link *link = CreateLink(30, false);

    static uint8_t req[] = {0x04, 0x10, 0x01, 0x00, 0xFF, 0xFF};
    SDF_Buff_S *tmp = SDF_BuffNewWithReserve(sizeof(req));
    uint8_t *tmpBuf = SDF_BuffAppend(tmp, sizeof(req));
    (void)memcpy_s(tmpBuf, sizeof(req), req, sizeof(req));
    SSAP_Recv(&g_dtapDataInfo, tmp);
    SDF_BuffFree(tmp);

    // 单包按MTU截断：载荷仅容纳服务1+属性1（17B），整包19B（经DTAP下发，mock捕获）
    EXPECT_EQ(g_sentFragCount, 1u);
    EXPECT_EQ(g_sentFragLens[0], 19u);
    EXPECT_EQ(g_sentFrags[0][0], SSAP_FIND_STRUCTURE_RSP);
    EXPECT_EQ(g_sentFrags[0][1] & 0x03, SSAP_CTRL_NO_FRAG);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_SEND] end: FindRsp_SingleSend_NoFragSupport");
}
