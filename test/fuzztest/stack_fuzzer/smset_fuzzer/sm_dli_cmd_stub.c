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

#include "dli_cmd.h"

uint32_t DLI_GetPublicAddress(DLI_PublicAddrParam *param)
{
    return 0;
}
uint32_t DLI_SetPublicAddress(uint8_t *addr)
{
    return 0;
}

uint32_t DLI_SetHostChannelClassification(uint8_t *channelMap, uint32_t len)
{
    return 0;
}

uint32_t DLI_ReadLocalFeatures(void)
{
    return 0;
}

uint32_t DLI_ReadBufferSize(void)
{
    return 0;
}

uint32_t DLI_ReadLocalVersion(void)
{
    return 0;
}

uint32_t DLI_ReadLocalPrivateFeatures(void)
{
    return 0;
}

uint32_t DLI_ReadCommConfigValue(void)
{
    return 0;
}

uint32_t DLI_SetScanParam(DLI_ScanParam *scanParam)
{
    return 0;
}

uint32_t DLI_EnableScan(DLI_ScanEnable *scanEnable)
{
    return 0;
}

uint32_t DLI_ReadMaximumAdvDataLen(void)
{
    return 0;
}

uint32_t DLI_ReadAdvSetsNum(void)
{
    return 0;
}

uint32_t DLI_SetAdvParam(DLI_AdvParam *advParam)
{
    return 0;
}

uint8_t DLI_GetPhyCountByFrameType(uint8_t frameType)
{
    uint8_t phyCount = 0;
    if ((frameType & SCAN_FRAME_TYPE_1) != 0) {
        phyCount++;
    }
    if ((frameType & SCAN_FRAME_TYPE_4) != 0) {
        phyCount++;
    }
    return phyCount;
}

uint32_t DLI_SetAdvData(DLI_AdvData *advData)
{
    return 0;
}

uint32_t DLI_SetScanRspData(DLI_ScanRspData *scanRspData)
{
    return 0;
}

uint32_t DLI_EnableAdv(uint8_t advHandle, DLI_AdvEnable *advEnable)
{
    return 0;
}

uint32_t DLI_RemoveAdvSet(uint8_t advHandle)
{
    return 0;
}

uint32_t DLI_ReadAcceptFilterListSize(void)
{
    return 0;
}

uint32_t DLI_ClearAcceptFilterList(void)
{
    return 0;
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

uint32_t DLI_RemoveDeviceFromAcceptFilterList(SLE_Addr_S *addr)
{
    return 0;
}

uint32_t DLI_CreateConnection(uint8_t version, uint16_t localIndex, DLI_ConnectionCreateParam *param)
{
    return 0;
}

uint32_t DLI_CancelCreateConnection(void)
{
    return 0;
}

uint32_t DLI_Disconnect(uint8_t version, uint16_t localIndex, DLI_DisconnectParam *param)
{
    return 0;
}

uint32_t DLI_UpdateConnectionParam(uint8_t version, uint16_t localIndex, DLI_ConnectionUpdateParam *param)
{
    return 0;
}

uint32_t DLI_SetPhy(DLI_SetPhyParam *param)
{
    return 0;
}

uint32_t DLI_SetAcbEventParam(DLI_SetAcbEvtParam *param)
{
    return 0;
}

uint32_t DLI_SetDataLength(DLI_SetDataLenParam *param)
{
    return 0;
}

uint32_t DLI_ReadRemoteRssi(DLI_ConnHandleStru *param)
{
    return 0;
}

uint32_t DLI_ReadRemoteFeatures(DLI_ConnHandleStru *param)
{
    return 0;
}

uint32_t DLI_ReadRemoteVersion(DLI_ConnHandleStru *param)
{
    return 0;
}

uint32_t DLI_RemoteConnectionParamReqReply(DLI_RemConParamReqReplyParam *param)
{
    return 0;
}

uint32_t DLI_SetMcs(DLI_SetMcsParam *param)
{
    return 0;
}

uint32_t DLI_SetTxPower(DLI_SetTxPowerParam *param)
{
    return 0;
}

uint32_t DLI_ReadSupportCryptoAlgo(void)
{
    return 0;
}

uint32_t DLI_SetControllerData(DLI_ControllerData *data)
{
    return 0;
}

uint32_t DLI_EnableEncryption(DLI_EnableEncryptParam *param)
{
    return 0;
}

uint32_t DLI_EncryptionParamReqNegativeReply(DLI_ConnHandleStru *param)
{
    return 0;
}

uint32_t DLI_EncryptionParamReqReply(DLI_EncryptReqReplyParam *param)
{
    return 0;
}

uint32_t DLI_Encrypt(DLI_EncryptParam *param)
{
    return 0;
}

uint32_t DLI_ReadLocalMeasureCaps(void)
{
    return 0;
}

uint32_t DLI_ReadRemoteMeasureCaps(DLI_ReadRemoteMeasureCapsParam *param)
{
    return 0;
}

uint32_t DLI_SetMeasureParam(DLI_SetMeasureConfigParam *param)
{
    return 0;
}

uint32_t DLI_SetMeasureEnable(DLI_SetMeasureEnableParam *param)
{
    return 0;
}

uint32_t DLI_SetICGParam(DLI_ICGParam *param, DLI_ICGCbkParam *cbkParam)
{
    return 0;
}

uint32_t DLI_SetICGTestParam(DLI_ICGTestParam *param, DLI_ICGCbkParam *cbkParam)
{
    return 0;
}

uint32_t DLI_RemoveICGParam(DLI_CmdOpcode opCode, DLI_ICGCbkParam *cbkParam)
{
    return 0;
}

uint32_t DLI_CreateICB(DLI_ICBConnectionParam *param, DLI_ICGCbkParam *cbkParam)
{
    return 0;
}

uint32_t DLI_DisconnectICB(DLI_DisconnectParam *param, DLI_ICGCbkParam *cbkParam)
{
    return 0;
}

uint32_t DLI_SetupICBDataPath(DLI_SetupICBDataPathParam *dataPath, DLI_ICGCbkParam *cbkParam)
{
    return 0;
}

uint32_t DLI_RemoveICBDataPath(DLI_RemoveICBDataPathParam *dataPath, DLI_ICGCbkParam *cbkParam)
{
    return 0;
}

uint32_t DLI_AcceptICBReq(DLI_AcceptICBReqParam *param)
{
    return 0;
}

uint32_t DLI_RejectICBReq(DLI_RejectICBReqParam *param)
{
    return 0;
}

uint16_t DLI_GetAcbDataLen(void)
{
    return 0;
}

uint32_t DLI_SetCmd(DLI_CmdParams *params)
{
    return 0;
}

uint32_t DLI_ACBSetSubrate(DLI_ACBSubrateParam *param)
{
    return 0;
}

uint32_t DLI_ACBEnableSubrate(DLI_ACBEnableSubrateParam *param)
{
    return 0;
}

uint32_t DLI_SetLocalPrivateFeatures(DLI_LocalPrivateFeatures *features)
{
    return 0;
}

uint32_t DLI_ReadRemotePrivateFeatures(DLI_ConnHandleStru *param)
{
    return 0;
}

uint32_t DLI_SetConnFramePowerLevel(DLI_SetConnFramePowerLevelParam *param)
{
    return 0;
}

uint32_t DLI_EnableIMGEncryption(DLI_IMGEncryptParam *param)
{
    return 0;
}

bool DLI_IsSupportNewDisMeasure()
{
    return false;
}

uint32_t DLI_SetICGTestParamExt(DLI_ICGTestParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    return 0;
}

uint32_t DLI_SetICGLabelExt(DLI_ICGLabelParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    return 0;
}

uint32_t DLI_CreateICBExt(DLI_ICBConnectionParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    return 0;
}

uint32_t DLI_UpdateICGParamExt(DLI_ICGUpdatedParam *param, bool mcast, DLI_ICGCbkParam *cbkParam)
{
    return 0;
}
