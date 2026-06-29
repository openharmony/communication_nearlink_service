/**
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

#ifndef SCAN_TYPE_H
#define SCAN_TYPE_H

#include <time.h>
#include "sdf_vector.h"
#include "nlstk_scan_api.h"
#include "nlstk_devd_scan_type.h"

#ifdef __cplusplus
extern "C" {
#endif

// Batch scan params, Unit slot
#define SCAN_MODE_BATCH_LOW_POWER_WINDOW 500               // 500 * 0.125 = 62.5ms
#define SCAN_MODE_BATCH_LOW_POWER_INTERVAL 45000           // 45000 * 0.125 = 5625ms
#define SCAN_MODE_BATCH_BALANCED_WINDOW 1000               // 1000 * 0.125 =125ms
#define SCAN_MODE_BATCH_BALANCED_INTERVAL 45000            // 45000 * 0.125 = 5625ms
#define SCAN_MODE_BATCH_LOW_LATENCY_WINDOW 1500            // 1500 * 0.125 = 187.5ms
#define SCAN_MODE_BATCH_LOW_LATENCY_INTERVAL 45000         // 45000 * 0.125 = 5625ms
#define SCAN_MODE_BATCH_FULL_SCAN_WINDOW 1248              // 1248 * 0.125 = 156ms
#define SCAN_MODE_BATCH_FULL_SCAN_INTERVAL 45000           // 45000 * 0.125 = 5625ms
#define SCAN_MODE_BATCH_OP_P2_60_3000_WINDOW 60            // 60 * 0.125 = 7.5ms
#define SCAN_MODE_BATCH_OP_P2_60_3000_INTERVAL 3000        // 3000 * 0.125 = 375ms
#define SCAN_MODE_BATCH_OP_P10_30_300_WINDOW 30            // 30 * 0.125 = 3.75ms
#define SCAN_MODE_BATCH_OP_P10_30_300_INTERVAL 300         // 300 * 0.125 = 37.5ms
#define SCAN_MODE_BATCH_OP_P10_60_600_WINDOW 60            // 60 * 0.125 = 7.5ms
#define SCAN_MODE_BATCH_OP_P10_60_600_INTERVAL 600         // 600 * 0.125 = 75ms
#define SCAN_MODE_BATCH_OP_P25_60_240_WINDOW 60            // 60 * 0.125 = 7.5ms
#define SCAN_MODE_BATCH_OP_P25_60_240_INTERVAL 240         // 240 * 0.125 = 30ms
#define SCAN_MODE_BATCH_OP_P50_100_200_WINDOW 100          // 100 * 0.125 = 12.5ms
#define SCAN_MODE_BATCH_OP_P50_100_200_INTERVAL 200        // 200 * 0.125 = 25ms
#define SCAN_MODE_BATCH_OP_P50_240_480_WINDOW 240          // 240 * 0.125 = 30ms
#define SCAN_MODE_BATCH_OP_P50_240_480_INTERVAL 480        // 480 * 0.125 = 60ms
#define SCAN_MODE_BATCH_OP_P50_480_960_WINDOW 480          // 480 * 0.125 = 60ms
#define SCAN_MODE_BATCH_OP_P50_480_960_INTERVAL 960        // 960 * 0.125 = 120ms
#define SCAN_MODE_BATCH_OP_P100_240_240_WINDOW 240         // 240 * 0.125 = 30ms
#define SCAN_MODE_BATCH_OP_P100_240_240_INTERVAL 240       // 240 * 0.125 = 30ms
#define SCAN_MODE_BATCH_OP_P100_1000_1000_WINDOW 1000      // 1000 * 0.125 = 125ms
#define SCAN_MODE_BATCH_OP_P100_1000_1000_INTERVAL 1000    // 1000 * 0.125 = 125ms

#define DEVD_SCAN_PHY_NUM_1 1
#define DEVD_SCAN_PHY_NUM_2 2

#define DEVD_BUSY_TIMEOUT_MSEC 5000


typedef enum {
    DEVD_TYPE_ALL_MATCHES = 0x00,   // 延迟上报
    DEVD_TYPE_FIRST_MATCH = 0x01,   // 直接上报
} DevdReportType_E;

typedef struct {
    uint8_t moduleId;
    NLSTK_DevdScanCbk_S scanCbk;
} DevdScanModuleParam_S;

typedef struct {
    uint32_t scannerId;
    uint8_t moduleId;
} DevdScannerParam_S;

typedef struct {
    uint32_t scannerId;
    NLSTK_DevdScanSetting_S setting;
    SDF_Vector_S *filters; // 成员 NLSTK_DevdScanFilter_S, 可能为空
} DevdStartScanParam_S;

#ifdef __cplusplus
}
#endif

#endif