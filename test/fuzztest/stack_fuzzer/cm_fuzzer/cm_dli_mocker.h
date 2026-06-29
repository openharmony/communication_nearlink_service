/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef TEST_FUZZTEST_CM_DLI_MOCKER_H
#define TEST_FUZZTEST_CM_DLI_MOCKER_H

#include <stdint.h>
#include "dli_cmd_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t DLI_CreateConnection(uint8_t version, uint16_t localIndex, DLI_ConnectionCreateParam *param);

uint32_t DLI_Disconnect(uint8_t version, uint16_t localIndex, DLI_DisconnectParam *param);

uint32_t DLI_CancelCreateConnection(void);

uint32_t DLI_UpdateConnectionParam(uint8_t version, uint16_t localIndex, DLI_ConnectionUpdateParam *param);

uint32_t DLI_SetPhy(DLI_SetPhyParam *param);

uint32_t DLI_ReadRemoteFeatures(DLI_ConnHandleStru *param);

uint32_t DLI_RemoteConnectionParamReqReply(DLI_RemConParamReqReplyParam *param);

uint32_t DLI_ReadRemoteVersion(DLI_ConnHandleStru *param);

uint16_t DLI_GetAcbDataLen(void);

uint32_t DLI_SetDataLength(DLI_SetDataLenParam *param);

void DLI_CmdCbkUnReg(const ModuleType module, const DLI_CbkLineStru *table, const uint32_t size);

uint32_t DLI_ReadAcceptFilterListSize(void);

uint32_t DLI_ClearAcceptFilterList(void);

bool DLI_IsSupportConnBypassAdv(void);

uint32_t DLI_AddDeviceToAcceptFilterList(SLE_Addr_S *addr);

uint32_t DLI_RemoveDeviceFromAcceptFilterList(SLE_Addr_S *addr);

uint32_t DLI_SetHostChannelClassification(uint8_t *channelMap, uint32_t len);

void DLI_SetLocalFeatures(DLI_LocalFeatures_S *features);

#ifdef __cplusplus
}
#endif

#endif // TEST_FUZZTEST_CM_DLI_MOCKER_H