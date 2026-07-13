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

#include "sle_access_dli.h"
#include "securec.h"
#include "byte_codec.h"
#include "cm_dli_adapter.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "cm_util.h"
#include "cm_common.h"
#include "cm_access_mgr.h"
#include "common_ext_func_wrapper.h"
#include "hadm_ext_func_wrapper.h"
#include "dli.h"
#include "dli_cmd.h"
#include "dli_errno.h"
#include "dli_cmd_struct.h"
#include "dli_event_struct.h"
#include "sle_logic_link_mgr.h"
#include "sdf_mem.h"
#include "sle_connect_param.h"


static void SleDliReadAcceptFilterListSizeCallback(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_LOGI("status:%hu", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "cmd res or event param is null");

    CM_ExeCmdCbk cbk = CM_AccessGetCbk(SLE_ACCESS_CBK_READ_ACCEPT_FLT_LIST_SIZE);
    if (cbk == NULL) {
        CM_LOGE("cbk is null");
        return;
    }

    CM_ReadAcceptFilterListSize_S param = {0};
    param.status = (uint8_t)status;
    uint8_t *data = (uint8_t *)cmdRes->eventParameter;
    param.listSize = data[0];  /* eventParameter直接指向最大实体个数(1字节) */

    CM_ExecuteCmdPar_S paramCbk = {0};
    paramCbk.eventParameter = &param;
    paramCbk.size = sizeof(CM_ReadAcceptFilterListSize_S);
    cbk(context, status, &paramCbk);
}

static void SleAccessLinkStatusReport(uint32_t versionAndIndex, uint8_t type, void *param, uint16_t size,
    uint8_t result)
{
    CM_ExecuteCmdPar_S cmdPara = {0};
    CM_ExeCmdCbk connectCbk = NULL;
    CM_LOGI("sle access link status report, versionAndIndex:0x%08x, type:%hhu", versionAndIndex, type);
    connectCbk = CM_AccessGetCbk(type);
    if (connectCbk == NULL) {
        CM_LOGE("connectCbk is null, type:%hhu", type);
        return;
    }
    cmdPara.eventParameter = param;
    cmdPara.size = size;
    connectCbk((void *)(uintptr_t)versionAndIndex, result, &cmdPara);
}

static void SleAccessReportConnectException(uint16_t lcid, uint8_t status)
{
    CM_ConnectExceptionRsp_S connectRsp = {0};
    connectRsp.lcid = lcid;
    SleAccessLinkStatusReport(CM_PackVersionLocalIndex(CM_CONNECT_VERSION_1_0, CM_CONNECT_LOCAL_INDEX_0),
        SLE_ACCESS_CBK_CONNECT_EXCEPTION, &connectRsp, sizeof(CM_ConnectExceptionRsp_S), status);
}

static void SleAccessLinkEstablishRsp(CM_AccessRspParam_S *rsp)
{
    CM_ConnectParamRsp_S connectRsp = { 0 };
    connectRsp.version = rsp->version;
    connectRsp.localIndex = rsp->localIndex;
    connectRsp.lcid = rsp->lcid;
    (void)memcpy_s(&connectRsp.addr, sizeof(SLE_Addr_S), &rsp->peerAddr, sizeof(SLE_Addr_S));
    connectRsp.role = rsp->role;
    connectRsp.discReason = rsp->discReason;
    connectRsp.connCompleteType = rsp->connCompleteType;
    connectRsp.advHandle = rsp->advHandle;
    SleAccessLinkStatusReport(CM_PackVersionLocalIndex(rsp->version, rsp->localIndex), SLE_ACCESS_CBK_CONNECT,
        &connectRsp, sizeof(CM_ConnectParamRsp_S), rsp->result);
}

static void SleAccessConnectCbkGenResponse(CM_AccessRspParam_S *response, const DLI_ConnectionCompleteEvt *evt,
    uint32_t versionAndLocalIndex)
{
    response->role = evt->role;
    response->lcid = evt->connHandle;
    response->peerAddr.type = evt->peerAddressType;
    response->version = CM_UnPackVersion(versionAndLocalIndex);
    response->localIndex = CM_UnPackLocalIndex(versionAndLocalIndex);
    response->discReason = evt->status;
    response->connCompleteType = evt->connCompleteType;
    response->advHandle = evt->advHandle;
}

static void SleAccessCancelConnectingLink(void)
{
    SleLogicLink_S *link = SleLogicLinkGetByStatus(CM_LINK_STATE_CONNECTING);
    CM_CHECK_RETURN((link != NULL), "connect timeout, current has not exist connecting logic link");
    link->discReason = DLI_CONNECTION_TIMEOUT; // 记录断开原因, 用于后面取消连接操作回调时记录返回给用户
    // 此处不做超时处理，否则背景连接扫描参数连接间隔较小容易超时，需要由连接管理层负责是否需要取消连接
}

static void SleAccessConnectCbkSetRspStatusAndDiscReason(CM_AccessRspParam_S *response, const SleLogicLink_S *link,
    uint8_t dliStatus)
{
    response->result = link->status;
    // 适配芯片操作，当取消连接后，回调状态值为DLI_UNKNOWN_CONNECTION_IDENTIFIER
    if (dliStatus == DLI_UNKNOWN_CONNECTION_IDENTIFIER && link->discReason == DLI_CONNECTION_TIMEOUT) {
        // 记录上次超时断开原因为超时，反馈给用户（可能为连接过慢或者也可能为无效地址）
        response->discReason = link->discReason;
    }
}

static void SleAccessConnectCbkSetLinkStatusAndInfo(SleLogicLink_S *link, const DLI_ConnectionCompleteEvt *evt)
{
    if (evt->status == DLI_SUCCESS) {
        link->status = CM_LINK_STATE_CONNECTED;
        link->lcid = evt->connHandle;
        link->centralClockAccuracy = evt->centralClockAccuracy;
        link->role = evt->role;
        link->connInterval = evt->connectionInterval;
        link->timeout = evt->supervisionTimeout;
        link->connCompleteType = evt->connCompleteType;
        link->advHandle = evt->advHandle;
    } else {
        link->status = CM_LINK_STATE_DISCONNECTED;
    }
}

static SleLogicLink_S* SleLogicLinkSetAddrWhenActiveConnDone(const SLE_Addr_S *rspAddr, uint16_t lcid)
{
    SleLogicLink_S *link = SleLogicLinkGetByAddr(rspAddr);
    if (link != NULL && link->status == CM_LINK_STATE_CONNECTED) {
        // 异常判断，该地址不应该在已完成连接列表中
        CM_LOGE("addr:%s has connected in logic link list", GET_ENC_ADDR(rspAddr));
        return NULL;
    }
    link = SleLogicLinkGetByStatus(CM_LINK_STATE_CONNECTING);
    if (link != NULL) {
        // 如果存在一个连接中的节点，代表本次处在白名单连接，连接前白名单已预置了一个连接中全0的空地址
        CM_LOGI("before connect, initiatorFilterPolicy is true, peerAddress:%s",
            GET_ENC_ADDR(&link->rmtAddr));
        (void)memcpy_s(&link->rmtAddr, sizeof(SLE_Addr_S), rspAddr, sizeof(SLE_Addr_S));
    } else {
        // 异常场景：如果不存在，此时有可能用户已经取消连接，尝试释放芯片连接资源
        CM_LOGW("has not a connecting address, release the connection, lcid:0x%04x", lcid);
        CM_AccessParamReq_S linkParam;
        linkParam.lcid = lcid;
        linkParam.discReason = CM_DISC_REASON_REMOTE_USER_TERMINATED;
        (void)SleAccessLinkReleaseReq(&linkParam);
    }
    return link;
}

static bool SleGetConnCompleteTypeIsPassive(uint8_t connCompleteType)
{
    return (connCompleteType == CM_CONN_COMPLETE_ADV) ? true : false;
}

static void SleAccessConnectCbkProc(const void *context, uint8_t status, const DLI_ExecuteCmdRetParam *cmdRes)
{
    uint32_t versionAndLocalIndex = (context != NULL) ? ((DLI_ConnCbkContext *)context)->versionAndLocalIndex : 0;
    SleLogicLink_S *link = NULL;
    CM_AccessRspParam_S rsp = { 0 };
    DLI_ConnectionCompleteEvt completeEvt = { 0 }; // 其他值设为0即可
    DLI_ConnectionCompleteEvt *evt = NULL;
    if (status != DLI_SUCCESS) {
        if (status == DLI_COMMAND_TIMEOUT) {
            SleAccessCancelConnectingLink();
            return;
        }
        evt = &completeEvt;
        evt->status = status; // 比如参数无效时，status值为DLI_INVALID_PARAMETERS
        evt->connHandle = CM_INVALID_LCID;
    } else {
        evt = (DLI_ConnectionCompleteEvt *)cmdRes->eventParameter;
    }
    CM_CHECK_RETURN((evt->connCompleteType == CM_CONN_COMPLETE_SCAN) ||
        (evt->connCompleteType == CM_CONN_COMPLETE_ADV), "CompleteType:%hhu is invalid", evt->connCompleteType);
    SleAccessConnectCbkGenResponse(&rsp, evt, versionAndLocalIndex);
    SLE_Addr_S *rspAddr = &rsp.peerAddr;
    bool isPassiveConnComplete = SleGetConnCompleteTypeIsPassive(evt->connCompleteType);
    if (evt->status == DLI_SUCCESS) {
        (void)memcpy_s(rspAddr->addr, sizeof(rspAddr->addr), evt->peerAddress, sizeof(evt->peerAddress));
        if (isPassiveConnComplete) {
            // 被动连接完成
            SleLogicLink_S *ll = SleLogicLinkGetByAddr(rspAddr);
            // 异常判断，该地址不应该在连接列表中
            CM_CHECK_RETURN((ll == NULL), "addr:%s has in logic link list", GET_ENC_ADDR(rspAddr));
            link = SleLogicLinkAdd(rspAddr);
            CM_CHECK_RETURN((link != NULL), "sle cm connection cbk link create failed");
        } else {
            link = SleLogicLinkSetAddrWhenActiveConnDone(rspAddr, evt->connHandle);
        }
    } else {
        if (!isPassiveConnComplete) {
            link = SleLogicLinkGetByStatus(CM_LINK_STATE_CONNECTING);
            if (link != NULL) {
                (void)memcpy_s(rspAddr, sizeof(SLE_Addr_S), &link->rmtAddr, sizeof(SLE_Addr_S));
            }
        }
    }
    CM_CHECK_RETURN((link != NULL), "logic link is null");
    SleAccessConnectCbkSetLinkStatusAndInfo(link, evt);
    SleAccessConnectCbkSetRspStatusAndDiscReason(&rsp, link, status);
    CM_LOGI("lcid:0x%04x, evt status:0x%02x, link status:%d, role:%hhu, connCompleteType:%hhu, advHandle:0x%02x",
        rsp.lcid, evt->status, link->status, rsp.role, evt->connCompleteType, evt->advHandle);
    SleAccessLinkEstablishRsp(&rsp);
}

static void SleAccessConnectCbk(void *context, uint16_t dliStatus, DLI_ExecuteCmdRetParam *cmdRes)
{
    uint8_t status = (uint8_t)dliStatus;
    CM_LOGI("status:0x%02x", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");
    CM_LOGI("complete evt expectSize:%u, cmd error expectSize:%u, realSize:%u",
        sizeof(DLI_ConnectionCompleteEvt), sizeof(DLI_CommandErrorStru), cmdRes->size);
    if (status == DLI_COMMAND_DISALLOWED) {
        // 若已经下发过Create_Connection命令，未取消，再重复发起Create_Connection命令，
        // 则Controller侧将返回一个命令不允许(0x0B)的错误码, 需忽略处理
        CM_LOGD("connect command disabled(0x0B), ignore it");
        return;
    }
    if (status != DLI_SUCCESS) {
        // 下发连接命令后，芯片可能由于内存不足等问题，导致命令下发失败，报0x20等错误码
        // 注意：芯片短时间内可能无法恢复，此处不再继续往下触发再次连接操作，可由用户刷新目标连接设备列表时，进行重新触发连接操作
        CM_LOGE("connect command status:0x%02x error!", status);
        return;
    }
    SleAccessConnectCbkProc(context, status, cmdRes);
}

static void SleAccessLinkReleaseRsp(CM_AccessRspParam_S *rsp)
{
    CM_DisconnectParamRsp_S disConnectRsp = { 0 };
    disConnectRsp.version = rsp->version;
    disConnectRsp.localIndex = rsp->localIndex;
    disConnectRsp.lcid = rsp->lcid;
    disConnectRsp.discReason = rsp->discReason;
    disConnectRsp.role = rsp->role;
    (void)memcpy_s(&disConnectRsp.addr, sizeof(SLE_Addr_S), &rsp->peerAddr, sizeof(SLE_Addr_S));
    SleAccessLinkStatusReport(CM_PackVersionLocalIndex(rsp->version, rsp->localIndex), SLE_ACCESS_CBK_DISCONNECT,
        &disConnectRsp, sizeof(CM_DisconnectParamRsp_S), rsp->result);
}

static void SleAccessDisconnectCbk(void *context, uint16_t statuss, DLI_ExecuteCmdRetParam *cmdRes)
{
    uint8_t status = (uint8_t)statuss;
    CM_LOGI("status:0x%02x", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");
    DLI_DisconnectEvt *evt = NULL;
    DLI_DisconnectEvt completeEvt = { 0 };
    if (cmdRes->cmdOpcode == DLI_CMD_ERROR_EVT) {
        evt = (DLI_DisconnectEvt *)&completeEvt;
        evt->connHandle = (context != NULL) ? ((DLI_ConnCbkContext *)context)->connHandle : CM_INVALID_LCID;
        evt->reason = status;
        // 已出错其他值设为0即可
    } else {
        evt = (DLI_DisconnectEvt *)cmdRes->eventParameter;
    }

    uint32_t versionAndLocalIndex = (context != NULL) ? ((DLI_ConnCbkContext *)context)->versionAndLocalIndex : 0;
    CM_AccessRspParam_S response = { 0 };
    response.version = CM_UnPackVersion(versionAndLocalIndex);
    response.localIndex = CM_UnPackLocalIndex(versionAndLocalIndex);
    response.lcid = evt->connHandle;
    response.discReason = evt->reason;
    response.result = status;
    SleLogicLink_S *link = SleLogicLinkGetByLcid(response.lcid);
    if (link != NULL) {
        response.role = link->role;
    }
    CM_LOGI("version:0x%02x, localIndex:0x%04x, handle:0x%04x, discReason:0x%02x, status:%hhu, role:%hhu, "
        "cmdOpcode:0x%04x, size:%u",
        response.version, response.localIndex, response.lcid, response.discReason, status, response.role,
        cmdRes->cmdOpcode, cmdRes->size);
    SleAccessLinkReleaseRsp(&response);
}

static void SleAccessClearAccessFilterListCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_LOGI("status:%hu", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "cmd res or event param is null");
}

static void SleAccessAddDevToAccessFilterListCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_LOGI("status:%hu", status);
    if (cmdRes == NULL || cmdRes->eventParameter == NULL) {
        // the cmd no cmd result
        CM_LOGD("has cmd res or event param");
        return;
    }
}

static void SleAccessRmvDevToAccessFilterListCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_LOGI("status:%hu", status);
    if (cmdRes == NULL || cmdRes->eventParameter == NULL) {
        // the cmd no cmd result
        CM_LOGD("has cmd res or event param");
        return;
    }
}

static void SleAccessConnectionCancelCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_LOGI("status:%hu", status);
    uint32_t versionAndLocalIndex = (context != NULL) ? ((DLI_ConnCbkContext *)context)->versionAndLocalIndex : 0;
    uint8_t version = CM_UnPackVersion(versionAndLocalIndex);
    uint16_t localIndex = CM_UnPackLocalIndex(versionAndLocalIndex);

    uint8_t result = (uint8_t)status;
    SleAccessLinkStatusReport(CM_PackVersionLocalIndex(version, localIndex), SLE_ACCESS_CBK_CONNECT_CANCEL,
        &result, sizeof(uint8_t), (uint8_t)status);
}

static void SleAccessLinkRemoteParamUpdateReq(uint32_t versionAndLocalIndex, DLI_RemoteConnParamReqEvt *rsp,
    uint16_t status)
{
    CM_ConnectRemoteUpdateParamReq_S updateReq = { 0 };
    updateReq.lcid = rsp->connHandle;
    updateReq.intervalMin = rsp->connIntervalMin;
    updateReq.intervalMax = rsp->connIntervalMax;
    updateReq.maxLatency = rsp->maxLatency;
    updateReq.supervisionTimeout = rsp->supervisionTimeout;
    SleAccessLinkStatusReport(versionAndLocalIndex, SLE_ACCESS_CBK_CONNECT_REMOTE_UPDATE_REQ,
        &updateReq, sizeof(CM_ConnectRemoteUpdateParamReq_S), (uint8_t)status);
}

static void SleAccessConnectUpdateRequestCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    const uint16_t CM_CONN_TEMP_INTERVAL = 0x18;
    // 收到此事件需要回复0x1808命令，调用方发送
    CM_LOGI("status:%hu", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "cmd res or event param is null");

    DLI_RemoteConnParamReqEvt *evt = (DLI_RemoteConnParamReqEvt *)cmdRes->eventParameter;

    SleLogicLink_S *link = SleLogicLinkGetByLcid(evt->connHandle);
    CM_CHECK_RETURN((link != NULL), "connect update request cbk failed, logic link not exist");

    uint32_t versionAndLocalIndex = (context != NULL) ? ((DLI_ConnCbkContext *)context)->versionAndLocalIndex : 0;
    SleAccessLinkRemoteParamUpdateReq(versionAndLocalIndex, evt, status);

    // 连接参数更新是暂时使用请求参数，后续做合并处理
    DLI_RemConParamReqReplyParam replyParam = { 0 };
    replyParam.connHandle = evt->connHandle;
    replyParam.reason = 0;
    bool isSupport = COMMON_IsSupportSetMaxInterval();
    if (isSupport && (evt->connIntervalMax < CM_CONN_TEMP_INTERVAL)) { // 芯片默认按max参数调度
        replyParam.connIntervalMin = CM_CONN_TEMP_INTERVAL;
        replyParam.connIntervalMax = CM_CONN_TEMP_INTERVAL;
        CM_LOGI("updated connIntervalMin and connIntervalMax to 0x%04x", CM_CONN_TEMP_INTERVAL);
    } else {
        replyParam.connIntervalMin = evt->connIntervalMin;
        replyParam.connIntervalMax = evt->connIntervalMax;
    }
    replyParam.txRxInterval  = evt->txRxInterval;
    replyParam.eventInterval = evt->eventInterval;
    replyParam.maxLatency = evt->maxLatency;
    replyParam.supervisionTimeout = evt->supervisionTimeout;
    replyParam.systemTimeUnit = evt->systemTimeUnit;
    replyParam.txRxFlag = evt->txRxFlag;
    CM_LOGI("update replay:connHandle:0x%04x, connIntervalMin:0x%04x, connIntervalMax:0x%04x",
        replyParam.connHandle, replyParam.connIntervalMin, replyParam.connIntervalMax);

    CM_LOGI("update replay:txRxInterval:0x%02x, eventInterval:0x%04x, maxLatency:0x%04x, "
        "supervisionTimeout:0x%04x, systemTimeUnit:0x%02x, txRxFlag:0x%02x",
        replyParam.txRxInterval,
        replyParam.eventInterval,
        replyParam.maxLatency,
        replyParam.supervisionTimeout,
        replyParam.systemTimeUnit,
        replyParam.txRxFlag);

    uint32_t ret = DLI_RemoteConnectionParamReqReply(&replyParam);
    CM_CHECK_RETURN((ret == DLI_SUCCESS), "DLI_RemoteConnectionParamReqReply failed");
}

static void SleAccessLinkParamUpdateRsp(uint32_t versionAndLocalIndex, DLI_ConnectionUpdateCmpEvt *rsp, uint16_t status)
{
    CM_ConnectUpdateParamRsp_S connectRsp = { 0 };
    connectRsp.version = CM_UnPackVersion(versionAndLocalIndex);
    connectRsp.localIndex = CM_UnPackLocalIndex(versionAndLocalIndex);
    connectRsp.lcid = rsp->connHandle;
    connectRsp.extension.interval = rsp->connInterval;
    connectRsp.extension.latency = rsp->maxLatency;
    connectRsp.extension.supervisionTimeout = rsp->supervisionTimeout;
    CM_LOGI("update rsp, connHandle:0x%04x, connInterval:0x%04x, maxLatency:0x%04x, supervisionTimeout:0x%04x, "
        "systemTimeUnit:0x%02x",
        rsp->connHandle, rsp->connInterval, rsp->maxLatency,  rsp->supervisionTimeout, rsp->systemTimeUnit);
    SleLogicLink_S *link = SleLogicLinkGetByLcid(rsp->connHandle);
    if (link == NULL) {
        CM_LOGE("logic link get handle fail, handle:0x%04x", rsp->connHandle);
        return;
    }
    (void)memcpy_s(&connectRsp.addr, sizeof(SLE_Addr_S), &link->rmtAddr, sizeof(SLE_Addr_S));
    connectRsp.result = link->status;
    SleAccessLinkStatusReport(versionAndLocalIndex, SLE_ACCESS_CBK_CONNECT_UPDATE,
        &connectRsp, sizeof(CM_ConnectUpdateParamRsp_S), (uint8_t)status);
}

static void SleAccessConnectUpdateCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_LOGI("status:%hu", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");

    uint32_t versionAndLocalIndex = (context != NULL) ? ((DLI_ConnCbkContext *)context)->versionAndLocalIndex : 0;
    DLI_ConnectionUpdateCmpEvt completeEvt = { 0 };
    DLI_ConnectionUpdateCmpEvt *evt = NULL;
    if (status == DLI_SUCCESS) {
        evt = (DLI_ConnectionUpdateCmpEvt *)cmdRes->eventParameter;
    } else {
        evt = (DLI_ConnectionUpdateCmpEvt *)&completeEvt;
        // 已出错其他值设为0即可
        evt->connHandle = (context != NULL) ? ((DLI_ConnCbkContext *)context)->connHandle : CM_INVALID_LCID;
    }
    CM_LOGI("SleAccessConnectUpdateCbk handle:0x%04x, status:%hu", evt->connHandle, status);
    SleAccessLinkParamUpdateRsp(versionAndLocalIndex, evt, status);
}

static void SleAccessRemoteConnectionParamRequestsReplyCbk(void *context, uint16_t status,
    DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_LOGI("status:%hu", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");
}

static void SleAccessRemoteRssiRequestsReplyCbk(void *context, uint16_t status,
    DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(status == DLI_SUCCESS, "req rssi failed");
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");

    DLI_ReadRemoteRssiEvt *evt = (DLI_ReadRemoteRssiEvt *)cmdRes->eventParameter;
    CM_LOGI("rssi cbk, lcid:0x%04x, rssi:%d", evt->connHandle, evt->rssi);
    CM_ExeCmdCbk cbk = CM_AccessGetCbk(SLE_ACCESS_CBK_READ_REMOTE_RSSI);
    CM_CHECK_RETURN(cbk != NULL, "cbk is null");

    CM_ReadRemoteRssiRsp_S rsp = {0};
    rsp.lcid = evt->connHandle;
    rsp.status = (uint8_t)status;
    rsp.rssi = evt->rssi;
    CM_ExecuteCmdPar_S paramCbk = {0};
    paramCbk.eventParameter = &rsp;
    paramCbk.size = sizeof(CM_ReadRemoteRssiRsp_S);
    cbk(context, status, &paramCbk);
}

static void SleAccessSetRxDataFilterCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_LOGI("status:%hu", status);
    CM_CHECK_RETURN((context != NULL && cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");

    uint32_t versionAndLocalIndex = ((DLI_ConnCbkContext *)context)->versionAndLocalIndex;

    DLI_SetRxDataFilterEvt completeEvt = { 0 };
    DLI_SetRxDataFilterEvt *evt = NULL;
    if (status == DLI_SUCCESS) {
        evt = (DLI_SetRxDataFilterEvt *)cmdRes->eventParameter;
    } else {
        evt = (DLI_SetRxDataFilterEvt *)&completeEvt;
        // 已出错其他值设为0即可
        evt->connHandle = ((DLI_ConnCbkContext *)context)->connHandle;
    }
    CM_SetRxDataFilterRsp_S rsp = {0};
    rsp.status = (uint8_t)status;
    rsp.lcid = evt->connHandle;
    CM_LOGD("set rx data filter cbk, lcid:0x%04x, status:0x%02x", evt->connHandle, rsp.status);
    SleAccessLinkStatusReport(versionAndLocalIndex, SLE_ACCESS_CBK_SET_RX_DATA_FILTER,
        &rsp, sizeof(CM_SetRxDataFilterRsp_S), (uint8_t)status);
}

static void SleAccessSetPhyCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_LOGI("status:%hu", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");

    uint32_t versionAndLocalIndex = (context != NULL) ? ((DLI_ConnCbkContext *)context)->versionAndLocalIndex : 0;
    uint8_t version = CM_UnPackVersion(versionAndLocalIndex);
    uint16_t localIndex = CM_UnPackLocalIndex(versionAndLocalIndex);

    DLI_SetPhyEvt *evt = (DLI_SetPhyEvt *)cmdRes->eventParameter;
    CM_SetPhyRsp_S setPhyRsp = {0};
    setPhyRsp.status = (uint8_t)evt->status;
    setPhyRsp.lcid = evt->connHandle;
    setPhyRsp.txFormat = evt->txFormat;
    setPhyRsp.rxFormat = evt->rxFormat;
    setPhyRsp.txPhy =  evt->txPhy;
    setPhyRsp.rxPhy = evt->rxPhy;
    setPhyRsp.txPilotDensity = evt->txPilotDensity;
    setPhyRsp.rxPilotDensity = evt->rxPilotDensity;
    setPhyRsp.gFeedback = evt->gFeedback;
    setPhyRsp.tFeedback = evt->tFeedback;
    SleAccessLinkStatusReport(CM_PackVersionLocalIndex(version, localIndex), SLE_ACCESS_CBK_SET_PHY,
        &setPhyRsp, sizeof(CM_SetPhyRsp_S), (uint8_t)status);
}

static void SleAccessDataLenChangeCbk(void *context, uint16_t statuss, DLI_ExecuteCmdRetParam *cmdRes)
{
    uint8_t status = (uint8_t)statuss;
    CM_LOGI("status:0x%02x", status);

    DLI_DataLenChangeEvt completeEvt = { 0 };
    DLI_DataLenChangeEvt *evt = NULL;
    if (status == DLI_SUCCESS) {
        CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "cmdRes or event param is null");
        evt = (DLI_DataLenChangeEvt *)cmdRes->eventParameter;
        SleLogicLink_S *link = SleLogicLinkGetByLcid(evt->connHandle);
        if (link != NULL) {
            link->maxRxOctets = evt->maxRxOctets;
            link->maxTxOctets = evt->maxTxOctets;
        }
    } else {
        evt = (DLI_DataLenChangeEvt *)&completeEvt;
        // 已出错其他值设为0即可
        evt->connHandle = (context != NULL) ? ((DLI_ConnCbkContext *)context)->connHandle : CM_INVALID_LCID;
    }
    CM_LOGI("data len change cbk, lcid:0x%04x, maxTxOctets:%hu, maxRxOctets:%hu",
        evt->connHandle, evt->maxTxOctets, evt->maxRxOctets);
}

static void SleAccessSetDataLenCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_LOGI("status:%hu", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");

    uint32_t versionAndLocalIndex = (context != NULL) ? ((DLI_ConnCbkContext *)context)->versionAndLocalIndex : 0;
    uint8_t version = CM_UnPackVersion(versionAndLocalIndex);
    uint16_t localIndex = CM_UnPackLocalIndex(versionAndLocalIndex);

    DLI_DataLenChangeEvt *evt = (DLI_DataLenChangeEvt *)cmdRes->eventParameter;
    CM_ConnectParamRsp_S connectRsp = {0};
    connectRsp.localIndex = localIndex;
    connectRsp.version = version;
    connectRsp.lcid = evt->connHandle;
    SleAccessLinkStatusReport(CM_PackVersionLocalIndex(version, localIndex), SLE_ACCESS_CBK_SET_DATA_LEN,
        &connectRsp, sizeof(CM_ConnectParamRsp_S), (uint8_t)status);
}

static void SleAccessSetHostChannelClassificaitonCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_LOGI("status:%hu", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");
}

static void SleAccessReadRemoteVersionRsp(uint32_t versionAndLocalIndex, uint16_t status, SleLogicLink_S *link)
{
    CM_ReadRemoteVersionRsp_S rsp = { 0 };
    rsp.lcid = link->lcid;
    rsp.role = link->role;
    rsp.version = link->version;
    rsp.companyId = link->companyId;
    rsp.subversion = link->subversion;
    (void)memcpy_s(&rsp.addr, sizeof(SLE_Addr_S), &link->rmtAddr, sizeof(SLE_Addr_S));
    CM_LOGI("lcid:0x%04x, role:0x%02x, version:0x%02x, companyId:0x%04x, subversion:0x%04x, addr:%s",
        rsp.lcid, rsp.role, rsp.version, rsp.companyId, rsp.subversion, GET_ENC_ADDR(&rsp.addr));
    SleAccessLinkStatusReport(versionAndLocalIndex, SLE_ACCESS_CBK_READ_REMOTE_VERSION, &rsp,
        sizeof(CM_ReadRemoteVersionRsp_S), (uint8_t)status);
}

static void SleAccessReadRemoteFeatureAndVersionRsp(uint32_t versionAndLocalIndex, uint16_t status,
    SleLogicLink_S *link)
{
    CM_ReadRemoteFeatureVersionRsp_S readRsp = { 0 };
    readRsp.lcid = link->lcid;
    readRsp.version = link->version;
    readRsp.companyId = link->companyId;
    readRsp.subversion = link->subversion;
    (void)memcpy_s(readRsp.features, sizeof(readRsp.features), link->features, sizeof(link->features));
    (void)memcpy_s(&readRsp.addr, sizeof(SLE_Addr_S), &link->rmtAddr, sizeof(SLE_Addr_S));
    CM_LOGI("lcid:0x%04x, version:0x%02x, companyId:0x%04x, subversion:0x%04x, readRsp.addr:%s",
        readRsp.lcid, readRsp.version, readRsp.companyId, readRsp.subversion, GET_ENC_ADDR(&readRsp.addr));
    SleAccessLinkStatusReport(versionAndLocalIndex, SLE_ACCESS_CBK_READ_REMOTE_FEATURE_AND_VERSION, &readRsp,
        sizeof(CM_ReadRemoteFeatureVersionRsp_S), (uint8_t)status);
}

static void SleAccessReadRemoteFeatureCbk(void *context, uint16_t statuss, DLI_ExecuteCmdRetParam *cmdRes)
{
    uint8_t status = (uint8_t)statuss;
    CM_LOGI("status:0x%02x", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");

    uint32_t versionAndLocalIndex = (context != NULL) ? ((DLI_ConnCbkContext *)context)->versionAndLocalIndex : 0;
    uint8_t version = CM_UnPackVersion(versionAndLocalIndex);
    uint16_t localIndex = CM_UnPackLocalIndex(versionAndLocalIndex);

    DLI_ReadRemoteFeatsEvt completeEvt = { 0 };
    DLI_ReadRemoteFeatsEvt *evt = NULL;
    if (status == DLI_SUCCESS) {
        evt = (DLI_ReadRemoteFeatsEvt *)cmdRes->eventParameter;
    } else {
        evt = (DLI_ReadRemoteFeatsEvt *)&completeEvt;
        // 已出错其他值设为0即可
        evt->connHandle = (context != NULL) ? ((DLI_ConnCbkContext *)context)->connHandle : CM_INVALID_LCID;
        evt->status = status;
    }

    SleLogicLink_S *link = SleLogicLinkGetByLcid(evt->connHandle);
    CM_CHECK_RETURN(link != NULL, "logic link get handle fail, handle:0x%04x", evt->connHandle);
    if (memcpy_s(link->features, CM_FEATURES_PARAM_LEN, evt->feats, sizeof(evt->feats)) != EOK) {
        CM_LOGE("read remote feature memcpy fail.");
        goto SLE_READ_REMOTE_FEATURE_CBK_FAILED;
    }

    CM_ReadRemoteFeatureVersionRsp_S readRsp = { 0 };
    readRsp.lcid = evt->connHandle;
    (void)memcpy_s(&readRsp.addr, sizeof(SLE_Addr_S), &link->rmtAddr, sizeof(SLE_Addr_S));
    (void)memcpy_s(readRsp.features, sizeof(readRsp.features), evt->feats, sizeof(evt->feats));
    SleAccessLinkStatusReport(CM_PackVersionLocalIndex(version, localIndex),
        SLE_ACCESS_CBK_READ_REMOTE_FEATURE, &readRsp, sizeof(CM_ReadRemoteFeatureVersionRsp_S), status);
    SleAccessReadRemoteFeatureAndVersionRsp(versionAndLocalIndex, status, link);
    if (status != CM_SUCCESS) {
        CM_LOGE("status is not success, interrupt proc");
        goto SLE_READ_REMOTE_FEATURE_CBK_FAILED;
    }

    /* 设置连接链路所使用的最大传输Payload字节数 */
    DLI_SetDataLenParam setDataLenParam = {0};
    setDataLenParam.connHandle = evt->connHandle;
    setDataLenParam.txOctets = DLI_GetAcbDataLen();
    CM_LOGI("handle:0x%04x, txOctets:%u", evt->connHandle, setDataLenParam.txOctets);
    uint32_t ret = DLI_SetDataLength(&setDataLenParam);
    if (ret != DLI_SUCCESS) {
        CM_LOGE("set data len failed");
        goto SLE_READ_REMOTE_FEATURE_CBK_FAILED;
    }
    return;
SLE_READ_REMOTE_FEATURE_CBK_FAILED:
    if (status != DLI_SUCCESS) {
        SleAccessReportConnectException(evt->connHandle, status);
    }
}

static void SleAccessReadRemoteVersionCbk(void *context, uint16_t statuss, DLI_ExecuteCmdRetParam *cmdRes)
{
    uint8_t status = (uint8_t)statuss;
    CM_LOGI("status:0x%02x", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");
    uint32_t versionAndLocalIndex = (context != NULL) ? ((DLI_ConnCbkContext *)context)->versionAndLocalIndex : 0;
    DLI_ReadRemoteVersionEvt completeEvt = { 0 };
    DLI_ReadRemoteVersionEvt *evt = NULL;
    if (status == DLI_SUCCESS) {
        evt = (DLI_ReadRemoteVersionEvt *)cmdRes->eventParameter;
    } else {
        evt = (DLI_ReadRemoteVersionEvt *)&completeEvt;
        // 已出错其他值设为0即可
        evt->connHandle = (context != NULL) ? ((DLI_ConnCbkContext *)context)->connHandle : CM_INVALID_LCID;
        evt->status = status;
    }
    SleLogicLink_S *link = SleLogicLinkGetByLcid(evt->connHandle);
    CM_CHECK_RETURN(link != NULL, "logic link get handle fail, handle:0x%04x", evt->connHandle);
    link->version = evt->version;
    link->companyId = evt->companyIdentifier;
    link->subversion = evt->subversion;
    if (status != CM_SUCCESS) {
        SleAccessReadRemoteFeatureAndVersionRsp(versionAndLocalIndex, status, link);
        goto SLE_READ_REMOTE_VERSION_CBK_FAILED;
    }
    SleAccessReadRemoteVersionRsp(versionAndLocalIndex, status, link);
    if (link->role == CM_G_NODE) {
        if (!DLI_ReadRemoteExtFeatures(link->companyId, link->subversion, link->lcid)) {
            uint32_t ret = SleAccessReadRemoteFeatures(link->lcid);
            if (ret != DLI_SUCCESS) {
                CM_LOGE("sle access read remote features failed");
                goto SLE_READ_REMOTE_VERSION_CBK_FAILED;
            }
        }
    }
SLE_READ_REMOTE_VERSION_CBK_FAILED:
    if (status != DLI_SUCCESS) {
        SleAccessReportConnectException(evt->connHandle, status);
    }
}

static void SleAccessEnableConnHighPowerCbk(void *context, uint16_t dliStatus, DLI_ExecuteCmdRetParam *cmdRes)
{
    uint8_t status = (uint8_t)dliStatus;
    CM_LOGD("status:0x%02x", status);

    DLI_ConnEnableHighPowerCmpEvt completeEvt = { 0 };
    DLI_ConnEnableHighPowerCmpEvt *evt = NULL;
    if (status == DLI_SUCCESS) {
        CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "cmdRes or event param is null");
        evt = (DLI_ConnEnableHighPowerCmpEvt *)cmdRes->eventParameter;
    } else {
        evt = (DLI_ConnEnableHighPowerCmpEvt *)&completeEvt;
        // 已出错其他值设为0即可
        evt->connHandle = (context != NULL) ? ((DLI_ConnCbkContext *)context)->connHandle : CM_INVALID_LCID;
    }
    uint32_t versionAndLocalIndex = (context != NULL) ? ((DLI_ConnCbkContext *)context)->versionAndLocalIndex : 0;
    CM_EnableConnHighPowerRsp_S rsp = {0};
    rsp.status = status;
    rsp.lcid = evt->connHandle;
    CM_LOGI("sle enable conn high power proc enter, status:0x%02x, lcid = 0x%04x", rsp.status, rsp.lcid);
    SleAccessLinkStatusReport(versionAndLocalIndex, SLE_ACCESS_CBK_ENABLE_CONN_HIGH_POWER, &rsp,
        sizeof(CM_EnableConnHighPowerRsp_S), (uint8_t)status);
}

static void SleAccessSetPeerDevTypeCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_LOGI("status:%hu", status);
    CM_CHECK_RETURN((context != NULL), "context is null");

    CM_ExeCmdCbk cbk = CM_AccessGetCbk(SLE_ACCESS_CBK_SET_PEER_DEV_TYPE);
    if (cbk == NULL) {
        CM_LOGE("cbk is null");
        return;
    }
    CM_SetPeerDevType_S param = {};
    param.status = (uint8_t)status;
    (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), (SLE_Addr_S *)context, sizeof(SLE_Addr_S));
    CM_ExecuteCmdPar_S paramCbk = {0};
    paramCbk.eventParameter = &param;
    paramCbk.size = (uint32_t)sizeof(CM_SetPeerDevType_S);
    cbk(context, (uint8_t)status, &paramCbk);
}
static void SleAccessSetACBSubrateCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(status == DLI_SUCCESS, "set acb subrate failed");
    CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "param is null");

    DLI_AcbSetSubrateEvt *param = (DLI_AcbSetSubrateEvt *)cmdRes->eventParameter;
    uint16_t lcid = DECODE2BYTE_LITTLE(&param->lcid);
    CM_LOGD("status=%u, lcid=%u", status, lcid);

    SleLogicLink_S *link = SleLogicLinkGetByLcid(lcid);
    if (link == NULL) {
        CM_LOGE("logic link get lcid fail, lcid:0x%04x", lcid);
        return;
    }
    uint16_t subrate = DECODE2BYTE_LITTLE(&param->subrateFactor);
    uint16_t maxLatency = DECODE2BYTE_LITTLE(&param->peripheralLatency);
    uint16_t continuationNum = DECODE2BYTE_LITTLE(&param->continuationNum);
    uint16_t supervisionTimeout = DECODE2BYTE_LITTLE(&param->supervisionTimeout);
    if (link->subrate == subrate) {
        link->isSubrateUpdating = false;
        link->subrate = subrate;
        link->subrateMax = subrate;
        link->maxLatency = maxLatency;
        link->continuationNum = continuationNum;
        link->supervisionTimeout = supervisionTimeout;
    }
    CM_LOGI("status=%u, lcid=%u, set subrate to %hu, maxLatency= %hu, continuationNum= %hu, supervisionTimeout= %hu",
        status, lcid, subrate, maxLatency, continuationNum, supervisionTimeout);

    CM_ExeCmdCbk cbk = CM_AccessGetCbk(SLE_ACCESS_CBK_SET_SUBRATE);
    if (cbk == NULL) {
        CM_LOGE("cbk is null");
        return;
    }
    CM_AcbSubrateCbParam_S subrateParam = {};
    subrateParam.subrate = subrate;
    subrateParam.subrateMax = subrate;
    subrateParam.maxLatency = maxLatency;
    subrateParam.continuationNum = continuationNum;
    subrateParam.supervisionTimeout = supervisionTimeout;
    (void)memcpy_s(&subrateParam.addr, sizeof(SLE_Addr_S), &link->rmtAddr, sizeof(SLE_Addr_S));
    CM_ExecuteCmdPar_S paramCbk = {0};
    paramCbk.eventParameter = &subrateParam;
    paramCbk.size = (uint32_t)sizeof(CM_AcbSubrateCbParam_S);
    cbk(context, (uint8_t)status, &paramCbk);
}

static void SleAccessReqACBSubrateCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    CM_CHECK_RETURN(status == DLI_SUCCESS, "req acb subrate failed");
    CM_CHECK_RETURN(cmdRes != NULL && cmdRes->eventParameter != NULL, "param is null");

    DLI_AcbReqSubrateEvt *param = (DLI_AcbReqSubrateEvt *)cmdRes->eventParameter;
    uint16_t lcid = DECODE2BYTE_LITTLE(&param->lcid);
    uint16_t subrate = DECODE2BYTE_LITTLE(&param->subrateMin);
    uint16_t subrateMax = DECODE2BYTE_LITTLE(&param->subrateMax);
    uint16_t maxLatency = DECODE2BYTE_LITTLE(&param->peripheralLatency);
    uint16_t continuationNum = DECODE2BYTE_LITTLE(&param->continuationNum);
    uint16_t supervisionTimeout = DECODE2BYTE_LITTLE(&param->supervisionTimeout);
    CM_LOGI("req subrate, status=%u, lcid=%u, subrate=%u, subrateMax=%u, maxLatency=%u, continuationNum=%u, "
        "supervisionTimeout=%u", status, lcid, subrate, subrateMax, maxLatency, continuationNum, supervisionTimeout);

    SleLogicLink_S *link = SleLogicLinkGetByLcid(lcid);
    if (link == NULL) {
        CM_LOGE("logic link get lcid fail, lcid:0x%04x", lcid);
        return;
    }

    CM_ExeCmdCbk cbk = CM_AccessGetCbk(SLE_ACCESS_CBK_REQ_SUBRATE);
    if (cbk == NULL) {
        CM_LOGE("cbk is null");
        return;
    }
    CM_AcbSubrateCbParam_S subrateParam = {};
    subrateParam.subrate = subrate;
    subrateParam.subrateMax = subrateMax;
    subrateParam.maxLatency = maxLatency;
    subrateParam.continuationNum = continuationNum;
    subrateParam.supervisionTimeout = supervisionTimeout;
    (void)memcpy_s(&subrateParam.addr, sizeof(SLE_Addr_S), &link->rmtAddr, sizeof(SLE_Addr_S));
    CM_ExecuteCmdPar_S paramCbk = {0};
    paramCbk.eventParameter = &subrateParam;
    paramCbk.size = (uint32_t)sizeof(CM_AcbSubrateCbParam_S);
    cbk(context, (uint8_t)status, &paramCbk);
}

static void SleAccessSetSleDataFilterCbk(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes)
{
    (void)context;
    CM_LOGI("status:%hu", status);
    CM_CHECK_RETURN((cmdRes != NULL && cmdRes->eventParameter != NULL), "param is null");
}

static const struct DLI_CbkLineStru g_sleCmCbk[] = {
    { DLI_CBK_READ_ACCESS_FLT_LIST_SIZE, SleDliReadAcceptFilterListSizeCallback},
    { DLI_CBK_CLEAR_ACCESS_FLT_LIST, (void *)SleAccessClearAccessFilterListCbk },
    { DLI_CBK_ADD_DEV_TO_ACCESS_FLT_LIST, (void *)SleAccessAddDevToAccessFilterListCbk },
    { DLI_CBK_RMV_DEV_FROM_ACCESS_FLT_LIST, (void *)SleAccessRmvDevToAccessFilterListCbk },
    { DLI_CBK_CONNECT_CANCEL, (void *)SleAccessConnectionCancelCbk },
    { DLI_CBK_REMOTE_CONNECT_PARAM_REQ, (void *)SleAccessConnectUpdateRequestCbk },
    { DLI_CBK_CONNECT_UPDATE, (void *)SleAccessConnectUpdateCbk },
    { DLI_CBK_READ_REMOTE_FEATURE, (void *)SleAccessReadRemoteFeatureCbk },
    { DLI_CBK_READ_REMOTE_VERSION, (void *)SleAccessReadRemoteVersionCbk },
    { DLI_CBK_SET_DATA_LEN, (void *)SleAccessSetDataLenCbk },
    { DLI_CBK_SET_HOST_CHANNEL_CLASSIFICATION, (void *)SleAccessSetHostChannelClassificaitonCbk },
    { DLI_CBK_REMOTE_CONNECT_PARAM_REQ_REPLY, (void *)SleAccessRemoteConnectionParamRequestsReplyCbk },
    { DLI_CBK_READ_REMOTE_RSSI, (void *)SleAccessRemoteRssiRequestsReplyCbk },
    { DLI_CBK_SET_RX_DATA_FILTER, (void *)SleAccessSetRxDataFilterCbk },
    { DLI_CBK_SET_PHY, (void *)SleAccessSetPhyCbk },
    { DLI_CBK_DATA_LEN_CHANGE, (void *)SleAccessDataLenChangeCbk },
    { DLI_CBK_ENABLE_CONN_HIGH_POWER, (void *)SleAccessEnableConnHighPowerCbk },
    { DLI_CBK_SET_PEER_DEV_TYPE, (void *)SleAccessSetPeerDevTypeCbk },
    { DLI_CBK_ACB_SET_SUBRATE, SleAccessSetACBSubrateCbk},
    { DLI_CBK_ACB_REQ_SUBRATE, SleAccessReqACBSubrateCbk},
    { DLI_CBK_SET_SLE_DATA_FILTER, (void *)SleAccessSetSleDataFilterCbk },
};

static uint8_t g_sleCmCbkSize = sizeof(g_sleCmCbk) / sizeof(struct DLI_CbkLineStru);

void SleAccessRegCbks(void)
{
    (void)CM_AccessRegDliCbks(g_sleCmCbk, g_sleCmCbkSize);
    (void)CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_CONNECT, SleAccessConnectCbk);
    (void)CM_RegisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_DISCONNECT, SleAccessDisconnectCbk);
}

void SleAccessUnRegCbks(void)
{
    CM_AccessUnRegDliCbk(g_sleCmCbk, g_sleCmCbkSize);
    (void)CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_CONNECT);
    (void)CM_UnregisterDliAdapterCbk(CM_DLI_ADAPTER_CM, CM_DLI_ADAPTER_DISCONNECT);
}

uint32_t SleAccessLinkEstablishReq(CM_AccessParamReq_S *linkParam)
{
    DLI_ConnectionCreateParam param = {0};
    CM_LOGI("SleAccessLinkEstablishReq connectParam get default param");
    DLI_ConnectionCreateParam *connectParam = SleGetDliConnectParam();
    (void)memcpy_s(&param, sizeof(DLI_ConnectionCreateParam), connectParam, sizeof(DLI_ConnectionCreateParam));

    param.peerAddressType = linkParam->peerAddr.type;
    (void)memcpy_s(param.peerAddress, SLE_ADDR_LEN, linkParam->peerAddr.addr, SLE_ADDR_LEN);
    CM_LOGI("bitFrameType:%hhu, version:0x%04x, localIndex:%d, initiatorFilterPolicy:%d, "
        "gtNegotiateInd=%d, initiatingPhys=%d, peerAddressType:%d, peerAddress:%s",
        connectParam->bitFrameType, linkParam->version, linkParam->localIndex, param.initiatorFilterPolicy,
        param.gtNegotiateInd, param.initiatingPhys, param.peerAddressType, GetEncryptAddr(&linkParam->peerAddr).buf);
    CM_LOGI("scanInterval:0x%04x, scanWindow:0x%04x, connectionIntervalMin:0x%04x, connectionIntervalMax:0x%04x, "
        "maxLatency:0x%04x, supervisionTimeout:0x%04x, minCeLength:0x%04x, maxCeLength:0x%04x, parLen:%u",
        param.scanInterval, param.scanWindow, param.connectionIntervalMin, param.connectionIntervalMax,
        param.maxLatency, param.supervisionTimeout, param.minCeLength, param.maxCeLength,
        sizeof(DLI_ConnectionCreateParam));
    uint32_t ret = DLI_CreateConnection(linkParam->version, linkParam->localIndex, &param);
    CM_CHECK_RETURN_RET((ret == DLI_SUCCESS), CM_FAIL, "DLI_CreateConnection failed, ret:0x%08x", ret);
    return CM_SUCCESS;
}

uint32_t SleAccessLinkReleaseReq(CM_AccessParamReq_S *linkParam)
{
    if (linkParam->lcid == CM_INVALID_LCID) {
        // 对于无效的lcid，下发给芯片不起作用，需要提前拦截
        CM_LOGE("lcid is not valid, ignore the release link req");
        return CM_INVALID_PARAM_ERR;
    }
    // 注意：reason若使用0x11，会导致主动断连鼠标后，再次发起连接需要唤醒鼠标才能连接上
    DLI_DisconnectParam param = {0};
    param.connHandle = linkParam->lcid;
    param.reason = linkParam->discReason;
    uint32_t ret = DLI_Disconnect(linkParam->version, linkParam->localIndex, &param);
    CM_CHECK_RETURN_RET((ret == DLI_SUCCESS), CM_FAIL, "DLI_Disconnect failed, ret:0x%08x", ret);
    return CM_SUCCESS;
}

uint32_t SleAccessReadRemoteFeatures(uint16_t handle)
{
    DLI_ConnHandleStru param;
    param.connHandle = handle;
    uint32_t ret = DLI_ReadRemoteFeatures(&param);
    CM_CHECK_RETURN_RET((ret == DLI_SUCCESS), CM_FAIL, "DLI_ReadRemoteFeatures failed, ret:0x%08x", ret);
    return CM_SUCCESS;
}

uint32_t SleAccessReadRemoteVersion(uint16_t handle)
{
    DLI_ConnHandleStru param;
    param.connHandle = handle;
    uint32_t ret = DLI_ReadRemoteVersion(&param);
    CM_CHECK_RETURN_RET((ret == DLI_SUCCESS), CM_FAIL, "DLI_ReadRemoteVersion failed, ret:0x%08x", ret);
    return CM_SUCCESS;
}

uint32_t SleAccessSetPhy(DLI_SetPhyParam *param)
{
    uint32_t ret = DLI_SetPhy(param);
    CM_CHECK_RETURN_RET((ret == DLI_SUCCESS), CM_FAIL, "DLI_SetPhy failed, ret:0x%08x", ret);
    return CM_SUCCESS;
}
