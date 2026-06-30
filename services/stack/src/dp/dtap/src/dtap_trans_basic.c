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
 * this file contains dtap basic transmission mode implement.
 *
 ***************************************************************************/

#include <stddef.h>
#include <stdint.h>

#include <arpa/inet.h>

#include "securec.h"

#include "cm_def.h"
#include "dpfwk_log.h"
#include "dtap_errno.h"
#include "dtap_tcid.h"
#include "dtap_trans.h"
#include "dtap_frame.h"
#include "sdf_buff.h"
#include "sdf_dlist.h"
#include "sdf_mem.h"

#define DTAP_BASIC_MODE_MAX_CACHE_NUM 64U

typedef struct DTAP_BasicCacheFrame {
    SDF_DListEntry_S entry;
    SDF_Buff_S *buff;
    DTAP_Data_Info_S info;
    int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *);
} DTAP_BasicCacheFrame_S;

static DTAP_BasicCacheFrame_S *DTAP_CreateBasicCacheFrame(SDF_Buff_S *buff, DTAP_Data_Info_S info,
    int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *))
{
    DTAP_BasicCacheFrame_S *basicFrame = SDF_MemZalloc(sizeof(DTAP_BasicCacheFrame_S));
    CHECK_AND_RETURN_RET_LOG(DTAP_TAG, basicFrame != NULL, NULL, "malloc err");
    SDF_DListEntryInit(&(basicFrame->entry));
    basicFrame->buff = SDF_BuffCopy(buff);
    if (basicFrame->buff == NULL) {
        DTAP_LOGE("copy buff err");
        SDF_MemFree(basicFrame);
        return NULL;
    }
    basicFrame->info = info;
    basicFrame->recvCb = recvCb;
    return basicFrame;
}

void DTAP_DestroyBasicCacheFrame(SDF_DListEntry_S *entry)
{
    if (entry == NULL) {
        return;
    }

    DTAP_BasicCacheFrame_S *frame = SDF_CONTAINER_OF(entry, DTAP_BasicCacheFrame_S, entry);
    if (frame == NULL) {
        return;
    }

    if (frame->buff != NULL) {
        SDF_BuffFree(frame->buff);
    }

    SDF_MemFree(frame);
}

void DTAP_RecvBasicFrameContinue(DTAP_Channel_S *transChan)
{
    CHECK_AND_RETURN_LOG(DTAP_TAG, transChan != NULL && transChan->attr != NULL, "channel is null");
    DTAP_Basic_Channel_S *basic = (DTAP_Basic_Channel_S *)transChan->attr;

    DTAP_BasicCacheFrame_S *frame = NULL;
    DTAP_BasicCacheFrame_S *temp = NULL;
    SDF_DListElmSafeForeach(frame, temp, &(basic->cacheRxBuffs), entry) {
        if (frame->recvCb != NULL) {
            frame->recvCb(&frame->info, frame->buff);
        }
        SDF_DListElmDel(&(basic->cacheRxBuffs), frame, entry);
        DTAP_DestroyBasicCacheFrame(&frame->entry);
    }

    basic->isReady = true;
}

static uint8_t DTAP_GetBasicModeType(void)
{
    return CM_TRANS_MODE_BASIC;
}

static bool DTAP_CheckBasicFrame(const DTAP_Channel_S *transChan, DTAP_Frame_S *dtapFrame)
{
    (void)transChan;
    DTAP_BasicFrameHeader_S *basicHeader = (DTAP_BasicFrameHeader_S *)dtapFrame->header;

    if (basicHeader->header.frameType != DTAP_FRAME_BASIC) {
        return false;
    }

    return true;
}

static uint32_t DTAP_RecvBasicFrame(DTAP_Channel_S *transChan, DTAP_Frame_S *dtapFrame,
                                    int (*recvCb)(DTAP_Data_Info_S *, SDF_Buff_S *))
{
    DTAP_Data_Info_S info = {0};
    SDF_Buff_S *buff = dtapFrame->buff;
    DTAP_Basic_Channel_S *basic = (DTAP_Basic_Channel_S *)transChan->attr;

    if (SDF_BuffTrimPrefix(buff, dtapFrame->headerLen) == NULL) {
        return DTAP_TRANS_BASIC_RECV_TRIM_PRE_ERR;
    }

    info.lcid = transChan->lcid;
    info.pi = dtapFrame->pi;
    info.tcid = transChan->srcTcid;
    if (!basic->isReady) {
        CHECK_AND_RETURN_RET_LOG(DTAP_TAG, SDF_DListCount(&(basic->cacheRxBuffs)) <= DTAP_BASIC_MODE_MAX_CACHE_NUM,
            DTAP_TRANS_BASIC_CACHE_OVERFLOW_ERR, "the number of cached buffs has reached its maximum value");
        DTAP_LOGW("channel is not ready, lcid: %hu, srcTcid: %hhu", transChan->lcid, transChan->srcTcid);
        DTAP_BasicCacheFrame_S *basicFrame = DTAP_CreateBasicCacheFrame(buff, info, recvCb);
        CHECK_AND_RETURN_RET_LOG(DTAP_TAG, basicFrame != NULL, DTAP_TRANS_BASIC_CREATE_FRAME_ERR, "copy err");
        SDF_DListElmTailInsert(&(basic->cacheRxBuffs), basicFrame, entry);
        return DTAP_SUCCESS;
    }

    if (recvCb != NULL) {
        recvCb(&info, buff);
    }
    return DTAP_SUCCESS;
}

static uint32_t DTAP_SendBasicFrame(DTAP_Channel_S *transChan, uint8_t pi, SDF_Buff_S *buff)
{
    DTAP_Frame_S frame = {0};
    frame.pi = pi;
    frame.buff = buff;
    DTAP_FrameCtx_S *ctx = DTAP_GetFrameCtx(DTAP_FRAME_BASIC);
    if (ctx == NULL) {
        return DTAP_TRANS_BASIC_SEND_GET_CTX_ERR;
    }

    frame.ctx = *ctx;
    uint32_t ret = ctx->buildFrame(&frame.ctx, transChan->dstTcid, 0);
    if (ret != DTAP_SUCCESS) {
        return ret;
    }

    return DTAP_SendFrame(transChan, buff);
}

static DTAP_TransMode_S g_dtapBasicTransMode = {
    .getModeType = DTAP_GetBasicModeType,
    .checkFrame = DTAP_CheckBasicFrame,
    .recvFrame = DTAP_RecvBasicFrame,
    .sendFrame = DTAP_SendBasicFrame,
    .transFrame = NULL,
    .setTransChannelStatus = NULL,
};

void DTAP_RegisterBasicTransMode(void)
{
    DTAP_RegisterTransMode(&g_dtapBasicTransMode);
}