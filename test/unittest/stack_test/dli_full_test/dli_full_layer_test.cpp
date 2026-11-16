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

/* dli 数据链路层通路用例：收包类型分发、异步数据分片重组、命令节点随事件的增删、发送分片切分 */
#include <gtest/gtest.h>
#include "securec.h"

#include "sdf_mem.h"
#include "sdf_buff.h"
#include "sdf_dlist.h"
#include "sdf_evc.h"
#include "sdf_thread.h"
#include "stub_replace.h"

#include "dli.h"
#include "dli_cmd.h"
#include "dli_cmd_struct.h"
#include "dli_event.h"
#include "dli_event_struct.h"
#include "dli_errno.h"
#include "dli_opcode.h"
#include "dli_layer_stru.h"
#include "dli_layer.h"
#include "dli_layer_utils.h"
#include "dli_layer_config.h"
#include "dli_thread.h"
#include "dli_layer_callback.h"
#include "dli_sapi.h"
#include "dli_data_stub.h"

#define LF_TEST_SLEEP_MS 150
#define LF_TEST_EVT_FINAL 0x0003
#define LF_TEST_CMD_READ_ADV 0x0C06
#define LF_TEST_CMD_SCAN DLI_SET_SCAN_PARAMETERS
#define LF_TEST_LCID 0x0011
#define LF_TEST_PAYLOAD_LEN 4
#define LF_TEST_ACB_LEN 8
#define LF_TEST_DATA_LEN 20

static volatile int g_lfAcbCount = 0;
static volatile uint16_t g_lfAcbLcid = 0;
static volatile int g_lfInnerCount = 0;
static volatile bool g_lfInnerCtxNull = true;
static volatile int g_lfExecCount = 0;
static volatile uint16_t g_lfExecStatus = 0;
static volatile uint32_t g_lfZallocCalls = 0;
static volatile long g_lfZallocFailAt = 0;
static volatile int g_lfMemAllocCalls = 0;

static void LfRecvAcb(uint16_t lcid, SDF_Buff_S *buf)
{
    g_lfAcbCount++;
    g_lfAcbLcid = lcid;
}

static void LfInnerEvt(void *context, void *arg, uint32_t len, uint16_t evtOpcode)
{
    (void)arg;
    (void)len;
    (void)evtOpcode;
    g_lfInnerCount++;
    g_lfInnerCtxNull = (context == NULL);
}

static void LfExecCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    (void)cmdRes;
    g_lfExecCount++;
    g_lfExecStatus = status;
}

static uint32_t LfSyncPost(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    if (cb != NULL) {
        cb(arg);
    }
    if (freeCb != NULL && arg != NULL) {
        freeCb(arg);
    }
    return 0;
}

static uint32_t LfSyncPostBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    (void)timeout;
    return LfSyncPost(cb, arg, freeCb);
}

static void *LfZallocCountStub(size_t size)
{
    g_lfZallocCalls++;
    if ((long)g_lfZallocCalls == g_lfZallocFailAt) {
        return NULL;
    }
    void *ptr = SDF_MemAlloc(size);
    if (ptr != NULL) {
        (void)memset_s(ptr, size, 0, size);
    }
    return ptr;
}

static void *LfMemAllocNullStub(size_t size)
{
    (void)size;
    g_lfMemAllocCalls++;
    return NULL;
}

static uint32_t LfDListCountFullStub(SDF_DListHead_S *head)
{
    (void)head;
    return 10000;
}

static void LfMakeAcb(uint8_t *data, uint32_t size, uint16_t lcid, uint8_t pb)
{
    (void)memset_s(data, size, 0, size);
    uint16_t head = (uint16_t)((pb << 12) | lcid);
    DLI_ENCODE2BYTE(data, head);
    DLI_ENCODE2BYTE(data + 2, size - 4);
    for (uint32_t i = 4; i < size; i++) {
        data[i] = 0xAB;
    }
}

static void LfSendAcb(uint16_t lcid, uint8_t pb)
{
    uint8_t data[4 + LF_TEST_PAYLOAD_LEN];
    LfMakeAcb(data, sizeof(data), lcid, pb);
    SlePacket packet = {data, sizeof(data)};
    SleSendToDliStub(PACKET_TYPE_SLE_ACB, &packet);
}

static void LfSendEvent(uint16_t event, uint16_t opcode, uint8_t num, uint8_t status)
{
    uint8_t data[8] = {0};
    DLI_ENCODE2BYTE(data, event);
    DLI_ENCODE2BYTE(data + 4, opcode);
    data[6] = num;
    data[7] = status;
    SlePacket packet = {data, sizeof(data)};
    SleSendToDliStub(PACKET_TYPE_SLE_EVENT, &packet);
    SDF_ThreadSleep(LF_TEST_SLEEP_MS);
}

static void LfSetCmd(uint16_t cmd, uint16_t event, DLI_ExecuteCmdCbk cbk)
{
    DLI_CmdParams params;
    (void)memset_s(&params, sizeof(params), 0, sizeof(params));
    params.cmd = cmd;
    params.event = event;
    params.cbk = cbk;
    ASSERT_EQ(DLI_SUCCESS, DLI_SetCmd(&params));
    SDF_ThreadSleep(LF_TEST_SLEEP_MS);
}

class UT_DLI_LAYER_FULL : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        EXPECT_EQ(0, SDF_EvcInit());
        EXPECT_EQ(0, SDF_ThreadInit(2));
    }

    static void TearDownTestCase()
    {
        SDF_ThreadDeinit();
        SDF_EvcDeinit();
    }

    void SetUp()
    {
        STUB_Init();
        g_lfAcbCount = 0;
        g_lfAcbLcid = 0;
        g_lfInnerCount = 0;
        g_lfInnerCtxNull = true;
        g_lfExecCount = 0;
        g_lfExecStatus = 0;
        g_lfZallocCalls = 0;
        g_lfZallocFailAt = 0;
        g_lfMemAllocCalls = 0;
        DLI_Callback cbk = {0};
        cbk.postOtherThread = LfSyncPost;
        cbk.postOtherBlockedThread = LfSyncPostBlocked;
        cbk.recvAcbHandler = LfRecvAcb;
        DLI_SetCallback(&cbk);
        EXPECT_EQ(DLI_SUCCESS, DLI_Init());
        EXPECT_EQ(DLI_SUCCESS, DLI_Enable());
        EXPECT_NE(0, DLI_ThreadEvcHandleGet());
    }

    void TearDown()
    {
        DLI_Disable();
        DLI_DeInit();
        DLI_SetCallback(NULL);
    }
};

// 校验收包类型分发：异步数据包送数据通道，事件包送事件通道，其余类型只记录不处理
TEST_F(UT_DLI_LAYER_FULL, TestCasePacketTypeDispatch)
{
    static DLI_InnerCbkLineStru evtTable[] = {{LF_TEST_EVT_FINAL, LfInnerEvt}};
    ASSERT_EQ(DLI_SUCCESS, DLI_InnerEventCbkReg(evtTable, 1));

    LfSendAcb(LF_TEST_LCID, DLI_FULL_FRAGMENT);
    EXPECT_EQ(1, g_lfAcbCount);
    EXPECT_EQ(LF_TEST_LCID, g_lfAcbLcid);

    uint8_t icbData[6] = {0};
    SlePacket icbPacket = {icbData, sizeof(icbData)};
    SleSendToDliStub(PACKET_TYPE_SLE_ICB, &icbPacket);
    SleSendToDliStub((SlePacketType)0x99, &icbPacket);
    EXPECT_EQ(1, g_lfAcbCount);

    LfSendEvent(LF_TEST_EVT_FINAL, 0, 0, DLI_SUCCESS);
    EXPECT_EQ(1, g_lfInnerCount);
    EXPECT_TRUE(g_lfInnerCtxNull);
    DLI_InnerEventCbkUnReg(evtTable, 1);
}

// 校验异步数据包长度域校验：声明长度与实际长度不符的包被丢弃，相符的包正常上送
TEST_F(UT_DLI_LAYER_FULL, TestCaseAcbLenMismatch)
{
    uint8_t data[4 + LF_TEST_PAYLOAD_LEN];
    LfMakeAcb(data, sizeof(data), LF_TEST_LCID, DLI_FULL_FRAGMENT);
    DLI_ENCODE2BYTE(data + 2, LF_TEST_PAYLOAD_LEN - 2);
    SlePacket packet = {data, sizeof(data)};
    SleSendToDliStub(PACKET_TYPE_SLE_ACB, &packet);
    EXPECT_EQ(0, g_lfAcbCount);

    LfSendAcb(LF_TEST_LCID, DLI_FULL_FRAGMENT);
    EXPECT_EQ(1, g_lfAcbCount);
}

// 校验分片标志分发：整包直接上送，首片与中间片暂存，末片触发重组后上送
TEST_F(UT_DLI_LAYER_FULL, TestCaseAcbPbDispatch)
{
    LfSendAcb(LF_TEST_LCID, DLI_FULL_FRAGMENT);
    EXPECT_EQ(1, g_lfAcbCount);

    LfSendAcb(LF_TEST_LCID, DLI_FIRST_FRAGMENT);
    LfSendAcb(LF_TEST_LCID, DLI_MIDDLE_FRAGMENT);
    EXPECT_EQ(1, g_lfAcbCount);

    LfSendAcb(LF_TEST_LCID, DLI_LAST_FRAGMENT);
    EXPECT_EQ(2, g_lfAcbCount);
    EXPECT_EQ(LF_TEST_LCID, g_lfAcbLcid);
}

// 校验乱序分片的处理：重复首片或低阶标志到达时丢弃旧数据重新接收，后续分片继续正确重组
TEST_F(UT_DLI_LAYER_FULL, TestCaseFragmentSequence)
{
    // 首片建节点，重复首片触发整包替换
    LfSendAcb(LF_TEST_LCID, DLI_FIRST_FRAGMENT);
    LfSendAcb(LF_TEST_LCID, DLI_FIRST_FRAGMENT);
    EXPECT_EQ(0, g_lfAcbCount);

    // 正常中间片追加，末片重组上送
    LfSendAcb(LF_TEST_LCID, DLI_MIDDLE_FRAGMENT);
    LfSendAcb(LF_TEST_LCID, DLI_LAST_FRAGMENT);
    EXPECT_EQ(1, g_lfAcbCount);

    // 无节点的中间片与末片直接丢弃
    LfSendAcb(LF_TEST_LCID, DLI_MIDDLE_FRAGMENT);
    LfSendAcb(LF_TEST_LCID, DLI_LAST_FRAGMENT);
    EXPECT_EQ(1, g_lfAcbCount);

    // 中间片后首片重到按新接收处理，随后仍能继续重组上送
    LfSendAcb(LF_TEST_LCID, DLI_FIRST_FRAGMENT);
    LfSendAcb(LF_TEST_LCID, DLI_MIDDLE_FRAGMENT);
    LfSendAcb(LF_TEST_LCID, DLI_FIRST_FRAGMENT);
    LfSendAcb(LF_TEST_LCID, DLI_MIDDLE_FRAGMENT);
    LfSendAcb(LF_TEST_LCID, DLI_LAST_FRAGMENT);
    EXPECT_EQ(2, g_lfAcbCount);
    EXPECT_EQ(LF_TEST_LCID, g_lfAcbLcid);
}

// 校验接收节点创建失败兜底：内存不足时新分片被丢弃，恢复后同一链路仍可正常重组
TEST_F(UT_DLI_LAYER_FULL, TestCaseRxNodeCreateFail)
{
    FuncStubInfo stubInfo;
    g_lfZallocFailAt = 2;
    EXPECT_EQ(0, STUB_Replace(&stubInfo, (void *)SDF_MemZalloc, (const void *)LfZallocCountStub));
    LfSendAcb(LF_TEST_LCID, DLI_FIRST_FRAGMENT);
    STUB_Reset(&stubInfo);
    LfSendAcb(LF_TEST_LCID, DLI_LAST_FRAGMENT);
    EXPECT_EQ(0, g_lfAcbCount);

    LfSendAcb(LF_TEST_LCID, DLI_FIRST_FRAGMENT);
    LfSendAcb(LF_TEST_LCID, DLI_LAST_FRAGMENT);
    EXPECT_EQ(1, g_lfAcbCount);
}

// 校验接收链表容量保护：链表计数达到上限时新建节点随即销毁，不上送也不残留
TEST_F(UT_DLI_LAYER_FULL, TestCaseRxListFull)
{
    FuncStubInfo stubInfo;
    EXPECT_EQ(0, STUB_Replace(&stubInfo, (void *)SDF_DListCount, (const void *)LfDListCountFullStub));
    LfSendAcb(LF_TEST_LCID, DLI_FIRST_FRAGMENT);
    STUB_Reset(&stubInfo);
    LfSendAcb(LF_TEST_LCID, DLI_LAST_FRAGMENT);
    EXPECT_EQ(0, g_lfAcbCount);

    LfSendAcb(LF_TEST_LCID, DLI_FIRST_FRAGMENT);
    LfSendAcb(LF_TEST_LCID, DLI_LAST_FRAGMENT);
    EXPECT_EQ(1, g_lfAcbCount);
}

// 校验命令节点随最终事件的增删：未收状态事件的最终事件不删节点，收过状态事件后才删并携带上下文上送
TEST_F(UT_DLI_LAYER_FULL, TestCaseEventNodeStates)
{
    static DLI_InnerCbkLineStru evtTable[] = {{LF_TEST_EVT_FINAL, LfInnerEvt}};
    ASSERT_EQ(DLI_SUCCESS, DLI_InnerEventCbkReg(evtTable, 1));

    LfSetCmd(LF_TEST_CMD_SCAN, LF_TEST_EVT_FINAL, NULL);
    LfSendEvent(LF_TEST_EVT_FINAL, 0, 0, DLI_SUCCESS);
    EXPECT_EQ(1, g_lfInnerCount);
    EXPECT_TRUE(g_lfInnerCtxNull);

    LfSendEvent(DLI_CMD_STATUS_EVT, LF_TEST_CMD_SCAN, 2, DLI_SUCCESS);
    EXPECT_EQ(1, g_lfInnerCount);

    LfSendEvent(LF_TEST_EVT_FINAL, 0, 0, DLI_SUCCESS);
    EXPECT_EQ(2, g_lfInnerCount);
    EXPECT_FALSE(g_lfInnerCtxNull);

    LfSendEvent(LF_TEST_EVT_FINAL, 0, 0, DLI_SUCCESS);
    EXPECT_EQ(3, g_lfInnerCount);
    EXPECT_TRUE(g_lfInnerCtxNull);
    DLI_InnerEventCbkUnReg(evtTable, 1);
}

// 校验状态事件失败删节点：收到失败状态后命令节点被移除，后续最终事件查不到该命令
TEST_F(UT_DLI_LAYER_FULL, TestCaseEventStatusFail)
{
    static DLI_InnerCbkLineStru evtTable[] = {{LF_TEST_EVT_FINAL, LfInnerEvt}};
    ASSERT_EQ(DLI_SUCCESS, DLI_InnerEventCbkReg(evtTable, 1));

    LfSetCmd(LF_TEST_CMD_READ_ADV, LF_TEST_EVT_FINAL, NULL);
    LfSendEvent(DLI_CMD_STATUS_EVT, LF_TEST_CMD_READ_ADV, 2, 9);
    LfSendEvent(LF_TEST_EVT_FINAL, 0, 0, DLI_SUCCESS);
    EXPECT_EQ(1, g_lfInnerCount);
    EXPECT_TRUE(g_lfInnerCtxNull);
    DLI_InnerEventCbkUnReg(evtTable, 1);
}

// 校验完成事件对命令节点的清理：成功与失败的完成事件都能触发上层回调并带回对应状态
TEST_F(UT_DLI_LAYER_FULL, TestCaseEventCompleteStates)
{
    LfSetCmd(LF_TEST_CMD_READ_ADV, DLI_CMD_COMPLETE_EVT, LfExecCbk);
    LfSendEvent(DLI_CMD_COMPLETE_EVT, LF_TEST_CMD_READ_ADV, 2, DLI_SUCCESS);
    EXPECT_EQ(1, g_lfExecCount);
    EXPECT_EQ(DLI_SUCCESS, g_lfExecStatus);

    LfSetCmd(LF_TEST_CMD_READ_ADV, DLI_CMD_COMPLETE_EVT, LfExecCbk);
    LfSendEvent(DLI_CMD_COMPLETE_EVT, LF_TEST_CMD_READ_ADV, 1, 9);
    EXPECT_EQ(2, g_lfExecCount);
    EXPECT_EQ(9, g_lfExecStatus);
}

// 校验多命令排队发送：已发送未完成的命令跳过重发，后续命令在前命令腾出并发额度后继续发送
TEST_F(UT_DLI_LAYER_FULL, TestCaseTwoCmdQueue)
{
    static DLI_InnerCbkLineStru evtTable[] = {{LF_TEST_EVT_FINAL, LfInnerEvt}};
    ASSERT_EQ(DLI_SUCCESS, DLI_InnerEventCbkReg(evtTable, 1));

    LfSetCmd(LF_TEST_CMD_SCAN, LF_TEST_EVT_FINAL, NULL);
    LfSendEvent(DLI_CMD_STATUS_EVT, LF_TEST_CMD_SCAN, 2, DLI_SUCCESS);
    LfSetCmd(LF_TEST_CMD_SCAN, LF_TEST_EVT_FINAL, NULL);

    LfSendEvent(LF_TEST_EVT_FINAL, 0, 0, DLI_SUCCESS);
    EXPECT_EQ(1, g_lfInnerCount);
    EXPECT_FALSE(g_lfInnerCtxNull);
    LfSendEvent(DLI_CMD_STATUS_EVT, LF_TEST_CMD_SCAN, 2, DLI_SUCCESS);
    LfSendEvent(LF_TEST_EVT_FINAL, 0, 0, DLI_SUCCESS);
    EXPECT_EQ(2, g_lfInnerCount);
    EXPECT_FALSE(g_lfInnerCtxNull);
    DLI_InnerEventCbkUnReg(evtTable, 1);
}

// 校验数据发送的失败重试：发送内存不足时按固定次数重试后放弃并释放数据
TEST_F(UT_DLI_LAYER_FULL, TestCaseDataSendRetry)
{
    DLI_AllDataSet(LF_TEST_ACB_LEN, 4, LF_TEST_ACB_LEN, 4);
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(LF_TEST_DATA_LEN);
    ASSERT_TRUE(buff != NULL);
    uint8_t *info = SDF_BuffAppend(buff, LF_TEST_DATA_LEN);
    ASSERT_TRUE(info != NULL);
    (void)memset_s(info, LF_TEST_DATA_LEN, 0x44, LF_TEST_DATA_LEN);
    DLI_DataStru *data = DLI_DefaultDataStruCreate(LF_TEST_LCID, DLI_DATATYPE_ACB, 0, 0, buff);
    ASSERT_TRUE(data != NULL);

    FuncStubInfo stubInfo;
    EXPECT_EQ(0, STUB_Replace(&stubInfo, (void *)SDF_MemAlloc, (const void *)LfMemAllocNullStub));
    EXPECT_EQ(DLI_SUCCESS, DLI_DataSend(data));
    SDF_ThreadSleep(LF_TEST_SLEEP_MS);
    STUB_Reset(&stubInfo);
    EXPECT_EQ(3, g_lfMemAllocCalls);
}

// 校验数据分片切分的标志序列：首包标记首片，中间包标记中间片，末包标记末片，整包短数据标记整片
TEST_F(UT_DLI_LAYER_FULL, TestCaseSplitDataPbSeq)
{
    DLI_AllDataSet(LF_TEST_ACB_LEN, 4, LF_TEST_ACB_LEN, 4);

    SDF_Buff_S *buff = SDF_BuffNewWithReserve(LF_TEST_DATA_LEN);
    ASSERT_TRUE(buff != NULL);
    ASSERT_TRUE(SDF_BuffAppend(buff, LF_TEST_DATA_LEN) != NULL);
    DLI_DataStru *data = DLI_DefaultDataStruCreate(LF_TEST_LCID, DLI_DATATYPE_ACB, 0, 0, buff);
    ASSERT_TRUE(data != NULL);
    ASSERT_EQ(3, DLI_GetDataFragmentNums(buff));

    SDF_Buff_S *frags[3];
    for (uint32_t i = 0; i < 3; i++) {
        frags[i] = SDF_BuffNew(DLI_GetFragmentMaxLen());
        ASSERT_TRUE(frags[i] != NULL);
    }
    ASSERT_EQ(DLI_SUCCESS, DLI_SplitData(data, frags, 3));
    for (uint32_t i = 0; i < 3; i++) {
        uint8_t *p = SDF_DataOffset(frags[i]);
        uint8_t pb = (uint8_t)((DLI_DECODE2BYTE(p + 1) >> 12) & 0x03);
        EXPECT_EQ(i + 1, pb);
        SDF_BuffFree(frags[i]);
    }
    DLI_DataStruDestroy(data);

    SDF_Buff_S *shortBuff = SDF_BuffNewWithReserve(LF_TEST_ACB_LEN - LF_TEST_PAYLOAD_LEN);
    ASSERT_TRUE(shortBuff != NULL);
    ASSERT_TRUE(SDF_BuffAppend(shortBuff, LF_TEST_ACB_LEN - LF_TEST_PAYLOAD_LEN) != NULL);
    DLI_DataStru *shortData = DLI_DefaultDataStruCreate(LF_TEST_LCID, DLI_DATATYPE_ACB, 0, 0, shortBuff);
    ASSERT_TRUE(shortData != NULL);
    ASSERT_EQ(1, DLI_GetDataFragmentNums(shortBuff));
    SDF_Buff_S *oneFrag = SDF_BuffNew(DLI_GetFragmentMaxLen());
    ASSERT_TRUE(oneFrag != NULL);
    ASSERT_EQ(DLI_SUCCESS, DLI_SplitData(shortData, &oneFrag, 1));
    uint8_t *p = SDF_DataOffset(oneFrag);
    EXPECT_EQ(DLI_FULL_FRAGMENT, (uint8_t)((DLI_DECODE2BYTE(p + 1) >> 12) & 0x03));
    SDF_BuffFree(oneFrag);
    DLI_DataStruDestroy(shortData);
}
