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

#include "cm_trans_channel_mgr.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "sdf_mem.h"
#include "securec.h"
#include "sle_logic_link_mgr.h"

static SDF_DListHead_S g_fixedTransListHead = { .list = { NULL, NULL }, .size = 0 }; /* 元素类型: SleTransLcid_S */
static SDF_DListHead_S g_dynTransListHead = { .list = { NULL, NULL }, .size = 0 };   /* 元素类型: SleTransLcid_S */

static CM_TransChannelCbk g_transChannelCbk = NULL;

// 固定传输通道激活列表
static uint8_t g_TransFixedChannels[] = {
    CM_TCID_SLE_CMTC,
    CM_TCID_SLE_SMTC,
    CM_TCID_SLE_CUTC,
};

void CM_TransChannelMgrInit(void)
{
    SDF_DListHeadInit(&g_fixedTransListHead);
    SDF_DListHeadInit(&g_dynTransListHead);
}

static void CM_TransChannelFreeNode(SDF_DListEntry_S *node)
{
    SleTransLcid_S *link = (SleTransLcid_S *)node;
    if (link == NULL) {
        return;
    }
    if (link->params != NULL) {
        if (link->params->accessParams != NULL) {
            SDF_MemFree(link->params->accessParams);
            link->params->accessParams = NULL;
        }
        SDF_MemFree(link->params);
        link->params = NULL;
    }
    SDF_MemFree(link);
    link = NULL;
}

void CM_TransChannelMgrDeInit(void)
{
    SDF_DListDestroy(&g_fixedTransListHead, CM_TransChannelFreeNode);
    SDF_DListDestroy(&g_dynTransListHead, CM_TransChannelFreeNode);
}

uint32_t CM_RegTransChannelCbk(CM_TransChannelCbk cbk)
{
    g_transChannelCbk = cbk;
    return CM_SUCCESS;
}

void CM_UnRegTransChannelCbk(void)
{
    g_transChannelCbk = NULL;
}

bool CM_IsExistFixedTransChannelByLcid(uint16_t lcid)
{
    SleTransLcid_S *node = NULL;

    SDF_DListElmForeach(node, &(g_fixedTransListHead), entry) {
        if (node->lcid == lcid) {
            return true;
        }
    }
    return false;
}

bool CM_IsExistDynamicTransChannelByLcid(uint16_t lcid)
{
    SleTransLcid_S *node = NULL;

    SDF_DListElmForeach(node, &(g_dynTransListHead), entry) {
        if (node->lcid == lcid) {
            return true;
        }
    }
    return false;
}

uint32_t CM_FindFixedTransChannelByLcid(uint16_t lcid, SDF_Vector_S *transVector)
{
    CM_CHECK_RETURN_RET((transVector != NULL), CM_INVALID_PARAM_ERR, "param is invalid");
    SleTransLcid_S *node = NULL;

    SDF_DListElmForeach(node, &(g_fixedTransListHead), entry) {
        if (node->lcid == lcid) {
            SDF_VectorEmplaceBack(transVector, node);
        }
    }
    if (transVector->size == 0) {
        return CM_NOT_FOUND;
    }
    return CM_SUCCESS;
}

uint32_t CM_FindDynTransChannelByLcid(uint16_t lcid, SDF_Vector_S *transVector)
{
    CM_CHECK_RETURN_RET((transVector != NULL), CM_INVALID_PARAM_ERR, "param is invalid");
    SleTransLcid_S *node = NULL;
    SDF_DListElmForeach(node, &(g_dynTransListHead), entry) {
        if (node->lcid == lcid) {
            SDF_VectorEmplaceBack(transVector, node);
        }
    }
    if (transVector->size == 0) {
        return CM_NOT_FOUND;
    }
    return CM_SUCCESS;
}

SleTransLcid_S *CM_FindTransChannelByLocalTcid(uint16_t lcid, uint8_t tcid)
{
    SleTransLcid_S *node = NULL;

    if (CM_TRANS_CHANNEL_IS_FIXED(tcid)) {
        SDF_DListElmForeach(node, &(g_fixedTransListHead), entry) {
            if ((node->lcid == lcid) && (node->lastTransid[CM_TRANS_LOCAL] == tcid)) {
                return node;
            }
        }
    } else {
        SDF_DListElmForeach(node, &(g_dynTransListHead), entry) {
            if ((node->lcid == lcid) && (node->lastTransid[CM_TRANS_LOCAL] == tcid)) {
                return node;
            }
        }
    }
    CM_LOGD("lcid:0x%04x and tcid:0x%02x not in current trans channel list", lcid, tcid);
    return NULL;
}

SleTransLcid_S *CM_FindTransChannelByRemoteTcid(uint16_t lcid, uint8_t tcid)
{
    SleTransLcid_S *node = NULL;

    if (CM_TRANS_CHANNEL_IS_FIXED(tcid)) {
        SDF_DListElmForeach(node, &(g_fixedTransListHead), entry) {
            if ((node->lcid == lcid) && (node->lastTransid[CM_TRANS_REMOTE] == tcid)) {
                return node;
            }
        }
    } else {
        SDF_DListElmForeach(node, &(g_dynTransListHead), entry) {
            if ((node->lcid == lcid) && (node->lastTransid[CM_TRANS_REMOTE] == tcid)) {
                return node;
            }
        }
    }
    CM_LOGD("lcid:0x%04x and tcid:0x%02x not in current trans channel list", lcid, tcid);
    return NULL;
}

static SleTransLcid_S *CM_AllocTransChannelNode(uint16_t lcid, SLE_Addr_S *addr, SleTransLcidParam_S *params,
    CM_Tcid_E localTcid, CM_Tcid_E remoteTcid)
{
    SleTransLcid_S *node = (SleTransLcid_S *)SDF_MemZalloc(sizeof(SleTransLcid_S));
    CM_CHECK_RETURN_RET(node != NULL, NULL, "malloc trans lcid node failed");

    node->lcid = lcid;
    (void)memcpy_s(&node->addr, sizeof(node->addr), addr, sizeof(SLE_Addr_S));
    node->lastTransid[CM_TRANS_LOCAL] = localTcid;
    node->lastTransid[CM_TRANS_REMOTE] = remoteTcid;
    if (params == NULL) {
        node->params = NULL;
    } else {
        node->params = SDF_MemZalloc(sizeof(SleTransLcidParam_S));
        if (node->params == NULL) {
            CM_LOGE("malloc trans lcid node param failed");
            SDF_MemFree(node);
            return NULL;
        }
        (void)memcpy_s(node->params, sizeof(SleTransLcidParam_S), params, sizeof(SleTransLcidParam_S));
    }
    SDF_DListEntryInit(&node->entry);
    return node;
}

static SleTransLcid_S *CM_AllocTransChannel(uint16_t lcid, SleTransLcidParam_S *params, CM_Tcid_E localTcid,
    CM_Tcid_E remoteTcid, uint8_t channelState)
{
    SleLogicLink_S *sll = SleLogicLinkGetByLcid(lcid);
    CM_CHECK_RETURN_RET(sll != NULL, NULL, "SleLogicLinkGetByLcid failed");
    SleTransLcid_S *node = CM_AllocTransChannelNode(lcid, &sll->rmtAddr, params, localTcid, remoteTcid);
    CM_CHECK_RETURN_RET(node != NULL, NULL, "CM_AllocTransChannelNode failed");

    node->state = channelState;
    if (CM_TRANS_CHANNEL_IS_FIXED(localTcid)) {
        CM_LOGD("dyn channel insert a fix channel node, channelState:0x%02x, lcid:0x%04x, localTcid:0x%02x, "
            "remoteTcid:0x%02x, addr:%s", channelState, lcid, localTcid, remoteTcid, GetEncryptAddr(&node->addr).buf);
        SDF_DListElmTailInsert(&g_fixedTransListHead, node, entry);
    } else {
        CM_LOGD("dyn channel insert a dyn channel node, channelState:0x%02x, lcid:0x%04x, localTcid:0x%02x, "
            "remoteTcid:0x%02x, addr:%s", channelState, lcid, localTcid, remoteTcid, GetEncryptAddr(&node->addr).buf);
        SDF_DListElmTailInsert(&g_dynTransListHead, node, entry);
    }
    return node;
}

static uint32_t CM_FixedTransChannelChangeCbk(uint16_t lcid, CM_TransChannelState_E state)
{
    if (g_transChannelCbk == NULL) {
        return CM_SUCCESS;
    }
    SDF_Traits transChannelTraits = { .dtor = SDF_MemFree };
    CM_TransChannelStateList_S stateList = { 0 };
    stateList.channelVector = SDF_CreateVector(transChannelTraits);
    CM_CHECK_RETURN_RET((stateList.channelVector != NULL), CM_FAIL, "SDF_CreateVector transChannel failed");
    SleTransLcid_S *node = NULL;
    SDF_Vector_S *channelVector = stateList.channelVector;
    SDF_DListElmForeach(node, &(g_fixedTransListHead), entry) {
        if (lcid != node->lcid) {
            continue;
        }
        CM_TransChan_S *channel = (CM_TransChan_S *)SDF_MemZalloc(sizeof(CM_TransChan_S));
        if (channel == NULL) {
            CM_LOGW("channel state malloc failed");
            SDF_DestroyVector(channelVector);
            return CM_FAIL;
        }
        channel->castMode = CM_ACCESS_TRANS_MODE_UNICAST;
        channel->lcid = node->lcid;
        channel->srcTcid = node->lastTransid[CM_TRANS_LOCAL];
        channel->dstTcid = node->lastTransid[CM_TRANS_REMOTE];
        channel->config.transMode = CM_TRANS_MODE_BASIC;
        channel->config.mtu = UINT16_MAX;
        channel->config.mps = UINT16_MAX;
        if (node->params != NULL) {
            (void)memcpy_s(&channel->config, sizeof(CM_TransModeConfig_S),
                &node->params->transModeConfig, sizeof(CM_TransModeConfig_S));
        }
        SDF_VectorEmplaceBack(channelVector, channel);
    }
    stateList.result = state;
    g_transChannelCbk(&stateList);
    SDF_DestroyVector(channelVector);
    CM_LOGI("fixed trans channel state change cbk, lcid:0x%04x, state:%d", lcid, state);
    return CM_SUCCESS;
}

/**
 * @brief 动态传输通道状态变更回调通知DTAP
 */
static void CM_DynTransChannelChangeCbk(SleTransLcid_S *node, CM_TransChannelState_E state)
{
    if (g_transChannelCbk == NULL) {
        return;
    }
    SDF_Traits transChannelTraits = { .dtor = SDF_MemFree };
    CM_TransChannelStateList_S stateList = { 0 };
    stateList.channelVector = SDF_CreateVector(transChannelTraits);
    CM_CHECK_RETURN((stateList.channelVector != NULL), "SDF_CreateVector transChannel failed");
    SDF_Vector_S *channelVector = stateList.channelVector;
    CM_TransChan_S *channel = (CM_TransChan_S *)SDF_MemZalloc(sizeof(CM_TransChan_S));
    if (channel == NULL) {
        CM_LOGW("channel state malloc failed");
        SDF_DestroyVector(channelVector);
        return;
    }
    channel->lcid = node->lcid;
    channel->srcTcid = node->lastTransid[CM_TRANS_LOCAL];
    channel->dstTcid = node->lastTransid[CM_TRANS_REMOTE];
    channel->config.transMode = CM_TRANS_MODE_BASIC;
    channel->config.mtu = UINT16_MAX;
    channel->config.mps = UINT16_MAX;
    if (node->params != NULL) {
        channel->castMode = node->params->castMode;
        (void)memcpy_s(&channel->config, sizeof(CM_TransModeConfig_S),
            &node->params->transModeConfig, sizeof(CM_TransModeConfig_S));
    }
    SDF_VectorEmplaceBack(channelVector, channel);
    stateList.result = state;
    g_transChannelCbk(&stateList);
    CM_LOGI("dyn trans channel state change cbk, lcid:0x%04x, state:%d", channel->lcid, state);
    SDF_DestroyVector(channelVector);
}

static void CM_TransChannelDestroyOneNode(SleTransLcid_S *transLcid, SDF_DListHead_S *transListHead)
{
    SDF_DListDel(transListHead, &transLcid->entry);

    if (transLcid->params != NULL) {
        if (transLcid->params->accessParams != NULL) {
            SDF_MemFree(transLcid->params->accessParams);
            transLcid->params->accessParams = NULL;
        }
        SDF_MemFree(transLcid->params);
        transLcid->params = NULL;
    }
    SDF_MemFree(transLcid);
    transLcid = NULL;
}

static void CM_TransChannelDestroyListNode(SDF_Vector_S *transChannelVector, SDF_DListHead_S *transListHead)
{
    for (uint32_t i = 0; i < transChannelVector->size; i++) {
        SleTransLcid_S *transLcid = SDF_VectorElementAt(transChannelVector, i);
        CM_CHECK_RETURN(transLcid != NULL, "transLcid is null");
        CM_TransChannelDestroyOneNode(transLcid, transListHead);
    }
}

uint32_t CM_ActivateFixedTransChannel(uint16_t lcid, SleTransLcidParam_S *params)
{
    if (CM_IsExistFixedTransChannelByLcid(lcid)) {
        CM_LOGE("lcid has been actived, lcid:0x%04x", lcid);
        return CM_FAIL;
    }

    for (uint8_t i = 0; i < (sizeof(g_TransFixedChannels) / sizeof(g_TransFixedChannels[0])); i++) {
        SleTransLcid_S *node = CM_AllocTransChannel(lcid, params, g_TransFixedChannels[i], g_TransFixedChannels[i],
            CM_TRANS_CHANNEL_STATE_ACTIVATED);
        if (node == NULL) {
            CM_LOGE("active fixed trans channel failed, lcid:0x%04x", lcid);
            CM_ReleaseFixedTransChannel(lcid);
            return CM_FAIL;
        }
    }
    CM_LOGI("fixed trans channel add node, lcid:0x%04x, size:%u", lcid, SDF_DListCount(&g_fixedTransListHead));
    CM_FixedTransChannelChangeCbk(lcid, CM_TRANS_CHANNEL_STATE_ACTIVATED);
    return CM_SUCCESS;
}

SleTransLcid_S *CM_ActivateDynamicTransChannel(uint16_t lcid, const CM_DynTransChanParam_S *params)
{
    CM_CHECK_RETURN_RET((params->localTcid != CM_TRANS_INVALID_TCID), NULL, "localTcid is invalid");
    SleTransLcidParam_S transParams = { 0 };
    transParams.version = params->version;
    transParams.localIndex = params->localIndex;
    (void)memcpy_s(&transParams.transModeConfig, sizeof(CM_TransModeConfig_S),
        &params->transModeConfig, sizeof(CM_TransModeConfig_S));
    transParams.channelMode = params->channelMode;
    transParams.direction = params->direction;
    transParams.exclusive = true;
    transParams.srcPort = params->srcPort;
    transParams.dstPort = params->dstPort;
    transParams.slqi = params->slqi;
    transParams.aid = params->aid;
    transParams.frameType = params->frameType;
    SleTransLcid_S *node = CM_AllocTransChannel(lcid, &transParams, params->localTcid, params->remoteTcid,
        params->state);
    CM_CHECK_RETURN_RET((node != NULL), NULL, "CM_AllocTransChannel localTcid:0x%02x failed", params->localTcid);
    CM_LOGI("dyn trans channel add node, lcid:0x%04x, transMode: %hhu, localTcid:0x%02x, remoteTcid:0x%02x, "
        "srcPort:0x%04x, dstPort:0x%04x, size:%u, frameType: %hhu", lcid, params->transModeConfig.transMode,
        params->localTcid, params->remoteTcid, transParams.srcPort, transParams.dstPort,
        SDF_DListCount(&g_dynTransListHead), transParams.frameType);

    if (params->state == CM_TRANS_CHANNEL_STATE_ACTIVATED) {
        CM_DynTransChannelChangeCbk(node, params->state);
    }
    return node;
}

void CM_DynamicTransChannelSetTcidAndChangeCbk(SleTransLcid_S *node, uint8_t remoteTcid, uint8_t state)
{
    if (node == NULL) {
        return;
    }
    node->state = state;
    node->lastTransid[CM_TRANS_REMOTE] = remoteTcid;
    if (state == CM_TRANS_CHANNEL_STATE_ACTIVATED) {
        CM_DynTransChannelChangeCbk(node, state);
    }
}

static void CM_FixedTransChannelFreeListNode(uint16_t lcid, SDF_Vector_S *transChannelVector)
{
    // Notice, first notify user callback, Then release the trans channel list node data
    uint32_t ret = CM_FixedTransChannelChangeCbk(lcid, CM_TRANS_CHANNEL_STATE_RELEASED);
    if (ret != CM_SUCCESS) {
        CM_LOGW("CM_FixedTransChannelChangeCbk failed, lcid 0x%04x", lcid);
        // Don't return, continue
    }
    CM_TransChannelDestroyListNode(transChannelVector, &g_fixedTransListHead);
}

/**
 * @brief 动态传输通道状态变更回调通知DTAP并释放节点
 */
static void CM_DynTransChannelFreeListNode(SleTransLcid_S *node)
{
    // Notice, first notify user callback, Then release the trans channel list node data
    CM_DynTransChannelChangeCbk(node, CM_TRANS_CHANNEL_STATE_RELEASED);
    CM_TransChannelDestroyOneNode(node, &g_dynTransListHead);
}

void CM_ReleaseFixedTransChannel(uint16_t lcid)
{
    SDF_Traits transChannelTraits = { .dtor = NULL };
    SDF_Vector_S *transChannelVector = SDF_CreateVector(transChannelTraits);
    CM_CHECK_RETURN((transChannelVector != NULL), "SDF_CreateVector transChannel failed");
    uint32_t ret = CM_FindFixedTransChannelByLcid(lcid, transChannelVector);
    if (ret != CM_SUCCESS) {
        if (ret == CM_NOT_FOUND) {
            SDF_DestroyVector(transChannelVector);
            return;
        }
        SDF_DestroyVector(transChannelVector);
        CM_LOGE("find lcid 0x%04x failed, release fixed tcid failed.", lcid);
        return;
    }
    CM_FixedTransChannelFreeListNode(lcid, transChannelVector);
    SDF_DestroyVector(transChannelVector);
    CM_LOGI("fixed trans channel remove a node, lcid:0x%04x, size:%u", lcid, SDF_DListCount(&g_fixedTransListHead));
}

/*
 * @brief 通知DTAP模块
 */
void CM_ReleaseDynamicTransChannelByLocalTcid(SleTransLcid_S *node)
{
    if (node == NULL) {
        return;
    }
    uint16_t lcid = node->lcid;
    uint16_t localTcid = node->lastTransid[CM_TRANS_LOCAL];
    CM_DynTransChannelFreeListNode(node);
    CM_LOGI("dyn trans channel remove a node, lcid:0x%04x, localTcid:0x%02x, size:%u",
        lcid, localTcid, SDF_DListCount(&g_dynTransListHead));
}

void CM_ReleaseDynamicTransChannelByRemoteTcid(SleTransLcid_S *node)
{
    if (node == NULL) {
        return;
    }
    uint16_t lcid = node->lcid;
    uint16_t remoteTcid = node->lastTransid[CM_TRANS_REMOTE];
    CM_DynTransChannelFreeListNode(node);
    CM_LOGI("dyn trans channel remove a node, lcid:0x%04x, remoteTcid:0x%02x, size:%u",
        lcid, remoteTcid, SDF_DListCount(&g_dynTransListHead));
}