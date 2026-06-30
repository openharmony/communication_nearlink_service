/****************************************************************************
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
****************************************************************************/

#include "cm_signaling_trans_channel.h"

#include "securec.h"

#include "byte_codec.h"
#include "cm_def.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "cm_signaling_cap.h"
#include "cm_signaling_manage.h"
#include "cm_signaling_struct.h"
#include "sdf_mem.h"

typedef struct {
    uint16_t lcid;
    uint8_t srcTcid;
    uint8_t dstTcid;
} CM_SignalingTransChanTimoutArgs_S;

static CM_SignalingTransChanCbks_S g_signalingTransChanCbks = { 0 };

#define CM_SLQI_SURPPORT_NUM 1U
#define CM_SLQI_SIZE (sizeof(CM_PreferredSlqiList_S) + sizeof(uint8_t) * CM_SLQI_SURPPORT_NUM)

uint32_t CM_SignalingTransChanCbksRegister(const CM_SignalingTransChanCbks_S *cbks)
{
    CM_CHECK_RETURN_RET(cbks != NULL && cbks->establishReqCbk != NULL && cbks->establishRspCbk != NULL &&
                            cbks->releaseReqCbk != NULL && cbks->releaseRspCbk != NULL,
                        CM_NULL_POINTER, "transport channel signaling cbks is null");
    g_signalingTransChanCbks = *cbks;
    return CM_SUCCESS;
}

void CM_SignalingTransChanCbksUnregister(void)
{
    CM_SignalingTransChanCbks_S cbksTemp = { 0 };

    g_signalingTransChanCbks = cbksTemp;
}

static CM_SignalingTransChanTimoutArgs_S *CM_SignalingTransChanTimoutArgsNew(uint16_t lcid, uint8_t srcTcid,
                                                                             uint8_t dstTcid)
{
    CM_SignalingTransChanTimoutArgs_S *args =
        (CM_SignalingTransChanTimoutArgs_S *)SDF_MemZalloc(sizeof(CM_SignalingTransChanTimoutArgs_S));
    if (args == NULL) {
        return NULL;
    }
    args->lcid = lcid;
    args->srcTcid = srcTcid;
    args->dstTcid = dstTcid;

    return args;
}

static void CM_SignalingTransChanEstablishReqTimeout(void *args)
{
    if (args == NULL) {
        return;
    }

    CM_SignalingTransChanTimoutArgs_S *timeoutArgs = (CM_SignalingTransChanTimoutArgs_S *)args;
    CM_LOGW("transport channel establish timeout, lcid: %hu, src tcid: %hhu", timeoutArgs->lcid, timeoutArgs->srcTcid);

    CM_SignalingTransChanEstablishRsp_S rsp = {
        .srcTcid = timeoutArgs->srcTcid,
        .dstTcid = timeoutArgs->dstTcid,
        .result = CM_RESULT_ESTABLISH_TIMEOUT,
        .lcConfig = NULL,
    };

    if (g_signalingTransChanCbks.establishRspCbk != NULL) {
        g_signalingTransChanCbks.establishRspCbk(timeoutArgs->lcid, &rsp);
    }
}

static uint32_t CM_SignalingTranModeStreamConfigPack(
    CM_SignalingTransModeStreamConfig_S *streamConfig, uint8_t *data)
{
    CM_TransModeStreamConfig_S *config = (CM_TransModeStreamConfig_S *)data;
    ENCODE2BYTE_LITTLE(&config->flushTimeout, streamConfig->flushTimeout);
    ENCODE2BYTE_LITTLE(&config->reorderTimeout, streamConfig->reorderTimeout);
    ENCODE2BYTE_LITTLE(&config->crcInit, streamConfig->crcInit);
    return CM_SUCCESS;
}

static uint32_t CM_SignalingTranModeReliableConfigPack(CM_SignalingTransModeReliableConfig_S *reliableConfig,
                                                       uint8_t *data)
{
    CM_TransModeReliableConfig_S *config = (CM_TransModeReliableConfig_S *)data;
    config->txWindow = reliableConfig->txWindow;
    config->maxTxThreshold = reliableConfig->maxTxThreshold;
    ENCODE2BYTE_LITTLE(&config->retransTimeout, reliableConfig->retransTimeout);
    ENCODE2BYTE_LITTLE(&config->rspTimeout, reliableConfig->rspTimeout);
    ENCODE2BYTE_LITTLE(&config->reorderTimeout, reliableConfig->reorderTimeout);
    ENCODE2BYTE_LITTLE(&config->crcInit, reliableConfig->crcInit);

    return CM_SUCCESS;
}

static uint32_t CM_SignalingTranModeConfigPack(CM_SignalingTransModeConfig_S *transModeConfig, uint8_t *data,
                                               uint8_t *offset)
{
    CM_CHECK_RETURN_RET(transModeConfig->commonConfig.mode < CM_TRANS_MODE_MAX, CM_INVALID_PARAM_ERR,
                        "invalid transport mode: %hhu", transModeConfig->commonConfig.mode);
    uint32_t ret = CM_SUCCESS;

    CM_TransModeCommonConfig_S *commonConfig = (CM_TransModeCommonConfig_S *)data;
    commonConfig->transMode = transModeConfig->commonConfig.mode;
    ENCODE2BYTE_LITTLE(&commonConfig->mtu, transModeConfig->commonConfig.mtu);
    ENCODE2BYTE_LITTLE(&commonConfig->mps, transModeConfig->commonConfig.mps);
    *offset = (uint8_t)sizeof(CM_TransModeCommonConfig_S);

    if (transModeConfig->commonConfig.mode == CM_TRANS_MODE_STREAM) {
        ret = CM_SignalingTranModeStreamConfigPack(&transModeConfig->streamConfig, data + *offset);
        *offset += ((uint8_t)sizeof(CM_TransModeStreamConfig_S));
    } else if (transModeConfig->commonConfig.mode == CM_TRANS_MODE_RELIABLE) {
        ret = CM_SignalingTranModeReliableConfigPack(&transModeConfig->reliableConfig, data + *offset);
        *offset += ((uint8_t)sizeof(CM_TransModeReliableConfig_S));
    } else {
        //  do nothing
    }

    return ret;
}

static uint32_t CM_SignalingPreferredSlqiListPack(CM_SignalingPreferredSlqiList_S *preferredSlqiList, uint8_t *data,
                                                  uint8_t *offset)
{
    CM_CHECK_RETURN_RET(preferredSlqiList->slqiNum == CM_SLQI_SURPPORT_NUM, CM_INVALID_PARAM_ERR,
                        "invalid slqi num: %hu", preferredSlqiList->slqiNum);

    CM_PreferredSlqiList_S *slqiList = (CM_PreferredSlqiList_S *)data;
    slqiList->slqiNum = preferredSlqiList->slqiNum;
    slqiList->slqi[0] = preferredSlqiList->slqi[0];
    *offset = (uint8_t)CM_SLQI_SIZE;
    return CM_SUCCESS;
}

static uint32_t CM_SignalingExtensionPack(CM_SignalingTransChanEstablishExtension_S *extension, uint8_t *data,
                                          uint8_t *offset)
{
    // 当前仅支持portConfig
    CM_CHECK_RETURN_RET(extension->portConfig != NULL, CM_NULL_POINTER, "portConfig is null");

    CM_TransChanEstablishReqExt_S *ext = (CM_TransChanEstablishReqExt_S *)data;
    ext->pc = 1;  // enable port config
    ext->lc = 0;
    ext->mc = 0;

    CM_PortConfig_S *config = (CM_PortConfig_S *)(data + sizeof(CM_TransChanEstablishReqExt_S));
    ENCODE2BYTE_LITTLE(&config->srcPort, extension->portConfig->srcPort);
    ENCODE2BYTE_LITTLE(&config->dstPort, extension->portConfig->dstPort);
    ENCODE2BYTE_LITTLE(&config->aid, extension->portConfig->aid);
    *offset = (uint8_t)(sizeof(CM_TransChanEstablishReqExt_S) + sizeof(CM_PortConfig_S));
    return CM_SUCCESS;
}

static uint16_t CM_SignalingEstablishReqSizeGet(const CM_SignalingTransChanEstablishReq_S *req)
{
    size_t dataLen = sizeof(CM_TransChanEstablishReqPkt_S);

    dataLen += sizeof(CM_TransModeCommonConfig_S);
    switch (req->transModeConfig.commonConfig.mode) {
        case CM_TRANS_MODE_STREAM:
            dataLen += sizeof(CM_TransModeStreamConfig_S);
            break;
        case CM_TRANS_MODE_RELIABLE:
            dataLen += sizeof(CM_TransModeReliableConfig_S);
            break;
        default:
            break;
    }

    // 当前仅支持一条星闪QoS索引
    dataLen += CM_SLQI_SIZE;

    // 当前仅支持端口配置信息
    if (req->extension.portConfig != NULL) {
        dataLen += (sizeof(CM_TransChanEstablishReqExt_S) + sizeof(CM_PortConfig_S));
    }

    return (uint16_t)dataLen;
}

static uint8_t *CM_SignalingTransChanEstablishReqPack(CM_SignalingTransChanEstablishReq_S *req, uint16_t *reqLen)
{
    CM_CHECK_RETURN_RET(req != NULL, NULL, "req is null");

    uint16_t dataLen = CM_SignalingEstablishReqSizeGet(req);
    uint8_t *data = SDF_MemZalloc(dataLen);
    CM_CHECK_RETURN_RET(data != NULL, NULL, "malloc failed");

    CM_TransChanEstablishReqPkt_S *pkt = (CM_TransChanEstablishReqPkt_S *)data;
    pkt->srcTcid = req->srcTcid;
    pkt->exclusive = req->exclusive;
    pkt->measure = req->measure;

    uint8_t offset = 0;
    uint8_t totalOff = (uint8_t)sizeof(CM_TransChanEstablishReqPkt_S);
    uint32_t ret = CM_SignalingTranModeConfigPack(&req->transModeConfig, data + totalOff, &offset);
    if (ret != CM_SUCCESS) {
        SDF_MemFree(data);
        return NULL;
    }
    totalOff += offset;

    ret = CM_SignalingPreferredSlqiListPack(&req->slqiList, data + totalOff, &offset);
    if (ret != CM_SUCCESS) {
        SDF_MemFree(data);
        return NULL;
    }
    totalOff += offset;

    ret = CM_SignalingExtensionPack(&req->extension, data + totalOff, &offset);
    if (ret != CM_SUCCESS) {
        SDF_MemFree(data);
        return NULL;
    }
    pkt->optionOffset = totalOff;
    *reqLen = dataLen;
    return data;
}

uint32_t CM_SignalingTransChanEstablishReqSend(uint16_t lcid, CM_SignalingTransChanEstablishReq_S *req)
{
    uint16_t reqDataLen = 0;
    uint8_t *reqData = CM_SignalingTransChanEstablishReqPack(req, &reqDataLen);
    CM_CHECK_RETURN_RET(reqData != NULL, CM_INVALID_PARAM_ERR, "invalid request parameters");

    CM_SignalingTransChanTimoutArgs_S *args = CM_SignalingTransChanTimoutArgsNew(lcid, req->srcTcid, 0);
    if (args == NULL) {
        CM_LOGE("new timeout args failed");
        SDF_MemFree(reqData);
        return CM_MEM_ERR;
    }

    uint8_t id = CM_GetIdentifier();
    uint32_t ret = CM_SignalingCacheInsert(lcid, id, TC_CONNECT_REQ, args, CM_SignalingTransChanEstablishReqTimeout);
    if (ret != CM_SUCCESS) {
        SDF_MemFree(reqData);
        SDF_MemFree(args);
        return ret;
    }

    SDF_Buff_S *buff = CM_CreateSignalingBuff(TC_CONNECT_REQ, id, (uint8_t *)reqData, reqDataLen);
    SDF_MemFree(reqData);
    if (buff == NULL) {
        CM_LOGE("create buff failed");
        CM_SignalingCacheRemove(id, TC_CONNECT_REQ);  // 移除map中的元素也会释放args
        return CM_MEM_ERR;
    }

    ret = CM_SendBuffToDtap(lcid, buff);
    if (ret != CM_SUCCESS) {
        CM_LOGE("send buff to dtap failed");
        CM_SignalingCacheRemove(id, TC_CONNECT_REQ);  // 移除map中的元素也会释放args
        SDF_BuffFree(buff);
        return ret;
    }

    return CM_SUCCESS;
}

uint32_t CM_SignalingTransChanEstablishRspSend(uint16_t lcid, uint8_t reqId, CM_SignalingTransChanEstablishRsp_S *rsp)
{
    CM_CHECK_RETURN_RET(rsp != NULL, CM_NULL_POINTER, "rsp is null");

    CM_TransChanEstablishRspPkt_S rspPkt = { 0 };
    rspPkt.srcTcid = rsp->srcTcid;
    rspPkt.dstTcid = rsp->dstTcid;
    rspPkt.result = rsp->result;
    rspPkt.lc = 0;

    SDF_Buff_S *buff =
        CM_CreateSignalingBuff(TC_CONNECT_RSP, reqId, (uint8_t *)&rspPkt, sizeof(CM_TransChanEstablishRspPkt_S));
    CM_CHECK_RETURN_RET(buff != NULL, CM_MEM_ERR, "create buff failed");
    uint32_t ret = CM_SendBuffToDtap(lcid, buff);
    if (ret != 0) {
        SDF_BuffFree(buff);
        return ret;
    }
    return CM_SUCCESS;
}

static void CM_SignalingTransChanReleaseReqTimeout(void *args)
{
    if (args == NULL) {
        return;
    }

    CM_SignalingTransChanTimoutArgs_S *timeoutArgs = (CM_SignalingTransChanTimoutArgs_S *)args;
    CM_LOGW("transport channel release timeout, lcid: %hu, src tcid: %hhu, dst tcid: %hhu", timeoutArgs->lcid,
            timeoutArgs->srcTcid, timeoutArgs->dstTcid);

    CM_SignalingTransChanReleaseRsp_S rsp = {
        .srcTcid = timeoutArgs->srcTcid,
        .dstTcid = timeoutArgs->dstTcid,
        .result = CM_RESULT_RELEASE_TIMEOUT,
    };

    if (g_signalingTransChanCbks.releaseRspCbk != NULL) {
        g_signalingTransChanCbks.releaseRspCbk(timeoutArgs->lcid, &rsp);
    }
}

uint32_t CM_SignalingTransChanReleaseReqSend(uint16_t lcid, CM_SignalingTransChanReleaseReq_S *req)
{
    CM_CHECK_RETURN_RET(req != NULL, CM_NULL_POINTER, "req is null");
    CM_TransChanReleaseReqPkt_S reqPkt = { 0 };
    reqPkt.srcTcid = req->srcTcid;
    reqPkt.dstTcid = req->dstTcid;

    CM_SignalingTransChanTimoutArgs_S *args = CM_SignalingTransChanTimoutArgsNew(lcid, req->srcTcid, req->dstTcid);
    CM_CHECK_RETURN_RET(args != NULL, CM_MEM_ERR, "new timeout args failed");

    uint8_t id = CM_GetIdentifier();
    uint32_t ret = CM_SignalingCacheInsert(lcid, id, TC_DISCONNECT_REQ, args, CM_SignalingTransChanReleaseReqTimeout);
    if (ret != CM_SUCCESS) {
        SDF_MemFree(args);
        return ret;
    }

    SDF_Buff_S *buff =
        CM_CreateSignalingBuff(TC_DISCONNECT_REQ, id, (uint8_t *)&reqPkt, sizeof(CM_TransChanReleaseReqPkt_S));
    if (buff == NULL) {
        CM_LOGE("create buff failed");
        CM_SignalingCacheRemove(id, TC_DISCONNECT_REQ);  // 移除map中的元素也会释放args
        return CM_MEM_ERR;
    }

    ret = CM_SendBuffToDtap(lcid, buff);
    if (ret != CM_SUCCESS) {
        CM_SignalingCacheRemove(id, TC_DISCONNECT_REQ);  // 移除map中的元素也会释放args
        SDF_BuffFree(buff);
        return ret;
    }

    return CM_SUCCESS;
}

uint32_t CM_SignalingTransChanReleaseRspSend(uint16_t lcid, uint8_t reqId, CM_SignalingTransChanReleaseRsp_S *rsp)
{
    CM_CHECK_RETURN_RET(rsp != NULL, CM_NULL_POINTER, "rsp is null");

    CM_TransChanReleaseRspPkt_S rspPkt = { 0 };
    rspPkt.srcTcid = rsp->srcTcid;
    rspPkt.dstTcid = rsp->dstTcid;
    rspPkt.result = rsp->result;

    SDF_Buff_S *buff =
        CM_CreateSignalingBuff(TC_DISCONNECT_RSP, reqId, (uint8_t *)&rspPkt, sizeof(CM_TransChanReleaseRspPkt_S));
    CM_CHECK_RETURN_RET(buff != NULL, CM_MEM_ERR, "create buff failed");
    uint32_t ret = CM_SendBuffToDtap(lcid, buff);
    if (ret != 0) {
        SDF_BuffFree(buff);
        return ret;
    }
    return CM_SUCCESS;
}

static uint32_t CM_SignalingTransModeStreamConfigParse(CM_SignalingTransModeStreamConfig_S *streamConfig,
    uint8_t *data, uint16_t dataLen)
{
    CM_CHECK_RETURN_RET(dataLen >= sizeof(CM_TransModeStreamConfig_S),
        CM_INVALID_PARAM_ERR, "invalid packet length: %hu", dataLen);
    CM_TransModeStreamConfig_S *config = (CM_TransModeStreamConfig_S *)data;
    streamConfig->flushTimeout = DECODE2BYTE_LITTLE((uint8_t *)&config->flushTimeout);
    streamConfig->reorderTimeout = DECODE2BYTE_LITTLE((uint8_t *)&config->reorderTimeout);
    streamConfig->crcInit = DECODE2BYTE_LITTLE((uint8_t *)&config->crcInit);
    return CM_SUCCESS;
}

static uint32_t CM_SignalingTransModeReliableConfigParse(CM_SignalingTransModeReliableConfig_S *config,
                                                         uint8_t *data, uint16_t dataLen)
{
    CM_CHECK_RETURN_RET(dataLen >= sizeof(CM_TransModeReliableConfig_S),
        CM_INVALID_PARAM_ERR, "invalid packet length: %hu", dataLen);
    CM_TransModeReliableConfig_S *reliableConfig = (CM_TransModeReliableConfig_S *)data;

    config->txWindow = reliableConfig->txWindow;
    config->maxTxThreshold = reliableConfig->maxTxThreshold;
    config->retransTimeout = DECODE2BYTE_LITTLE((uint8_t *)&reliableConfig->retransTimeout);
    config->rspTimeout = DECODE2BYTE_LITTLE((uint8_t *)&reliableConfig->rspTimeout);
    config->reorderTimeout = DECODE2BYTE_LITTLE((uint8_t *)&reliableConfig->reorderTimeout);
    config->crcInit = DECODE2BYTE_LITTLE((uint8_t *)&reliableConfig->crcInit);

    return CM_SUCCESS;
}

static uint32_t CM_SignalingTransModeConfigParse(CM_SignalingTransModeConfig_S *transModeConfig, uint8_t *data,
                                                 uint16_t dataLen, uint8_t *offset)
{
    CM_CHECK_RETURN_RET(dataLen >= sizeof(CM_TransModeCommonConfig_S), CM_INVALID_PARAM_ERR,
                        "invalid packet length: %hu", dataLen);
    CM_TransModeCommonConfig_S *commonConfig = (CM_TransModeCommonConfig_S *)data;
    CM_CHECK_RETURN_RET(commonConfig->transMode < CM_TRANS_MODE_MAX &&
                        commonConfig->transMode != CM_TRANS_MODE_TRANSPARENT, CM_INVALID_PARAM_ERR,
                        "unsupported trans mode: %hu", commonConfig->transMode);
    transModeConfig->commonConfig.mode = commonConfig->transMode;
    transModeConfig->commonConfig.mtu = DECODE2BYTE_LITTLE((uint8_t *)&commonConfig->mtu);
    transModeConfig->commonConfig.mps = DECODE2BYTE_LITTLE((uint8_t *)&commonConfig->mps);
    *offset = (uint8_t)sizeof(CM_TransModeCommonConfig_S);
    uint32_t ret = CM_SUCCESS;
    if (transModeConfig->commonConfig.mode == CM_TRANS_MODE_STREAM) {
        ret = CM_SignalingTransModeStreamConfigParse(&transModeConfig->streamConfig,
            data + *offset, dataLen - *offset);
        *offset += ((uint8_t)sizeof(CM_TransModeStreamConfig_S));
    } else if (transModeConfig->commonConfig.mode == CM_TRANS_MODE_RELIABLE) {
        ret = CM_SignalingTransModeReliableConfigParse(&transModeConfig->reliableConfig,
            data + *offset, dataLen - *offset);
        *offset += ((uint8_t)sizeof(CM_TransModeReliableConfig_S));
    } else {
        // do nothing
    }

    return ret;
}

static uint32_t CM_SignalingPreferredSlqiListParse(CM_SignalingPreferredSlqiList_S *slqiList, uint8_t *data,
                                                   uint16_t dataLen, uint8_t *offset)
{
    CM_CHECK_RETURN_RET(dataLen >= CM_SLQI_SIZE, CM_INVALID_PARAM_ERR, "invalid packet length: %hu", dataLen);
    CM_PreferredSlqiList_S *slqiListData = (CM_PreferredSlqiList_S *)data;
    CM_CHECK_RETURN_RET(slqiListData->slqiNum == CM_SLQI_SURPPORT_NUM, CM_INVALID_PARAM_ERR, "invalid slqi num: %hu",
                        slqiListData->slqiNum);
    slqiList->slqiNum = slqiListData->slqiNum;
    slqiList->slqi[0] = slqiListData->slqi[0];
    *offset = (uint8_t)CM_SLQI_SIZE;
    return CM_SUCCESS;
}

static uint32_t CM_SignalingExtensionParse(CM_SignalingTransChanEstablishExtension_S *extension, uint8_t *data,
                                           uint16_t dataLen, uint8_t *offset)
{
    static CM_SignalingPortConfig_S portConfig = { 0 };
    CM_CHECK_RETURN_RET(dataLen >= sizeof(CM_TransChanEstablishReqExt_S), CM_INVALID_PARAM_ERR,
                        "invalid packet length: %hu", dataLen);
    CM_TransChanEstablishReqExt_S *extensionData = (CM_TransChanEstablishReqExt_S *)data;
    if (extensionData->pc != 0) {
        CM_CHECK_RETURN_RET(dataLen >= sizeof(CM_TransChanEstablishReqExt_S) + sizeof(CM_PortConfig_S),
                            CM_INVALID_PARAM_ERR, "invalid packet length: %hu", dataLen);
        CM_PortConfig_S *portConfigData = (CM_PortConfig_S *)(extensionData + 1);
        extension->portConfig = &portConfig;
        extension->portConfig->srcPort = DECODE2BYTE_LITTLE((uint8_t *)&portConfigData->srcPort);
        extension->portConfig->dstPort = DECODE2BYTE_LITTLE((uint8_t *)&portConfigData->dstPort);
        extension->portConfig->aid = DECODE2BYTE_LITTLE((uint8_t *)&portConfigData->aid);
        *offset = (uint8_t)(sizeof(CM_TransChanEstablishReqExt_S) + sizeof(CM_PortConfig_S));
    } else {
        *offset = 0;
    }

    return CM_SUCCESS;
}

static uint32_t CM_SignalingTransChanEstablishReqParse(CM_SignalingHead_S *pkt,
                                                       CM_SignalingTransChanEstablishReq_S *req)
{
    CM_CHECK_RETURN_RET(pkt != NULL && pkt->data != NULL, CM_INVALID_PARAM_ERR, "pkt or pkt data is NULL");
    CM_CHECK_RETURN_RET(pkt->length >= sizeof(CM_TransChanEstablishReqPkt_S), CM_INVALID_PARAM_ERR,
                        "invalid packet length: %hu", pkt->length);

    CM_TransChanEstablishReqPkt_S *data = (CM_TransChanEstablishReqPkt_S *)pkt->data;
    req->srcTcid = data->srcTcid;
    req->exclusive = data->exclusive;
    req->measure = data->measure;

    uint8_t totalOffset = (uint8_t)sizeof(CM_TransChanEstablishReqPkt_S);
    uint8_t offset = 0;
    uint32_t ret = CM_SignalingTransModeConfigParse(&req->transModeConfig, pkt->data + totalOffset,
                                                    pkt->length - totalOffset, &offset);
    if (ret != CM_SUCCESS) {
        return ret;
    }
    totalOffset += offset;

    ret = CM_SignalingPreferredSlqiListParse(&req->slqiList, pkt->data + totalOffset,
        pkt->length - totalOffset, &offset);
    if (ret != CM_SUCCESS) {
        return ret;
    }
    totalOffset += offset;

    if (data->optionOffset != 0) {
        CM_CHECK_RETURN_RET(data->optionOffset == totalOffset, CM_INVALID_PARAM_ERR,
                            "option offset: %hhu is not equal to actual offset: %hhu",
                            data->optionOffset, totalOffset);
        ret = CM_SignalingExtensionParse(&req->extension, pkt->data + totalOffset, pkt->length - totalOffset, &offset);
        if (ret != CM_SUCCESS) {
            return ret;
        }
    }
    return CM_SUCCESS;
}

uint32_t CM_SignalingTransChanEstablishReqProc(uint16_t lcid, CM_SignalingHead_S *pkt)
{
    uint32_t ret;
    CM_SignalingTransChanEstablishReq_S req = { 0 };

    ret = CM_SignalingTransChanEstablishReqParse(pkt, &req);
    if (ret != CM_SUCCESS) {
        return ret;
    }

    CM_LOGD("srcTcid: %hhu, transMode: %hhu, slqiNum: %hhu", req.srcTcid, req.transModeConfig.commonConfig.mode,
            req.slqiList.slqiNum);
    if (g_signalingTransChanCbks.establishReqCbk != NULL) {
        g_signalingTransChanCbks.establishReqCbk(lcid, pkt->identifier, &req);
    }
    return CM_SUCCESS;
}

uint32_t CM_SignalingTransChanEstablishRspProc(uint16_t lcid, CM_SignalingHead_S *pkt)
{
    CM_CHECK_RETURN_RET(pkt != NULL && pkt->data != NULL, CM_INVALID_PARAM_ERR, "pkt or pkt data is NULL");
    CM_CHECK_RETURN_RET(pkt->length >= sizeof(CM_TransChanEstablishRspPkt_S),
                        CM_INVALID_PARAM_ERR, "invalid packet length: %hu", pkt->length);

    CM_TransChanEstablishRspPkt_S *data = (CM_TransChanEstablishRspPkt_S *)pkt->data;
    CM_SignalingTransChanEstablishRsp_S rsp = { 0 };
    rsp.srcTcid = data->srcTcid;
    rsp.dstTcid = data->dstTcid;
    rsp.result = data->result;

    CM_LOGD("srcTcid: %hhu, dstTcid: %hhu, result: %hhu", rsp.srcTcid, rsp.dstTcid, rsp.result);
    if (g_signalingTransChanCbks.establishRspCbk != NULL) {
        g_signalingTransChanCbks.establishRspCbk(lcid, &rsp);
    }
    return CM_SUCCESS;
}

uint32_t CM_SignalingTransChanReleaseReqProc(uint16_t lcid, CM_SignalingHead_S *pkt)
{
    CM_CHECK_RETURN_RET(pkt != NULL && pkt->data != NULL, CM_INVALID_PARAM_ERR, "pkt or pkt data is NULL");
    CM_CHECK_RETURN_RET(pkt->length >= sizeof(CM_TransChanReleaseReqPkt_S),
                        CM_INVALID_PARAM_ERR, "invalid packet length: %hu", pkt->length);

    CM_TransChanReleaseReqPkt_S *data = (CM_TransChanReleaseReqPkt_S *)pkt->data;
    CM_SignalingTransChanReleaseReq_S req = { 0 };
    req.srcTcid = data->srcTcid;
    req.dstTcid = data->dstTcid;

    CM_LOGD("srcTcid: %hhu, dstTcid: %hhu", req.srcTcid, req.dstTcid);
    if (g_signalingTransChanCbks.releaseReqCbk != NULL) {
        g_signalingTransChanCbks.releaseReqCbk(lcid, pkt->identifier, &req);
    }
    return CM_SUCCESS;
}

uint32_t CM_SignalingTransChanReleaseRspProc(uint16_t lcid, CM_SignalingHead_S *pkt)
{
    CM_CHECK_RETURN_RET(pkt != NULL && pkt->data != NULL, CM_INVALID_PARAM_ERR, "pkt or pkt data is NULL");
    CM_CHECK_RETURN_RET(pkt->length >= sizeof(CM_TransChanReleaseRspPkt_S),
                        CM_INVALID_PARAM_ERR, "invalid packet length: %hu", pkt->length);

    CM_TransChanReleaseRspPkt_S *data = (CM_TransChanReleaseRspPkt_S *)pkt->data;
    CM_SignalingTransChanReleaseRsp_S rsp = { 0 };
    rsp.srcTcid = data->srcTcid;
    rsp.dstTcid = data->dstTcid;
    rsp.result = data->result;

    CM_LOGD("srcTcid: %hhu, dstTcid: %hhu, result: %hhu", rsp.srcTcid, rsp.dstTcid, rsp.result);
    if (g_signalingTransChanCbks.releaseRspCbk != NULL) {
        g_signalingTransChanCbks.releaseRspCbk(lcid, &rsp);
    }
    return CM_SUCCESS;
}