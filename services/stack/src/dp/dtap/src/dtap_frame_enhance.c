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
 * this file contains implement for dtap enhance frame, including simplx
 * aggragate frame, simplx fragment frame, duplex aggragate frame,
 * dtap duplex fragment frame and dtap acknowledgement frame.
 *
 ***************************************************************************/

#include <stdbool.h>
#include <stdint.h>

#include "securec.h"

#include "byte_codec.h"
#include "crc16.h"
#include "dtap.h"
#include "dtap_errno.h"
#include "dtap_frame.h"
#include "dtap_tcid.h"
#include "sdf_buff.h"
#include "sdf_mem.h"
#include "sdf_typedef.h"
#include "dpfwk_log.h"

#define DTAP_MAX_FRAG_TOTAL_LEN  65535 // mtu最大值为65535，因此应用数据最大为65535，分片总长度也不会超过该值

uint32_t DTAP_RecvAggregateFrame(DTAP_Frame_S *frame, DTAP_Data_Info_S *info,
    int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *))
{
    if (frame == NULL || frame->payload == NULL) {
        return DTAP_ENHANCED_FRAME_NULL;
    }

    if (recvCb == NULL || frame->payloadLen == 0) {
        return DTAP_SUCCESS;
    }

    uint16_t sduTotalLen = frame->payloadLen;
    DTAP_AggregateSdu_S *sdu = (DTAP_AggregateSdu_S *)frame->payload;
    int result = DTAP_SUCCESS;
    do {
        if (sduTotalLen < sizeof(DTAP_AggregateSdu_S)) {
            return DTAP_ENHANCED_FRAME_SDU_HDR_LEN_TOO_SHORT;
        }

        sduTotalLen -= sizeof(DTAP_AggregateSdu_S);
        uint16_t sduLen = DECODE2BYTE_LITTLE(&sdu->length);
        if (sduLen == 0 || sduTotalLen < sduLen) {
            return DTAP_ENHANCED_FRAME_SDU_LEN_TOO_SHORT;
        }

        SDF_Buff_S *buff = SDF_BuffNew(sduLen);
        if (buff == NULL) {
            return DTAP_ENHANCED_FRAME_BUFF_NEW_ERR;
        }
        uint8_t *data = SDF_BuffAppend(buff, sduLen);
        if (data == NULL) {
            SDF_BuffFree(buff);
            return DTAP_ENHANCED_FRAME_APPEND_ERR;
        }
        (void)memcpy_s(data, sduLen, sdu->data, sduLen);
        // 正常接收完成上报：payload也偏移，payloadlen也要变；接收端返回队列满了，payload不偏移，payloadlen不变
        result = recvCb(info, buff);
        DTAP_LOGD("recvCb result is %hu", result);
        SDF_BuffFree(buff);
        if (result == DTAP_TRANS_RELIABLE_TRANS_CHANNEL_FULL) {
            frame->payload = (uint8_t *)sdu;
            frame->payloadLen = sduTotalLen;
            return DTAP_TRANS_RELIABLE_TRANS_CHANNEL_FULL;
        }
        sduTotalLen -= sduLen;
        sdu = (DTAP_AggregateSdu_S *)((uint8_t *)sdu + sizeof(DTAP_AggregateSdu_S) + sduLen);
    } while (sduTotalLen > 0);

    return DTAP_SUCCESS;
}

static uint32_t DTAP_UpdateFragmentFrame(SDF_Buff_S **buffs, SDF_Buff_S *buff)
{
    uint64_t totalFragLen = SDF_DataLenGet(*buffs) + SDF_DataLenGet(buff);
    if (totalFragLen > DTAP_MAX_FRAG_TOTAL_LEN) {
        SDF_BuffFree(*buffs);
        *buffs = NULL;
        return DTAP_ENHANCED_FRAME_FRAGMENT_FRAME_LEN_ERR;
    }

    SDF_Buff_S *newBuff = SDF_BuffNewWithReserve(totalFragLen);
    if (newBuff == NULL) {
        goto ERR;
    }

    uint8_t *data = SDF_BuffAppend(newBuff, SDF_DataLenGet(*buffs));
    if (data == NULL) {
        goto ERR;
    }
    (void)memcpy_s(data, SDF_DataLenGet(*buffs), SDF_DataOffset(*buffs), SDF_DataLenGet(*buffs));

    data = SDF_BuffAppend(newBuff, SDF_DataLenGet(buff));
    if (data == NULL) {
        goto ERR;
    }
    (void)memcpy_s(data, SDF_DataLenGet(buff), SDF_DataOffset(buff), SDF_DataLenGet(buff));
    SDF_BuffFree(*buffs);
    *buffs = newBuff;
    return DTAP_SUCCESS;
ERR:
    SDF_BuffFree(newBuff);
    SDF_BuffFree(*buffs);
    *buffs = NULL;
    return DTAP_ENHANCED_FRAME_UPDATE_FRAG_FRAME_ERR;
}

uint32_t DTAP_RecvFragmentFrame(SDF_Buff_S **buffs, const DTAP_Frame_S *frame, DTAP_Data_Info_S *info,
    int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *))
{
    if (buffs == NULL || frame == NULL) {
        return DTAP_ENHANCED_FRAME_NULL;
    }

    if (SDF_BuffTrimPrefix(frame->buff, frame->headerLen + frame->extensionLen) == NULL) {
        return DTAP_ENHANCED_FRAME_TRIM_PREFIX_ERR;
    }

    if (SDF_BuffTrimSuffix(frame->buff, DTAP_CRC_LEN) != 0) {
        return DTAP_ENHANCED_FRAME_TRIM_SUFFIX_ERR;
    }

    uint32_t ret = DTAP_SUCCESS;
    int result = DTAP_SUCCESS;
    switch (frame->enhance.sar) {
        case DTAP_SAR_UNSEG:
            if (recvCb != NULL) {
                result = recvCb(info, frame->buff);
            }
            break;
        case DTAP_SAR_FIRST:
            // 新收到首包释放原有的数据，填充新的数据
            if (*buffs != NULL) {
                SDF_BuffFree(*buffs);
                *buffs = NULL;
                ret = DTAP_ENHANCED_FRAME_DUP_FIRST_FRAG_FRAME;
            }
            *buffs = SDF_BuffCopy(frame->buff);
            break;
        case DTAP_SAR_MID:
        case DTAP_SAR_LAST:
            // 未收到首包，直接丢弃该包
            if (*buffs == NULL) {
                return DTAP_ENHANCED_FRAME_NO_FIRST_FRAG_FRAME;
            }
            ret = DTAP_UpdateFragmentFrame(buffs, frame->buff);
            if (frame->enhance.sar == DTAP_SAR_LAST && *buffs != NULL) {
                if (recvCb != NULL) {
                    result = recvCb(info, *buffs);
                }
                SDF_BuffFree(*buffs);
                *buffs = NULL;
            }
            break;
        default:
            break;
    }
    if (result == DTAP_TRANS_RELIABLE_TRANS_CHANNEL_FULL) {
        return DTAP_TRANS_RELIABLE_TRANS_CHANNEL_FULL;
    }

    return ret;
}

static uint8_t DTAP_GetFragFrameSar(uint16_t fragCnt, uint16_t curFragCnt)
{
    if (fragCnt == 1) {
        return DTAP_SAR_UNSEG;
    }

    if (curFragCnt == 1) {
        return DTAP_SAR_FIRST;
    } else if (curFragCnt < fragCnt) {
        return DTAP_SAR_MID;
    } else {
        return DTAP_SAR_LAST;
    }
}

void DTAP_FragmentFrame(SDF_Buff_S *buff, uint16_t mps, uint8_t frameType,
    DTAP_Frame_S *outFrames[DTAP_MAX_FRAGMENT_NUM], uint16_t *outCnt)
{
    if (buff == NULL || SDF_DataLenGet(buff) == 0 || outFrames == NULL || outCnt == NULL) {
        return;
    }

    if (frameType != DTAP_FRAME_SIMPLEX_FRAG && frameType != DTAP_FRAME_DUPLEX_FRAG) {
        return;
    }

    *outCnt = 0;
    uint64_t totalLen = SDF_DataLenGet(buff);
    uint8_t *data = SDF_DataOffset(buff);
    uint16_t cnt = DTAP_GetFragmentFramesNum(totalLen, mps);
    if (cnt > DTAP_MAX_FRAGMENT_NUM) {
        return;
    }

    int i = 0;
    while (totalLen > 0) {
        DTAP_Frame_S *frame = DTAP_CreateFrame(frameType);
        if (frame == NULL) {
            goto ERR;
        }

        uint64_t buffLen = (mps == 0) ? totalLen : (totalLen < mps ? totalLen : mps);
        SDF_Buff_S *subBuff = SDF_BuffNewWithReserve(buffLen);
        if (subBuff == NULL) {
            DTAP_DestroyFrame(frame);
            goto ERR;
        }

        uint8_t *buffData = SDF_BuffAppend(subBuff, buffLen);
        if (buffData == NULL) {
            DTAP_DestroyFrame(frame);
            SDF_BuffFree(subBuff);
            goto ERR;
        }

        (void)memcpy_s(buffData, buffLen, data, buffLen);
        outFrames[i++] = frame;
        frame->buff = subBuff;
        frame->enhance.sar = DTAP_GetFragFrameSar(cnt, i);
        totalLen -= buffLen;
        data = data + buffLen;
    }
    *outCnt = cnt;
    return;
ERR:
    for (i = i - 1; i >= 0; i--) {
        DTAP_DestroyFrame(outFrames[i]);
        outFrames[i] = NULL;
    }
}

static uint32_t DTAP_ParseEnhanceFrame(const DTAP_FrameCtx_S *ctx)
{
    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    uint64_t dataLen = SDF_DataLenGet(frame->buff);
    uint16_t headerLen = ctx->getHeaderLen();
    if (dataLen < headerLen) {
        return DTAP_ENHANCED_FRAME_HDR_LEN_TOO_SHORT;
    }

    DTAP_BasicHeader_S *header = (DTAP_BasicHeader_S *)frame->header;
    frame->bits = header->typeBits >> DTAP_BASIC_FRAME_BITS_SHIFT;
    frame->pi = *(uint8_t *)(header + 1);
    frame->headerLen = headerLen;

    if (header->optionBit != 0) {
        uint32_t ret = DTAP_ParseExtension(frame);
        if (ret != DTAP_SUCCESS) {
            return ret;
        }
    }

    frame->payload = (uint8_t *)frame->header + frame->headerLen + frame->extensionLen;
    frame->payloadLen = (uint16_t)(dataLen - frame->headerLen - frame->extensionLen);
    // 增强帧必须携带2字节的CRC校验码
    if (frame->payloadLen < DTAP_CRC_LEN) {
        return DTAP_ENHANCED_FRAME_LENGTH_TOO_SHORT;
    }
    frame->payloadLen -= DTAP_CRC_LEN;
    return DTAP_SUCCESS;
}

static bool DTAP_CheckEnhanceFrameHeader(const DTAP_FrameCtx_S *ctx)
{
    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    DTAP_BasicHeader_S *frameHeader = (DTAP_BasicHeader_S *)frame->header;

    return frameHeader->crcBit != 0 ? true : false;
}

// 单向聚合帧
static uint8_t DTAP_GetSimplexAggrFrameType(void)
{
    return DTAP_FRAME_SIMPLEX_AGGR;
}

static uint8_t DTAP_GetSimplexAggrFrameHdrLen(void)
{
    return DTAP_SIMPLEX_AGGR_FRAME_HEADER_LEN;
}

static uint32_t DTAP_ParseSimplexAggrFrame(const DTAP_FrameCtx_S *ctx)
{
    uint32_t ret = DTAP_ParseEnhanceFrame(ctx);
    if (ret != DTAP_SUCCESS) {
        return ret;
    }

    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    DTAP_SimplexAggrFrameHeader_S *header = (DTAP_SimplexAggrFrameHeader_S *)frame->header;
    frame->enhance.txSeq = header->txSeq;
    frame->enhance.reqSeq = DTAP_FRAME_SEQ_INIT;
    return DTAP_SUCCESS;
}

static bool DTAP_CheckSimplexAggrFrameHeader(const DTAP_FrameCtx_S *ctx)
{
    return DTAP_CheckEnhanceFrameHeader(ctx);
}

static uint32_t DTAP_BuildSimplexAggrFrame(const struct DTAP_FrameCtx *ctx, uint8_t tcid, uint16_t crcInit)
{
    return DTAP_ENHANCED_FRAME_NOT_SUPPORT_ERR;
}

// 单向分片帧
static uint8_t DTAP_GetSimplexFragFrameType(void)
{
    return DTAP_FRAME_SIMPLEX_FRAG;
}

static uint8_t DTAP_GetSimplexFragFrameHdrLen(void)
{
    return DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN;
}

static uint32_t DTAP_ParseSimplexFragFrame(const DTAP_FrameCtx_S *ctx)
{
    uint32_t ret = DTAP_ParseEnhanceFrame(ctx);
    if (ret != DTAP_SUCCESS) {
        return ret;
    }

    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    DTAP_SimplexFragFrameHeader_S *header = (DTAP_SimplexFragFrameHeader_S *)frame->header;
    frame->enhance.sar = header->sar;
    frame->enhance.txSeq = header->txSeq;
    frame->enhance.reqSeq = DTAP_FRAME_SEQ_INIT;
    return DTAP_SUCCESS;
}

static bool DTAP_CheckSimplexFragFrameHeader(const DTAP_FrameCtx_S *ctx)
{
    return DTAP_CheckEnhanceFrameHeader(ctx);
}

static uint32_t DTAP_BuildSimplexFragFrame(const struct DTAP_FrameCtx *ctx, uint8_t tcid, uint16_t crcInit)
{
    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    DTAP_SimplexFragFrameHeader_S *simplexFragHeader = (DTAP_SimplexFragFrameHeader_S *)SDF_BuffPrepend(frame->buff,
        DTAP_SIMPLEX_FRAG_FRAME_HEADER_LEN);
    if (simplexFragHeader == NULL) {
        return DTAP_ENHANCED_FRAME_PREPEND_ERR;
    }
    simplexFragHeader->header.tcid = tcid;
    simplexFragHeader->header.frameType = DTAP_FRAME_SIMPLEX_FRAG;
    simplexFragHeader->header.optionBit = 0;
    simplexFragHeader->header.crcBit = 1;
    simplexFragHeader->header.pBit = DTAP_CheckFrameBit(frame->bits, DTAP_FRAME_P_BIT) ? 1 : 0;
    simplexFragHeader->header.fBit = 0;
    ENCODE2BYTE_LITTLE(&simplexFragHeader->header.length, SDF_DataLenGet(frame->buff) -
        DTAP_BASIC_HEADER_LEN + DTAP_CRC_LEN);
    simplexFragHeader->pi = frame->pi;
    simplexFragHeader->txSeq = frame->enhance.txSeq;
    simplexFragHeader->sar = frame->enhance.sar;

    uint16_t crcValue = CRC16(crcInit, SDF_DataOffset(frame->buff), SDF_DataLenGet(frame->buff));
    DTAP_LOGD("txSeq: %hu, crc is %hu", simplexFragHeader->txSeq, crcValue);
    uint16_t *crc = (uint16_t *)SDF_BuffAppend(frame->buff, DTAP_CRC_LEN);
    if (crc == NULL) {
        return DTAP_ENHANCED_FRAME_APPEND_ERR;
    }
    *crc = crcValue;
    return DTAP_SUCCESS;
}

// 双向聚合帧
static uint8_t DTAP_GetDuplexAggrFrameType(void)
{
    return DTAP_FRAME_DUPLEX_AGGR;
}

static uint8_t DTAP_GetDuplexAggrFrameHdrLen(void)
{
    return DTAP_DUPLEX_AGGR_FRAME_HEADER_LEN;
}

static uint32_t DTAP_ParseDuplexAggrFrame(const DTAP_FrameCtx_S *ctx)
{
    uint32_t ret = DTAP_ParseEnhanceFrame(ctx);
    if (ret != DTAP_SUCCESS) {
        return ret;
    }

    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    DTAP_DuplexAggrFrameHeader_S *header = (DTAP_DuplexAggrFrameHeader_S *)frame->header;
    frame->enhance.txSeq = header->txSeq;
    frame->enhance.reqSeq = header->reqSeq;
    return DTAP_SUCCESS;
}

static bool DTAP_CheckDuplexAggrFrameHeader(const DTAP_FrameCtx_S *ctx)
{
    return DTAP_CheckEnhanceFrameHeader(ctx);
}

static uint32_t DTAP_BuildDuplexAggrFrame(const struct DTAP_FrameCtx *ctx, uint8_t tcid, uint16_t crcInit)
{
    return DTAP_ENHANCED_FRAME_NOT_SUPPORT_ERR;
}

// 双向分片帧
static uint8_t DTAP_GetDuplexFragFrameType(void)
{
    return DTAP_FRAME_DUPLEX_FRAG;
}

static uint8_t DTAP_GetDuplexFragFrameHdrLen(void)
{
    return DTAP_DUPLEX_FRAG_FRAME_HEADER_LEN;
}

static uint32_t DTAP_ParseDuplexFragFrame(const DTAP_FrameCtx_S *ctx)
{
    uint32_t ret = DTAP_ParseEnhanceFrame(ctx);
    if (ret != DTAP_SUCCESS) {
        return ret;
    }

    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    DTAP_DuplexFragFrameHeader_S *header = (DTAP_DuplexFragFrameHeader_S *)frame->header;
    frame->enhance.sar = header->sar;
    frame->enhance.txSeq = header->txSeq;
    frame->enhance.reqSeq = header->reqSeq;
    return DTAP_SUCCESS;
}

static bool DTAP_CheckDuplexFragFrameHeader(const DTAP_FrameCtx_S *ctx)
{
    return DTAP_CheckEnhanceFrameHeader(ctx);
}

static uint32_t DTAP_BuildDuplexFragFrame(const struct DTAP_FrameCtx *ctx, uint8_t tcid, uint16_t crcInit)
{
    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    DTAP_DuplexFragFrameHeader_S *duplexFragHeader = (DTAP_DuplexFragFrameHeader_S *)SDF_BuffPrepend(frame->buff,
        DTAP_DUPLEX_FRAG_FRAME_HEADER_LEN);
    if (duplexFragHeader == NULL) {
        return DTAP_ENHANCED_FRAME_PREPEND_ERR;
    }
    duplexFragHeader->header.tcid = tcid;
    duplexFragHeader->header.frameType = DTAP_FRAME_DUPLEX_FRAG;
    duplexFragHeader->header.optionBit = 0;
    duplexFragHeader->header.crcBit = 1;
    duplexFragHeader->header.pBit = DTAP_CheckFrameBit(frame->bits, DTAP_FRAME_P_BIT) ? 1 : 0;
    duplexFragHeader->header.fBit = DTAP_CheckFrameBit(frame->bits, DTAP_FRAME_F_BIT) ? 1 : 0;
    duplexFragHeader->pi = frame->pi;
    ENCODE2BYTE_LITTLE(&duplexFragHeader->header.length, SDF_DataLenGet(frame->buff) -
        DTAP_BASIC_HEADER_LEN + DTAP_CRC_LEN);
    duplexFragHeader->txSeq = frame->enhance.txSeq;
    duplexFragHeader->sar = frame->enhance.sar;
    duplexFragHeader->reqSeq = frame->enhance.reqSeq;

    uint16_t crcValue = CRC16(crcInit, SDF_DataOffset(frame->buff), SDF_DataLenGet(frame->buff));
    uint16_t *crc = (uint16_t *)SDF_BuffAppend(frame->buff, DTAP_CRC_LEN);
    if (crc == NULL) {
        return DTAP_ENHANCED_FRAME_APPEND_ERR;
    }
    *crc = crcValue;
    return DTAP_SUCCESS;
}

// 应答帧
static uint8_t DTAP_GetAckFrameType(void)
{
    return DTAP_FRAME_ACK;
}

static uint8_t DTAP_GetAckFrameHdrLen(void)
{
    return DTAP_ACK_FRAME_HEADER_LEN;
}

static uint32_t DTAP_ParseAckFrame(const DTAP_FrameCtx_S *ctx)
{
    uint32_t ret = DTAP_ParseEnhanceFrame(ctx);
    if (ret != DTAP_SUCCESS) {
        return ret;
    }

    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    DTAP_AckFrameHeader_S *header = (DTAP_AckFrameHeader_S *)frame->header;
    frame->ack.reqSeq = header->reqSeq;
    frame->ack.sBit = header->sBit == 0 ? false : true;
    return DTAP_SUCCESS;
}

static bool DTAP_CheckAckFrameHeader(const DTAP_FrameCtx_S *ctx)
{
    return DTAP_CheckEnhanceFrameHeader(ctx);
}

static uint32_t DTAP_BuildAckFrame(const struct DTAP_FrameCtx *ctx, uint8_t tcid, uint16_t crcInit)
{
    SDF_Buff_S *buff = SDF_BuffNewWithReserve(0);
    if (buff == NULL) {
        return DTAP_ENHANCED_FRAME_BUFF_NEW_ERR;
    }

    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    DTAP_AckFrameHeader_S *ackHeader = (DTAP_AckFrameHeader_S *)SDF_BuffPrepend(buff, DTAP_ACK_FRAME_HEADER_LEN);
    if (ackHeader == NULL) {
        SDF_BuffFree(buff);
        return DTAP_ENHANCED_FRAME_PREPEND_ERR;
    }
    ackHeader->header.tcid = tcid;
    ackHeader->header.frameType = DTAP_FRAME_ACK;
    ackHeader->header.optionBit = 0;
    ackHeader->header.crcBit = 1;
    ackHeader->header.pBit = 0;
    ackHeader->header.fBit = (frame->bits & DTAP_FRAME_F_BIT) != 0 ? 1 : 0;
    ENCODE2BYTE_LITTLE(&ackHeader->header.length, SDF_DataLenGet(buff) - DTAP_BASIC_HEADER_LEN + DTAP_CRC_LEN);
    ackHeader->reqSeq = frame->ack.reqSeq;
    ackHeader->sBit = frame->ack.sBit ? 1 : 0;

    uint16_t crcValue = CRC16(crcInit, SDF_DataOffset(buff), SDF_DataLenGet(buff));
    uint16_t *crc = (uint16_t *)SDF_BuffAppend(buff, DTAP_CRC_LEN);
    if (crc == NULL) {
        SDF_BuffFree(buff);
        return DTAP_ENHANCED_FRAME_APPEND_ERR;
    }
    *crc = crcValue;

    frame->buff = buff;
    return DTAP_SUCCESS;
}

static DTAP_FrameCtx_S g_dtapEnhanceFrameCtx[] = {
    {
        .getFrameType = DTAP_GetSimplexAggrFrameType,
        .getHeaderLen = DTAP_GetSimplexAggrFrameHdrLen,
        .parseFrame = DTAP_ParseSimplexAggrFrame,
        .checkFrameHeader = DTAP_CheckSimplexAggrFrameHeader,
        .buildFrame = DTAP_BuildSimplexAggrFrame,
    },
    {
        .getFrameType = DTAP_GetSimplexFragFrameType,
        .getHeaderLen = DTAP_GetSimplexFragFrameHdrLen,
        .parseFrame = DTAP_ParseSimplexFragFrame,
        .checkFrameHeader = DTAP_CheckSimplexFragFrameHeader,
        .buildFrame = DTAP_BuildSimplexFragFrame,
    },
    {
        .getFrameType = DTAP_GetDuplexAggrFrameType,
        .getHeaderLen = DTAP_GetDuplexAggrFrameHdrLen,
        .parseFrame = DTAP_ParseDuplexAggrFrame,
        .checkFrameHeader = DTAP_CheckDuplexAggrFrameHeader,
        .buildFrame = DTAP_BuildDuplexAggrFrame,
    },
    {
        .getFrameType = DTAP_GetDuplexFragFrameType,
        .getHeaderLen = DTAP_GetDuplexFragFrameHdrLen,
        .parseFrame = DTAP_ParseDuplexFragFrame,
        .checkFrameHeader = DTAP_CheckDuplexFragFrameHeader,
        .buildFrame = DTAP_BuildDuplexFragFrame,
    },
    {
        .getFrameType = DTAP_GetAckFrameType,
        .getHeaderLen = DTAP_GetAckFrameHdrLen,
        .parseFrame = DTAP_ParseAckFrame,
        .checkFrameHeader = DTAP_CheckAckFrameHeader,
        .buildFrame = DTAP_BuildAckFrame,
    },
};

void DTAP_RegisterEnhanceFrameCtx(void)
{
    for (size_t i = 0; i < sizeof(g_dtapEnhanceFrameCtx) / sizeof(g_dtapEnhanceFrameCtx[0]); i++) {
        DTAP_RegisterFrameCtx(&g_dtapEnhanceFrameCtx[i]);
    }
}
