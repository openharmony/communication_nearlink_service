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

#include "ssap_manager.h"
#include "ssap_link.h"
#include "ssap_utils.h"
#include "ssap_pkt.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_addr.h"
#include "sdf_timer.h"
#include "cpfwk_log.h"
#include "stack_schedule_mock.h"
#include "stack_schedule_stub.h"
#include "stack_dtap_mock.h"
#include "stack_dtap_stub.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

#define TEST_MAX_BUF_CACHE 4096
#define TEST_MAX_FRAG_NUM 16

static SLE_Addr_S g_addr = {.type = PUBLIC_ADDRESS, .addr = {0x03, 0x03, 0x04, 0x04, 0x05, 0x05}};
static uint16_t g_lcid = 1;

static uint8_t g_buffCache[TEST_MAX_BUF_CACHE] = {0};
static uint8_t g_buffLen = 0;
static bool g_isSendRsp = false;

static uint8_t g_sentFrags[TEST_MAX_FRAG_NUM][TEST_MAX_BUF_CACHE] = {{0}};
static uint32_t g_sentFragLens[TEST_MAX_FRAG_NUM] = {0};
static uint32_t g_sentFragCount = 0;
static uint32_t g_failAtFrag = UINT32_MAX;

static SDF_TimerParam g_savedTimerParam = {0};
static int g_nextTimerHandle = 100;
static bool g_timerAddFail = false;

// 白名单：ctrl含fragment字段、支持分包的PDU
static const uint8_t g_fragWhitelist[] = {
    SSAP_FIND_STRUCTURE_RSP,
    SSAP_FIND_STRUCTURE_BY_UUID_RSP,
    SSAP_READ_REQ,
    SSAP_READ_RSP,
    SSAP_READ_BY_UUID_RSP,
    SSAP_WRITE_CMD,
    SSAP_WRITE_REQ,
    SSAP_WRITE_RSP,
    SSAP_VALUE_NTF,
    SSAP_VALUE_IND,
    SSAP_VALUE_ACK,
    SSAP_CALL_METHOD_CMD,
    SSAP_CALL_METHOD_REQ,
    SSAP_CALL_METHOD_RSP,
};

// 非白名单：ctrl低bit不是分片标记的PDU
static const uint8_t g_notFragWhitelist[] = {
    SSAP_ERROR_RSP,
    SSAP_EXCHANGE_INFO_REQ,
    SSAP_EXCHANGE_INFO_RSP,
    SSAP_FIND_STRUCTURE_REQ,
    SSAP_FIND_STRUCTURE_BY_UUID_REQ,
    SSAP_READ_BY_UUID_REQ,
};

// 定时器mock：记录handle并保存定时器参数，供超时回调测试手动触发；g_timerAddFail模拟添加失败
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

// DTAP发送mock失败：g_failAtFrag指定第几片失败（不释放，由SSAP_SendBuffToDTAP释放）
static uint32_t SendFragFail(DTAP_Data_S *data)
{
    if (data == NULL || data->buff == NULL) {
        return 1;
    }
    if (g_sentFragCount == g_failAtFrag) {
        g_sentFragCount++;
        return 1;
    }
    return CaptureFragSend(data);
}

// 捕获单包发送（sendFunc）
static void MockSendCb(SSAP_Link *link, SDF_Buff_S *buff, uint8_t opcode)
{
    (void)link;
    (void)opcode;
    g_isSendRsp = true;
    g_buffLen = SDF_DataLenGet(buff);
    (void)memcpy_s(g_buffCache, TEST_MAX_BUF_CACHE, SDF_DataOffset(buff), g_buffLen);
}

static SSAP_Link* CreateLink(uint16_t mtu, bool fragment)
{
    SSAP_Link *link = SSAP_CreateSsapLink(&g_addr, g_lcid, MockSendCb);
    if (link != NULL) {
        link->mtu = mtu;
        link->fragCtx.fragment = fragment;
        link->fragCtx.reassemTimerHandle = SSAP_TIMER_NO_USED_HANDLE;
    }
    return link;
}

static void DeleteLink()
{
    SSAP_DeleteSsapLinkByAddr(&g_addr);
}

static SDF_Buff_S* BuildPkt(uint8_t opcode, uint8_t ctrl, const uint8_t *payload, uint32_t payloadLen)
{
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(SSAP_PDU_BASE_LEN + payloadLen);
    if (buff == NULL) {
        return NULL;
    }
    uint8_t *buf = SDF_BuffAppend(buff, SSAP_PDU_BASE_LEN + payloadLen);
    if (buf == NULL) {
        SDF_BuffFree(buff);
        return NULL;
    }
    buf[0] = opcode;
    buf[1] = ctrl;
    if (payloadLen > 0) {
        (void)memcpy_s(buf + SSAP_PDU_BASE_LEN, payloadLen, payload, payloadLen);
    }
    return buff;
}

static void ResetSendCapture()
{
    g_sentFragCount = 0;
    (void)memset_s(g_sentFrags, sizeof(g_sentFrags), 0, sizeof(g_sentFrags));
    (void)memset_s(g_sentFragLens, sizeof(g_sentFragLens), 0, sizeof(g_sentFragLens));
    g_failAtFrag = UINT32_MAX;
    g_isSendRsp = false;
    g_buffLen = 0;
}

class UT_SSAP_FRAGMENT : public testing::Test {
protected:
    NiceMock<ScheduleMock> scheduleMock;
    NiceMock<DtapMock> dtapMock;
    void SetUp() override
    {
        EXPECT_CALL(scheduleMock, SchedulePostTask).WillRepeatedly(TEST_SchedulePostTaskStub);
        EXPECT_CALL(scheduleMock, SchedulePostTaskBlocked).WillRepeatedly(TEST_SchedulePostTaskBlockedStub);
        EXPECT_CALL(scheduleMock, ScheduleTimerAdd).WillRepeatedly(TEST_ScheduleTimerAddStubEx);
        EXPECT_CALL(scheduleMock, ScheduleTimerDel).WillRepeatedly(TEST_ScheduleTimerDelStub);
        EXPECT_CALL(dtapMock, DTAP_RegisterDataRecvCb).WillRepeatedly(TEST_DTAP_RegisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_UnregisterDataRecvCb).WillRepeatedly(TEST_DTAP_UnregisterDataRecvCb);
        EXPECT_CALL(dtapMock, DTAP_DataSend).WillRepeatedly(Invoke(SendFragFail));
        SSAP_LinkInit();
        ResetSendCapture();
    }

    void TearDown() override
    {
        SSAP_LinkDeInit();
    }
};

/* ---------- SSAP_IsFragPkt 白名单 ---------- */

TEST_F(UT_SSAP_FRAGMENT, IsFragPkt_NullBuff)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: IsFragPkt_NullBuff");
    EXPECT_FALSE(SSAP_IsFragPkt(NULL));
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: IsFragPkt_NullBuff");
}

TEST_F(UT_SSAP_FRAGMENT, IsFragPkt_ShortLen)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: IsFragPkt_ShortLen");
    // 直接构造1字节短报文（小于SSAP_PDU_BASE_LEN）：长度<2的报文不参与分包判断
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(1);
    ASSERT_NE(buff, nullptr);
    uint8_t *buf = SDF_BuffAppend(buff, 1);
    ASSERT_NE(buf, nullptr);
    buf[0] = SSAP_READ_RSP;
    EXPECT_FALSE(SSAP_IsFragPkt(buff));
    SDF_BuffFree(buff);
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: IsFragPkt_ShortLen");
}

TEST_F(UT_SSAP_FRAGMENT, IsFragPkt_WhiteListBegin)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: IsFragPkt_WhiteListBegin");
    for (size_t i = 0; i < sizeof(g_fragWhitelist); i++) {
        SDF_Buff_S *buff = BuildPkt(g_fragWhitelist[i], SSAP_CTRL_FRAG_BEGIN, NULL, 0);
        ASSERT_NE(buff, nullptr);
        EXPECT_TRUE(SSAP_IsFragPkt(buff)) << "opcode 0x" << std::hex << (int)g_fragWhitelist[i];
        SDF_BuffFree(buff);
    }
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: IsFragPkt_WhiteListBegin");
}

TEST_F(UT_SSAP_FRAGMENT, IsFragPkt_WhiteListMid)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: IsFragPkt_WhiteListMid");
    for (size_t i = 0; i < sizeof(g_fragWhitelist); i++) {
        SDF_Buff_S *buff = BuildPkt(g_fragWhitelist[i], SSAP_CTRL_FRAG_MID, NULL, 0);
        ASSERT_NE(buff, nullptr);
        EXPECT_TRUE(SSAP_IsFragPkt(buff)) << "opcode 0x" << std::hex << (int)g_fragWhitelist[i];
        SDF_BuffFree(buff);
    }
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: IsFragPkt_WhiteListMid");
}

TEST_F(UT_SSAP_FRAGMENT, IsFragPkt_WhiteListEnd)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: IsFragPkt_WhiteListEnd");
    for (size_t i = 0; i < sizeof(g_fragWhitelist); i++) {
        SDF_Buff_S *buff = BuildPkt(g_fragWhitelist[i], SSAP_CTRL_FRAG_END, NULL, 0);
        ASSERT_NE(buff, nullptr);
        EXPECT_TRUE(SSAP_IsFragPkt(buff)) << "opcode 0x" << std::hex << (int)g_fragWhitelist[i];
        SDF_BuffFree(buff);
    }
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: IsFragPkt_WhiteListEnd");
}

TEST_F(UT_SSAP_FRAGMENT, IsFragPkt_WhiteListNoFrag)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: IsFragPkt_WhiteListNoFrag");
    for (size_t i = 0; i < sizeof(g_fragWhitelist); i++) {
        SDF_Buff_S *buff = BuildPkt(g_fragWhitelist[i], SSAP_CTRL_NO_FRAG, NULL, 0);
        ASSERT_NE(buff, nullptr);
        EXPECT_FALSE(SSAP_IsFragPkt(buff)) << "opcode 0x" << std::hex << (int)g_fragWhitelist[i];
        SDF_BuffFree(buff);
    }
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: IsFragPkt_WhiteListNoFrag");
}

// 回归：非白名单PDU（错误响应、信息交换、服务发现请求、按UUID读取请求）的ctrl低bit不是分片标记
TEST_F(UT_SSAP_FRAGMENT, IsFragPkt_NotInWhiteList)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: IsFragPkt_NotInWhiteList");
    uint8_t fragTypes[] = {SSAP_CTRL_FRAG_BEGIN, SSAP_CTRL_FRAG_MID, SSAP_CTRL_FRAG_END};
    for (size_t i = 0; i < sizeof(g_notFragWhitelist); i++) {
        for (size_t j = 0; j < sizeof(fragTypes); j++) {
            SDF_Buff_S *buff = BuildPkt(g_notFragWhitelist[i], fragTypes[j], NULL, 0);
            ASSERT_NE(buff, nullptr);
            EXPECT_FALSE(SSAP_IsFragPkt(buff)) << "opcode 0x" << std::hex << (int)g_notFragWhitelist[i]
                << " ctrl 0x" << (int)fragTypes[j];
            SDF_BuffFree(buff);
        }
    }
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: IsFragPkt_NotInWhiteList");
}

TEST_F(UT_SSAP_FRAGMENT, IsFragPkt_OpOutOfRange)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: IsFragPkt_OpOutOfRange");
    SDF_Buff_S *buff = BuildPkt(0xFF, SSAP_CTRL_FRAG_BEGIN, NULL, 0);
    ASSERT_NE(buff, nullptr);
    EXPECT_FALSE(SSAP_IsFragPkt(buff));
    SDF_BuffFree(buff);
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: IsFragPkt_OpOutOfRange");
}

TEST_F(UT_SSAP_FRAGMENT, IsFragPkt_HighCtrlBitsKeep)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: IsFragPkt_HighCtrlBitsKeep");
    // ctrl高6位（0xE1 & ~0x03 = 0xE0）不影响分片判断
    SDF_Buff_S *buff = BuildPkt(SSAP_READ_RSP, 0xE1, NULL, 0);
    ASSERT_NE(buff, nullptr);
    EXPECT_TRUE(SSAP_IsFragPkt(buff));
    SDF_BuffFree(buff);
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: IsFragPkt_HighCtrlBitsKeep");
}

/* ---------- SSAP_ReassemFragment ---------- */

TEST_F(UT_SSAP_FRAGMENT, Reassem_ParamNull)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_ParamNull");
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload[10] = {0};
    SDF_Buff_S *buff = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, payload, sizeof(payload));

    EXPECT_EQ(SSAP_ReassemFragment(NULL, buff, &complete), SSAP_REASSEM_DROP);
    EXPECT_EQ(SSAP_ReassemFragment(link, NULL, &complete), SSAP_REASSEM_DROP);
    EXPECT_EQ(SSAP_ReassemFragment(link, buff, NULL), SSAP_REASSEM_DROP);

    SDF_BuffFree(buff);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_ParamNull");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_LenInvalid)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_LenInvalid");
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    // 构造1字节报文（小于SSAP_PDU_BASE_LEN）
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(1);
    ASSERT_NE(buff, nullptr);
    uint8_t *buf = SDF_BuffAppend(buff, 1);
    ASSERT_NE(buf, nullptr);
    buf[0] = SSAP_READ_RSP;
    EXPECT_EQ(SSAP_ReassemFragment(link, buff, &complete), SSAP_REASSEM_DROP);
    SDF_BuffFree(buff);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_LenInvalid");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_WriteCancel_WithCache)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_WriteCancel_WithCache");
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload[100] = {0};
    SDF_Buff_S *begin = BuildPkt(SSAP_WRITE_REQ, SSAP_CTRL_FRAG_BEGIN, payload, sizeof(payload));
    EXPECT_EQ(SSAP_ReassemFragment(link, begin, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(begin);
    ASSERT_NE(link->fragCtx.reassemBuff, nullptr);

    // WRITE_REQ ctrl oper=0b10(CANCEL)：清空缓存并返回CANCEL
    uint8_t cancelPkt[] = {SSAP_WRITE_REQ, 0x12};  // fragment=END(0b10), oper=CANCEL(0b10<<3)
    SDF_Buff_S *cancel = BuildPkt(cancelPkt[0], cancelPkt[1], NULL, 0);
    EXPECT_EQ(SSAP_ReassemFragment(link, cancel, &complete), SSAP_REASSEM_CANCEL);
    SDF_BuffFree(cancel);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_WriteCancel_WithCache");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_WriteCancel_NoCache)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_WriteCancel_NoCache");
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t cancelPkt[] = {SSAP_WRITE_CMD, 0x12};  // fragment=END(0b10), oper=CANCEL(0b10<<3)
    SDF_Buff_S *cancel = BuildPkt(cancelPkt[0], cancelPkt[1], NULL, 0);
    EXPECT_EQ(SSAP_ReassemFragment(link, cancel, &complete), SSAP_REASSEM_CANCEL);
    SDF_BuffFree(cancel);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_WriteCancel_NoCache");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_NonWriteNotCancel)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_NonWriteNotCancel");
    // 非WRITE类报文ctrl oper位不影响取消判断，走正常组包逻辑
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload[100] = {0};
    uint8_t pkt[] = {SSAP_READ_RSP, 0x12};  // fragment=END(0b10), 高bit不触发CANCEL
    SDF_Buff_S *end = BuildPkt(pkt[0], pkt[1], payload, sizeof(payload));
    EXPECT_EQ(SSAP_ReassemFragment(link, end, &complete), SSAP_REASSEM_DROP);  // 无上下文
    SDF_BuffFree(end);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_NonWriteNotCancel");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_OverMtu_WithCache)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_OverMtu_WithCache");
    SSAP_Link *link = CreateLink(128, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload[100] = {0};
    SDF_Buff_S *begin = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, payload, 100);
    EXPECT_EQ(SSAP_ReassemFragment(link, begin, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(begin);

    // 分片超过MTU：丢弃并清空缓存
    uint8_t bigPayload[200] = {0};
    SDF_Buff_S *over = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_MID, bigPayload, sizeof(bigPayload));
    EXPECT_EQ(SSAP_ReassemFragment(link, over, &complete), SSAP_REASSEM_DROP);
    SDF_BuffFree(over);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_OverMtu_WithCache");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_OverMtu_NoCache)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_OverMtu_NoCache");
    SSAP_Link *link = CreateLink(128, true);
    SDF_Buff_S *complete = NULL;
    uint8_t bigPayload[200] = {0};
    SDF_Buff_S *over = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, bigPayload, sizeof(bigPayload));
    EXPECT_EQ(SSAP_ReassemFragment(link, over, &complete), SSAP_REASSEM_DROP);
    SDF_BuffFree(over);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_OverMtu_NoCache");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_Begin_NoOld)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_Begin_NoOld");
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload[100] = {0};
    for (int i = 0; i < 100; i++) {
        payload[i] = (uint8_t)i;
    }
    // ctrl=0x04：fragment=BEGIN(0b00), 高bit(verify等)保留
    SDF_Buff_S *begin = BuildPkt(SSAP_READ_RSP, 0x04, payload, sizeof(payload));
    EXPECT_EQ(SSAP_ReassemFragment(link, begin, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(begin);

    ASSERT_NE(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->fragCtx.reassemOp, SSAP_READ_RSP);
    EXPECT_NE(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);
    // 缓存报文：op不变，ctrl fragment位还原为NO_FRAG
    uint8_t *cached = SDF_DataOffset(link->fragCtx.reassemBuff);
    EXPECT_EQ(cached[0], SSAP_READ_RSP);
    EXPECT_EQ(cached[1], 0x04 & ~0x03 | SSAP_CTRL_NO_FRAG);
    EXPECT_EQ(memcmp(cached + SSAP_PDU_BASE_LEN, payload, sizeof(payload)), 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_Begin_NoOld");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_Begin_WriteOperRestore)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_Begin_WriteOperRestore");
    // WRITE_REQ BEGIN分片oper=PART(0b01)：缓存时oper还原为INSTANT
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload[50] = {0};
    uint8_t beginCtrl = SSAP_CTRL_FRAG_BEGIN | (SSAP_CTRL_WRITE_PART << SSAP_CTRL_WRITE_OPER_SHIFT);  // 0x08
    SDF_Buff_S *begin = BuildPkt(SSAP_WRITE_REQ, beginCtrl, payload, sizeof(payload));
    EXPECT_EQ(SSAP_ReassemFragment(link, begin, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(begin);

    ASSERT_NE(link->fragCtx.reassemBuff, nullptr);
    uint8_t *cached = SDF_DataOffset(link->fragCtx.reassemBuff);
    EXPECT_EQ(cached[0], SSAP_WRITE_REQ);
    // fragment还原为NO_FRAG且oper还原为INSTANT
    EXPECT_EQ(cached[1] & 0x03, SSAP_CTRL_NO_FRAG);
    EXPECT_EQ((cached[1] >> SSAP_CTRL_WRITE_OPER_SHIFT) & 0x03, SSAP_CTRL_WRITE_INSTANT);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_Begin_WriteOperRestore");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_Begin_WithOldCache)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_Begin_WithOldCache");
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload1[50] = {1};
    uint8_t payload2[50] = {2};
    SDF_Buff_S *begin1 = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, payload1, sizeof(payload1));
    EXPECT_EQ(SSAP_ReassemFragment(link, begin1, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(begin1);

    // 未组包完成又收到新BEGIN：丢弃旧缓存重新开始
    SDF_Buff_S *begin2 = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, payload2, sizeof(payload2));
    EXPECT_EQ(SSAP_ReassemFragment(link, begin2, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(begin2);

    ASSERT_NE(link->fragCtx.reassemBuff, nullptr);
    uint8_t *cached = SDF_DataOffset(link->fragCtx.reassemBuff);
    EXPECT_EQ(memcmp(cached + SSAP_PDU_BASE_LEN, payload2, sizeof(payload2)), 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_Begin_WithOldCache");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_Mid_NoContext)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_Mid_NoContext");
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload[50] = {0};
    SDF_Buff_S *mid = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_MID, payload, sizeof(payload));
    EXPECT_EQ(SSAP_ReassemFragment(link, mid, &complete), SSAP_REASSEM_DROP);
    SDF_BuffFree(mid);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_Mid_NoContext");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_Mid_OpMismatch)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_Mid_OpMismatch");
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload[50] = {0};
    SDF_Buff_S *begin = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, payload, sizeof(payload));
    EXPECT_EQ(SSAP_ReassemFragment(link, begin, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(begin);

    // opcode不一致的MID分片：丢弃
    SDF_Buff_S *mid = BuildPkt(SSAP_WRITE_RSP, SSAP_CTRL_FRAG_MID, payload, sizeof(payload));
    EXPECT_EQ(SSAP_ReassemFragment(link, mid, &complete), SSAP_REASSEM_DROP);
    SDF_BuffFree(mid);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_Mid_OpMismatch");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_Mid_AppendOk)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_Mid_AppendOk");
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload1[100] = {1};
    uint8_t payload2[50] = {2};
    SDF_Buff_S *begin = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, payload1, sizeof(payload1));
    EXPECT_EQ(SSAP_ReassemFragment(link, begin, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(begin);
    SDF_Buff_S *mid = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_MID, payload2, sizeof(payload2));
    EXPECT_EQ(SSAP_ReassemFragment(link, mid, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(mid);

    ASSERT_NE(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(SDF_DataLenGet(link->fragCtx.reassemBuff), (uint32_t)(SSAP_PDU_BASE_LEN + 100 + 50));
    uint8_t *cached = SDF_DataOffset(link->fragCtx.reassemBuff);
    EXPECT_EQ(memcmp(cached + SSAP_PDU_BASE_LEN, payload1, sizeof(payload1)), 0);
    EXPECT_EQ(memcmp(cached + SSAP_PDU_BASE_LEN + 100, payload2, sizeof(payload2)), 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_Mid_AppendOk");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_Append_OverMax)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_Append_OverMax");
    // 连续MID追加超过SSAP_REASSEM_MAX_SIZE：清空缓存并DROP
    SSAP_Link *link = CreateLink(1024, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload[1022] = {0};  // 分片长度=1024=mtu
    SDF_Buff_S *begin = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, payload, sizeof(payload));
    EXPECT_EQ(SSAP_ReassemFragment(link, begin, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(begin);

    SSAP_ReassemStatus_E status = SSAP_REASSEM_WAIT;
    for (int i = 0; i < 40; i++) {
        SDF_Buff_S *mid = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_MID, payload, sizeof(payload));
        status = SSAP_ReassemFragment(link, mid, &complete);
        SDF_BuffFree(mid);
        if (status == SSAP_REASSEM_DROP) {
            break;
        }
        EXPECT_EQ(status, SSAP_REASSEM_WAIT);
    }
    EXPECT_EQ(status, SSAP_REASSEM_DROP);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_Append_OverMax");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_End_NoContext)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_End_NoContext");
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload[50] = {0};
    SDF_Buff_S *end = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_END, payload, sizeof(payload));
    EXPECT_EQ(SSAP_ReassemFragment(link, end, &complete), SSAP_REASSEM_DROP);
    SDF_BuffFree(end);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_End_NoContext");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_End_Complete)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_End_Complete");
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload1[100] = {0};
    uint8_t payload2[50] = {0};
    uint8_t payload3[30] = {0};
    for (int i = 0; i < 100; i++) {
        payload1[i] = (uint8_t)i;
    }
    for (int i = 0; i < 50; i++) {
        payload2[i] = (uint8_t)(i + 100);
    }
    for (int i = 0; i < 30; i++) {
        payload3[i] = (uint8_t)(i + 150);
    }
    SDF_Buff_S *begin = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_BEGIN, payload1, sizeof(payload1));
    EXPECT_EQ(SSAP_ReassemFragment(link, begin, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(begin);
    SDF_Buff_S *mid = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_MID, payload2, sizeof(payload2));
    EXPECT_EQ(SSAP_ReassemFragment(link, mid, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(mid);
    SDF_Buff_S *end = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_FRAG_END, payload3, sizeof(payload3));
    EXPECT_EQ(SSAP_ReassemFragment(link, end, &complete), SSAP_REASSEM_COMPLETE);
    SDF_BuffFree(end);

    ASSERT_NE(complete, nullptr);
    EXPECT_EQ(SDF_DataLenGet(complete), (uint32_t)(SSAP_PDU_BASE_LEN + 180));
    uint8_t *data = SDF_DataOffset(complete);
    EXPECT_EQ(data[0], SSAP_READ_RSP);
    EXPECT_EQ(data[1] & 0x03, SSAP_CTRL_NO_FRAG);
    EXPECT_EQ(memcmp(data + SSAP_PDU_BASE_LEN, payload1, sizeof(payload1)), 0);
    EXPECT_EQ(memcmp(data + SSAP_PDU_BASE_LEN + 100, payload2, sizeof(payload2)), 0);
    EXPECT_EQ(memcmp(data + SSAP_PDU_BASE_LEN + 150, payload3, sizeof(payload3)), 0);
    EXPECT_EQ(link->fragCtx.reassemBuff, nullptr);
    EXPECT_EQ(link->fragCtx.reassemTimerHandle, SSAP_TIMER_NO_USED_HANDLE);
    SDF_BuffFree(complete);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_End_Complete");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_End_WriteCtrlRestore)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_End_WriteCtrlRestore");
    // WRITE_REQ两片组包：complete的ctrl oper还原为INSTANT、fragment为NO_FRAG
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload1[100] = {1};
    uint8_t payload2[50] = {2};
    uint8_t beginCtrl = SSAP_CTRL_FRAG_BEGIN | (SSAP_CTRL_WRITE_PART << SSAP_CTRL_WRITE_OPER_SHIFT);
    SDF_Buff_S *begin = BuildPkt(SSAP_WRITE_REQ, beginCtrl, payload1, sizeof(payload1));
    EXPECT_EQ(SSAP_ReassemFragment(link, begin, &complete), SSAP_REASSEM_WAIT);
    SDF_BuffFree(begin);
    SDF_Buff_S *end = BuildPkt(SSAP_WRITE_REQ, SSAP_CTRL_FRAG_END, payload2, sizeof(payload2));
    EXPECT_EQ(SSAP_ReassemFragment(link, end, &complete), SSAP_REASSEM_COMPLETE);
    SDF_BuffFree(end);

    ASSERT_NE(complete, nullptr);
    uint8_t *data = SDF_DataOffset(complete);
    EXPECT_EQ(data[0], SSAP_WRITE_REQ);
    EXPECT_EQ(data[1] & 0x03, SSAP_CTRL_NO_FRAG);
    EXPECT_EQ((data[1] >> SSAP_CTRL_WRITE_OPER_SHIFT) & 0x03, SSAP_CTRL_WRITE_INSTANT);
    SDF_BuffFree(complete);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_End_WriteCtrlRestore");
}

TEST_F(UT_SSAP_FRAGMENT, Reassem_FragTypeNoFrag)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: Reassem_FragTypeNoFrag");
    // 直调且fragment=NO_FRAG(0b11)：非BEGIN/MID/END，返回DROP
    SSAP_Link *link = CreateLink(512, true);
    SDF_Buff_S *complete = NULL;
    uint8_t payload[50] = {0};
    SDF_Buff_S *buff = BuildPkt(SSAP_READ_RSP, SSAP_CTRL_NO_FRAG, payload, sizeof(payload));
    EXPECT_EQ(SSAP_ReassemFragment(link, buff, &complete), SSAP_REASSEM_DROP);
    SDF_BuffFree(buff);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: Reassem_FragTypeNoFrag");
}

/* ---------- SSAP_SendFragPkt ---------- */

TEST_F(UT_SSAP_FRAGMENT, SendFrag_ParamNull)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_ParamNull");
    SSAP_Link *link = CreateLink(128, true);
    uint8_t payload[100] = {0};
    SDF_Buff_S *buff = BuildPkt(SSAP_VALUE_ACK, SSAP_CTRL_NO_FRAG, payload, sizeof(payload));
    EXPECT_FALSE(SSAP_SendFragPkt(NULL, buff, SSAP_VALUE_ACK));
    EXPECT_FALSE(SSAP_SendFragPkt(link, NULL, SSAP_VALUE_ACK));
    SDF_BuffFree(buff);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_ParamNull");
}

TEST_F(UT_SSAP_FRAGMENT, SendFrag_LenInvalid)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_LenInvalid");
    SSAP_Link *link = CreateLink(128, true);
    SDF_Buff_S *buff = BuildPkt(SSAP_VALUE_ACK, SSAP_CTRL_NO_FRAG, NULL, 0);
    EXPECT_FALSE(SSAP_SendFragPkt(link, buff, SSAP_VALUE_ACK));
    // 参数校验失败分支不释放原报文，由调用方负责
    SDF_BuffFree(buff);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_LenInvalid");
}

TEST_F(UT_SSAP_FRAGMENT, SendFrag_3Frags_NonWrite)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_3Frags_NonWrite");
    SSAP_Link *link = CreateLink(128, true);
    uint8_t payload[300] = {0};
    for (int i = 0; i < 300; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    SDF_Buff_S *buff = BuildPkt(SSAP_VALUE_ACK, SSAP_CTRL_NO_FRAG, payload, sizeof(payload));
    EXPECT_TRUE(SSAP_SendFragPkt(link, buff, SSAP_VALUE_ACK));
    EXPECT_EQ(g_sentFragCount, 3u);

    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    uint8_t fragTypes[] = {SSAP_CTRL_FRAG_BEGIN, SSAP_CTRL_FRAG_MID, SSAP_CTRL_FRAG_END};
    for (uint32_t i = 0; i < g_sentFragCount; i++) {
        uint8_t *frag = g_sentFrags[i];
        EXPECT_EQ(frag[0], SSAP_VALUE_ACK);
        EXPECT_EQ(frag[1] & 0x03, fragTypes[i]);
        EXPECT_EQ(frag[1] & ~0x03, SSAP_CTRL_NO_FRAG & ~0x03);  // 高6位保留
        uint32_t offset = i * maxPayload;
        uint32_t expectLen = (300 - offset > maxPayload) ? maxPayload : (300 - offset);
        EXPECT_EQ(g_sentFragLens[i], SSAP_PDU_BASE_LEN + expectLen);
        EXPECT_EQ(memcmp(frag + SSAP_PDU_BASE_LEN, payload + offset, expectLen), 0);
    }
    // 非REPLY类：不设置任务
    EXPECT_EQ(link->curTask.opcode, 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_3Frags_NonWrite");
}

TEST_F(UT_SSAP_FRAGMENT, SendFrag_WriteReq_OperPart)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_WriteReq_OperPart");
    SSAP_Link *link = CreateLink(128, true);
    uint8_t payload[300] = {0};
    SDF_Buff_S *buff = BuildPkt(SSAP_WRITE_REQ, SSAP_CTRL_NO_FRAG, payload, sizeof(payload));
    EXPECT_TRUE(SSAP_SendFragPkt(link, buff, SSAP_WRITE_REQ));
    EXPECT_EQ(g_sentFragCount, 3u);

    for (uint32_t i = 0; i < g_sentFragCount; i++) {
        uint8_t *frag = g_sentFrags[i];
        EXPECT_EQ(frag[0], SSAP_WRITE_REQ);
        uint8_t oper = (frag[1] >> SSAP_CTRL_WRITE_OPER_SHIFT) & 0x03;
        if (i == g_sentFragCount - 1) {
            EXPECT_EQ(oper, SSAP_CTRL_WRITE_INSTANT);  // 末片恢复立即写入
        } else {
            EXPECT_EQ(oper, SSAP_CTRL_WRITE_PART);  // 非末片指示继续接收
        }
    }
    // WRITE_REQ为REPLY类：分包成功后设置任务
    EXPECT_EQ(link->curTask.opcode, SSAP_WRITE_REQ);
    EXPECT_NE(link->curTask.buff, nullptr);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_WriteReq_OperPart");
}

TEST_F(UT_SSAP_FRAGMENT, SendFrag_WriteCmd_OperPart)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_WriteCmd_OperPart");
    SSAP_Link *link = CreateLink(128, true);
    uint8_t payload[200] = {0};
    SDF_Buff_S *buff = BuildPkt(SSAP_WRITE_CMD, SSAP_CTRL_NO_FRAG, payload, sizeof(payload));
    EXPECT_TRUE(SSAP_SendFragPkt(link, buff, SSAP_WRITE_CMD));
    EXPECT_EQ(g_sentFragCount, 2u);

    uint8_t *frag1 = g_sentFrags[0];
    uint8_t *frag2 = g_sentFrags[1];
    EXPECT_EQ((frag1[1] >> SSAP_CTRL_WRITE_OPER_SHIFT) & 0x03, SSAP_CTRL_WRITE_PART);
    EXPECT_EQ((frag2[1] >> SSAP_CTRL_WRITE_OPER_SHIFT) & 0x03, SSAP_CTRL_WRITE_INSTANT);
    // WRITE_CMD非REPLY类：不设置任务
    EXPECT_EQ(link->curTask.opcode, 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_WriteCmd_OperPart");
}

TEST_F(UT_SSAP_FRAGMENT, SendFrag_Reply_TaskSet)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_Reply_TaskSet");
    SSAP_Link *link = CreateLink(128, true);
    uint8_t payload[200] = {0};
    SDF_Buff_S *buff = BuildPkt(SSAP_READ_REQ, SSAP_CTRL_NO_FRAG, payload, sizeof(payload));
    EXPECT_TRUE(SSAP_SendFragPkt(link, buff, SSAP_READ_REQ));
    EXPECT_GT(g_sentFragCount, 0u);
    EXPECT_EQ(link->curTask.opcode, SSAP_READ_REQ);
    EXPECT_NE(link->curTask.buff, nullptr);
    EXPECT_EQ(link->status, SSAP_LINK_BUSY);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_Reply_TaskSet");
}

TEST_F(UT_SSAP_FRAGMENT, SendFrag_NoReply_TaskNotSet)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_NoReply_TaskNotSet");
    SSAP_Link *link = CreateLink(128, true);
    uint8_t payload[200] = {0};
    SDF_Buff_S *buff = BuildPkt(SSAP_VALUE_ACK, SSAP_CTRL_NO_FRAG, payload, sizeof(payload));
    EXPECT_TRUE(SSAP_SendFragPkt(link, buff, SSAP_VALUE_ACK));
    EXPECT_GT(g_sentFragCount, 0u);
    EXPECT_EQ(link->curTask.opcode, 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_NoReply_TaskNotSet");
}

TEST_F(UT_SSAP_FRAGMENT, SendFrag_Fail_FirstFrag)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_Fail_FirstFrag");
    SSAP_Link *link = CreateLink(128, true);
    uint8_t payload[200] = {0};
    SDF_Buff_S *buff = BuildPkt(SSAP_VALUE_ACK, SSAP_CTRL_NO_FRAG, payload, sizeof(payload));
    g_failAtFrag = 0;
    EXPECT_FALSE(SSAP_SendFragPkt(link, buff, SSAP_VALUE_ACK));
    // 发送失败时原报文被释放，无泄漏；失败位置记录在g_sentFragCount
    EXPECT_EQ(g_sentFragCount, 1u);
    EXPECT_EQ(link->curTask.opcode, 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_Fail_FirstFrag");
}

TEST_F(UT_SSAP_FRAGMENT, SendFrag_Fail_MidFrag)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_Fail_MidFrag");
    SSAP_Link *link = CreateLink(128, true);
    uint8_t payload[300] = {0};
    SDF_Buff_S *buff = BuildPkt(SSAP_VALUE_ACK, SSAP_CTRL_NO_FRAG, payload, sizeof(payload));
    g_failAtFrag = 1;  // 第2片（MID）发送失败
    EXPECT_FALSE(SSAP_SendFragPkt(link, buff, SSAP_VALUE_ACK));
    EXPECT_EQ(g_sentFragCount, 2u);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_Fail_MidFrag");
}

TEST_F(UT_SSAP_FRAGMENT, SendFrag_Boundary_MtuPlusOne)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_Boundary_MtuPlusOne");
    SSAP_Link *link = CreateLink(128, true);
    uint8_t payload[127] = {0};  // 报文=2+127=129=mtu+1
    for (int i = 0; i < 127; i++) {
        payload[i] = (uint8_t)(i & 0xFF);
    }
    SDF_Buff_S *buff = BuildPkt(SSAP_VALUE_ACK, SSAP_CTRL_NO_FRAG, payload, sizeof(payload));
    EXPECT_TRUE(SSAP_SendFragPkt(link, buff, SSAP_VALUE_ACK));
    EXPECT_EQ(g_sentFragCount, 2u);

    uint32_t maxPayload = link->mtu - SSAP_PDU_BASE_LEN;
    EXPECT_EQ(g_sentFragLens[0], SSAP_PDU_BASE_LEN + maxPayload);  // 片1满载
    EXPECT_EQ(g_sentFragLens[1], SSAP_PDU_BASE_LEN + 1);  // 片2剩余1字节
    EXPECT_EQ(memcmp(g_sentFrags[1] + SSAP_PDU_BASE_LEN, payload + maxPayload, 1), 0);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_Boundary_MtuPlusOne");
}

// 定时器添加失败不影响分包发送结果（StartTimer失败仅打日志）
TEST_F(UT_SSAP_FRAGMENT, SendFrag_TimerAddFail)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_TimerAddFail");
    SSAP_Link *link = CreateLink(128, true);
    uint8_t payload[200] = {0};
    SDF_Buff_S *buff = BuildPkt(SSAP_READ_REQ, SSAP_CTRL_NO_FRAG, payload, sizeof(payload));
    g_timerAddFail = true;
    EXPECT_TRUE(SSAP_SendFragPkt(link, buff, SSAP_READ_REQ));
    g_timerAddFail = false;
    EXPECT_GT(g_sentFragCount, 0u);
    EXPECT_EQ(link->curTask.opcode, SSAP_READ_REQ);
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_TimerAddFail");
}

TEST_F(UT_SSAP_FRAGMENT, SendFrag_HighCtrlBitsKeep)
{
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] begin: SendFrag_HighCtrlBitsKeep");
    SSAP_Link *link = CreateLink(128, true);
    uint8_t payload[200] = {0};
    // ctrl=0x2C：高6位=0x2C&0xFC=0x2C，分片时保留
    SDF_Buff_S *buff = BuildPkt(SSAP_READ_RSP, 0x2C, payload, sizeof(payload));
    EXPECT_TRUE(SSAP_SendFragPkt(link, buff, SSAP_READ_RSP));
    EXPECT_GT(g_sentFragCount, 0u);
    for (uint32_t i = 0; i < g_sentFragCount; i++) {
        EXPECT_EQ(g_sentFrags[i][1] & ~0x03, 0x2C & ~0x03);
    }
    DeleteLink();
    CP_LOG_INFO("[UT_SSAP_FRAGMENT] end: SendFrag_HighCtrlBitsKeep");
}
