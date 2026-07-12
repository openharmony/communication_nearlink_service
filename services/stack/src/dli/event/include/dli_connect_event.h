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

/****************************************************************************
 *
 * cm模块的回调处理
 *
 ***************************************************************************/

#ifndef DLI_CONNECT_EVENT_H
#define DLI_CONNECT_EVENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void DLI_DataLengthChangeCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_SetPhyCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_AcbLowLatencyEnableCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_RemoteConnParamReqCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_ConnectionCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_DisconnectionCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_ReadRemoteFeaturesCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_SetAcbEvtParamCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_ConnectionUpdateCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_ReadRemoteVersionCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_FreqBandSwitchCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IOBConnectReqCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IOBEstablishedCbkStd(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IOBEstablishedCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IOBReportParamCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IOGLabelReportCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IOGUpdateParamCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IMBConnectReqCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IMBEstablishedCbkStd(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IMBEstablishedCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IMBReportParamCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IMGLabelReportCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_IMGUpdateParamCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

void DLI_AcbSubrateCbk(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

#ifdef __cplusplus
}
#endif

#endif