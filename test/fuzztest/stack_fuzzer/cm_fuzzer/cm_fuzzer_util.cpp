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
#include "cm_fuzzer_util.h"
#include <type_traits>
#include "dli_errno.h"
#include "sdf_worker.h"
#include "cp_errno_base.h"
#include "cm_log.h"
#include "cm_def.h"
#include "cm_util.h"
#include "cm_api.h"

// PR-10 修复：原 mock 单表保存（g_cmdCbk 只留最后注册的表），CM_InitInner 顺序为
// CM_DliAdapterInit(CM_COMMON: DLI_CBK_CONNECT/DISCONNECT) -> SleAccessRegCbks(CM: 22 项命令表)，
// 连接/断开完成事件注入永远查不到 CM_COMMON 表——SleAccessConnectCbk/SleAccessDisconnectCbk
// 及下游 CM_SleCreateConnectionProc -> CM_ActivateFixedTransChannel 整链不可达（cov_round4
// sle_access_dli 424 miss / cm_event_core 185 miss / cm_trans_channel_mgr 5% 的结构性根因）。
// 真实 dli.c 语义是各模块表并存、按 opcode 广播，此处按 module 分槽对齐
static const DLI_CbkLineStru *g_cmModuleCbk = NULL;    /* CM 模块表: g_sleCmCbk(22 项) */
static uint32_t g_cmModuleCbkSize = 0;
static const DLI_CbkLineStru *g_cmCommonCbk = NULL;    /* CM_COMMON 表: g_dliCbks(CONNECT/DISCONNECT) */
static uint32_t g_cmCommonCbkSize = 0;

extern "C" uint32_t DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    CM_LOGI("DLI_CmdCbkReg enter");
    if (module != CM && module != CM_COMMON) {
        CM_LOGE("DLI_CmdCbkReg module is not CM");
        return DLI_INVALID_PARAMETERS;
    }
    if (module == CM) {
        g_cmModuleCbk = table;
        g_cmModuleCbkSize = size;
    } else {
        g_cmCommonCbk = table;
        g_cmCommonCbkSize = size;
    }
    return DLI_SUCCESS;
}

// DLI_CmdCbkUnReg（cm_dli_mocker.c 转调）：按 module 清空对应表槽，对齐真实反注册语义
extern "C" void CmFuzzerUtilClearModuleTable(ModuleType module)
{
    if (module == CM) {
        g_cmModuleCbk = NULL;
        g_cmModuleCbkSize = 0;
    } else if (module == CM_COMMON) {
        g_cmCommonCbk = NULL;
        g_cmCommonCbkSize = 0;
    }
}

static uint32_t CP_PostTaskStub(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    cb(arg);
    if (freeCb != nullptr) {
        freeCb(arg);
    }
    return CP_OK;
}

static uint32_t CP_PostTaskBlockedStub(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    (void)timeout;
    cb(arg);
    if (freeCb != nullptr) {
        freeCb(arg);
    }
    return CP_OK;
}

// 从事件参数提取 connHandle 填充 DLI_ConnCbkContext：CM 表绝大多数命令事件布局为
// status(1) + connHandle/lcid(2 小端)，取 data[1..2]；无 handle 的事件（如 READ_FLT_LIST_SIZE）返回 0
static uint16_t CmFuzzerUtilGetEvtConnHandle(DLI_ExecuteCmdRetParam *cmdParam)
{
    uint8_t *data = (uint8_t *)cmdParam->eventParameter;
    if ((data == NULL) || (cmdParam->size < 3)) {
        return 0;
    }
    return (uint16_t)((uint16_t)data[1] | ((uint16_t)data[2] << 8));
}

extern "C" uint32_t CP_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    return CP_PostTaskStub(cb, arg, freeCb);
}

extern "C" uint32_t CP_PostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    return CP_PostTaskBlockedStub(cb, arg, freeCb, timeout);
}

// 单表分发：opcode 命中即调用（两张表 opcode 无重叠，先查后查等价）。rawContext 非空时
// 直接作为 cbk context（SET_PEER_DEV_TYPE 的 context 是 SLE_Addr_S*，不走 DLI_ConnCbkContext）
static void MockDliCmdDispatchTable(const DLI_CbkLineStru *table, uint32_t tableSize, uint16_t cmdOpcode,
    uint8_t status, void *rawContext, DLI_ExecuteCmdRetParam *cmdParam)
{
    for (uint32_t i = 0; i < tableSize; i++) {
        if (table[i].opcode == cmdOpcode) {
            if (table[i].func != NULL) {
                if (rawContext != NULL) {
                    table[i].func(rawContext, status, cmdParam);
                    return;
                }
                DLI_ConnCbkContext cbkContext = {0};
                cbkContext.versionAndLocalIndex = CM_PackVersionLocalIndex(CM_CONNECT_LOCAL_INDEX_0,
                    CM_CONNECT_VERSION_1_0);
                cbkContext.connHandle = CmFuzzerUtilGetEvtConnHandle(cmdParam);
                table[i].func(&cbkContext, status, cmdParam);
                return;
            }
        }
    }
}

void MockDliCmdExecuteCbk(DLI_ExecuteCmdRetParam *cmdParam)
{
    if (cmdParam == NULL) {
        return;
    }
    uint16_t cmdOpcode = cmdParam->cmdOpcode;
    // CM_COMMON 先注册（连接/断开适配器），CM 表随后；两表 opcode 不重叠，顺序不影响语义
    MockDliCmdDispatchTable(g_cmCommonCbk, g_cmCommonCbkSize, cmdOpcode, 0, NULL, cmdParam);
    MockDliCmdDispatchTable(g_cmModuleCbk, g_cmModuleCbkSize, cmdOpcode, 0, NULL, cmdParam);
}

// 带 status 与自定义 context 的事件分发（PR-10）：SleAccessConnectCbk 的 DISALLOWED/错误分支、
// SleAccessSetRxDataFilterCbk（context->connHandle）等需要非 0 status 或真实 connHandle 的路径
void CmFuzzerDispatchCmdStatus(uint16_t cbkOpcode, void *evt, uint32_t evtSize, uint8_t status, uint16_t connHandle,
    void *rawContext)
{
    DLI_ExecuteCmdRetParam cmdParam = { 0 };
    cmdParam.cmdOpcode = cbkOpcode;
    cmdParam.size = evtSize;
    cmdParam.eventParameter = evt;
    DLI_ConnCbkContext cbkContext = {0};
    cbkContext.versionAndLocalIndex = CM_PackVersionLocalIndex(CM_CONNECT_LOCAL_INDEX_0, CM_CONNECT_VERSION_1_0);
    cbkContext.connHandle = connHandle;
    void *context = (rawContext != NULL) ? rawContext : &cbkContext;
    MockDliCmdDispatchTable(g_cmCommonCbk, g_cmCommonCbkSize, cbkOpcode, status, context, &cmdParam);
    MockDliCmdDispatchTable(g_cmModuleCbk, g_cmModuleCbkSize, cbkOpcode, status, context, &cmdParam);
}

void CmFuzzerGenDifferentAddress(SLE_Addr_S *addr, uint32_t handle)
{
    addr->addr[0] = handle; // make for different address
}

void CmFuzzerSleConnectCompleteEvt(uint16_t handle)
{
    DLI_ConnectionCompleteEvt connCompleteEvt = {0};
    connCompleteEvt.connHandle = handle;
    connCompleteEvt.role = CM_T_NODE;
    connCompleteEvt.connCompleteType = 0;
    connCompleteEvt.advHandle = 0x00;
    SLE_Addr_S addr = {0};
    CmFuzzerGenDifferentAddress(&addr, handle);
    (void)memcpy_s(connCompleteEvt.peerAddress, SLE_ADDR_LEN, addr.addr, SLE_ADDR_LEN);
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_CONNECT,
        .size = sizeof(connCompleteEvt),
        .eventParameter = &connCompleteEvt,
    };
    MockDliCmdExecuteCbk(&cmdParam);
}

void CmFuzzerSleConnectCompleteEvtWithAddr(uint16_t handle, const SLE_Addr_S *addr, uint8_t status)
{
    if (addr == NULL) {
        CM_LOGE("addr is null");
        return;
    }
    DLI_ConnectionCompleteEvt connCompleteEvt = {0};
    connCompleteEvt.connHandle = handle;
    connCompleteEvt.role = CM_T_NODE;
    connCompleteEvt.connCompleteType = CM_CONN_COMPLETE_SCAN; // 主动扫描直连
    connCompleteEvt.advHandle = 0x00;
    connCompleteEvt.status = status;
    (void)memcpy_s(connCompleteEvt.peerAddress, SLE_ADDR_LEN, addr->addr, SLE_ADDR_LEN);
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_CONNECT,
        .size = sizeof(connCompleteEvt),
        .eventParameter = &connCompleteEvt,
    };
    MockDliCmdExecuteCbk(&cmdParam);
}

void CmFuzzerSetMcsEvt(uint16_t connHandle, uint8_t status)
{
    for (size_t i = 0; i < g_cmdCbkSize; i++) {
        if ((g_cmdCbk[i].opcode == DLI_CBK_SET_MCS) && (g_cmdCbk[i].func != NULL)) {
            // CM_SleSetMcsProc 不读 cmdRes（SetMcs 无返回参数），lcid 取自 context->connHandle
            DLI_ConnCbkContext cbkContext = {0};
            cbkContext.versionAndLocalIndex = CM_PackVersionLocalIndex(CM_CONNECT_LOCAL_INDEX_0,
                CM_CONNECT_VERSION_1_0);
            cbkContext.connHandle = connHandle;
            DLI_ExecuteCmdRetParam cmdRes = { 0 };
            g_cmdCbk[i].func(&cbkContext, status, &cmdRes);
            break;
        }
    }
}

void CmFuzzerReadRemoteVersionCompleteEvt(DLI_ReadRemoteVersionEvt *evt)
{
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_READ_REMOTE_VERSION,
        .size = sizeof(DLI_ReadRemoteVersionEvt),
        .eventParameter = evt,
    };
    MockDliCmdExecuteCbk(&cmdParam);
}

void CmFuzzerReadRemoteFeaturesCompleteEvt(DLI_ReadRemoteFeatsEvt *evt)
{
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_READ_REMOTE_FEATURE,
        .size = sizeof(DLI_ReadRemoteFeatsEvt),
        .eventParameter = evt,
    };
    MockDliCmdExecuteCbk(&cmdParam);
}

void CmFuzzerSleDisconnectEvt(uint16_t handle, uint8_t reason)
{
    DLI_DisconnectEvt disconnectEvt = { 0 };
    disconnectEvt.connHandle = handle;
    disconnectEvt.reason = reason;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_DISCONNECT,
        .size = sizeof(disconnectEvt),
        .eventParameter = &disconnectEvt,
    };
    MockDliCmdExecuteCbk(&cmdParam);
}

void CmFuzzerSleConnectParamUpdateEvt(DLI_ConnectionUpdateCmpEvt *evt)
{
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_CONNECT_UPDATE,
        .size = sizeof(DLI_ConnectionUpdateCmpEvt),
        .eventParameter = evt,
    };
    MockDliCmdExecuteCbk(&cmdParam);
}

void CmFuzzerSleConnectRemoteParamUpdateReqEvt(DLI_RemoteConnParamReqEvt *evt)
{
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_REMOTE_CONNECT_PARAM_REQ,
        .size = sizeof(DLI_RemoteConnParamReqEvt),
        .eventParameter = evt,
    };
    MockDliCmdExecuteCbk(&cmdParam);
}

void CmFuzzerSetPhyEvt(DLI_SetPhyEvt *evt)
{
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_SET_PHY,
        .size = sizeof(DLI_SetPhyEvt),
        .eventParameter = evt,
    };
    MockDliCmdExecuteCbk(&cmdParam);
}

// ---- PR-10 DLI 帧事件时序注入族（DLI_CBK_CONNECT 经修复后的 CM_COMMON 表到达
// SleAccessConnectCbk，完整支持 role/被动连接/失败 status 等时序变体）----

// 完整版连接完成事件：role 区分 G/T 节点（SleAccessReadRemoteVersionCbk 分支），connCompleteType
// 区分主动(SCAN)/被动(ADV)，evt->status 非 0 走建链失败路径
void CmFuzzerSleConnectCompleteEvtEx(uint16_t handle, const SLE_Addr_S *addr, uint8_t status, uint8_t role,
    uint8_t connCompleteType)
{
    if (addr == NULL) {
        CM_LOGE("addr is null");
        return;
    }
    DLI_ConnectionCompleteEvt connCompleteEvt = {0};
    connCompleteEvt.status = status;
    connCompleteEvt.connHandle = handle;
    connCompleteEvt.role = role;
    connCompleteEvt.connCompleteType = connCompleteType;
    connCompleteEvt.advHandle = 0x00;
    (void)memcpy_s(connCompleteEvt.peerAddress, SLE_ADDR_LEN, addr->addr, SLE_ADDR_LEN);
    (void)memcpy_s(&connCompleteEvt.peerAddressType, sizeof(uint8_t), &addr->type, sizeof(uint8_t));
    CmFuzzerDispatchCmdStatus(DLI_CBK_CONNECT, &connCompleteEvt, sizeof(connCompleteEvt), DLI_SUCCESS, handle, NULL);
}

// 连接命令级错误注入：DISALLOWED(忽略)/TIMEOUT(取消连接中链路)/其他错误码(直接返回)三分支，
// SleAccessConnectCbk 在 status!=SUCCESS 时仍要求 eventParameter 非空，用哑事件占位
void CmFuzzerSleConnectErrEvt(uint8_t dliStatus)
{
    DLI_CommandErrorStru errEvt = { 0 };
    errEvt.status = dliStatus;
    CmFuzzerDispatchCmdStatus(DLI_CBK_CONNECT, &errEvt, sizeof(errEvt), dliStatus, CM_INVALID_LCID, NULL);
}

// 断开事件变体：正常事件分支 + 命令错误分支（后者 cmdRes->cmdOpcode 必须为 DLI_CMD_ERROR_EVT，
// connHandle 从 context 取、reason 取 status，模拟芯片回命令错误而非断开事件）
void CmFuzzerSleDisconnectEvtEx(uint16_t handle, uint8_t reason, uint16_t cmdOpcode)
{
    DLI_DisconnectEvt disconnectEvt = { 0 };
    disconnectEvt.connHandle = handle;
    disconnectEvt.reason = reason;
    CmFuzzerDispatchCmdStatus(DLI_CBK_DISCONNECT, &disconnectEvt, sizeof(disconnectEvt), DLI_SUCCESS, handle,
        NULL);
    if (cmdOpcode == DLI_CMD_ERROR_EVT) {
        DLI_CommandErrorStru errEvt = { 0 };
        errEvt.status = reason;
        DLI_ExecuteCmdRetParam cmdParam = { 0 };
        cmdParam.cmdOpcode = DLI_CMD_ERROR_EVT;
        cmdParam.size = sizeof(errEvt);
        cmdParam.eventParameter = &errEvt;
        DLI_ConnCbkContext cbkContext = {0};
        cbkContext.versionAndLocalIndex = CM_PackVersionLocalIndex(CM_CONNECT_LOCAL_INDEX_0,
            CM_CONNECT_VERSION_1_0);
        cbkContext.connHandle = handle;
        MockDliCmdDispatchTable(g_cmCommonCbk, g_cmCommonCbkSize, DLI_CBK_DISCONNECT, reason, &cbkContext,
            &cmdParam);
    }
}

// CM 表通用事件注入：ACB subrate(SET/REQ)/高功率/RxDataFilter/DataLenChange/RSSI 等，
// evt 按各 cbk 期望的结构体填好直接投递；connHandle 填充 DLI_ConnCbkContext
void CmFuzzerGenericCmEvt(uint16_t cbkOpcode, void *evt, uint32_t evtSize, uint8_t status, uint16_t connHandle)
{
    CmFuzzerDispatchCmdStatus(cbkOpcode, evt, evtSize, status, connHandle, NULL);
}

// SET_PEER_DEV_TYPE 专用：SleAccessSetPeerDevTypeCbk 把 context 直接当 SLE_Addr_S* 使用
void CmFuzzerPeerDevTypeEvt(const SLE_Addr_S *addr, uint8_t status)
{
    if (addr == NULL) {
        return;
    }
    DLI_CommandErrorStru stubEvt = { 0 };
    CmFuzzerDispatchCmdStatus(DLI_CBK_SET_PEER_DEV_TYPE, &stubEvt, sizeof(stubEvt), status, 0, (void *)addr);
}