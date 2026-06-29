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

#ifndef NLSTK_DEVD_SCAN_TYPE_EXT_H
#define NLSTK_DEVD_SCAN_TYPE_EXT_H

#include <stdint.h>
#include "sdf_vector.h"
#include "sdf_addr.h"
#include "nlstk_public_define_ext.h"

#ifdef __cplusplus
extern "C" {
#endif

// Regular scan params, Unit slot
#define SCAN_MODE_LOW_POWER_WINDOW 900             // 900 * 0.125 = 112.5ms
#define SCAN_MODE_LOW_POWER_INTERVAL 9000          // 9000 * 0.125 = 1125ms
#define SCAN_MODE_BALANCED_WINDOW 1024             // 1024 * 0.125 = 128ms
#define SCAN_MODE_BALANCED_INTERVAL 4096           // 4096 * 0.125 = 512ms
#define SCAN_MODE_LOW_LATENCY_WINDOW 4096          // 4096 * 0.125 = 512ms
#define SCAN_MODE_LOW_LATENCY_INTERVAL 4096        // 4096 * 0.125 = 512ms
#define SCAN_MODE_FULL_SCAN_WINDOW 2048            // 2048 * 0.125 = 256ms
#define SCAN_MODE_FULL_SCAN_INTERVAL 4096          // 4096 * 0.125 = 512ms
#define SCAN_MODE_OP_P2_60_3000_WINDOW 60          // 60 * 0.125 = 7.5ms
#define SCAN_MODE_OP_P2_60_3000_INTERVAL 3000      // 3000 * 0.125 = 375ms
#define SCAN_MODE_OP_P10_30_300_WINDOW 30          // 30 * 0.125 = 4.85ms
#define SCAN_MODE_OP_P10_30_300_INTERVAL 300       // 300 * 0.125 = 48.5ms
#define SCAN_MODE_OP_P10_60_600_WINDOW 60          // 60 * 0.125 = 7.5ms
#define SCAN_MODE_OP_P10_60_600_INTERVAL 600       // 600 * 0.125 = 75ms
#define SCAN_MODE_OP_P25_60_240_WINDOW 60          // 60 * 0.125 = 7.5ms
#define SCAN_MODE_OP_P25_60_240_INTERVAL 240       // 240 * 0.125 = 30ms
#define SCAN_MODE_OP_P50_100_200_WINDOW 100        // 100 * 0.125 = 12.5ms
#define SCAN_MODE_OP_P50_100_200_INTERVAL 200      // 200 * 0.125 = 25ms
#define SCAN_MODE_OP_P50_240_480_WINDOW 240        // 240 * 0.125 = 30ms
#define SCAN_MODE_OP_P50_240_480_INTERVAL 480      // 480 * 0.125 = 60ms
#define SCAN_MODE_OP_P50_480_960_WINDOW 480        // 480 * 0.125 = 60ms
#define SCAN_MODE_OP_P50_480_960_INTERVAL 960      // 960 * 0.125 = 120ms
#define SCAN_MODE_OP_P100_240_240_WINDOW 240       // 240 * 0.125 = 30ms
#define SCAN_MODE_OP_P100_240_240_INTERVAL 240     // 240 * 0.125 = 30ms
#define SCAN_MODE_OP_P100_1000_1000_WINDOW 1000    // 1000 * 0.125 = 125ms
#define SCAN_MODE_OP_P100_1000_1000_INTERVAL 1000  // 1000 * 0.125 = 125ms

#define SERVICE_UUID_LEN_16  2
#define SERVICE_UUID_LEN_128 16

#define DEVD_MAX_MODULE_ID 5
#define DEVD_INVALID_SCANNER_ID 0

#define NLSTK_DEVD_MULTI_SCAN_MAX_NUM 1024
#define NLSTK_DEVD_TIMER_NO_USED_HANDLE (-1)

#define DEVD_SCAN_FILTER_BASE_LEN 2
#define DEVD_SCAN_FILTER_PATH_LEN 4
#define DEVD_SCAN_FILTER_ADDR_LEN 12
#define DEVD_MAX_SERVICE_DATA_FILTER 24
#define DEVD_SCAN_FILTER_SERVICE_DATA_BASE_LEN 5
#define DEVD_DOUBLE_FOR_MASK 2
#define DEVD_SCAN_FILTER_NOT_FILTERED_LEN 5
#define DEVD_SCAN_FILTER_SWITCH_LEN 3

typedef enum {
    DEVD_FILTER_DISABLE = 0,
    DEVD_FILTER_ENABLE,
    DEVD_FILTER_DELETE_DISABLE,
} NLSTK_DevdScanFilterSwitchAction_E;

typedef enum {
    DEVD_FILTER_SWITCH = 0,
    DEVD_FILTER_DESIGNATE_RECV_PATH,
    DEVD_FILTER_NOT_FILTERED,
    DEVD_FILTER_DEVICE_ADDR,
    DEVD_FILTER_DEVICE_NAME,
    DEVD_FILTER_SERVICE_DATA,
    DEVD_FILTER_DESIGNATE_TYPE_DATA,
    DEVD_FILTER_DESIGNATE_TYPE_DATA_MASK,
} NLSTK_DevdScanFilterSubCode_E;

typedef enum {
    SCAN_MODE_LOW_POWER = 0x00,
    SCAN_MODE_BALANCED = 0x01,
    SCAN_MODE_LOW_LATENCY = 0x02,
    SCAN_MODE_OP_P2_60_3000 = 0x03,
    SCAN_MODE_OP_P10_60_600 = 0x04,
    SCAN_MODE_OP_P25_60_240 = 0x05,
    SCAN_MODE_OP_P100_1000_1000 = 0x06,
    SCAN_MODE_OP_P50_100_200 = 0x07,
    SCAN_MODE_OP_P50_240_480 = 0x08,
    SCAN_MODE_OP_P50_480_960 = 0x09,
    SCAN_MODE_OP_P100_240_240 = 0x0A,
    SCAN_MODE_FULL_SCAN = 0x0B,
    SCAN_MODE_MONITOR = 0x0C, // 由于扫描和过滤逻辑均已下移至协议栈，而service需要存储广播数据，此模式用于下发全通软件过滤监听扫描到的广播数据而不实际起扫描与下发硬过滤
    SCAN_MODE_OP_P10_30_300 = 0x0D,
    SCAN_MODE_INVALID,
} NLSTK_DevdScanMode_E;

typedef enum {
    DEVD_SCAN_FRAME_TYPE_1 = 0x01,  // 无线帧类型1
    DEVD_SCAN_FRAME_TYPE_4 = 0x02,  // 无线帧类型4
} NLSTK_DevdScanFrameType_E;  // 与DLI_ScanFrameType定义保持一致

typedef enum {
    NLSTK_DEVD_DELETE_FILTER = 0,
    NLSTK_DEVD_ADD_FILTER,
} NLSTK_DevdScanFilterAction_E;

typedef enum NLSTK_DevdScanStmEvent {
    NLSTK_DEVD_POTIENTIAL_UPDATE = 0,
    NLSTK_DEVD_START_OK,
    NLSTK_DEVD_START_ERR,
    NLSTK_DEVD_STOP_OK,
    NLSTK_DEVD_STOP_ERR,
    NLSTK_DEVD_INTERNAL_TIMEOUT,
    // 用户自定义事件值开始
    NLSTK_DEVD_START_USER_DEF_START = 10,
    // 用户自定义事件值结束
    NLSTK_DEVD_START_USER_DEF_END = NLSTK_DEVD_START_USER_DEF_START + 60,
} NLSTK_DevdScanStmEvent_E;

typedef enum NLSTK_DevdScanState {
    NLSTK_DEVD_STATE_STOPPED,
    NLSTK_DEVD_STATE_STARTING,
    NLSTK_DEVD_STATE_STARTED,
    NLSTK_DEVD_STATE_STOPPING,
    // 用户自定义状态开始
    NLSTK_DEVD_STATE_USER_DEF_START = 5,
    // 用户自定义状态结束
    NLSTK_DEVD_STATE_USER_DEF_END = NLSTK_DEVD_STATE_USER_DEF_START + 10,
} NLSTK_DevdScanState_E;

typedef struct {
    uint8_t uuid[SERVICE_UUID_LEN_128];
    uint16_t len;
    uint8_t data[0];
} NLSTK_DevdAdvServiceData_S;

typedef struct {
    uint8_t uuid[SERVICE_UUID_LEN_128];
} NLSTK_DevdAdvServiceUuid_S;

typedef struct {
    uint16_t manufacturerId;
    uint16_t len;
    uint8_t data[0];
} NLSTK_DevdAdvManufacturerData_S;

typedef struct {
    SDF_Vector_S *scannerIds;           // base data: uint32_t
    SLE_Addr_S addr;
    int8_t rssi;
    int8_t txPower;
    uint8_t frameType;
    uint8_t discoveryLevel;
    bool isConnectable;
    bool isPencil;
    NLSTK_VariableData_S localName;
    NLSTK_VariableData_S meshInfo;
    SDF_Vector_S *serviceDataList;      // base data: NLSTK_DevdAdvServiceData_S
    SDF_Vector_S *serviceUuids;         // base data: NLSTK_DevdAdvServiceUuid_S
    SDF_Vector_S *manufacturerDataList; // base data: NLSTK_DevdAdvManufacturerData_S
    NLSTK_VariableData_S advData;
} NLSTK_DevdAdvResult_S;

typedef void (*NLSTK_DevdStartOrStopScanEvent)(NLSTK_Errcode_E resultCode, bool isStartScan);
typedef void (*NLSTK_DevdScanReportCallback)(NLSTK_DevdAdvResult_S *result);

typedef struct {
    NLSTK_DevdStartOrStopScanEvent onStartOrStopEvent; // 允许为空
    NLSTK_DevdScanReportCallback onScanCallback;       // 允许为空
} NLSTK_DevdScanCbk_S;

typedef struct {
    bool hasAddr;
    SLE_Addr_S addr;
    NLSTK_VariableData_S name;

    bool hasServiceUuid;
    uint8_t serviceUuid[SERVICE_UUID_LEN_128];
    bool hasServiceUuidMask;
    uint8_t serviceUuidMask[SERVICE_UUID_LEN_128];
    bool hasSolicitationUuid;
    uint8_t serviceSolicitationUuid[SERVICE_UUID_LEN_128];
    bool hasSolicitationUuidMask;
    uint8_t serviceSolicitationUuidMask[SERVICE_UUID_LEN_128];

    NLSTK_VariableData_S serviceData;
    NLSTK_VariableData_S serviceDataMask;

    uint16_t manufacturerId;
    NLSTK_VariableData_S manufacturerData;
    NLSTK_VariableData_S manufacturerDataMask;

    bool hasRssiThreshold;
    int8_t rssiThreshold;
    bool isSensorHubChannel;
    bool advIndReport;
    bool meshInfoReport;

    // 下面字段上层应用不需要关注
    uint8_t filterIndex;
    uint8_t action;
    bool isNoFilter;
} NLSTK_DevdScanFilter_S;

typedef struct {
    int64_t reportDelayMillis;  // reportDelayMillis == 0 for report first matched
    int32_t duration;           // duration == 0 for never expired
    int32_t scanMode;           // scanMode refer to NLSTK_DevdScanMode_E
    bool legacy;
    int32_t phy;
    uint8_t frameType;          // refer to NLSTK_DevdScanFrameType_E
} NLSTK_DevdScanSetting_S;

typedef struct {
    uint16_t window;    // 扫描窗口，一个扫描间隔内实际处于接收扫描状态的时间
    uint16_t interval;  // 扫描间隔，上一次扫描开始到下一次扫描开始的时间差
    NLSTK_DevdScanSetting_S setting;
    struct timespec expireTime;
    bool hasDuration;
} NLSTK_DevdScanSettingInner_S;

typedef struct {
    uint32_t scannerId;
    uint8_t moduleId;
    NLSTK_DevdScanSettingInner_S scanSetting;
    NLSTK_DevdScanSettingInner_S frame4ScanSetting;
    SDF_Vector_S *filters;         // base data:NLSTK_DevdScanFilter_S
} NLSTK_DevdScanner_S;

typedef struct {
    bool used;
    NLSTK_DevdScanCbk_S scanCbk;
} NLSTK_DevdScanModule_S;

typedef struct {
    SDF_Vector_S *scanners;      // base data:NLSTK_DevdScanner_S
    NLSTK_DevdScanModule_S scanModule[DEVD_MAX_MODULE_ID];
    NLSTK_DevdScanSettingInner_S currentScanSetting;      // 当前已经起的扫描参数
    NLSTK_DevdScanSettingInner_S inflightScanSetting;     // 计算出待更新的扫描参数
    NLSTK_DevdScanSettingInner_S currentFrame4ScanSetting;
    NLSTK_DevdScanSettingInner_S inflightFrame4ScanSetting;
    int internalTimer;          // 启停扫描内部定时器
    int durationTimer;          // 扫描持续时间定时器
} NLSTK_DevdScanManager_S;

typedef struct {
    StateMachine base;
    NLSTK_DevdScanManager_S *scanManager;  // 模块初始化时，已提前创建，保证对象不为空
} NLSTK_DevdStateMachine_S;

#ifdef __cplusplus
}
#endif

#endif // NLSTK_DEVD_SCAN_TYPE_EXT_H