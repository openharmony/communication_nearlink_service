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
#ifndef DLI_DEF_H
#define DLI_DEF_H

#include <stdint.h>
#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SLE_ADV_INTV_SIZE 3
#define CHANNEL_MAP_LEN 10
#define DLI_LOCAL_FEATURES_LEN 10
#define SCAN_VALID_NUMS_OF_FRAME_TYPE_IND 2
#define CONN_VALID_MULTI_OF_FRAME_TYPE_IND 2
#define CONN_VALID_SINGLE_OF_FRAME_TYPE_IND 1

#define SLE_MEASURE_PM_24G_BAND_LEN 10
#define DEFAULT_CONN_PILOT 0x0F
#define MEASURE_CONFIG_DIRECT 0x90010004
#define DEFAULT_FRAME_FORMAT_M_SEQ_IND 0x3F

#define SLE_SHIFT_BITS_8 8
#define SLE_SHIFT_BITS_16 16
#define DEFAULT_FRAME_TYPE_1_ADV_TIMEOUT 200
#define DEFAULT_FRAME_TYPE_4_ADV_TIMEOUT 2000
#define DEFAULT_FRAME_4_CONN_INTERVAL 240

// 扫描帧类型bit定义
typedef enum {
    DLI_SCAN_BIT_FRAME_FORMAT_1_IND = 0x01,
    DLI_SCAN_BIT_FRAME_FORMAT_4_IND = 0x02
} DLI_ScanBitFrameTypeInd_E;

// 连接帧类型bit定义
typedef enum {
    CONN_BIT_FRAME_FORMAT_1_IND = 0x01,
    CONN_BIT_FRAME_FORMAT_4_IND = 0x08,
} DLI_ConnBitFrameTypeInd_E;

// 不做字节对齐 begin
typedef struct {
    uint16_t status;  /* ! 0为SUCCESS, 其他为异常 */
    uint8_t feats[DLI_LOCAL_FEATURES_LEN]; /*!< 特性表格式，按协议bit定义 */
} DLI_LocalFeatures_S;
// 不做字节对齐 end

#pragma pack(1)
typedef struct DLI_AddrStru {
    uint8_t addr[SLE_ADDR_LEN];
} DLI_AddrStru;

typedef struct DLI_AddrExtStru {
    uint8_t addr[SLE_ADDR_LEN];
    uint8_t ext;
} DLI_AddrExtStru;

typedef struct DLI_SetAdvertisingEnable {
    uint8_t enable;       /*!< 开启广播指示  */
    uint8_t advHandle;    /*!< 广播句柄，0x00~0x3f  */
    uint16_t duration;    /*!< 广播持续时间，10ms~655350ms */
    uint8_t maxAdvEvents; /*!< 最大扩展广播事件 */
} DLI_SetAdvertisingEnable;

typedef struct {
    uint8_t advHandle;                              /*!< 广播句柄 */
    uint8_t advMode;                                /*!< 广播模式 */
    uint8_t advGtRole;                              /*!< GT角色指示 */
    uint8_t primAdvIntervalMin[SLE_ADV_INTV_SIZE]; /*!< 基础广播周期最小值，0x000020~0xffffff，125us unit */
    uint8_t primAdvIntervalMax[SLE_ADV_INTV_SIZE]; /*!< 基础广播周期最大值，0x000020~0xffffff，125us unit */
    uint8_t primAdvChannelMap;                      /*!< 基础广播信道指示，0:76，1:77，2:78 */
    uint8_t ownAddrType;            /*!< 本端标识类型，public=0 / random=1 / rpa_or_pub=2 / rpa_or_rnd=3 */
    uint8_t peerAddrType;           /*!< 对端标识类型 */
    uint8_t ownAddr[SLE_ADDR_LEN];  /*!< 本端标识 */
    uint8_t peerAddr[SLE_ADDR_LEN]; /*!< 对端标识 */
    uint8_t advFilterPolicy;        /*!< 广播过滤策略 */
    int8_t advTxPower;              /*!< 广播发送功率:-127~20，127:host has no preference */
    uint8_t primAdvFrameFormat;     /*!< 基础广播无线帧类型，0: 无线帧类型1, 1: 无线帧类型4，m序列0 */
    uint8_t secondAdvFrameFormat;   /*!< 扩展广播无线帧类型 */
    uint8_t secondAdvPhy;           /*!< 扩展广播信道带宽，0:1M 1:2M 2:4M */
    uint8_t secondAdvPilot;         /*!< 扩展广播导频密度，0:4:1 1:8:1 2:16:1 3:NO */
    uint8_t secondAdvMcs;           /*!< 扩展广播调制编码参数 */
    uint8_t secondAdvMaxSkip;       /*!< 扩展广播发送时机 */
    uint8_t advSid;                 /*!< 广播集合标识 */
    uint8_t scanReqNotifEnable;     /*!< 查询请求通知配置 0:关闭 1:打开 */
    uint8_t scanReqRecvNumberMax;   /*!< 查询请求/接入请求最大数量 */
    uint16_t scanReqRxDurMax;       /*!< 接收查询请求/接入请求最大持续时间 */
    /*!< 连接参数，做G时有效 */
    uint16_t connIntervalMin;       /*!< 异步数据链路事件组周期最小值 */
    uint16_t connIntervalMax;       /*!< 异步数据链路事件组周期最大值 */
    uint16_t maxLatency;            /*!< 最大延迟周期 */
    uint16_t supervisionTimeout;    /*!< 超时时间 */
    uint16_t minCeLength;           /*!< 最小事件组长度 */
    uint16_t maxCeLength;           /*!< 最大事件组长度 */
} DLI_AdvParam;

typedef struct DLI_SetScanningParameter {
    uint8_t ownAddrType;           /*!< 本端地址类型，参考sle_addr_type定义 */
    uint8_t scanFilterPolicy;      /*!< 扫描过滤策略，参考sle_scan_filter_policy定义 */
    uint8_t scanPhys;              /*!< the length equals to the valid bits number of scanPhys * 5 octets */
    uint8_t typeIntervalWindow[0]; /*!< 扫描类型: 1，扫描间隔: 2，扫描窗口: 2 */
} DLI_SetScanningParameter;

typedef struct {
    uint8_t id;                         /* 用于标识一个ICB，取值范围[0x00,0xEF] */
    struct DLI_SetICBParam {
        uint16_t maxSduG2T;             /* G到T最大SDU Payload字节数 */
        uint16_t maxSduT2G;             /* T到G最大SDU Payload字节数 */
        uint8_t rtnG2T;                 /* G到T每一个IMB数据PDU重传次数 */
        uint8_t rtnT2G;                 /* T到G每一个IMB数据PDU重传次数 */
    } param[0];
} DLI_SetICBParameter;

typedef struct {
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint8_t sduIntervalG2T[3];          /*!< G到T两个连续SDU之间的时间间隔，us */
    uint8_t sduIntervalT2G[3];          /*!< T到G两个连续SDU之间的时间间隔，us */
    uint8_t sca;                        /*!< 睡眠时钟精度 */
    uint8_t packing;                    /*!< 当前仅支持交叉 */
    uint8_t framing;                    /*!< 当前仅支持未切分 */
    uint16_t maxLatencyG2T;             /*!< G到T最大传输延迟，ms */
    uint16_t maxLatencyT2G;             /*!< T到G最大传输延迟，ms */
    uint8_t icbCnt;
    uint8_t paramCnt;
    uint8_t icbParam[0];
} DLI_SetICGParameter;

typedef struct {
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint8_t sduIntervalG2T[3];          /* G到T两个连续SDU之间的时间间隔，us */
    uint8_t sduIntervalT2G[3];          /* T到G两个连续SDU之间的时间间隔，us */
    uint8_t ftG2T;                      /*!< G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint8_t ftT2G;                      /*!< T到G方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint16_t icbInterval;               /*!< 连续两个IMB Anchor Point的间隔时间，
                                            取值范围[0x0014,0x3E80]，时间 = N * 0.25ms，时间范围[5ms,4s] */
    uint8_t sca;                        /* 睡眠时钟精度 */
    uint8_t packing;                    /* 当前仅支持交叉 */
    uint8_t framing;                    /* 当前仅支持未切分 */
    uint8_t paramCnt;
    uint8_t param[0];
} DLI_SetICGTestParameter;

typedef struct {
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint8_t channelCnt;
    uint8_t channel[0];
} DLI_CreateICBParameter;

typedef struct {
    uint16_t connHandle;
    uint8_t reason;
} DLI_RejectICBReqParameter;

typedef struct {
    uint16_t connHandle;                 /* 单个同步链路handler */
    uint8_t direction;                   /* 0： host到controller，1：controller到host */
    uint8_t pathId;                      /* 0：数据通过HCI传输，其他值：通过Vendor专用传输 */
    struct {
        uint8_t codecId;                 /* 0xff表示使用vendor codec */
        uint16_t vendorId;               /* 仅codecId为0xff时有效 */
        uint16_t vendorCodecId;          /* 仅codecId为0xff时有效 */
    } codec;
    uint8_t controllerDelay[3];          /* Controller延迟时间，以us为单位，取值范围[0x000000,0x3D0900]，时间范围[0s,4s] */
    uint8_t codecConfigLen;
    uint8_t codecConfigData[0];
} DLI_ICBDataPathParameter;

typedef struct DLI_ScanParamCoreNoPhy {
    uint8_t scanType;      /*!< 扫描类型，参考sle_scan_type定义 */
    uint16_t scanInterval; /*!< 扫描间隔，取值范围[0x0004, 0xFFFF]，time = N * 0.125ms */
    uint16_t scanWindow;   /*!< 扫描窗口，取值范围[0x0004, 0xFFFF]，time = N * 0.125ms */
} DLI_ScanParamCoreNoPhy;

typedef struct DLI_ScanParam {
    uint8_t ownAddrType;      /*!< 本端地址类型，参考sle_addr_type定义 */
    uint8_t scanFilterPolicy; /*!< 扫描过滤策略，参考sle_scan_filter_policy定义 */
    uint8_t frameFormatInd;   /*!< 无线帧类型指示，bit0:帧1，bit1:帧四 */
    DLI_ScanParamCoreNoPhy param[0];
} DLI_ScanParam;

typedef struct {
    uint8_t mediaAccessLayerIDType; /*!< 媒体接入层标识类型 */
} DLI_PublicAddrParam;

typedef struct {
    uint16_t connHandle;
    uint8_t configId;                   /*!< 位置测量信号配置索引 */
    uint8_t schedulingTimeslot;         /*!< 系统调度时隙 */
    uint8_t rttPhy;                     /*!< 测量信号带宽指示 */
    uint8_t freqHoppingMode;            /*!< 跳频方式指示 */
    uint16_t fmFreq;                    /*!< 初始化阶段的频点 */
    uint16_t fmInteractionType;         /*!< 初始化阶段交互类型指示，和glp有关 stepmode */
    uint16_t occurrenceGroupPeriod;     /*!< 事件组周期 */
    uint16_t fmOccurrenceGroupInterval; /*!< 初始化阶段事件间间隔, t_fc */
    uint16_t pmMeasureType1Interval;    /*!< 测量帧类型1 事件间间隔 t_pm_type1 */
    uint16_t pmMeasureType2Interval;    /*!< 测量帧类型2 事件间间隔 t_pm_type2 */
    uint16_t fmTIp1Time;                /*!< 初始化阶段事件内间隔 t_ip1 120 */
    uint16_t pmTIp2Time;                /*!< 事件内间隔 t_ip2 */
    uint8_t fmTGuard;                   /*!< 初始化阶段的测量帧的内部切换间隔  t_guard 默认配10 */
    uint8_t fmSignal2Length;            /*!< 初始化阶段的测量信号2的长度,t_fm */
    uint8_t pmInitAntCount;             /*!< 先发节点天线数量指示 */
    uint8_t pmInitSignal2Tone;          /*!< 先发链路测量信号2 多音指示 */
    uint8_t pmReflAntCount;             /*!< 后发节点天线数量指示 */
    uint8_t pmReflSignal2Tone;          /*!< 后发链路测量信号2 多音指示 */
    uint8_t pmFreqHoppingBand;          /*!< 跳频频带 */
    uint8_t pm2400mBand[SLE_MEASURE_PM_24G_BAND_LEN]; /* 2.4GHz 跳频信道指示 */
    uint8_t glpMode;
    uint8_t sleHadmMode;  /*!< slehadm的模式 */
    uint8_t isCsParamChg; /*!< 是否需要更改测距连接参数，0：不需要更改，1：需要更改（首次设置参数需置0）[车钥匙应用] */
    uint8_t freqSpace;    /*!< 频率间隔， 0：1M， 1：2M */
    uint8_t conAnchorNum; /*!< 需要连接的锚点数量 */
    uint8_t refreshRate;  /*!< 刷新率,1：高频刷新，2：中频刷新；4：低频刷新 */
    uint16_t acbInterval; /*!< 根据参数计算的acb周期 */
    uint16_t csInterval;  /*!< 根据参数计算的测距周期 */
} DLI_MeasureConfigExtParam;   // 与HadmSoundingParam_S相差一个connHandle

#pragma pack()

typedef void (*DLI_InnerExecuteCmdCbk)(void *context, void *arg, uint32_t len, uint16_t evtOpcode);

typedef struct DLI_InnerCbkLineStru {
    uint16_t opcode; // 完成事件是动态注册的，opcode=cmd；其他事件在DLI初始化时注册，opcode=event
    DLI_InnerExecuteCmdCbk func;
} DLI_InnerCbkLineStru;

typedef struct DLI_InnerCbkNodeStru {
    SDF_DListEntry_S entry;
    const DLI_InnerCbkLineStru *table;
    uint32_t size;
} DLI_InnerCbkNodeStru;

#define SLE_OPTYPE_BITS 0x0000FF00

typedef enum : uint8_t {
    CONNECTABLE_ADV_PACKET = 0,
    SCANNABLE_ADV_PACKET,
    DIRECTED_ADV_PACKET,
    CONNECTABLE_AND_SCANNABLE_ADV_PACKET,
} DLI_AdvMode;

typedef enum : uint8_t {
    SCAN_FRAME_TYPE_1 = 0x01,
    SCAN_FRAME_TYPE_4 = 0x02,
} DLI_ScanFrameType;

typedef enum : uint8_t {
    PRIM_ADV_FRAME_TYPE_1 = 0,
    PRIM_ADV_FRAME_TYPE_4_M_0,
    PRIM_ADV_FRAME_TYPE_4_M_1,
    PRIM_ADV_FRAME_TYPE_4_M_2,
    PRIM_ADV_FRAME_TYPE_4_M_3,
    PRIM_ADV_FRAME_TYPE_4_M_4,
    PRIM_ADV_FRAME_TYPE_4_M_5,
} DLI_PrimAdvFrameType;

typedef enum : uint8_t {
    SECOND_ADV_FRAME_TYPE_1 = 0,
    SECOND_ADV_FRAME_TYPE_2,
    SECOND_ADV_FRAME_TYPE_3_M_0,
    SECOND_ADV_FRAME_TYPE_3_M_1,
    SECOND_ADV_FRAME_TYPE_3_M_2,
    SECOND_ADV_FRAME_TYPE_3_M_3,
    SECOND_ADV_FRAME_TYPE_3_M_4,
    SECOND_ADV_FRAME_TYPE_3_M_5,
    SECOND_ADV_FRAME_TYPE_4_M_0,
    SECOND_ADV_FRAME_TYPE_4_M_1,
    SECOND_ADV_FRAME_TYPE_4_M_2,
    SECOND_ADV_FRAME_TYPE_4_M_3,
    SECOND_ADV_FRAME_TYPE_4_M_4,
    SECOND_ADV_FRAME_TYPE_4_M_5,
} DLI_SecondAdvFrameType;

#define DLI_UNUSED(var) do { \
    (void)(var); \
} while (0)

#define DLI_DECODE2BYTE(_ptr) (uint16_t)(*(uint8_t *)(_ptr) | (*(uint8_t *)((_ptr) + 1) << 8))
#define DLI_ENCODE2BYTE(_ptr, value) \
do { \
    *((uint8_t *)(_ptr) + 1) = (uint8_t)((uint16_t)(value) >> 8); \
    *(uint8_t *)(_ptr) = (uint8_t)(value); \
} while (0)

#define DLI_ENCODE2BYTE_LITTLE(_ptr, data)                           \
do {                                                             \
    *((uint8_t *)(_ptr) + 1) = (uint8_t)((uint16_t)(data) >> 8); \
    *(uint8_t *)(_ptr) = (uint8_t)(data);                        \
} while (0)

#define DLI_ENCODE3BYTE_LITTLE(_ptr, value)                          \
do {                                                             \
    *((uint8_t *)(_ptr) + 2) = (uint8_t)((value) >> 16);         \
    *((uint8_t *)(_ptr) + 1) = (uint8_t)((value) >> 8);          \
    *(uint8_t *)(_ptr) = (uint8_t)(value);                       \
} while (0)

#ifdef __cplusplus
}
#endif
#endif