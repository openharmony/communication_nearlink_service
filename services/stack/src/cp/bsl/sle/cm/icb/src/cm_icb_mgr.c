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

#include "cm_icb_mgr.h"
#include <stdint.h>
#include <stddef.h>
#include "byte_codec.h"
#include "cm_inner_api.h"
#include "cm_dli_adapter.h"
#include "cm_errno.h"
#include "cm_icb_init.h"
#include "cm_icb_inner_api.h"
#include "cm_log.h"
#include "dli.h"
#include "dli_cmd.h"
#include "dli_cmd_struct.h"
#include "dli_def.h"
#include "dli_errno.h"
#include "dli_event_struct.h"
#include "dli_layer_stru.h"
#include "sdf_mem.h"
#include "sdf_dlist.h"
#include "securec.h"
#include "dli_ext_func_wrapper.h"

#define ICB_MTU_SIZE 4096
#define ICB_MPS_SIZE 4096
#define CRC_INIT_VALUE (0x5555)
#define STREAM_TIMEOUT 1000 // ms

typedef struct {
    DLI_CmdOpcode opCode;
    CM_ICBConnectionState state;
} DliOpCodeToState;

typedef struct {
    uint16_t connHandle;
    uint16_t lcid;
    uint8_t labelCnt;
    DLI_ICGLabel *label;
    bool isPassiveConn;     /* 是否被动连接 */
    bool isConnected;
} ICBChannelInfo;

typedef struct {
    SDF_DListEntry_S entry;
    CM_ICBType type;
    uint8_t id;              /* iog/img唯一标识 */
    uint16_t gHandle;        /* 组播G端Handle */
    uint8_t channelCnt;
    ICBChannelInfo *channel;
} ICGConnectionNode;

typedef struct {
    SDF_DListEntry_S entry;
    CM_FreqBandListener listener;
} FreqBandListenerNode;

static SDF_DListHead_S g_icgConnectionListHead;
static CM_ICBCallback g_icbCallback = {NULL};
static CM_ICBConnectionStatusCbk g_icbConnStatusCb = NULL;
static SDF_DListHead_S g_freqBandListener = {{&g_freqBandListener.list, &g_freqBandListener.list}, 0};

static ICGConnectionNode *MallocICGChannelNode(uint8_t channelCnt)
{
    ICGConnectionNode *channelNode = (ICGConnectionNode *)SDF_MemZalloc(sizeof(ICGConnectionNode));
    if (channelNode == NULL) {
        return NULL;
    }
    channelNode->channel = (ICBChannelInfo *)SDF_MemZalloc(channelCnt * sizeof(ICBChannelInfo));
    if (channelNode->channel == NULL) {
        SDF_MemFree(channelNode);
        CM_LOGE("malloc channel failed");
        return NULL;
    }
    SDF_DListEntryInit(&channelNode->entry);
    return channelNode;
}

static void FreeICBChannel(ICBChannelInfo *channel, uint8_t channelCnt)
{
    if (channel == NULL) {
        return;
    }
    for (uint8_t i = 0; i < channelCnt; i++) {
        if (channel[i].label != NULL) {
            SDF_MemFree(channel[i].label);
        }
    }
    SDF_MemFree(channel);
}

static void FreeICGChannelNode(SDF_DListEntry_S *channel)
{
    ICGConnectionNode *channelNode = (ICGConnectionNode *)channel;
    if (channelNode == NULL) {
        return;
    }
    FreeICBChannel(channelNode->channel, channelNode->channelCnt);
    SDF_MemFree(channelNode);
}

static void AddICGChannelNode(ICGConnectionNode *channelNode)
{
    SDF_DListElmTailInsert(&g_icgConnectionListHead, channelNode, entry);
    CM_LOGI("add icg channel node, id: %u, type: %u, channel list count: %u",
        channelNode->id, channelNode->type, g_icgConnectionListHead.size);
}

static ICGConnectionNode *DelICGChannelNode(uint8_t id)
{
    ICGConnectionNode *channelNode = NULL;
    ICGConnectionNode *temp = NULL;
    SDF_DListElmSafeForeach(channelNode, temp, &g_icgConnectionListHead, entry) {
        if (channelNode->id == id) {
            SDF_DListElmDel(&g_icgConnectionListHead, channelNode, entry);
            CM_LOGI("remove icg channel node, id: %u, type: %u,  channel list count: %u",
                channelNode->id, channelNode->type, g_icgConnectionListHead.size);
            return channelNode;
        }
    }
    return NULL;
}
static ICGConnectionNode *FindICGConnectionNodeByTypeId(CM_ICBType type, uint8_t id)

{
    ICGConnectionNode *channelNode = NULL;
    ICGConnectionNode *temp = NULL;
    SDF_DListElmSafeForeach(channelNode, temp, &g_icgConnectionListHead, entry) {
        if (channelNode->type == type && channelNode->id == id) {
            return channelNode;
        }
    }
    return NULL;
}

static CM_ICBErrorCode GetICBErrorCode(uint16_t status, uint8_t type)
{
    if (status == DLI_SUCCESS) {
        return CM_ICB_SUCCESS;
    }
    return (type == CM_IMB) ? CM_IMB_FAILED : CM_IOB_FAILED;
}

static void NotifyUpperCallback(CM_ICBConnection *param)
{
    CM_ICBCallback cb = g_icbCallback;
    if (cb.connectionCbk != NULL) {
        cb.connectionCbk(param);
    }
}

static void NotifyMultiChannelCallback(CM_ICBConnectionState state, CM_ICBErrorCode errorCode,
    ICGConnectionNode *channelNode)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel[CM_MAX_CHANNEL_COUNT] = {};
    icbConn.state = state;
    icbConn.errorCode = errorCode;
    icbConn.id = channelNode->id;
    icbConn.isIMG = channelNode->type == CM_IMB;
    icbConn.gHandle = channelNode->gHandle;
    icbConn.channelCnt = channelNode->channelCnt;
    for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
        channel[i].connHandle = channelNode->channel[i].connHandle;
        channel[i].lcid = channelNode->channel[i].lcid;
    }
    icbConn.channel = channel;
    NotifyUpperCallback(&icbConn);
}

static void NotifyParamChangeFailedCbk(CM_ICBConnectionState state, DLI_ICGCbkParam *cbkParam)
{
    CM_ICBConnection icbConn = {};
    icbConn.state = state;
    icbConn.errorCode = GetICBErrorCode(DLI_UNSPECIFIED_ERROR, cbkParam->type);
    icbConn.id = cbkParam->id;
    NotifyUpperCallback(&icbConn);
}

static void NotifySingleChannelCallback(CM_ICBConnectionState state, CM_ICBErrorCode errorCode, uint16_t connHandle,
    uint16_t lcid, ICGConnectionNode *channelNode)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = state;
    icbConn.errorCode = errorCode;
    icbConn.isIMG = channelNode->type == CM_IMB;
    icbConn.gHandle = channelNode->gHandle;
    icbConn.id = channelNode->id;
    icbConn.channelCnt = 1;
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    icbConn.channel = &channel;
    NotifyUpperCallback(&icbConn);
}

/**
 * @brief 收到dli的回调已经切换到CP线程
 */
static void CM_IOGSetParamCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(context != NULL, "context is null");

    DLI_ICGCbkParam *cbkParam = (DLI_ICGCbkParam *)context;
    CM_LOGI("status=%u, id=%u", status, cbkParam->id);
    if (status != DLI_SUCCESS) {
        CM_LOGE("set param failed, id=%u", cbkParam->id);
        NotifyParamChangeFailedCbk(CM_ICB_STATE_IOG_CREATED, cbkParam);
        return;
    }
    if (cmdRes == NULL || cmdRes->eventParameter == NULL) {
        CM_LOGE("param is null, id=%u", cbkParam->id);
        NotifyParamChangeFailedCbk(CM_ICB_STATE_IOG_CREATED, cbkParam);
        return;
    }

    DLI_SetIOGParamEvt *param = (DLI_SetIOGParamEvt *)cmdRes->eventParameter;
    CM_LOGI("param id: %u, param count: %u, expected id: %u", param->id, param->paramCnt, cbkParam->id);
    if (cbkParam->id != param->id || param->paramCnt == 0 || param->paramCnt > CM_MAX_CHANNEL_COUNT) {
        CM_LOGE("illegal param, id=%u, param count=%u", cbkParam->id, param->paramCnt);
        NotifyParamChangeFailedCbk(CM_ICB_STATE_IOG_CREATED, cbkParam);
        return;
    }

    ICGConnectionNode *channelNode = FindICGConnectionNodeByTypeId(CM_IOB, param->id);
    if (channelNode != NULL) {
        CM_LOGI("iob channel node already existed");
        NotifyMultiChannelCallback(CM_ICB_STATE_IOG_CREATED, CM_ICB_SUCCESS, channelNode);
        return;
    }

    channelNode = MallocICGChannelNode(param->paramCnt);
    if (channelNode != NULL) {
        channelNode->type = CM_IOB;
        channelNode->id = param->id;
        channelNode->channelCnt = param->paramCnt;
        for (uint8_t i = 0; i < param->paramCnt; i++) {
            channelNode->channel[i].connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle[i]);
            CM_LOGI("[%u] add iob conn handler 0x%04x", i, channelNode->channel[i].connHandle);
        }
        AddICGChannelNode(channelNode);
        NotifyMultiChannelCallback(CM_ICB_STATE_IOG_CREATED, CM_ICB_SUCCESS, channelNode);
        return;
    }
    NotifyParamChangeFailedCbk(CM_ICB_STATE_IOG_CREATED, cbkParam);
    CM_LOGE("malloc channel failed, id=%u", param->id);
}

static void CM_IMGSetParamCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(context != NULL, "context is null");

    DLI_ICGCbkParam *cbkParam = (DLI_ICGCbkParam *)context;
    CM_LOGI("status=%u, id=%u", status, cbkParam->id);
    if (status != DLI_SUCCESS) {
        CM_LOGE("set param failed, id=%u", cbkParam->id);
        NotifyParamChangeFailedCbk(CM_ICB_STATE_IMG_CREATED, cbkParam);
        return;
    }
    if (cmdRes == NULL || cmdRes->eventParameter == NULL) {
        CM_LOGE("param is null, id=%u", cbkParam->id);
        NotifyParamChangeFailedCbk(CM_ICB_STATE_IMG_CREATED, cbkParam);
        return;
    }

    DLI_SetIMGParamEvt *param = (DLI_SetIMGParamEvt *)cmdRes->eventParameter;
    CM_LOGI("param id: %u, param count: %u, expected id: %u", param->id, param->paramCnt, cbkParam->id);
    if (cbkParam->id != param->id || param->paramCnt == 0 || param->paramCnt > CM_MAX_CHANNEL_COUNT) {
        CM_LOGE("illegal param, id=%u, param count=%u", cbkParam->id, param->paramCnt);
        NotifyParamChangeFailedCbk(CM_ICB_STATE_IMG_CREATED, cbkParam);
        return;
    }

    ICGConnectionNode *channelNode = FindICGConnectionNodeByTypeId(CM_IMB, param->id);
    if (channelNode != NULL) {
        CM_LOGI("imb channel node already existed");
        NotifyMultiChannelCallback(CM_ICB_STATE_IOG_CREATED, CM_ICB_SUCCESS, channelNode);
        return;
    }

    channelNode = MallocICGChannelNode(param->paramCnt);
    if (channelNode != NULL) {
        channelNode->type = CM_IMB;
        channelNode->id = param->id;
        channelNode->gHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->gHandle);
        channelNode->channelCnt = param->paramCnt;
        for (uint8_t i = 0; i < param->paramCnt; i++) {
            channelNode->channel[i].connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle[i]);
            CM_LOGI("[%u] add imb conn handler 0x%04x", i, channelNode->channel[i].connHandle);
        }
        AddICGChannelNode(channelNode);
        NotifyMultiChannelCallback(CM_ICB_STATE_IMG_CREATED, CM_ICB_SUCCESS, channelNode);
        return;
    }
    NotifyParamChangeFailedCbk(CM_ICB_STATE_IMG_CREATED, cbkParam);
    CM_LOGE("malloc channel failed, id=%u", param->id);
}

static void CM_ICGRemoveParamCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(context != NULL, "context is null");

    DLI_ICGCbkParam *cbkParam = (DLI_ICGCbkParam *)context;
    CM_LOGI("status=%u, type=%u, id=%u", status, cbkParam->type, cbkParam->id);

    CM_ICBConnectionState state = cbkParam->type == CM_IMB ? CM_ICB_STATE_IMG_REMOVED : CM_ICB_STATE_IOG_REMOVED;
    if (status != DLI_SUCCESS) {
        CM_LOGE("remove param failed, id=%u", cbkParam->id);
        NotifyParamChangeFailedCbk(state, cbkParam);
        return;
    }
    if (cmdRes == NULL || cmdRes->eventParameter == NULL) {
        CM_LOGE("param is null, id=%u", cbkParam->id);
        NotifyParamChangeFailedCbk(state, cbkParam);
        return;
    }

    DLI_RemoveICGParamEvt *param = (DLI_RemoveICGParamEvt *)cmdRes->eventParameter;
    if (cbkParam->id != param->id) {
        CM_LOGE("illegal param, id=%u", cbkParam->id);
        NotifyParamChangeFailedCbk(state, cbkParam);
        return;
    }

    CM_ICBErrorCode errorCode = CM_ICB_SUCCESS;
    ICGConnectionNode *channelNode = DelICGChannelNode(param->id);
    if (channelNode == NULL) {
        errorCode = GetICBErrorCode(DLI_UNSPECIFIED_ERROR, cbkParam->type);
        CM_LOGE("delete channel node failed, id=%u", param->id);
        NotifyParamChangeFailedCbk(state, cbkParam);
        return;
    }
    CM_LOGI("id %u del channel, channel count: %u", param->id, channelNode->channelCnt);
    for (uint32_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
        CM_LOGI("remove conn handler 0x%04x", channelNode->channel[i].connHandle);
    }
    NotifyMultiChannelCallback(state, errorCode, channelNode);
    FreeICGChannelNode((SDF_DListEntry_S *)channelNode);
}

static void NotifyLabelReportCallback(DLI_ICGLabelReportEvt *param, CM_ICBErrorCode errorCode)
{
    if (g_icbCallback.labelReportCbk == NULL) {
        CM_LOGE("callback is null");
        return;
    }
    CM_ICBLabelReportParam labelReportParam = {};
    labelReportParam.errorCode = errorCode;
    labelReportParam.lcid = DECODE2BYTE_LITTLE((uint8_t *)&param->lcid);
    labelReportParam.connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
    labelReportParam.icgId = param->icgId;
    labelReportParam.icbId = param->icbId;
    labelReportParam.icbInterval = DECODE2BYTE_LITTLE((uint8_t *)&param->icbInterval);
    labelReportParam.latencyG2T = DECODE3BYTE_LITTLE((uint8_t *)&param->latencyG2T);
    labelReportParam.latencyT2G = DECODE3BYTE_LITTLE((uint8_t *)&param->latencyT2G);
    labelReportParam.bnG2T = param->bnG2T;
    labelReportParam.bnT2G = param->bnT2G;
    labelReportParam.ftG2T = param->ftG2T;
    labelReportParam.ftT2G = param->ftT2G;
    labelReportParam.labelCnt = param->labelCnt;
    size_t labelSize = param->labelCnt * sizeof(CM_ICBLabel);
    labelReportParam.label = (CM_ICBLabel *)SDF_MemZalloc(labelSize);
    CM_CHECK_RETURN(labelReportParam.label != NULL, "malloc label failed");
    for (uint8_t i = 0; i < param->labelCnt; i++) {
        DLI_ICGLabel *label = (DLI_ICGLabel *)(param->label + i * sizeof(DLI_ICGLabel));
        labelReportParam.label[i].labelId = label->labelId;
        labelReportParam.label[i].txPhy = label->txPhy;
        labelReportParam.label[i].rxPhy = label->rxPhy;
        labelReportParam.label[i].txMcs = label->txMcs;
        labelReportParam.label[i].rxMcs = label->rxMcs;
        labelReportParam.label[i].txFrame = label->txFrame;
        labelReportParam.label[i].rxFrame = label->rxFrame;
        labelReportParam.label[i].maxSduG2T = DECODE2BYTE_LITTLE((uint8_t *)&label->maxSduG2T);
        labelReportParam.label[i].maxSduT2G = DECODE2BYTE_LITTLE((uint8_t *)&label->maxSduT2G);
        labelReportParam.label[i].maxPduG2T = DECODE2BYTE_LITTLE((uint8_t *)&label->maxPduG2T);
        labelReportParam.label[i].maxPduT2G = DECODE2BYTE_LITTLE((uint8_t *)&label->maxPduT2G);
        labelReportParam.label[i].nse = label->nse;
    }
    g_icbCallback.labelReportCbk(&labelReportParam);
    SDF_MemFree(labelReportParam.label);
}

static void NotifyLabelReportFailedCbk(uint8_t type, uint8_t icgId, uint16_t connHandle)
{
    if (g_icbCallback.labelReportCbk == NULL) {
        CM_LOGE("callback is null");
        return;
    }
    CM_ICBLabelReportParam labelReportParam = {};
    labelReportParam.errorCode = GetICBErrorCode(DLI_UNSPECIFIED_ERROR, type);
    labelReportParam.icgId = icgId;
    labelReportParam.connHandle = connHandle;
    g_icbCallback.labelReportCbk(&labelReportParam);
}

static void CM_LabelReportCbkProc(ICGConnectionNode *channelNode, DLI_ICGLabelReportEvt *param)
{
    uint16_t connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
    uint16_t lcid = DECODE2BYTE_LITTLE((uint8_t *)&param->lcid);
    CM_LOGI("label report, id: %u, conn handler: 0x%04x, lcid: 0x%04x", param->icgId, connHandle, lcid);

    for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
        CM_LOGI("checking channel node, type: %u, id: %u, conn handler: 0x%04x",
            channelNode->type, channelNode->id, channelNode->channel[i].connHandle);
        if (channelNode->channel[i].connHandle != connHandle) {
            continue;
        }
        CM_LOGI("find channel, now add label, label count: %u", param->labelCnt);
        channelNode->channel[i].lcid = lcid;
        channelNode->channel[i].labelCnt = param->labelCnt;
        size_t labelSize = param->labelCnt * sizeof(DLI_ICGLabel);
        channelNode->channel[i].label = (DLI_ICGLabel *)SDF_MemZalloc(labelSize);
        if (channelNode->channel[i].label == NULL) {
            NotifyLabelReportCallback(param, CM_ICB_FAILED);
            CM_LOGE("malloc label failed, connHandle=%u, lcid=%u", connHandle, lcid);
            return;
        }
        (void)memcpy_s(channelNode->channel[i].label, labelSize, param->label, labelSize);
        NotifyLabelReportCallback(param, CM_ICB_SUCCESS);
        return;
    }
    NotifyLabelReportFailedCbk(channelNode->type, channelNode->id, connHandle);
    CM_LOGE("channelNode is not exist, connHandle=%u, lcid=%u", connHandle, lcid);
}

static void CM_LabelReportCbk(uint8_t type, void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "param is null");

    if (status != DLI_SUCCESS && context != NULL) {
        CM_CHECK_RETURN(g_icbCallback.labelReportCbk != NULL, "callback is null");
        DLI_ICGCbkParam *cbkParam = (DLI_ICGCbkParam *)context;
        for (uint8_t i = 0; i < cbkParam->connHandleNum && i < SLE_ICB_MAX_NUM; i++) {
            CM_ICBLabelReportParam labelReportParam = {};
            labelReportParam.errorCode = CM_ICB_FAILED;
            labelReportParam.connHandle = cbkParam->connHandle[i];
            labelReportParam.icgId = cbkParam->id;
            labelReportParam.labelCnt = 0;
            CM_LOGE("set label failed, connHandle=%u", labelReportParam.connHandle);
            g_icbCallback.labelReportCbk(&labelReportParam);
        }
        return;
    }
    DLI_ICGLabelReportEvt *param = (DLI_ICGLabelReportEvt *)cmdRes->eventParameter;
    uint16_t connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);

    ICGConnectionNode *channelNode = NULL;
    ICGConnectionNode *temp = NULL;
    SDF_DListElmSafeForeach(channelNode, temp, &g_icgConnectionListHead, entry) {
        if (channelNode->type != type) {
            continue;
        }
        for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
            if (channelNode->channel[i].connHandle != connHandle) {
                continue;
            }
            if (status != DLI_SUCCESS) {
                CM_LOGE("set label failed, connHandle=%u", connHandle);
                NotifyLabelReportFailedCbk(type, channelNode->id, connHandle);
                return;
            }
            if (channelNode->id != param->icgId || param->labelCnt == 0 || param->labelCnt > CM_MAX_LABEL_COUNT) {
                CM_LOGE("illegal param, id=%u, label count=%u", channelNode->id, param->labelCnt);
                NotifyLabelReportFailedCbk(type, channelNode->id, connHandle);
                return;
            }
            CM_LabelReportCbkProc(channelNode, param);
            return;
        }
    }
    CM_LOGE("channelNode is not exist, connHandle=%u", connHandle);
}

static void CM_IOGLabelReportCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_LabelReportCbk(CM_IOB, context, status, cmdRes);
}

static void CM_IMGLabelReportCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_LabelReportCbk(CM_IMB, context, status, cmdRes);
}

static void CM_NotifyICBConnectionCbk(uint16_t connHandle, ICBConnectionType type)
{
    if (g_icbConnStatusCb != NULL) {
        CM_TransChan_S trans = {};
        trans.castMode = CM_ACCESS_TRANS_MODE_DATA_MCST;
        trans.lcid = connHandle;
        trans.srcTcid = CM_TCID_MC_BEGIN;
        trans.dstTcid = CM_TCID_MC_BEGIN;
        trans.config.transMode = CM_TRANS_MODE_STREAM;
        trans.config.mtu = ICB_MTU_SIZE;
        trans.config.mps = ICB_MPS_SIZE;
        trans.config.streamMode.reorderTimeout = STREAM_TIMEOUT;
        trans.config.streamMode.crcInit = CRC_INIT_VALUE;
        trans.config.streamMode.flushTimeout = STREAM_TIMEOUT;
        g_icbConnStatusCb(type, &trans);
    }
}

static void CM_EstablishedCbk(uint8_t type, void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "param is null, type=%u", type);

    DLI_ICBEstablishedEvt *param = (DLI_ICBEstablishedEvt *)cmdRes->eventParameter;
    uint16_t connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
    uint16_t lcid = DECODE2BYTE_LITTLE((uint8_t *)&param->lcid);
    CM_LOGI("connHandle=%u, lcid=%u, status: %u", connHandle, lcid, status);

    CM_ICBConnectionState state = type == CM_IMB ? CM_ICB_STATE_IMB_CREATED : CM_ICB_STATE_IOB_CREATED;
    ICGConnectionNode *channelNode = NULL;
    ICGConnectionNode *temp = NULL;
    SDF_DListElmSafeForeach(channelNode, temp, &g_icgConnectionListHead, entry) {
        if (status != DLI_SUCCESS) {
            CM_CHECK_RETURN(context != NULL, "context is null");
            DLI_ICGCbkParam *contexts = (DLI_ICGCbkParam*)context;
            for (uint8_t i = 0; i < contexts->connHandleNum; i++) {
                NotifySingleChannelCallback(state, CM_ICB_FAILED, contexts->connHandle[i], 0, channelNode);
            }
            return;
        }
        for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
            if (channelNode->channel[i].connHandle != connHandle) {
                continue;
            }
            if (!channelNode->channel[i].isPassiveConn && channelNode->type == CM_IMB) {
                CM_NotifyICBConnectionCbk(channelNode->gHandle, ICB_CONNECTED);
            } else {
                CM_NotifyICBConnectionCbk(connHandle, ICB_CONNECTED);
            }
            channelNode->channel[i].isConnected = true;
            NotifySingleChannelCallback(state, CM_ICB_SUCCESS, connHandle, lcid, channelNode);
            return;
        }
    }
    CM_LOGE("channelNode is not exist, connHandle=%u, lcid=%u", connHandle, lcid);
}

static void CM_IOBEstablishedCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_EstablishedCbk(CM_IOB, context, status, cmdRes);
}

static void CM_IMBEstablishedCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_EstablishedCbk(CM_IMB, context, status, cmdRes);
}

static void CM_ICGHandleClean(ICGConnectionNode *channelNode)
{
    if (channelNode->type == CM_IMB) {
        bool isAllDisconnected = true;
        for (uint8_t j = 0; j < channelNode->channelCnt && j < CM_MAX_CHANNEL_COUNT; j++) {
            if (channelNode->channel[j].isConnected) {
                isAllDisconnected = false;
            }
        }
        if (isAllDisconnected) {
            channelNode->gHandle = 0;
        }
    }
}

static void NotifyDisconnectCallback(CM_ICBConnectionState state, CM_ICBErrorCode errorCode, uint8_t channelIndex,
    CM_LinkType disconnectType, ICGConnectionNode *channelNode)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = state;
    icbConn.errorCode = errorCode;
    icbConn.disconnectType = disconnectType;
    icbConn.isIMG = channelNode->type == CM_IMB;
    icbConn.gHandle = channelNode->gHandle;
    icbConn.id = channelNode->id;
    icbConn.channelCnt = 1;
    channel.connHandle = channelNode->channel[channelIndex].connHandle;
    channel.lcid = channelNode->channel[channelIndex].lcid;
    icbConn.channel = &channel;
    NotifyUpperCallback(&icbConn);
}

static void NotifyAllDisconnectCallback(CM_ICBConnectionState state, CM_ICBErrorCode errorCode,
    CM_LinkType disconnectType, ICGConnectionNode *channelNode)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel[CM_MAX_CHANNEL_COUNT] = {};
    icbConn.state = state;
    icbConn.errorCode = errorCode;
    icbConn.disconnectType = disconnectType;
    icbConn.id = channelNode->id;
    icbConn.isIMG = channelNode->type == CM_IMB;
    icbConn.gHandle = channelNode->gHandle;
    icbConn.channelCnt = channelNode->channelCnt;
    for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
        channel[i].connHandle = channelNode->channel[i].connHandle;
        channel[i].lcid = channelNode->channel[i].lcid;
    }
    icbConn.channel = channel;
    NotifyUpperCallback(&icbConn);
}

static void CM_ICBDisconnectCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "param is null");

    DLI_DisconnectEvt *param = (DLI_DisconnectEvt *)cmdRes->eventParameter;
    uint16_t connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
    CM_LOGI("status=%u, connHandle=%u, reason=%u", status, connHandle, param->reason);

    ICGConnectionNode *channelNode = NULL;
    ICGConnectionNode *temp = NULL;
    SDF_DListElmSafeForeach(channelNode, temp, &g_icgConnectionListHead, entry) {
        CM_LOGI("checking channel node, type: %u, id: %u, ghandler: 0x%04x, channel cnt: %u",
            channelNode->type, channelNode->id, channelNode->gHandle, channelNode->channelCnt);
        for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
            CM_LOGI("checking channel[%u], conn handle: 0x%04x, lcid: 0x%04x, label cnt: %u,"
                " passive: %u, connected: %d",
                i, channelNode->channel[i].connHandle, channelNode->channel[i].lcid, channelNode->channel[i].labelCnt,
                channelNode->channel[i].isPassiveConn, channelNode->channel[i].isConnected);
            if (channelNode->channel[i].connHandle != connHandle &&
                channelNode->channel[i].lcid != connHandle) {
                continue;
            }
            if (!channelNode->channel[i].isPassiveConn && channelNode->type == CM_IMB) {
                CM_NotifyICBConnectionCbk(channelNode->gHandle, ICB_DISCONNECTED);
            } else {
                CM_NotifyICBConnectionCbk(channelNode->channel[i].connHandle, ICB_DISCONNECTED);
            }
            channelNode->channel[i].isConnected = false;
            CM_LOGI("conn handler %u disconnected", channelNode->channel[i].connHandle);
            CM_ICGHandleClean(channelNode);

            CM_LinkType type = channelNode->channel[i].lcid == connHandle ? CM_LINK_ACB : CM_LINK_ICB;
            NotifyDisconnectCallback(CM_ICB_STATE_ICB_DELETED, CM_ICB_SUCCESS, i, type, channelNode);

            if (channelNode->channel[i].isPassiveConn) {
                NotifyAllDisconnectCallback(CM_ICB_STATE_ICB_DELETED, CM_ICB_SUCCESS, type, channelNode);
                SDF_DListElmDel(&g_icgConnectionListHead, channelNode, entry);
                FreeICGChannelNode((SDF_DListEntry_S *)channelNode);
            }
            return;
        }
    }
    CM_LOGI("connHandle=%u is not icb connection", connHandle);
}

static void NotifyEstablishedCbk(CM_ICBConnectionState state, CM_ICBErrorCode errorCode, uint8_t id,
    uint16_t connHandle, uint16_t lcid)
{
    CM_ICBConnection icbConn = {};
    icbConn.state = state;
    icbConn.errorCode = errorCode;
    icbConn.id = id;
    icbConn.channelCnt = 1;
    CM_ICBChannel channel = {};
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    icbConn.channel = &channel;
    NotifyUpperCallback(&icbConn);
}

static void CM_ConnectReqCbk(CM_ICBType type, void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "param connect req cbk param is null");
    DLI_ICBConnectReqEvt *param = (DLI_ICBConnectReqEvt *)cmdRes->eventParameter;
    ICGConnectionNode *channelNode = MallocICGChannelNode(1);
    if (channelNode != NULL) {
        DLI_AcceptICBReqParam reqParam = {};
        reqParam.type = type;
        reqParam.opCode = type == CM_IMB ? DLI_ACCEPT_IMB_REQ : DLI_ACCEPT_IOB_REQ;
        reqParam.connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
        uint32_t ret = DLI_AcceptICBReq(&reqParam);
        if (ret == DLI_SUCCESS) {
            channelNode->type = type;
            channelNode->id = param->icgId;
            channelNode->channelCnt = 1;
            channelNode->channel->connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
            channelNode->channel->lcid = DECODE2BYTE_LITTLE((uint8_t *)&param->lcid);
            channelNode->channel->isPassiveConn = true;
            AddICGChannelNode(channelNode);
            CM_LOGI("accept imb req success, connHandle=%u", param->connHandle);
            return;
        } else {
            FreeICGChannelNode((SDF_DListEntry_S *)channelNode);
            CM_LOGE("accept connect req failed, ret = %u", ret);
        }
    } else {
        CM_LOGE("malloc channel failed, id=%u", param->icgId);
    }
    DLI_RejectICBReqParam rejectParam = {};
    rejectParam.opCode = type == CM_IMB ? DLI_REJECT_IMB_REQ : DLI_REJECT_IOB_REQ;
    rejectParam.regOpCode = type == CM_IMB ? DLI_CBK_REJECT_IMB_REQ : DLI_CBK_REJECT_IOB_REQ;
    rejectParam.id = param->icgId;
    rejectParam.connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
    rejectParam.reason = DLI_MEMORY_CAPACITY_EXCEEDED;
    uint32_t ret = DLI_RejectICBReq(&rejectParam);
    CM_CHECK_RETURN(ret != DLI_SUCCESS, "reject connect req success");
    CM_LOGE("reject connect req failed, ret = %u", ret);
    NotifyEstablishedCbk(type == CM_IMB ? CM_ICB_STATE_IMB_REJECTED : CM_ICB_STATE_IOB_REJECTED,
        type == CM_IMB ? CM_IMB_FAILED : CM_IOB_FAILED, param->icgId,
        DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle), DECODE2BYTE_LITTLE((uint8_t *)&param->lcid));
}

static void CM_IOBConnectReqCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_ConnectReqCbk(CM_IOB, context, status, cmdRes);
}

static void CM_IMBConnectReqCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_ConnectReqCbk(CM_IMB, context, status, cmdRes);
}

static void CM_RejectReqCbk(CM_ICBType type, void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(context != NULL && cmdRes != NULL && cmdRes->eventParameter != NULL,
        "param reject req cbk param is null");
    DLI_ICBRejectReqEvt  *param = (DLI_ICBRejectReqEvt  *)cmdRes->eventParameter;
    CM_LOGI("status=%u, connHandle=%u", status, param->connHandle);
    CM_ICBConnection icbConn = {};
    icbConn.state = type == CM_IMB ? CM_ICB_STATE_IMB_REJECTED : CM_ICB_STATE_IOB_REJECTED;
    icbConn.errorCode = GetICBErrorCode(status, type);
    icbConn.id = *(uint8_t *)(uintptr_t)context;
    icbConn.channelCnt = 1;
    CM_ICBChannel channel = {};
    channel.connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
    icbConn.channel = &channel;
    NotifyUpperCallback(&icbConn);
}

static void CM_IOBRejectReqCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_RejectReqCbk(CM_IOB, context, status, cmdRes);
}

static void CM_IMBRejectReqCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_RejectReqCbk(CM_IMB, context, status, cmdRes);
}

static void NotifyDataPathAddedCallback(CM_ICBErrorCode errorCode, uint16_t connHandle, uint16_t lcid,
    uint8_t direction, ICGConnectionNode *channelNode)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = CM_ICB_STATE_ICB_DATA_PATH_SETUP;
    icbConn.errorCode = errorCode;
    if (channelNode != NULL) {
        if (channelNode->type == CM_IMB) {
            icbConn.isIMG = true;
            icbConn.gHandle = channelNode->gHandle;
        }
        icbConn.id = channelNode->id;
    }
    icbConn.channelCnt = 1;
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    channel.direction = direction;
    icbConn.channel = &channel;
    NotifyUpperCallback(&icbConn);
}

static void NotifyDataPathFailedCbk(CM_ICBConnectionState state, DLI_ICGCbkParam *cbkParam)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = state;
    icbConn.errorCode = GetICBErrorCode(DLI_UNSPECIFIED_ERROR, cbkParam->type);
    icbConn.id = cbkParam->id;
    icbConn.channelCnt = 1;
    channel.connHandle = cbkParam->connHandle[0];
    channel.direction = cbkParam->direction;
    icbConn.channel = &channel;
    NotifyUpperCallback(&icbConn);
}

static void CM_ICBSetDataPathCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(context != NULL, "context is null");

    DLI_ICGCbkParam *cbkParam = (DLI_ICGCbkParam *)context;
    CM_LOGI("status=%u, type=%u, id=%u", status, cbkParam->type, cbkParam->id);

    ICGConnectionNode *channelNode = FindICGConnectionNodeByTypeId(cbkParam->type, cbkParam->id);
    if (channelNode == NULL) {
        CM_LOGE("icg param has not been set, id=%u", cbkParam->id);
        NotifyDataPathFailedCbk(CM_ICB_STATE_ICB_DATA_PATH_SETUP, cbkParam);
        return;
    }
    if (status != DLI_SUCCESS) {
        CM_LOGE("set data path failed, id=%u", cbkParam->id);
        NotifyDataPathFailedCbk(CM_ICB_STATE_ICB_DATA_PATH_SETUP, cbkParam);
        return;
    }
    if (cmdRes == NULL || cmdRes->eventParameter == NULL) {
        CM_LOGE("param is null, id=%u", cbkParam->id);
        NotifyDataPathFailedCbk(CM_ICB_STATE_ICB_DATA_PATH_SETUP, cbkParam);
        return;
    }

    DLI_ICBDataPathEvt *param = (DLI_ICBDataPathEvt *)cmdRes->eventParameter;
    uint16_t connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);

    for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
        if (channelNode->channel[i].connHandle != connHandle &&
            (channelNode->type == CM_IMB && channelNode->gHandle != connHandle)) {
            continue;
        }
        NotifyDataPathAddedCallback(CM_ICB_SUCCESS, connHandle, channelNode->channel[i].lcid,
            cbkParam->direction, channelNode);
        return;
    }
    NotifyDataPathFailedCbk(CM_ICB_STATE_ICB_DATA_PATH_SETUP, cbkParam);
    CM_LOGE("channelNode is not exist, connHandle=%u", connHandle);
}

static void NotifyDataPathRemovedCallback(CM_ICBErrorCode errorCode, uint16_t connHandle, uint16_t lcid,
    uint8_t direction, ICGConnectionNode *channelNode)
{
    CM_ICBConnection icbConn = {};
    CM_ICBChannel channel = {};
    icbConn.state = CM_ICB_STATE_ICB_DATA_PATH_REMOVED;
    icbConn.errorCode = errorCode;
    if (channelNode != NULL) {
        if (channelNode->type == CM_IMB) {
            icbConn.isIMG = true;
            icbConn.gHandle = channelNode->gHandle;
        }
        icbConn.id = channelNode->id;
    }
    icbConn.channelCnt = 1;
    channel.connHandle = connHandle;
    channel.lcid = lcid;
    channel.direction = direction;
    icbConn.channel = &channel;
    NotifyUpperCallback(&icbConn);
}

static void CM_ICBRemoveDataPathCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(context != NULL, "context is null");

    DLI_ICGCbkParam *cbkParam = (DLI_ICGCbkParam *)context;
    CM_LOGI("status=%u, type=%u, id=%u", status, cbkParam->type, cbkParam->id);

    ICGConnectionNode *channelNode = FindICGConnectionNodeByTypeId(cbkParam->type, cbkParam->id);
    if (channelNode == NULL) {
        CM_LOGE("icg param has not been set, id=%u", cbkParam->id);
        NotifyDataPathFailedCbk(CM_ICB_STATE_ICB_DATA_PATH_REMOVED, cbkParam);
        return;
    }
    if (status != DLI_SUCCESS) {
        CM_LOGE("set data path failed, id=%u", cbkParam->id);
        NotifyDataPathFailedCbk(CM_ICB_STATE_ICB_DATA_PATH_REMOVED, cbkParam);
        return;
    }
    if (cmdRes == NULL || cmdRes->eventParameter == NULL) {
        CM_LOGE("param is null, id=%u", cbkParam->id);
        NotifyDataPathFailedCbk(CM_ICB_STATE_ICB_DATA_PATH_REMOVED, cbkParam);
        return;
    }

    DLI_ICBDataPathEvt *param = (DLI_ICBDataPathEvt *)cmdRes->eventParameter;
    uint16_t connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);

    for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
        if (channelNode->channel[i].connHandle != connHandle &&
            (channelNode->type == CM_IMB && channelNode->gHandle != connHandle)) {
            continue;
        }
        NotifyDataPathRemovedCallback(CM_ICB_SUCCESS, connHandle, channelNode->channel[i].lcid,
            cbkParam->direction, channelNode);
        return;
    }
    NotifyDataPathFailedCbk(CM_ICB_STATE_ICB_DATA_PATH_REMOVED, cbkParam);
    CM_LOGE("channelNode is not exist, connHandle=%u", connHandle);
}

static void CM_IOBQualityReportConvert(CM_ICBQuality *icbQuality, const DLI_IOBQualityReportEvt *param)
{
    icbQuality->icbType = CM_IOB;
    icbQuality->diffTotal = DECODE4BYTE_LITTLE((uint8_t *)&param->diffTotal);
    icbQuality->diffMax = DECODE4BYTE_LITTLE((uint8_t *)&param->diffMax);
    icbQuality->diffAvg = DECODE4BYTE_LITTLE((uint8_t *)&param->diffAvg);
    icbQuality->connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
    icbQuality->txFlushed = DECODE2BYTE_LITTLE((uint8_t *)&param->txFlushed);
    icbQuality->rxLossPktCnt = DECODE2BYTE_LITTLE((uint8_t *)&param->rxLossPktCnt);
    icbQuality->rxLossMaxContPkt = DECODE2BYTE_LITTLE((uint8_t *)&param->rxLossMaxContPkt);
    icbQuality->rssi = param->rssi;
    icbQuality->ackRate = param->ackRate;
    icbQuality->reserve1 = DECODE4BYTE_LITTLE((uint8_t *)&param->reserve1);
    icbQuality->reserve2 = DECODE4BYTE_LITTLE((uint8_t *)&param->reserve2);
    icbQuality->reserve3 = DECODE4BYTE_LITTLE((uint8_t *)&param->reserve3);
    icbQuality->reserve4 = DECODE4BYTE_LITTLE((uint8_t *)&param->reserve4);
}

static void CM_IMBQualityReportConvert(CM_ICBQuality *icbQuality, const DLI_IMBQualityReportEvt *param, uint8_t linkIdx)
{
    icbQuality->icbType = CM_IMB;
    icbQuality->diffTotal = DECODE4BYTE_LITTLE((uint8_t *)&param->diffTotal);
    icbQuality->diffMax = DECODE4BYTE_LITTLE((uint8_t *)&param->diffMax);
    icbQuality->diffAvg = DECODE4BYTE_LITTLE((uint8_t *)&param->diffAvg);
    icbQuality->connHandle = param->channelInfo[linkIdx].channelConnHandle;
    icbQuality->txFlushed = DECODE2BYTE_LITTLE((uint8_t *)&param->txFlushed);
    icbQuality->rxLossPktCnt = DECODE2BYTE_LITTLE((uint8_t *)&param->rxLossPktCnt);
    icbQuality->rxLossMaxContPkt = DECODE2BYTE_LITTLE((uint8_t *)&param->rxLossMaxContPkt);
    icbQuality->rssi = param->rssi;
    icbQuality->ackRate = param->ackRate;
    icbQuality->missedRate = DECODE4BYTE_LITTLE((uint8_t *)&param->missedRate);
    icbQuality->errPacketRate = DECODE4BYTE_LITTLE((uint8_t *)&param->errPacketRate);
}

static void CM_IOBQualityReportCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    (void)status;
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "param is null");

    DLI_IOBQualityReportEvt *param = (DLI_IOBQualityReportEvt *)cmdRes->eventParameter;
    CM_ICBQuality iobQuality = {0};
    CM_IOBQualityReportConvert(&iobQuality, param);
    if (g_icbCallback.qualityReportCbk != NULL) {
        g_icbCallback.qualityReportCbk(&iobQuality);
    }
}

static void CM_ParamUpdateCbk(CM_ICBType type, void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "param is null, type=%u", type);

    DLI_ICBParamUpdateEvt *param = (DLI_ICBParamUpdateEvt *)cmdRes->eventParameter;
    uint16_t connHandle = DECODE2BYTE_LITTLE((uint8_t *)&param->connHandle);
    uint16_t lcid = DECODE2BYTE_LITTLE((uint8_t *)&param->lcid);
    CM_LOGI("type=%u, status=%u, connHandle=%u, lcid=%u", type, status, connHandle, lcid);

    CM_ICBConnectionState state = type == CM_IMB ? CM_ICB_STATE_IMG_UPDATED : CM_ICB_STATE_IOG_UPDATED;
    ICGConnectionNode *channelNode = NULL;
    ICGConnectionNode *temp = NULL;
    SDF_DListElmSafeForeach(channelNode, temp, &g_icgConnectionListHead, entry) {
        for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
            if (channelNode->channel[i].connHandle != connHandle) {
                continue;
            }
            if (status != DLI_SUCCESS) {
                NotifySingleChannelCallback(state, CM_ICB_FAILED, connHandle, lcid, channelNode);
                return;
            }
            CM_ICBConnection icbConn = {};
            CM_ICBChannel channel = {};
            icbConn.state = state;
            icbConn.errorCode = CM_ICB_SUCCESS;
            icbConn.isIMG = channelNode->type == CM_IMB;
            icbConn.gHandle = channelNode->gHandle;
            icbConn.id = channelNode->id;
            icbConn.channelCnt = 1;
            channel.connHandle = connHandle;
            channel.lcid = lcid;
            channel.labelId = param->labelId;
            icbConn.channel = &channel;
            NotifyUpperCallback(&icbConn);
            return;
        }
    }
    CM_LOGE("channelNode is not exist, connHandle=%u", connHandle);
}

static void CM_IOGParamUpdateCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_ParamUpdateCbk(CM_IOB, context, status, cmdRes);
}

static void CM_IMGParamUpdateCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_ParamUpdateCbk(CM_IMB, context, status, cmdRes);
}

static void CM_IMBQualityReportCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    (void)status;
    CM_CHECK_RETURN(CM_ICBIsInited(), "icb mgr is not inited");
    CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "param is null");

    DLI_IMBQualityReportEvt *param = (DLI_IMBQualityReportEvt *)cmdRes->eventParameter;
    uint8_t txCount = param->txCount;
    for (uint8_t i = 0; i < txCount; i++) {
        CM_ICBQuality imbQuality = {0};
        // 无效handle号不上报
        if (param->channelInfo[i].channelConnHandle == 0) {
            continue;
        }
        CM_IMBQualityReportConvert(&imbQuality, param, i);
        if (g_icbCallback.qualityReportCbk != NULL) {
            g_icbCallback.qualityReportCbk(&imbQuality);
        }
    }
}

void CM_GetLcidByConnHandleInner(void *arg)
{
    CM_CHECK_RETURN(arg != NULL, "arg is null");
    CM_GetLcidCtx *ctx = (CM_GetLcidCtx *)arg;
    ctx->lcid = CM_INVALID_LCID;

    uint16_t connHandle = ctx->connHandle;
    ICGConnectionNode *channelNode = NULL;
    ICGConnectionNode *temp = NULL;
    SDF_DListElmSafeForeach(channelNode, temp, &g_icgConnectionListHead, entry) {
        for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
            if (channelNode->channel[i].connHandle != connHandle) {
                continue;
            }
            ctx->lcid = channelNode->channel[i].lcid;
            return;
        }
    }
}

static void CM_FreqBandSwitchCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "cmd res is null");
    DLI_FreqBandSwitchEvt *evtParam = (DLI_FreqBandSwitchEvt *)cmdRes->eventParameter;

    CM_FreqBandSwitchParam param = {};
    param.status = (uint8_t)status;
    param.connHandle = DECODE2BYTE_LITTLE(&evtParam->connHandle);

    CM_GetLcidCtx ctx = {0};
    ctx.connHandle = param.connHandle;
    CM_GetLcidByConnHandleInner((void *)&ctx);
    param.lcid = ctx.lcid;
    param.oldFreqBand = evtParam->oldFreqBand;
    param.newFreqBand = evtParam->newFreqBand;

    FreqBandListenerNode *node = NULL;
    FreqBandListenerNode *temp = NULL;
    SDF_DListElmSafeForeach(node, temp, &g_freqBandListener, entry) {
        if (node->listener != NULL) {
            node->listener(&param);
        }
    }
}

static struct DLI_CbkLineStru g_icbDliCbks[] = {
    {DLI_CBK_REMOVE_ICG_PARAM, CM_ICGRemoveParamCbk},                    // cmd cmp evt

    {DLI_CBK_SET_IOG_PARAM, CM_IOGSetParamCbk},                          // cmd cmp evt
    {DLI_CBK_IOG_LABEL_REPORT, CM_IOGLabelReportCbk},                    // 本端和对端都是这个回调
    {DLI_CBK_IOB_ESTABLISHED, CM_IOBEstablishedCbk},                     // create（本端） 和 accept req（对端）都是这个回调
    {DLI_CBK_IOB_CONNECT_REQ, CM_IOBConnectReqCbk},                      // 单独触发
    {DLI_CBK_IOB_REPORT_PARAM, CM_IOBQualityReportCbk},                  // 收发数据以后触发
    {DLI_CBK_IOG_PARAM_UPDATE, CM_IOGParamUpdateCbk},                    // 本端和对端都是这个回调
    {DLI_CBK_REJECT_IOB_REQ, CM_IOBRejectReqCbk},                        // cmd cmp evt

    {DLI_CBK_SET_IMG_PARAM, CM_IMGSetParamCbk},                          // cmd cmp evt
    {DLI_CBK_IMG_LABEL_REPORT, CM_IMGLabelReportCbk},                    // 本端和对端都是这个回调
    {DLI_CBK_IMB_ESTABLISHED, CM_IMBEstablishedCbk},                     // create（本端） 和 accept req（对端）都是这个回调
    {DLI_CBK_IMB_CONNECT_REQ, CM_IMBConnectReqCbk},                      // 单独触发
    {DLI_CBK_IMB_REPORT_PARAM, CM_IMBQualityReportCbk},                  // 收发数据以后触发
    {DLI_CBK_IMG_PARAM_UPDATE, CM_IMGParamUpdateCbk},                    // 本端和对端都是这个回调
    {DLI_CBK_REJECT_IMB_REQ, CM_IMBRejectReqCbk},                        // cmd cmp evt

    {DLI_CBK_ICB_SETUP_DATA_PATH, CM_ICBSetDataPathCbk},                 // cmd cmp evt
    {DLI_CBK_ICB_REMOVE_DATA_PATH, CM_ICBRemoveDataPathCbk},             // cmd cmp evt

    {DLI_CBK_SWITCH_FREQ_BAND, CM_FreqBandSwitchCbk},                    // 频段切换后触发
};

uint32_t CM_ICBMgrInit(void)
{
    SDF_DListHeadInit(&g_icgConnectionListHead);
    uint32_t ret = DLI_CmdCbkReg(CM_ICB, NULL, 0,
        g_icbDliCbks, sizeof(g_icbDliCbks) / sizeof(struct DLI_CbkLineStru));
    if (ret != DLI_SUCCESS) {
        CM_LOGE("dli register cbk failed, ret=%u", ret);
        return CM_FAIL;
    }
    ret = CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_ICB, CM_DLI_ADAPTER_DISCONNECT, CM_ICBDisconnectCbk);
    if (ret != CM_SUCCESS) {
        DLI_CmdCbkUnReg(CM_ICB, NULL, 0,
            g_icbDliCbks, sizeof(g_icbDliCbks) / sizeof(struct DLI_CbkLineStru));
        CM_LOGE("register dli adapter cbk failed, ret=%u", ret);
        return CM_FAIL;
    }
    CM_LOGI("icb mgr init success");
    return CM_SUCCESS;
}

uint32_t CM_ICBMgrDeinit(void)
{
    SDF_DListDestroy(&g_icgConnectionListHead, FreeICGChannelNode);
    DLI_CmdCbkUnReg(CM_ICB, NULL, 0, g_icbDliCbks, sizeof(g_icbDliCbks) / sizeof(struct DLI_CbkLineStru));
    (void)CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_ICB, CM_DLI_ADAPTER_DISCONNECT);
    CM_LOGI("icb mgr deinit success");
    return CM_SUCCESS;
}

uint32_t CM_ICBMgrEnable(void)
{
    CM_LOGI("icb mgr enable success");
    return CM_SUCCESS;
}

uint32_t CM_ICBMgrDisable(void)
{
    SDF_DListDestroy(&g_icgConnectionListHead, FreeICGChannelNode);
    SDF_DListHeadInit(&g_icgConnectionListHead);
    CM_LOGI("icb mgr disable success");
    return CM_SUCCESS;
}

uint32_t CM_ICBMgrRegisterCb(const CM_ICBCallback *cb)
{
    g_icbCallback = *cb;
    CM_LOGI("icb mgr register cb success");
    return CM_SUCCESS;
}

uint32_t CM_ICBMgrUnregisterCb(void)
{
    CM_ICBCallback cb = {NULL};
    g_icbCallback = cb;
    CM_LOGI("icb mgr unregister cb success");
    return CM_SUCCESS;
}

uint32_t CM_ICBMgrSetParam(DLI_ICGParam *param)
{
    CM_ICBErrorCode errorCode = GetICBErrorCode(DLI_UNSPECIFIED_ERROR, param->type);
    ICGConnectionNode *channelNode = FindICGConnectionNodeByTypeId(param->type, param->id);

    if (channelNode != NULL) {
        CM_LOGE("icg channel has been set, id=%u", param->id);
        return errorCode;
    }
    DLI_ICGCbkParam cbkParam = {};
    cbkParam.type = param->type;
    cbkParam.id = param->id;
    cbkParam.connHandleNum = 0;
    if (DLI_SetICGParam(param, &cbkParam) == DLI_SUCCESS) {
        return CM_ICB_SUCCESS;
    }
    CM_LOGE("dli set icg param failed, id=%u", param->id);
    return errorCode;
}

uint32_t CM_ICBMgrSetTestParam(DLI_ICGTestParam *param, bool mcast, bool supportAutorate)
{
    DLI_ICGCbkParam cbkParam = {};
    cbkParam.type = param->type;
    cbkParam.id = param->id;
    cbkParam.connHandleNum = 0;
    if (supportAutorate) {
        if (DLI_SetICGTestParamExt(param, mcast, &cbkParam) == DLI_SUCCESS) {
            return CM_ICB_SUCCESS;
        }
    } else {
        if (DLI_SetICGTestParam(param, &cbkParam) == DLI_SUCCESS) {
            return CM_ICB_SUCCESS;
        }
    }
    CM_LOGE("dli set icg param failed, id=%u", param->id);
    return GetICBErrorCode(DLI_UNSPECIFIED_ERROR, param->type);
}

uint32_t CM_ICBMgrRemoveParam(CM_ICGRemovedParam *param)
{
    DLI_CmdOpcode opCode = (param->type == CM_IMB) ? DLI_REMOVE_IMG_PARAM : DLI_REMOVE_IOG_PARAM;
    CM_ICBErrorCode errorCode = GetICBErrorCode(DLI_UNSPECIFIED_ERROR, param->type);
    ICGConnectionNode *channelNode = NULL;
    ICGConnectionNode *temp = NULL;
    SDF_DListElmSafeForeach(channelNode, temp, &g_icgConnectionListHead, entry) {
        if (channelNode->id != param->id) {
            continue;
        }
        DLI_ICGCbkParam cbkParam = {};
        cbkParam.type = param->type;
        cbkParam.id = param->id;
        cbkParam.connHandleNum = 0;
        if (DLI_RemoveICGParam(opCode, &cbkParam) == DLI_SUCCESS) {
            return CM_ICB_SUCCESS;
        }
        CM_LOGE("dli remove icg param failed, id=%u", param->id);
        return errorCode;
    }
    CM_LOGE("icg channel is not existed, id=%u", param->id);
    return errorCode;
}

uint32_t CM_ICGMgrSetLabel(DLI_ICGLabelParam *param, bool mcast, bool supportSubrate, bool supportAutorate)
{
    ICGConnectionNode *channelNode = FindICGConnectionNodeByTypeId(param->type, param->id);
    CM_ICBErrorCode errorCode = GetICBErrorCode(DLI_UNSPECIFIED_ERROR, param->type);
    if (channelNode == NULL) {
        CM_LOGE("icg param has not been set, id=%u", param->id);
        return errorCode;
    }

    for (uint32_t i = 0; i < param->icbCnt; i++) {
        if (supportSubrate) {
            CM_SetACBSubrateInnerParam subrateParam = {};
            subrateParam.lcid = param->icb[i].lcid;
            subrateParam.subrate = SLE_ACB_SUBRATE_AUDIO;
            uint32_t ret = CM_InnerSetACBSubrate(&subrateParam);
            CM_LOGI("set acb subrate, lcid=%u, ret=%u", param->icb[i].lcid, ret);
        }

        for (uint32_t j = 0; j < channelNode->channelCnt && j < CM_MAX_CHANNEL_COUNT; j++) {
            if (param->icb[i].connHandle == channelNode->channel[j].connHandle) {
                channelNode->channel[j].lcid = param->icb[i].lcid;
                CM_LOGI("conn handler 0x%04x update lcid to 0x%04x",
                    channelNode->channel[j].connHandle, channelNode->channel[j].lcid);
            }
        }
    }

    if (supportAutorate) {
        DLI_ICGCbkParam cbkParam = {};
        cbkParam.type = param->type;
        cbkParam.id = param->id;
        cbkParam.connHandleNum = param->icbCnt;
        for (uint8_t i = 0; i < cbkParam.connHandleNum && i < SLE_ICB_MAX_NUM; i++) {
            cbkParam.connHandle[i] = param->icb[i].connHandle;
        }
        if (DLI_SetICGLabelExt(param, mcast, &cbkParam) == DLI_SUCCESS) {
            return CM_ICB_SUCCESS;
        }
        CM_LOGE("dli set icg label failed, id=%u", param->id);
        return errorCode;
    }
    
    return CM_ICB_SUCCESS;
}

uint32_t CM_ICBMgrAddConnection(DLI_ICBConnectionParam *param, bool mcast, bool supportAutorate)
{
    ICGConnectionNode *channelNode = FindICGConnectionNodeByTypeId(param->type, param->id);
    CM_ICBErrorCode errorCode = GetICBErrorCode(DLI_UNSPECIFIED_ERROR, param->type);
    if (channelNode == NULL) {
        CM_LOGE("icg param has not been set, id=%u", param->id);
        return errorCode;
    }
    DLI_ICGCbkParam cbkParam = {};
    cbkParam.type = param->type;
    cbkParam.id = param->id;
    cbkParam.connHandleNum = param->channelCnt;
    for (uint8_t i = 0; i < cbkParam.connHandleNum; i++) {
        cbkParam.connHandle[i] = param->channel->connHandle;
    }

    if (supportAutorate) {
        if (DLI_CreateICBExt(param, mcast, &cbkParam) == DLI_SUCCESS) {
            return CM_ICB_SUCCESS;
        }
    } else {
        if (DLI_CreateICB(param, &cbkParam) == DLI_SUCCESS) {
            return CM_ICB_SUCCESS;
        }
    }
    CM_LOGE("dli create icb failed, id=%u", param->id);
    return errorCode;
}

uint32_t CM_ICBMgrDelConnection(DLI_ICBConnectionParam *param)
{
    ICGConnectionNode *channelNode = FindICGConnectionNodeByTypeId(param->type, param->id);
    if (channelNode == NULL) {
        CM_LOGE("icg channel node has not been created, type=%u, id=%u", param->type, param->id);
        return CM_ICB_FAILED;
    }
    bool isSuccess = false;
    for (uint8_t i = 0; i < param->channelCnt; i++) {
        uint16_t connHandle = param->channel[i].connHandle;
        DLI_DisconnectParam disconnectParam = {};
        disconnectParam.connHandle = connHandle;
        disconnectParam.reason = DLI_CONNECTION_TERMINATED_BY_LOCAL_HOST;
        DLI_ICGCbkParam cbkParam = {};
        cbkParam.type = param->type;
        cbkParam.id = param->id;
        cbkParam.connHandleNum = 0;
        if (DLI_DisconnectICB(&disconnectParam, &cbkParam) == DLI_SUCCESS) {
            CM_LOGI("dli disconnect success, conHandle=%u", connHandle);
            isSuccess = true;
            continue;
        }
        CM_LOGE("dli destroy icb failed, id=%u, conHandle=%u", param->id, connHandle);
        return CM_ICB_FAILED;
    }
    return isSuccess ? CM_ICB_SUCCESS : CM_ICB_FAILED;
}

uint32_t CM_ICGMgrUpdateParam(DLI_ICGUpdatedParam *param, bool mcast)
{
    ICGConnectionNode *channelNode = FindICGConnectionNodeByTypeId(param->type, param->id);
    CM_ICBErrorCode errorCode = GetICBErrorCode(DLI_UNSPECIFIED_ERROR, param->type);
    if (channelNode == NULL) {
        CM_LOGE("icg param has not been set, id=%u", param->id);
        return errorCode;
    }
    DLI_ICGCbkParam cbkParam = {};
    cbkParam.type = param->type;
    cbkParam.id = param->id;
    cbkParam.connHandleNum = 0;
    if (DLI_UpdateICGParamExt(param, mcast, &cbkParam) == DLI_SUCCESS) {
        return CM_ICB_SUCCESS;
    }
    CM_LOGE("dli update icg param failed, id=%u", param->id);
    return errorCode;
}

uint32_t CM_ICBMgrSetupDataPath(DLI_SetupICBDataPathParam *param)
{
    ICGConnectionNode *channelNode = NULL;
    ICGConnectionNode *temp = NULL;
    SDF_DListElmSafeForeach(channelNode, temp, &g_icgConnectionListHead, entry) {
        for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
            if (channelNode->channel[i].connHandle != param->connHandle &&
                (channelNode->type == CM_IMB && channelNode->gHandle != param->connHandle)) {
                continue;
            }
            DLI_ICGCbkParam cbkParam = {};
            cbkParam.type = channelNode->type;
            cbkParam.id = channelNode->id;
            cbkParam.direction = param->direction;
            cbkParam.connHandle[0] = param->connHandle;
            cbkParam.connHandleNum = 1;
            if (DLI_SetupICBDataPath(param, &cbkParam) == DLI_SUCCESS) {
                return CM_ICB_SUCCESS;
            }
            CM_LOGE("dli setup icb data path failed, connHandle=%u", param->connHandle);
            return CM_ICB_FAILED;
        }
    }
    CM_LOGE("icb channel is not existed, conHandle=%u", param->connHandle);
    return CM_ICB_FAILED;
}

uint32_t CM_ICBMgrRemoveDataPath(DLI_RemoveICBDataPathParam *param)
{
    ICGConnectionNode *channelNode = NULL;
    ICGConnectionNode *temp = NULL;
    SDF_DListElmSafeForeach(channelNode, temp, &g_icgConnectionListHead, entry) {
        for (uint8_t i = 0; i < channelNode->channelCnt && i < CM_MAX_CHANNEL_COUNT; i++) {
            if (channelNode->channel[i].connHandle != param->connHandle &&
                (channelNode->type == CM_IMB && channelNode->gHandle != param->connHandle)) {
                continue;
            }
            DLI_ICGCbkParam cbkParam = {};
            cbkParam.type = channelNode->type;
            cbkParam.id = channelNode->id;
            cbkParam.direction = param->direction;
            cbkParam.connHandle[0] = param->connHandle;
            cbkParam.connHandleNum = 1;
            if (DLI_RemoveICBDataPath(param, &cbkParam) == DLI_SUCCESS) {
                return CM_ICB_SUCCESS;
            }
            CM_LOGE("dli remove icb data path failed, connHandle=%u", param->connHandle);
            return CM_ICB_FAILED;
        }
    }
    CM_LOGE("icb channel is not existed, conHandle=%u", param->connHandle);
    return CM_ICB_FAILED;
}

uint32_t CM_RegisterICBConnectionCbk(CM_ICBConnectionStatusCbk cbk)
{
    if (cbk == NULL) {
        return CM_INVALID_PARAM_ERR;
    }
    g_icbConnStatusCb = cbk;
    return CM_SUCCESS;
}

uint32_t CM_UnregisterICBConnectionCbk(void)
{
    g_icbConnStatusCb = NULL;
    return CM_SUCCESS;
}

uint32_t CM_ICBMgrListenFreqBandSwitchEvent(CM_FreqBandListener listener)
{
    FreqBandListenerNode *node = NULL;
    FreqBandListenerNode *temp = NULL;
    SDF_DListElmSafeForeach(node, temp, &g_freqBandListener, entry) {
        if (node->listener == listener) {
            CM_LOGW("listener has been added");
            return CM_SUCCESS;
        }
    }
    node = (FreqBandListenerNode *)SDF_MemZalloc(sizeof(FreqBandListenerNode));
    if (node == NULL) {
        CM_LOGE("malloc listener node failed");
        return CM_FAIL;
    }
    node->listener = listener;
    SDF_DListEntryInit(&node->entry);
    SDF_DListElmTailInsert(&g_freqBandListener, node, entry);
    CM_LOGI("listener add successfully");
    return CM_SUCCESS;
}

uint32_t CM_ICBMgrUnlistenFreqBandSwitchEvent(CM_FreqBandListener listener)
{
    FreqBandListenerNode *node = NULL;
    FreqBandListenerNode *temp = NULL;
    SDF_DListElmSafeForeach(node, temp, &g_freqBandListener, entry) {
        if (node->listener == listener) {
            SDF_DListElmDel(&g_freqBandListener, node, entry);
            SDF_MemFree(node);
            return CM_SUCCESS;
        }
    }
    CM_LOGW("listener is already removed");
    return CM_SUCCESS;
}
