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

#include "byte_codec.h"
#include "cm_trans_channel_mgr.h"
#include "dli_callback.h"
#include "dli_errno.h"
#include "sle_logic_link_mgr.h"

namespace {
using namespace DtapTransTest;

DTAP_TransMode_S *ReliableMode()
{
    return DTAP_GetTransMode(CM_TRANS_MODE_RELIABLE);
}

uint16_t AckReqSeqOf(size_t idx)
{
    uint8_t lo = TEST_DTAP_SentFrameByte(idx, 4);
    uint8_t hi = TEST_DTAP_SentFrameByte(idx, 5);
    return (uint16_t)(lo | ((hi & 0x3F) << 8));
}

DTAP_BasicHeader_S *FrameHeaderOf(DTAP_Frame_S *frame)
{
    return (DTAP_BasicHeader_S *)SDF_DataOffset(frame->buff);
}

uint32_t TxListCount(DTAP_ReliableChannel_S &reliable)
{
    return SDF_DListCount(&reliable.txWindow.txList);
}

void RxListClear(DTAP_ReliableChannel_S &reliable)
{
    DTAP_Frame_S *node = nullptr;
    DTAP_Frame_S *tmp = nullptr;
    SDF_DListElmAllFree(node, tmp, &reliable.rxWindow.rxList, entry, DTAP_DestroyFrame);
}
}

TEST_F(UT_DTAP_TRANS, DtapReliableSendFragmentAndPolling)
{
    // 窗口8片载荷80字节切4片：全部入队成功，序列号连续推进
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 8, 20);
    chan.mps = DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN + 20;

    NEW_BUFF(buff, 80);
    EXPECT_EQ(ReliableMode()->sendFrame(&chan, 0, buff), DTAP_SUCCESS);
    EXPECT_EQ(TxListCount(reliable), 4);

    DTAP_Frame_S *node = nullptr;
    uint16_t seq = 0;
    SDF_DListElmForeach(node, &reliable.txWindow.txList, entry) {
        EXPECT_EQ(node->enhance.txSeq, seq);
        seq++;
    }
    EXPECT_EQ(reliable.txWindow.nextTxSeq, 4);
    EXPECT_EQ(reliable.reTxSeq, 4);

    // 计数达到窗口四分之一的那一片带轮询位，其余片不带
    uint8_t idx = 0;
    SDF_DListElmForeach(node, &reliable.txWindow.txList, entry) {
        if (idx == 2) {
            EXPECT_EQ(FrameHeaderOf(node)->pBit, 1);
        } else {
            EXPECT_EQ(FrameHeaderOf(node)->pBit, 0);
        }
        idx++;
    }
    TxListClear(reliable);
}

TEST_F(UT_DTAP_TRANS, DtapReliableSendWindowJustFull)
{
    // 2片载荷装满窗口2：按窗口满返回且帧仍在队列
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 2, 20);
    chan.mps = DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN + 10;

    NEW_BUFF(buff, 20);
    EXPECT_EQ(ReliableMode()->sendFrame(&chan, 0, buff), DTAP_TRANS_RELIABLE_TX_WINDOW_FULL);
    EXPECT_EQ(TxListCount(reliable), 2);
    EXPECT_TRUE(reliable.isTxWindowFull);
    SDF_BuffFree(buff);
    TxListClear(reliable);
}

TEST_F(UT_DTAP_TRANS, DtapReliableSendCacheWhenExceed)
{
    // 窗口1装不下2片的包：第一次缓存，第二次仍复用同一缓存
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 1, 20);
    chan.mps = DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN + 10;

    NEW_BUFF(buff, 20);
    EXPECT_EQ(ReliableMode()->sendFrame(&chan, 0, buff), DTAP_TRANS_RELIABLE_TX_WINDOW_FULL);
    ASSERT_NE(reliable.cacheBuff, nullptr);
    SDF_Buff_S *firstCache = reliable.cacheBuff;

    NEW_BUFF(buff2, 20);
    EXPECT_EQ(ReliableMode()->sendFrame(&chan, 0, buff2), DTAP_TRANS_RELIABLE_TX_WINDOW_FULL);
    EXPECT_EQ(reliable.cacheBuff, firstCache);

    SDF_BuffFree(buff);
    SDF_BuffFree(buff2);
    TxListClear(reliable);
    SDF_BuffFree(reliable.cacheBuff);
    reliable.cacheBuff = nullptr;
}

TEST_F(UT_DTAP_TRANS, DtapReliableSendInvalidMpsAndOverLimit)
{
    // mps不足以容纳头与校验时拒绝；有效载荷1字节时129字节载荷需129片超上限
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 8, 20);
    chan.mps = DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN;
    NEW_BUFF(buff, 10);
    EXPECT_EQ(ReliableMode()->sendFrame(&chan, 0, buff), DTAP_TRANS_RELIABLE_INVALID_MPS);

    chan.mps = DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN + 1;
    NEW_BUFF(big, 129);
    EXPECT_EQ(ReliableMode()->sendFrame(&chan, 0, big), DTAP_TRANS_RELIABLE_INVALID_FRAME_NUM);
    SDF_BuffFree(buff);
    SDF_BuffFree(big);
}

TEST_F(UT_DTAP_TRANS, DtapReliableSendFragmentAllocFail)
{
    // 第一次帧对象分配即失败：分片数为零路径返回分片错误
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 8, 20);
    chan.mps = DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN + 10;

    NEW_BUFF(buff, 30);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail);
    EXPECT_EQ(ReliableMode()->sendFrame(&chan, 0, buff), DTAP_TRANS_RELIABLE_FRAGMENT_ERR);
    SDF_BuffFree(buff);
    TxListClear(reliable);
}

TEST_F(UT_DTAP_TRANS, DtapReliableSendRetransOldFirst)
{
    // 重传序列号落后于下一发送序列号：本次发送先补传旧帧再发新帧
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 8, 20);
    chan.mps = DTAP_DUPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN + 10;
    reliable.isRecvFrame = true;
    NEW_FRAME(old, DTAP_FRAME_DUPLEX_FRAG, 0, 10);
    TxInsert(reliable, old);
    reliable.txWindow.nextTxSeq = 1;
    reliable.reTxSeq = 0;

    NEW_BUFF(buff, 10);
    EXPECT_EQ(ReliableMode()->sendFrame(&chan, 0, buff), DTAP_SUCCESS);
    EXPECT_EQ(old->enhance.reTxCnt, 1);
    EXPECT_EQ(TxListCount(reliable), 2);
    EXPECT_EQ(reliable.reTxSeq, reliable.txWindow.nextTxSeq);
    TxListClear(reliable);
    DTAP_ChannelDown(chan.lcid, chan.srcTcid);
}

TEST_F(UT_DTAP_TRANS, DtapReliableRetransReachMax)
{
    // 旧帧重传计数已达上限：发送入口直接按达到上限返回
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 8, 2);
    chan.mps = DTAP_DUPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN + 10;
    reliable.isRecvFrame = true;
    NEW_FRAME(old, DTAP_FRAME_DUPLEX_FRAG, 0, 10);
    old->enhance.reTxCnt = 2;
    TxInsert(reliable, old);
    reliable.txWindow.nextTxSeq = 1;
    reliable.reTxSeq = 0;

    NEW_BUFF(buff, 10);
    EXPECT_EQ(ReliableMode()->sendFrame(&chan, 0, buff), DTAP_TRANS_RELIABLE_REACH_MAX_RE_TX_CNT_ERR);
    EXPECT_FALSE(reliable.isDestroyMsgSent);
    SDF_BuffFree(buff);

    // 建立真实逻辑链路与动态通道节点后拆链请求可找到目标，标志只置一次
    SLE_Addr_S addr = {0};
    SleLogicLink_S *link = SleLogicLinkAdd(&addr);
    ASSERT_NE(link, nullptr);
    link->lcid = chan.lcid;
    CM_DynTransChanParam_S dynParam = {0};
    dynParam.lcid = chan.lcid;
    dynParam.localTcid = chan.srcTcid;
    dynParam.remoteTcid = chan.dstTcid;
    ASSERT_NE(CM_ActivateDynamicTransChannel(chan.lcid, &dynParam), nullptr);

    buff = NewBuff(10);
    ASSERT_NE(buff, nullptr);
    EXPECT_EQ(ReliableMode()->sendFrame(&chan, 0, buff), DTAP_TRANS_RELIABLE_REACH_MAX_RE_TX_CNT_ERR);
    EXPECT_TRUE(reliable.isDestroyMsgSent);
    SDF_BuffFree(buff);
    TxListClear(reliable);
    CM_ReleaseFixedTransChannel(chan.lcid);
    SleLogicLinkRemove(link);
}

TEST_F(UT_DTAP_TRANS, DtapReliableNackRetrans)
{
    // 异常应答指定序列号：补传对应帧；计数到上限时返回达到上限错误
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 8, 20);
    chan.mps = DTAP_DUPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN + 10;
    reliable.isRecvFrame = true;
    NEW_FRAME(sent, DTAP_FRAME_DUPLEX_FRAG, 0, 10);
    TxInsert(reliable, sent);
    reliable.txWindow.nextTxSeq = 1;

    DTAP_Frame_S *nack = MakeAckFrame(0, true);
    ASSERT_NE(nack, nullptr);
    EXPECT_EQ(ReliableMode()->recvFrame(&chan, nack, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(sent->enhance.reTxCnt, 1);
    DTAP_DestroyFrame(nack);

    sent->enhance.reTxCnt = 20;
    reliable.maxTxThreshold = 20;
    nack = MakeAckFrame(0, true);
    ASSERT_NE(nack, nullptr);
    EXPECT_EQ(ReliableMode()->recvFrame(&chan, nack, TEST_DTAP_RecvCb),
        DTAP_TRANS_RELIABLE_REACH_MAX_RE_TX_CNT_ERR);
    DTAP_DestroyFrame(nack);
    TxListClear(reliable);
}

TEST_F(UT_DTAP_TRANS, DtapReliableRetransPbitSkipAndEmptyGuard)
{
    // 旧帧已带轮询位：补传时不重复置位也不重算校验
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 8, 20);
    chan.mps = DTAP_DUPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN + 10;
    reliable.isRecvFrame = true;
    NEW_FRAME(polled, DTAP_FRAME_DUPLEX_FRAG, 0, 10, 0xFFFF, DTAP_SAR_UNSEG, DTAP_FRAME_P_BIT | DTAP_FRAME_C_BIT);
    TxInsert(reliable, polled);
    reliable.txWindow.nextTxSeq = 1;
    reliable.reTxSeq = 0;

    NEW_BUFF(buff, 10);
    EXPECT_EQ(ReliableMode()->sendFrame(&chan, 0, buff), DTAP_SUCCESS);
    EXPECT_EQ(polled->enhance.reTxCnt, 1);
    SDF_BuffFree(buff);

    TxListClear(reliable);
    DTAP_ChannelDown(chan.lcid, chan.srcTcid);
}

TEST_F(UT_DTAP_TRANS, DtapReliableSlideTxWindowByAck)
{
    // 初始序列号应答不滑动；部分确认滑出旧帧；全部确认清队停表
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 8, 20);
    chan.mps = DTAP_DUPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN + 10;
    for (uint16_t seq = 0; seq < 3; seq++) {
        NEW_FRAME(frame, DTAP_FRAME_DUPLEX_FRAG, seq, 10);
        TxInsert(reliable, frame);
    }
    reliable.txWindow.nextTxSeq = 3;
    reliable.reTxSeq = 0;

    DTAP_Frame_S *ackInit = MakeAckFrame(DTAP_FRAME_SEQ_INIT, false);
    ASSERT_NE(ackInit, nullptr);
    ReliableMode()->recvFrame(&chan, ackInit, TEST_DTAP_RecvCb);
    EXPECT_EQ(TxListCount(reliable), 3);
    DTAP_DestroyFrame(ackInit);

    reliable.retransTimer = 5;
    DTAP_Frame_S *ackOne = MakeAckFrame(1, false);
    ASSERT_NE(ackOne, nullptr);
    EXPECT_EQ(ReliableMode()->recvFrame(&chan, ackOne, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(TxListCount(reliable), 2);
    EXPECT_EQ(reliable.txWindow.expectedAckSeq, 1);
    EXPECT_EQ(reliable.reTxSeq, 1);
    DTAP_DestroyFrame(ackOne);

    reliable.retransTimer = 5;
    DTAP_Frame_S *ackAll = MakeAckFrame(3, false);
    ASSERT_NE(ackAll, nullptr);
    EXPECT_EQ(ReliableMode()->recvFrame(&chan, ackAll, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(TxListCount(reliable), 0);
    EXPECT_EQ(reliable.retransTimer, INVALID_TIMER_HANDLE);
    DTAP_DestroyFrame(ackAll);
}

TEST_F(UT_DTAP_TRANS, DtapReliableCheckTxSeqMatrix)
{
    // 构造不同序列号位置的帧逐个过校验，覆盖全部判定分支
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 4, 20);

    NEW_FRAME(stale, DTAP_FRAME_SIMPLEX_FRAG, 5, 1);
    reliable.rxWindow.expectedTxSeq = 6;
    EXPECT_FALSE(ReliableMode()->checkFrame(&chan, stale));
    DTAP_DestroyFrame(stale);

    reliable.rxWindow.expectedTxSeq = 0;
    reliable.rxWindow.bufferSeq = 0;
    NEW_FRAME(beyond, DTAP_FRAME_SIMPLEX_FRAG, 5, 1);
    EXPECT_FALSE(ReliableMode()->checkFrame(&chan, beyond));
    DTAP_DestroyFrame(beyond);

    NEW_FRAME(fresh, DTAP_FRAME_SIMPLEX_FRAG, 2, 1);
    EXPECT_TRUE(ReliableMode()->checkFrame(&chan, fresh));

    // 序号已入接收链表：按重复拒绝
    SDF_DListElmTailInsert(&reliable.rxWindow.rxList, fresh, entry);
    EXPECT_FALSE(ReliableMode()->checkFrame(&chan, fresh));
    RxListClear(reliable);

    DTAP_Frame_S *ack = MakeAckFrame(0, false);
    ASSERT_NE(ack, nullptr);
    EXPECT_TRUE(ReliableMode()->checkFrame(&chan, ack));
    DTAP_DestroyFrame(ack);
}

TEST_F(UT_DTAP_TRANS, DtapReliableRecvInOrderAndOutOfOrder)
{
    // 先收带轮询位的按序帧：递交上层并发应答；再连续收乱序帧触发重排序保护
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 4, 20);
    TEST_DTAP_DliConnectDo(chan.lcid, 0);
    TEST_DTAP_AcbNumChange(ACB_DATA_TYPE, 8);

    NEW_FRAME(inOrder, DTAP_FRAME_DUPLEX_FRAG, 0, 8, 0, DTAP_SAR_UNSEG, DTAP_FRAME_P_BIT | DTAP_FRAME_C_BIT);
    EXPECT_EQ(ReliableMode()->recvFrame(&chan, inOrder, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 1);
    EXPECT_EQ(reliable.rxWindow.bufferSeq, 1);
    EXPECT_TRUE(reliable.isRecvFrame);
    ASSERT_GE(TEST_DTAP_SentFrameCnt(), 1U);
    EXPECT_EQ(TEST_DTAP_SentFrameType(0), DTAP_FRAME_ACK);
    EXPECT_EQ(AckReqSeqOf(0), 1);
    DTAP_DestroyFrame(inOrder);

    // 每收一个乱序帧重排序定时器是最后一次登记，到期即发异常应答并计数
    CP_TimerSetExecCallbackPostponed(true);
    for (uint16_t seq = 3; seq < 3 + reliable.maxTxThreshold + 1; seq++) {
        NEW_FRAME(gap, DTAP_FRAME_SIMPLEX_FRAG, seq, 4);
        EXPECT_EQ(ReliableMode()->recvFrame(&chan, gap, TEST_DTAP_RecvCb), DTAP_SUCCESS);
        DTAP_DestroyFrame(gap);
        CP_TimerPostponedTimeout();
    }
    CP_TimerSetExecCallbackPostponed(false);
    EXPECT_GE(reliable.nackCnt, reliable.maxTxThreshold);
    RxListClear(reliable);
}

TEST_F(UT_DTAP_TRANS, DtapReliableRecvChannelFullAndResume)
{
    // 满位期间新帧只入链表不递交；恢复后续交首帧，队列彻底空闲后全部消化
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 4, 20);

    TEST_DTAP_RecvCbReset(DTAP_TRANS_RELIABLE_TRANS_CHANNEL_FULL);
    NEW_FRAME(frame, DTAP_FRAME_DUPLEX_FRAG, 0, 8, 0);
    EXPECT_EQ(ReliableMode()->recvFrame(&chan, frame, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 1);
    EXPECT_TRUE(reliable.isRxWindowFull);
    EXPECT_EQ(SDF_DListCount(&reliable.rxWindow.rxList), 1);

    NEW_FRAME(frame2, DTAP_FRAME_DUPLEX_FRAG, 1, 8, 0);
    ReliableMode()->recvFrame(&chan, frame2, TEST_DTAP_RecvCb);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 1);
    EXPECT_EQ(SDF_DListCount(&reliable.rxWindow.rxList), 2);
    DTAP_DestroyFrame(frame2);

    // 恢复信号重置满位：首帧重新递交仍报满，停留在链表头
    ReliableMode()->setTransChannelStatus(&chan, DTAP_SUCCESS, TEST_DTAP_RecvCb);
    EXPECT_TRUE(reliable.isRxWindowFull);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 2);
    EXPECT_EQ(SDF_DListCount(&reliable.rxWindow.rxList), 2);

    // 上层恢复接收后两帧依次消化，缓存下界推进到2
    TEST_DTAP_RecvCbReset(0);
    ReliableMode()->setTransChannelStatus(&chan, DTAP_SUCCESS, TEST_DTAP_RecvCb);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 2);
    EXPECT_EQ(SDF_DListCount(&reliable.rxWindow.rxList), 0);
    EXPECT_EQ(reliable.rxWindow.bufferSeq, 2);
    DTAP_DestroyFrame(frame);
}

TEST_F(UT_DTAP_TRANS, DtapReliableReorderTimerFlow)
{
    // 白盒置表句柄后分别驱动重启与停止两条路径
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 8, 20);

    NEW_FRAME(gap, DTAP_FRAME_SIMPLEX_FRAG, 2, 4);
    ReliableMode()->recvFrame(&chan, gap, TEST_DTAP_RecvCb);
    DTAP_DestroyFrame(gap);

    // 句柄非空且缺口仍在：走重启路径
    reliable.reorderTimer = 5;
    gap = MakeDataFrame(DTAP_FRAME_SIMPLEX_FRAG, 3, 4);
    ASSERT_NE(gap, nullptr);
    ReliableMode()->recvFrame(&chan, gap, TEST_DTAP_RecvCb);
    DTAP_DestroyFrame(gap);

    // 补齐全部缺口：期待序号追上上界，重排序表停止
    reliable.reorderTimer = 5;
    for (uint16_t seq = 0; seq <= 1; seq++) {
        NEW_FRAME(fill, DTAP_FRAME_SIMPLEX_FRAG, seq, 4);
        ReliableMode()->recvFrame(&chan, fill, TEST_DTAP_RecvCb);
        DTAP_DestroyFrame(fill);
    }
    EXPECT_EQ(reliable.reorderTimer, INVALID_TIMER_HANDLE);
    EXPECT_EQ(SDF_DListCount(&reliable.rxWindow.rxList), 0);
}

TEST_F(UT_DTAP_TRANS, DtapReliableRecvAggregateSduWalk)
{
    // 手工排布SDU数组覆盖遍历过程的各判定分支
    DTAP_Frame_S frame = {0};
    uint8_t payload[16] = {0};
    DTAP_Data_Info_S info = {0};

    frame.payload = payload;
    frame.payloadLen = 1;
    EXPECT_EQ(DTAP_RecvAggregateFrame(&frame, &info, TEST_DTAP_RecvCb),
        DTAP_ENHANCED_FRAME_SDU_HDR_LEN_TOO_SHORT);

    payload[0] = 10;
    payload[1] = 0;
    frame.payloadLen = sizeof(DTAP_AggregateSdu_S) + 1;
    EXPECT_EQ(DTAP_RecvAggregateFrame(&frame, &info, TEST_DTAP_RecvCb),
        DTAP_ENHANCED_FRAME_SDU_LEN_TOO_SHORT);

    payload[0] = 0;
    payload[1] = 0;
    EXPECT_EQ(DTAP_RecvAggregateFrame(&frame, &info, TEST_DTAP_RecvCb),
        DTAP_ENHANCED_FRAME_SDU_LEN_TOO_SHORT);

    // 两个合法SDU全部递交
    payload[0] = 2;
    payload[1] = 0;
    payload[4] = 2;
    payload[5] = 0;
    frame.payload = payload;
    frame.payloadLen = 2 * sizeof(DTAP_AggregateSdu_S) + 2 + 2;
    TEST_DTAP_RecvCbReset(0);
    EXPECT_EQ(DTAP_RecvAggregateFrame(&frame, &info, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 2);

    // 第二个SDU递交时上层报满：剩余SDU保留在帧内并原样返回
    frame.payload = payload;
    frame.payloadLen = 2 * sizeof(DTAP_AggregateSdu_S) + 2 + 2;
    TEST_DTAP_RecvCbReset(DTAP_TRANS_RELIABLE_TRANS_CHANNEL_FULL);
    EXPECT_EQ(DTAP_RecvAggregateFrame(&frame, &info, TEST_DTAP_RecvCb),
        DTAP_TRANS_RELIABLE_TRANS_CHANNEL_FULL);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 1);
    EXPECT_EQ(frame.payloadLen, 2 * sizeof(DTAP_AggregateSdu_S) + 2);
}

TEST_F(UT_DTAP_TRANS, DtapReliableRecvFragmentReassemble)
{
    // 扩展头声明17个扩展字段超过上限；空帧拷贝保持字段一致
    DTAP_Frame_S frame = {0};
    NEW_BUFF(buff, 32);
    frame.buff = buff;
    frame.header = SDF_DataOffset(buff);
    DTAP_ExtensionHeader_S *ext = (DTAP_ExtensionHeader_S *)frame.header;
    ext->num = 17;
    frame.headerLen = 0;
    EXPECT_EQ(DTAP_ParseExtension(&frame), DTAP_FRAME_INVALID_EXT_NUM);
    SDF_BuffFree(buff);

    DTAP_Frame_S src = {0};
    DTAP_Frame_S *copy = DTAP_CopyFrame(&src);
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->buff, nullptr);
    DTAP_DestroyFrame(copy);

    // 每个阶段用新缓冲驱动一遍状态机
    DTAP_Data_Info_S info = {0};
    SDF_Buff_S *frags = nullptr;

    EXPECT_EQ(DTAP_RecvFragmentFrame(&frags, nullptr, &info, TEST_DTAP_RecvCb),
        DTAP_ENHANCED_FRAME_NULL);

    // 未见首片时中间片与末片都丢弃
    DTAP_Frame_S mid = {0};
    mid.enhance.sar = DTAP_SAR_MID;
    mid.buff = NewBuff(8);
    ASSERT_NE(mid.buff, nullptr);
    EXPECT_EQ(DTAP_RecvFragmentFrame(&frags, &mid, &info, TEST_DTAP_RecvCb),
        DTAP_ENHANCED_FRAME_NO_FIRST_FRAG_FRAME);
    SDF_BuffFree(mid.buff);
    DTAP_Frame_S tail = {0};
    tail.enhance.sar = DTAP_SAR_LAST;
    tail.buff = NewBuff(8);
    ASSERT_NE(tail.buff, nullptr);
    EXPECT_EQ(DTAP_RecvFragmentFrame(&frags, &tail, &info, TEST_DTAP_RecvCb),
        DTAP_ENHANCED_FRAME_NO_FIRST_FRAG_FRAME);
    SDF_BuffFree(tail.buff);

    // 首片缓存数据，重复首片覆盖旧缓存
    DTAP_Frame_S first = {0};
    first.enhance.sar = DTAP_SAR_FIRST;
    first.buff = NewBuff(8);
    ASSERT_NE(first.buff, nullptr);
    TEST_DTAP_RecvCbReset(0);
    EXPECT_EQ(DTAP_RecvFragmentFrame(&frags, &first, &info, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    ASSERT_NE(frags, nullptr);
    SDF_Buff_S *firstBuff = first.buff;
    first.buff = NewBuff(8);
    ASSERT_NE(first.buff, nullptr);
    EXPECT_EQ(DTAP_RecvFragmentFrame(&frags, &first, &info, TEST_DTAP_RecvCb),
        DTAP_ENHANCED_FRAME_DUP_FIRST_FRAG_FRAME);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 0);

    // 末片拼装完成递交上层并清空缓存
    tail.buff = NewBuff(8);
    ASSERT_NE(tail.buff, nullptr);
    EXPECT_EQ(DTAP_RecvFragmentFrame(&frags, &tail, &info, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(frags, nullptr);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 1);
    SDF_BuffFree(tail.buff);

    // 未分片帧直接递交
    DTAP_Frame_S unseg = {0};
    unseg.enhance.sar = DTAP_SAR_UNSEG;
    unseg.buff = NewBuff(8);
    ASSERT_NE(unseg.buff, nullptr);
    EXPECT_EQ(DTAP_RecvFragmentFrame(&frags, &unseg, &info, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 2);
    SDF_BuffFree(unseg.buff);

    // 拼装总长超过协议上限时丢弃缓存并返回长度错误
    first.buff = NewBuff(8);
    ASSERT_NE(first.buff, nullptr);
    EXPECT_EQ(DTAP_RecvFragmentFrame(&frags, &first, &info, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    ASSERT_NE(frags, nullptr);
    NEW_BUFF(huge, 65532);
    tail.buff = huge;
    EXPECT_EQ(DTAP_RecvFragmentFrame(&frags, &tail, &info, TEST_DTAP_RecvCb),
        DTAP_ENHANCED_FRAME_FRAGMENT_FRAME_LEN_ERR);
    EXPECT_EQ(frags, nullptr);
    SDF_BuffFree(huge);
    SDF_BuffFree(firstBuff);
    SDF_BuffFree(first.buff);
}


TEST_F(UT_DTAP_TRANS, DtapReliableTransChannelStatusFlow)
{
    // 依次驱动非可靠通道、未满窗、窗口不足、续发成功、无缓存恢复、续发失败六个分支
    DTAP_Channel_S chan;
    DTAP_ReliableChannel_S reliable;
    InitReliableChannel(chan, reliable, 4, 20);
    chan.mps = DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN + DTAP_CRC_LEN + 10;
    TEST_DTAP_RegisterStatusCbk();

    chan.mode = CM_TRANS_MODE_BASIC;
    DTAP_TransChannelStatusChange(&chan, DTAP_SUCCESS);
    chan.mode = CM_TRANS_MODE_RELIABLE;
    DTAP_TransChannelStatusChange(&chan, DTAP_SUCCESS);
    EXPECT_EQ(TEST_DTAP_StatusCnt(), 0);

    // 窗口仍装不下整包：继续等待，不上报
    reliable.isTxWindowFull = true;
    reliable.cacheBuff = NewBuff(20);
    ASSERT_NE(reliable.cacheBuff, nullptr);
    reliable.txWindow.size = 1;
    DTAP_TransChannelStatusChange(&chan, DTAP_SUCCESS);
    EXPECT_EQ(TEST_DTAP_StatusCnt(), 0);
    EXPECT_NE(reliable.cacheBuff, nullptr);

    // 窗口放行后缓存续发成功并清空缓存，结果码透传给上层
    reliable.txWindow.size = 4;
    DTAP_TransChannelStatusChange(&chan, 7);
    EXPECT_EQ(reliable.cacheBuff, nullptr);
    EXPECT_EQ(TEST_DTAP_StatusCnt(), 1);
    EXPECT_EQ(TEST_DTAP_LastStatusResult(), 7);

    // 无缓存且窗口有余量：解除满窗并上报
    reliable.isTxWindowFull = true;
    DTAP_TransChannelStatusChange(&chan, DTAP_SUCCESS);
    EXPECT_FALSE(reliable.isTxWindowFull);
    EXPECT_EQ(TEST_DTAP_StatusCnt(), 2);

    // 续发失败时丢弃缓存但同样完成状态上报
    TxListClear(reliable);
    reliable.isTxWindowFull = true;
    reliable.cacheBuff = NewBuff(20);
    ASSERT_NE(reliable.cacheBuff, nullptr);
    chan.priority = DTAP_PRIORITY_MAX;
    DTAP_TransChannelStatusChange(&chan, DTAP_SUCCESS);
    EXPECT_EQ(reliable.cacheBuff, nullptr);
    EXPECT_EQ(TEST_DTAP_StatusCnt(), 3);
    TxListClear(reliable);
    DTAP_ChannelDown(chan.lcid, chan.srcTcid);
}

