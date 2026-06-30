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
#ifndef STACK_DLI_CMD_STUB_H
#define STACK_DLI_CMD_STUB_H

#include "dli_opcode.h"
#include "dli_cmd_struct.h"

#ifdef __cplusplus
extern "C" {
#endif
uint32_t TEST_DLI_GetPublicAddress(DLI_PublicAddrParam *param);

uint32_t TEST_DLI_GetPublicAddressFail(DLI_PublicAddrParam *param);

uint32_t TEST_DLI_SetPublicAddress(uint8_t *addr);

uint32_t TEST_DLI_SetHostChannelClassification(uint8_t *channelMap, uint32_t len);

uint32_t TEST_DLI_ReadLocalFeatures(void);

uint32_t TEST_DLI_ReadLocalFeaturesFail(void);

uint32_t TEST_DLI_ReadBufferSize(void);

uint32_t TEST_DLI_ReadLocalVersion(void);

uint32_t TEST_DLI_ReadLocalVersionFail(void);

uint32_t TEST_DLI_ReadCommConfigValue(void);

uint32_t TEST_DLI_SetScanParam(DLI_ScanParam *scanParam);

uint32_t TEST_DLI_EnableScan(DLI_ScanEnable *scanEnable);

uint32_t TEST_DLI_ReadMaximumAdvDataLen(void);

uint32_t TEST_DLI_ReadMaximumAdvDataLenFail(void);

uint32_t TEST_DLI_ReadAdvSetsNum(void);

uint32_t TEST_DLI_ReadAdvSetsNumFail(void);

uint32_t TEST_DLI_SetAdvParam(DLI_AdvParam *advParam);

uint32_t TEST_DLI_SetAdvData(DLI_AdvData *advData, uint16_t dataOff, uint16_t dataLen, uint16_t ops);

uint32_t TEST_DLI_EnableAdv(uint8_t advHandle, DLI_AdvEnable *advEnable);

uint32_t TEST_DLI_RemoveAdvSet(uint8_t advHandle);

uint32_t TEST_DLI_ReadAcceptFilterListSize(void);

uint32_t TEST_DLI_ClearAcceptFilterList(void);

void TEST_DLI_SetLocalFeatures(DLI_LocalFeatures_S *features);

uint32_t TEST_DLI_RemoveDeviceFromAcceptFilterList(SLE_Addr_S *addr);

uint32_t TEST_DLI_CreateConnection(uint8_t version, uint16_t localIndex, DLI_ConnectionCreateParam *param);

uint32_t TEST_DLI_CancelCreateConnection(void);

uint32_t TEST_DLI_Disconnect(uint8_t version, uint16_t localIndex, DLI_DisconnectParam *param);

uint32_t TEST_DLI_UpdateConnectionParam(uint8_t version, uint16_t localIndex, DLI_ConnectionUpdateParam *param);

uint32_t TEST_DLI_SetPhy(DLI_SetPhyParam *param);

uint32_t TEST_DLI_SetDataLength(DLI_SetDataLenParam *param);

uint32_t TEST_DLI_ReadRemoteRssi(DLI_ConnHandleStru *param);

uint32_t TEST_DLI_ReadRemoteFeatures(DLI_ConnHandleStru *param);

uint32_t TEST_DLI_ReadRemoteVersion(DLI_ConnHandleStru *param);

uint32_t TEST_DLI_RemoteConnectionParamReqReply(DLI_RemConParamReqReplyParam *param);

uint32_t TEST_DLI_SetMcs(DLI_SetMcsParam *param);

uint32_t TEST_DLI_ReadSupportCryptoAlgo(void);

uint32_t TEST_DLI_ReadSupportCryptoAlgoFail(void);

uint32_t TEST_DLI_SetControllerData(DLI_ControllerData *data);

uint32_t TEST_DLI_EnableEncryption(DLI_EnableEncryptParam *param);

uint32_t TEST_DLI_EncryptionParamReqNegativeReply(DLI_ConnHandleStru *param);

uint32_t TEST_DLI_EncryptionParamReqReply(DLI_EncryptReqReplyParam *param);

uint32_t TEST_DLI_Encrypt(DLI_EncryptParam *param);

uint32_t TEST_DLI_ReadLocalMeasureCaps(void);

uint32_t TEST_DLI_ReadRemoteMeasureCaps(DLI_ReadRemoteMeasureCapsParam *param);

uint32_t TEST_DLI_SetMeasureParam(DLI_SetMeasureConfigParam *param);

uint32_t TEST_DLI_SetMeasureEnable(DLI_SetMeasureEnableParam *param);

uint32_t TEST_DLI_CreateICB(DLI_ICBConnectionParam *param, DLI_ICGCbkParam *cbkParam);

uint32_t TEST_DLI_SetupICBDataPath(DLI_SetupICBDataPathParam *dataPath, DLI_ICGCbkParam *cbkParam);

uint32_t TEST_DLI_AcceptICBReq(DLI_AcceptICBReqParam *param);

uint32_t TEST_DLI_RejectICBReq(DLI_RejectICBReqParam *param);

#ifdef __cplusplus
}
#endif

#endif