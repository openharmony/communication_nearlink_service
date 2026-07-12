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

#ifndef SCAN_MANAGER_H
#define SCAN_MANAGER_H

#include "nlstk_public_define.h"
#include "nlstk_scan_api.h"
#include "devd_scan_stm.h"

#ifdef __cplusplus
extern "C" {
#endif

NLSTK_Errcode_E DevdScanManagerInit(void);

void DevdScanManagerDeInit(void);

NLSTK_DevdStateMachine_S *DevdGetStateMachine(void);

uint8_t DevdCreateScanModule(NLSTK_DevdScanCbk_S *scanCbk);

void DevdRemoveScanModule(uint8_t moduleId);

uint32_t DevdCreateScanner(uint8_t moduleId);

void DevdDestroyScanner(uint32_t scannerId);

NLSTK_Errcode_E DevdAddScanParam(uint32_t scannerId, NLSTK_DevdScanSetting_S *setting, SDF_Vector_S *filters);

NLSTK_Errcode_E DevdRemoveScanParam(uint32_t scannerId);

void DevdRemoveAllScanParam(void);

#ifdef __cplusplus
}
#endif

#endif