/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef DEVD_CBK_H
#define DEVD_CBK_H

#include "dli_cmd_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

void DevdSetAdvParamCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);

void DevdSetAdvDataCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);

void DevdEnableAdvCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);

void DevdSetTxPowerCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);

void DevdRemoveAdvCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);

void DevdAdvTerminatedCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);

void DevdSleSetScanParamCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);

void DevdSleEnableScanCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);

bool DevdSleScanReportCbkCheck(const struct DLI_ExecuteCmdRetParam *cmdRes);

void DevdSleScanReportCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);

void DevdSleSetScanFilterCbk(void *context, uint16_t status, struct DLI_ExecuteCmdRetParam *cmdRes);

#ifdef __cplusplus
}
#endif
#endif