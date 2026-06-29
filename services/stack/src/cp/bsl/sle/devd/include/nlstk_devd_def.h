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
#ifndef NLSTK_DEVD_DEF_H
#define NLSTK_DEVD_DEF_H

#include <stdint.h>
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEVD_ADV_INTERVAL_MIN                   0x0000A0         /*!< 最小广播间隔 */
#define DEVD_ADV_INTERVAL_MAX                   0xFFFFFF         /*!< 最大广播间隔 */


#define SUPPORT_PHY_MAX                    3                /*!< 支持的带宽类型最大数量 */
#define SCAN_INTERVAL_MIN                  0x0004           /*!< 扫描间隔最小值 */
#define SCAN_WINDOW_MIN                    0x0004           /*!< 扫描窗口最小值 */
#define CONN_INTV_MIN                      0x6              /*!< 连接间隔最小值 (N*1.250ms) */
#define CONN_INTV_MAX                      0x3E80           /*!< 连接间隔最大值 (N*1.250ms) */
#define CONN_MAX_LATENCY                   0x1F3            /*!< 最大休眠连接间隔 */
#define CONN_SUPERVISION_TIMEOUT_MIN       0xA              /*!< 连接超时时间最小值 (N*10ms) */
#define CONN_SUPERVISION_TIMEOUT_MAX       0xC80            /*!< 连接超时时间最大值 (N*10ms) */

#define ADV_TX_POWER_MIN                   (-127)           /*!< 最小发送功率 */
#define ADV_TX_POWER_DEFAULT               0x7F             /*!< 缺省发送功率 */
#define ADV_TX_POWER_MAX                   21               /*!< 最大发送功率 */

#define ADV_MAX_MCS 16

#pragma pack (1)

/**
 * @brief 广播数据
 */
typedef struct {
    uint16_t advDataLen;     /* 广播数据长度 */
    uint16_t scanRspDataLen; /* 扫描响应数据长度 */
    uint8_t *advData;        /* 广播数据 */
    uint8_t *scanRspData;    /* 扫描响应数据 */
} NLSTK_DevdAdvData_S;

/**
 * @brief 广播数据设置集
 */
typedef struct {
    uint8_t advHandle;   /* 广播句柄 */
    NLSTK_DevdAdvData_S data; /* 广播数据 */
} NLSTK_DevdSetAdvData_S;

/**
 * @brief  接入层类型指示
 */
typedef enum {
    ADV_ACCESS_MODE_SLB     = 1,                        /*!< SLB */
    ADV_ACCESS_MODE_SLE     = 2,                        /*!< SLE */
    ADV_ACCESS_MODE_DEFAULT = 3,                        /*!< 缺省, 基础应用层不明确指示 */
} NLSTK_DevdAdvAccessMode_E;

/**
 * @brief  发现等级
 */
typedef enum {
    ADV_DISCOVERY_LEVEL_NONE,                           /*!< 不可见发现 */
    ADV_DISCOVERY_LEVEL_NORMAL,                         /*!< 一般可发现 */
    ADV_DISCOVERY_LEVEL_PRIORITY,                       /*!< 优先可发现 */
    ADV_DISCOVERY_LEVEL_PAIRED,                         /*!< 被配对过的设备发现 */
    ADV_DISCOVERY_LEVEL_SPECIAL,                        /*!< 被指定设备发现 */
} NLSTK_DevdAdvDiscoveryLevel_E;

/**
 * @brief  广播类型
 */
typedef enum {
    ADV_MODE_NONCONN_NONSCAN      = 0x00,          /* 不可连接不可扫描 */
    ADV_MODE_CONNECTABLE_NONSCAN  = 0x01,          /* 可连接不可扫描 */
    ADV_MODE_NONCONN_SCANABLE     = 0x02,          /* 不可连接可扫描 */
    ADV_MODE_CONNECTABLE_SCANABLE = 0x03,          /* 可连接可扫描 */
    ADV_MODE_CONNECTABLE_DIRECTED = 0x07,          /* 可连接可扫描，定向 */
} NLSTK_DevdAdvMode_E;

/**
 * @brief  G/T 角色协商指示
 */
typedef enum {
    ADV_GT_ROLE_T_CAN_NEGO = 0,   /* 期望做T可协商 */
    ADV_GT_ROLE_G_CAN_NEGO,       /* 期望做G可协商 */
    ADV_GT_ROLE_T_NO_NEGO,        /* 期望做T不可协商 */
    ADV_GT_ROLE_G_NO_NEGO         /* 期望做G不可协商 */
} NLSTK_DevdAdvGtRole_E;

/**
 * @brief  信道映射
 */
typedef enum {
    ADV_CHANNEL_MAP_76                 = 0x00,
    ADV_CHANNEL_MAP_77                 = 0x01,
    ADV_CHANNEL_MAP_78                 = 0x02,
    ADV_CHANNEL_MAP_79                 = 0x04,
    ADV_CHANNEL_MAP_DEFAULT            = 0x07
} NLSTK_DevdAdvChannelMap_E;

/**
 * @brief  广播间隔
 */
typedef enum {
    // Minimum value for advertising interval. 0x0000A0(160) * 125 = 20000us = 20ms
    ADV_INTERVAL_MIN = 0x0000A0,
    // Set connectable sle for advertising interval. 0x0004B0(1200) *125 = 150000us = 150ms
    ADV_SLE_CONNECTABLE_ADV_INTERBAL = 0x0004B0,
    // Maximum value for advertising interval.
    ADV_INTERVAL_MAX = 0xFFFFFF,
    // Default value for advertising interval. 0x001388(5000) * 125 = 625000us = 625ms
    ADV_INTERVAL_DEFAULT = 0x001388
} NLSTK_DevdAdvInterval_E;

/**
 * @brief  连接参数，做G时有效
 */
typedef struct {
    uint16_t intervalMin;                     /*!< 连接间隔的最小取值，取值范围[0x001E,0x3E80]，时间 = N * 0.25ms, 时间范围是[7.5ms,4s] */
    uint16_t intervalMax;                     /*!< 连接间隔的最大取值，取值范围[0x001E,0x3E80]，时间 = N * 0.25ms, 时间范围是[7.5ms,4s] */
    uint16_t maxLatency;                      /*!< 最大休眠连接间隔，取值范围[0x0000,0x01F3]，默认0x0000 */
    uint16_t supervisionTimeout;              /*!< 最大超时时间，取值范围[0x000A,0x0C80]，时间 = N * 10ms，时间范围是[100ms,32s] */
    uint16_t minCeLength;                     /*!< 推荐的连接事件的最小取值，取值范围[0x0000,0xFFFF]，时间 = N * 0.125ms */
    uint16_t maxCeLength;                     /*!< 推荐的连接事件的最大取值，取值范围[0x0000,0xFFFF]，时间 = N * 0.125ms */
} NLSTK_DevdConnParam_S;

typedef struct {
    uint8_t mod;
    uint8_t polar;
} NLSTK_DevdMcsToModAndPolar_S;

typedef enum {
    ADV_MOD_GFSK = 0x00,
    ADV_MOD_16QAM = 0x01,
    ADV_MOD_QPSK = 0x02,
    ADV_MOD_8PSK = 0x03,
} NLSTK_DevdAdvMod_E;

typedef enum {
    ADV_POLAR_NO = 0x00,
    ADV_POLAR_1_4 = 0x02,
    ADV_POLAR_3_8 = 0x03,
    ADV_POLAR_1_2 = 0x04,
    ADV_POLAR_5_8 = 0x05,
    ADV_POLAR_3_4 = 0x06,
    ADV_POLAR_7_8 = 0x07,
} NLSTK_DevdAdvPolar_E;

/**
 * @brief  调制模式
 */
typedef enum {
    ADV_MCS_00 = 0,                                     /*!< MCS0:  BPSK1/4 */
    ADV_MCS_01,                                         /*!< MCS1:  BPSK3/8 */
    ADV_MCS_02,                                         /*!< MCS2:  QPSK1/4 */
    ADV_MCS_03,                                         /*!< MCS3:  QPSK3/8 */
    ADV_MCS_04,                                         /*!< MCS4:  QPSK1/2 */
    ADV_MCS_05,                                         /*!< MCS5:  QPSK5/8 */
    ADV_MCS_06,                                         /*!< MCS6:  QPSK3/4 */
    ADV_MCS_07,                                         /*!< MCS7:  QPSK7/8 */
    ADV_MCS_08,                                         /*!< MCS8:  QPSK 1 */
    ADV_MCS_09,                                         /*!< MCS9:  8PSK5/8 */
    ADV_MCS_10,                                         /*!< MCS10: 8PSK3/4 */
    ADV_MCS_11,                                         /*!< MCS11: 8PSK7/8 */
    ADV_MCS_12,                                         /*!< MCS12：8PSK 1 */
    ADV_MCS_MAX,
} NLSTK_DevdAdvMcs_E;

/**
 * @brief  pilot码率
 */
typedef enum {
    ADV_PILOT_RATIO_4_1                = 0x00,
    ADV_PILOT_RATIO_8_1,
    ADV_PILOT_RATIO_16_1,
    ADV_PILOT_RATIO_NO,
} NLSTK_DevdAdvPilot_E;

/**
 * @brief  广播帧类型
 */
typedef enum {
    ADV_FRAME_TYPE_GFSK                = 0x00,          /*!< 帧类型为GFSK */
    ADV_FRAME_TYPE_SHORT_HEADER        = 0x01,          /*!< 帧类型为短帧 */
} NLSTK_DevdAdvFrame_E;

/**
 * @brief  过滤策略
 */
typedef enum {
    ADV_FLT_ANY_SCAN_ANY_CONNECT,                       /*!< 接受所有scan_req/conn_req */
    ADV_FLT_ALLOW_SCAN_ANY_CONNECT,                     /*!< 只接受符合过滤器的scan_req */
    ADV_FLT_ANY_SCAN_ALLOW_CONNECT,                     /*!< 只接受符合过滤器的conn_req */
    ADV_FLT_ALLOW_SCAN_ALLOW_CONNECT                    /*!< 接受符合过滤器的scan_req/conn_req */
} NLSTK_DevdAdvFilterPolicy_E;

/**
 * @brief  广播带宽
 */
typedef enum {
    ADV_PHY_1M                         = 0x00,
    ADV_PHY_2M                         = 0x01,
    ADV_PHY_4M                         = 0x02,
} NLSTK_DevdAdvPhy_E;

/**
 * @brief  广播无线帧类型指示
 */
typedef enum {
    ADV_FRAME_TYPE_1 = 0x00,
    ADV_FRAME_TYPE_4 = 0x01,
} NLSTK_DevdAdvFrameType_E;

/**
 * @brief  广播使能高功率选项
 */
typedef enum {
    ADV_HIGH_POWER_DISABLE = 0x00,  // 不使能高功率
    ADV_HIGH_POWER_ENABLE = 0x01,   // 使能高功率
} NLSTK_DevdAdvEnableHighPower_E;

/**
 * @brief  广播信道参数
 */
typedef struct {
    uint8_t primAdvPhy;                      /*!< 基础广播带宽, 参见NLSTK_DevdAdvPhy_E定义 */
    uint8_t secondAdvFrame;                  /*!< 数据广播帧类型, 参见NLSTK_DevdAdvFrame_E定义 */
    uint8_t secondAdvPhy;                    /*!< 数据广播带宽, 参见NLSTK_DevdAdvPhy_E定义 */
    uint8_t secondAdvPilot;                  /*!< 数据广播pilot码率, 参见NLSTK_DevdAdvPilot_E定义 */
    uint8_t secondAdvMcs;                    /*!< 数据广播编码模式, 参见NLSTK_DevdAdvMcs_E定义 */
    uint8_t secondAdvMaxSkip;                /*!< 0: 优先发送数据广播, [0x1,0xFF]:发送数据广播前可跳过的最大evt数 */
} NLSTK_DevdAdvPhyParam_S;

/**
 * @brief  广播扫描参数
 */
typedef struct {
    uint8_t  scanReqNotifEnable;             /*!< 是否上报收到的scan req,0: 不上报收到的scan req; 1:上报收到的scan req */
    uint8_t  scanReqRecvNumberMax;           /*!< 一个广播周期收scan_req数量 */
    uint16_t scanReqRxDurMax;                /*!< 一个广播周期收scan_req最大时间 */
} NLSTK_DevdAdvScanParam_S;

/**
 * @brief  广播扩展参数
 */
typedef struct {
    uint8_t advFilterPolicy;                 /*!< 广播过滤, 参见NLSTK_DevdAdvFilterPolicy_E定义 */
    int8_t  advTxPower;                      /*!< 广播发送功率, 单位dbm, 取值范围[-127, 20], 0x7F：不设置特定发送功率 */
    uint8_t advSid;                          /*!< 广播分组, 取值范围[0x0, 0xFF] */
    NLSTK_DevdAdvPhyParam_S phyParam;         /*!< 广播信道参数 */
    NLSTK_DevdAdvScanParam_S scanParam;       /*!< 广播扫描参数 */
} NLSTK_DevdAdvExtParam_S;

/**
 * @brief  广播参数
 */
typedef struct {
    uint8_t  advHandle;                       /*!< 广播句柄，取值范围[0, 0xFF] */
    uint8_t  advMode;                         /*!< 广播类型，参见NLSTK_DevdAdvMode_E定义 */
    uint8_t  advGtRole;                       /*!< G/T 角色协商指示,参见NLSTK_DevdAdvGtRole_E定义 */
    uint32_t advIntervalMin;                  /*!< 最小广播周期, 0x000020~0xffffff, 单位125us */
    uint32_t advIntervalMax;                  /*!< 最大广播周期, 0x000020~0xffffff, 单位125us */
    uint8_t  advChannelMap;                   /*!< 广播信道, 参见NLSTK_DevdAdvChannelMap_E,  0:76, 1:77, 2:78 */
    SLE_Addr_S ownAddr;                       /*!< 本端地址 */
    SLE_Addr_S peerAddr;                      /*!< 对端地址 */
    uint8_t primaryFrameType;                 /*!< 基础广播无线帧类型，0: 无线帧类型1, 1: 无线帧类型4，m序列0 */
    NLSTK_DevdAdvExtParam_S *extParam;          /*!< 扩展广播参数, 默认配置，用户可自定义 */
    NLSTK_DevdConnParam_S *connParam;           /*!< advGtRole为ADV_GT_ROLE_T_NO_NEGO时置空, 其它模式需配置 */
} NLSTK_DevdAdvParam_S;

/**
 * @brief  广播设置参数集
 */
typedef struct {
    uint8_t accessMode;                       /*!< 接入层类型指示, 参见NLSTK_DevdAdvAccessMode_E */
    uint8_t discoveryLevel;                   /*!< 发现等级, 参见NLSTK_DevdAdvDiscoveryLevel_E */
    NLSTK_DevdAdvParam_S param;                /*!< 广播参数 */
    NLSTK_DevdAdvData_S  data;                 /*!< 广播数据 */
} NLSTK_DevdSetAdvParams_S;

/**
 * @brief 广播使能参数
 */
typedef struct {
    uint8_t advHandle;   /* 广播句柄 */
    uint8_t enable;      /* 0x0:停止广播 0x1:开启广播 */
    uint16_t duration;   /* 0x0:广播时间无限制 0x1~0xffff:广播时间 = N*10ms */
    uint8_t maxAdvEvent; /* 0x0:广播事件数量无限制 0x1~0xff:最大广播事件个数 */
} NLSTK_DevdSetAdvEnable_S;

/**
 * @brief 发送功率参数
 */
typedef struct {
    int8_t bleMaxPower; /* BLE发送功率值  */
    int8_t sleMaxPower; /* SLE发送功率值  */
} NLSTK_DevdSetTxPower_S;

/**
 * @brief  扫描过滤策略
 */
typedef enum {
    SCAN_FLT_BASIC_NONE          = 0x00,     /*!< 基本不过滤的扫描过滤策略 */
    SCAN_FLT_BASIC               = 0x01,     /*!< 基本过滤的扫描过滤策略 */
    SCAN_FLT_EXTEND_NONE         = 0x02,     /*!< 扩展不过滤的扫描过滤策略 */
    SCAN_FLT_EXTEND              = 0x03,     /*!< 扩展过滤的扫描过滤策略 */
} NLSTK_DevdScanFilterPolicy_E;

/**
 * @brief  扫描类型
 */
typedef enum {
    SCAN_TYPE_PASSIVE       = 0x00,
    SCAN_TYPE_ACTIVE        = 0x01,
} NLSTK_DevdScanType_E;

/**
 * @brief  扫描带宽
 */
typedef enum {
    SCAN_PHY_1M             = 0x01,
    SCAN_PHY_2M             = 0x02,
    SCAN_PHY_4M             = 0x04,
} NLSTK_DevdScanPhy_E;

/**
 * @brief  扫描请求上报使能
 */
typedef enum {
    SCAN_REQ_NO_REPORT,
    SCAN_REQ_REPORT
} NLSTK_DevdScanReqNotifEnable_E;

/**
 * @brief 单PHY扫描参数
 */
typedef struct {
    uint8_t scanType;              /*!< 扫描类型，参考NLSTK_DevdScanType_E定义 */
    uint16_t scanInterval;         /*!< 扫描间隔，取值范围[0x0004, 0xFFFF]，time = N * 0.125ms */
    uint16_t scanWindow;           /*!< 扫描窗口，取值范围[0x0004, 0xFFFF]，time = N * 0.125ms */
} NLSTK_DevdSleScanParamsNoPhy_S;

/**
 * @brief 扫描参数设置
 */
typedef struct {
    uint8_t localAddrType;    /*!< 本端地址类型 */
    uint8_t scanFilterPolicy; /*!< 扫描过滤策略，参考NLSTK_DevdScanFilterPolicy_E定义 */
    uint8_t frameType;        /*!< 无线帧类型指示 */
    NLSTK_DevdSleScanParamsNoPhy_S params[0];
} NLSTK_DevdSleScanParams_S;

/**
 * @brief 扫描使能
 */
typedef struct {
    uint8_t scanEnable;                 /*!< 使能开关，0：关闭，1：开启 */
    uint8_t scanFilterDuplicates;       /*!< 重复过滤开关，0：关闭，1：开启 */
} NLSTK_DevdSleScanEnable_S;

/**
 * @brief 下发扫描过滤器
 */

typedef struct {
    uint8_t event;
    uint8_t advHandle;
    uint8_t result;
} NLSTK_DevdAdvCbkParam_S;

/**
 * @brief  广播事件回调函数
 */
typedef void (*NLSTK_DevdAdvEventCbk)(NLSTK_DevdAdvCbkParam_S *param);

/**
 * @brief  扫描结果上报扩展参数
 */
typedef struct {
    uint8_t eventType;                  /*!< 上报event类型 */
    uint8_t dataStatus;                 /*!< 数据状态 */
    uint8_t directAddrType;             /*!< 直接地址类型 */
    uint8_t directAddr[SLE_ADDR_LEN];   /*!< 直接地址 */
    uint8_t primFrameType;              /*!< 第一广播信道无线帧类型；0：帧一；1：帧四 */
    uint8_t primPhy;                    /*!< 广播在第一信道上使用的PHY */
    uint8_t secondPhy;                  /*!< 广播在第二信道上使用的PHY */
    uint8_t secondFrameType;            /*!< 广播在第二信道上使用的无线帧类型，0：GFSK，1：短帧 */
    uint8_t secondMod;                  /*!< 广播在第二信道上使用的调制 */
    uint8_t secondPilotRatio;           /*!< 广播在第二信道上使用的导频码率 */
    uint8_t secondPolar;                /*!< 广播在第二信道上使用的Polar编码码率 */
} NLSTK_DevdAdvReportExtendParams_S;

/**
 * @brief  扫描结果报告单设备信息
 */
typedef struct {
    uint8_t addrType;                   /*!< 地址类型 */
    uint8_t addr[SLE_ADDR_LEN];        /*!< 地址 */
    uint8_t rssi;                       /*!< 信号强度指示，取值范围[-127dBm, 20dBm]，0x7F表示不提供信号强度指示 */
    NLSTK_DevdAdvReportExtendParams_S extendParams;
    uint8_t dataLength;                 /*!< adv report数据长度 */
    uint8_t data[0];                    /*!< adv report数据 */
} NLSTK_DevdAdvReportInfo_S;

/**
 * @brief  扫描过滤器信息
 */
typedef struct {
    uint8_t status;
    uint8_t subCode;
    uint8_t numAvailable;
} NLSTK_DevdScanFilterInfo_S;

/**
 * @brief  扫描事件回调函数
 */
typedef void (*NLSTK_DevdScanEventCbk)(uint8_t eventMsg, uint8_t result);

/**
 * @brief  扫描数据上报事件回调函数
 */
typedef void (*NLSTK_DevdAdvReportCbk)(NLSTK_DevdAdvReportInfo_S *report);

/**
 * @brief  扫描过滤器数据上报回调函数
 * @param  [in]  <info> 上报的扫描过滤器信息
 */
typedef void (*NLSTK_DevdScanFilterCbk)(NLSTK_DevdScanFilterInfo_S *info);

/**
 * @brief  设备发现模块扫描回调函数集
 */
typedef struct {
    NLSTK_DevdScanEventCbk scanCbk;        /*!< 扫描事件回调函数 */
    NLSTK_DevdAdvReportCbk reportCbk;      /*!< 扫描数据上报事件回调函数 */
    NLSTK_DevdScanFilterCbk scanFilterCbk; /*!< 扫描过滤器数据上报回调函数 */
} NLSTK_DevdSleScanExterCbk_S;

#pragma pack ()

#ifdef __cplusplus
}
#endif
#endif
