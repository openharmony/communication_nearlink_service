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

#include "dtap_trans_test.h"

#include "dli_callback.h"
#include "dli_errno.h"
#include "dtap_scheduler.h"

namespace {
using namespace DtapTransTest;

DTAP_Channel_S *NewSchedChannel(uint16_t lcid, uint8_t srcTcid)
{
    DTAP_Channel_S *chan = (DTAP_Channel_S *)SDF_MemZalloc(sizeof(DTAP_Channel_S));
    if (chan == nullptr) {
        return nullptr;
    }
    SDF_DListHeadInit(&chan->pktList);
    SDF_DListEntryInit(&chan->schedEntry);
    SDF_DListEntryInit(&chan->entry);
    chan->priority = DTAP_PRIORITY_CMD;
    chan->lcid = lcid;
    chan->srcTcid = srcTcid;
    return chan;
}

void FreeSchedChannel(DTAP_Channel_S *chan)
{
    SDF_MemFree(chan);
}
}

TEST_F(UT_DTAP_TRANS, DtapSchedConnectCbkGuards)
{
    // 同一连接句柄只建一个配额节点；异常事件直接忽略
    TEST_DTAP_DliConnectDo(0x11, 0);
    EXPECT_NE(DTAP_GetLcidBufferNode(0x11), nullptr);
    TEST_DTAP_DliConnectDo(0x11, 0);
    TEST_DTAP_DliConnectDo(0x11, 1);
    // 重复连接与异常状态均不重复建账
    TEST_DTAP_DliConnectDo(0x12, 0);
    EXPECT_NE(DTAP_GetLcidBufferNode(0x12), nullptr);

    TEST_DTAP_DliDisconnectDo(0x11);
    TEST_DTAP_DliDisconnectDo(0x12);
    EXPECT_EQ(DTAP_GetLcidBufferNode(0x11), nullptr);
}

TEST_F(UT_DTAP_TRANS, DtapSchedQuotaRemainderAlloc)
{
    // 三条链路排队2/1/1、总配额8：截断分配后按最大余数补齐，总额等于总配额
    TEST_DTAP_DliConnectDo(0x21, 0);
    TEST_DTAP_DliConnectDo(0x22, 0);
    TEST_DTAP_DliConnectDo(0x23, 0);
    DTAP_Channel_S *ch1 = NewSchedChannel(0x21, 0);
    DTAP_Channel_S *ch2 = NewSchedChannel(0x22, 0);
    DTAP_Channel_S *ch3 = NewSchedChannel(0x23, 0);
    ASSERT_NE(ch1, nullptr);
    ASSERT_NE(ch2, nullptr);
    ASSERT_NE(ch3, nullptr);

    // 冻结发送使排队计数保留
    TEST_DTAP_SetApBufferNum(0);
    for (int i = 0; i < 2; i++) {
        NEW_BUFF(buff, 10);
        EXPECT_EQ(DTAP_DataSendWithPriority(ch1, buff), DTAP_SUCCESS);
    }
    for (int i = 0; i < 1; i++) {
        NEW_BUFF(buff, 10);
        EXPECT_EQ(DTAP_DataSendWithPriority(ch2, buff), DTAP_SUCCESS);
        buff = NewBuff(10);
        ASSERT_NE(buff, nullptr);
        EXPECT_EQ(DTAP_DataSendWithPriority(ch3, buff), DTAP_SUCCESS);
    }

    TEST_DTAP_SetApBufferNum(8);
    DTAP_LcidBufferNode *n1 = DTAP_GetLcidBufferNode(0x21);
    DTAP_LcidBufferNode *n2 = DTAP_GetLcidBufferNode(0x22);
    DTAP_LcidBufferNode *n3 = DTAP_GetLcidBufferNode(0x23);
    ASSERT_NE(n1, nullptr);
    ASSERT_NE(n2, nullptr);
    ASSERT_NE(n3, nullptr);
    EXPECT_EQ(n1->quota, 4);
    EXPECT_EQ(n2->quota, 2);
    EXPECT_EQ(n3->quota, 2);
    EXPECT_EQ(n1->quota + n2->quota + n3->quota, 8);

    DTAP_ChannelDown(0x21, 0);
    DTAP_ChannelDown(0x22, 0);
    DTAP_ChannelDown(0x23, 0);
    TEST_DTAP_DliDisconnectDo(0x21);
    TEST_DTAP_DliDisconnectDo(0x22);
    TEST_DTAP_DliDisconnectDo(0x23);
    FreeSchedChannel(ch1);
    FreeSchedChannel(ch2);
    FreeSchedChannel(ch3);
}

TEST_F(UT_DTAP_TRANS, DtapSchedFragmentSaveAndReuse)
{
    // 总配额1时大包部分发送，余片入碎片队列；完成回执后碎片按已分片路径续发
    TEST_DTAP_DliConnectDo(0x31, 0);
    TEST_DTAP_SetApBufferNum(1);
    DTAP_Channel_S *chan = NewSchedChannel(0x31, 0);
    ASSERT_NE(chan, nullptr);

    DTAP_LcidBufferNode *node = DTAP_GetLcidBufferNode(0x31);
    ASSERT_NE(node, nullptr);

    // 12000字节按600分片为20片，每次最多发1片，其余入碎片队列
    NEW_BUFF(big, 12000);
    EXPECT_EQ(DTAP_DataSendWithPriority(chan, big), DTAP_SUCCESS);
    uint32_t queuedAfterFirst = node->queuedPktCnt;

    // 第二个大包先冻结入队，完成回执后调度时复用既有碎片通道保存余片
    NEW_BUFF(big2, 12000);
    EXPECT_EQ(DTAP_DataSendWithPriority(chan, big2), DTAP_SUCCESS);
    EXPECT_GT(node->queuedPktCnt, queuedAfterFirst);
    EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, 0x31, 1), DLI_SUCCESS);

    // 连续完成回执把两个大包的碎片队列全部消化
    for (int i = 0; i < 48; i++) {
        EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, 0x31, 1), DLI_SUCCESS);
    }
    EXPECT_EQ(node->queuedPktCnt, 0);

    DTAP_ChannelDown(0x31, 0);
    TEST_DTAP_DliDisconnectDo(0x31);
    FreeSchedChannel(chan);
}

TEST_F(UT_DTAP_TRANS, DtapSchedSendFailKeepsQueue)
{
    // 发送接口返回失败：本优先级停止调度，排队数不减
    TEST_DTAP_DliConnectDo(0x41, 0);
    TEST_DTAP_SetApBufferNum(4);
    DTAP_Channel_S *chan = NewSchedChannel(0x41, 0);
    ASSERT_NE(chan, nullptr);
    DTAP_LcidBufferNode *node = DTAP_GetLcidBufferNode(0x41);
    ASSERT_NE(node, nullptr);

    EXPECT_CALL(dliLayerMock, DLI_DataSend).WillRepeatedly(testing::Return(DLI_HARDWARE_FAILURE));
    NEW_BUFF(buff, 10);
    EXPECT_EQ(DTAP_DataSendWithPriority(chan, buff), DTAP_SUCCESS);
    EXPECT_EQ(node->queuedPktCnt, 1);

    DTAP_ChannelDown(0x41, 0);
    TEST_DTAP_DliDisconnectDo(0x41);
    FreeSchedChannel(chan);
}

TEST_F(UT_DTAP_TRANS, DtapSchedSendCompleteUnknownLink)
{
    // 未知连接句柄的完成回执不崩溃、不影响既有账目
    TEST_DTAP_DliConnectDo(0x51, 0);
    EXPECT_EQ(TEST_NOCPEventDo(DLI_REG_MODULE_DTAP, 0x99, 1), DLI_SUCCESS);
    EXPECT_NE(DTAP_GetLcidBufferNode(0x51), nullptr);
    TEST_DTAP_DliDisconnectDo(0x51);
}

TEST_F(UT_DTAP_TRANS, DtapSchedChannelDownBilling)
{
    // 同链路两个通道各自排队后分别下线：排队数同步扣减
    TEST_DTAP_DliConnectDo(0x61, 0);
    TEST_DTAP_SetApBufferNum(0);
    DTAP_Channel_S *ch1 = NewSchedChannel(0x61, 1);
    DTAP_Channel_S *ch2 = NewSchedChannel(0x61, 2);
    ASSERT_NE(ch1, nullptr);
    ASSERT_NE(ch2, nullptr);
    NEW_BUFF(buff, 10);
    EXPECT_EQ(DTAP_DataSendWithPriority(ch1, buff), DTAP_SUCCESS);
    buff = NewBuff(10);
    ASSERT_NE(buff, nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(ch2, buff), DTAP_SUCCESS);
    DTAP_LcidBufferNode *node = DTAP_GetLcidBufferNode(0x61);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->queuedPktCnt, 2);

    DTAP_ChannelDown(0x61, 1);
    EXPECT_EQ(node->queuedPktCnt, 1);
    DTAP_ChannelDown(0x61, 2);
    EXPECT_EQ(node->queuedPktCnt, 0);

    FreeSchedChannel(ch1);
    FreeSchedChannel(ch2);
    TEST_DTAP_DliDisconnectDo(0x61);
}

TEST_F(UT_DTAP_TRANS, DtapSchedPushDupAndInitTwice)
{
    // 同链路同通道重复入队不重复建节点；初始化与反初始化重复调用幂等
    TEST_DTAP_DliConnectDo(0x71, 0);
    TEST_DTAP_SetApBufferNum(0);
    DTAP_Channel_S *chan = NewSchedChannel(0x71, 1);
    ASSERT_NE(chan, nullptr);
    NEW_BUFF(buff, 10);
    EXPECT_EQ(DTAP_DataSendWithPriority(chan, buff), DTAP_SUCCESS);
    buff = NewBuff(10);
    ASSERT_NE(buff, nullptr);
    EXPECT_EQ(DTAP_DataSendWithPriority(chan, buff), DTAP_SUCCESS);
    DTAP_LcidBufferNode *node = DTAP_GetLcidBufferNode(0x71);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->queuedPktCnt, 2);

    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);

    TEST_DTAP_DliDisconnectDo(0x71);
    FreeSchedChannel(chan);
}

TEST_F(UT_DTAP_TRANS, DtapSchedAcbNumChangeGate)
{
    // 用配额变化观测回调效果：目标类型更新配额，非目标类型不更新
    TEST_DTAP_DliConnectDo(0x81, 0);
    TEST_DTAP_SetApBufferNum(0);
    DTAP_Channel_S *chan = NewSchedChannel(0x81, 1);
    ASSERT_NE(chan, nullptr);
    NEW_BUFF(buff, 10);
    EXPECT_EQ(DTAP_DataSendWithPriority(chan, buff), DTAP_SUCCESS);
    DTAP_LcidBufferNode *node = DTAP_GetLcidBufferNode(0x81);
    ASSERT_NE(node, nullptr);

    TEST_DTAP_AcbNumChange(ACB_DATA_TYPE + 1, 4);
    EXPECT_EQ(node->quota, 1);
    TEST_DTAP_AcbNumChange(ACB_DATA_TYPE, 4);
    EXPECT_EQ(node->quota, 4);

    DTAP_ChannelDown(0x81, 1);
    TEST_DTAP_DliDisconnectDo(0x81);
    FreeSchedChannel(chan);
}

TEST_F(UT_DTAP_TRANS, DtapSchedCreatePacketAllocFail)
{
    // 待发节点分配失败：入队返回内存错误且不建账
    TEST_DTAP_DliConnectDo(0x91, 0);
    DTAP_Channel_S *chan = NewSchedChannel(0x91, 1);
    ASSERT_NE(chan, nullptr);

    NEW_BUFF(buff, 10);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail);
    EXPECT_EQ(DTAP_DataSendWithPriority(chan, buff), DTAP_TRANS_MALLOC_ERR);
    SDF_BuffFree(buff);

    TEST_DTAP_DliDisconnectDo(0x91);
    FreeSchedChannel(chan);
}

TEST_F(UT_DTAP_TRANS, DtapSchedSendInvalidParams)
{
    // 反初始化后入队按未初始化拒绝；空指针与非法优先级拒绝
    EXPECT_EQ(DTAP_SchedulerDeinit(), DTAP_SUCCESS);
    EXPECT_EQ(DTAP_DataSendWithPriority(nullptr, nullptr), DTAP_TRANS_INIT_ERR);
    EXPECT_EQ(DTAP_SchedulerInit(), DTAP_SUCCESS);

    NEW_BUFF(buff, 10);
    EXPECT_EQ(DTAP_DataSendWithPriority(nullptr, buff), DTAP_TRANS_INVALID_PARAM_ERR);
    DTAP_Channel_S chan = {};
    SDF_DListHeadInit(&chan.pktList);
    SDF_DListEntryInit(&chan.schedEntry);
    SDF_DListEntryInit(&chan.entry);
    chan.priority = DTAP_PRIORITY_MAX;
    EXPECT_EQ(DTAP_DataSendWithPriority(&chan, buff), DTAP_TRANS_INVALID_MODULE_TYPE);
    SDF_BuffFree(buff);
}
