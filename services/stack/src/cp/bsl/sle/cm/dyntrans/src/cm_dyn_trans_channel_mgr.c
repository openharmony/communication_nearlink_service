/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "cm_dyn_trans_channel_api.h"
#include "securec.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "cm_dyn_trans_channel_mgr.h"
#include "cm_trans_channel_api.h"
#include "cm_trans_channel_mgr.h"
#include "cm_dyn_trans_chan_state_mgr.h"
#include "cm_logic_link_api.h"
#include "cm_dyn_tcid.h"
#include "cm_signaling_trans_channel.h"
#include "sle_logic_link_mgr.h"

static volatile bool g_cmIsRegistered = false;

static uint8_t CM_DynTransChanGetTransportMode(uint8_t tcid)
{
    if (tcid >= CM_TCID_UC_BEGIN && tcid <= CM_TCID_UC_END) {
        return CM_ACCESS_TRANS_MODE_UNICAST;
    }
    CM_LOGE("tcid:0x%02x mapping transport mode is not support", tcid);
    return CM_ACCESS_TRANS_MODE_MAX;
}

/**
 * @brief 创建动态传输通道
 */
static uint32_t CM_DynTransChannelCreate(const CM_DynTransChannelEstablishParamReq_S *reqParam)
{
    SleLogicLink_S *logicLink = SleLogicLinkGetByAddr(&reqParam->addr);
    CM_CHECK_RETURN_RET(logicLink != NULL, CM_FAIL, "get logic link by addr failed");
    uint16_t lcid = logicLink->lcid;
    CM_LOGD("cm dyn trans channel create lcid:0x%04x, addr:%s", lcid, GetEncryptAddr(&reqParam->addr).buf);
    CM_DynTransChanParam_S loopParam = { 0 };
    loopParam.version = reqParam->version;
    loopParam.localIndex = reqParam->localIndex;
    loopParam.lcid = logicLink->lcid;
    loopParam.srcPort = reqParam->srcPort;
    loopParam.dstPort = reqParam->dstPort;
    loopParam.localTcid = CM_TRANS_INVALID_TCID;
    loopParam.remoteTcid = CM_TRANS_INVALID_TCID;
    loopParam.aid = CM_TRANS_INVALID_AID;
    loopParam.exclusive = true;
    loopParam.slqi = reqParam->slqi;
    loopParam.expectedTransportMode = reqParam->expectedTransportMode;
    loopParam.state = CM_TRANS_CHANNEL_STATE_INIT;
    loopParam.frameType = reqParam->frameType;
    (void)memcpy_s(&loopParam.transModeConfig, sizeof(CM_TransModeConfig_S),
        &reqParam->transModeConfig, sizeof(CM_TransModeConfig_S));
    uint32_t ret = CM_DynTransChanPostEventAndHandler(CM_DYN_TRANS_CHAN_EVENT_ACTIVE_ESTABLISHING_REQ, &loopParam);
    CM_CHECK_RETURN_RET((ret == CM_SUCCESS), CM_FAIL, "post event and handler failed, ret=0x%08x", ret);
    return ret;
}

/*
 * 接收传输通道被动请求建立回调
 */
static void CM_DynTransChannelEstablishReqCbk(uint16_t lcid, uint8_t reqId, CM_SignalingTransChanEstablishReq_S *req)
{
    CM_CHECK_RETURN(req != NULL, "req is null");

    CM_LOGI("CM_DynTransChannelEstablishReqCbk, lcid:0x%04x, reqId:0x%02x, srcTcid:0x%02x", lcid, reqId, req->srcTcid);
    CM_CHECK_RETURN(CM_TRANS_CHANNEL_IS_DYNAMIC(req->srcTcid), "srcTcid is not defined in dynamic range");
    SleTransLcid_S *trans = CM_FindTransChannelByRemoteTcid(lcid, req->srcTcid);
    CM_CHECK_RETURN(trans == NULL, "dyn trans channel has established, lcid:%hu, srcTcid:%hhu", lcid, req->srcTcid);
    CM_DynTransChanParam_S loopParam = { 0 };
    loopParam.lcid = lcid;
    loopParam.remoteTcid = req->srcTcid; // 收到被动建立请求，srcTCID即为本端设备的dstTCID, 释放时，使用srcTCID去释放本端设备
    loopParam.result = CM_RESULT_ESTABLISH_SUCCESS; // 被动请求建立时，这个状态默认为成功
    loopParam.expectedTransportMode = CM_DynTransChanGetTransportMode(loopParam.remoteTcid);
    loopParam.reqId = reqId;
    loopParam.slqi = req->slqiList.slqi[0];
    if (req->extension.portConfig != NULL) { // 协议定义，可空
        loopParam.srcPort = req->extension.portConfig->dstPort; // 收到被动建立请求，dstPort即为本端设备的srcPort
        loopParam.dstPort = req->extension.portConfig->srcPort; // 收到被动建立请求，srcPort即为本端设备的dstPort
        loopParam.aid = req->extension.portConfig->aid;
    }
    if (memcpy_s(&loopParam.transModeConfig, sizeof(CM_TransModeConfig_S),
        &req->transModeConfig, sizeof(CM_SignalingTransModeConfig_S)) != EOK) {
        CM_LOGE("copy trans mode config failed");
        return;
    }
    uint32_t ret = CM_DynTransChanPostEventAndHandler(CM_DYN_TRANS_CHAN_EVENT_PASSIVE_ESTABLISHED_REQ, &loopParam);
    CM_CHECK_RETURN((ret == CM_SUCCESS), "post event and handler failed, ret=0x%08x", ret);
}

static void CM_DynTransChannelEstablishRspCbk(uint16_t lcid, CM_SignalingTransChanEstablishRsp_S *rsp)
{
    CM_CHECK_RETURN(rsp != NULL, "rsp is null");

    CM_LOGI("CM_DynTransChannelEstablishRspCbk, lcid:0x%04x, result:0x%02x, srcTcid:0x%02x, dstTcid:0x%02x",
        lcid, rsp->result, rsp->srcTcid, rsp->dstTcid);
    CM_CHECK_RETURN(CM_TRANS_CHANNEL_IS_DYNAMIC(rsp->srcTcid), "srcTcid is not defined in dynamic range");
    if (rsp->result == CM_RESULT_ESTABLISH_SUCCESS) {
        CM_CHECK_RETURN(CM_TRANS_CHANNEL_IS_DYNAMIC(rsp->dstTcid), "dstTcid is not defined in dynamic range");
    }
    CM_DynTransChanParam_S loopParam = { 0 };
    loopParam.lcid = lcid;
    loopParam.localTcid = rsp->srcTcid;
    loopParam.remoteTcid = rsp->dstTcid; // 响应超时时，dstTcid为0
    loopParam.result = rsp->result;
    loopParam.expectedTransportMode = CM_DynTransChanGetTransportMode(loopParam.localTcid);
    SleTransLcid_S *trans = CM_FindTransChannelByLocalTcid(loopParam.lcid, loopParam.localTcid);
    CM_CHECK_RETURN((trans != NULL) && (trans->state == CM_TRANS_CHANNEL_STATE_ACTIVATING),
        "dyn trans channel has not activing, lcid:0x%04x, localTcid:0x%02x, remoteTcid:0x%02x",
        lcid, loopParam.localTcid, loopParam.remoteTcid);
    uint32_t ret = CM_SUCCESS;
    if (loopParam.result == CM_RESULT_ESTABLISH_SUCCESS) {
        ret = CM_DynTransChanPostEventAndHandler(CM_DYN_TRANS_CHAN_EVENT_ACTIVE_ESTABLISHED_RSP, &loopParam);
    } else {
        ret = CM_DynTransChanPostEventAndHandler(CM_DYN_TRANS_CHAN_EVENT_ACTIVE_ESTABLISHING_FAIL_RSP, &loopParam);
    }
    CM_CHECK_RETURN((ret == CM_SUCCESS), "post event and handler failed, ret=0x%08x", ret);
}

static void CM_DynTransChannelReleaseReqCbk(uint16_t lcid, uint8_t reqId, CM_SignalingTransChanReleaseReq_S *req)
{
    CM_CHECK_RETURN(req != NULL, "req is null");

    CM_LOGI("CM_DynTransChannelReleaseReqCbk, lcid:0x%04x, reqId:0x%02x, srcTcid:0x%02x, dstTcid:0x%02x",
        lcid, reqId, req->srcTcid, req->dstTcid);
    CM_CHECK_RETURN(CM_TRANS_CHANNEL_IS_DYNAMIC(req->srcTcid), "srcTcid is not defined in dynamic range");
    CM_DynTransChanParam_S loopParam = { 0 };
    loopParam.lcid = lcid;
    loopParam.remoteTcid = req->srcTcid; // 收到被动断开请求，srcTCID即为本端设备的remoteTcid
    loopParam.localTcid = req->dstTcid;  // 收到被动断开请求，dstTCID即为本端设备的localTcid
    loopParam.result = CM_RESULT_ESTABLISH_SUCCESS; // 被动请求建立时，这个状态默认为成功
    loopParam.expectedTransportMode = CM_DynTransChanGetTransportMode(loopParam.remoteTcid);
    loopParam.reqId = reqId;
    uint32_t ret = CM_DynTransChanPostEventAndHandler(CM_DYN_TRANS_CHAN_EVENT_PASSIVE_RELEASED_REQ, &loopParam);
    CM_CHECK_RETURN((ret == CM_SUCCESS), "post event and handler failed, ret=0x%08x", ret);
}

static void CM_DynTransChannelReleaseRspCbk(uint16_t lcid, CM_SignalingTransChanReleaseRsp_S *rsp)
{
    CM_CHECK_RETURN(rsp != NULL, "rsp is null");

    CM_LOGI("CM_DynTransChannelReleaseRspCbk, lcid:0x%04x, srcTcid:0x%02x, result:0x%02x",
        lcid, rsp->srcTcid, rsp->result);
    CM_CHECK_RETURN(CM_TRANS_CHANNEL_IS_DYNAMIC(rsp->srcTcid), "srcTcid is not defined in dynamic range");
    CM_DynTransChanParam_S loopParam = { 0 };
    loopParam.lcid = lcid;
    loopParam.result = rsp->result;
    loopParam.localTcid = rsp->srcTcid;
    loopParam.remoteTcid = rsp->dstTcid;  // 响应超时时，dstTcid为0
    loopParam.expectedTransportMode = CM_DynTransChanGetTransportMode(loopParam.localTcid);
    uint32_t ret = CM_SUCCESS;
    if (rsp->result == CM_RESULT_RELEASE_SUCCESS) {
        ret = CM_DynTransChanPostEventAndHandler(CM_DYN_TRANS_CHAN_EVENT_ACTIVE_RELEASED_RSP, &loopParam);
    } else {
        ret = CM_DynTransChanPostEventAndHandler(CM_DYN_TRANS_CHAN_EVENT_ACTIVE_RELEASING_FAIL_RSP, &loopParam);
    }
    CM_CHECK_RETURN((ret == CM_SUCCESS), "post event and handler failed, ret=0x%08x", ret);
}

static void CM_DynTransLogicLinkCbk(CM_LogicLinkState_S *state)
{
    CM_CHECK_RETURN(state != NULL, "param is null");
    uint32_t ret = CM_SUCCESS;
    uint16_t lcid = state->lcid;
    if (state->result == CM_LINK_STATE_CONNECTED) {
        CM_LOGI("CM_DynTransLogicLinkCbk, lcid:0x%04x, state:CONNECTED", lcid);
        ret = CM_DynTcidActivatePool(lcid);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "activate dyn trans channel failed, lcid:0x%04x", lcid);
    } else if (state->result == CM_LINK_STATE_DISCONNECTED) {
        CM_LOGI("CM_DynTransLogicLinkCbk, lcid:0x%04x, state:DISCONNECTED", lcid);
        CM_DynTransChannReleaseAllNode(lcid);
        ret = CM_DynTcidDestroyPool(lcid);
        CM_CHECK_RETURN(ret == CM_SUCCESS, "destroy dyn trans channel failed, lcid:0x%04x", lcid);
    }
}

uint32_t CM_RegDynTransChannelCbks(const CM_DynTransChannelCbks_S *cbks)
{
    CM_CHECK_RETURN_RET(!g_cmIsRegistered, CM_FAIL, "has register trans channel cbks");
    CM_CHECK_RETURN_RET((cbks != NULL) && (cbks->establishRspCbk != NULL) && (cbks->releaseRspCbk != NULL) &&
        (cbks->statusIndicationCbk != NULL), CM_INVALID_PARAM_ERR,
        "has not register dyn trans channel cbks");

    CM_DynTransChannStateMgrRegCbks(cbks);
    CM_LogicLinkCbks_S cmDynTransCbks = { 0 };
    cmDynTransCbks.moduleId = CM_MODULE_CM_DYNTRANS;
    cmDynTransCbks.logicLinkCbk = CM_DynTransLogicLinkCbk;
    uint32_t ret = CM_RegLogicLinkListener(&cmDynTransCbks);
    CM_CHECK_RETURN_RET(ret == CM_SUCCESS, ret, "CM_RegLogicLinkListener failed, ret=0x%08x", ret);

    CM_SignalingTransChanCbks_S signalingTransCbks = { 0 };
    signalingTransCbks.establishReqCbk = CM_DynTransChannelEstablishReqCbk;
    signalingTransCbks.establishRspCbk = CM_DynTransChannelEstablishRspCbk;
    signalingTransCbks.releaseReqCbk = CM_DynTransChannelReleaseReqCbk;
    signalingTransCbks.releaseRspCbk = CM_DynTransChannelReleaseRspCbk;
    ret = CM_SignalingTransChanCbksRegister(&signalingTransCbks);
    CM_CHECK_RETURN_RET((ret == CM_SUCCESS), CM_FAIL, "register signaling trans channel cbks failed");
    g_cmIsRegistered = true;
    CM_LOGI("CM_RegDynTransChannelCbks success");
    return CM_SUCCESS;
}

void CM_UnRegDynTransChannelCbks(void)
{
    CM_CHECK_RETURN(g_cmIsRegistered, "has not reg dyn trans channel cbks");
    CM_DynTransChannStateMgrUnRegCbks();
    g_cmIsRegistered = false;
}

uint32_t CM_DynTransChannelEstablishReq(const CM_DynTransChannelEstablishParamReq_S *param)
{
    CM_CHECK_RETURN_RET(g_cmIsRegistered, CM_FAIL, "has not reg dyn trans channel cbks");
    CM_CHECK_RETURN_RET(param != NULL, CM_INVALID_PARAM_ERR, "param is null");
    CM_CHECK_RETURN_RET(((param->expectedTransportMode == CM_ACCESS_TRANS_MODE_UNICAST) &&
        (param->transModeConfig.transMode < CM_TRANS_MODE_MAX)),
        CM_INVALID_PARAM_ERR, "param is invalid, expectedTransportMode:%hhu, transMode:%hhu",
        param->expectedTransportMode, param->transModeConfig.transMode);
    CM_CHECK_RETURN_RET(param->slqi < CM_TRANS_CHANNEL_SLQI_MAX, CM_INVALID_PARAM_ERR,
        "invalid slqi:%hhu.", param->slqi);

    CM_LOGD("CM_DynTransChannelEstablishReq start, addr:%s", GetEncryptAddr(&param->addr).buf);
    uint32_t ret = CM_DynTransChannelCreate(param);
    CM_CHECK_RETURN_RET((ret == CM_SUCCESS), CM_FAIL, "dyn trans channel create failed");
    return CM_SUCCESS;
}

uint32_t CM_DynTransChannelReleaseReq(const CM_DynTransChannelReleaseParamReq_S *param)
{
    CM_CHECK_RETURN_RET(g_cmIsRegistered, CM_FAIL, "has not reg dyn trans channel cbks");
    CM_CHECK_RETURN_RET(param != NULL, CM_INVALID_PARAM_ERR, "param is null");
    CM_LOGI("CM_DynTransChannelReleaseReq start, addr:%s, srcTcid:0x%02x",
        GetEncryptAddr(&param->addr).buf, param->srcTcid);
    CM_CHECK_RETURN_RET(CM_TRANS_CHANNEL_IS_DYNAMIC(param->srcTcid), CM_INVALID_PARAM_ERR, "srcTcid "
        "is not defined in dynamic range");
    SleLogicLink_S *logicLink = SleLogicLinkGetByAddr(&param->addr);
    CM_CHECK_RETURN_RET(logicLink != NULL, CM_FAIL, "get logic link by addr failed");
    CM_DynTransChanParam_S loopParam = { 0 };
    loopParam.lcid = logicLink->lcid;
    loopParam.localTcid = param->srcTcid;
    loopParam.remoteTcid = param->dstTcid;
    uint32_t ret = CM_DynTransChanPostEventAndHandler(CM_DYN_TRANS_CHAN_EVENT_ACTIVE_RELEASING_REQ, &loopParam);
    CM_CHECK_RETURN_RET((ret == CM_SUCCESS), CM_FAIL, "post event and handler failed, ret=0x%08x", ret);
    return CM_SUCCESS;
}