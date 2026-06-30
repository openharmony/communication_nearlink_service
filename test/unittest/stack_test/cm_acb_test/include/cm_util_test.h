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
#ifndef CM_UTIL_TEST_H
#define CM_UTIL_TEST_H

#include "securec.h"
#include "sdf_worker.h"
#include "cp_worker.h"
#include "dli_errno.h"
#include "dli_cmd_struct.h"
#include "cm_api.h"
#include "cm_icb_def.h"
#include "cm_trans_channel_api.h"

// 宏定义测试值，与生产实际值不一定一致
#define UT_CM_COMPANY_ID 0x0009
#define UT_CM_REMOTE_FEATURES 0x01

#define UT_CM_CONN_TX_FORMAT CM_RADIO_FRAME_TYPE_2
#define UT_CM_CONN_INTERAL 0x320           // 链路调度最小间隔 800*0.125ms=100ms
#define UT_CM_CONN_EVENT_IFS 125            // 链路调度间隔.125us
#define UT_CM_CONN_SUPERVISION_TIMEOUT 500 // 超时时间5000ms
#define UT_CM_CONN_MAX_LATENCY 0x1F3
#define UT_CM_CONN_TIME_UNIT 4              // 系统调度时隙, 125us
#define UT_CM_CONN_T_TX_RX_FLAG 1
#define UT_CM_CONN_ADV_HANDLE 0x01

// 未定义的DLI事件
typedef struct {
    uint32_t status;
} CM_TestDLI_UndefinedEvt;

#ifdef __cplusplus
extern "C" {
#endif

// ********************** 测试工具函数 **********************
void UT_CM_GenDifferentAddress(SLE_Addr_S *addr, uint16_t i);

void UT_CM_TestNormalReset(void);

void UT_CM_SetNodeRole(uint8_t role);

uint32_t UT_CM_GetTestFixedTransChannelSize(void);

size_t UT_CM_GetTestConnectListSize(void);

size_t UT_CM_GetDtapTestConnectListSize(void);

CM_LogicLinkState_S UT_CM_GetDtapTestConnectListFirst(void);

void UT_CM_GetTestConnectListClear(void);

// ********************** DLI CMD 回调事件 打桩 **********************
void UT_CM_MockDliCmdExecuteCbk(DLI_ExecuteCmdRetParam *cmdParam, uint8_t status);

uint32_t UT_CM_DLI_CmdCbkReg(const ModuleType module, const DLI_InnerCbkLineStru *innerTable, const uint32_t innerSize,
    const DLI_CbkLineStru *table, const uint32_t size);

void UT_CM_SleConnectCompleteEvt(uint16_t handle, uint8_t status, uint8_t connCompleteType);

void UT_CM_SleDisconnectCompleteEvt(uint16_t handle, uint8_t status);

void UT_CM_SleConnectCompleteCommandErrorEvt(uint16_t handle, uint8_t status);

void UT_CM_DtapTransChannelStateCbk(CM_TransChannelStateList_S *param);

void UT_CM_SleDisconnectEvt(uint16_t handle, uint8_t reason);

void UT_CM_CancelConnectResult(uint8_t result);

void UT_CM_ConnectCancelCbk(uint8_t *param);

void UT_CM_ServiceConnectStateCbk(CM_LogicLinkState_S *param);

void UT_CM_SetTestTcid(uint16_t lcid);

uint16_t UT_CM_GetTestTcid(void);

CM_LogicLinkState_S UT_CM_GetLogicLinkState(uint16_t lcid);

void UT_CM_DtapConnectStateCbk(CM_LogicLinkState_S *param);

void UT_CM_ReadRemoteFeatureCbk(CM_LogicLinkRemoteFeatures_S *param);

void UT_CM_ConnUpdateParamCbk(CM_LogicLinkConnUpdateParam_S *param);

uint8_t UT_CM_GetTestNodeRole(void);

void UT_CM_SleCancelConnectEvt(void);

void UT_CM_SleConnectReadRemoteVersion(uint16_t handle);

void UT_CM_SleConnectReadFeatures(uint16_t handle);

void UT_CM_SleConnectSetDataLen(uint16_t handle);

void UT_CM_SleConnectReadLocalFeature(void);

void UT_CM_SleSetAcbSubrateCbkEvt(uint16_t handle, CM_SetACBSubrateParam *param);

void UT_CM_SleReqAcbSubrateCbkEvt(uint16_t handle, CM_SetACBSubrateParam *param);

void UT_CM_SetPowerModeCbkEvt(uint16_t handle);

void UT_CM_SleFreqBandSwitchCbkEvt(uint16_t handle, CM_FreqBandSwitchParam *param);

// ********************** DLI CMD CBK 校验 **********************
void UT_CM_SsapConnectReadRemoteFetureVersionCbk(CM_ReadRemoteFeatureVersionRsp_S *param);

void UT_CM_SsapConnectUpdatePramCbk(CM_ConnectUpdateParamRsp_S *param);

void UT_CM_SsapLogicLinkUpdatePramCbk(CM_LogicLinkConnUpdateParam_S *param);

void UT_CM_ConnectRemoteUpdateParamReqCbk(CM_ConnectRemoteUpdateParamReq_S *param);

void UT_CM_SetAcbSubrateCbk(CM_AcbSubrateCbParam_S *param);

void UT_CM_ReqAcbSubrateCbk(CM_AcbSubrateCbParam_S *param);

void UT_CM_SetRxDataFilterCbk(CM_SetRxDataFilterRsp_S *param);

CM_AcbSubrateCbParam_S UT_CM_GetAcbSubrateCbkParam(void);

void UT_CM_SetPhyCbk(CM_SetPhyRsp_S *param);

#ifdef __cplusplus
}
#endif

#endif // CM_UTIL_TEST_H