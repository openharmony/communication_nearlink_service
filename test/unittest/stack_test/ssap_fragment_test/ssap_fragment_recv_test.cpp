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
#include "ssap_common.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_addr.h"
#include "sdf_timer.h"
#include "cpfwk_log.h"
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
#define TEST_WRITE_VALUE_LEN 100

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;
static DTAP_Data_Info_S g_dtapDataInfo = {.lcid = g_lcid};

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint16_t g_buffLen = 0;
static bool g_isSendRsp = false;

static SDF_TimerParam g_savedTimerParam = {0};
static int g_nextTimerHandle = 100;
static bool g_timerAddFail = false;   // 定时器mock失败开关：置true时ScheduleTimerAdd返回失败

static NLSTK_SsapUuid_S g_uuid1 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}};
static NLSTK_SsapUuid_S g_uuid2 = {.uuid = {0x37, 0xBE, 0xA8, 0x80, 0xFC, 0x70, 0x11, 0xEA,
    0xB7, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03}};
static NLSTK_SsapUuid_S g_uuid3 = {.uuid = {0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
    0x0E, 0x0F, 0x10, 0x11, 0x12}};

// 定时器mock：记录handle并保存定时器参数，供超时回调测试手动触发
static uint32_t TEST_ScheduleTimerAddStubEx(int *handle, SDF_TimerParam *param)
{
    if (handle == NULL || param == NULL) {
        return 1;
    }
    if (g_timerAddFail) {
        return 1;
    }
    *handle = g_nextTimerHandle++;
    g_savedTimerParam = *param;
    return 0;
}

// DTAP发送mock（接收侧用例不发送分片，此处仅保证mock可调用）
static uint32_t CaptureFragSend(DTAP_Data_S *data)
{
    if (data == NULL || data->buff == NULL) {
        return 1;
    }
    SDF_BuffFree(data->buff);
    return 0;
}

// 捕获单包发送（sendFunc）
static void MockSendCb(SSAP_Link *link, SDF_Buff_S *buff, uint8_t opcode)
{
    g_isSendRsp = true;
    g_buffLen = SDF_DataLenGet(buff);
    (void)memcpy_s(g_buffCache, TEST_MAX_BUF_CACHE, SDF_DataOffset(buff), g_buffLen);
    uint8_t type = SSAP_GetOpcodeType(opcode);
    if ((type & SSAP_REPLY_MASK) == 0) {
        SDF_BuffFree(buff);
    } else {
        SSAP_LinkSetTask(link, buff, opcode);
    }
}

static SSAP_Link* CreateLink(uint16_t mtu)
{
    SSAP_Link *link = SSAP_CreateSsapLink(&g_addr, g_lcid, MockSendCb);
    if (link != NULL) {
        link->mtu = mtu;
        link->fragCtx.reassemTimerHandle = SSAP_TIMER_NO_USED_HANDLE;
    }
    return link;
}

static void DeleteLink()
{
    SSAP_DeleteSsapLinkByAddr(&g_addr);
}

// 构造报文并注入SSAP_Recv（模拟对端收到分片）
static void RecvPkt(uint8_t opcode, uint8_t ctrl, const uint8_t *payload, uint32_t payloadLen)
{
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(SSAP_PDU_BASE_LEN + payloadLen);
    ASSERT_NE(buff, nullptr);
    uint8_t *buf = SDF_BuffAppend(buff, SSAP_PDU_BASE_LEN + payloadLen);
    ASSERT_NE(buf, nullptr);
    buf[0] = opcode;
    buf[1] = ctrl;
    if (payloadLen > 0) {
        (void)memcpy_s(buf + SSAP_PDU_BASE_LEN, payloadLen, payload, payloadLen);
    }
    SSAP_Recv(&g_dtapDataInfo, buff);
    SDF_BuffFree(buff);
}

// ReadByUuid完成回调捕获：记录错误码供用例断言
static uint8_t g_readByUuidErrCode = 0xFF;
static void CaptureReadByUuidComplete(int32_t appId, void *arg)
{
    (void)appId;
    SSAP_ReadByUuidComplete_S *complete = (SSAP_ReadByUuidComplete_S *)arg;
    if (complete != nullptr) {
        g_readByUuidErrCode = complete->errCode;
    }
}

// 队列续跑桩：记录被调用次数
static int g_queuedTaskCalled = 0;
static void QueuedTaskStub(SSAP_Link_S *link, void *arg)
{
    (void)link;
    (void)arg;
    g_queuedTaskCalled++;
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
    // 允许写入：WRITE_REQ有响应 + WRITE_CMD无响应，否则SSAPS_WriteControlCheck返回FORBID_WRITE
    propertyParam->operation.operationValue =
        SSAP_OPERATE_INDICATION_WRITE | SSAP_OPERATE_INDICATION_WRITE_NO_RSP;
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

// 构造写入载荷：WRITE类报文载荷 = handle(2) + type(1) + value
static void BuildWritePayload(uint8_t *payload, uint32_t *payloadLen, uint16_t handle, uint8_t type,
    const uint8_t *value, uint16_t valueLen)
{
    payload[0] = (uint8_t)handle;
    payload[1] = (uint8_t)(handle >> 8);
    payload[2] = type;
    (void)memcpy_s(payload + 3, valueLen, value, valueLen);
    *payloadLen = 3 + valueLen;
}

class UT_SSAP_FRAGMENT_RECV : public testing::Test {
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
        g_isSendRsp = false;
        g_buffLen = 0;
    }

    void TearDown() override
    {
        SSAP_LinkDeInit();
        SSAP_ServerDeInit();
    }
};

/* ---------- 服务端分片接收 ---------- */

// 分片WRITE_REQ（BEGIN+MID+END）组包完成后正常写入并返回WRITE_RSP
TEST_F(UT_SSAP_FRAGMENT_RECV, Server_WriteReq_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Server_WriteReq_FragFlow");
    AddService();
    SSAP_Link *link = CreateLink(50);
    // 对端已协商支持分包：verify回显(2+3+100=105B)超mtu由发送侧分包承载，本用例聚焦组包→写入→完整回显；
    // 不支持分包时超限回显按规则回单个SERVER_FRAG错误项（由MULTI_WRITE_ORIGIN_OVER_MTU_NO_FRAG覆盖）
    link->fragCtx.fragment = true;
    uint8_t value[TEST_WRITE_VALUE_LEN] = {0};
    for (int i = 0; i < TEST_WRITE_VALUE_LEN; i++) {
        value[i] = (uint8_t)(i & 0xFF);
    }
    uint8_t payload[200] = {0};
    uint32_t payloadLen = 0;
    BuildWritePayload(payload, &payloadLen, 0x0011, 0, value, TEST_WRITE_VALUE_LEN);  // 104B载荷

    // 完整报文ctrl=0x23（fragment=NO_FRAG, verify=1）；分片ctrl: BEGIN=0x28/MID=0x29/END=0x22
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_WRITE_REQ, 0x28, payload, maxPayload);                       // BEGIN
    EXPECT_NE(link->fragCtx.reassemBuff, nullptr);
    EXPECT_NE(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);
    RecvPkt(SSAP_WRITE_REQ, 0x29, payload + maxPayload, maxPayload);          // MID
    RecvPkt(SSAP_WRITE_REQ, 0x22, payload + 2 * maxPayload,
        payloadLen - 2 * maxPayload);                                          // END

    // 组包完成，写处理完成并发送WRITE_RSP（verify=1回显原值）
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);
    EXPECT_TRUE(g_isSendRsp);
    EXPECT_EQ(g_buffCache[0], SSAP_WRITE_RSP);
    EXPECT_EQ(g_buffLen, (uint32_t)(sizeof(SSAP_PduWriteRsp_S) + 3 + TEST_WRITE_VALUE_LEN));
    EXPECT_EQ(g_buffCache[1] & 0x03, SSAP_CTRL_NO_FRAG);
    EXPECT_EQ(memcmp(g_buffCache + sizeof(SSAP_PduWriteRsp_S) + 3, value, TEST_WRITE_VALUE_LEN), 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Server_WriteReq_FragFlow");
}

// 分片写入中途取消：清空缓存并返回取消写入结果
TEST_F(UT_SSAP_FRAGMENT_RECV, Server_WriteReq_Cancel)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Server_WriteReq_Cancel");
    AddService();
    SSAP_Link *link = CreateLink(50);
    uint8_t payload[100] = {0};
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_WRITE_REQ, 0x28, payload, maxPayload);  // BEGIN缓存
    EXPECT_NE(link->fragCtx.reassemBuff, nullptr);

    // 取消写入分片：ctrl=0x12（fragment=END, oper=CANCEL）
    RecvPkt(SSAP_WRITE_REQ, 0x12, NULL, 0);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);
    // 取消响应：{0x0E, ctrl}，ctrl=NO_FRAG|result(CANCEL<<2)=0x0B
    EXPECT_TRUE(g_isSendRsp);
    EXPECT_EQ(g_buffLen, sizeof(SSAP_PduWriteRsp_S));
    EXPECT_EQ(g_buffCache[0], SSAP_WRITE_RSP);
    EXPECT_EQ(g_buffCache[1], (uint8_t)(SSAP_CTRL_NO_FRAG | (SSAP_CTRL_WRITE_CANCEL << 2)));
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Server_WriteReq_Cancel");
}

// 分片WRITE_CMD组包完成后执行写入（命令无响应）
TEST_F(UT_SSAP_FRAGMENT_RECV, Server_WriteCmd_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Server_WriteCmd_FragFlow");
    AddService();
    SSAP_Link *link = CreateLink(50);
    uint8_t value[TEST_WRITE_VALUE_LEN] = {0};
    uint8_t payload[200] = {0};
    uint32_t payloadLen = 0;
    BuildWritePayload(payload, &payloadLen, 0x0011, 0, value, TEST_WRITE_VALUE_LEN);

    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_WRITE_CMD, 0x28, payload, maxPayload);
    RecvPkt(SSAP_WRITE_CMD, 0x29, payload + maxPayload, maxPayload);
    RecvPkt(SSAP_WRITE_CMD, 0x22, payload + 2 * maxPayload, payloadLen - 2 * maxPayload);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);
    EXPECT_FALSE(g_isSendRsp);  // 命令无响应
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Server_WriteCmd_FragFlow");
}

// 单包WRITE_CMD取消写入：清空缓存，命令无响应
TEST_F(UT_SSAP_FRAGMENT_RECV, Server_WriteCmd_Cancel)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Server_WriteCmd_Cancel");
    AddService();
    SSAP_Link *link = CreateLink(50);
    uint8_t payload[100] = {0};
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_WRITE_CMD, 0x28, payload, maxPayload);  // BEGIN缓存
    EXPECT_NE(link->fragCtx.reassemBuff, nullptr);

    RecvPkt(SSAP_WRITE_CMD, 0x12, NULL, 0);  // oper=CANCEL
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_FALSE(g_isSendRsp);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Server_WriteCmd_Cancel");
}

// 分片READ_REQ（多句柄）组包完成后正常读取并返回READ_RSP
TEST_F(UT_SSAP_FRAGMENT_RECV, Server_ReadReq_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Server_ReadReq_FragFlow");
    AddService();
    SSAP_Link *link = CreateLink(20);
    // 10个句柄的读取请求载荷 = 10 * 3B
    uint8_t payload[64] = {0};
    for (int i = 0; i < 10; i++) {
        payload[i * 3] = (uint8_t)(0x10 + i);
        payload[i * 3 + 1] = 0;
        payload[i * 3 + 2] = 0;
    }
    uint32_t payloadLen = 30;
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_READ_REQ, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    RecvPkt(SSAP_READ_REQ, SSAP_CTRL_FRAG_END, payload + maxPayload, payloadLen - maxPayload);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_TRUE(g_isSendRsp);  // 多句柄读取返回READ_RSP
    EXPECT_EQ(g_buffCache[0], SSAP_READ_RSP);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Server_ReadReq_FragFlow");
}

// 分片VALUE_ACK组包完成后回调确认并清理任务
TEST_F(UT_SSAP_FRAGMENT_RECV, Server_ValueAck_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Server_ValueAck_FragFlow");
    SSAP_Link *link = CreateLink(50);
    // 模拟服务端发送过VALUE_IND（VALUE_ACK的opcode校验要求curTask为VALUE_IND）
    SDF_Buff_S *indBuff = SDF_BuffNewWithReserve(2);
    ASSERT_NE(indBuff, nullptr);
    uint8_t *indBuf = SDF_BuffAppend(indBuff, 2);
    ASSERT_NE(indBuf, nullptr);
    indBuf[0] = SSAP_VALUE_IND;
    indBuf[1] = SSAP_CTRL_NO_FRAG;
    SSAP_LinkSetTask(link, indBuff, SSAP_VALUE_IND);
    EXPECT_EQ(link->curTask.opcode, SSAP_VALUE_IND);
    // 任务需带有效param（SsapTaskValid校验curTask.param非空且valid），否则分片ACK不进入组包与任务清理流程；
    // 组包完成经LinkClearCurrentTask释放（DeleteLink兜底，无泄漏）
    SSAP_TaskParam_S *taskParam = (SSAP_TaskParam_S *)SDF_MemZalloc(sizeof(SSAP_TaskParam_S));
    ASSERT_NE(taskParam, nullptr);
    taskParam->valid = true;
    link->curTask.param = taskParam;

    uint8_t payload[100] = {0};
    for (int i = 0; i < 100; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_VALUE_ACK, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    RecvPkt(SSAP_VALUE_ACK, SSAP_CTRL_FRAG_MID, payload + maxPayload, maxPayload);
    RecvPkt(SSAP_VALUE_ACK, SSAP_CTRL_FRAG_END, payload + 2 * maxPayload, 100 - 2 * maxPayload);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    // 分片ACK组包完成后任务被清理
    EXPECT_EQ(link->curTask.opcode, 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Server_ValueAck_FragFlow");
}

/* ---------- 客户端分片接收 ---------- */

TEST_F(UT_SSAP_FRAGMENT_RECV, Client_ReadRsp_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Client_ReadRsp_FragFlow");
    SSAP_Link *link = CreateLink(50);
    SSAP_ReadReqInfo_S *readReqInfo = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadReqInfo_S));
    ASSERT_NE(readReqInfo, nullptr);
    readReqInfo->handle = 0x0011;
    readReqInfo->type = 0;
    (void)memcpy_s(&readReqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = readReqInfo, .freeFunc = SDF_MemFree, .func = SSAPC_ReadReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(link->curTask.opcode, SSAP_READ_REQ);

    // 分片READ_RSP：单值读取载荷为value
    uint8_t payload[100] = {0};
    for (int i = 0; i < 100; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    // 分片未完成：任务不被清理（回归：分片期间不释放任务）
    EXPECT_EQ(link->curTask.opcode, SSAP_READ_REQ);
    RecvPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_MID, payload + maxPayload, maxPayload);
    RecvPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_END, payload + 2 * maxPayload, 100 - 2 * maxPayload);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->curTask.opcode, 0);  // 组包完成后任务清理
    EXPECT_EQ(link->status, SSAP_LINK_IDLE);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Client_ReadRsp_FragFlow");
}

// 分片RSP到达时task已失效（appid反注册场景）：对齐单包路径清理任务并续跑队列
TEST_F(UT_SSAP_FRAGMENT_RECV, Client_ReadRsp_FragFlow_TaskInvalid)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Client_ReadRsp_FragFlow_TaskInvalid");
    SSAP_Link *link = CreateLink(50);
    // 构造task失效：READ_REQ残留（status=BUSY/opcode匹配以通过opcode校验），param为空使SsapTaskValid为false
    SDF_Buff_S *reqBuff = SDF_BuffNewWithReserve(SSAP_PDU_BASE_LEN);
    ASSERT_NE(reqBuff, nullptr);
    uint8_t *reqBuf = SDF_BuffAppend(reqBuff, SSAP_PDU_BASE_LEN);
    ASSERT_NE(reqBuf, nullptr);
    reqBuf[0] = SSAP_READ_REQ;
    reqBuf[1] = SSAP_CTRL_NO_FRAG;
    SSAP_LinkSetTask(link, reqBuff, SSAP_READ_REQ);
    EXPECT_EQ(link->curTask.opcode, SSAP_READ_REQ);
    EXPECT_FALSE(SsapTaskValid(link));
    // 队列预置待执行任务（param由SSAP_AllocTaskParam克隆，DeleteLink统一释放）
    g_queuedTaskCalled = 0;
    SSAP_TaskParam_S queuedParam = {.appId = 2, .arg = nullptr, .freeFunc = nullptr, .func = QueuedTaskStub,
        .timeout = 3000, .valid = true, .appCallback = nullptr};
    SSAP_AddTaskParamToLink(link, SSAP_AllocTaskParam(&queuedParam));

    // 分片READ_RSP到达：task失效时不组包，清理当前任务并续跑队列
    uint8_t payload[8] = {0};
    RecvPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, payload, sizeof(payload));

    EXPECT_EQ(g_queuedTaskCalled, 1);        // 队列任务被续跑执行（修复前不续跑）
    EXPECT_EQ(link->curTask.opcode, 0);      // 原任务被清理
    EXPECT_EQ(link->curTask.buff, nullptr);  // 原任务缓存被释放
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Client_ReadRsp_FragFlow_TaskInvalid");
}

TEST_F(UT_SSAP_FRAGMENT_RECV, Client_ReadByUuidRsp_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Client_ReadByUuidRsp_FragFlow");
    SSAP_Link *link = CreateLink(50);
    SSAP_ReadByUuidReqInfo_S *reqInfo = (SSAP_ReadByUuidReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadByUuidReqInfo_S));
    ASSERT_NE(reqInfo, nullptr);
    reqInfo->handleStart = 0x01;
    reqInfo->handleEnd = 0xFF;
    reqInfo->dataType = 0;
    (void)memcpy_s(&reqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(&reqInfo->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = reqInfo, .freeFunc = SDF_MemFree, .func = SSAPC_ReadByUuidReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(link->curTask.opcode, SSAP_READ_BY_UUID_REQ);

    // 分片READ_BY_UUID_RSP：单实例载荷 = {handle(2), value}
    uint8_t payload[100] = {0};
    payload[0] = 0x11;
    payload[1] = 0x00;
    for (int i = 2; i < 100; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_READ_BY_UUID_RSP, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    RecvPkt(SSAP_READ_BY_UUID_RSP, SSAP_CTRL_FRAG_MID, payload + maxPayload, maxPayload);
    RecvPkt(SSAP_READ_BY_UUID_RSP, SSAP_CTRL_FRAG_END, payload + 2 * maxPayload, 100 - 2 * maxPayload);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->curTask.opcode, 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Client_ReadByUuidRsp_FragFlow");
}

// 组包完成的READ_BY_UUID_RSP仅4字节（短畸形报文）：上限豁免但下限仍拦截，报INVALID_PDU（修复前误报ITEM_INEXIST）
TEST_F(UT_SSAP_FRAGMENT_RECV, Client_ReadByUuidRsp_FragShortPkt)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Client_ReadByUuidRsp_FragShortPkt");
    SSAP_Link *link = CreateLink(50);
    SSAP_ReadByUuidReqInfo_S *reqInfo = (SSAP_ReadByUuidReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadByUuidReqInfo_S));
    ASSERT_NE(reqInfo, nullptr);
    reqInfo->handleStart = 0x01;
    reqInfo->handleEnd = 0xFF;
    reqInfo->dataType = 0;
    (void)memcpy_s(&reqInfo->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(&reqInfo->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = reqInfo, .freeFunc = SDF_MemFree, .func = SSAPC_ReadByUuidReq,
        .timeout = 3000, .valid = true, .appCallback = CaptureReadByUuidComplete};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(link->curTask.opcode, SSAP_READ_BY_UUID_REQ);

    g_readByUuidErrCode = 0xFF;
    // BEGIN无载荷 + END载荷仅含2字节handle：组包完成仅4字节，命中长度下限
    uint8_t payload[2] = {0x11, 0x00};
    RecvPkt(SSAP_READ_BY_UUID_RSP, SSAP_CTRL_FRAG_BEGIN, nullptr, 0);
    RecvPkt(SSAP_READ_BY_UUID_RSP, SSAP_CTRL_FRAG_END, payload, sizeof(payload));

    EXPECT_EQ(g_readByUuidErrCode, SSAP_ERRCODE_INVALID_PDU);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->curTask.opcode, 0);  // 组包完成后任务清理
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Client_ReadByUuidRsp_FragShortPkt");
}

TEST_F(UT_SSAP_FRAGMENT_RECV, Client_WriteRsp_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Client_WriteRsp_FragFlow");
    SSAP_Link *link = CreateLink(50);
    uint16_t len = 20;
    SSAP_WriteReqInfo_S *writeReq = (SSAP_WriteReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_WriteReqInfo_S) + len);
    ASSERT_NE(writeReq, nullptr);
    writeReq->handle = 0x0011;
    writeReq->type = 0;
    writeReq->value.len = len;
    for (int i = 0; i < len; i++) {
        writeReq->value.value[i] = (uint8_t)(i & 0xFF);
    }
    (void)memcpy_s(&writeReq->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    // 走任务机建任务：curTask.param有效，分片WRITE_RSP才能通过SsapTaskValid进入组包与任务清理
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = writeReq, .freeFunc = SDF_MemFree, .func = SSAPC_WriteReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(link->curTask.opcode, SSAP_WRITE_REQ);

    // 分片WRITE_RSP：verify回显载荷 = {handle(2), type(1), value}
    uint8_t payload[100] = {0};
    payload[0] = 0x11;
    payload[1] = 0x00;
    payload[2] = 0x00;
    for (int i = 3; i < 100; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_WRITE_RSP, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    RecvPkt(SSAP_WRITE_RSP, SSAP_CTRL_FRAG_MID, payload + maxPayload, maxPayload);
    RecvPkt(SSAP_WRITE_RSP, SSAP_CTRL_FRAG_END, payload + 2 * maxPayload, 100 - 2 * maxPayload);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->curTask.opcode, 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Client_WriteRsp_FragFlow");
}

TEST_F(UT_SSAP_FRAGMENT_RECV, Client_FindRsp_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Client_FindRsp_FragFlow");
    SSAP_Link *link = CreateLink(4);
    SSAP_ParamFind_S *findParam = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
    ASSERT_NE(findParam, nullptr);
    findParam->startHandle = 1;
    findParam->endHandle = 0xFF;
    findParam->type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
    (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = findParam, .freeFunc = SDF_MemFree, .func = SSAPC_FindReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(link->curTask.opcode, SSAP_FIND_STRUCTURE_REQ);

    // 分片FIND_RSP：混合模式标准服务条目（8B载荷，mtu=4时maxPayload=2切4片）
    uint8_t payload[] = {0x01, 0x10, 0x00, 0x11, 0x00, 0x02, 0x01, 0x02};
    RecvPkt(SSAP_FIND_STRUCTURE_RSP, SSAP_CTRL_FRAG_BEGIN, payload, 2);
    RecvPkt(SSAP_FIND_STRUCTURE_RSP, SSAP_CTRL_FRAG_MID, payload + 2, 2);
    RecvPkt(SSAP_FIND_STRUCTURE_RSP, SSAP_CTRL_FRAG_MID, payload + 4, 2);
    RecvPkt(SSAP_FIND_STRUCTURE_RSP, SSAP_CTRL_FRAG_END, payload + 6, 2);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->curTask.opcode, 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Client_FindRsp_FragFlow");
}

// 对端支持分包：FIND_STRUCTURE_REQ响应模式置1（多次响应），FIND_RSP按BEGIN/MID/END分片组包
TEST_F(UT_SSAP_FRAGMENT_RECV, Client_FindRsp_FragFlow_RspMode)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Client_FindRsp_FragFlow_RspMode");
    SSAP_Link *link = CreateLink(30);
    link->fragCtx.fragment = true;  // 对端支持分包：请求响应模式置为多次响应
    SSAP_ParamFind_S *findParam = (SSAP_ParamFind_S *)SDF_MemZalloc(sizeof(SSAP_ParamFind_S));
    ASSERT_NE(findParam, nullptr);
    findParam->startHandle = 1;
    findParam->endHandle = 0xFF;
    findParam->type = FIND_STRUCTURE_TYPE_SERVICE_STRUCTURE;
    (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = findParam, .freeFunc = SDF_MemFree, .func = SSAPC_FindReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    // 对端支持分包：ctrl rspMode位=1（bit5）；findType=0(服务结构)+itemType=2(混合)：0x10|0x20=0x30
    static uint8_t reqPktFrag[] = {0x04, 0x30, 0x01, 0x00, 0xFF, 0x00};
    EXPECT_EQ(g_buffLen, sizeof(reqPktFrag));
    EXPECT_EQ(memcmp(g_buffCache, reqPktFrag, g_buffLen), 0);
    EXPECT_EQ(link->curTask.opcode, SSAP_FIND_STRUCTURE_REQ);

    // 分片FIND_RSP：混合模式服务结构载荷82B，maxPayload=28切3片BEGIN/MID/END
    uint8_t payload[82] = {
        0x10, 0x00, 0x00, 0x02, 0x01, 0x00, 0x11, 0x00, 0x02, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x01, 0x12, 0x00, 0x08, 0x12, 0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B,
        0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x00, 0x13, 0x00, 0x08, 0x13, 0x12, 0x10,
        0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03,
        0x00, 0x14, 0x00, 0x0A, 0x14, 0x13, 0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
        0x07, 0x06, 0x05, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
    };
    RecvPkt(SSAP_FIND_STRUCTURE_RSP, SSAP_CTRL_FRAG_BEGIN, payload, 28);
    RecvPkt(SSAP_FIND_STRUCTURE_RSP, SSAP_CTRL_FRAG_MID, payload + 28, 28);
    RecvPkt(SSAP_FIND_STRUCTURE_RSP, SSAP_CTRL_FRAG_END, payload + 56, sizeof(payload) - 56);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->curTask.opcode, 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Client_FindRsp_FragFlow_RspMode");
}

// 对端支持分包：FIND_BY_UUID_REQ响应模式置1（多次响应）
TEST_F(UT_SSAP_FRAGMENT_RECV, Client_FindByUuidReq_RspMode)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Client_FindByUuidReq_RspMode");
    SSAP_Link *link = CreateLink(30);
    link->fragCtx.fragment = true;  // 对端支持分包：请求响应模式置为多次响应
    SSAP_ParamFindByUuid_S *findParam = (SSAP_ParamFindByUuid_S *)SDF_MemZalloc(sizeof(SSAP_ParamFindByUuid_S));
    ASSERT_NE(findParam, nullptr);
    findParam->startHandle = 1;
    findParam->endHandle = 0xFF;
    findParam->type = FIND_STRUCTURE_TYPE_PRIMARY_SERVICE;
    (void)memcpy_s(&findParam->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(&findParam->uuid, sizeof(NLSTK_SsapUuid_S), &g_uuid1, sizeof(NLSTK_SsapUuid_S));
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = findParam, .freeFunc = SDF_MemFree, .func = SSAPC_FindByUuidReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    // 对端支持分包：ctrl rspMode位=1（bit5）；findType=1(首要服务)+itemType=2(混合)：0x11|0x20=0x31
    static uint8_t reqPktFrag[] = {0x06, 0x31, 0x01, 0x00, 0xFF, 0x00, 0x02, 0x01};
    EXPECT_EQ(g_buffLen, sizeof(reqPktFrag));
    EXPECT_EQ(memcmp(g_buffCache, reqPktFrag, g_buffLen), 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Client_FindByUuidReq_RspMode");
}

TEST_F(UT_SSAP_FRAGMENT_RECV, Client_ValueNtf_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Client_ValueNtf_FragFlow");
    SSAP_Link *link = CreateLink(50);
    // 分片VALUE_NTF：载荷 = {handle(2), length(2), value}
    uint8_t payload[104] = {0};
    payload[0] = 0x11;
    payload[1] = 0x00;
    payload[2] = 100;
    payload[3] = 0x00;
    for (int i = 4; i < 104; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_VALUE_NTF, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    RecvPkt(SSAP_VALUE_NTF, SSAP_CTRL_FRAG_MID, payload + maxPayload, maxPayload);
    RecvPkt(SSAP_VALUE_NTF, SSAP_CTRL_FRAG_END, payload + 2 * maxPayload, 104 - 2 * maxPayload);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Client_ValueNtf_FragFlow");
}

TEST_F(UT_SSAP_FRAGMENT_RECV, Client_ValueInd_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Client_ValueInd_FragFlow");
    SSAP_Link *link = CreateLink(50);
    // 分片VALUE_IND：载荷 = {handle(2), length(2), value}
    uint8_t payload[104] = {0};
    payload[0] = 0x11;
    payload[1] = 0x00;
    payload[2] = 100;
    payload[3] = 0x00;
    for (int i = 4; i < 104; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_VALUE_IND, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    RecvPkt(SSAP_VALUE_IND, SSAP_CTRL_FRAG_MID, payload + maxPayload, maxPayload);
    RecvPkt(SSAP_VALUE_IND, SSAP_CTRL_FRAG_END, payload + 2 * maxPayload, 104 - 2 * maxPayload);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_TRUE(g_isSendRsp);  // 指示解析后发送VALUE_ACK
    EXPECT_EQ(g_buffCache[0], SSAP_VALUE_ACK);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Client_ValueInd_FragFlow");
}

TEST_F(UT_SSAP_FRAGMENT_RECV, Client_CallMethodRsp_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Client_CallMethodRsp_FragFlow");
    SSAP_Link *link = CreateLink(50);
    uint16_t len = 20;
    SSAP_CallMethodReqInfo_S *info = (SSAP_CallMethodReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_CallMethodReqInfo_S) + len);
    ASSERT_NE(info, nullptr);
    info->handle = 0x0011;
    info->value.len = len;
    (void)memcpy_s(&info->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    // 走任务机建任务：curTask.param有效，分片CALL_METHOD_RSP才能通过SsapTaskValid进入组包与任务清理
    SSAP_TaskParam_S taskParam = {.appId = 1, .arg = info, .freeFunc = SDF_MemFree, .func = SSAPC_CallMethodReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam, false);
    EXPECT_EQ(link->curTask.opcode, SSAP_CALL_METHOD_REQ);

    uint8_t payload[100] = {0};
    for (int i = 0; i < 100; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_CALL_METHOD_RSP, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    RecvPkt(SSAP_CALL_METHOD_RSP, SSAP_CTRL_FRAG_MID, payload + maxPayload, maxPayload);
    RecvPkt(SSAP_CALL_METHOD_RSP, SSAP_CTRL_FRAG_END, payload + 2 * maxPayload, 100 - 2 * maxPayload);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->curTask.opcode, 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Client_CallMethodRsp_FragFlow");
}

TEST_F(UT_SSAP_FRAGMENT_RECV, Server_MethodCmd_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Server_MethodCmd_FragFlow");
    SSAP_Link *link = CreateLink(50);
    // 分片方法命令：载荷 = {handle(2), param}
    uint8_t payload[104] = {0};
    payload[0] = 0x11;
    payload[1] = 0x00;
    for (int i = 2; i < 104; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_CALL_METHOD_CMD, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    RecvPkt(SSAP_CALL_METHOD_CMD, SSAP_CTRL_FRAG_MID, payload + maxPayload, maxPayload);
    RecvPkt(SSAP_CALL_METHOD_CMD, SSAP_CTRL_FRAG_END, payload + 2 * maxPayload, 104 - 2 * maxPayload);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Server_MethodCmd_FragFlow");
}

TEST_F(UT_SSAP_FRAGMENT_RECV, Server_MethodReq_FragFlow)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Server_MethodReq_FragFlow");
    SSAP_Link *link = CreateLink(50);
    uint8_t payload[104] = {0};
    payload[0] = 0x11;
    payload[1] = 0x00;
    for (int i = 2; i < 104; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_CALL_METHOD_REQ, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    RecvPkt(SSAP_CALL_METHOD_REQ, SSAP_CTRL_FRAG_MID, payload + maxPayload, maxPayload);
    RecvPkt(SSAP_CALL_METHOD_REQ, SSAP_CTRL_FRAG_END, payload + 2 * maxPayload, 104 - 2 * maxPayload);

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Server_MethodReq_FragFlow");
}

/* ---------- 任务管理与异常路径 ---------- */

// 分片未收齐前任务不被清理，组包完成后任务清理并继续执行下一任务
TEST_F(UT_SSAP_FRAGMENT_RECV, Recv_Frag_KeepTaskAndNext)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Recv_Frag_KeepTaskAndNext");
    SSAP_Link *link = CreateLink(50);
    // 任务1
    SSAP_ReadReqInfo_S *readReqInfo1 = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadReqInfo_S));
    ASSERT_NE(readReqInfo1, nullptr);
    readReqInfo1->handle = 0x0011;
    readReqInfo1->type = 0;
    (void)memcpy_s(&readReqInfo1->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam1 = {.appId = 1, .arg = readReqInfo1, .freeFunc = SDF_MemFree, .func = SSAPC_ReadReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam1, false);
    // 任务2（排队等待）
    SSAP_ReadReqInfo_S *readReqInfo2 = (SSAP_ReadReqInfo_S *)SDF_MemZalloc(sizeof(SSAP_ReadReqInfo_S));
    ASSERT_NE(readReqInfo2, nullptr);
    readReqInfo2->handle = 0x0012;
    readReqInfo2->type = 0;
    (void)memcpy_s(&readReqInfo2->addr, sizeof(SLE_Addr_S), &g_addr, sizeof(SLE_Addr_S));
    SSAP_TaskParam_S taskParam2 = {.appId = 1, .arg = readReqInfo2, .freeFunc = SDF_MemFree, .func = SSAPC_ReadReq,
        .timeout = 3000, .valid = true, .appCallback = NULL};
    SSAP_ProcessRequestTask(link, &taskParam2, false);
    EXPECT_EQ(link->curTask.opcode, SSAP_READ_REQ);

    uint8_t payload[100] = {0};
    for (int i = 0; i < 100; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    // 分片响应未收齐：curTask保留（回归：分片期间不清理任务）
    RecvPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    EXPECT_EQ(link->curTask.opcode, SSAP_READ_REQ);
    EXPECT_NE(link->fragCtx.reassemBuff, nullptr);
    RecvPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_MID, payload + maxPayload, maxPayload);
    RecvPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_END, payload + 2 * maxPayload, 100 - 2 * maxPayload);

    // 组包完成后任务清理，并自动执行下一个排队任务（任务2再次发送READ_REQ）
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->curTask.opcode, SSAP_READ_REQ);
    EXPECT_EQ(g_buffCache[0], SSAP_READ_REQ);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Recv_Frag_KeepTaskAndNext");
}

// 分片接收超时：定时器超时后重组上下文被清理，后续END分片被丢弃
TEST_F(UT_SSAP_FRAGMENT_RECV, Recv_Timeout_Clean)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Recv_Timeout_Clean");
    SSAP_Link *link = CreateLink(50);
    uint8_t payload[100] = {0};
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_VALUE_NTF, SSAP_CTRL_FRAG_BEGIN, payload, maxPayload);
    EXPECT_NE(link->fragCtx.reassemBuff, nullptr);
    EXPECT_NE(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);

    // 手动触发重组超时回调（定时器mock保存的参数）
    ASSERT_NE(g_savedTimerParam.callback, nullptr);
    g_savedTimerParam.callback(g_savedTimerParam.args);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);

    // 超时后到达的END分片无上下文，被丢弃
    RecvPkt(SSAP_VALUE_NTF, SSAP_CTRL_FRAG_END, payload + maxPayload, 100 - maxPayload);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Recv_Timeout_Clean");
}

// 分片交错：BEGIN后未收齐又收到新BEGIN，旧缓存被丢弃、新序列正常组包
TEST_F(UT_SSAP_FRAGMENT_RECV, Recv_Interleave_NewBegin)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Recv_Interleave_NewBegin");
    SSAP_Link *link = CreateLink(50);
    uint8_t payload1[60] = {1};
    uint8_t payload2[60] = {2};
    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    RecvPkt(SSAP_VALUE_NTF, SSAP_CTRL_FRAG_BEGIN, payload1, maxPayload);
    EXPECT_NE(link->fragCtx.reassemBuff, nullptr);

    // 未组包完成又收到新BEGIN：旧缓存丢弃
    RecvPkt(SSAP_VALUE_NTF, SSAP_CTRL_FRAG_BEGIN, payload2, maxPayload);
    ASSERT_NE(link->fragCtx.reassemBuff, nullptr);
    uint8_t *cached = SDF_DataOffset(link->fragCtx.reassemBuff);
    EXPECT_EQ(memcmp(cached + SSAP_PDU_BASE_LEN, payload2, maxPayload), 0);

    // 新序列END组包完成
    RecvPkt(SSAP_VALUE_NTF, SSAP_CTRL_FRAG_END, payload2 + maxPayload, 60 - maxPayload);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Recv_Interleave_NewBegin");
}

// 对端恶意发送超MTU分片：丢弃并清理重组上下文
TEST_F(UT_SSAP_FRAGMENT_RECV, Recv_Frag_Drop_OverMtu)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: Recv_Frag_Drop_OverMtu");
    SSAP_Link *link = CreateLink(50);
    uint8_t payload[100] = {0};
    // READ_REQ为REQ类报文，无需curTask即可通过opcode校验，直接进入分片处理
    RecvPkt(SSAP_READ_REQ, SSAP_CTRL_FRAG_BEGIN, payload, sizeof(payload));  // 100B > mtu 50

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: Recv_Frag_Drop_OverMtu");
}

// BEGIN分片到达但重组超时定时器启动失败：释放已缓存的重组数据并丢弃（修复前缓冲悬挂至链路删除）
TEST_F(UT_SSAP_FRAGMENT_RECV, ReassemTimerStartFail_DropBuff)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] begin: ReassemTimerStartFail_DropBuff");
    SSAP_Link *link = CreateLink(50);
    g_timerAddFail = true;
    uint8_t payload[8] = {0};
    // READ_REQ为REQ类报文，无需curTask即可通过opcode校验，直接进入分片处理
    RecvPkt(SSAP_READ_REQ, SSAP_CTRL_FRAG_BEGIN, payload, sizeof(payload));
    g_timerAddFail = false;

    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT_RECV] end: ReassemTimerStartFail_DropBuff");
}
