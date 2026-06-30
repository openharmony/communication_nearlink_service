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

/****************************************************************************
 *
 * this file contains dtap reliable transmission mode implement.
 *
 ***************************************************************************/

#include <stdint.h>

#include "securec.h"

#include "cm_dyn_trans_chan_state_mgr.h"
#include "crc16.h"
#include "dpfwk_log.h"
#include "dtap_errno.h"
#include "dtap_tcid.h"
#include "dtap_trans.h"
#include "dtap_frame.h"
#include "sdf_buff.h"
#include "sdf_mem.h"

#define SEND_POLLING_CNT_THRESHOLD_FACTOR 4

static uint32_t DTAP_SendAckFrame(DTAP_Channel_S *channel, bool fBit, bool sBit)
{
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)channel->attr;
    DTAP_Frame_S frame = {0};
    frame.bits = DTAP_FRAME_C_BIT;
    if (fBit) {
        frame.bits |= DTAP_FRAME_F_BIT;
    }
    frame.ack.sBit = sBit;
    // 正常应答发送时，接收端的增强帧中的ReqSeq域应设置为BufferSeq（保证两边窗口缓存报文一致），异常应答发送时，
    // 接收端的增强帧中的ReqSeq域应设置为ExpectedTxSeq
    frame.ack.reqSeq = sBit ? reliable->rxWindow.expectedTxSeq : reliable->rxWindow.bufferSeq;
    DTAP_FrameCtx_S *ctx = DTAP_GetFrameCtx(DTAP_FRAME_ACK);
    if (ctx == NULL) {
        return DTAP_TRANS_RELIABLE_GET_CTX_ERR;
    }
 
    frame.ctx = *ctx;
    uint32_t ret = ctx->buildFrame(&frame.ctx, channel->dstTcid, reliable->crcInit);
    if (ret != DTAP_SUCCESS) {
        return ret;
    }

    ret = DTAP_SendFrame(channel, frame.buff);
    if (ret != DTAP_SUCCESS) {
        SDF_BuffFree(frame.buff);
    }
    return ret;
}

void DTAP_RspTimerExpireProc(void *args)
{
    DTAP_Channel_S *channel = (DTAP_Channel_S *)args;
    if (channel == NULL || channel->attr == NULL) {
        return;
    }

    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)channel->attr;
    DTAP_LOGD("rsp timer expired, send ack frame. lcid: %hu, tcid: %hhu, bufferSeq: %hu",
        channel->lcid, channel->dstTcid, reliable->rxWindow.bufferSeq);
    uint32_t ret = DTAP_SendAckFrame(channel, false, false);
    if (ret != DTAP_SUCCESS) {
        DTAP_LOGE("send ack frame failed, ret: %08x", ret);
    }
}

static uint8_t DTAP_GetReliableModeType(void)
{
    return CM_TRANS_MODE_RELIABLE;
}

static DTAP_Frame_S *DTAP_FindFrameBySeq(const SDF_DListHead_S *head, uint16_t seq)
{
    DTAP_Frame_S *frame = NULL;
    SDF_DListElmForeach(frame, head, entry) {
        if (frame->enhance.txSeq == seq) {
            return frame;
        }
    }

    return NULL;
}

static bool DTAP_CheckEnhanceFrameTxSeq(const DTAP_RxWindow_S *window, uint16_t txSeq)
{
    if (DTAP_IsFrameSeqSmaller(txSeq, window->expectedTxSeq)) {
        return false;
    }

    if (txSeq == window->expectedTxSeq) {
        return true;
    }

    uint16_t boundarySeq = ((window->bufferSeq + window->size) & DTAP_FRAME_SEQ_MAX);
    if (!DTAP_IsFrameSeqSmaller(txSeq, boundarySeq)) {
        return false;
    }

    if (DTAP_FindFrameBySeq(&window->rxList, txSeq) == NULL) {
        return true;
    }

    return false;
}

static bool DTAP_CheckEnhanceFrame(const DTAP_Channel_S *transChan, DTAP_Frame_S *dtapFrame)
{
    (void)transChan;
    DTAP_ReliableChannel_S *channel = (DTAP_ReliableChannel_S *)transChan->attr;

    // 检查帧类型
    uint8_t frameType = dtapFrame->ctx.getFrameType();
    if (!(frameType >= DTAP_FRAME_SIMPLEX_AGGR && frameType <= DTAP_FRAME_ACK)) {
        DTAP_LOGE("invalid frame type: %hhu", frameType);
        return false;
    }

    // crc校验
    if (CRC16(channel->crcInit, SDF_DataOffset(dtapFrame->buff), SDF_DataLenGet(dtapFrame->buff)) != 0) {
        DTAP_LOGE("crc calculate error");
        return false;
    }

    if (frameType == DTAP_FRAME_ACK) {
        return true;
    }

    // TxSeq校验
    if (!DTAP_CheckEnhanceFrameTxSeq(&channel->rxWindow, dtapFrame->enhance.txSeq)) {
        DTAP_LOGE("check tx seq error. txSeq: %hu, bufferSeq : %hu, expectedTxSeq: %hu", dtapFrame->enhance.txSeq,
            channel->rxWindow.bufferSeq, channel->rxWindow.expectedTxSeq);
        return false;
    }
    return true;
}

static void DTAP_DynTransChannReleaseReq(DTAP_Channel_S *channel)
{
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)channel->attr;
    if (reliable->isDestroyMsgSent) {
        DTAP_LOGI("Destroy msg is already sent, tcid: %hu", channel->dstTcid);
        return;
    }
    CM_DynTransChannelReleaseParamReq_S req = {
        .version = 0,
        .localIndex = 0,
        .srcTcid = channel->srcTcid,
        .dstTcid = channel->dstTcid,
    };
    SleTransLcid_S *node = CM_FindTransChannelByLocalTcid(channel->lcid, channel->srcTcid);
    if (node == NULL) {
        DTAP_LOGE("find trans chann by lcid 0x%04x, srcTcid:0x%02x failed.", channel->lcid, channel->srcTcid);
        return;
    }
    (void)memcpy_s(&req.addr, sizeof(SLE_Addr_S), &node->addr, sizeof(SLE_Addr_S));

    DTAP_LOGI("start to destroy tranport channel, addr: %s, src tcid: %hu", GET_ENC_ADDR(&req.addr), req.srcTcid);
    uint32_t ret = CM_DynTransChannelReleaseReq(&req);
    reliable->isDestroyMsgSent = true;
    if (ret == DTAP_SUCCESS) {
        DTAP_LOGI("Successfully send destroy tranport channel req, src tcid: %hu", req.srcTcid);
    } else {
        DTAP_LOGE("Failed to send destroy tranport channel req, src tcid: %hu, ret: %u", req.srcTcid, ret);
    }
    return;
}

static void DTAP_ReorderTimerExpireProc(void *args)
{
    DTAP_Channel_S *channel = (DTAP_Channel_S *)args;
    if (channel == NULL || channel->attr == NULL) {
        return;
    }

    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)channel->attr;
    DTAP_LOGI("reorder timer expired, send nack. lcid: %hu, tcid: %hhu, expectedTxSeq: %hu, nack cnt: %hhu",
        channel->lcid, channel->dstTcid, reliable->rxWindow.expectedTxSeq, reliable->nackCnt);
    // 异常应答次数超过最大重传输次数，断开传输通道
    if (reliable->nackCnt >= reliable->maxTxThreshold) {
        DTAP_LOGE("send nack frame reaches max tx threshold: %hhu", reliable->maxTxThreshold);
        DTAP_DynTransChannReleaseReq(channel);
        return;
    }

    uint32_t ret = DTAP_SendAckFrame(channel, false, true);
    if (ret != DTAP_SUCCESS) {
        DTAP_LOGE("send ack frame failed, ret: %08x", ret);
        return;
    }

    // 发送响应重置应答定时器
    (void)DTAP_TransRestartTimer(&reliable->rspTimer, reliable->rspTimeout, true, DTAP_RspTimerExpireProc, channel);
    reliable->nackCnt++;
}

static uint32_t DTAP_RetransEnhanceFrame(DTAP_Channel_S *channel)
{
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)channel->attr;
    DTAP_Frame_S *frame = DTAP_FindFrameBySeq(&(reliable->txWindow.txList), reliable->reTxSeq);
    DTAP_LOGD("Retrans frame, lcid:%hu, tcid: %hhu, reTxSeq: %hu", channel->lcid,
            channel->dstTcid, reliable->reTxSeq);
    if (frame == NULL) {
        DTAP_LOGE("find frame err. lcid: %hu, tcid: %hhu, reTxSeq: %hu", channel->lcid,
            channel->dstTcid, reliable->reTxSeq);
        return DTAP_TRANS_RELIABLE_FIND_FRAME_ERR;
    }

    // 重传次数超过最大重传输次数，断开传输通道
    if (frame->enhance.reTxCnt >= reliable->maxTxThreshold) {
        DTAP_LOGE("retrans frame reaches max tx threshold: %hhu", reliable->maxTxThreshold);
        DTAP_DynTransChannReleaseReq(channel);
        return DTAP_TRANS_RELIABLE_REACH_MAX_RE_TX_CNT_ERR;
    }

    SDF_Buff_S *buff = SDF_BuffCopy(frame->buff);
    if (buff == NULL) {
        DTAP_LOGE("copy buff failed");
        return DTAP_TRANS_RELIABLE_COPY_BUFF_ERR;
    }

    uint32_t ret = DTAP_SendFrame(channel, buff);
    if (ret != DTAP_SUCCESS) {
        SDF_BuffFree(buff);
        return ret;
    }

    frame->enhance.reTxCnt++;
    DTAP_LOGI("retrans frame. lcid: %hu, tcid: %hhu, reTxSeq: %hu, retrans cnt: %hhu", channel->lcid,
        channel->dstTcid, reliable->reTxSeq, frame->enhance.reTxCnt);

    // 发送双向帧重置应答定时器
    if (frame->ctx.getFrameType() == DTAP_FRAME_DUPLEX_FRAG) {
        (void)DTAP_TransRestartTimer(&reliable->rspTimer, reliable->rspTimeout, true,
            DTAP_RspTimerExpireProc, channel);
    }
    return DTAP_SUCCESS;
}

static void DTAP_RetransTimerExpireProc(void *args)
{
    DTAP_Channel_S *channel = (DTAP_Channel_S *)args;
    if (channel == NULL || channel->attr == NULL) {
        return;
    }

    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)channel->attr;
    if (SDF_DListCount(&(reliable->txWindow.txList)) == 0) {
        return;
    }

    DTAP_LOGI("retrans timer expired, retrans frame");
    reliable->reTxSeq = reliable->txWindow.expectedAckSeq;
    (void)DTAP_RetransEnhanceFrame(channel);
}

static void DTAP_SlideTxWindow(DTAP_Channel_S *transChan, const DTAP_Frame_S *dtapFrame)
{
    DTAP_LOGD("Start SlideTxWindow, lcid: %hu, tcid: %hu", transChan->lcid, transChan->dstTcid);
    uint8_t frameType = dtapFrame->ctx.getFrameType();
    uint16_t reqSeq = frameType == DTAP_FRAME_ACK ? dtapFrame->ack.reqSeq : dtapFrame->enhance.reqSeq;
    if (reqSeq == DTAP_FRAME_SEQ_INIT) {
        DTAP_LOGD("reqSeq is DTAP_FRAME_SEQ_INIT");
        return;
    }

    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)transChan->attr;
    // 收到对端ACK，发送轮询技术器重置
    reliable->sendPollingCounter = 0;
    if (SDF_DListIsEmpty(&(reliable->txWindow.txList))) {
        DTAP_LOGD("txList is empty, lcid: %hu, tcid: %hu", transChan->lcid, transChan->dstTcid);
        return;
    }

    // 接收到正常应答之后，如果ReTxSeq < ReqSeq（说明当前正在重传更早的PDU）则置ReTxSeq为ReqSeq
    if (DTAP_IsFrameSeqSmaller(reliable->reTxSeq, reqSeq)) {
        DTAP_LOGD("reliable->reTxSeq is reset from %hu to %hu", reliable->reTxSeq, reqSeq);
        reliable->reTxSeq = reqSeq;
    }

    // 发送队列滑窗
    DTAP_LOGD("Before expectedAckSeq = %hu, nextTxSeq = %hu, reqSeq = %hu",
        reliable->txWindow.expectedAckSeq, reliable->txWindow.nextTxSeq, reqSeq);
    if ((DTAP_IsFrameSeqSmaller(reliable->txWindow.expectedAckSeq, reqSeq) &&
        DTAP_IsFrameSeqSmaller(reqSeq, reliable->txWindow.nextTxSeq)) || reqSeq == reliable->txWindow.nextTxSeq) {
        DTAP_LOGD("expectedAckSeq is reset from %hu to %hu", reliable->txWindow.expectedAckSeq, reqSeq);
        reliable->txWindow.expectedAckSeq = reqSeq;
        DTAP_Frame_S *frame = NULL;
        DTAP_Frame_S *tmp = NULL;
        SDF_DListElmSafeForeach(frame, tmp, &(reliable->txWindow.txList), entry) {
            if (DTAP_IsFrameSeqSmaller(frame->enhance.txSeq, reqSeq)) {
                SDF_DListElmDel(&(reliable->txWindow.txList), frame, entry);
                DTAP_DestroyFrame(frame);
            } else {
                break;
            }
        }
        if (reliable->txWindow.expectedAckSeq == reliable->txWindow.nextTxSeq) {
            // 所有已发送PDU均已收到，停止重传定时器
            DTAP_LOGD("All PDU Received and DTAP_TransStopTimer");
            DTAP_TransStopTimer(&reliable->retransTimer);
        } else {
            // 收到应答之后，如果ExpectedAckSeq < ReqSeq < NextTxSeq则重启重传定时器。
            (void)DTAP_TransRestartTimer(&reliable->retransTimer, reliable->retransTimeout, true,
                DTAP_RetransTimerExpireProc, transChan);
        }
    }
    DTAP_LOGD("End SlideTxWindow, lcid: %hu, tcid: %hu", transChan->lcid, transChan->dstTcid);
    DTAP_LOGD("After expectedAckSeq = %hu, nextTxSeq = %hu, reqSeq = %hu",
        reliable->txWindow.expectedAckSeq, reliable->txWindow.nextTxSeq, reqSeq);
}

static uint32_t DTAP_RecvNackFrame(DTAP_Channel_S *transChan, const DTAP_Frame_S *dtapFrame)
{
    if (dtapFrame->ctx.getFrameType() == DTAP_FRAME_ACK && dtapFrame->ack.sBit) {
        DTAP_LOGI("ackframe and sBit is true");
        DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)transChan->attr;
        reliable->reTxSeq = dtapFrame->ack.reqSeq;
        if (DTAP_RetransEnhanceFrame(transChan) == DTAP_TRANS_RELIABLE_REACH_MAX_RE_TX_CNT_ERR) {
            return DTAP_TRANS_RELIABLE_REACH_MAX_RE_TX_CNT_ERR;
        }
    }

    return DTAP_SUCCESS;
}

static void DTAP_SlideRxWindow(DTAP_Channel_S *transChan, DTAP_Frame_S *dtapFrame)
{
    uint16_t txSeq = dtapFrame->enhance.txSeq;
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)transChan->attr;

    reliable->isRecvFrame = true;
    // 按照txSeq由小到大插入到收包队列
    DTAP_Frame_S *cur = NULL;
    SDF_DListElmForeach(cur, &(reliable->rxWindow.rxList), entry) {
        if (DTAP_IsFrameSeqSmaller(txSeq, cur->enhance.txSeq)) {
            break;
        }
    }
    SDF_DListAdd(&(reliable->rxWindow.rxList), &dtapFrame->entry, SDF_DListPrev(&cur->entry), &cur->entry);

    // 如果TxSeq == ExpectedTxSeq，则将ExpectedTxSeq置为下一个未成功接收的PDU的TxSeq
    if (txSeq == reliable->rxWindow.expectedTxSeq) {
        uint16_t seq = DTAP_GetNextFrameSeq(txSeq);
        for (size_t i = 0; i < reliable->rxWindow.size; i++) {
            if (DTAP_FindFrameBySeq(&(reliable->rxWindow.rxList), seq) == NULL) {
                reliable->rxWindow.expectedTxSeq = seq;
                reliable->nackCnt = 0;
                break;
            }
            seq = DTAP_GetNextFrameSeq(seq);
        }
    }

    // 如果 TxSeq >= MaxExpectedTxSeq，则将MaxExpectedTxSeq置为TxSeq+1
    if (DTAP_IsFrameSeqSmaller(reliable->rxWindow.maxExpectedTxSeq, txSeq) ||
        reliable->rxWindow.maxExpectedTxSeq == txSeq) {
        reliable->rxWindow.maxExpectedTxSeq = DTAP_GetNextFrameSeq(txSeq);
    }
}

static void DTAP_OperateReorderTimer(DTAP_Channel_S *transChan)
{
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)transChan->attr;

    if (reliable->reorderTimer == INVALID_TIMER_HANDLE) {
        // ExpectedTxSeq < MaxExpectedTxSeq启动重排序定时器
        if (DTAP_IsFrameSeqSmaller(reliable->rxWindow.expectedTxSeq, reliable->rxWindow.maxExpectedTxSeq)) {
            (void)DTAP_TransStartTimer(&reliable->reorderTimer, reliable->reorderTimeout, true,
            DTAP_ReorderTimerExpireProc, transChan);
        }
        return;
    }

    // ExpectedTxSeq == MaxExpectedTxSeq停止重排序定时器
    if (reliable->rxWindow.expectedTxSeq == reliable->rxWindow.maxExpectedTxSeq) {
        DTAP_TransStopTimer(&reliable->reorderTimer);
        return;
    }

    // ExpectedTxSeq < MaxExpectedTxSeq重启重排序定时器
    if (DTAP_IsFrameSeqSmaller(reliable->rxWindow.expectedTxSeq, reliable->rxWindow.maxExpectedTxSeq)) {
        (void)DTAP_TransRestartTimer(&reliable->reorderTimer, reliable->reorderTimeout, true,
            DTAP_ReorderTimerExpireProc, transChan);
    }
}

static void DTAP_RecvEnhanceFrameInner(DTAP_Channel_S *transChan, int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *))
{
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)transChan->attr;
    if (reliable->isRxWindowFull) {
        DTAP_LOGE("Trans channel is full");
        return;
    }
    while (!SDF_DListIsEmpty(&(reliable->rxWindow.rxList))) {
        DTAP_Frame_S *firstFrame = SDF_CONTAINER_OF(SDF_DListFirst(&(reliable->rxWindow.rxList)), DTAP_Frame_S, entry);
        if (firstFrame->enhance.txSeq != reliable->rxWindow.bufferSeq) {
            return;
        }

        // 合片/分拆后递交到上层
        uint32_t ret = DTAP_SUCCESS;
        DTAP_Data_Info_S info = {.pi = firstFrame->pi, .lcid = transChan->lcid, .tcid = transChan->dstTcid};
        switch (firstFrame->ctx.getFrameType()) {
            case DTAP_FRAME_SIMPLEX_AGGR:
            case DTAP_FRAME_DUPLEX_AGGR:
                ret = DTAP_RecvAggregateFrame(firstFrame, &info, recvCb);
                break;
            case DTAP_FRAME_SIMPLEX_FRAG:
            case DTAP_FRAME_DUPLEX_FRAG:
                ret = DTAP_RecvFragmentFrame(&reliable->frags, firstFrame, &info, recvCb);
                break;
            default:
                DTAP_LOGW("invalid frame type: %hhu", firstFrame->ctx.getFrameType());
                break;
        }
        DTAP_LOGI("DTAP_RecvEnhanceFrameInner, ret:%hu", ret);
        if (ret == DTAP_TRANS_RELIABLE_TRANS_CHANNEL_FULL) {
            reliable->isRxWindowFull = true;
            DTAP_LOGE("Trans channel is full, stop at txSeq: %hu, ret: 0x%08x", firstFrame->enhance.txSeq, ret);
            return;
        }

        if (ret != DTAP_SUCCESS) {
            DTAP_LOGE("recv enhance frame failed, txSeq: %hu, ret: 0x%08x", firstFrame->enhance.txSeq, ret);
        }

        SDF_DListElmDel(&(reliable->rxWindow.rxList), firstFrame, entry);
        DTAP_DestroyFrame(firstFrame);
        reliable->rxWindow.bufferSeq = DTAP_GetNextFrameSeq(reliable->rxWindow.bufferSeq);
    }
}

static void DTAP_SetTransChannelStatus(DTAP_Channel_S *transChan, uint16_t result,
                                        int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *))
{
    DTAP_LOGI("SetTransChannelStatus, result:%hu", result);
    if (result == DTAP_SUCCESS) {
        DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)transChan->attr;
        reliable->isRxWindowFull = false;
        DTAP_RecvEnhanceFrameInner(transChan, recvCb);
    }
}

static uint32_t DTAP_RecvEnhanceFrame(DTAP_Channel_S *transChan, DTAP_Frame_S *dtapFrame,
                                      int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *))
{
    uint32_t ret = DTAP_SUCCESS;
    uint8_t frameType = dtapFrame->ctx.getFrameType();
    uint16_t reqSeq = frameType == DTAP_FRAME_ACK ? dtapFrame->ack.reqSeq : dtapFrame->enhance.reqSeq;
    DTAP_LOGD("Start RecvEnhanceFrame, lcid: %hu, tcid: %hu, reqSeq: %hu",
        transChan->lcid, transChan->dstTcid, reqSeq);
    ret = DTAP_RecvNackFrame(transChan, dtapFrame);
    if (ret != DTAP_SUCCESS) {
        DTAP_LOGE("Failed to DTAP_RecvNackFrame");
        return ret;
    }
    DTAP_SlideTxWindow(transChan, dtapFrame);

    if (dtapFrame->ctx.getFrameType() == DTAP_FRAME_ACK) {
        DTAP_LOGD("ACK frame");
        DTAP_TransChannelStatusChange(transChan, DTAP_SUCCESS);
        return DTAP_SUCCESS;
    }

    DTAP_Frame_S *copyFrame = DTAP_CopyFrame(dtapFrame);
    if (copyFrame == NULL) {
        return DTAP_TRANS_RELIABLE_COPY_FRAME_ERR;
    }
    DTAP_SlideRxWindow(transChan, copyFrame);
    DTAP_OperateReorderTimer(transChan);

    // pbit为1需要立即回复ack帧
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)transChan->attr;
    if (DTAP_CheckFrameBit(copyFrame->bits, DTAP_FRAME_P_BIT)) {
        DTAP_LOGI("p bit is set in received frame tx seq %hu, send ack immediately", copyFrame->enhance.txSeq);
        ret = DTAP_SendAckFrame(transChan, true, false);
        if (ret != DTAP_SUCCESS) {
            DTAP_LOGE("send ack frame err. ret: %08x", ret);
        } else {
            // 发送响应重置应答定时器
            (void)DTAP_TransRestartTimer(&reliable->rspTimer, reliable->rspTimeout,
                true, DTAP_RspTimerExpireProc, transChan);
        }
    }

    DTAP_RecvEnhanceFrameInner(transChan, recvCb);
    DTAP_TransChannelStatusChange(transChan, DTAP_SUCCESS);
    return DTAP_SUCCESS;
}

static uint32_t DTAP_PackEnhanceFrames(DTAP_Channel_S *transChan, uint8_t pi, SDF_Buff_S *buff,
    uint16_t mps, DTAP_Frame_S *frames[DTAP_MAX_FRAGMENT_NUM])
{
    uint16_t frameCnt = 0;
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)transChan->attr;
    uint8_t frameType = reliable->isRecvFrame ? DTAP_FRAME_DUPLEX_FRAG : DTAP_FRAME_SIMPLEX_FRAG;
    DTAP_FragmentFrame(buff, mps, frameType, frames, &frameCnt);
    if (frameCnt == 0) {
        return DTAP_TRANS_RELIABLE_FRAGMENT_ERR;
    }

    uint16_t nextTxSeq = reliable->txWindow.nextTxSeq;
    uint32_t ret;
    for (uint16_t i = 0; i < frameCnt; i++) {
        DTAP_Frame_S *frame = frames[i];
        frame->bits = DTAP_FRAME_C_BIT;
        frame->pi = pi;
        frame->enhance.txSeq = nextTxSeq;
        frame->enhance.reqSeq = reliable->rxWindow.expectedTxSeq;
        ret = frame->ctx.buildFrame(&frame->ctx, transChan->dstTcid, reliable->crcInit);
        if (ret != DTAP_SUCCESS) {
            goto ERR;
        }
        nextTxSeq = DTAP_GetNextFrameSeq(nextTxSeq);
    }

    return DTAP_SUCCESS;
ERR:
    for (uint16_t i = 0; i < frameCnt; i++) {
        DTAP_DestroyFrame(frames[i]);
        frames[i] = NULL;
    }
    return ret;
}

static bool DTAP_TriggerSendPolling(DTAP_ReliableChannel_S *reliable)
{
    if (reliable->sendPollingCounter * SEND_POLLING_CNT_THRESHOLD_FACTOR >= reliable->txWindow.size) {
        reliable->sendPollingCounter = 0;
        return true;
    }
    reliable->sendPollingCounter++;
    return false;
}

static void DTAP_SetBitForSendPolling(DTAP_Frame_S *frame, uint16_t crcInit)
{
    if (frame == NULL) {
        return;
    }
    DTAP_BasicHeader_S *frameHeader = (DTAP_BasicHeader_S *)SDF_DataOffset(frame->buff);
    uint8_t oriBits = frameHeader->typeBits >> DTAP_BASIC_FRAME_BITS_SHIFT;
    uint8_t newBits = oriBits | DTAP_FRAME_P_BIT;
    if (DTAP_SetFrameBit(frameHeader, newBits) == DTAP_SUCCESS) {
        DTAP_LOGI("p bit is set in sending frame tx seq %hu", frame->enhance.txSeq);
        // recalculate crc value
        uint32_t ret = DTAP_ReCalculateCrcValue(crcInit, frame);
        if (ret != DTAP_SUCCESS) {
            DTAP_LOGE("recalculate crc value failed, ret: 0x%08x.", ret);
        }
    }
}

static uint32_t DTAP_SendEnhanceFrameInner(DTAP_Channel_S *transChan, DTAP_Frame_S *frames[DTAP_MAX_FRAGMENT_NUM],
    uint16_t frameCnt)
{
    uint32_t ret;
    bool isResetRspTimer = false;
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)transChan->attr;

    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, frameCnt <= DTAP_MAX_FRAGMENT_NUM, DTAP_TRANS_RELIABLE_INVALID_FRAME_NUM,
        "invalid frame cnt: %hu", frameCnt);

    if (DTAP_IsFrameSeqSmaller(reliable->reTxSeq, reliable->txWindow.nextTxSeq)) {
        ret = DTAP_RetransEnhanceFrame(transChan);          // 重传旧包
        // 重传达到最大次数直接返回
        if (ret == DTAP_TRANS_RELIABLE_REACH_MAX_RE_TX_CNT_ERR) {
            return ret;
        } else if (ret == DTAP_SUCCESS) {
            // 有一次发送双向帧成功需要重置应答定时器
            isResetRspTimer = true;
        } else {
            // 发送失败等待下一次重传
            DTAP_LOGE("retrans frame failed, ret: 0x%08x, code: %u", ret, SDF_ErrGetErrId(ret));
        }
        // 只重传reTxSeq对应的PDU，重传完成后恢复到nextTxSeq以便后续发送新PDU，已对齐这部分实现属于规范允许范围
        reliable->reTxSeq = reliable->txWindow.nextTxSeq;
    }

    for (size_t i = 0; i < frameCnt; i++) {
        DTAP_Frame_S *frame = frames[i];
        SDF_Buff_S *copyBuff = NULL;

        if (DTAP_TriggerSendPolling(reliable)) {
            DTAP_SetBitForSendPolling(frame, reliable->crcInit);
        }

        copyBuff = SDF_BuffCopy(frame->buff);
        ret = DTAP_SendFrame(transChan, copyBuff);    // 发送新包
        if (ret != DTAP_SUCCESS) {
            // 发送失败等待下一次重传
            SDF_BuffFree(copyBuff);
            DTAP_LOGE("send frame failed, ret: 0x%08x, code: %u", ret, SDF_ErrGetErrId(ret));
        } else {
            // 有一次发送双向帧成功需要重置应答定时器
            isResetRspTimer = true;
        }

        reliable->reTxSeq = DTAP_GetNextFrameSeq(reliable->reTxSeq);
        reliable->txWindow.nextTxSeq = DTAP_GetNextFrameSeq(reliable->txWindow.nextTxSeq);
        // 缓存新包到发包队列
        SDF_DListElmTailInsert(&(reliable->txWindow.txList), frame, entry);
    }

    // 发送双向帧重置应答定时器
    if (isResetRspTimer && reliable->isRecvFrame) {
        (void)DTAP_TransRestartTimer(&reliable->rspTimer, reliable->rspTimeout, true,
            DTAP_RspTimerExpireProc, transChan);
    }

    // 启动重传定时器，启动失败若出现丢包且对端未启动重排序定时器则无法重传该包
    if (reliable->retransTimer == INVALID_TIMER_HANDLE) {
        (void)DTAP_TransStartTimer(&reliable->retransTimer, reliable->retransTimeout, true,
            DTAP_RetransTimerExpireProc, transChan);
    }

    return DTAP_SUCCESS;
}

static uint32_t DTAP_SendEnhanceFrame(DTAP_Channel_S *transChan, uint8_t pi, SDF_Buff_S *buff)
{
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)transChan->attr;
    uint8_t headerLen = reliable->isRecvFrame ? DTAP_DUPLEX_FRAG_FRAME_HEADER_LEN : DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN;
    if (transChan->mps <= headerLen + DTAP_CRC_LEN) {
        return DTAP_TRANS_RELIABLE_INVALID_MPS;
    }
    uint16_t mps = transChan->mps - headerLen - DTAP_CRC_LEN;
    uint16_t frameCnt = DTAP_GetFragmentFramesNum(SDF_DataLenGet(buff), mps);
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, frameCnt <= DTAP_MAX_FRAGMENT_NUM, DTAP_TRANS_RELIABLE_INVALID_FRAME_NUM,
        "invalid frame cnt: %hu", frameCnt);

    // 如果当前这个包超出发送队列，缓存这个包，等每次滑窗的时候再尝试发送
    if (SDF_DListCount(&(reliable->txWindow.txList)) + frameCnt > reliable->txWindow.size) {
        DTAP_LOGE("Size is over txWindow size");
        if (reliable->cacheBuff == NULL) {
            DTAP_LOGI("reliable->cacheBuff is created");
            reliable->cacheBuff = SDF_BuffCopy(buff);
            reliable->cachePi = pi;
        }
        reliable->isTxWindowFull = true;
        return DTAP_TRANS_RELIABLE_TX_WINDOW_FULL;
    }
    // 如果当前这个包刚好填满发送队列，继续处理
    if (SDF_DListCount(&(reliable->txWindow.txList)) + frameCnt == reliable->txWindow.size) {
        DTAP_LOGE("Size equal txWindow size");
        reliable->isTxWindowFull = true;
    }

    DTAP_Frame_S *frames[DTAP_MAX_FRAGMENT_NUM] = {NULL};
    uint32_t ret = DTAP_PackEnhanceFrames(transChan, pi, buff, mps, frames);
    if (ret != DTAP_SUCCESS) {
        return ret;
    }

    ret = DTAP_SendEnhanceFrameInner(transChan, frames, frameCnt);
    if (ret == DTAP_SUCCESS) {
        // 判断发送队列是否刚好已满
        if (reliable->isTxWindowFull) {
            ret = DTAP_TRANS_RELIABLE_TX_WINDOW_FULL;
        } else {
            SDF_BuffFree(buff);
        }
    } else {
        for (uint16_t i = 0; i < frameCnt; i++) {
            DTAP_DestroyFrame(frames[i]);
        }
    }
    return ret;
}

static DTAP_TransMode_S g_dtapReliableTransMode = {
    .getModeType = DTAP_GetReliableModeType,
    .checkFrame = DTAP_CheckEnhanceFrame,
    .recvFrame = DTAP_RecvEnhanceFrame,
    .sendFrame = DTAP_SendEnhanceFrame,
    .transFrame = NULL,
    .setTransChannelStatus = DTAP_SetTransChannelStatus,
};

void DTAP_RegisterReliableTransMode(void)
{
    DTAP_RegisterTransMode(&g_dtapReliableTransMode);
}