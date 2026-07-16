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

#include "cm_event_core.h"
#include <stddef.h>
#include "securec.h"
#include "sdf_mem.h"
#include "cm_errno.h"
#include "dli_errno.h"
#include "cm_log.h"
#include "cm_api.h"
#include "cm_access_mgr.h"
#include "cm_exter_cbks_mgr.h"
#include "cm_common.h"
#include "cm_util.h"
#include "cm_trans_channel_mgr.h"
#include "cm_logic_link_listener_mgr.h"
#include "sle_logic_link_mgr.h"
#include "sle_access_dli.h"
#include "cm_concurrent_conn.h"

void CM_ReportLogicLinkCbks(CM_ConnectParamRsp_S *connectRsp)
{
    if (connectRsp->result == CM_LINK_STATE_CONNECTED || connectRsp->result == CM_LINK_STATE_DISCONNECTED) {
        CM_LOGI("enter");
        CM_LogicLinkState_S param = { 0 };
        param.lcid = connectRsp->lcid;
        param.role = connectRsp->role;
        (void)memcpy_s(&param.addr, sizeof(SLE_Addr_S), &connectRsp->addr, sizeof(SLE_Addr_S));
        param.result = connectRsp->result;
        param.discReason = connectRsp->discReason;
        param.connCompleteType = connectRsp->connCompleteType;
        param.advHandle = connectRsp->advHandle;
        CM_NotifyLogicLinkCbks(&param);
    }
}

static void CM_ConnectionProcInterruptReq(SleLogicLink_S *link, uint8_t version, uint16_t localIndex)
{
    // 基础连接完成后，若流程异常，则发起释放连接请求
    CM_AccessParamReq_S accessParam = { 0 };
    link->status = CM_LINK_STATE_DISCONNECTTING;
    accessParam.lcid = link->lcid;
    accessParam.version = version;
    accessParam.localIndex = localIndex;
    CM_LOGI("exception, release link, lcid:0x%04x, version:0x%02x, localIndex:0x%04x", link->lcid, version, localIndex);
    SleAccessLinkReleaseReq(&accessParam);
}

static void CM_SleCreateConnectionProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    uint32_t ret = 0;
    CM_ConnectParamRsp_S *connectRsp;

    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle create connect param invalid");

    connectRsp = (CM_ConnectParamRsp_S *)par->eventParameter;
    CM_LOGI("sle connect create connection proc enter, result = 0x%02x, lcid=0x%04x, addr:%s, addrType:%hhu, "
        "expectSize:%u, realSize:%u", result, connectRsp->lcid, GET_ENC_ADDR(&connectRsp->addr),
        connectRsp->addr.type, sizeof(CM_ConnectParamRsp_S), par->size);
    SleLogicLink_S *link = SleLogicLinkGetByAddr(&connectRsp->addr);
    if (link == NULL) {
        CM_LOGE("link is not exists");
        return;
    }
    CM_LOGI("sle connect create connection proc, link status:%d", link->status);
    uint8_t connCompleteType = link->connCompleteType; // 先保存临时副本，后续link可能会被释放
    if (link->status == CM_LINK_STATE_CONNECTED) {
        connectRsp->result = link->status;
        // 获取对端重要字段信息companyId，用于连接管理查询能力等功能使用，然后在其回调里再读取对端Feature
        ret = SleAccessReadRemoteVersion(connectRsp->lcid);
        if (ret != CM_SUCCESS) {
            CM_LOGE("sle read remote version failed");
            CM_ConnectionProcInterruptReq(link, connectRsp->version, connectRsp->localIndex);
            return;
        }
        link->role = connectRsp->role;
        // 完成芯片连接后，立即通知DTAP模块，创建固定传输通道，用于接收数据并缓存或者立即上报
        // 若缓存，则需要通过CM_NotifyLogicLinkDtapCbks通知连接确认上报再上报给其他模块，比如SSAP
        ret = CM_ActivateFixedTransChannel(connectRsp->lcid, NULL);
        if (ret != CM_SUCCESS) {
            CM_LOGE("activate fixed trans channel failed");
            CM_ConnectionProcInterruptReq(link, connectRsp->version, connectRsp->localIndex);
            return;
        }
    } else {
        connectRsp->result = CM_LINK_STATE_DISCONNECTED;
        SleLogicLinkRemove(link);
        // 连接取消时，此处可不通知空地址，由调用方自行管理通知
        if (!CM_IsEmptyAddr(&connectRsp->addr)) {
            CM_ReportLogicLinkCbks(connectRsp);
        }
    }
    if (connCompleteType == CM_CONN_COMPLETE_ADV) {
        // 双端同时发起建链场景：若在被动连接完成时，逻辑链路列表可能存在主动连接中的节点，需要移除，后续可重新下发
        link = SleLogicLinkGetByStatus(CM_LINK_STATE_CONNECTING);
        if (link != NULL) {
            SleLogicLinkRemove(link);
        }
    }
    CM_ConcurrentConnDoingComplete(&connectRsp->addr, connectRsp->result);
}

static void CM_SleCancelConnectionProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("sle cancel connect proc enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle cancel connect param invalid");
    uint8_t *param = (uint8_t *)par->eventParameter;
    if (SleLogicLinkGetByStatus(CM_LINK_STATE_CONNECTING) == NULL) {
        // 没有连接中的节点，下发取消连接指令后，回调结果设置为非0
        *param = DLI_UNKNOWN_CONNECTION_IDENTIFIER;
    }
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_CONNECT_CANCEL, param);
}

static void CM_SleDisconnectProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle disconnect param invalid");

    CM_ConnectParamRsp_S *connectRsp = (CM_ConnectParamRsp_S *)par->eventParameter;
    CM_LOGI("sle connect disconnect proc enter, result = 0x%02x, lcid=0x%04x, addr:%s, addrType:%hhu, "
        "expectSize:%u, realSize:%u", result, connectRsp->lcid, GetEncryptAddr(&connectRsp->addr).buf,
        connectRsp->addr.type, sizeof(CM_ConnectParamRsp_S), par->size);
    if (connectRsp->lcid == CM_INVALID_LCID) {
        // 异常防护: 对于无效的lcid，需要提前拦截
        CM_LOGW("lcid is not valid, ignore it");
        return;
    }
    SleLogicLink_S *link = SleLogicLinkGetByLcid(connectRsp->lcid);
    if (link == NULL) {
        CM_LOGE("link is not exists");
        return;
    }
    connectRsp->result = CM_LINK_STATE_DISCONNECTED;
    (void)memcpy_s(&connectRsp->addr, sizeof(SLE_Addr_S), &link->rmtAddr, sizeof(SLE_Addr_S));
    CM_LOGI("sle connect disconnect proc, link status:%d", link->status);
    if (link->status != CM_LINK_STATE_DISCONNECTED) {
        if (link->status == CM_LINK_STATE_CONNECTED || link->status == CM_LINK_STATE_DISCONNECTTING) {
            CM_ReleaseFixedTransChannel(connectRsp->lcid);
        }
        SleLogicLinkRemove(link);
    } else {
        link->status = CM_LINK_STATE_DISCONNECTED;
    }
    CM_ReportLogicLinkCbks(connectRsp);
}

static void CM_SleConnectUpdateProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("sle connnect update proc enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle update param invalid");

    CM_ConnectUpdateParamRsp_S *connectRsp = (CM_ConnectUpdateParamRsp_S *)par->eventParameter;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_CONNECT_PARAM_UPDATE, connectRsp);
    CM_LogicLinkConnUpdateParam_S updateParamRsp = { 0 };
    updateParamRsp.result = connectRsp->result;
    updateParamRsp.lcid = connectRsp->lcid;
    (void)memcpy_s(&updateParamRsp.addr, sizeof(SLE_Addr_S), &connectRsp->addr, sizeof(SLE_Addr_S));
    CM_ExecLogicLinkConnUpdateParamCbks(&updateParamRsp);
}

static void CM_SleConnectRemoteParamUpdateReqProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("sle connnect remote param update req proc enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle update param invalid");

    CM_ConnectRemoteUpdateParamReq_S *connectRsp = (CM_ConnectRemoteUpdateParamReq_S *)par->eventParameter;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_CONNECT_REMOTE_UPDATE_PARAM_REQ, connectRsp);
}

static void CM_SleSetPhyProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("sle set phy proc enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle set phy param invalid");

    CM_SetPhyRsp_S *setPhyRsp = (CM_SetPhyRsp_S *)par->eventParameter;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_SET_PHY, setPhyRsp);
}

static void CM_SleReadRemoteVersionProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("sle read remote version proc enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle read remote feature param invalid");
    // 走到此处，连接已经成功，需要上报连接状态给其他各个监听模块
    CM_ReadRemoteVersionRsp_S *readRsp = (CM_ReadRemoteVersionRsp_S *)par->eventParameter;
    CM_LogicLinkState_S rsp = { 0 };
    rsp.lcid = readRsp->lcid;
    rsp.role = readRsp->role;
    rsp.result = CM_LINK_STATE_CONNECTED;
    rsp.discReason = 0;
    (void)memcpy_s(&rsp.addr, sizeof(SLE_Addr_S), &readRsp->addr, sizeof(SLE_Addr_S));
    SleLogicLink_S *link = SleLogicLinkGetByLcid(rsp.lcid);
    CM_CHECK_RETURN((link != NULL), "logic link is not exist, lcid:0x%04x", rsp.lcid);
    rsp.connCompleteType = link->connCompleteType;
    rsp.advHandle = link->advHandle;
    CM_NotifyLogicLinkCbks(&rsp);
}

static void CM_SleReadRemoteFeatureProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("sle read remote feature proc enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle read remote feature param invalid");
    CM_ReadRemoteFeatureVersionRsp_S *readRsp = (CM_ReadRemoteFeatureVersionRsp_S *)par->eventParameter;
    CM_LogicLinkRemoteFeatures_S remoteParam = { 0 };
    remoteParam.lcid = readRsp->lcid;
    (void)memcpy_s(&remoteParam.addr, sizeof(SLE_Addr_S), &readRsp->addr, sizeof(SLE_Addr_S));
    (void)memcpy_s(remoteParam.features, sizeof(remoteParam.features), readRsp->features, sizeof(readRsp->features));
    CM_LOGI("remoteParam->addr = %s", GET_ENC_ADDR(&remoteParam.addr));
    CM_ExecLogicLinkRemoteFeaturesCbks(&remoteParam);
}

static void CM_SleReadLocalFeatureProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("sle read local feature proc enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle read local feature param invalid");
    CM_ReadLocalFeatureRsp_S *param = (CM_ReadLocalFeatureRsp_S *)par->eventParameter;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_READ_LOCAL_FEATURE, param);
}

static void CM_SleSetDataLenProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("sle set data len proc enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle set data len param invalid");
    if (result != CM_SUCCESS) {
        // SetDataLen 属于整个连接流程中的一个内部流程，连接异常时，需要断开连接
        CM_ConnectParamRsp_S *connectRsp = (CM_ConnectParamRsp_S *)par->eventParameter;
        if (connectRsp->lcid == CM_INVALID_LCID) {
            // 对于无效的lcid，下发给芯片不起作用，需要提前拦截
            CM_LOGE("lcid is not valid, ignore the release link req");
            return;
        }
        SleLogicLink_S *link = SleLogicLinkGetByLcid(connectRsp->lcid);
        if (link == NULL) {
            CM_LOGE("set data len proc, link is not exists");
            return;
        }
        CM_ConnectionProcInterruptReq(link, connectRsp->version, connectRsp->localIndex);
        return;
    }
}

static void CM_SleReadRemoteFeatureAndVersionProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("sle read remote feature remote version enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle read remote feature param invalid");
    CM_ReadRemoteFeatureVersionRsp_S *readRsp = (CM_ReadRemoteFeatureVersionRsp_S *)par->eventParameter;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_READ_REMOTE_FEATURE_VERSION, readRsp);
    if (result != CM_SUCCESS) {
        // ReadRemoteVersion 属于整个连接流程中的一个内部流程，连接异常时，需要断开连接
        if (readRsp->lcid == CM_INVALID_LCID) {
            // 对于无效的lcid，下发给芯片不起作用，需要提前拦截
            CM_LOGE("lcid is not valid, ignore the release link req");
            return;
        }
        SleLogicLink_S *link = SleLogicLinkGetByLcid(readRsp->lcid);
        if (link == NULL) {
            CM_LOGE("read remote feature and version, link is not exists");
            return;
        }
        CM_ConnectionProcInterruptReq(link, CM_CONNECT_VERSION_1_0, CM_CONNECT_LOCAL_INDEX_0);
        return;
    }
}

static void CM_SleEnableConnHighPowerProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGD("sle enable conn high power proc enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle enable conn high power param invalid");
    CM_EnableConnHighPowerRsp_S *param = (CM_EnableConnHighPowerRsp_S *)par->eventParameter;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_ENABLE_CONN_HIGH_POWER, param);
}

static void CM_SleSetPeerDevTypeProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "param is invalid");
    CM_SetPeerDevType_S *param = (CM_SetPeerDevType_S *)par->eventParameter;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_SET_PEER_DEV_TYPE, param);
}

static void CM_SleSetRxDataFilterProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("sle set data filter proc enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle set data filter param invalid");

    CM_SetRxDataFilterRsp_S *setRxDataFilterRsp = (CM_SetRxDataFilterRsp_S *)par->eventParameter;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_SET_RX_DATA_FILTER, setRxDataFilterRsp);
}

static void CM_SleConnectExpectionProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("sle connect expection proc enter, result = 0x%02x", result);
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "sle connect expection param invalid");
    CM_ConnectExceptionRsp_S *connectRsp = (CM_ConnectExceptionRsp_S *)par->eventParameter;
    if (connectRsp->lcid == CM_INVALID_LCID) {
        // 对于无效的lcid，下发给芯片不起作用，需要提前拦截
        CM_LOGE("lcid is not valid, ignore the release link req");
        return;
    }
    SleLogicLink_S *link = SleLogicLinkGetByLcid(connectRsp->lcid);
    if (link == NULL) {
        CM_LOGE("expection proc, link is not exists");
        return;
    }
    CM_ConnectionProcInterruptReq(link, CM_CONNECT_VERSION_1_0, CM_CONNECT_LOCAL_INDEX_0);
}

static void CM_SleSubrateCbkProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "CM_SleSubrateCbkProc param invalid");
    CM_AcbSubrateCbParam_S *param = (CM_AcbSubrateCbParam_S *)par->eventParameter;
    CM_LOGD("result %u, notify subrate to %u", result, param->subrate);
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_SET_SUBRATE, (void *)param);
}

static void CM_SleReqSubrateCbkProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "CM_SleReqSubrateCbkProc param invalid");
    CM_AcbSubrateCbParam_S *param = (CM_AcbSubrateCbParam_S *)par->eventParameter;
    CM_LOGI("result %u, req subrate to %u", result, param->subrate);
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_REQ_SUBRATE, (void *)param);
}

static void CM_SleReadAcceptFilterListSizeProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "param is invalid");
    CM_ReadAcceptFilterListSize_S *param = (CM_ReadAcceptFilterListSize_S *)par->eventParameter;
    CM_LOGI("Read accept filter list size result=%u, size=%hhu", result, param->listSize);
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_READ_ACCEPT_FLT_LIST_SIZE, (void *)param);
}

static void CM_SleReadRemoteRssiCbkProc(void *context, uint8_t result, const CM_ExecuteCmdPar_S *par)
{
    CM_LOGI("CM_SleReadRemoteRssiCbkProc");
    CM_CHECK_RETURN((par != NULL && par->eventParameter != NULL), "CM_SleReadRemoteRssiCbkProc param invalid");
    CM_ReadRemoteRssiRsp_S *param = (CM_ReadRemoteRssiRsp_S *)par->eventParameter;
    CM_ExecuteEventCbk(CM_SLE_CBK_EVENT_READ_REMOTE_RSSI, (void *)param);
}

static void CM_RegEventCbk(void)
{
    CM_AccessRegCbk(SLE_ACCESS_CBK_CONNECT, CM_SleCreateConnectionProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_CONNECT_CANCEL, CM_SleCancelConnectionProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_DISCONNECT, CM_SleDisconnectProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_CONNECT_REMOTE_UPDATE_REQ, CM_SleConnectRemoteParamUpdateReqProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_CONNECT_UPDATE, CM_SleConnectUpdateProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_SET_PHY, CM_SleSetPhyProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_SET_DATA_LEN, CM_SleSetDataLenProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_READ_REMOTE_VERSION, CM_SleReadRemoteVersionProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_READ_REMOTE_FEATURE, CM_SleReadRemoteFeatureProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_READ_REMOTE_FEATURE_AND_VERSION, CM_SleReadRemoteFeatureAndVersionProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_READ_LOCAL_FEATURE, CM_SleReadLocalFeatureProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_ENABLE_CONN_HIGH_POWER, CM_SleEnableConnHighPowerProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_SET_PEER_DEV_TYPE, CM_SleSetPeerDevTypeProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_SET_RX_DATA_FILTER, CM_SleSetRxDataFilterProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_CONNECT_EXCEPTION, CM_SleConnectExpectionProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_SET_SUBRATE, CM_SleSubrateCbkProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_REQ_SUBRATE, CM_SleReqSubrateCbkProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_READ_ACCEPT_FLT_LIST_SIZE, CM_SleReadAcceptFilterListSizeProc);
    CM_AccessRegCbk(SLE_ACCESS_CBK_READ_REMOTE_RSSI, CM_SleReadRemoteRssiCbkProc);
}

void CM_EventCoreInit(void)
{
    CM_RegEventCbk();
}

void CM_EventCoreDeInit(void)
{
    CM_AccessUnRegAllCbk();
}