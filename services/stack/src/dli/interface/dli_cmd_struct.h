/**
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
 * dli命令结构体定义
 *
 ***************************************************************************/

#ifndef DLI_CMD_STRUCT_H
#define DLI_CMD_STRUCT_H

#include "dli_opcode.h"
#include "dli_def.h"
#include "sdf_struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SLE_SM_LINK_KEY_LEN 16
#define SLE_MEASURE_PM_24G_BAND_LEN 10
#define SLE_SM_PLAINTEXT_LEN 16
#define SLE_SM_ENCRYPT_KEY_LEN 16
#if defined(TV_STANDARD) || defined(SLE_POWER_MANAGER_SUPPORT)
#define SLE_DATA_FILTER_CODE_LEN 8
#endif
#define SLE_IMG_ENCRYPTION_IV_LEN 8
#define SLE_IMG_ENCRYPTION_GROUP_KEY_LEN 16

#define SLE_INDEX_0 0
#define SLE_INDEX_1 1
#define SLE_INDEX_2 2
#define SLE_INDEX_5 5

#define SLE_ACB_SUBRATE_MIN 1
#define SLE_ACB_SUBRATE_AUDIO 6
#define SLE_ACB_SUBRATE_MAX 48
#define SLE_ACB_MAX_LATENCY 0
#define SLE_ACB_CONTINUATION_NUM 0
#define SLE_ACB_SUPERVISION_TIMEOUT 500
#define SLE_ACB_SUPERVISION_TIMEOUT_MAX 1000

#define SLE_LOCAL_SUBRATE_FEATURE 16
#define SLE_LOCAL_AUTORATE_FEATURE 23

#define SLE_ICB_MAX_NUM 16

/* ------------------------------------------发送命令相关结构体----------------------------------------------------------*/
#pragma pack(1)
typedef struct DLI_AdvPhyParam {
    uint8_t primAdvFrameFormat; /*!< 基础广播无线帧类型，0: 无线帧类型1, 1: 无线帧类型4，m序列0 */
    uint8_t primAdvPhy;         /*!< 基础广播信道带宽，参见sle_adv_phy定义 */
    uint8_t secondAdvFrameFormat; /*!< 扩展广播无线帧类型，参见sle_adv_frame定义 */
    uint8_t secondAdvPhy;         /*!< 扩展广播信道带宽，参见sle_adv_phy定义 */
    uint8_t secondAdvPilot;       /*!< 扩展广播导频密度，参见sle_adv_pilot定义 */
    uint8_t secondAdvMcs;         /*!< 扩展广播调制编码参数，参见sle_adv_mcs定义 */
    uint8_t secondAdvMaxSkip; /*!< 扩展广播发送时机，0: 优先发送数据广播，[0x1,0xFF]:发送数据广播前可跳过的最大evt数 */
} DLI_AdvPhyParam;

typedef struct DLI_AdvScanParam {
    uint8_t scanReqNotifEnable; /*!< 是否上报收到的scan req，0: 不上报收到的scan req; 1:上报收到的scan req */
    uint8_t scanReqRecvNumberMax; /*!< 一个广播周期收scan_req数量 */
    uint16_t scanReqRxDurMax;     /*!< 一个广播周期收scan_req最大时间 */
} DLI_AdvScanParam;

typedef struct DLI_AdvExtParam {
    uint8_t advFilterPolicy; /*!< 广播过滤，参见sle_adv_filter_policy定义 */
    int8_t advTxPower; /*!< 广播发送功率，单位dbm，取值范围[-127, 20]，0x7F：不设置特定发送功率 */
    uint8_t advSid;    /*!< 广播分组，取值范围[0x0, 0xFF] */
    DLI_AdvPhyParam phyParam;   /*!< 广播信道参数 */
    DLI_AdvScanParam scanParam; /*!< 广播扫描参数 */
} DLI_AdvExtParam;

/**
 * @brief  连接参数，做G时有效
 */
typedef struct DLI_ConnParam {
    uint16_t connIntervalMin; /*!< 异步数据链路事件组周期最小值，[0x001E,0x3E80]，时间 = N * 0.25ms，
                                 时间范围是[7.5ms,4s] */
    uint16_t connIntervalMax; /*!< 异步数据链路事件组周期最大值，[0x001E,0x3E80]，时间 = N * 0.25ms，
                                 时间范围是[7.5ms,4s] */
    uint16_t maxLatency;      /*!< 最大延迟周期，取值范围[0x0000,0x01F3]，默认0x0000 */
    uint16_t supervisionTimeout; /*!< 超时时间，取值范围[0x000A,0x0C80]，时间 = N * 10ms，时间范围是[100ms,32s] */
    uint16_t minCeLength; /*!< 最小事件组长度，取值范围[0x0000,0xFFFF]，时间 = N * 0.125ms */
    uint16_t maxCeLength; /*!< 最大事件组长度，取值范围[0x0000,0xFFFF]，时间 = N * 0.125ms */
} DLI_ConnParam;

typedef struct {
    uint8_t advHandle;  /*!< 广播句柄，取值范围[0, 0xFF] */
    uint8_t operation;  /*!< 分片操作 */
    uint8_t selection;  /*!< 分片选择，0x00：控制器可以对主机所有广播数据分片；0x01：控制器不可以对主机广播数据分片 */
    uint8_t advDataLen; /*!< 广播数据长度 */
    uint8_t advData[0]; /*!< 广播数据 */
} DLI_AdvData;

typedef struct {
    uint8_t advHandle; /*!< 广播句柄，取值范围[0, 0xFF] */
    uint8_t operation; /*!< 分片操作 */
    uint8_t selection; /*!< 分片选择，0x00：控制器可以对主机所有广播数据分片；0x01：控制器不可以对主机广播数据分片 */
    uint8_t scanRspDataLen; /*!< 查询响应数据长度 */
    uint8_t scanRspData[0]; /*!< 查询响应数据 */
} DLI_ScanRspData;

typedef struct DLI_AdvEnable {
    uint8_t enable;       /*!< 0x0：关闭广播，0x1: 使能广播 */
    uint16_t duration;    /*!< 0x0:广播时间无限制，0x1~0xFFFF:广播时间=N*10ms */
    uint8_t maxAdvEvents; /*!< 0x0:广播事件个数无限制，0x1~0xFF:广播事件个数限制 */
} DLI_AdvEnable;

typedef struct DLI_ScanEnable {
    uint8_t enable;           /*!< 使能开关，0：关闭，1：开启 */
    uint8_t filterDuplicates; /*!< 重复过滤开关，0：关闭，1：开启 */
} DLI_ScanEnable;

typedef struct DLI_RemoveAdvertisingSet {
    uint8_t advHandle;
} DLI_RemoveAdvertisingSet;

typedef struct DLI_AdvCbkContext {
    uint8_t advHandle;
} DLI_AdvCbkContext;

typedef struct {
    uint16_t scanInterval; /*!< 两次扫描之间的时间间隔 */
    uint16_t scanWindow;   /*!< 扫描持续时间 */
} DLI_ConnScanParam;

typedef struct {
    uint8_t initiatorFilterPolicy;                             /*!< 过滤策略 */
    uint8_t ownAddressType;                                    /*!< 本端标识类型 */
    uint8_t peerAddressType;                                   /*!< 对端标识类型 */
    uint8_t peerAddress[SLE_ADDR_LEN];                         /*!< 对端标识 */
    uint8_t gtNegotiateInd;                                    /*!< GT角色协商指示 */
    uint8_t scanFrameFormatInd;                                /*!< 查询无线帧类型指示 */
    DLI_ConnScanParam scanParam[CONN_VALID_MULTI_OF_FRAME_TYPE_IND]; /*!< 扫描持续时间 */
    uint8_t connePhy;                                          /*!< 连接带宽，0:1M 1:2M 2:4M */
    uint8_t connPilot;                                         /*!< 连接导频密度，0:4:1 1:8:1 2:16:1 3:NO */
    uint8_t connFrameFormatMSeqInd;                            /*!< 连接无线帧类型M序列指示 */
    uint8_t connFrameFormatInd;                                /*!< 连接无线帧类型指示 */
    DLI_ConnParam connParam[CONN_VALID_MULTI_OF_FRAME_TYPE_IND];     /*!< 扫描持续时间 */
} DLI_ConnectionCreateParamMultiInd;

typedef struct {
    uint8_t initiatorFilterPolicy;                             /*!< 过滤策略 */
    uint8_t ownAddressType;                                    /*!< 本端标识类型 */
    uint8_t peerAddressType;                                   /*!< 对端标识类型 */
    uint8_t peerAddress[SLE_ADDR_LEN];                         /*!< 对端标识 */
    uint8_t gtNegotiateInd;                                    /*!< GT角色协商指示 */
    uint8_t scanFrameFormatInd;                                /*!< 查询无线帧类型指示 */
    DLI_ConnScanParam scanParam; /*!< 扫描持续时间 */
    uint8_t connePhy;                                          /*!< 连接带宽，0:1M 1:2M 2:4M */
    uint8_t connPilot;                                         /*!< 连接导频密度，0:4:1 1:8:1 2:16:1 3:NO */
    uint8_t connFrameFormatMSeqInd;                            /*!< 连接无线帧类型M序列指示 */
    uint8_t connFrameFormatInd;                                /*!< 连接无线帧类型指示 */
    DLI_ConnParam connParam;     /*!< 扫描持续时间 */
} DLI_ConnectionCreateParamSingleInd;

typedef struct {
    uint8_t action;      /*!< 0：删除过滤器，1：添加过滤器 */
    uint8_t filterIndex; /*!< 过滤器的ID号，如果是添加操作，此index已有过滤器，新的会覆盖旧的 */
    int8_t rssi;         /*!< 收到的包的rssi大于等于此值，如果配置的是0x7F，所有rssi都满足 */
    uint8_t addrType;           /*!< 地址类型 */
    uint8_t addr[SLE_ADDR_LEN]; /*!< 地址 */
} DLI_SetAddrScanFilterParam;

typedef enum {
    SLE_PACKET_FILTER_ENABLE = 0,
    SLE_PACKET_FILTER_SPECIFY_DATA_RECV_CHANNEL,
    SLE_PACKET_FILTER_ALL,
    SLE_PACKET_FILTER_ADDR,
    SLE_PACKET_FILTER_NAME,
    SLE_PACKET_FILTER_SRVC_DATA,
    SLE_PACKET_FILTER_MANU_DATA,
    SLE_PACKET_FILTER_SPECIFY_TYPE_AND_DATA,
    SLE_PACKET_FILTER_SPECIFY_TYPE_AND_DATA_MASK,
} DLI_FilterSubCode;

typedef struct DLI_ConnectionCreateParam {
    uint8_t bitFrameType;              /*!< 帧类型, 参见DLI_ScanBitFrameTypeInd_E定义 */
    uint8_t initiatorFilterPolicy;     /*!< 过滤策略 */
    uint8_t ownAddressType;            /*!< 本端标识类型 */
    uint8_t peerAddressType;           /*!< 对端标识类型 */
    uint8_t peerAddress[SLE_ADDR_LEN]; /*!< 对端标识 */
    uint8_t gtNegotiateInd;            /*!< GT角色协商指示 */
    uint8_t initiatingPhys;            /*!< 带宽指示 */
    uint16_t scanInterval;             /*!< 两次扫描之间的时间间隔 */
    uint16_t scanWindow;               /*!< 扫描持续时间 */
    uint16_t connectionIntervalMin;    /*!< 连接间隔的最小取值 */
    uint16_t connectionIntervalMax;    /*!< 连接间隔的最大取值 */
    uint16_t maxLatency;               /*!< 最大延迟周期 */
    uint16_t supervisionTimeout;       /*!< 超时时间 */
    uint16_t minCeLength;              /*!< 推荐的连接事件的最小取值 */
    uint16_t maxCeLength;              /*!< 推荐的连接事件的最大取值 */
} DLI_ConnectionCreateParam;

typedef struct DLI_ConnCbkContext {
    uint32_t versionAndLocalIndex;
    uint16_t connHandle;
} DLI_ConnCbkContext;

typedef struct DLI_DisconnectParam {
    uint16_t connHandle;
    uint8_t reason;
} DLI_DisconnectParam;

typedef struct DLI_ConnectionUpdateParam {
    uint16_t connHandle;         /*!< 连接句柄 */
    uint16_t connIntervalMin;    /*!< 链路调度最小间隔，单位slot */
    uint16_t connIntervalMax;    /*!< 链路调度最大间隔，单位slot */
    uint8_t txRxInterval;        /*!< 事件内间隔 */
    uint16_t eventInterval;      /*!< 事件间间隔和事件组间间隔 */
    uint16_t maxLatency;         /*!< 延迟周期，单位为事件组周期 */
    uint16_t supervisionTimeout; /*!< 超时时间，单位10ms */
    uint8_t systemTimeUnit;      /*!< 系统调度时隙 */
    uint8_t txRxFlag; /*!< 先发后发指示，指示信令T方在事件中先发送数据还是后发送数据，0为先发，1为后发 */
} DLI_ConnectionUpdateParam;

typedef struct DLI_SetPhyParam {
    uint16_t connHandle;    /*!< 连接句柄 */
    uint8_t txFormat;       /*!< 发送无线帧类型，参考radio_frame_type_t */
    uint8_t rxFormat;       /*!< 接收无线帧类型，参考radio_frame_type_t */
    uint8_t txPhy;          /*!< 发送PHY，参考tx_rx_phy_t */
    uint8_t rxPhy;          /*!< 接收PHY，参考tx_rx_phy_t */
    uint8_t txPilotDensity; /*!< 发送导频密度指示，参考tx_rx_pilot_density_t */
    uint8_t rxPilotDensity; /*!< 接收导频密度指示，参考tx_rx_pilot_density_t */
    uint8_t gFeedback;      /*!< 先发链路反馈类型指示，取值范围0-63。
                                  0：指示基于CBG的反馈
                                  1-25：指示不携带数据信息场景组播反馈信息的比特位置，其中位置信息为指示信息的数值
                                  26：指示基于TB的反馈
                                  27-34：指示携带数据信息场景组播反馈信息的比特位置，其中位置信息为(指示信息的数值-26)
                                  35-63：预留 */
    uint8_t tFeedback;      /*!< 后发链路反馈类型指示，取值范围0-7
                                  0-5：指示半可靠组播反馈，并指示采用m序列的编号
                                  6：指示基于CBG的反馈
                                  7：指示基于TB的反馈 */
} DLI_SetPhyParam;

typedef struct DLI_ControllerData {
    uint16_t connHandle;
    uint16_t opcode;
    uint8_t dataLength;
    uint8_t dataBuffer[0];
} DLI_ControllerData;

typedef struct DLI_EnableEncryptParam {
    uint16_t connHandle;
    uint8_t linkKey[SLE_SM_LINK_KEY_LEN];
    uint8_t cryptoAlgo;
    uint8_t keyDerivAlgo;
    uint8_t integrChkInd;
} DLI_EnableEncryptParam;

typedef struct DLI_SmCbkContext {
    uint16_t connHandle;
    bool isGroup;
} DLI_SmCbkContext;

typedef struct {
    uint16_t connHandle;
} DLI_ReadRemoteMeasureCapsParam;

typedef enum {
    FREQUENCY_BAND_2_4GHZ = (1 << 0),  // 比特位 0：2.4GHz 跳频信道指示字段是否存在
    FREQUENCY_BAND_5_1GHZ = (1 << 1),  // 比特位 1：5.1GHz 跳频信道指示字段是否存在
    FREQUENCY_BAND_5_8GHZ = (1 << 2),  // 比特位 2：5.8GHz 跳频信道指示字段是否存在
} DLI_FrequencyBand;

typedef enum {
    MEASUREMENT_TARGET_START = 0,  // 启动目标链路的测量
    MEASUREMENT_TARGET_STOP = 1,   // 停止目标链路的测量，释放链路相关资源
    MEASUREMENT_TARGET_PAUSE = 2,  // 暂停目标链路的测量，但不释放链路相关资源
} DLI_MeasurementTargetState;

typedef enum {
    TX_ORDER_FIRST = 0,   // 先发：先发送数据
    TX_ORDER_SECOND = 1,  // 后发：后发送数据
} DLI_TxOrderIndication;

typedef struct {
    uint16_t connHandle;                         /*!< 连接句柄 */
    uint8_t configId;                            /*!< 位置测量信号配置索引 */
    uint32_t measureConfigDirect;                     /*!< 测量量配置指示 */
    uint32_t occurrenceGroupPeriod;              /*!< 事件组周期 */
    uint8_t schedulingTimeslot;                  /*!< 系统调度时隙 */
    uint8_t rttPhy;                              /*!< 测量信号带宽指示 */
    uint8_t freqHoppingMode;                     /*!< 跳频方式指示 */
    uint16_t fmFreq;                             /*!< 初始化阶段的频点 */
    uint8_t sendDirect;                          /*!< 对端先发后发指示，0：先发，1：后发 */
    uint8_t antennaOrderConfig;                  /*!< 多天线次序配置指示 */
    uint8_t firstAntennaTypeConfig;              /*!< 先发链路的测量信号多天线类型配置 */
    uint8_t secondAntennaTypeConfig;             /*!< 后发链路的测量信号多天线类型配置 */
    uint8_t eventsCount;                         /*!< 事件总数 */
    uint8_t bitWidth;                            /*!< 多天线随机次序位宽K */
    uint8_t pmInitAntCount;                      /*!< 先发节点天线数量指示 */
    uint8_t pmInitSignal2Tone;                   /*!< 先发链路测量信号2多音指示 */
    uint8_t firstNodeInterval;                   /*!< 先发节点天线切换间隔 */
    uint8_t pmReflAntCount;                      /*!< 后发节点天线数量指示 */
    uint8_t pmReflSignal2Tone;                   /*!< 后发链路测量信号2多音指示 */
    uint8_t secondNodeInterval;                  /*!< 后发节点天线切换间隔 */
    uint8_t channelBandwidth;                    /*!< 跳频信道频带指示 */
    uint8_t pm2400mBand[SLE_MEASURE_PM_24G_BAND_LEN]; /*!< 2.4GHz跳频信道指示 */
} DLI_SetMeasureConfigParam;

typedef struct {
    uint16_t connHandle;
    uint8_t enable;
} DLI_SetMeasureEnableParam;

typedef struct {
    uint8_t key[SLE_SM_ENCRYPT_KEY_LEN];     /*!< 对已给数据进行加密时所用的128bit的密钥 */
    uint8_t plaintext[SLE_SM_PLAINTEXT_LEN]; /*!< 请求加密的128bit的数据块内容 */
    uint8_t algorithm;                       /*!< 0x00：HA1(HMAC-SM3)，0x01：HA2(AES-CMAC)，其他预留*/
} DLI_EncryptParam;

typedef struct {
    uint16_t connHandle;
    uint8_t linkKey[SLE_SM_LINK_KEY_LEN];
    uint8_t cryptoAlgo;
    uint8_t keyDerivAlgo;
} DLI_EncryptReqReplyParam;

typedef struct {
    uint16_t handler;                       /*!< 同步组播事件组集合连接句柄 */
    uint8_t algo;                           /*!< 0x00：AC1(SM4-CCM)，0x01：AC2(AES-CCM)，
                                                 0x02: EA1/IA1(ZUC), 0x03: EA2/IA2(AES-CTR) */
    uint8_t iv[SLE_IMG_ENCRYPTION_IV_LEN];  /*!< 初始化向量 */
    uint8_t key[SLE_IMG_ENCRYPTION_GROUP_KEY_LEN]; /*!< 组秘钥 */
} DLI_IMGEncryptParam;

typedef struct {
    uint16_t connHandle; /*!< 连接链路句柄 */
    uint16_t txOctets;   /*!< 连接链路上所偏好的最大传输payload字节数 */
} DLI_SetDataLenParam;

typedef struct {
    uint16_t connHandle;
    uint16_t eventInterval; /*!< 事件间间隔和事件组间间隔 */
    uint8_t eventNumber;    /*!< 事件的数量 */
} DLI_SetAcbEvtParam;

typedef struct {
    uint16_t bitNumber;         /*!< feature的具体bit位 */
    uint8_t bitValue;           /*!< 1：feature bit位支持；0：feature bit位不支持 */
} DLI_LocalPrivateFeatures;

typedef struct {
    uint16_t connHandle;
} DLI_ConnHandleStru;

typedef struct {
    uint16_t connHandle;
    uint8_t reason;
    uint16_t connIntervalMin;    /*!< 链路调度最小间隔，单位slot */
    uint16_t connIntervalMax;    /*!< 链路调度最大间隔，单位slot */
    uint8_t txRxInterval;        /*!< 事件内间隔 */
    uint16_t eventInterval;      /*!< 事件间间隔和事件组间间隔 */
    uint16_t maxLatency;         /*!< 延迟周期，单位为事件组周期 */
    uint16_t supervisionTimeout; /*!< 超时时间，单位10ms */
    uint8_t systemTimeUnit;      /*!< 系统调度时隙 */
    uint8_t txRxFlag; /*!< 先发后发指示，指示信令T方在事件中先发送数据还是后发送数据，0为先发，1为后发 */
} DLI_RemConParamReqReplyParam;

typedef struct {
    uint16_t connHandle;
    uint8_t mcs;
} DLI_SetMcsParam;

typedef struct {
    int8_t bleMaxPower; /* 设置发送功率值 BLE */
    int8_t sleMaxPower; /* 设置发送功率值 SLE */
} DLI_SetTxPowerParam;

typedef struct {
    uint16_t connHandle;    /* 连接句柄 */
    uint8_t enable;         /* 0：不允许使用高功率，1：允许使用高功率 */
    uint8_t powerLevel;     /* 高功率档位: 7档或者8档等 */
} DLI_EnableConnHighPowerParam;

typedef struct {
    uint8_t frameType;  /* 帧类型: 帧1或者帧4 */
    int8_t txPower;     /* 高功率档位对应的功率: 7档或者8档 */
} DLI_SetConnFramePowerLevelParam;

typedef struct {
    uint8_t type;
    uint8_t id;
    uint8_t direction;      /* 设置/删除data path的方向（上行/下行） */
    uint16_t connHandle[SLE_ICB_MAX_NUM];
    uint8_t connHandleNum;
} DLI_ICGCbkParam;

typedef struct DLI_ICBParam {
    uint8_t id;                         /* 用于标识一个ICB，取值范围[0x00,0xEF] */
    struct DLI_ICB {
        uint16_t maxSduG2T;             /* G到T最大SDU Payload字节数 */
        uint16_t maxSduT2G;             /* T到G最大SDU Payload字节数 */
        uint8_t rtnG2T;                 /* G到T每一个IMB数据PDU重传次数 */
        uint8_t rtnT2G;                 /* T到G每一个IMB数据PDU重传次数 */
    } *param;
} DLI_ICBParam;

typedef struct {
    uint8_t type;                       /* 详见CM_ICBType */
    DLI_CmdOpcode opCode;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint32_t sduIntervalG2T;            /*!< G到T两个连续SDU之间的时间间隔，us */
    uint32_t sduIntervalT2G;            /*!< T到G两个连续SDU之间的时间间隔，us */
    uint8_t sca;                        /*!< 睡眠时钟精度 */
    uint8_t packing;                    /*!< 当前仅支持交叉 */
    uint8_t framing;                    /*!< 当前仅支持未切分 */
    uint16_t maxLatencyG2T;             /*!< G到T最大传输延迟，ms */
    uint16_t maxLatencyT2G;             /*!< T到G最大传输延迟，ms */
    uint8_t icbCnt;                     /* ICB链路数 */
    uint8_t paramCnt;                   /* 每一个ICB链路需要设置的参数个数 */
    DLI_ICBParam *icbParam;             /* 同一个ICG底下所有ICB链路需要设置的参数 */
} DLI_ICGParam;

typedef struct {
    uint8_t type;                       /* 详见CM_ICBType */
    DLI_CmdOpcode opCode;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
} DLI_ICGRemoveParam;

typedef struct DLI_ICBTestParam {
    uint8_t id;                         /* 用于标识一个ICB，取值范围[0x00,0xEF] */
    uint8_t nse;                        /* 0x01-0x1F	在IMG中每一个IMB每个间隔内的子事件个数 */
    uint16_t maxSduG2T;                 /* G到T最大SDU Payload字节数 */
    uint16_t maxSduT2G;
    uint16_t maxPduG2T;                 /* G到T最大PDU Payload字节数 */
    uint16_t maxPduT2G;
    uint8_t frameG2T;                   /* 0	无线帧类型一
                                           1	无线帧类型二
                                           2	无线帧类型四 */
    uint8_t frameT2G;
    uint8_t phyG2T;                     /* 0	G到T方向上传输包使用的PHY是1M
                                           1	G到T方向上传输包使用的PHY是2M
                                           2	G到T方向上传输包使用的PHY是4M */
    uint8_t phyT2G;
    uint8_t mcsG2T;                     /* 0x00	G到T方向上使用的调制方式和polar编码，BPSK1/4
                                           0x01	G到T方向上使用的调制方式和polar编码，BPSK3/8
                                           0x02	G到T方向上使用的调制方式和polar编码，QPSK1/4
                                           0x03	G到T方向上使用的调制方式和polar编码，QPSK3/8
                                           0x04	G到T方向上使用的调制方式和polar编码，QPSK1/2
                                           0x05	G到T方向上使用的调制方式和polar编码，QPSK5/8
                                           0x06	G到T方向上使用的调制方式和polar编码，QPSK3/4
                                           0x07	G到T方向上使用的调制方式和polar编码，QPSK7/8
                                           0x08	G到T方向上使用的调制方式和polar编码，QPSK 1
                                           0x09	G到T方向上使用的调制方式和polar编码，8PSK5/8
                                           0x0A	G到T方向上使用的调制方式和polar编码，8PSK3/4
                                           0x0B	G到T方向上使用的调制方式和polar编码，8PSK7/8
                                           0x0C	G到T方向上使用的调制方式和polar编码，8PSK 1
                                           0x0D	预留未来使用
                                           0x0E	预留未来使用
                                           0x0F	预留未来使用
                                           0x10	G到T方向上使用的调制方式和polar编码，16QAM3/4
                                           0x11	G到T方向上使用的调制方式和polar编码，16QAM7/8
                                           0x12	G到T方向上使用的调制方式和polar编码，16QAM1 */
    uint8_t mcsT2G;
    uint8_t pilotG2T;                   /* 0x00-0x07 标识Polar编码时插导频pilot的比例，代表[数据:导频]=[2^ratio:1] */
    uint8_t pilotT2G;
    uint8_t bnG2T;                      /* 0x00	G到T方向上没有ICB数据包
                                           0x01-0x0F	G到T方向上每一个IMB每个间隔内传输新ICB数据包的个数 */
    uint8_t bnT2G;
} DLI_ICBTestParam;

typedef struct {
    uint8_t type;                       /* 详见CM_ICBType */
    DLI_CmdOpcode opCode;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint8_t labelId;                    /* label */
    uint32_t sduIntervalG2T;            /* G到T两个连续SDU之间的时间间隔，us */
    uint32_t sduIntervalT2G;            /* T到G两个连续SDU之间的时间间隔，us */
    uint8_t ftG2T;                      /*!< G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint8_t ftT2G;                      /*!< T到G方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint16_t icbInterval;               /*!< 连续两个IMB Anchor Point的间隔时间，
                                            取值范围[0x0014,0x3E80]，时间 = N * 0.25ms，时间范围[5ms,4s] */
    uint8_t sca;                        /* 睡眠时钟精度 */
    uint8_t packing;                    /* 当前仅支持交叉 */
    uint8_t framing;                    /* 当前仅支持未切分 */
    uint8_t paramCnt;
    DLI_ICBTestParam *icbParam;
} DLI_ICGTestParam;

typedef struct {
    uint16_t connHandle;
    uint16_t lcid;
} DLI_ICBChannel;

typedef struct {
    uint8_t type;                       /* 详见CM_ICBType */
    DLI_CmdOpcode opCode;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint8_t icbCnt;
    DLI_ICBChannel *icb;
} DLI_ICGLabelParam;

typedef struct {
    uint8_t type;                       /* 详见CM_ICBType */
    DLI_CmdOpcode opCode;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint8_t labelId;
    uint8_t channelCnt;
    DLI_ICBChannel *channel;
} DLI_ICBConnectionParam;

typedef struct {
    uint8_t type;                       /* 详见CM_ICBType */
    DLI_CmdOpcode opCode;
    uint16_t connHandle;
} DLI_AcceptICBReqParam;

typedef struct {
    DLI_CmdOpcode opCode;
    DLI_RegOpcode regOpCode;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint16_t connHandle;
    uint8_t reason;
} DLI_RejectICBReqParam;

typedef struct {
    uint16_t connHandle;
    uint8_t direction;                  /* 0： host到controller，1：controller到host */
    uint8_t pathId;                     /* 0：数据通过HCI传输，其他值：通过Vendor专用传输 */
    struct {
        uint8_t codecId;                /* 0xff表示使用vendor codec */
        uint16_t vendorId;              /* 仅codecId为0xff时有效 */
        uint16_t vendorCodecId;         /* 仅codecId为0xff时有效 */
    } codec;
    uint32_t controllerDelay;           /* Controller延迟时间，以us为单位，取值范围[0x000000,0x3D0900]，时间范围[0s,4s] */
    uint8_t codecConfigLen;
    uint8_t *codecConfigData;
} DLI_SetupICBDataPathParam;

typedef struct {
    uint16_t connHandle;                /* 单个同步链路handler */
    uint8_t direction;                  /* 0： host到controller，1：controller到host */
} DLI_RemoveICBDataPathParam;

typedef struct {
    uint8_t type;                       /* 详见CM_ICBType */
    DLI_CmdOpcode opCode;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint8_t labelId;
    uint8_t icbCnt;
    uint16_t *connHandle;
} DLI_ICGUpdatedParam;

typedef struct {
    uint8_t allowedBand;
    uint8_t trigType;
} DLI_FreqBandExtParam;

typedef struct {
    uint16_t subrateMin;                /* 基础周期的最小倍数 */
    uint16_t subrateMax;                /* 基础周期的最大倍数 */
    uint16_t maxLatency;                /* T侧跳过基础周期的个数，默认值0 */
    uint16_t continuationNum;           /* 当前执行周期里面基础周期的执行个数 */
    uint16_t supervisionTimeout;        /* 超时时间，单位10ms */
} DLI_ACBEnableSubrateParam;

typedef struct {
    uint16_t lcid;                      /* 异步链路唯一标识 */
    uint16_t subrateMin;                /* 基础周期的最小倍数 */
    uint16_t subrateMax;                /* 基础周期的最大倍数 */
    uint16_t maxLatency;                /* T侧跳过基础周期的个数，默认值0 */
    uint16_t continuationNum;           /* 当前执行周期里面基础周期的执行个数 */
    uint16_t supervisionTimeout;        /* 超时时间，单位10ms */
} DLI_ACBSubrateParam;

#pragma pack()

typedef struct DLI_ExecuteCmdRetParam {
    uint16_t cmdOpcode;   /*!< 命令的操作码 */
    uint32_t size;        /*!< 携带参数数据的大小 */
    void *eventParameter; /*!< 携带参数数据的指针 */
} DLI_ExecuteCmdRetParam;


typedef void (*DLI_ExecuteCmdCbk)(void *context, uint16_t status, DLI_ExecuteCmdRetParam *cmdRes);
typedef struct DLI_ManagerContext {
    void *cbkContext;      /*!< 发送命令时传的cbkContext */
    DLI_ExecuteCmdCbk cbk; /*!< 外部的回调 */
    DLI_InnerCbkLineStru *innerCbkTable;
} DLI_ManagerContext;

typedef struct DLI_CbkLineStru {
    DLI_RegOpcode opcode;
    DLI_ExecuteCmdCbk func;
} DLI_CbkLineStru;

typedef struct DLI_CmdParams {
    uint16_t cmd;
    uint16_t event;
    void *inParam;
    uint16_t paramLen;
    DLI_ExecuteCmdCbk cbk;
    void *cbkContext;
    uint16_t cbkContextLen;
} DLI_CmdParams;

typedef enum {
    DEVD = 0,
    CM = 1,
    CM_ICB = 2,
    CM_COMMON = 3,
    SM = 4,
    HADM = 5,
    NBC = 6,
    QOSM = 7,

    EXT_START = 10,
    EXT_END = 17,
} ModuleType;

#ifdef __cplusplus
}
#endif
#endif