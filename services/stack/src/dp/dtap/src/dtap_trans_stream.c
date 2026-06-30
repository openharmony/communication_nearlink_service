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
 * this file contains dtap stream transmission mode implement.
 *
 ***************************************************************************/

#include "dtap_frame.h"
#include <stddef.h>
#include <stdint.h>

#include <arpa/inet.h>

#include "securec.h"

#include "byte_codec.h"
#include "cp_worker.h"
#include "crc16.h"
#include "dpfwk_log.h"
#include "time_utils.h"
#include "dtap_channel.h"
#include "dtap_errno.h"
#include "dtap_tcid.h"
#include "dtap_trans.h"
#include "sdf_mem.h"
#include "sdf_timer.h"

#define DTAP_STREAM_CACHE_MAX 256

typedef struct {
    SDF_DListEntry_S entry;
    uint8_t sar; /* 分片状态指示 */
    uint16_t txSeq;
    SDF_Buff_S *buff;
    uint64_t timeStamp;
    uint8_t pi;
    uint16_t lcid;
    uint8_t tcid;
} StreamRxNode;

static void DTAP_ReorderStreamFrameCbk(void *args);

static uint8_t DTAP_GetStreamModeType(void)
{
    return CM_TRANS_MODE_STREAM;
}

static bool DTAP_CheckStreamFrame(const DTAP_Channel_S *transChan, DTAP_Frame_S *dtapFrame)
{
    (void)transChan;
    DTAP_BasicHeader_S *frameHeader = (DTAP_BasicHeader_S *)dtapFrame->header;

    if (frameHeader->frameType == DTAP_FRAME_SIMPLEX_FRAG ||
        frameHeader->frameType == DTAP_FRAME_SIMPLEX_AGGR) {
        return true;
    }

    return false;
}

static StreamRxNode *NewStreamRxNode(SDF_Buff_S *buff)
{
    StreamRxNode *node = (StreamRxNode *)SDF_MemZalloc(sizeof(StreamRxNode));
    if (node == NULL) {
        return NULL;
    }
    node->buff = SDF_BuffCopy(buff);
    if (node->buff == NULL) {
        SDF_MemFree(node);
        return NULL;
    }
    SDF_DListEntryInit(&node->entry);
    return node;
}

static void InsertStreamRxNode(SDF_DListHead_S *head, StreamRxNode *node)
{
    //  按照txSeq从小到大排序，找到合适的位置插入
    StreamRxNode *tmp = NULL;
    StreamRxNode *cur = NULL;
    SDF_DListElmSafeForeach(cur, tmp, head, entry) {
        if (DTAP_IsFrameSeqSmaller(node->txSeq, cur->txSeq)) {
            // node->txSeq < cur->txSeq
            break;
        }
    }
    SDF_DListAdd(head, &node->entry, SDF_DListPrev(&cur->entry), &cur->entry);
}

static void FreeStreamRxNode(StreamRxNode *node)
{
    if (node != NULL) {
        SDF_BuffFree(node->buff);
        SDF_MemFree(node);
    }
}

static StreamRxNode *FindStreamRxNode(SDF_DListHead_S *head, uint16_t txSeq)
{
    StreamRxNode *node = NULL;
    SDF_DListElmForeach(node, head, entry) {
        if (node->txSeq == txSeq) {
            return node;
        }
    }
    return NULL;
}

static void RecvCallback(DTAP_Channel_S *transChan, SDF_Buff_S *buff, uint8_t pi)
{
    DTAP_Data_Info_S info = {};
    info.pi = pi;
    info.lcid = transChan->lcid;
    info.tcid = transChan->srcTcid;
    DTAP_Stream_Channel_S *stream = (DTAP_Stream_Channel_S *)transChan->attr;
    if (stream->recvCb != NULL) {
        stream->recvCb(&info, buff);
    }
}

static void StopReorderTimer(DTAP_Stream_Channel_S *stream)
{
    if (stream->reorderTimerHandle != INVALID_TIMER_HANDLE) {
        CP_TimerDel(stream->reorderTimerHandle);
        stream->reorderTimerHandle = INVALID_TIMER_HANDLE;
    }
}

static void StartReorderTimer(DTAP_Channel_S *transChan)
{
    DTAP_Stream_Channel_S *stream = (DTAP_Stream_Channel_S *)transChan->attr;
    if (stream->expectedSeq != stream->maxExpectedSeq) {
        uint64_t now = DP_GetMonoTimeMs();
        uint64_t minTimeStamp = now;
        StreamRxNode *node = NULL;
        SDF_DListElmForeach(node, &stream->rxList, entry) {
            if (node->timeStamp < minTimeStamp) {
                minTimeStamp = node->timeStamp;
                stream->reorderTxSeq = node->txSeq;
            }
        }
        uint64_t diff = now - minTimeStamp;
        uint64_t expires = diff > stream->reorderTimeout ? 0 : stream->reorderTimeout - diff;
        SDF_TimerParam param = {
            .expires = expires,
            .period = false,
            .callback = DTAP_ReorderStreamFrameCbk,
            .args = (void *)(uintptr_t)((((uint32_t)transChan->lcid) << 16) | ((uint32_t)transChan->srcTcid)),
        };
        if (CP_TimerAdd(&stream->reorderTimerHandle, &param) != SDF_OK) {
            DTAP_LOGE("reorder timer add failed");
        }
    }
}

static void ReportExpectedStreamFrame(DTAP_Channel_S *transChan)
{
    DTAP_Stream_Channel_S *stream = (DTAP_Stream_Channel_S *)transChan->attr;
    StreamRxNode *node = NULL;
    StreamRxNode *tmp = NULL;
    SDF_DListElmSafeForeach(node, tmp, &stream->rxList, entry) {
        if (node->txSeq != stream->expectedSeq) {
            break;
        }
        RecvCallback(transChan, node->buff, node->pi);
        SDF_DListElmDel(&stream->rxList, node, entry);
        FreeStreamRxNode(node);
        stream->expectedSeq = DTAP_GetNextFrameSeq(stream->expectedSeq);
    }
}

static void DTAP_ReorderStreamFrameTask(void *args)
{
    uint32_t timeoutArgs = (uint32_t)(uintptr_t)args;
    uint16_t lcid = (timeoutArgs >> 16) & 0xFFFF;
    uint8_t tcid = timeoutArgs & 0xFFFF;

    DTAP_Channel_S *transChan = DTAP_ChannelSearch(lcid, tcid);
    if (transChan == NULL || transChan->mode != CM_TRANS_MODE_STREAM) {
        DTAP_LOGE("reorder stream frame task, trans chan not found");
        return;
    }

    DTAP_Stream_Channel_S *stream = (DTAP_Stream_Channel_S *)transChan->attr;
    stream->reorderTimerHandle = INVALID_TIMER_HANDLE;

    StreamRxNode *tmp = NULL;
    StreamRxNode *node = NULL;
    SDF_DListElmSafeForeach(node, tmp, &stream->rxList, entry) {
        if (!DTAP_IsFrameSeqSmaller(node->txSeq, stream->reorderTxSeq)) {
            break;
        }
        stream->expectedSeq = DTAP_GetNextFrameSeq(node->txSeq);
        RecvCallback(transChan, node->buff, node->pi);
        SDF_DListElmDel(&stream->rxList, node, entry);
        FreeStreamRxNode(node);
    }

    if (node != NULL) {
        ReportExpectedStreamFrame(transChan);
        stream->maxExpectedSeq = ((StreamRxNode *)SDF_DListLast(&stream->rxList))->txSeq + 1;
        StartReorderTimer(transChan);
    }
}

static void DTAP_ReorderStreamFrameCbk(void *args)
{
    uint32_t ret = CP_PostTask(DTAP_ReorderStreamFrameTask, args, NULL);
    if (ret != CP_OK) {
        DTAP_LOGE("DTAP_ReorderStreamFrameTask post failed, ret:0x%08x", ret);
    }
}

static uint32_t BuffCheck(DTAP_Frame_S *dtapFrame, DTAP_Stream_Channel_S *stream, SDF_Buff_S *buff)
{
    uint8_t *crc = (SDF_DataOffset(buff) + SDF_DataLenGet(buff) - DTAP_CRC_LEN);
    uint16_t buffCrc = DECODE2BYTE_LITTLE(crc);
    if (SDF_BuffTrimSuffix(buff, DTAP_CRC_LEN) != 0) {
        DTAP_LOGE("buff trim crc failed");
        return DTAP_TRANS_STREAM_RECV_TRIM_SUF_ERR;
    }
    uint16_t crcValue = CRC16(stream->crcInit, SDF_DataOffset(buff), SDF_DataLenGet(buff));
    if (buffCrc != crcValue) {
        DTAP_LOGE("crc check failed, crcInit:0x%04x, buffLen:%u, buffCrc:0x%04x, crcValue:0x%04x",
            stream->crcInit, SDF_DataLenGet(buff), buffCrc, crcValue);
        return DTAP_TRANS_STREAM_RECV_CRC_ERR;
    }
    if (SDF_BuffTrimPrefix(buff, dtapFrame->headerLen) == NULL) {
        DTAP_LOGE("buff trim header failed");
        return DTAP_TRANS_STREAM_RECV_TRIM_PRE_ERR;
    }
    return DTAP_SUCCESS;
}

static uint32_t DTAP_RecvStreamFrame(DTAP_Channel_S *transChan, DTAP_Frame_S *dtapFrame,
    int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *))
{
    DTAP_SimplexFragFrameHeader_S *simplexFragHeader = (DTAP_SimplexFragFrameHeader_S *)dtapFrame->header;
    if (simplexFragHeader->sar != DTAP_SAR_UNSEG) {
        DTAP_LOGE("sar:%u not support", simplexFragHeader->sar);
        return DTAP_TRANS_UNSUPPORTED;
    }
    DTAP_Stream_Channel_S *stream = (DTAP_Stream_Channel_S *)transChan->attr;
    stream->recvCb = recvCb;
    bool isNotFull = SDF_DListCount(&stream->rxList) <= DTAP_STREAM_CACHE_MAX;
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, isNotFull, DTAP_TRANS_STREAM_RX_FULL, "rxList is full");
    SDF_Buff_S *buff = dtapFrame->buff;
    uint32_t ret = BuffCheck(dtapFrame, stream, buff);
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, ret == DTAP_SUCCESS, ret, "");
    // 首次收到包，直接回调
    if (stream->expectedSeq == DTAP_FRAME_SEQ_INIT) {
        stream->expectedSeq = DTAP_GetNextFrameSeq(simplexFragHeader->txSeq);
        stream->maxExpectedSeq = DTAP_GetNextFrameSeq(simplexFragHeader->txSeq);
        RecvCallback(transChan, buff, dtapFrame->pi);
        return DTAP_SUCCESS;
    }
    // 收到期待的包，回调，更新expectedSeq，检查链表是否已经有expectedSeq的包，有的话回调并删除
    if (simplexFragHeader->txSeq == stream->expectedSeq) {
        RecvCallback(transChan, buff, dtapFrame->pi);
        stream->expectedSeq = DTAP_GetNextFrameSeq(stream->expectedSeq);
        ReportExpectedStreamFrame(transChan);
        StopReorderTimer(stream);
        StartReorderTimer(transChan);
        return DTAP_SUCCESS;
    }
    // 丢弃重复包
    if (DTAP_IsFrameSeqSmaller(simplexFragHeader->txSeq, stream->expectedSeq)) {
        DTAP_LOGE("recv duplicate frame, seq:%u", simplexFragHeader->txSeq);
        return DTAP_TRANS_STREAM_RECV_DUPLICATED_ERR;
    }
    if (FindStreamRxNode(&stream->rxList, simplexFragHeader->txSeq) != NULL) {
        DTAP_LOGE("recv duplicate frame, seq:%u", simplexFragHeader->txSeq);
        return DTAP_TRANS_STREAM_RECV_DUPLICATED_ERR;
    }
    StreamRxNode *node = NewStreamRxNode(buff);
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, node != NULL, DTAP_TRANS_MALLOC_ERR, "new rx node failed");
    node->sar = simplexFragHeader->sar;
    node->txSeq = simplexFragHeader->txSeq;
    node->timeStamp = DP_GetMonoTimeMs();
    node->pi = dtapFrame->pi;
    node->lcid = transChan->lcid;
    node->tcid = transChan->srcTcid;
    InsertStreamRxNode(&stream->rxList, node);
    stream->maxExpectedSeq = ((StreamRxNode *)SDF_DListLast(&stream->rxList))->txSeq + 1;
    if (stream->reorderTimerHandle == INVALID_TIMER_HANDLE) {
        StartReorderTimer(transChan);
    }
    return DTAP_SUCCESS;
}

static uint32_t DTAP_PackStreamFrame(DTAP_Stream_Channel_S *stream, uint8_t tcid, uint8_t pi,
    SDF_Buff_S *buff, uint8_t sar)
{
    // buff预留了16个字节：SDF_BUFF_MAX_HEADROOM_SIZE + SDF_BUFF_MAX_TAILROOM_SIZE
    DTAP_SimplexFragFrameHeader_S *basicHeader =
        (DTAP_SimplexFragFrameHeader_S *)SDF_BuffPrepend(buff, DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN);
    if (basicHeader == NULL) {
        return DTAP_TRANS_STREAM_PACK_HEADER_ERR;
    }

    stream->frameTxSeq = DTAP_GetNextFrameSeq(stream->frameTxSeq);
    basicHeader->header.tcid = tcid;
    basicHeader->header.frameType = DTAP_FRAME_SIMPLEX_FRAG; // 当前没有实现单向聚合帧
    basicHeader->header.optionBit = 0;
    basicHeader->header.crcBit = 1;
    basicHeader->header.pBit = 0;
    basicHeader->header.fBit = 0;
    basicHeader->pi = pi;
    basicHeader->txSeq = stream->frameTxSeq;
    basicHeader->sar = sar;

    ENCODE2BYTE_LITTLE(&basicHeader->header.length, SDF_DataLenGet(buff) - DTAP_BASIC_HEADER_LEN + DTAP_CRC_LEN);

    uint16_t crcValue = CRC16(stream->crcInit, SDF_DataOffset(buff), SDF_DataLenGet(buff));
    DTAP_LOGD("crcInit:0x%04x, buffLen:0x%08x, crcValue:0x%04x", stream->crcInit, SDF_DataLenGet(buff), crcValue);
    uint8_t *crc = SDF_BuffAppend(buff, DTAP_CRC_LEN);
    if (crc == NULL) {
        return DTAP_TRANS_STREAM_PACK_CRC_ERR;
    }
    ENCODE2BYTE_LITTLE(crc, crcValue);
    return DTAP_SUCCESS;
}

static uint32_t DTAP_SendStreamFrame(DTAP_Channel_S *transChan, uint8_t pi, SDF_Buff_S *buff)
{
    // 单播、组播的tcid为dstTcid，广播的tcid为srcTcid
    uint8_t tcid = transChan->dstTcid;
    if (transChan->castMode == DTAP_FRAME_BROADCAST) {
        tcid = transChan->srcTcid;
    }
    // 遗留: 扩展字段长度计算
    DTAP_Stream_Channel_S *stream = (DTAP_Stream_Channel_S *)transChan->attr;
    if (SDF_DataLenGet(buff) <= transChan->mtu) {
        // 当前没有实现单向聚合帧，所以直接发送
        uint32_t ret = DTAP_PackStreamFrame(stream, tcid, pi, buff, DTAP_SAR_UNSEG);
        if (ret != DTAP_SUCCESS) {
            return ret;
        }
        return DTAP_SendFrame(transChan, buff);
    }
    return DTAP_TRANS_UNSUPPORTED;
}

static DTAP_TransMode_S g_dtapStreamTransMode = {
    .getModeType = DTAP_GetStreamModeType,
    .checkFrame = DTAP_CheckStreamFrame,
    .recvFrame = DTAP_RecvStreamFrame,
    .sendFrame = DTAP_SendStreamFrame,
    .transFrame = NULL,
    .setTransChannelStatus = NULL,
};

void DTAP_RegisterStreamMode(void)
{
    DTAP_RegisterTransMode(&g_dtapStreamTransMode);
}

void FreeStreamList(SDF_DListEntry_S *entry)
{
    StreamRxNode *node = (StreamRxNode *)entry;
    if (node != NULL) {
        SDF_BuffFree(node->buff);
        SDF_MemFree(node);
    }
}