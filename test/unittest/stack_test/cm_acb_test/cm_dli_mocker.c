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
#include "cm_dli_mocker.h"
#include "cm_util_test.h"
#include "cm_def.h"
#include "cm_errno.h"
#include "cm_log.h"
#include "cm_util.h"
#include "dli_event_struct.h"

static const DLI_CbkLineStru *g_cmdCbk = NULL;
static uint8_t g_cmdCbkSize = 0;
static const DLI_CbkLineStru *g_cmdConnectionCbk = NULL;
static uint8_t g_cmdConnectionCbkSize = 0;

uint32_t DLI_CancelCreateConnection(void)
{
    CM_LOGI("DLI_CancelCreateConnection enter");
    UT_CM_SleCancelConnectEvt();
    CM_LOGI("DLI_CancelCreateConnection end");
    return DLI_SUCCESS;
}

uint32_t DLI_Disconnect(uint8_t version, uint16_t localIndex, DLI_DisconnectParam *param)
{
    CM_LOGI("DLI_Disconnect enter, discReason:0x%02x", param->reason);
    UT_CM_SleDisconnectEvt(param->connHandle, param->reason);
    return DLI_SUCCESS;
}

uint32_t DLI_CreateConnection(uint8_t version, uint16_t localIndex, DLI_ConnectionCreateParam *param)
{
    CM_LOGI("DLI_CreateConnection enter");
    return DLI_SUCCESS;
}

uint32_t DLI_RemoteConnectionParamReqReply(DLI_RemConParamReqReplyParam *param)
{
    return DLI_SUCCESS;
}

uint32_t DLI_ReadAcceptFilterListSize(void)
{
    return DLI_SUCCESS;
}

uint32_t DLI_ReadRemoteVersion(DLI_ConnHandleStru *param)
{
    CM_LOGI("DLI_ReadRemoteVersion enter");
    UT_CM_SleConnectReadRemoteVersion(param->connHandle);
    return DLI_SUCCESS;
}


uint32_t DLI_ReadRemoteFeatures(DLI_ConnHandleStru *param)
{
    CM_LOGI("DLI_ReadRemoteFetures enter");
    UT_CM_SleConnectReadFeatures(param->connHandle);
    return DLI_SUCCESS;
}

uint32_t DLI_ClearAcceptFilterList(void)
{
    CM_LOGI("DLI_ClearAcceptFilterList enter");
    return DLI_SUCCESS;
}

bool DLI_IsSupportConnBypassAdv(void)
{
    return true;
}

uint32_t DLI_AddDeviceToAcceptFilterList(SLE_Addr_S *addr)
{
    return 0;
}

void DLI_SetLocalFeatures(DLI_LocalFeatures_S *features)
{
}

uint32_t DLI_SetDataLength(DLI_SetDataLenParam *param)
{
    UT_CM_SleConnectSetDataLen(param->connHandle);
    UT_CM_SleConnectReadLocalFeature();
    return DLI_SUCCESS;
}

uint32_t DLI_SetPhy(DLI_SetPhyParam *param)
{
    return DLI_SUCCESS;
}

uint16_t DLI_GetAcbDataLen(void)
{
    return DLI_SUCCESS;
}

uint32_t DLI_SetHostChannelClassification(uint8_t *channelMap, uint32_t len)
{
    return DLI_SUCCESS;
}

uint32_t DLI_RemoveDeviceFromAcceptFilterList(SLE_Addr_S *addr)
{
    return DLI_SUCCESS;
}

uint32_t DLI_UpdateConnectionParam(uint8_t version, uint16_t localIndex, DLI_ConnectionUpdateParam *param)
{
    CM_LOGI("DLI_UpdateConnectionParam enter");
    DLI_ConnectionUpdateCmpEvt evt = { 0 };
    evt.connHandle = param->connHandle;
    DLI_ExecuteCmdRetParam cmdParam = {
        .cmdOpcode = DLI_CBK_CONNECT_UPDATE,
        .size = sizeof(evt),
        .eventParameter = &evt,
    };
    UT_CM_MockDliCmdExecuteCbk(&cmdParam, DLI_SUCCESS);
    return DLI_SUCCESS;
}


uint32_t DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    CM_LOGI("DLI_CmdCbkReg enter");
    if (module == CM_COMMON) {
        g_cmdConnectionCbk = table;
        g_cmdConnectionCbkSize = size;
        return DLI_SUCCESS;
    }
    if (module != CM) {
        CM_LOGE("DLI_CmdCbkReg module is not CM");
        return DLI_INVALID_PARAMETERS;
    }
    g_cmdCbk = table;
    g_cmdCbkSize = size;
    return DLI_SUCCESS;
}

uint32_t CP_PostTask(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb)
{
    cb(arg);
    if (freeCb != NULL) {
        freeCb(arg);
    }
    return CP_OK;
}

uint32_t CP_PostTaskBlocked(SDF_WorkCb cb, void *arg, SDF_FreeWorkArg freeCb, int timeout)
{
    (void)timeout;
    cb(arg);
    if (freeCb != NULL) {
        freeCb(arg);
    }
    return CP_OK;
}

void DLI_CmdCbkUnReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size)
{
    CM_LOGI("DLI_CmdCbkUnReg enter");
    return;
}

const DLI_CbkLineStru *DLI_GetCbk(void)
{
    return g_cmdCbk;
}

const DLI_CbkLineStru *DLI_GetDisconectCbk(void)
{
    return g_cmdConnectionCbk;
}

uint8_t DLI_GetCbkSize(void)
{
    return g_cmdCbkSize;
}

uint8_t DLI_GetDisconnectCbkSize(void)
{
    return g_cmdConnectionCbkSize;
}

uint32_t DLI_ReadRemoteRssi(DLI_ConnHandleStru *param)
{
    return 0;
}
