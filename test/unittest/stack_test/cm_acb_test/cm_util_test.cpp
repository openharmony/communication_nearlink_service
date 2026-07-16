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

#include "cm_util_test.h"
#include <vector>
#include <algorithm>
#include "gtest/gtest.h"
#include "cm_dli_mocker.h"
#include "cm_def.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "cm_util.h"
#include "dli_event_struct.h"
#include "byte_codec.h"

typedef struct {
    uint16_t  lcid;         /* 星闪逻辑链路handle */
    SLE_Addr_S addr;        /* 星闪设备地址 */
} CM_LogicLinkKeyObject_S;

static uint8_t g_testConnectState = 0;
static uint8_t g_testNodeRole = CM_T_NODE;
static uint8_t g_testadvHandle = UT_CM_CONN_ADV_HANDLE;
static uint8_t g_testDiscReason = 0;
static uint16_t g_testLcid = 0;
static uint16_t g_testDtapLcid = 0;
static std::vector<std::pair<CM_LogicLinkKeyObject_S, CM_LogicLinkState_S>> g_testConnectionRspParamList;
static std::vector<CM_LogicLinkState_S> g_testDtapLogicLRspParamList;
static CM_AcbSubrateCbParam_S g_testAcbSubrateCbParam = {};

// CM模块开放固定传输通道列表
uint8_t g_testFixedTransChannel[] = {
    CM_TCID_SLE_CMTC,
    CM_TCID_SLE_SMTC,
    CM_TCID_SLE_CUTC,
};

uint32_t UT_CM_GetTestFixedTransChannelSize(void)
{
    return sizeof(g_testFixedTransChannel) / sizeof(g_testFixedTransChannel[0]);
}

void UT_CM_TestNormalReset(void)
{
    g_testDiscReason = 0;
    g_testNodeRole = CM_T_NODE; // 恢复为T角色
}

void UT_CM_SetNodeRole(uint8_t role)
{
    g_testNodeRole = role;
    CM_LOGI("UT_CM_SetNodeRole role:%hhu", role);
}

void UT_CM_GenDifferentAddress(SLE_Addr_S *addr, uint16_t i)
{
    addr->addr[0] = (uint16_t)i; // different
}

void UT_CM_MockDliCmdExecuteCbk(DLI_ExecuteCmdRetParam *cmdParam, uint8_t status)
{
    if (cmdParam == NULL) {
        return;
    }
    uint16_t cmdOpcode = cmdParam->cmdOpcode;
    if (cmdOpcode == DLI_CBK_DISCONNECT || cmdOpcode == DLI_CBK_CONNECT) {
        for (size_t i = 0; i < DLI_GetDisconnectCbkSize(); i++) {
            if (DLI_GetDisconectCbk()[i].opcode == cmdOpcode && DLI_GetDisconectCbk()[i].func != NULL) {
                DLI_ExecuteCmdRetParam cmdRes = { 0 };
                cmdRes.cmdOpcode = cmdOpcode;
                cmdRes.eventParameter = cmdParam->eventParameter;
                cmdRes.size = cmdParam->size;
                uint16_t localIndex = CM_CONNECT_LOCAL_INDEX_0;
                uint8_t version = CM_CONNECT_VERSION_1_0;
                DLI_ConnCbkContext cbkContext = {0};
                cbkContext.versionAndLocalIndex = CM_PackVersionLocalIndex(version, localIndex);
                CM_LOGI("UT_CM_MockDliCmdExecuteCbk, cmdOpcode:0x%04x, size:%u, versionLocalIndex:0x%04x",
                    cmdOpcode, cmdParam->size, cbkContext.versionAndLocalIndex);
                DLI_GetDisconectCbk()[i].func(&cbkContext, status, &cmdRes);
            }
        }
        return;
    }
    for (size_t i = 0; i < DLI_GetCbkSize(); i++) {
        if (DLI_GetCbk()[i].opcode == cmdOpcode) {
            if (DLI_GetCbk()[i].func != NULL) {
                DLI_ExecuteCmdRetParam cmdRes = { 0 };
                cmdRes.cmdOpcode = cmdOpcode;
                cmdRes.eventParameter = cmdParam->eventParameter;
                cmdRes.size = cmdParam->size;
                uint16_t localIndex = CM_CONNECT_LOCAL_INDEX_0;
                uint8_t version = CM_CONNECT_VERSION_1_0;
                DLI_ConnCbkContext cbkContext = {0};
                cbkContext.versionAndLocalIndex = CM_PackVersionLocalIndex(version, localIndex);
                CM_LOGI("UT_CM_MockDliCmdExecuteCbk, cmdOpcode:0x%04x, size:%u, versionLocalIndex:0x%04x",
                    cmdOpcode, cmdParam->size, cbkContext.versionAndLocalIndex);
                DLI_GetCbk()[i].func(&cbkContext, status, &cmdRes);
                break;
            }
        }
    }
}

void UT_CM_SleConnectCompleteEvt(uint16_t handle, uint8_t status, uint8_t connCompleteType)
{
    DLI_ConnectionCompleteEvt connCompleteEvt = { 0 };
    connCompleteEvt.connHandle = handle;
    connCompleteEvt.role = g_testNodeRole;
    connCompleteEvt.status = status;
    connCompleteEvt.connCompleteType = connCompleteType;
    connCompleteEvt.advHandle = g_testadvHandle;
    SLE_Addr_S addr = { 0 };
    UT_CM_GenDifferentAddress(&addr, handle);
    (void)memcpy_s(connCompleteEvt.peerAddress, SLE_ADDR_LEN, addr.addr, SLE_ADDR_LEN);
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_CONNECT,
        .size = sizeof(connCompleteEvt),
        .eventParameter = &connCompleteEvt,
    };
    g_testConnectState = CM_LINK_STATE_CONNECTED;
    CM_LOGI("UT_CM_SleConnectCompleteEvt:0x%04x, connCompleteType:%d, advHandle:0x%02x",
        handle, connCompleteEvt.connCompleteType, connCompleteEvt.advHandle);
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}

void UT_CM_SleDisconnectCompleteEvt(uint16_t handle, uint8_t status)
{
    CM_LOGI("UT_CM_SleDisconnectCompleteEvt:%hu", handle);
    DLI_DisconnectEvt connCompleteEvt = { 0 };
    connCompleteEvt.status = status;
    connCompleteEvt.connHandle = handle;
    connCompleteEvt.reason = 0;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_DISCONNECT,
        .size = sizeof(connCompleteEvt),
        .eventParameter = &connCompleteEvt,
    };
    g_testConnectState = CM_LINK_STATE_CONNECTED;
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}

void UT_CM_SleConnectCompleteCommandErrorEvt(uint16_t handle, uint8_t status)
{
    CM_LOGI("UT_CM_SleConnectCompleteCommandErrorEvt, handle:%hu", handle);
    DLI_CommandErrorStru connCompleteEvt = { 0 };
    connCompleteEvt.cmd = DLI_CREATE_CONNECTION;
    connCompleteEvt.status = status;
    connCompleteEvt.req[0] = 0;
    // 此时角色默认为0, 属无效值
    g_testNodeRole = 0;
    g_testDiscReason = connCompleteEvt.status;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_CONNECT,
        .size = sizeof(connCompleteEvt),
        .eventParameter = &connCompleteEvt,
    };
    g_testConnectState = CM_LINK_STATE_DISCONNECTED;
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, connCompleteEvt.status);

    if (status == DLI_COMMAND_TIMEOUT) {
        // 取消连接回调后，还有连接状态失败回调
        DLI_ConnectionCompleteEvt connCompleteEvt = { 0 };
        connCompleteEvt.connHandle = CM_INVALID_LCID;
        connCompleteEvt.role = g_testNodeRole;
        connCompleteEvt.status = DLI_CONNECTION_TIMEOUT;
        SLE_Addr_S addr = { 0 };
        UT_CM_GenDifferentAddress(&addr, handle);
        (void)memcpy_s(connCompleteEvt.peerAddress, SLE_ADDR_LEN, addr.addr, SLE_ADDR_LEN);
        DLI_ExecuteCmdRetParam cmdParam = {
            .cmdOpcode = DLI_CBK_CONNECT,
            .size = sizeof(connCompleteEvt),
            .eventParameter = &connCompleteEvt,
        };
        g_testDiscReason = DLI_CONNECTION_TIMEOUT;
        g_testConnectState = CM_LINK_STATE_DISCONNECTED;
        UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
    }
}

void UT_CM_SleDisconnectEvt(uint16_t handle, uint8_t reason)
{
    DLI_DisconnectEvt disconnectEvt = { 0 };
    disconnectEvt.connHandle = handle;
    disconnectEvt.reason = reason;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_DISCONNECT,
        .size = sizeof(disconnectEvt),
        .eventParameter = &disconnectEvt,
    };
    g_testConnectState = CM_LINK_STATE_DISCONNECTED;
    g_testDiscReason = reason;
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}

static bool UT_CM_LogicLinkDtapIsSame(const CM_LogicLinkState_S& p)
{
    if ((p.lcid == g_testDtapLcid)) {
        return true;
    }
    return false;
}

void UT_CM_ServiceConnectStateCbk(CM_LogicLinkState_S *param)
{
    CM_LOGI("CM_API UT_CM_ServiceConnectStateCbk enter, lcid:0x%02x, result:%u(0: connected, 1:connecting, "
        "2:disconnected, 3: disconnecting), discReason:0x%04x, role:0x%02x, advHandle:0x%02x",
        param->lcid, param->result, param->discReason, param->role, param->advHandle);
    if (g_testDiscReason == DLI_COMMAND_TIMEOUT) {
        // 连接命令超时，错误码为连接超时
        EXPECT_EQ(param->discReason, DLI_CONNECTION_TIMEOUT);
    }
    g_testLcid = param->lcid;
    if (param->lcid != CM_INVALID_LCID) {
        if (param->result == CM_LINK_STATE_CONNECTED) {
            CM_LOGI("UT_CM_ServiceConnectStateCbk connect list add a lcid:0x%02x", param->lcid);
            CM_LogicLinkKeyObject_S key = {};
            key.lcid = param->lcid;
            (void)memcpy_s(&key.addr, sizeof(key.addr), &param->addr, sizeof(key.addr));
            g_testConnectionRspParamList.push_back(std::make_pair(key, *param));
        } else if (param->result == CM_LINK_STATE_DISCONNECTED) {
            CM_LOGI("UT_CM_ServiceConnectStateCbk connect list remove a lcid:0x%02x", param->lcid);
            uint16_t lcid = param->lcid;
            g_testConnectionRspParamList.erase(std::remove_if(g_testConnectionRspParamList.begin(),
                g_testConnectionRspParamList.end(), [lcid](const auto &param) -> bool {
                return param.first.lcid == lcid;
            }), g_testConnectionRspParamList.end());
        }
    }
    CM_LOGI("ConnectionRspParamList size:%zu", g_testConnectionRspParamList.size());
}

void UT_CM_SetTestTcid(uint16_t lcid)
{
    g_testLcid = lcid;
}

uint16_t UT_CM_GetTestTcid(void)
{
    return g_testLcid;
}

CM_LogicLinkState_S UT_CM_GetLogicLinkState(uint16_t lcid)
{
    auto it = std::find_if(g_testConnectionRspParamList.begin(),
        g_testConnectionRspParamList.end(), [lcid](const auto &param) -> bool {
        return param.first.lcid == lcid;
    });
    if (it == g_testConnectionRspParamList.end()) {
        CM_LOGW("logic link not is exist, lcid:0x%04x", lcid);
        return {};
    }
    return it->second;
}

void UT_CM_DtapConnectStateCbk(CM_LogicLinkState_S *param)
{
    CM_LOGI("CM_API UT_CM_DtapConnectStateCbk enter, lcid:0x%02x, result:%u(0: connected, 1:connecting, "
        "2:disconnected, 3: disconnecting, 4: connected_ack), discReason:0x%04x, role:0x%02x",
        param->lcid, param->result, param->discReason, param->role);
    g_testDtapLcid = param->lcid;
    if (param->lcid != CM_INVALID_LCID) {
        if (param->result == CM_LINK_STATE_CONNECTED) {
            CM_LOGI("UT_CM_DtapConnectStateCbk connect list add a lcid:0x%02x", param->lcid);
            g_testDtapLogicLRspParamList.push_back(*param);
        } else if (param->result == CM_LINK_STATE_DISCONNECTED) {
            CM_LOGI("UT_CM_DtapConnectStateCbk connect list remove a lcid:0x%02x", param->lcid);
            g_testDtapLogicLRspParamList.erase(std::remove_if(g_testDtapLogicLRspParamList.begin(),
                g_testDtapLogicLRspParamList.end(), UT_CM_LogicLinkDtapIsSame), g_testDtapLogicLRspParamList.end());
        }
    }
    CM_LOGI("DtapConnectionRspParamList size:%zu", g_testDtapLogicLRspParamList.size());
}

void UT_CM_ReadRemoteFeatureCbk(CM_LogicLinkRemoteFeatures_S *param)
{
    CM_LOGI("CM_API UT_CM_ReadRemoteFeatureCbk enter, lcid:0x%02x", param->lcid);
    EXPECT_EQ(g_testLcid, param->lcid);
}

void UT_CM_ConnUpdateParamCbk(CM_LogicLinkConnUpdateParam_S *param)
{
    CM_LOGI("CM_API UT_CM_ConnUpdateParamCbk enter, lcid:0x%02x, result:%u", param->lcid, param->result);
    EXPECT_EQ(g_testLcid, param->lcid);
}

size_t UT_CM_GetTestConnectListSize(void)
{
    return g_testConnectionRspParamList.size();
}

size_t UT_CM_GetDtapTestConnectListSize(void)
{
    return g_testDtapLogicLRspParamList.size();
}

CM_LogicLinkState_S UT_CM_GetDtapTestConnectListFirst(void)
{
    CM_LogicLinkState_S state = {};
    if (g_testDtapLogicLRspParamList.size() == 0) {
        return state;
    } else {
        return g_testDtapLogicLRspParamList[0];
    }
}

void UT_CM_GetTestConnectListClear(void)
{
    g_testConnectionRspParamList.clear();
    g_testDtapLogicLRspParamList.clear();
}

void UT_CM_CancelConnectResult(uint8_t result)
{
}

uint8_t UT_CM_GetTestNodeRole(void)
{
    return g_testNodeRole;
}

void UT_CM_SleCancelConnectEvt(void)
{
    uint8_t result = 0;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_CONNECT_CANCEL,
        .size = sizeof(uint8_t),
        .eventParameter = &result,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}

void UT_CM_SleConnectReadRemoteVersion(uint16_t handle)
{
    DLI_ReadRemoteVersionEvt evt = { 0 };
    evt.connHandle = handle;
    evt.companyIdentifier = UT_CM_COMPANY_ID;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_READ_REMOTE_VERSION,
        .size = sizeof(evt),
        .eventParameter = &evt,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}

void UT_CM_SsapConnectReadRemoteFetureVersionCbk(CM_ReadRemoteFeatureVersionRsp_S *param)
{
    CM_LOGI("UT_CM_SsapConnectReadRemoteFetureVersionCbk enter, lcid:0x%02x, version:0x%02x, companyId:0x%04x, "
        "subversion:0x%04x", param->lcid, param->version, param->companyId, param->subversion);
    EXPECT_EQ(param->companyId, UT_CM_COMPANY_ID);
    EXPECT_EQ(param->features[0], UT_CM_REMOTE_FEATURES);
}

void UT_CM_SsapConnectUpdatePramCbk(CM_ConnectUpdateParamRsp_S *param)
{
    CM_LOGI("UT_CM_SsapConnectUpdatePramCbk enter, lcid:0x%02x, localIndex:0x%04x, version:0x%02x",
        param->lcid, param->localIndex, param->version);
    EXPECT_EQ(param->localIndex, CM_CONNECT_LOCAL_INDEX_0);
    EXPECT_EQ(param->version, CM_CONNECT_VERSION_1_0);
}

void UT_CM_SsapLogicLinkUpdatePramCbk(CM_LogicLinkConnUpdateParam_S *param)
{
    CM_LOGI("UT_CM_SsapLogicLinkUpdatePramCbk enter, lcid:0x%02x, result:0x%02x",
        param->lcid, param->result);
}

void UT_CM_ConnectRemoteUpdateParamReqCbk(CM_ConnectRemoteUpdateParamReq_S *param)
{
    CM_LOGI("UT_CM_ConnectRemoteUpdateParamReqCbk enter, lcid:0x%02x, intervalMin:0x%02x, intervalMax:0x%02x, "
        "maxLatency:0x%02x, supervisionTimeout:0x%02x",
        param->lcid, param->intervalMin, param->intervalMax, param->maxLatency, param->supervisionTimeout);
    EXPECT_EQ(param->intervalMin, UT_CM_CONN_INTERAL);
    EXPECT_EQ(param->intervalMax, UT_CM_CONN_INTERAL);
    EXPECT_EQ(param->maxLatency, UT_CM_CONN_MAX_LATENCY);
    EXPECT_EQ(param->supervisionTimeout, UT_CM_CONN_SUPERVISION_TIMEOUT);
}

void UT_CM_SetPhyCbk(CM_SetPhyRsp_S *param)
{
    CM_LOGI("CM_SetPhyRsp_S enter, lcid:0x%02x, txFormat:0x%02x", param->lcid, param->txFormat);
    EXPECT_EQ(param->txFormat, UT_CM_CONN_TX_FORMAT);
}

void UT_CM_SetAcbSubrateCbk(CM_AcbSubrateCbParam_S *param)
{
    CM_LOGI("CM_SetAcbSubrateRsp_S enter, addr:%s, subrate:0x%02x", GET_ENC_ADDR(&param->addr), param->subrate);
    (void)memcpy_s(&g_testAcbSubrateCbParam, sizeof(CM_AcbSubrateCbParam_S), param, sizeof(CM_AcbSubrateCbParam_S));
}

void UT_CM_ReqAcbSubrateCbk(CM_AcbSubrateCbParam_S *param)
{
    CM_LOGI("UT_CM_ReqAcbSubrateCbk enter, addr:%s", GET_ENC_ADDR(&param->addr));
    (void)memcpy_s(&g_testAcbSubrateCbParam, sizeof(CM_AcbSubrateCbParam_S), param, sizeof(CM_AcbSubrateCbParam_S));
}

void UT_CM_SetRxDataFilterCbk(CM_SetRxDataFilterRsp_S *param)
{
    CM_LOGI("CM_SetAcbSubrateRsp_S enter, lcid:0x%04x", param->lcid);
}

CM_AcbSubrateCbParam_S UT_CM_GetAcbSubrateCbkParam(void)
{
    return g_testAcbSubrateCbParam;
}

void UT_CM_SleConnectReadFeatures(uint16_t handle)
{
    DLI_ReadRemoteFeatsEvt evt = { 0 };
    evt.connHandle = handle;
    evt.feats[0] = UT_CM_REMOTE_FEATURES;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_READ_REMOTE_FEATURE,
        .size = sizeof(evt),
        .eventParameter = &evt,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}

void UT_CM_SleConnectSetDataLen(uint16_t handle)
{
    DLI_DataLenChangeEvt evt = { 0 };
    evt.connHandle = handle;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_SET_DATA_LEN,
        .size = sizeof(evt),
        .eventParameter = &evt,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);

    DLI_DataLenChangeEvt cevt = { 0 };
    cevt.connHandle = handle;
    DLI_ExecuteCmdRetParam cCmdParam = {
        .cmdOpcode = DLI_CBK_DATA_LEN_CHANGE,
        .size = sizeof(cevt),
        .eventParameter = &cevt,
    };
    UT_CM_MockDliCmdExecuteCbk(&cCmdParam, DLI_SUCCESS);
}

void UT_CM_SleConnectReadLocalFeature(void)
{
    DLI_ReadLocalFeatsEvt evt = { 0 };
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_READ_LOCAL_FEATURE,
        .size = sizeof(evt),
        .eventParameter = &evt,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}

void UT_CM_SleSetAcbSubrateCbkEvt(uint16_t handle, CM_SetACBSubrateParam *param)
{
    DLI_AcbSetSubrateEvt evt = { 0 };
    evt.lcid = handle;
    evt.status = DLI_SUCCESS;
    ENCODE2BYTE_LITTLE(&evt.subrateFactor, param->subrate);
    ENCODE2BYTE_LITTLE(&evt.peripheralLatency, param->maxLatency);
    ENCODE2BYTE_LITTLE(&evt.continuationNum, param->continuationNum);
    ENCODE2BYTE_LITTLE(&evt.supervisionTimeout, param->supervisionTimeout);
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_ACB_SET_SUBRATE,
        .size = sizeof(evt),
        .eventParameter = &evt,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}

void UT_CM_SleReqAcbSubrateCbkEvt(uint16_t handle, CM_SetACBSubrateParam *param)
{
    DLI_AcbReqSubrateEvt evt = { 0 };
    evt.lcid = handle;
    evt.status = DLI_SUCCESS;
    ENCODE2BYTE_LITTLE(&evt.subrateMin, param->subrate);
    ENCODE2BYTE_LITTLE(&evt.subrateMax, param->subrate);
    ENCODE2BYTE_LITTLE(&evt.peripheralLatency, param->maxLatency);
    ENCODE2BYTE_LITTLE(&evt.continuationNum, param->continuationNum);
    ENCODE2BYTE_LITTLE(&evt.supervisionTimeout, param->supervisionTimeout);

    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_ACB_REQ_SUBRATE,
        .size = sizeof(evt),
        .eventParameter = &evt,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}

void UT_CM_SleFreqBandSwitchCbkEvt(uint16_t handle, CM_FreqBandSwitchParam *param)
{
    DLI_FreqBandSwitchEvt evt = { 0 };
    evt.connHandle = handle;
    evt.status = DLI_SUCCESS;

    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_SWITCH_FREQ_BAND,
        .size = sizeof(evt),
        .eventParameter = &evt,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}

void UT_CM_SetPowerModeCbkEvt(uint16_t handle)
{
    CM_TestDLI_UndefinedEvt evt = { 0 };
    evt.status = DLI_SUCCESS;

    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_SET_POWER_MODE,
        .size = sizeof(evt),
        .eventParameter = &evt,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
}