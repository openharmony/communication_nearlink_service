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
#include "dli_errno.h"

uint32_t DLI_CreateConnection(uint8_t version, uint16_t localIndex, DLI_ConnectionCreateParam *param)
{
    return DLI_SUCCESS;
}

uint32_t DLI_Disconnect(uint8_t version, uint16_t localIndex, DLI_DisconnectParam *param)
{
    return DLI_SUCCESS;
}

uint32_t DLI_CancelCreateConnection(void)
{
    return DLI_SUCCESS;
}

uint32_t DLI_UpdateConnectionParam(uint8_t version, uint16_t localIndex, DLI_ConnectionUpdateParam *param)
{
    return DLI_SUCCESS;
}

uint32_t DLI_SetPhy(DLI_SetPhyParam *param)
{
    return DLI_SUCCESS;
}

uint32_t DLI_ReadRemoteRssi(DLI_ConnHandleStru *param)
{
    return 0;
}

uint32_t DLI_ReadRemoteFeatures(DLI_ConnHandleStru *param)
{
    return DLI_SUCCESS;
}

uint32_t DLI_RemoteConnectionParamReqReply(DLI_RemConParamReqReplyParam *param)
{
    return DLI_SUCCESS;
}

uint32_t DLI_ReadRemoteVersion(DLI_ConnHandleStru *param)
{
    return DLI_SUCCESS;
}

uint16_t DLI_GetAcbDataLen(void)
{
    return DLI_SUCCESS;
}

uint32_t DLI_SetDataLength(DLI_SetDataLenParam *param)
{
    return DLI_SUCCESS;
}

void DLI_CmdCbkUnReg(const ModuleType module, const DLI_CbkLineStru *table, const uint32_t size)
{
}

uint32_t DLI_ReadAcceptFilterListSize(void)
{
    return DLI_SUCCESS;
}

uint32_t DLI_ClearAcceptFilterList(void)
{
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

uint32_t DLI_RemoveDeviceFromAcceptFilterList(SLE_Addr_S *addr)
{
    return DLI_SUCCESS;
}

uint32_t DLI_SetHostChannelClassification(uint8_t *channelMap, uint32_t len)
{
    return DLI_SUCCESS;
}

void DLI_SetLocalFeatures(DLI_LocalFeatures_S *features)
{
}
