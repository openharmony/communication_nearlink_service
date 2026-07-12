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

#include "dtap_frame.h"

#include "securec.h"
#include "crc16.h"
#include "dli_layer.h"
#include "dtap_errno.h"
#include "dtap_scheduler.h"
#include "dpfwk_log.h"
#include "sdf_buff.h"
#include "sdf_mem.h"

#define DTAP_EXTENSIONS_MAX_NUM 16U

static DTAP_FrameCtx_S *g_dtapFrameCtx[DTAP_FRAME_MAX] = { NULL };

void DTAP_RegisterFrameCtx(DTAP_FrameCtx_S *dtapFrameCtx)
{
    CHECK_AND_RETURN_LOG(DTAP_TAG, dtapFrameCtx != NULL, "dtapFrameCtx is null.");

    CHECK_AND_RETURN_LOG(DTAP_TAG, dtapFrameCtx->getFrameType != NULL, "getFrameType is null.");
    uint8_t frameType = dtapFrameCtx->getFrameType();
    CHECK_AND_RETURN_LOG(DTAP_TAG, frameType < DTAP_FRAME_MAX, "invalid frame type: %hhu.", frameType);

    g_dtapFrameCtx[frameType] = dtapFrameCtx;
}

DTAP_FrameCtx_S *DTAP_GetFrameCtx(uint8_t frameType)
{
    if (frameType >= DTAP_FRAME_MAX) {
        return NULL;
    }

    return g_dtapFrameCtx[frameType];
}

uint32_t DTAP_ParseFrame(uint8_t frameType, SDF_Buff_S *buff, DTAP_Frame_S *dtapFrame)
{
    DTAP_FrameCtx_S *dtapFrameCtx = DTAP_GetFrameCtx(frameType);
    if (dtapFrameCtx == NULL) {
        return DTAP_FRAME_INVALID_FRAME_TYPE;
    }

    SDF_DListEntryInit(&dtapFrame->entry);
    dtapFrame->buff = buff;
    dtapFrame->header = SDF_DataOffset(buff);
    dtapFrame->ctx = *dtapFrameCtx;
    return dtapFrame->ctx.parseFrame(&dtapFrame->ctx);
}

uint32_t DTAP_SendFrame(DTAP_Channel_S *transChan, SDF_Buff_S *buff)
{
    uint32_t ret = DTAP_DataSendWithPriority(transChan, buff);
    if (ret != 0) {
        DTAP_LOGE("DTAP_DataSendWithPriority failed, errno %08x", ret);
        return DTAP_FRAME_SEND_DLI_SEND_ERR;
    }

    return DTAP_SUCCESS;
}

DTAP_Frame_S *DTAP_CreateFrame(uint8_t frameType)
{
    DTAP_FrameCtx_S *ctx = DTAP_GetFrameCtx(frameType);
    if (ctx == NULL) {
        DTAP_LOGE("get frame ctx failed, type: %hhu", frameType);
        return NULL;
    }

    DTAP_Frame_S *frame = SDF_MemZalloc(sizeof(DTAP_Frame_S));
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, frame != NULL, NULL, "malloc frame failed");
    SDF_DListEntryInit(&frame->entry);
    frame->ctx = *ctx;
    return frame;
}

void DTAP_DestroyFrame(DTAP_Frame_S *frame)
{
    if (frame == NULL) {
        return;
    }

    if (frame->buff != NULL) {
        SDF_BuffFree(frame->buff);
    }
    SDF_MemFree(frame);
}

DTAP_Frame_S *DTAP_CopyFrame(const DTAP_Frame_S *srcFrame)
{
    if (srcFrame == NULL) {
        return NULL;
    }

    DTAP_Frame_S *dstFrame = (DTAP_Frame_S *)SDF_MemAlloc(sizeof(DTAP_Frame_S));
    if (dstFrame == NULL) {
        return NULL;
    }
    (void)memcpy_s(dstFrame, sizeof(DTAP_Frame_S), srcFrame, sizeof(DTAP_Frame_S));

    if (srcFrame->buff == NULL) {
        return dstFrame;
    }

    SDF_Buff_S *buff = SDF_BuffCopy(srcFrame->buff);
    if (buff == NULL) {
        SDF_MemFree(dstFrame);
        return NULL;
    }
    dstFrame->buff = buff;
    return dstFrame;
}

uint32_t DTAP_ParseExtension(DTAP_Frame_S *frame)
{
    if (frame == NULL || frame->buff == NULL || frame->header == NULL) {
        return DTAP_FRAME_NULL_PTR_ERR;
    }

    if (SDF_DataLenGet(frame->buff) < frame->headerLen) {
        return DTAP_FRAME_INVALID_BUFF_LEN;
    }

    uint16_t dataLen = (uint16_t)(SDF_DataLenGet(frame->buff) - frame->headerLen);
    if (dataLen < sizeof(DTAP_ExtensionHeader_S)) {
        return DTAP_FRAME_INVALID_EXT_HDR_LEN;
    }

    frame->extension = (uint8_t *)frame->header + frame->headerLen;
    uint16_t extensionLen = sizeof(DTAP_ExtensionHeader_S);
    const DTAP_ExtensionHeader_S *extHeader = (const DTAP_ExtensionHeader_S *)frame->extension;
    const DTAP_Extension_S *extension = extHeader->data;
    if (extHeader->num > DTAP_EXTENSIONS_MAX_NUM) {
        return DTAP_FRAME_INVALID_EXT_NUM;
    }
    for (int i = 0; i < extHeader->num; i++) {
        if (dataLen < extensionLen || (dataLen - extensionLen < sizeof(DTAP_Extension_S))) {
            return DTAP_FRAME_INVALID_EXT_LEN;
        }
        extensionLen += sizeof(DTAP_Extension_S);

        if (dataLen < extensionLen || (dataLen - extensionLen < extension->length)) {
            return DTAP_FRAME_INVALID_EXT_LEN;
        }
        extensionLen += extension->length;

        extension = (const DTAP_Extension_S *)((uint8_t *)extension + sizeof(DTAP_Extension_S) + extension->length);
    }

    frame->extensionLen = extensionLen;
    return DTAP_SUCCESS;
}

uint32_t DTAP_SetFrameBit(DTAP_BasicHeader_S *frameHeader, uint8_t bits)
{
    if (frameHeader == NULL) {
        return DTAP_FRAME_NULL_PTR_ERR;
    }

    frameHeader->optionBit = DTAP_CheckFrameBit(bits, DTAP_FRAME_O_BIT) ? 1 : 0;
    frameHeader->crcBit = DTAP_CheckFrameBit(bits, DTAP_FRAME_C_BIT) ? 1 : 0;
    frameHeader->pBit = DTAP_CheckFrameBit(bits, DTAP_FRAME_P_BIT) ? 1 : 0;
    frameHeader->fBit = DTAP_CheckFrameBit(bits, DTAP_FRAME_F_BIT) ? 1 : 0;
    return DTAP_SUCCESS;
}

uint32_t DTAP_ReCalculateCrcValue(uint16_t crcInit, DTAP_Frame_S *frame)
{
    if (frame == NULL || frame->buff == NULL) {
        return DTAP_FRAME_NULL_PTR_ERR;
    }
    if (SDF_DataLenGet(frame->buff) < DTAP_CRC_LEN) {
        return DTAP_FRAME_INVALID_BUFF_LEN;
    }
    // 获取除去crc之外的数据长度
    uint64_t realDataLen = SDF_DataLenGet(frame->buff) - DTAP_CRC_LEN;
    uint16_t *originCrc = (uint16_t *)(SDF_DataOffset(frame->buff) + realDataLen);
    // 清除原来的crc, 重新计算crc
    (void)memset_s(originCrc, DTAP_CRC_LEN, 0, DTAP_CRC_LEN);
    uint16_t newCrcValue = CRC16(crcInit, SDF_DataOffset(frame->buff), realDataLen);
    *originCrc = newCrcValue;
    return DTAP_SUCCESS;
}