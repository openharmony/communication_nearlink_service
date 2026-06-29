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
 * @file         devd_local.h
 * @brief        本端设备配置
 */
#ifndef DEVD_LOCAL_H
#define DEVD_LOCAL_H

#include "sdf_dlist.h"
#include "devd_tbl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SDF_DListHead_S advList;
    NLSTK_DevdScanEventCbk scanCbk;
    NLSTK_DevdAdvReportCbk reportCbk;
    NLSTK_DevdScanFilterCbk scanFilterCbk;
    DevdMgrScanItem_S scanMgr;
} DevdLocalDevice_S;

DevdLocalDevice_S *DevdGetLocalDevice(void);
uint8_t DevdGetPreAdvHandle(void);
void DevdSetPreAdvHandle(uint8_t newHandle);

#define DEVD_ADV_LIST (&(DevdGetLocalDevice()->advList))

#define SCAN_EVENT_CBK (DevdGetLocalDevice()->scanCbk)
#define SCAN_REPORT_CBK (DevdGetLocalDevice()->reportCbk)
#define DEVD_SCAN_MGR (DevdGetLocalDevice()->scanMgr)
#define SCAN_FILTER_CBK (DevdGetLocalDevice()->scanFilterCbk)

void DevdLocalDeviceInit(void);

void DevdLocalDeviceDeInit(void);

#ifdef __cplusplus
}
#endif
#endif