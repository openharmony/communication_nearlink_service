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
#include <inttypes.h>
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
#include "time_utils.h"

#define DTAP_PACKET_MAX_SIZE 10000
#define DTAP_SCHED_BLOCK_TIMEOUT_MS 1000
#define DTAP_SCHED_SUB(x, y) (((x) > (y)) ? ((x) - (y)) : 0)
#define DTAP_FRAGMENT_TCID TCID_MAX
#define DTAP_MIN_BUFFER_NUM 1
#define DTAP_RATE_CALC_INTERVAL_MS 10000
#define DTAP_RATE_PRINT_MIN_VALUE 1.0f
#define DTAP_MS_PER_SEC 1000
#define DTAP_BYTES_PER_KB 1024

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
        DTAP_LOGE("lcid %hu, malloc lcid buffer node failed", connHandle);
        return false;
    }
    node->lcid = connHandle;
    node->quota = 0;
    node->lastQuota = 0;
    node->sendNotAckPktCnt = 0;
    node->queuedPktCnt = 0;
    node->windowBytes = 0;
    node->lastRateCalcTime = 0;
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

DTAP_LcidBufferNode *DTAP_GetLcidBufferNode(uint16_t lcid)
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

static void DTAP_DecLcidQueuedPktCnt(uint16_t lcid, uint32_t cnt)
{
    DTAP_LcidBufferNode *node = DTAP_GetLcidBufferNode(lcid);
    if (node != NULL) {
        node->queuedPktCnt = DTAP_SCHED_SUB(node->queuedPktCnt, cnt);
    }
}

static void DTAP_CalcLcidSendRate(DTAP_LcidBufferNode *node, uint64_t pktBytes)
{
    node->windowBytes += pktBytes;
    uint64_t now = DP_GetMonoTimeMs();
    if (node->lastRateCalcTime == 0) {
        node->lastRateCalcTime = now;
        return;
    }

    uint64_t elapsed = DTAP_SCHED_SUB(now, node->lastRateCalcTime);
    if (elapsed < DTAP_RATE_CALC_INTERVAL_MS) {
        return;
    }

    double rateKB = (double)node->windowBytes / elapsed * DTAP_MS_PER_SEC / DTAP_BYTES_PER_KB;
    node->windowBytes = 0;
    node->lastRateCalcTime = now;
    if (rateKB > DTAP_RATE_PRINT_MIN_VALUE) {
        DTAP_LOGI("connHandle %hu, average sendRate %.2f KB/s in %" PRIu64 "s",
            node->lcid, rateKB, elapsed / DTAP_MS_PER_SEC);
    }
}

static void DTAP_FreeLcidBufferNode(SDF_DListEntry_S *entry)
{
    DTAP_LcidBufferNode *node = (DTAP_LcidBufferNode *)entry;
    if (node == NULL) {
        return;
    }
    SDF_MemFree(node);
}

// 统计所有链路待发送pkt之和，无需求时返回0
static uint64_t DTAP_CalcTotalQueuedPktCnt(void)
{
    uint64_t total = 0;
    DTAP_LcidBufferNode *node = NULL;
    SDF_DListElmForeach(node, &g_lcidBufferList, entry) {
        total += node->queuedPktCnt;
    }
    return total;
}

// 按比例分配LCID配额：按比例截断分配 + 最大余数法补齐， 保证Σbonus=bonusPool
// 内部函数，调用前保证totalQueuedPktCnt不为0
static void DTAP_AllocLcidQuota(uint64_t totalQueuedPktCnt)
{
    uint32_t lcidNums = SDF_DListCount(&g_lcidBufferList);
    uint8_t bonusPool = (g_apBufferNum > lcidNums * DTAP_MIN_BUFFER_NUM) ?
        (g_apBufferNum - lcidNums * DTAP_MIN_BUFFER_NUM) : 0;
    // 按比例截断分配
    uint8_t sumBonus = 0;
    DTAP_LcidBufferNode *node = NULL;
    SDF_DListElmForeach(node, &g_lcidBufferList, entry) {
        uint8_t bonus = (uint64_t)bonusPool * node->queuedPktCnt / totalQueuedPktCnt;
        node->quota = DTAP_MIN_BUFFER_NUM + bonus;
        sumBonus += bonus;
    }
    // 最大余数法补齐：选frac最大的未补齐节点各+1
    for (uint8_t rem = bonusPool - sumBonus; rem > 0; rem--) {
        uint64_t maxFrac = 0;
        DTAP_LcidBufferNode *maxNode = NULL;
        SDF_DListElmForeach(node, &g_lcidBufferList, entry) {
            uint64_t product = (uint64_t)bonusPool * node->queuedPktCnt;
            uint8_t floorBonus = product / totalQueuedPktCnt;
            // quota - MIN_BUFFER_NUM 为该节点已分到的bonus；若已超过向下取整值 floorBonus，
            // 说明本轮已被补齐过(+1)，跳过以保证补齐阶段每个节点最多+1
            if (node->quota - DTAP_MIN_BUFFER_NUM > floorBonus) {
                continue;
            }
            uint64_t frac = product % totalQueuedPktCnt;
            if (frac > maxFrac) {
                maxFrac = frac;
                maxNode = node;
            }
        }
        if (maxNode != NULL) {
            maxNode->quota++;
        }
    }
}

static void DTAP_RecalcLcidQuota(void)
{
    DTAP_LcidBufferNode *node = NULL;
    uint64_t totalQueuedPktCnt = DTAP_CalcTotalQueuedPktCnt();
    if (totalQueuedPktCnt != 0) {
        DTAP_AllocLcidQuota(totalQueuedPktCnt);
        SDF_DListElmForeach(node, &g_lcidBufferList, entry) {
            if (node->quota != node->lastQuota) {
                DTAP_LOGD("lcid %hu, quota %u, queued %u, sendNotAck %u", node->lcid, node->quota,
                    node->queuedPktCnt, node->sendNotAckPktCnt);
                node->lastQuota = node->quota;
            }
        }
        return;
    }

    SDF_DListElmForeach(node, &g_lcidBufferList, entry) {
        node->quota = DTAP_MIN_BUFFER_NUM;
        if (node->quota != node->lastQuota) {
            DTAP_LOGD("lcid %hu, quota %u, queued %u, sendNotAck %u", node->lcid, node->quota,
                node->queuedPktCnt, node->sendNotAckPktCnt);
            node->lastQuota = node->quota;
        }
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
    DTAP_LOGI("g_sendNotAckPktCnt: %hhu, g_apBufferNum: %hhu", g_sendNotAckPktCnt, g_apBufferNum);
}

static void DTAP_DLIDisconnectCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    if (cmdRes == NULL || cmdRes->eventParameter == NULL) {
        DTAP_LOGE("param is null");
        return;
    }
    DLI_DisconnectEvt *param = (DLI_DisconnectEvt *)cmdRes->eventParameter;
    uint16_t connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
    DTAP_LcidBufferNode *node = DTAP_GetLcidBufferNode(connHandle);
    if (node != NULL) {
        DTAP_LOGI("lcid %hu buffer node is exist, sendNotAckPktCnt %u, g_sendNotAckPktCnt %u",
            connHandle, node->sendNotAckPktCnt, g_sendNotAckPktCnt);
        g_sendNotAckPktCnt = DTAP_SCHED_SUB(g_sendNotAckPktCnt, node->sendNotAckPktCnt);
        COLLAB_ContinueAssignTransBuffer(g_sendNotAckPktCnt);
    }
    if (DTAP_DeleteLcidBufferNode(connHandle)) {
        DTAP_RecalcLcidQuota();
    }
    DTAP_LOGI("g_sendNotAckPktCnt: %hhu, g_apBufferNum: %hhu", g_sendNotAckPktCnt, g_apBufferNum);
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
    DTAP_LOGI("ap buffer num is set:%hhu", bufferNum);
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
            uint32_t channelPktCnt = SDF_DListCount(&channelNode->pktList);
            DTAP_DecLcidQueuedPktCnt(lcid, channelPktCnt);
            SDF_DListElmDel(&lcidNode->channelList, channelNode, schedEntry);
            lcidNode->pktCnt = DTAP_SCHED_SUB(lcidNode->pktCnt, channelPktCnt);
            q->pktCnt = DTAP_SCHED_SUB(q->pktCnt, channelPktCnt);
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
    DTAP_Channel_S *channelNode, DTAP_LcidBufferNode *node, DTAP_PendingPacket *pkt)
{
    uint32_t seq = pkt->seq;
    if (!SDF_DListIsEmpty(&channelNode->pktList)) {
        SDF_DListElmDel(&channelNode->pktList, pkt, entry);
    }
    DTAP_DestroyPacket((SDF_DListEntry_S *)pkt);

    lcidNode->pktCnt = DTAP_SCHED_SUB(lcidNode->pktCnt, 1);
    q->pktCnt = DTAP_SCHED_SUB(q->pktCnt, 1);
    node->queuedPktCnt = DTAP_SCHED_SUB(node->queuedPktCnt, 1);
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
            DTAP_LOGE("create %u fragment buff failed, lcid %hu, buff len %u", i, lcid, fragmentLen);
            for (uint32_t j = 0; j < i; j++) {
                SDF_BuffFree(fragmentBuf[j]);
            }
            return false;
        }
    }
    DLI_DataStru dliData = { lcid, DLI_DATATYPE_ACB, 0, 0, buff };
    uint32_t ret = DLI_SplitData(&dliData, fragmentBuf, fragmentCnt);
    if (ret != DLI_SUCCESS) {
        DTAP_LOGE("split data failed, ret %d, lcid %hu, buff len %llu", ret, lcid, SDF_DataLenGet(buff));
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

static void DTAP_PrintSendInfo(const DTAP_PriorityQueue *q, const DTAP_LcidNode *lcidNode,
    const DTAP_Channel_S *channelNode, const DTAP_LcidBufferNode *node, const DTAP_PendingPacket *pkt)
{
    DTAP_LOGD("send start, queue pktCnt %u, lcid pktCnt %u, channel pktCnt %u, priority %hhu, lcid %hu, "
        "srcTcid %hhu, dstTcid %hhu, pkt seq %u, pkt len %llu, quota %hhu, sendNotAck %u, queued %u", q->pktCnt,
        lcidNode->pktCnt, SDF_DListCount((SDF_DListHead_S *)&channelNode->pktList), q->priority, channelNode->lcid,
        channelNode->srcTcid, channelNode->dstTcid, pkt->seq, SDF_DataLenGet(pkt->buff), node->quota,
        node->sendNotAckPktCnt, node->queuedPktCnt);
}

static bool DTAP_ScheduleLcid(DTAP_PriorityQueue *q, DTAP_LcidNode *lcidNode, DTAP_LcidBufferNode *node)
{
    DTAP_Channel_S *channelNode = NULL;
    while (DTAP_CanSend(node) && DTAP_PriorityQueuePeek(q, lcidNode, &channelNode)) {
        DTAP_PendingPacket *pkt = (DTAP_PendingPacket *)SDF_DListFirst(&channelNode->pktList);

        DTAP_PrintSendInfo(q, lcidNode, channelNode, node, pkt);

        if (pkt->isSplited) {
            uint64_t dataLen = SDF_DataLenGet(pkt->buff);
            if (!DTAP_SendData(channelNode->lcid, pkt->buff)) {
                DTAP_LOGE("send data failed, priority %d, lcid %d, srcTcid %d, len %" PRIu64 "", q->priority,
                    channelNode->lcid, channelNode->srcTcid, dataLen);
                return false;
            }
            DTAP_CalcLcidSendRate(node, dataLen);
            DTAP_PriorityQueuePop(q, lcidNode, channelNode, node, pkt);
            node->sendNotAckPktCnt++;
            g_sendNotAckPktCnt++;
            continue;
        }

        uint32_t fragmentCnt = DLI_GetDataFragmentNums(pkt->buff);
        SDF_Buff_S *fragmentBuf[fragmentCnt];
        if (!DTAP_SplitData(channelNode->lcid, pkt->buff, fragmentBuf, fragmentCnt)) {
            return false;
        }
        SDF_BuffFree(pkt->buff);
        uint32_t sendCnt = 0;
        for (; DTAP_CanSend(node) && sendCnt < fragmentCnt; sendCnt++, node->sendNotAckPktCnt++) {
            uint64_t dataLen = SDF_DataLenGet(fragmentBuf[sendCnt]);
            if (!DTAP_SendData(channelNode->lcid, fragmentBuf[sendCnt])) {
                // 发送失败，暂停发送，等待下次继续发送
                DTAP_LOGE("send data failed, priority %d, lcid %d, srcTcid %d, len %" PRIu64 "", q->priority,
                    channelNode->lcid, channelNode->srcTcid, dataLen);
                break;
            }
            DTAP_CalcLcidSendRate(node, dataLen);
            g_sendNotAckPktCnt++;
        }

        DTAP_PriorityQueuePop(q, lcidNode, channelNode, node, pkt);
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

static DTAP_LcidNode *DTAP_GetLcidNode(const DTAP_PriorityQueue *q, uint16_t lcid)
{
    if (SDF_DListIsEmpty(&q->lcidList)) {
        return NULL;
    }
    DTAP_LcidNode *lcidNode = NULL;
    SDF_DListElmForeach(lcidNode, &q->lcidList, entry) {
        if (lcidNode->lcid == lcid) {
            return lcidNode;
        }
    }
    return NULL;
}

static void DTAP_InsertChannelToLcidNode(const DTAP_PriorityQueue *q, DTAP_LcidNode *lcidNode,
    DTAP_Channel_S *transChan)
{
    DTAP_Channel_S *channelNode = NULL;
    SDF_DListElmForeach(channelNode, &lcidNode->channelList, schedEntry) {
        if (channelNode->lcid == transChan->lcid && channelNode->srcTcid == transChan->srcTcid) {
            return;
        }
    }
    SDF_DListElmTailInsert(&lcidNode->channelList, transChan, schedEntry);
    DTAP_LOGI("insert channel node to queue, priority %d, lcid %d, srcTcid %d, dstTcid %d", q->priority,
        transChan->lcid, transChan->srcTcid, transChan->dstTcid);
}

static uint32_t DTAP_PriorityQueuePush(DTAP_PriorityQueue *q, DTAP_Channel_S *transChan, uint32_t pktCnt)
{
    DTAP_LOGD("enter, q size %d, priority %d, lcid %d, srcTcid %d", SDF_DListCount(&q->lcidList), q->priority,
        transChan->lcid, transChan->srcTcid);
    DTAP_LcidNode *lcidNode = DTAP_GetLcidNode(q, transChan->lcid);
    if (lcidNode == NULL) {
        lcidNode = DTAP_CreateLcidNode(transChan->lcid, q->priority);
        if (lcidNode == NULL) {
            DTAP_LOGE("malloc lcid node failed");
            return DTAP_TRANS_MALLOC_ERR;
        }
        SDF_DListElmTailInsert(&q->lcidList, lcidNode, entry);
        DTAP_LOGI("insert lcid node to queue, priority %d, lcid %d", q->priority, lcidNode->lcid);
    }

    DTAP_InsertChannelToLcidNode(q, lcidNode, transChan);

    lcidNode->pktCnt += pktCnt;
    q->pktCnt += pktCnt;
    DTAP_LcidBufferNode *bufNode = DTAP_GetLcidBufferNode(transChan->lcid);
    if (bufNode != NULL) {
        bufNode->queuedPktCnt += pktCnt;
    }

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
    if (q->pktCnt >= DTAP_PACKET_MAX_SIZE) {
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

    DTAP_LcidBufferNode *bufNode = DTAP_GetLcidBufferNode(transChan->lcid);
    if (bufNode != NULL) {
        // 配额已满且全局有余量时重算：配额有余量时重算无收益，全局满载时重算也发不出
        if (bufNode->sendNotAckPktCnt >= bufNode->quota && g_sendNotAckPktCnt < g_apBufferNum) {
            DTAP_RecalcLcidQuota();
        }
    }
    DTAP_SchedulerRun();

    return DTAP_SUCCESS;
}

static void DTAP_SendCompleteCbk(uint16_t connHandle, uint8_t numCompletedPackets)
{
    DTAP_LOGD("enter, connHandle %d, numCompletedPackets %d", connHandle, numCompletedPackets);
    DTAP_LcidBufferNode *node = DTAP_GetLcidBufferNode(connHandle);
    if (node == NULL) {
        DTAP_LOGE("connHandle %u is not exist, completed packet num is %u", connHandle, numCompletedPackets);
        return;
    }
    g_sendNotAckPktCnt = DTAP_SCHED_SUB(g_sendNotAckPktCnt, numCompletedPackets);
    COLLAB_ContinueAssignTransBuffer(g_sendNotAckPktCnt);
    node->sendNotAckPktCnt = DTAP_SCHED_SUB(node->sendNotAckPktCnt, numCompletedPackets);
    DTAP_RecalcLcidQuota();
    DTAP_SchedulerRun();
}
