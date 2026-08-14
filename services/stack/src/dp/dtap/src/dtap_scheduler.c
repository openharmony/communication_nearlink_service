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

#include "dtap_scheduler.h"
#include <stdlib.h>
#include <string.h>
#include "securec.h"
#include "byte_codec.h"
#include "cm_dli_adapter.h"
#include "collab_ext_func_wrapper.h"
#include "cp_worker.h"
#include "dli.h"
#include "dli_callback.h"
#include "dli_errno.h"
#include "dli_layer.h"
#include "dpfwk_log.h"
#include "dtap.h"
#include "dtap_errno.h"
#include "dtap_tcid.h"
#include "sdf_dlist.h"
#include "sdf_mem.h"
#include "nlstk_public_define.h"

#define DTAP_PACKET_MAX_SIZE 10000
#define DTAP_SCHED_BLOCK_TIMEOUT_MS 1000
#define DTAP_SCHED_SUB(x, y) (((x) > (y)) ? ((x) - (y)) : 0)
#define DTAP_FRAGMENT_TCID TCID_MAX
#define DTAP_MIN_BUFFER_NUM 1

typedef struct {
    SDF_DListEntry_S entry;
    SDF_Buff_S *buff;
    uint32_t seq;
    bool isSplited;               // 是否已分片
} DTAP_PendingPacket;

typedef struct {
    SDF_DListEntry_S entry;
    SDF_DListHead_S channelList;  // DTAP_Channel_S，链表节点的内存由dtap_channel.c管理
    bool hasScheduled;            // 是否已调度
    uint16_t lcid;
    uint8_t priority;             // 即DTAP_ModuleType，值越小优先级越高
    uint32_t pktCnt;
} DTAP_LcidNode;

typedef struct {
    SDF_DListHead_S lcidList;     // DTAP_LcidNode
    uint8_t priority;             // 即DTAP_ModuleType，值越小优先级越高
    uint32_t pktCnt;              // 最大为DTAP_MODULE_QUEUE_MAX_SIZE
} DTAP_PriorityQueue;

typedef struct {
    SDF_DListEntry_S entry;
    uint16_t lcid;
    uint32_t quota;               // 该LCID的发送配额
    uint32_t sendNotAckPktCnt;    // 该LCID的发送但未确认的包数
} DTAP_LcidBufferNode;

static DTAP_PriorityQueue g_dtapScheduler[DTAP_PRIORITY_MAX] = {0};
static SDF_DListHead_S g_lcidBufferList = {{&g_lcidBufferList.list, &g_lcidBufferList.list}, 0};
static uint8_t g_sendNotAckPktCnt = 0;
static bool g_isInited = false;
static uint32_t g_pktSeq = 0;
static uint8_t g_apBufferNum = DTAP_MIN_BUFFER_NUM;
static DLI_AcbNumChangeCbk g_acbNumChangeCbk = NULL;

static void DTAP_SendCompleteCbk(uint16_t connHandle, uint8_t numCompletedPackets);
static void DTAP_ChannelDownProc(DTAP_PriorityQueue *q, uint16_t lcid, uint8_t srcTcid);
static uint32_t DTAP_PriorityQueuePush(DTAP_PriorityQueue *q, DTAP_Channel_S *transChan, uint32_t pktCnt);

static bool DTAP_AddLcidBufferNode(uint16_t connHandle)
{
    DTAP_LcidBufferNode *node = NULL;
    DTAP_LcidBufferNode *temp = NULL;
    SDF_DListElmSafeForeach(node, temp, &g_lcidBufferList, entry) {
        if (node->lcid == connHandle) {
            return false;
        }
    }
    node = (DTAP_LcidBufferNode *)SDF_MemZalloc(sizeof(DTAP_LcidBufferNode));
    if (node == NULL) {
        return false;
    }
    node->lcid = connHandle;
    node->sendNotAckPktCnt = 0;
    SDF_DListEntryInit(&node->entry);
    SDF_DListElmTailInsert(&g_lcidBufferList, node, entry);
    return true;
}

static bool DTAP_DeleteLcidBufferNode(uint16_t connHandle)
{
    DTAP_LcidBufferNode *node = NULL;
    DTAP_LcidBufferNode *temp = NULL;
    SDF_DListElmSafeForeach(node, temp, &g_lcidBufferList, entry) {
        if (node->lcid != connHandle) {
            continue;
        }
        SDF_DListElmDel(&g_lcidBufferList, node, entry);
        SDF_MemFree(node);
        return true;
    }
    return false;
}

static DTAP_LcidBufferNode *DTAP_GetLcidBufferNode(uint16_t lcid)
{
    DTAP_LcidBufferNode *node = NULL;
    SDF_DListElmForeach(node, &g_lcidBufferList, entry) {
        if (node->lcid != lcid) {
            continue;
        }
        return node;
    }
    return NULL;
}

static void DTAP_FreeLcidBufferNode(SDF_DListEntry_S *entry)
{
    DTAP_LcidBufferNode *node = (DTAP_LcidBufferNode *)entry;
    if (node == NULL) {
        return;
    }
    SDF_MemFree(node);
}

static void DTAP_RecalcLcidQuota(void)
{
    uint32_t lcidNums = SDF_DListCount(&g_lcidBufferList);
    uint32_t quota = (lcidNums == 0) ? 0 :
        (g_apBufferNum / lcidNums) == 0 ? DTAP_MIN_BUFFER_NUM : g_apBufferNum / lcidNums;
    uint32_t remainQuota = DTAP_SCHED_SUB(g_apBufferNum, (quota * lcidNums));
    DTAP_LcidBufferNode *node = NULL;
    SDF_DListElmForeach(node, &g_lcidBufferList, entry) {
        if (remainQuota > 0) {
            node->quota = quota + 1;
            remainQuota--;
            DTAP_LOGI("lcid %d, quota %d, sendNotAckPktCnt %d", node->lcid, node->quota, node->sendNotAckPktCnt);
            continue;
        }
        node->quota = quota;
        DTAP_LOGI("lcid %d, quota %d, sendNotAckPktCnt %d", node->lcid, node->quota, node->sendNotAckPktCnt);
    }
}

static void DTAP_DLIConnectCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    if (status != DLI_SUCCESS) {
        DTAP_LOGE("connect failed, status %d", status);
        return;
    }
    if (cmdRes == NULL || cmdRes->eventParameter == NULL) {
        DTAP_LOGE("param is null");
        return;
    }
    DLI_ConnectionCompleteEvt *param = (DLI_ConnectionCompleteEvt *)cmdRes->eventParameter;
    uint16_t connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
    if (DTAP_AddLcidBufferNode(connHandle)) {
        DTAP_RecalcLcidQuota();
    }
}

static void DTAP_DLIDisconnectCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    if (cmdRes == NULL || cmdRes->eventParameter == NULL) {
        DTAP_LOGE("param is null");
        return;
    }
    DLI_DisconnectEvt *param = (DLI_DisconnectEvt *)cmdRes->eventParameter;
    uint16_t connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
    if (DTAP_DeleteLcidBufferNode(connHandle)) {
        DTAP_RecalcLcidQuota();
    }
}

static void DTAP_DestroyPacket(SDF_DListEntry_S *entry)
{
    DTAP_PendingPacket *pkt = (DTAP_PendingPacket *)entry;
    if (pkt == NULL) {
        return;
    }
    DTAP_LOGD("destroy pending packet, seq %d", pkt->seq);
    SDF_MemFree(pkt);
}

static void DTAP_DestroyPacketAndBuff(SDF_DListEntry_S *entry)
{
    DTAP_PendingPacket *pkt = (DTAP_PendingPacket *)entry;
    if (pkt == NULL) {
        return;
    }
    DTAP_LOGD("destroy pending packet and buff, seq %d", pkt->seq);
    SDF_BuffFree(pkt->buff);
    SDF_MemFree(pkt);
}

static DTAP_PendingPacket *DTAP_CreatePacket(SDF_Buff_S *buff)
{
    DTAP_PendingPacket *pkt = (DTAP_PendingPacket *)SDF_MemZalloc(sizeof(DTAP_PendingPacket));
    if (pkt == NULL) {
        return NULL;
    }
    pkt->buff = buff;
    pkt->seq = g_pktSeq++;
    SDF_DListEntryInit(&pkt->entry);
    DTAP_LOGD("create pending packet success, seq %d", pkt->seq);
    return pkt;
}

static void DTAP_DestroyLcidNode(SDF_DListEntry_S *entry)
{
    DTAP_LcidNode *lcidNode = (DTAP_LcidNode *)entry;
    if (lcidNode == NULL) {
        return;
    }
    DTAP_LOGI("destroy lcid node , lcid %d, priority %d", lcidNode->lcid, lcidNode->priority);
    DTAP_Channel_S *channelNode = NULL;
    DTAP_Channel_S *temp = NULL;
    SDF_DListElmSafeForeach(channelNode, temp, &lcidNode->channelList, schedEntry) {
        DTAP_LOGI("delete channel node, priority %d, lcid %d, srcTcid %d", lcidNode->priority, channelNode->lcid,
            channelNode->srcTcid);
        SDF_DListElmDel(&lcidNode->channelList, channelNode, schedEntry);
        SDF_DListDestroy(&channelNode->pktList, DTAP_DestroyPacketAndBuff);
        SDF_DListHeadInit(&channelNode->pktList);
        if (channelNode->priority == DTAP_PRIORITY_FRAGMENT) {
            SDF_MemFree(channelNode);
        }
    }
    SDF_DListHeadInit(&lcidNode->channelList);
    SDF_MemFree(lcidNode);
}

static DTAP_LcidNode *DTAP_CreateLcidNode(uint16_t lcid, uint8_t priority)
{
    DTAP_LcidNode *lcidNode = (DTAP_LcidNode *)SDF_MemZalloc(sizeof(DTAP_LcidNode));
    if (lcidNode == NULL) {
        return NULL;
    }
    lcidNode->lcid = lcid;
    lcidNode->priority = priority;
    lcidNode->pktCnt = 0;
    SDF_DListEntryInit(&lcidNode->entry);
    SDF_DListHeadInit(&lcidNode->channelList);
    DTAP_LOGD("create lcid node success, lcid %d, priority %d", lcid, priority);
    return lcidNode;
}

static DTAP_Channel_S *DTAP_GetOrCreateFragmentChannel(uint16_t lcid)
{
    DTAP_LcidNode *lcidNode = NULL;
    DTAP_LcidNode *temp = NULL;
    SDF_DListElmSafeForeach(lcidNode, temp, &g_dtapScheduler[DTAP_PRIORITY_FRAGMENT].lcidList, entry) {
        if (lcidNode->lcid != lcid) {
            continue;
        }
        DTAP_Channel_S *channelNode = NULL;
        DTAP_Channel_S *temp1 = NULL;
        SDF_DListElmSafeForeach(channelNode, temp1, &lcidNode->channelList, schedEntry) {
            if (channelNode->lcid != lcid || channelNode->srcTcid != DTAP_FRAGMENT_TCID) {
                continue;
            }
            return channelNode;
        }
    }
    DTAP_Channel_S *channel = (DTAP_Channel_S *)SDF_MemZalloc(sizeof(DTAP_Channel_S));
    if (channel == NULL) {
        DTAP_LOGE("malloc dtap channel failed, lcid %d", lcid);
        return NULL;
    }
    SDF_DListHeadInit(&channel->pktList);
    SDF_DListEntryInit(&channel->schedEntry);
    channel->priority = DTAP_PRIORITY_FRAGMENT;
    channel->lcid = lcid;
    channel->srcTcid = DTAP_FRAGMENT_TCID;
    channel->dstTcid = DTAP_FRAGMENT_TCID;
    DTAP_LOGD("create fragment channel success, lcid %d", lcid);
    return channel;
}

static void DTAP_SetApBufferNum(uint8_t bufferNum)
{
    DTAP_LOGI("ap buffer num is setted:%hhu", bufferNum);
    g_apBufferNum = bufferNum;
    DTAP_RecalcLcidQuota();
}

static uint8_t DTAP_DLIAcbNumGet(void)
{
    return (uint8_t)DLI_DataNumGet(ACB_DATA_TYPE);
}

static void DTAP_DLIDataNumChangecbk(DLI_DataType type, uint16_t dataNum)
{
    if (type == ACB_DATA_TYPE) {
        DTAP_SetApBufferNum(dataNum);
        if (g_acbNumChangeCbk != NULL) {
            g_acbNumChangeCbk(dataNum);
        }
    }
}

static void DTAP_DLIAcbNumChangeRegister(DLI_AcbNumChangeCbk cbk)
{
    g_acbNumChangeCbk = cbk;
}

static void DTAP_SchedulerInitInner(void *args)
{
    if (g_isInited) {
        DTAP_LOGI("scheduler is already init");
        return;
    }
    for (uint8_t i = DTAP_PRIORITY_FRAGMENT; i < DTAP_PRIORITY_MAX; i++) {
        SDF_DListHeadInit(&g_dtapScheduler[i].lcidList);
        g_dtapScheduler[i].priority = i;
        g_dtapScheduler[i].pktCnt = 0;
    }
    SDF_DListHeadInit(&g_lcidBufferList);
    uint32_t ret = DLI_RegNOCPEventCbk(DLI_REG_MODULE_DTAP, DTAP_SendCompleteCbk);
    if (ret != DLI_SUCCESS) {
        DTAP_LOGE("DLI_RegNOCPEventCbk failed, ret %d", ret);
    }
    ret = CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_DTAP, CM_DLI_ADAPTER_CONNECT, DTAP_DLIConnectCbk);
    if (ret != DLI_SUCCESS) {
        DTAP_LOGE("register dli connect cbk failed, ret %d", ret);
    }
    ret = CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_DTAP, CM_DLI_ADAPTER_DISCONNECT, DTAP_DLIDisconnectCbk);
    if (ret != DLI_SUCCESS) {
        DTAP_LOGE("register dli disconnect cbk failed, ret %d", ret);
    }
    DLI_DataNumChangeRegister(DTAP_DLIDataNumChangecbk);
    COLLAB_TransFuncExt transFunc = {};
    transFunc.setApBufferNum = DTAP_SetApBufferNum;
    transFunc.dliAcbNumGet = DTAP_DLIAcbNumGet;
    transFunc.dliAcbNumChangeRegister = DTAP_DLIAcbNumChangeRegister;
    ret = COLLAB_TransFuncRegister(&transFunc);
    if (ret != NLSTK_ERRCODE_SUCCESS) {
        DTAP_LOGE("collab trans init failed, ret %d", ret);
    }
    g_sendNotAckPktCnt = 0;
    g_isInited = true;
    DTAP_LOGI("init success");
}

uint32_t DTAP_SchedulerInit(void)
{
    DTAP_LOGI("enter");
    uint32_t ret = CP_PostTaskBlocked(DTAP_SchedulerInitInner, NULL, NULL, DTAP_SCHED_BLOCK_TIMEOUT_MS);
    if (ret != NLSTK_OK) {
        CM_LOGE("CP_PostTaskBlocked failed, ret:0x%08x", ret);
        return DTAP_TRANS_CP_POST_ERR;
    }
    return DTAP_SUCCESS;
}

static void DTAP_SchedulerDeinitInner(void *args)
{
    if (!g_isInited) {
        DTAP_LOGI("scheduler is already deinit");
        return;
    }
    uint32_t ret = CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_DTAP, CM_DLI_ADAPTER_DISCONNECT);
    if (ret != DLI_SUCCESS) {
        DTAP_LOGE("unregister dli disconnect cbk failed, ret %d", ret);
    }
    ret = CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_DTAP, CM_DLI_ADAPTER_CONNECT);
    if (ret != DLI_SUCCESS) {
        DTAP_LOGE("unregister dli connect cbk failed, ret %d", ret);
    }
    ret = DLI_UnregNOCPEventCbk(DLI_REG_MODULE_DTAP);
    if (ret != DLI_SUCCESS) {
        DTAP_LOGE("DLI_UnregNOCPEventCbk failed, ret %d", ret);
    }
    SDF_DListDestroy(&g_lcidBufferList, DTAP_FreeLcidBufferNode);
    SDF_DListHeadInit(&g_lcidBufferList);
    for (uint8_t i = DTAP_PRIORITY_FRAGMENT; i < DTAP_PRIORITY_MAX; i++) {
        SDF_DListDestroy(&g_dtapScheduler[i].lcidList, DTAP_DestroyLcidNode);
        SDF_DListHeadInit(&g_dtapScheduler[i].lcidList);
        g_dtapScheduler[i].pktCnt = 0;
    }
    DTAP_LOGI("deinit success");
    g_isInited = false;
}

uint32_t DTAP_SchedulerDeinit(void)
{
    DTAP_LOGI("enter");
    uint32_t ret = CP_PostTaskBlocked(DTAP_SchedulerDeinitInner, NULL, NULL, DTAP_SCHED_BLOCK_TIMEOUT_MS);
    if (ret != NLSTK_OK) {
        CM_LOGE("CP_PostTaskBlocked failed, ret:0x%08x", ret);
        return DTAP_TRANS_CP_POST_ERR;
    }
    return DTAP_SUCCESS;
}

static void DTAP_ChannelDownProc(DTAP_PriorityQueue *q, uint16_t lcid, uint8_t srcTcid)
{
    DTAP_LcidNode *lcidNode = NULL;
    DTAP_LcidNode *temp = NULL;
    SDF_DListElmSafeForeach(lcidNode, temp, &q->lcidList, entry) {
        if (lcidNode->lcid != lcid) {
            continue;
        }
        DTAP_Channel_S *channelNode = NULL;
        DTAP_Channel_S *temp1 = NULL;
        SDF_DListElmSafeForeach(channelNode, temp1, &lcidNode->channelList, schedEntry) {
            if (channelNode->lcid != lcid || channelNode->srcTcid != srcTcid) {
                continue;
            }
            SDF_DListElmDel(&lcidNode->channelList, channelNode, schedEntry);
            lcidNode->pktCnt = DTAP_SCHED_SUB(lcidNode->pktCnt, (uint32_t)SDF_DListCount(&channelNode->pktList));
            q->pktCnt = DTAP_SCHED_SUB(q->pktCnt, (uint32_t)SDF_DListCount(&channelNode->pktList));
            SDF_DListDestroy(&channelNode->pktList, DTAP_DestroyPacketAndBuff);
            SDF_DListHeadInit(&channelNode->pktList);
            if (q->priority == DTAP_PRIORITY_FRAGMENT) {
                SDF_MemFree(channelNode);
            }
            DTAP_LOGI("delete channel node, priority %d, lcid %d, srcTcid %d", q->priority, lcid, srcTcid);
            break;
        }
        if (SDF_DListIsEmpty(&lcidNode->channelList)) {
            SDF_DListElmDel(&q->lcidList, lcidNode, entry);
            DTAP_DestroyLcidNode((SDF_DListEntry_S *)lcidNode);
            DTAP_LOGI("delete lcid node, priority %d, lcid %d", q->priority, lcid);
        }
        break;
    }
}

void DTAP_ChannelDown(uint16_t lcid, uint8_t srcTcid)
{
    DTAP_LOGI("enter");
    for (uint8_t i = DTAP_PRIORITY_FRAGMENT; i < DTAP_PRIORITY_MAX; i++) {
        DTAP_PriorityQueue *q = &g_dtapScheduler[i];
        uint8_t tcid = (i == DTAP_PRIORITY_FRAGMENT) ? DTAP_FRAGMENT_TCID : srcTcid;
        DTAP_ChannelDownProc(q, lcid, tcid);
        if (SDF_DListIsEmpty(&q->lcidList)) {
            SDF_DListHeadInit(&q->lcidList);
        }
    }
}

static bool DTAP_PriorityQueuePeek(DTAP_PriorityQueue *q, DTAP_LcidNode *lcidNode, DTAP_Channel_S **channel)
{
    if (lcidNode->pktCnt == 0) {
        DTAP_LOGD("lcidNode->pktCnt is empty, continue, lcid %d, priority %d", lcidNode->lcid, q->priority);
        return false;
    }
    DTAP_Channel_S *channelNode = NULL;
    SDF_DListElmForeach(channelNode, &lcidNode->channelList, schedEntry) {
        if (SDF_DListIsEmpty(&channelNode->pktList)) {
            DTAP_LOGD("channelNode->pktList is empty, continue, priority %d, lcid %d, srcTcid %d", q->priority,
                channelNode->lcid, channelNode->srcTcid);
            continue;
        }
        *channel = channelNode;
        DTAP_LOGD("peek success, priority %d, lcid %d, srcTcid %d, dstTcid %d", q->priority, channelNode->lcid,
            channelNode->srcTcid, channelNode->dstTcid);
        return true;
    }
    DTAP_LOGI("no packet to send, pktCnt %d", q->pktCnt);
    return false;
}

static void DTAP_PriorityQueuePop(DTAP_PriorityQueue *q, DTAP_LcidNode *lcidNode,
    DTAP_Channel_S *channelNode, DTAP_PendingPacket *pkt)
{
    uint32_t seq = pkt->seq;
    if (!SDF_DListIsEmpty(&channelNode->pktList)) {
        SDF_DListElmDel(&channelNode->pktList, pkt, entry);
    }
    DTAP_DestroyPacket((SDF_DListEntry_S *)pkt);

    lcidNode->pktCnt = DTAP_SCHED_SUB(lcidNode->pktCnt, 1);
    q->pktCnt = DTAP_SCHED_SUB(q->pktCnt, 1);
    DTAP_LOGD("pop success, priority %d, lcid %d, srcTcid %d, queue pktCnt %d, lcid pktCnt %d, pkt seq %d", q->priority,
        channelNode->lcid, channelNode->srcTcid, q->pktCnt, lcidNode->pktCnt, seq);
    // 已调度发送过的非空channel，移动到链表尾部，保证公平性
    if (!SDF_DListIsEmpty(&channelNode->pktList)) {
        SDF_DListElmDel(&lcidNode->channelList, channelNode, schedEntry);
        SDF_DListElmTailInsert(&lcidNode->channelList, channelNode, schedEntry);
    }
}

static bool DTAP_CanSend(const DTAP_LcidBufferNode *node)
{
    return node == NULL ? false : g_sendNotAckPktCnt >= g_apBufferNum ? false : node->sendNotAckPktCnt < node->quota;
}

static bool DTAP_SplitData(uint16_t lcid, SDF_Buff_S *buff,
    SDF_Buff_S *fragmentBuf[], uint32_t fragmentCnt)
{
    uint32_t fragmentLen = DLI_GetFragmentMaxLen();
    for (uint32_t i = 0; i < fragmentCnt; i++) {
        fragmentBuf[i] = SDF_BuffNew(fragmentLen);
        if (fragmentBuf[i] == NULL) {
            DTAP_LOGE("create fragment buf failed");
            for (uint32_t j = 0; j < i; j++) {
                SDF_BuffFree(fragmentBuf[j]);
            }
            return false;
        }
    }
    DLI_DataStru dliData = { lcid, DLI_DATATYPE_ACB, 0, 0, buff };
    uint32_t ret = DLI_SplitData(&dliData, fragmentBuf, fragmentCnt);
    if (ret != DLI_SUCCESS) {
        DTAP_LOGE("split data failed, ret %d", ret);
        for (uint32_t i = 0; i < fragmentCnt; i++) {
            SDF_BuffFree(fragmentBuf[i]);
        }
        return false;
    }
    return true;
}

static DLI_DataStru *DTAP_CreateDataStru(uint16_t lcid, uint16_t type,
    uint8_t ts, uint8_t prio, SDF_Buff_S *buf)
{
    DLI_DataStru *dataInfo = (DLI_DataStru*)SDF_MemZalloc(sizeof(DLI_DataStru));
    if (dataInfo == NULL) {
        DTAP_LOGE("DLI_DefaultDataStruCreate SDF_MemZalloc failed");
        return NULL;
    }
    dataInfo->buf = buf;
    dataInfo->lcid = lcid;
    dataInfo->prio = prio;
    dataInfo->ts = ts;
    dataInfo->type = type;
    return dataInfo;
}

static bool DTAP_SendData(uint16_t lcid, SDF_Buff_S *buff)
{
    uint64_t buffLen = SDF_DataLenGet(buff);
    DLI_DataStru *dliData = DTAP_CreateDataStru(lcid, DLI_DATATYPE_ACB, 0, 0, buff);
    if (dliData == NULL) {
        DTAP_LOGE("malloc dli data failed, lcid %d, buff len %llu", lcid, buffLen);
        return false;
    }
    uint32_t ret = DLI_DataSend(dliData);
    DTAP_LOGD("dli send data, ret %d, lcid %d, buff len %llu", ret, lcid, buffLen);
    if (ret != DLI_SUCCESS) {
        SDF_MemFree(dliData);
        return false;
    }
    return true;
}

static bool DTAP_SaveFragmentData(uint16_t lcid, SDF_Buff_S *buff[], uint32_t remainBuffCnt)
{
    DTAP_PendingPacket *pendingPkt[remainBuffCnt];
    for (uint32_t i = 0; i < remainBuffCnt; i++) {
        DTAP_PendingPacket *pkt = DTAP_CreatePacket(buff[i]);
        if (pkt == NULL) {
            for (uint32_t j = 0; j < i; j++) {
                DTAP_DestroyPacket((SDF_DListEntry_S *)pendingPkt[j]);
            }
            DTAP_LOGE("malloc pending packet failed, i %d, remainBuffCnt %d", i, remainBuffCnt);
            return false;
        }
        pkt->isSplited = true;
        pendingPkt[i] = pkt;
    }

    DTAP_Channel_S *fragmentChannel = DTAP_GetOrCreateFragmentChannel(lcid);
    if (fragmentChannel == NULL) {
        for (uint32_t i = 0; i < remainBuffCnt; i++) {
            DTAP_DestroyPacket((SDF_DListEntry_S *)pendingPkt[i]);
        }
        DTAP_LOGE("get fragment channel failed, lcid %d", lcid);
        return false;
    }

    for (uint32_t i = 0; i < remainBuffCnt; i++) {
        SDF_DListElmTailInsert(&fragmentChannel->pktList, pendingPkt[i], entry);
    }

    uint32_t ret = DTAP_PriorityQueuePush(&g_dtapScheduler[fragmentChannel->priority], fragmentChannel, remainBuffCnt);
    if (ret != DTAP_SUCCESS) {
        DTAP_LOGE("push pending packet failed, priority %d, lcid %d, srcTcid %d", fragmentChannel->priority,
            lcid, fragmentChannel->srcTcid);
        SDF_DListDestroy(&fragmentChannel->pktList, DTAP_DestroyPacket);
        SDF_MemFree(fragmentChannel);
        return false;
    }
    DTAP_LOGD("save fragment data success, lcid %d, remainBuffCnt %d", lcid, remainBuffCnt);
    return true;
}

static bool DTAP_ScheduleLcid(DTAP_PriorityQueue *q, DTAP_LcidNode *lcidNode, DTAP_LcidBufferNode *node)
{
    DTAP_Channel_S *channelNode = NULL;
    while (DTAP_CanSend(node) && DTAP_PriorityQueuePeek(q, lcidNode, &channelNode)) {
        DTAP_PendingPacket *pkt = (DTAP_PendingPacket *)SDF_DListFirst(&channelNode->pktList);

        DTAP_LOGD("send start, queue pktCnt %d, lcid pktCnt %d, channel pktCnt %d, priority %d, lcid %d, "
            "srcTcid %d, dstTcid %d, pkt seq %d, pkt len %d", q->pktCnt, lcidNode->pktCnt,
            SDF_DListCount(&channelNode->pktList), q->priority, channelNode->lcid, channelNode->srcTcid,
            channelNode->dstTcid, pkt->seq, SDF_DataLenGet(pkt->buff));

        if (pkt->isSplited) {
            if (DTAP_SendData(channelNode->lcid, pkt->buff)) {
                DTAP_PriorityQueuePop(q, lcidNode, channelNode, pkt);
                node->sendNotAckPktCnt++;
                g_sendNotAckPktCnt++;
                continue;
            } else {
                DTAP_LOGD("send data failed, priority %d, lcid %d, srcTcid %d, len %d", q->priority,
                    channelNode->lcid, channelNode->srcTcid, SDF_DataLenGet(pkt->buff));
                return false;
            }
        }

        uint32_t fragmentCnt = DLI_GetDataFragmentNums(pkt->buff);
        SDF_Buff_S *fragmentBuf[fragmentCnt];
        if (!DTAP_SplitData(channelNode->lcid, pkt->buff, fragmentBuf, fragmentCnt)) {
            return false;
        }
        SDF_BuffFree(pkt->buff);
        uint32_t sendCnt = 0;
        for (; DTAP_CanSend(node) && sendCnt < fragmentCnt; sendCnt++, node->sendNotAckPktCnt++) {
            if (!DTAP_SendData(channelNode->lcid, fragmentBuf[sendCnt])) {
                // 发送失败，暂停发送，等待下次继续发送
                DTAP_LOGD("send data failed, priority %d, lcid %d, srcTcid %d, len %d", q->priority,
                    channelNode->lcid, channelNode->srcTcid, SDF_DataLenGet(fragmentBuf[sendCnt]));
                break;
            }
            g_sendNotAckPktCnt++;
        }

        DTAP_PriorityQueuePop(q, lcidNode, channelNode, pkt);
        if (sendCnt >= fragmentCnt) {
            continue;
        }
        if (!DTAP_SaveFragmentData(channelNode->lcid, &fragmentBuf[sendCnt], (fragmentCnt - sendCnt))) {
            for (uint32_t i = sendCnt; i < fragmentCnt; i++) {
                SDF_BuffFree(fragmentBuf[i]);
            }
            DTAP_LOGE("save fragment channel failed, lcid %d", channelNode->lcid);
            return false;
        }
        break;
    }
    return true;
}

static bool DTAP_SchedulePriority(DTAP_PriorityQueue *q)
{
    bool ret = true;
    DTAP_LcidNode *lcidNode = NULL;
    SDF_DListElmForeach(lcidNode, &q->lcidList, entry) {
        DTAP_LcidBufferNode *node = DTAP_GetLcidBufferNode(lcidNode->lcid);
        if (!DTAP_CanSend(node)) {
            continue;
        }
        lcidNode->hasScheduled = true;
        if (!DTAP_ScheduleLcid(q, lcidNode, node)) {
            ret = false;
            break;
        }
    }
    lcidNode = NULL;
    DTAP_LcidNode *temp = NULL;
    SDF_DListElmSafeForeach(lcidNode, temp, &q->lcidList, entry) {
        if (!lcidNode->hasScheduled) {
            continue;
        }
        lcidNode->hasScheduled = false;
        SDF_DListElmDel(&q->lcidList, lcidNode, entry);
        SDF_DListElmTailInsert(&q->lcidList, lcidNode, entry);
    }
    return ret;
}

static void DTAP_SchedulerRun(void)
{
    for (uint8_t i = DTAP_PRIORITY_FRAGMENT; i < DTAP_PRIORITY_MAX; i++) {
        DTAP_PriorityQueue *q = &g_dtapScheduler[i];
        DTAP_LOGD("enter, g_sendNotAckPktCnt %u, q priority %u, q pktCnt %u",
            g_sendNotAckPktCnt, q->priority, q->pktCnt);
        if (g_sendNotAckPktCnt >= g_apBufferNum) {
            DTAP_LOGD("chip buffer is full, g_sendNotAckPktCnt %d", g_sendNotAckPktCnt);
            return;
        }
        if (q->pktCnt == 0) {
            DTAP_LOGD("queue is empty, priority %d, queue pktCnt %d", q->priority, q->pktCnt);
            continue;
        }
        if (!DTAP_SchedulePriority(q)) {
            return;
        }
    }
}

static uint32_t DTAP_PriorityQueuePush(DTAP_PriorityQueue *q, DTAP_Channel_S *transChan, uint32_t pktCnt)
{
    DTAP_LOGD("enter, q size %d, priority %d, lcid %d, srcTcid %d", SDF_DListCount(&q->lcidList), q->priority,
        transChan->lcid, transChan->srcTcid);
    bool found = false;
    DTAP_LcidNode *lcidNode = NULL;
    if (!SDF_DListIsEmpty(&q->lcidList)) {
        SDF_DListElmForeach(lcidNode, &q->lcidList, entry) {
            if (lcidNode->lcid == transChan->lcid) {
                found = true;
                break;
            }
        }
    }
    if (!found) {
        DTAP_LOGD("not found lcid node, malloc new lcid node");
        lcidNode = DTAP_CreateLcidNode(transChan->lcid, q->priority);
        if (lcidNode == NULL) {
            DTAP_LOGE("malloc lcid node failed");
            return DTAP_TRANS_MALLOC_ERR;
        }
        SDF_DListElmTailInsert(&lcidNode->channelList, transChan, schedEntry);
        DTAP_LOGI("insert channel node to queue, priority %d, lcid %d, srcTcid %d, dstTcid %d", q->priority,
            transChan->lcid, transChan->srcTcid, transChan->dstTcid);

        SDF_DListElmTailInsert(&q->lcidList, lcidNode, entry);
        DTAP_LOGD("insert lcid node to queue, priority %d, lcid %d", q->priority, lcidNode->lcid);
    } else {
        DTAP_LOGD("found lcid node %d, begin foreach channel node", lcidNode->lcid);
        found = false;
        DTAP_Channel_S *channelNode = NULL;
        SDF_DListElmForeach(channelNode, &lcidNode->channelList, schedEntry) {
            if (channelNode->lcid == transChan->lcid && channelNode->srcTcid == transChan->srcTcid) {
                found = true;
                break;
            }
        }
        if (!found) {
            SDF_DListElmTailInsert(&lcidNode->channelList, transChan, schedEntry);
            DTAP_LOGI("insert channel node to queue, priority %d, lcid %d, srcTcid %d, dstTcid %d", q->priority,
                transChan->lcid, transChan->srcTcid, transChan->dstTcid);
        }
    }
    lcidNode->pktCnt += pktCnt;
    q->pktCnt += pktCnt;
    DTAP_LOGD("push success, priority %d, lcid %d, srcTcid %d, queue pktCnt %d, lcid pktCnt %d", q->priority,
        transChan->lcid, transChan->srcTcid, q->pktCnt, lcidNode->pktCnt);
    return DTAP_SUCCESS;
}

uint32_t DTAP_DataSendWithPriority(DTAP_Channel_S *transChan, SDF_Buff_S *buff)
{
    DTAP_LOGD("enter");

    if (!g_isInited) {
        DTAP_LOGE("scheduler is not init, return");
        return DTAP_TRANS_INIT_ERR;
    }

    if (transChan == NULL || buff == NULL) {
        DTAP_LOGE("transChan or buff is NULL");
        return DTAP_TRANS_INVALID_PARAM_ERR;
    }

    if (transChan->priority >= DTAP_PRIORITY_MAX) {
        DTAP_LOGE("priority type is invalid, priority: %d", transChan->priority);
        return DTAP_TRANS_INVALID_MODULE_TYPE;
    }

    DTAP_PriorityQueue *q = &g_dtapScheduler[transChan->priority];
    if (q->priority != DTAP_PRIORITY_CMD && q->pktCnt >= DTAP_PACKET_MAX_SIZE) {
        DTAP_LOGE("queue is full, priority: %d, lcid: %d, srcTcid: %d", q->priority, transChan->lcid,
            transChan->srcTcid);
        return DTAP_TRANS_EXCEED_MAX_ERR;
    }

    DTAP_PendingPacket *pkt = DTAP_CreatePacket(buff);
    if (pkt == NULL) {
        DTAP_LOGE("malloc pending packet failed");
        return DTAP_TRANS_MALLOC_ERR;
    }

    uint32_t ret = DTAP_PriorityQueuePush(q, transChan, 1);
    if (ret != DTAP_SUCCESS) {
        DTAP_DestroyPacket((SDF_DListEntry_S *)pkt);
        DTAP_LOGE("push pending packet failed, priority %d, lcid %d, srcTcid %d", q->priority, transChan->lcid,
            transChan->srcTcid);
        return ret;
    }

    SDF_DListElmTailInsert(&transChan->pktList, pkt, entry);
    DTAP_LOGD("insert pending packet to channel node success, priority %d, lcid %d, srcTcid %d, dstTcid %d, pkt seq %d",
        q->priority, transChan->lcid, transChan->srcTcid, transChan->dstTcid, pkt->seq);

    DTAP_SchedulerRun();

    return DTAP_SUCCESS;
}

static void DTAP_SendCompleteCbk(uint16_t connHandle, uint8_t numCompletedPackets)
{
    DTAP_LOGD("enter, connHandle %d, numCompletedPackets %d", connHandle, numCompletedPackets);
    g_sendNotAckPktCnt = DTAP_SCHED_SUB(g_sendNotAckPktCnt, numCompletedPackets);
    COLLAB_ContinueAssignTransBuffer(g_sendNotAckPktCnt);
    DTAP_LcidBufferNode *node = DTAP_GetLcidBufferNode(connHandle);
    if (node == NULL) {
        DTAP_LOGE("connHandle %u is not exist, completed packet num is %u", connHandle, numCompletedPackets);
        return;
    }
    node->sendNotAckPktCnt = DTAP_SCHED_SUB(node->sendNotAckPktCnt, numCompletedPackets);
    DTAP_SchedulerRun();
}
