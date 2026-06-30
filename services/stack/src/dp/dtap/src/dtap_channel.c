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

#include "dtap_channel.h"

#include <stdbool.h>

#include "cm_def.h"
#include "cm_errno.h"
#include "cm_logic_link_api.h"
#include "cm_icb_inner_api.h"
#include "cm_trans_channel_api.h"
#include "cm_inner_api.h"
#include "collab_ext_func_wrapper.h"
#include "cp_worker.h"
#include "dli_cmd.h"
#include "dli_layer_stru.h"
#include "dpfwk_log.h"
#include "dtap_scheduler.h"
#include "dtap_trans.h"
#include "dtap_errno.h"
#include "sdf_dlist.h"
#include "sdf_map.h"
#include "sdf_mem.h"
#include "sdf_traits.h"
#include "sdf_vector.h"
#include "cm_signaling_internal.h"

#define DTAP_RELIABLE_REORDER_TIMEOUT 10000 /* ms */
#define DTAP_RELIABLE_RETRANS_TIMEOUT 1000  /* ms */
#define DTAP_RELIABLE_RSP_TIMEOUT 1000  /* ms */

typedef struct DTAP_Logic_Link {
    uint16_t lcid;         // 逻辑链路标识
    SDF_DListHead_S list;  // 保存逻辑链路上的传输通道列表
} DTAP_Logic_Link_S;

static SDF_Map *g_dtapLogicLinksMap = NULL;

static void DTAP_BasicChannelDestroy(DTAP_Channel_S *channel)
{
    DTAP_Basic_Channel_S *basic = (DTAP_Basic_Channel_S *)channel->attr;
    SDF_DListDestroy(&(basic->cacheRxBuffs), DTAP_DestroyBasicCacheFrame);
}

static void DTAP_StreamChannelDestroy(DTAP_Channel_S *channel)
{
    DTAP_Stream_Channel_S *stream = (DTAP_Stream_Channel_S *)channel->attr;
    if (stream->reorderTimerHandle != INVALID_TIMER_HANDLE) {
        CP_TimerDel(stream->reorderTimerHandle);
        stream->reorderTimerHandle = INVALID_TIMER_HANDLE;
    }
    SDF_DListDestroy(&stream->txList, NULL);
    SDF_DListDestroy(&stream->rxList, FreeStreamList);
}

static void DTAP_ReliableChannelDestroy(DTAP_Channel_S *channel)
{
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)channel->attr;
    DTAP_TransStopTimer(&reliable->reorderTimer);
    DTAP_TransStopTimer(&reliable->retransTimer);
    DTAP_TransStopTimer(&reliable->rspTimer);
    DTAP_Frame_S *node = NULL;
    DTAP_Frame_S *tmpNode = NULL;
    SDF_DListElmAllFree(node, tmpNode, &reliable->txWindow.txList, entry, DTAP_DestroyFrame);
    node = NULL;
    tmpNode = NULL;
    SDF_DListElmAllFree(node, tmpNode, &reliable->rxWindow.rxList, entry, DTAP_DestroyFrame);
    if (reliable->frags != NULL) {
        SDF_BuffFree(reliable->frags);
    }
    if (reliable->cacheBuff != NULL) {
        SDF_BuffFree(reliable->cacheBuff);
    }
}

static void DTAP_ChannelDestroy(DTAP_Channel_S *channel)
{
    if (channel == NULL) {
        return;
    }

    if (channel->attr != NULL) {
        if (channel->mode == CM_TRANS_MODE_STREAM) {
            DTAP_StreamChannelDestroy(channel);
        } else if (channel->mode == CM_TRANS_MODE_RELIABLE) {
            DTAP_ReliableChannelDestroy(channel);
        } else if (channel->mode == CM_TRANS_MODE_BASIC) {
            DTAP_BasicChannelDestroy(channel);
        }
        SDF_MemFree(channel->attr);
    }

    SDF_MemFree(channel);
}

static inline int DTAP_LogicLinkCompare(const void *lhs_, const void *rhs_)
{
    uint16_t lhs = *(uint16_t *)lhs_;
    uint16_t rhs = *(uint16_t *)rhs_;
    return lhs - rhs;
}

static void DTAP_LogicLinksDtor(void *args)
{
    DTAP_Logic_Link_S *logicLink = (DTAP_Logic_Link_S *)args;
    DTAP_Channel_S *channel = NULL;
    DTAP_Channel_S *tmp = NULL;

    SDF_DListElmSafeForeach(channel, tmp, &logicLink->list, entry) {
        SDF_DListElmDel(&logicLink->list, channel, entry);
        DTAP_ChannelDestroy(channel);
    }
    SDF_MemFree(logicLink);
}

static DTAP_Logic_Link_S *DTAP_LogicLinkCreate(uint16_t lcid)
{
    DTAP_Logic_Link_S *logicLink = (DTAP_Logic_Link_S *)SDF_MemZalloc(sizeof(DTAP_Logic_Link_S));
    if (logicLink == NULL) {
        DTAP_LOGE("create dtap logic link failed due to insufficient memory");
        return NULL;
    }

    logicLink->lcid = lcid;
    SDF_DListHeadInit(&logicLink->list);
    return logicLink;
}

static void DTAP_ChannelDoNothing(void *arg) {}
static uint32_t DTAP_LogicLinkMapInit(void)
{
    SDF_Traits keyTraits = {
        .dtor = DTAP_ChannelDoNothing,
        .cmptor = DTAP_LogicLinkCompare,
    };
    SDF_Traits valTraits = {
        .dtor = DTAP_LogicLinksDtor,
        .cmptor = NULL,
    };

    g_dtapLogicLinksMap = SDF_MapCtor(keyTraits, valTraits);
    if (g_dtapLogicLinksMap == NULL) {
        DTAP_LOGE("create dtap logic link map fail.");
        return DTAP_TRANS_CHANNEL_CREATE_LINK_MAP_ERR;
    }

    return DTAP_SUCCESS;
}

static void DTAP_LogicLinkMapDeInit(void)
{
    if (g_dtapLogicLinksMap != NULL) {
        SDF_MapDtor(g_dtapLogicLinksMap);
        g_dtapLogicLinksMap = NULL;
    }
}

static DTAP_Logic_Link_S *DTAP_LogicLinkFind(uint16_t lcid)
{
    if (g_dtapLogicLinksMap == NULL) {
        DTAP_LOGE("logic link map is null.");
        return NULL;
    }

    SDF_MapIter *iter = SDF_MapFind(g_dtapLogicLinksMap, &lcid);
    if (iter != NULL) {
        return iter->val;
    }
    return NULL;
}

static DTAP_Channel_S *DTAP_ChannelFind(DTAP_Logic_Link_S *logicLink, uint8_t tcid)
{
    DTAP_Channel_S *channel = NULL;
    SDF_DListHead_S *head = &logicLink->list;

    SDF_DListElmForeach(channel, head, entry)
    {
        // 透传模式的传输通道和逻辑链路一一映射，接收的报文不携带tcid，不需要检查tcid
        if (channel->mode == CM_TRANS_MODE_TRANSPARENT) {
            return channel;
        }

        if (channel->srcTcid == tcid) {
            return channel;
        }
    }
    return NULL;
}

DTAP_Channel_S *DTAP_ChannelSearch(uint16_t lcid, uint8_t tcid)
{
    DTAP_Logic_Link_S *logicLink = DTAP_LogicLinkFind(lcid);
    if (logicLink == NULL) {
        DTAP_LOGE("search dtap logic link failed, lcid: %hu", lcid);
        return NULL;
    }

    return DTAP_ChannelFind(logicLink, tcid);
}

static DTAP_Basic_Channel_S *DTAP_BasicChannelCreate(uint8_t srcTcid)
{
    DTAP_Basic_Channel_S *basic = (DTAP_Basic_Channel_S *)SDF_MemZalloc(sizeof(DTAP_Basic_Channel_S));
    if (basic == NULL) {
        DTAP_LOGE("create dtap basic channel failed due to insufficient memory");
        return NULL;
    }
    basic->isReady = (srcTcid == CM_TCID_SLE_SMTC) ? false : true;
    SDF_DListHeadInit(&basic->cacheRxBuffs);
    return basic;
}

static void DTAP_InitTxWindow(DTAP_TxWindow_S *window, uint8_t size)
{
    window->size = size;
    window->nextTxSeq = 0;
    window->expectedAckSeq = 0;
    SDF_DListHeadInit(&window->txList);
}

static void DTAP_InitRxWindow(DTAP_RxWindow_S *window, uint8_t size)
{
    window->size = size;
    window->bufferSeq = 0;
    window->expectedTxSeq = 0;
    window->maxExpectedTxSeq = 0;
    SDF_DListHeadInit(&window->rxList);
}

static DTAP_Stream_Channel_S *DTAP_StreamChannelCreate(CM_TransChan_S *param)
{
    DTAP_Stream_Channel_S *stream = (DTAP_Stream_Channel_S *)SDF_MemZalloc(sizeof(DTAP_Stream_Channel_S));
    if (stream == NULL) {
        DTAP_LOGE("create dtap stream channel failed due to insufficient memory");
        return NULL;
    }
    if (param->config.streamMode.flushTimeout == 0 ||
        param->config.streamMode.flushTimeout > DTAP_STREAM_FLUSH_TIMEOUT) {
        stream->flushTimeout = DTAP_STREAM_FLUSH_TIMEOUT;
    } else {
        stream->flushTimeout = param->config.streamMode.flushTimeout;
    }
    if (param->config.streamMode.reorderTimeout == 0 ||
        param->config.streamMode.reorderTimeout > DTAP_STREAM_REORDER_TIMEOUT) {
        stream->reorderTimeout = DTAP_STREAM_REORDER_TIMEOUT;
    } else {
        stream->reorderTimeout = param->config.streamMode.reorderTimeout;
    }
    stream->crcInit = param->config.streamMode.crcInit;
    stream->expectedSeq = DTAP_FRAME_SEQ_INIT;
    stream->maxExpectedSeq = DTAP_FRAME_SEQ_INIT;
    stream->reorderTimerHandle = INVALID_TIMER_HANDLE;
    SDF_DListHeadInit(&stream->txList);
    SDF_DListHeadInit(&stream->rxList);
    return stream;
}

static void SetChannelCastMode(DTAP_Channel_S *channel, uint8_t castMode)
{
    if (castMode == CM_ACCESS_TRANS_MODE_UNICAST) {
        channel->castMode = DTAP_FRAME_UNICAST;
    } else if (castMode == CM_ACCESS_TRANS_MODE_DATA_MCST || castMode == CM_ACCESS_TRANS_MODE_FEEDBACK_MCST ||
        castMode == CM_ACCESS_TRANS_MODE_BIDI_MCST) {
        channel->castMode = DTAP_FRAME_MULTICAST;
    } else if (castMode == CM_ACCESS_TRANS_MODE_SEND_BCST || castMode == CM_ACCESS_TRANS_MODE_RECV_BCST) {
        channel->castMode = DTAP_FRAME_BROADCAST;
    } else {
        channel->castMode = DTAP_FRAME_UNICAST;
        DTAP_LOGE("invalid cast mode, set default unicast mode");
    }
}

static DTAP_ReliableChannel_S *DTAP_ReliableChannelCreate(DTAP_Channel_S *channel, CM_TransChan_S *param)
{
    DTAP_ReliableChannel_S *reliable = (DTAP_ReliableChannel_S *)SDF_MemZalloc(sizeof(DTAP_ReliableChannel_S));
    if (reliable == NULL) {
        DTAP_LOGE("create dtap channel failed due to insufficient memory");
        return NULL;
    }

    reliable->reorderTimeout = param->config.reliableMode.reorderTimeout == 0 ? DTAP_RELIABLE_REORDER_TIMEOUT :
        param->config.reliableMode.reorderTimeout;
    reliable->crcInit = param->config.reliableMode.crcInit;
    reliable->retransTimeout = param->config.reliableMode.retransTimeout == 0 ? DTAP_RELIABLE_RETRANS_TIMEOUT :
        param->config.reliableMode.retransTimeout;
    reliable->rspTimeout = param->config.reliableMode.rspTimeout == 0 ? DTAP_RELIABLE_RSP_TIMEOUT :
        param->config.reliableMode.rspTimeout;
    reliable->maxTxThreshold = param->config.reliableMode.maxTxThreshold;
    reliable->reorderTimer = INVALID_TIMER_HANDLE;
    reliable->retransTimer = INVALID_TIMER_HANDLE;
    reliable->rspTimer = INVALID_TIMER_HANDLE;
    reliable->isTxWindowFull = false;
    reliable->isDestroyMsgSent = false;
    // 通道建立时立即启动应答定时器
    uint32_t ret = DTAP_TransStartTimer(&reliable->rspTimer, reliable->rspTimeout,
        true, DTAP_RspTimerExpireProc, channel);
    if (ret != DTAP_SUCCESS) {
        SDF_MemFree(reliable);
        return NULL;
    }
    uint8_t window = param->config.reliableMode.txWindow;
    if (window == 0) {
        window = CM_CAP_WND;
    }
    DTAP_InitTxWindow(&reliable->txWindow, window);
    DTAP_InitRxWindow(&reliable->rxWindow, window);
    DTAP_LOGI("reorderTimeout: %hu, crcInit: 0x%04x, retransTimeout: %hu, rspTimeout: %hu, window: %hhu, "
        "maxTxThreshold: %hhu", reliable->reorderTimeout, reliable->crcInit, reliable->retransTimeout,
        reliable->rspTimeout, window, reliable->maxTxThreshold);
    return reliable;
}

static DTAP_Channel_S *DTAP_ChannelCreate(CM_TransChan_S *param)
{
    DTAP_Channel_S *channel = (DTAP_Channel_S *)SDF_MemZalloc(sizeof(DTAP_Channel_S));
    if (channel == NULL) {
        DTAP_LOGE("create dtap channel failed due to insufficient memory");
        return NULL;
    }

    if (param->srcTcid == CM_TCID_SLE_CMTC) {
        channel->priority = DTAP_PRIORITY_CMD;
    } else if (param->srcTcid <= CM_TCID_SLE_CUTC) {
        channel->priority = DTAP_PRIORITY_HIGH;
    } else if (param->srcTcid >= CM_TCID_BC_BEGIN && param->srcTcid <= CM_TCID_UC_END) {
        channel->priority = DTAP_PRIORITY_NORMAL;
    } else {
        channel->priority = DTAP_PRIORITY_NORMAL;
    }

    SDF_DListHeadInit(&channel->pktList);
    SDF_DListEntryInit(&channel->schedEntry);
    SDF_DListEntryInit(&channel->entry);
    channel->lcid = param->lcid;
    channel->srcTcid = param->srcTcid;
    channel->dstTcid = param->dstTcid;
    channel->mode = param->config.transMode;
    channel->mps = param->config.mps;
    channel->mtu = param->config.mtu;
    SetChannelCastMode(channel, param->castMode);
    if (channel->mode == CM_TRANS_MODE_STREAM) {
        channel->attr = DTAP_StreamChannelCreate(param);
    } else if (channel->mode == CM_TRANS_MODE_RELIABLE) {
        channel->attr = DTAP_ReliableChannelCreate(channel, param);
    } else if (channel->mode == CM_TRANS_MODE_BASIC) {
        channel->attr = DTAP_BasicChannelCreate(channel->srcTcid);
    } else {
        return channel;
    }
    if (channel->attr == NULL) {
        SDF_MemFree(channel);
        return NULL;
    }

    return channel;
}

static void DTAP_ChannelAdd(CM_TransChan_S *channel)
{
    DTAP_LOGI("add dtap channel, lcid: %hu, srcTcid: %hu, dstTcid: %hu, mode: %hhu, mtu: %hu, mps: %hu",
              channel->lcid, channel->srcTcid, channel->dstTcid, channel->config.transMode,
              channel->config.mtu, channel->config.mps);

    DTAP_Logic_Link_S *logicLink = DTAP_LogicLinkFind(channel->lcid);
    if (logicLink == NULL) {
        logicLink = DTAP_LogicLinkCreate(channel->lcid);
        if (logicLink == NULL) {
            return;
        }

        if (!SDF_MapMoveInsert(g_dtapLogicLinksMap, &logicLink->lcid, logicLink)) {
            DTAP_LOGE("insert dtap logic link to map failed");
            SDF_MemFree(logicLink);
            return;
        }
    }

    DTAP_Channel_S *dtapChannel = DTAP_ChannelFind(logicLink, channel->srcTcid);
    if (dtapChannel != NULL) {
        DTAP_LOGW("dtap channel has created before, delete first");
        SDF_DListElmDel(&logicLink->list, dtapChannel, entry);
        DTAP_ChannelDestroy(dtapChannel);
    }

    dtapChannel = DTAP_ChannelCreate(channel);
    if (dtapChannel == NULL) {
        if (SDF_DListIsEmpty(&logicLink->list)) {
            (void)SDF_MapErase(g_dtapLogicLinksMap, &logicLink->lcid);
        }
        return;
    }
    SDF_DListElmHeadInsert(&logicLink->list, dtapChannel, entry);
}

static void DTAP_ChannelDel(CM_TransChan_S *channel)
{
    DTAP_LOGI("del dtap channel, lcid: %hu, srcTcid: %hu, dstTcid: %hu, mode: %hhu, mtu: %hu, mps: %hu",
              channel->lcid, channel->srcTcid, channel->dstTcid, channel->config.transMode,
              channel->config.mtu, channel->config.mps);

    DTAP_Logic_Link_S *logicLink = DTAP_LogicLinkFind(channel->lcid);
    if (logicLink == NULL) {
        DTAP_LOGE("find dtap logic link failed");
        return;
    }

    DTAP_Channel_S *dtapChannel = DTAP_ChannelFind(logicLink, channel->srcTcid);
    if (dtapChannel == NULL) {
        DTAP_LOGE("find dtap channel failed");
        return;
    }
    DTAP_ChannelDown(channel->lcid, channel->srcTcid);
    SDF_DListElmDel(&logicLink->list, dtapChannel, entry);
    DTAP_ChannelDestroy(dtapChannel);
    if (SDF_DListIsEmpty(&logicLink->list)) {
        (void)SDF_MapErase(g_dtapLogicLinksMap, &logicLink->lcid);
    }
}

static void DTAP_ChannelsOperate(SDF_Vector_S *channelVector, void (*DTAP_ChannelPOptFunc)(CM_TransChan_S *channel))
{
    size_t count = channelVector->size;

    for (uint32_t i = 0; i < count; i++) {
        CM_TransChan_S *channel = SDF_VectorElementAt(channelVector, i);
        if (channel == NULL) {
            DTAP_LOGE("channel is null");
            continue;
        }
        DTAP_ChannelPOptFunc(channel);
    }
}

static void DTAP_ChannelStateChangeCbk(CM_TransChannelStateList_S *param)
{
    if (param == NULL || param->channelVector == NULL) {
        DTAP_LOGE("invalid channel state response parameter");
        return;
    }

    switch (param->result) {
        case CM_TRANS_CHANNEL_STATE_ACTIVATED:
            DTAP_ChannelsOperate(param->channelVector, DTAP_ChannelAdd);
            break;
        case CM_TRANS_CHANNEL_STATE_RELEASED:
            DTAP_ChannelsOperate(param->channelVector, DTAP_ChannelDel);
            break;
        default:
            DTAP_LOGW("invalid channel change result %hhu", param->result);
            break;
    }
}

static void DTAP_LinkCollabProc(CM_LogicLinkState_S *param)
{
    if (param->result == CM_LINK_STATE_CONNECTED || param->result == CM_LINK_STATE_DISCONNECTED) {
        uint32_t ret = COLLAB_PreAssignTransBuffer((uint8_t)CM_GetLogicLinkConnectedSize());
        DTAP_LOGI("COLLAB_PreAssignTransBuffer ret:%u", ret);
    }
}

static void DTAP_LogicLinkStateChangeCbk(CM_LogicLinkState_S *param)
{
    CHECK_AND_RETURN_LOG(DTAP_TAG, param != NULL, "param is null");
    DTAP_LinkCollabProc(param);
    if (param->result != CM_LINK_STATE_CONNECTED) {
        return;
    }

    DTAP_Logic_Link_S *logicLink = DTAP_LogicLinkFind(param->lcid);
    if (logicLink == NULL) {
        DTAP_LOGE("find dtap logic link failed");
        return;
    }

    DTAP_Channel_S *channel = NULL;
    SDF_DListHead_S *head = &logicLink->list;
    SDF_DListElmForeach(channel, head, entry) {
        if (channel->mode == CM_TRANS_MODE_BASIC) {
            DTAP_RecvBasicFrameContinue(channel);
        }
    }
}

uint32_t DTAP_ChannelInit(void)
{
    uint32_t ret;

    ret = DTAP_LogicLinkMapInit();
    if (ret != DTAP_SUCCESS) {
        return ret;
    }

    ret = DTAP_SchedulerInit();
    if (ret != DTAP_SUCCESS) {
        DTAP_LogicLinkMapDeInit();
        return ret;
    }

    ret = CM_RegTransChannelListener(DTAP_ChannelStateChangeCbk);
    if (ret != CM_SUCCESS) {
        (void)DTAP_SchedulerDeinit();
        DTAP_LogicLinkMapDeInit();
        return ret;
    }

    CM_LogicLinkCbks_S cbks = {0};
    cbks.moduleId = CM_MODULE_DTAP;
    cbks.logicLinkCbk = DTAP_LogicLinkStateChangeCbk;
    ret = CM_RegLogicLinkListener(&cbks);
    if (ret != CM_SUCCESS) {
        CM_UnRegTransChannelListener();
        (void)DTAP_SchedulerDeinit();
        DTAP_LogicLinkMapDeInit();
        return ret;
    }

    DTAP_LOGD("DTAP_ChannelInit success");
    return DTAP_SUCCESS;
}

void DTAP_ChannelDeInit(void)
{
    CM_UnRegLogicLinkListener(CM_MODULE_DTAP);
    CM_UnRegTransChannelListener();
    (void)DTAP_SchedulerDeinit();
    DTAP_LogicLinkMapDeInit();
    DTAP_LOGD("DTAP_ChannelDeInit success");
}