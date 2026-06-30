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

static const DLI_CbkLineStru *g_cmdCbk = NULL;
static uint8_t g_cmdCbkSize = 0;

extern "C" uint32_t DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    CM_LOGI("DLI_CmdCbkReg enter");
    if (module != CM && module != CM_COMMON) {
        CM_LOGE("DLI_CmdCbkReg module is not CM");
        return DLI_INVALID_PARAMETERS;
    }
    g_cmdCbk = table;
    g_cmdCbkSize = size;
    return DLI_SUCCESS;
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

extern "C" uint32_t CP_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    return CP_PostTaskStub(cb, arg, freeCb);
}

extern "C" uint32_t CP_PostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    return CP_PostTaskBlockedStub(cb, arg, freeCb, timeout);
}

void MockDliCmdExecuteCbk(DLI_ExecuteCmdRetParam *cmdParam)
{
    if (cmdParam == NULL) {
        return;
    }
    uint16_t cmdOpcode = cmdParam->cmdOpcode;
    for (size_t i = 0; i < g_cmdCbkSize; i++) {
        if (g_cmdCbk[i].opcode == cmdOpcode) {
            if (g_cmdCbk[i].func != NULL) {
                DLI_ExecuteCmdRetParam cmdRes = { 0 };
                cmdRes.cmdOpcode = cmdOpcode;
                cmdRes.eventParameter = cmdParam->eventParameter;
                cmdRes.size = cmdParam->size;
                uint16_t localIndex = CM_CONNECT_VERSION_1_0;
                uint8_t version = CM_CONNECT_LOCAL_INDEX_0;
                DLI_ConnCbkContext cbkContext = {0};
                cbkContext.versionAndLocalIndex = CM_PackVersionLocalIndex(version, localIndex);
                CM_LOGI("MockDliCmdExecuteCbk, cmdOpcode:0x%04x, size:%u, versionLocalIndex:0x%4x",
                    cmdOpcode, cmdParam->size, cbkContext.versionAndLocalIndex);
                g_cmdCbk[i].func(&cbkContext, 0, &cmdRes);
                break;
            }
        }
    }
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