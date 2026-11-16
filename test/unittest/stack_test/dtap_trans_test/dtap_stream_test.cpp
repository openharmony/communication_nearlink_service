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

namespace {
using namespace DtapTransTest;

DTAP_TransMode_S *StreamMode()
{
    return DTAP_GetTransMode(CM_TRANS_MODE_STREAM);
}

void RxListFree(DTAP_Stream_Channel_S &stream)
{
    while (!SDF_DListIsEmpty(&stream.rxList)) {
        SDF_DListEntry_S *node = SDF_DListFirst(&stream.rxList);
        SDF_DListElmDel(&stream.rxList, node, entry);
        FreeStreamList(node);
    }
}
}

TEST_F(UT_DTAP_TRANS, DtapStreamRecvFirstAndExpected)
{
    // 首包直接回调并推进期待序号；期待序号命中的包回调后继续消化链表
    DTAP_Channel_S chan;
    DTAP_Stream_Channel_S stream;
    InitStreamChannel(chan, stream);

    NEW_FRAME(first, DTAP_FRAME_SIMPLEX_FRAG, 0, 8);
    EXPECT_EQ(StreamMode()->recvFrame(&chan, first, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 1);
    EXPECT_EQ(stream.expectedSeq, 1);
    DTAP_DestroyFrame(first);

    // 期待序号命中的包同样直接回调
    NEW_FRAME(second, DTAP_FRAME_SIMPLEX_FRAG, 1, 8);
    EXPECT_EQ(StreamMode()->recvFrame(&chan, second, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 2);
    DTAP_DestroyFrame(second);

    // 接收回调为空：链表为空的直接回调路径不解引用空指针
    NEW_FRAME(third, DTAP_FRAME_SIMPLEX_FRAG, 2, 8);
    EXPECT_EQ(StreamMode()->recvFrame(&chan, third, nullptr), DTAP_SUCCESS);
    DTAP_DestroyFrame(third);
}

TEST_F(UT_DTAP_TRANS, DtapStreamRecvOutOfOrderAndDuplicate)
{
    // 乱序包插入链表；重复与过期序号被拒绝；缓存满后拒收
    DTAP_Channel_S chan;
    DTAP_Stream_Channel_S stream;
    InitStreamChannel(chan, stream);

    // 首包建立期待序号1
    NEW_FRAME(first, DTAP_FRAME_SIMPLEX_FRAG, 0, 8);
    EXPECT_EQ(StreamMode()->recvFrame(&chan, first, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    DTAP_DestroyFrame(first);

    // 乱序包3、2先后缓存，链表按序号升序排列
    for (uint16_t seq = 3; seq >= 2; seq--) {
        NEW_FRAME(gap, DTAP_FRAME_SIMPLEX_FRAG, seq, 8);
        EXPECT_EQ(StreamMode()->recvFrame(&chan, gap, TEST_DTAP_RecvCb), DTAP_SUCCESS);
        DTAP_DestroyFrame(gap);
    }
    EXPECT_EQ(SDF_DListCount(&stream.rxList), 2);

    // 链表内已有序号2：重复拒绝
    NEW_FRAME(dup, DTAP_FRAME_SIMPLEX_FRAG, 2, 8);
    EXPECT_EQ(StreamMode()->recvFrame(&chan, dup, TEST_DTAP_RecvCb),
        DTAP_TRANS_STREAM_RECV_DUPLICATED_ERR);
    DTAP_DestroyFrame(dup);

    // 期待序号之前的过期包：重复拒绝
    NEW_FRAME(stale, DTAP_FRAME_SIMPLEX_FRAG, 0, 8);
    EXPECT_EQ(StreamMode()->recvFrame(&chan, stale, TEST_DTAP_RecvCb),
        DTAP_TRANS_STREAM_RECV_DUPLICATED_ERR);
    DTAP_DestroyFrame(stale);

    // 期待包命中后链表中的连续帧一并补交，乱序窗口关闭
    TEST_DTAP_SentFramesReset();
    NEW_FRAME(hit, DTAP_FRAME_SIMPLEX_FRAG, 1, 8);
    EXPECT_EQ(StreamMode()->recvFrame(&chan, hit, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 4);
    EXPECT_EQ(SDF_DListCount(&stream.rxList), 0);
    DTAP_DestroyFrame(hit);
}

TEST_F(UT_DTAP_TRANS, DtapStreamRecvCacheFullAndCrcFail)
{
    // 缓存到第257个乱序包时拒收；校验失败的包拒收
    DTAP_Channel_S chan;
    DTAP_Stream_Channel_S stream;
    InitStreamChannel(chan, stream);

    NEW_FRAME(first, DTAP_FRAME_SIMPLEX_FRAG, 0, 8);
    EXPECT_EQ(StreamMode()->recvFrame(&chan, first, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    DTAP_DestroyFrame(first);

    // 乱序包填满缓存上限：链表存257个后第258个被拒
    for (uint16_t seq = 2; seq <= 259; seq++) {
        NEW_FRAME(gap, DTAP_FRAME_SIMPLEX_FRAG, seq, 8);
        uint32_t ret = StreamMode()->recvFrame(&chan, gap, TEST_DTAP_RecvCb);
        if (seq <= 258) {
            EXPECT_EQ(ret, DTAP_SUCCESS);
        } else {
            EXPECT_EQ(ret, DTAP_TRANS_STREAM_RX_FULL);
        }
        DTAP_DestroyFrame(gap);
    }

    // 非未分片类型直接拒绝
    NEW_FRAME(frag, DTAP_FRAME_SIMPLEX_FRAG, 300, 8, 0xFFFF, DTAP_SAR_FIRST);
    EXPECT_EQ(StreamMode()->recvFrame(&chan, frag, TEST_DTAP_RecvCb), DTAP_TRANS_UNSUPPORTED);
    DTAP_DestroyFrame(frag);

    // 校验种子被篡改后校验失败
    NEW_FRAME(badCrc, DTAP_FRAME_SIMPLEX_FRAG, 300, 8);
    stream.crcInit = TEST_CRC_INIT + 1;
    EXPECT_EQ(StreamMode()->recvFrame(&chan, badCrc, TEST_DTAP_RecvCb),
        DTAP_TRANS_STREAM_RECV_CRC_ERR);
    DTAP_DestroyFrame(badCrc);
    RxListFree(stream);
}

TEST_F(UT_DTAP_TRANS, DtapStreamReorderTaskFlush)
{
    // 缓存乱序帧2、3后白盒抬水位线到3：超时任务补交两帧并清空链表
    CM_TransChannelStateList_S state = {0};
    ChannelInject(state, 2, TCID_SLE_CUTC, TCID_SLE_CUTC, CM_TRANS_MODE_STREAM);
    DTAP_Channel_S *chan = DTAP_ChannelSearch(2, TCID_SLE_CUTC);
    ASSERT_NE(chan, nullptr);
    auto *stream = (DTAP_Stream_Channel_S *)chan->attr;

    // 首包建立期待序号1，乱序包2、3进入缓存
    CP_TimerSetExecCallbackPostponed(true);
    for (uint16_t seq = 0; seq <= 3; seq++) {
        if (seq == 1) {
            continue;
        }
        NEW_FRAME(frame, DTAP_FRAME_SIMPLEX_FRAG, seq, 8);
        EXPECT_EQ(StreamMode()->recvFrame(chan, frame, TEST_DTAP_RecvCb), DTAP_SUCCESS);
        DTAP_DestroyFrame(frame);
    }
    EXPECT_EQ(SDF_DListCount(&stream->rxList), 2);
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 1);

    // 白盒把重排水位线抬到3：超时任务冲刷序号2、3的缓存帧并补交
    stream->reorderTxSeq = 3;
    CP_TimerPostponedTimeout();
    EXPECT_EQ(TEST_DTAP_RecvCbCnt(), 3);
    EXPECT_EQ(stream->expectedSeq, 4);
    EXPECT_EQ(SDF_DListCount(&stream->rxList), 0);

    // 通道已删除：延后槽仍保留重排序回调，此时触发走安全返回分支
    ChannelRelease(state);
    CP_TimerPostponedTimeout();
    CP_TimerSetExecCallbackPostponed(false);
}

TEST_F(UT_DTAP_TRANS, DtapStreamReorderTimerNoNode)
{
    // 链表非空但时间戳都晚于当前时钟：重排序任务按剩余帧推进
    CM_TransChannelStateList_S state = {0};
    ChannelInject(state, 3, TCID_SLE_CUTC, TCID_SLE_CUTC, CM_TRANS_MODE_STREAM);
    DTAP_Channel_S *chan = DTAP_ChannelSearch(3, TCID_SLE_CUTC);
    ASSERT_NE(chan, nullptr);
    auto *stream = (DTAP_Stream_Channel_S *)chan->attr;

    NEW_FRAME(first, DTAP_FRAME_SIMPLEX_FRAG, 0, 8);
    EXPECT_EQ(StreamMode()->recvFrame(chan, first, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    DTAP_DestroyFrame(first);
    CP_TimerSetExecCallbackPostponed(true);
    NEW_FRAME(gap, DTAP_FRAME_SIMPLEX_FRAG, 5, 8);
    EXPECT_EQ(StreamMode()->recvFrame(chan, gap, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    DTAP_DestroyFrame(gap);

    // 缓存节点时间戳与当前时钟相同：超时任务不冲刷新帧，链表保持
    TEST_DTAP_SetMonoTimeMs(0);
    CP_TimerPostponedTimeout();
    CP_TimerSetExecCallbackPostponed(false);
    EXPECT_EQ(SDF_DListCount(&stream->rxList), 1);
    ChannelRelease(state);
}

TEST_F(UT_DTAP_TRANS, DtapStreamSendBroadcastAndMtu)
{
    // 广播帧头通道号取本端源通道号；载荷超MTU返回不支持
    DTAP_Channel_S chan;
    DTAP_Stream_Channel_S stream;
    InitStreamChannel(chan, stream);
    TEST_DTAP_DliConnectDo(chan.lcid, 0);

    chan.castMode = DTAP_FRAME_BROADCAST;
    NEW_BUFF(buff, 10);
    EXPECT_EQ(StreamMode()->sendFrame(&chan, 0, buff), DTAP_SUCCESS);

    // 载荷超过MTU：不支持
    chan.mtu = 4;
    NEW_BUFF(big, 10);
    EXPECT_EQ(StreamMode()->sendFrame(&chan, 0, big), DTAP_TRANS_UNSUPPORTED);
    SDF_BuffFree(big);

    // 单播正常发送
    chan.castMode = DTAP_FRAME_UNICAST;
    chan.mtu = UINT16_MAX;
    NEW_BUFF(uni, 10);
    EXPECT_EQ(StreamMode()->sendFrame(&chan, 0, uni), DTAP_SUCCESS);
    DTAP_ChannelDown(chan.lcid, chan.srcTcid);
}

TEST_F(UT_DTAP_TRANS, DtapStreamRecvNodeAllocFail)
{
    // 首次节点分配失败：乱序包按内存错误返回
    DTAP_Channel_S chan;
    DTAP_Stream_Channel_S stream;
    InitStreamChannel(chan, stream);

    NEW_FRAME(first, DTAP_FRAME_SIMPLEX_FRAG, 0, 8);
    EXPECT_EQ(StreamMode()->recvFrame(&chan, first, TEST_DTAP_RecvCb), DTAP_SUCCESS);
    DTAP_DestroyFrame(first);

    NEW_FRAME(gap, DTAP_FRAME_SIMPLEX_FRAG, 3, 8);
    EXPECT_CALL(sdfMemMock, SDF_MemZalloc).WillOnce(TEST_SDF_MemZallocFail);
    EXPECT_EQ(StreamMode()->recvFrame(&chan, gap, TEST_DTAP_RecvCb), DTAP_TRANS_MALLOC_ERR);
    DTAP_DestroyFrame(gap);
}

TEST_F(UT_DTAP_TRANS, DtapStreamCheckFrameType)
{
    // 双向帧与基础帧拒绝，单向两种帧放行
    NEW_FRAME(duplex, DTAP_FRAME_DUPLEX_FRAG, 0, 4, 0);
    EXPECT_FALSE(StreamMode()->checkFrame(nullptr, duplex));
    DTAP_DestroyFrame(duplex);

    NEW_FRAME(simplex, DTAP_FRAME_SIMPLEX_FRAG, 0, 4);
    EXPECT_TRUE(StreamMode()->checkFrame(nullptr, simplex));
    DTAP_DestroyFrame(simplex);
}
