/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
 * this file contains dtap basic frame implement.
 *
 ***************************************************************************/

#include "dtap_frame.h"

#include <stdbool.h>
#include <stdint.h>

#include "byte_codec.h"
#include "dtap.h"
#include "dtap_errno.h"
#include "dtap_tcid.h"
#include "sdf_typedef.h"

static uint8_t DTAP_GetBasicFrameType(void)
{
    return DTAP_FRAME_BASIC;
}

static uint8_t DTAP_GetBasicFrameHdrLen(void)
{
    return DTAP_BASIC_HEADER_LEN;
}

static uint32_t DTAP_ParseBasicFrame(const DTAP_FrameCtx_S *ctx)
{
    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    DTAP_BasicFrameHeader_S *basicHeader = (DTAP_BasicFrameHeader_S *)frame->header;
    uint64_t dataLen = SDF_DataLenGet(frame->buff);

    // 当TCID=0x01~0x1D时为固定传输通道标识，基础帧格式中省略PI字段
    if (basicHeader->header.tcid <= TCID_FTC_RFU_END) {
        frame->pi = DTAP_PI_NONE;
        frame->headerLen = (uint16_t)DTAP_BASIC_HEADER_LEN;
        frame->payloadLen = (uint16_t)(dataLen - DTAP_BASIC_HEADER_LEN);
    } else {
        if (dataLen < DTAP_BASIC_HEADER_LEN + sizeof(uint8_t)) {
            return DTAP_BASIC_FRAME_INVALID_PI_LEN;
        }

        frame->pi = *(uint8_t *)(basicHeader->pi);
        frame->headerLen = (uint16_t)(DTAP_BASIC_HEADER_LEN + sizeof(uint8_t));
        frame->payloadLen = (uint16_t)(dataLen - DTAP_BASIC_HEADER_LEN - sizeof(uint8_t));
    }

    return DTAP_SUCCESS;
}

static bool DTAP_CheckBasicFrameHeader(const DTAP_FrameCtx_S *ctx)
{
    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    DTAP_BasicFrameHeader_S *basicHeader = (DTAP_BasicFrameHeader_S *)frame->header;

    // 基础帧O指示位、C指示位和RFU均为0
    if (!(basicHeader->header.optionBit == 0 && basicHeader->header.crcBit == 0 && basicHeader->header.pBit == 0 &&
          basicHeader->header.fBit == 0)) {
        return false;
    }

    return true;
}

static uint32_t DTAP_BuildBasicFrame(const struct DTAP_FrameCtx *ctx, uint8_t tcid, uint16_t crcInit)
{
    DTAP_Frame_S *frame = SDF_CONTAINER_OF(ctx, DTAP_Frame_S, ctx);
    uint8_t headerLen = (uint8_t)DTAP_BASIC_HEADER_LEN;

    if (tcid > TCID_FTC_RFU_END) {
        headerLen++;
    }

    DTAP_BasicFrameHeader_S *basicHeader = (DTAP_BasicFrameHeader_S *)SDF_BuffPrepend(frame->buff, headerLen);
    if (basicHeader == NULL) {
        return DTAP_TRANS_BASIC_SEND_PREPEND_ERR;
    }

    basicHeader->header.tcid = tcid;
    basicHeader->header.frameType = DTAP_FRAME_BASIC;
    basicHeader->header.optionBit = 0;
    basicHeader->header.crcBit = 0;
    basicHeader->header.pBit = 0;
    basicHeader->header.fBit = 0;
    ENCODE2BYTE_LITTLE(&basicHeader->header.length, SDF_DataLenGet(frame->buff) - DTAP_BASIC_HEADER_LEN);
    if (tcid > TCID_FTC_RFU_END) {
        *(basicHeader->pi) = frame->pi;
    }

    return DTAP_SUCCESS;
}

static DTAP_FrameCtx_S g_dtapFrameBasicCtx = {
    .getFrameType = DTAP_GetBasicFrameType,
    .getHeaderLen = DTAP_GetBasicFrameHdrLen,
    .parseFrame = DTAP_ParseBasicFrame,
    .checkFrameHeader = DTAP_CheckBasicFrameHeader,
    .buildFrame = DTAP_BuildBasicFrame,
};

void DTAP_RegisterBasicFrameCtx(void)
{
    DTAP_RegisterFrameCtx(&g_dtapFrameBasicCtx);
}