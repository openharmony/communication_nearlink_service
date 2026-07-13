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
 * dli事件结构体定义
 *
 ***************************************************************************/

#ifndef DLI_EVENT_STRUCT_H
#define DLI_EVENT_STRUCT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------宏定义-------------------------------------------------------*/
#define SLE_ADDR_LEN 6
#define SLE_MEASURE_LEN 0x18
#define SLE_IQ_DATA_LENGTH 3
#define SLE_FEATURE_LEN 10
#define SLE_SM_PLAINTEXT_LEN 16
#define SLE_RSSI_REP_SIZE 14
/* ---------------------------------------------事件处理相关结构体-------------------------------------------------------*/

#pragma pack(1)

typedef struct DLI_ReadBufSizeEvt {
    uint16_t acbTxDataLen;
    uint8_t acbTxDataNum;
    uint16_t icbTxDataLen;
    uint8_t icbTxDataNum;
} DLI_ReadBufSizeEvt;

typedef struct {
    uint8_t version;
    uint16_t companyIdentifier;
    uint16_t subversion;
} DLI_ReadLocalVerEvt;

typedef struct {
    uint8_t feats[SLE_FEATURE_LEN]; /*!< 特性表格式，按协议bit定义 */
} DLI_ReadLocalFeatsEvt;

typedef struct DLI_AdvReportEvt {
    uint8_t eventType;                /*!< 事件类型，
                                       *   bit number | description
                                       *   0          | connectable SLE adv packet
                                       *   1          | scanable SLE adv packet
                                       *   2          | direct SLE packet
                                       *   3          | scan response packet
                                       *   4-5        | data status:
                                       *              |       0b00, complete adv report
                                       *              |       0b01, incomplete adv report, more data is following
                                       *              |       0b10, incomplete adv report, data is truncated
                                       *              |       0b11, RFU
                                       */
    uint8_t addrType;                 /*!< 广播标识类型 */
    uint8_t addr[SLE_ADDR_LEN];       /*!< 广播标识 */
    uint8_t directAddrType;           /*!< 定向标识类型 */
    uint8_t directAddr[SLE_ADDR_LEN]; /*!< 定向标识 */
    uint8_t primFrameType;            /*!< 第一广播信道无线帧类型 */
    uint8_t secondPhy;                /*!< 第二广播信道带宽，0：1M，1：2M，3：4M */
    uint8_t secondFrameType;          /*!< 第二广播信道无线帧类型 */
    uint8_t secondPilotRatio; /*!< 第二广播信道导频密度，0：4:1，1：8:1，2：16:1，3：无导频 */
    uint8_t secondMcs;        /*!< 第二广播信道调制编码参数 */
    uint8_t rssi; /*!< 信号强度指示，取值范围[-127dBm, 20dBm]，0x7F表示不提供信号强度指示 */
    uint8_t dataLength; /*!< adv report数据长度 */
    uint8_t data[0];    /*!< adv report数据 */
} DLI_AdvReportEvt;

typedef struct DLI_SetScanFilterEvt {
    uint8_t subCode;
    uint8_t numAvailable;
} DLI_SetScanFilterEvt;

typedef struct DLI_AdvertisingTerminatedEvt {
    uint8_t status;
    uint8_t advHandle;
    uint16_t connHandle;
} DLI_AdvertisingTerminatedEvt;

typedef struct DLI_AdvPowerChangeEvt {
    uint8_t advHandle;
    uint8_t powerLevel;
} DLI_AdvPowerChangeEvt;

typedef struct DLI_SetPhyEvt {
    uint8_t status;
    uint16_t connHandle;
    uint8_t txFormat;
    uint8_t rxFormat;
    uint8_t txPhy;
    uint8_t rxPhy;
    uint8_t txPilotDensity;
    uint8_t rxPilotDensity;
    uint8_t gFeedback;
    uint8_t tFeedback;
} DLI_SetPhyEvt;

typedef struct DLI_SetRxDataFilterEvt {
    uint16_t connHandle;
} DLI_SetRxDataFilterEvt;

typedef struct {
    uint8_t status;          /*!< 创建结果，0：创建成功，非0；创建失败 */
    uint16_t connHandle;     /*!< 连接句柄 */
    uint8_t role;            /*!< GT角色，0：做G先发，1：做T后发，2：做G后发，3：做T先发 */
    uint8_t peerAddressType; /*!< 对端标识类型 */
    uint8_t peerAddress[SLE_ADDR_LEN];                   /*!< 对端标识 */
    uint8_t localResolvablePrivateAddress[SLE_ADDR_LEN]; /*!< 本端可解析随机地址 */
    uint8_t peerResolvablePrivateAddress[SLE_ADDR_LEN];  /*!< 对端可解析随机地址 */
    uint16_t connectionInterval; /*!< 事件组周期，两个相邻的事件组起始时刻的时间差，时间 = N * 基础时隙单位 */
    uint16_t peripheralLatency;   /*!< T节点延迟周期 */
    uint16_t supervisionTimeout;  /*!< 超时时间，允许超时的最大时间，时间 = N * 10ms */
    uint8_t centralClockAccuracy; /*!< 时钟精度 */
    uint8_t connCompleteType; /*!< 连接完成类型，0：通过本端扫描建立的连接，1：通过本端广播建立的连接 */
    uint8_t advHandle;        /*!< 广播标识，若是通过本端广播建立的连接，则为对应启动广播标识，否则默认为0，无效值 */
} DLI_ConnectionCompleteEvt;

typedef struct DLI_DisconnectEvt {
    uint8_t status;      /*!< 释放结果：0：成功，1：失败 */
    uint16_t connHandle; /*!< 释放逻辑链路handle */
    uint8_t reason;      /*!< 释放逻辑原因 */
} DLI_DisconnectEvt;

typedef struct DLI_ConnectionUpdateCmpEvt {
    uint8_t status;
    uint16_t connHandle;         /*!< 逻辑链路handle */
    uint16_t connInterval;       /*!< 链路调度间隔，单位slot */
    uint8_t txRxInterval;        /*!< 事件内间隔 */
    uint16_t eventInterval;      /*!< 事件间间隔和事件组间间隔 */
    uint16_t maxLatency;         /*!< 延迟周期，单位为事件组周期 */
    uint16_t supervisionTimeout; /*!< 超时时间，单位10ms */
    uint8_t systemTimeUnit;      /*!< 系统调度时隙，枚举值 */
    uint8_t txRxFlag; /*!< 先发后发指示，指示信令T方在事件中先发送数据还是后发送数据，0为先发，1为后发 */
} DLI_ConnectionUpdateCmpEvt;

typedef struct DLI_EncryptChangeEvt {
    uint8_t status; /*!< 0：加密启动成功，其他：加密启动失败 */
    uint16_t connHandle;
    uint8_t encryptChange; /*!< 加密变更，0：加密停止，1：初次加密启动，2：链路暂停加密更新密钥后重启加密 */
} DLI_EncryptChangeEvt;

typedef struct DLI_CommandComplete {
    uint16_t cmdOpcode;
    uint8_t numDliCommandPackets;
    uint8_t parameters[0];
} DLI_CommandComplete;

typedef struct DLI_CommandErrorStru {
    uint16_t status; /*!< 命令执行状态 */
    uint16_t cmd;
    uint16_t event;
    uint8_t req[1]; /*!< 返回参数 */
} DLI_CommandErrorStru;

typedef struct DLI_ControllerDataEvt {
    uint16_t connHandle;    /*!< 连接句柄 */
    uint16_t ctrlDataIndex; /*!< 控制面信令标识 */
    uint8_t len;            /*!< 数据长度 */
    uint8_t data[0];        /*!< 数据 */
} DLI_ControllerDataEvt;

typedef struct DLI_CommandStatus {
    uint16_t cmdOpcode;
    uint8_t numDliCommandPackets;
    uint8_t status;
} DLI_CommandStatus;

typedef struct {
    uint8_t caps[SLE_MEASURE_LEN]; // 与DLI_ReadCsCapsEvt长度一致
} DLI_ReadLocalCsCapsEvt;

typedef struct {
    uint8_t status;
    uint16_t connHandle;
    uint8_t caps[SLE_MEASURE_LEN]; // 与DLI_ReadCsCapsEvt长度一致
} DLI_ReadRemoteCsCapsEvt;

typedef struct {
    uint32_t measureSignalCapabilitySupported;
    uint8_t multiAntennasSupported;
    uint8_t multiAntennasSwitchInterval;
    uint8_t type1MinTimeIp1;
    uint8_t type1MinTimeIp2;
    uint8_t type1MinTimeIp3;
    uint8_t type1MinTimeIp4;
    uint16_t type1MinTimeInterEvt;
    uint16_t type2MinTimeInterEvt;
    uint16_t minTimeInitializeInterEvt;
    uint16_t minTimeIntraEvt;
    uint16_t minTimeIntraEvtGroup;
    uint16_t phaseCaliOffsetCm;  // 相位校准offset值，单位cm
    uint16_t tofCaliOffsetM;      // TOF校准offset值，单位m
} DLI_ReadCsCapsEvt;

typedef struct {
    uint8_t status;
    uint16_t connHandle;
    uint32_t measureSignalCapabilitySupported;
    uint32_t measureReportingCapabilitySupported;
    uint8_t multiAntennasSupported;
    uint8_t multiAntennasSwitchInterval;
    uint8_t type1MinTimeIp1;
    uint8_t type1MinTimeIp2;
    uint8_t type1MinTimeIp3;
    uint8_t type1MinTimeIp4;
    uint16_t type1MinTimeInterEvt;
    uint16_t type2MinTimeInterEvt;
    uint16_t minTimeInitializeInterEvt;
    uint16_t type1MinTimeIntraEvt;
    uint16_t type2MinTimeIntraEvt;
    uint16_t minTimeInitializeIntraEvt;
    uint16_t minTimeIntraEvtGroup;
    uint16_t phaseCaliOffsetCm;
    uint16_t tofCaliOffsetM;
} DLI_ReadRemoteCsCapsPrivEvt;

typedef struct {
    uint8_t status;
    uint32_t measureSignalCapabilitySupported;
    uint32_t measureReportingCapabilitySupported;
    uint8_t multiAntennasSupported;
    uint8_t multiAntennasSwitchInterval;
    uint8_t type1MinTimeIp1;
    uint8_t type1MinTimeIp2;
    uint8_t type1MinTimeIp3;
    uint8_t type1MinTimeIp4;
    uint16_t type1MinTimeInterEvt;
    uint16_t type2MinTimeInterEvt;
    uint16_t minTimeInitializeInterEvt;
    uint16_t type1MinTimeIntraEvt;
    uint16_t type2MinTimeIntraEvt;
    uint16_t minTimeInitializeIntraEvt;
    uint16_t minTimeIntraEvtGroup;
    uint16_t phaseCaliOffsetCm;
    uint16_t tofCaliOffsetM;
} DLI_ReadLocalCsCapsPrivEvt;

typedef struct {
    uint8_t status;
    uint8_t posMeasureSigConfigIdx;
    uint8_t measureState;
} DLI_MeasureStateChangeEvt;

typedef struct {
    uint32_t tofResult;
} DLI_SlemTof;

typedef struct {
    uint8_t rssi;
    uint8_t iqBitLength;  // IQ有效位，目前是12bit
    uint8_t iqData[0];    // DLI_SlemIqData
} DLI_SlemChnlMeas;

/**
 * @brief iqdata[0]: 保存i_data的低8bit
    iqdata[1]: 低4bit保存i_data的高4bit, 高4bit保存q_data的低4bit; iqdata[2]: 保存q_data的高8bit
 */
typedef struct {
    uint8_t iqData[SLE_IQ_DATA_LENGTH];
} DLI_SlemIqData;

typedef struct {
    uint8_t localId;   // G结点即启动测量端为0，其余结点被测量段非0
    uint8_t remoteId;  // 未使用
} DLI_SlemVenderData;

typedef struct {
    uint8_t venderDataLen;
    DLI_SlemVenderData venderData;
} DLI_SlemVender;

typedef struct {
    uint16_t aoa : 1;
    uint16_t aod : 1;
    uint16_t chnlInfo24g : 1;
    uint16_t chnlInfo51g : 1;
    uint16_t chnlInfo58g : 1;
    uint16_t freqDiff : 1;
    uint16_t tof : 1;
    uint16_t chnlMeas : 1;
    uint16_t sinr : 1;
    uint16_t rsvd : 6;
    uint16_t vender : 1;
} DLI_SlemInfoType;

typedef struct {
    uint8_t status;
    uint16_t connHandle;
    uint8_t slemIdx;
    DLI_SlemInfoType slemInfoType;
    uint32_t timestampSn;
    uint8_t data[0];
} DLI_CsIqReportEvt;

typedef struct DLI_NumberOfCompletedPacketsEvt {
    uint16_t connHandle;
    uint8_t numCompletedPackets;
} DLI_NumberOfCompletedPacketsEvt;

typedef struct DLI_EncryptParamReqEvt {
    uint16_t connHandle;
} DLI_EncryptParamReqEvt;

typedef struct {
    uint16_t connHandle;
    uint16_t maxTxOctets; /*!< 连接链路上能够发送的最大Payload字节数 */
    uint16_t maxRxOctets; /*!< 连接链路上能够接收的最大Payload字节数 */
} DLI_DataLenChangeEvt;

typedef struct {
    uint16_t connHandle;
    int8_t rssi; /*!< 信号强度,-128 to 127,127代表此值无效 */
} DLI_ReadRemoteRssiEvt;

typedef struct {
    uint16_t connHandle;
    uint8_t status; /*!< 执行结果 */
    uint8_t enable; /*!< 使能状态 */
    uint8_t rate;   /*!< 下发低时延时调度最大能力 */
} DLI_AcbLowLatencyEnableEvt;

typedef struct {
    uint16_t connHandle;         /*!< 连接句柄 */
    uint16_t connIntervalMin;    /*!< 连接事件组周期最小取值，时间 = N * 基础时隙单位 */
    uint16_t connIntervalMax;    /*!< 连接事件组周期最大取值，时间 = N * 基础时隙单位 */
    uint8_t txRxInterval;        /*!< 事件内间隔 */
    uint16_t eventInterval;      /*!< 事件间间隔和事件组间间隔 */
    uint16_t maxLatency;         /*!< 最大延迟周期，单位为事件组周期 */
    uint16_t supervisionTimeout; /*!< 超时时间，单位10ms */
    uint8_t systemTimeUnit;      /*!< 系统调度时隙 */
    uint8_t txRxFlag; /*!< 先发后发指示，指示信令T方在事件中先发送数据还是后发送数据，0为先发，1为后发 */
} DLI_RemoteConnParamReqEvt;

typedef struct {
    uint8_t status;
    uint16_t connHandle;
    uint8_t feats[SLE_FEATURE_LEN]; /*!< 特性表格式，按协议bit定义 */
} DLI_ReadRemoteFeatsEvt;

typedef struct {
    uint8_t status;
    uint16_t connHandle;
    uint16_t eventInterval; /*!< 事件间间隔和事件组间间隔 */
    uint8_t eventNumber;    /*!< 事件的数量 */
} DLI_SetAcbEvtParamEvt;

typedef struct {
    uint8_t status;
    uint16_t connHandle;
    uint8_t version;            /*!< 版本号 */
    uint16_t companyIdentifier; /*!< 星闪联盟公司标识 */
    uint16_t subversion;        /*!< 子版本号 */
} DLI_ReadRemoteVersionEvt;

typedef struct {
    uint16_t connHandle;
    int8_t signalStrength;
    uint32_t actualRssiTs;
    uint8_t rssiIdx[SLE_RSSI_REP_SIZE];
    int8_t actualRssiValue[SLE_RSSI_REP_SIZE];
} DLI_RssiEvt;

typedef struct {
    uint16_t connHandle;
    uint8_t powerLevel;
} DLI_PowerLevelEvt;

typedef struct {
    uint8_t status;
} DLI_ChipResetNotifyEvt;

typedef struct {
    uint16_t connHandle;
} DLI_ConnEnableHighPowerCmpEvt;

typedef struct {
    uint8_t id;
    uint8_t paramCnt;
    uint16_t connHandle[0];
} DLI_SetIOGParamEvt;

typedef struct {
    uint8_t id;
    uint16_t gHandle;
    uint8_t paramCnt;
    uint16_t connHandle[0];
} DLI_SetIMGParamEvt;

typedef struct {
    uint8_t id;
} DLI_RemoveICGParamEvt;

typedef struct {
    uint16_t lcid;
    uint16_t connHandle;
    uint8_t icgId;
    uint8_t icbId;
} DLI_ICBConnectReqEvt;

typedef struct {
    uint8_t status;
    uint8_t labelId;
    uint16_t lcid;
    uint16_t connHandle;
    uint8_t imgSyncDelay[3];          /*!< 0x0000EA-0x7FFFFF, 一个IMG事件内所有IMB上传输PDU占用的最大时间，以us为单位 */
    uint8_t imbSyncDelay[3];          /*!< 0x0000EA-0x7FFFFF, 一个IMG事件内指定IMB上传输PDU占用的最大时间，以us为单位 */
    uint8_t transLatencyG2T[3];       /*!< 0x0000EA-0x7FFFFF, G到T方向上实际的传输延迟，以us为单位 */
    uint8_t transLatencyT2G[3];       /*!< 0x0000EA-0x7FFFFF, T到G方向上实际的传输延迟，以us为单位 */
    uint8_t phyG2T;
    uint8_t phyT2G;
    uint8_t mcsG2T;
    uint8_t mcsT2G;
    uint8_t pilotG2T;
    uint8_t pilotT2G;
    uint8_t nse;                      /*!< 0x01-0x1F, 在IMG中每一个IMB每个间隔内的子事件个数 */
    uint8_t bnG2T;
    uint8_t bnT2G;
    uint8_t ftG2T;                    /*!< G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint8_t ftT2G;                    /*!< T到G方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint16_t maxPduG2T;               /*!< 0x0000-0x07FF, G到T方向上PDU Payload的最大字节数，以字节为单位 */
    uint16_t maxPduT2G;               /*!< 0x0000-0x07FF, T到G方向上PDU Payload的最大字节数，以字节为单位 */
    uint16_t imbInterval;             /*!< 连续两个IMB Anchor Point的间隔时间，取值范围[0x0014,0x3E80]，时间=N * 0.25ms，时间范围[5ms,4s] */
} DLI_ICBEstablishedEvt;

typedef struct {
    uint32_t diffTotal;              /* 500ms期间的cnt_diff之和 */
    uint32_t diffMax;                /* 500ms期间的cnt_diff最大值 */
    uint32_t diffAvg;                /* 500ms期间的cnt_diff平均值 */
    uint16_t connHandle;
    uint16_t txFlushed;              /* 500ms期间发出的包被Flush的次数，包含被软件及硬件flush */
    uint16_t rxLossPktCnt;
    uint16_t rxLossMaxContPkt;
    int8_t rssi;                     /* 平均信号强度 */
    uint8_t ackRate;                 /* 500ms期间的ACK帧接收率，tx_ack * 100 / tx_total，取值范围[0,100] */
    uint32_t reserve1;               /* 保留字段1 */
    uint32_t reserve2;               /* 保留字段2 */
    uint32_t reserve3;               /* 保留字段3 */
    uint32_t reserve4;               /* 保留字段4 */
} DLI_IOBQualityReportEvt;

typedef struct {
    uint8_t channelConnHandle;
    uint8_t channelMissedRate;    /* rx_loss / rx_pkt */
    uint8_t channelErrPacketRate; /* total_per */
    int8_t channelRssi;           /* rx */
} DLI_nodeChannelInfo;

typedef struct {
    uint32_t diffTotal;              /* 500ms期间的cnt_diff之和   组播中无参考意义 */
    uint32_t diffMax;                /* 500ms期间的cnt_diff最大值 组播中无参考意义 */
    uint32_t diffAvg;                /* 500ms期间的cnt_diff平均值 组播中无参考意义 */
    uint16_t connHandle;             /* 下行链路 handle */
    uint16_t txFlushed;              /* 500ms期间发出的包被Flush的次数，包含被软件及硬件flush */
    uint16_t rxLossPktCnt;           /* 上行两条链路 组播中无参考意义     */
    uint16_t rxLossMaxContPkt;       /* 上行两条链路 host丢包最大值 */
    int8_t rssi;                     /* 最小信号强度 */
    uint8_t ackRate;                 /* 组播中无参考意义 */
    uint32_t errPacketRate;          /* 500ms期间收包误包率 */
    uint32_t missedRate;             /* 500ms期间丢包率 */
    uint8_t txCount;                 /* tx节点数量 */
    DLI_nodeChannelInfo channelInfo[0]; /* 上行链路 */
} DLI_IMBQualityReportEvt;

typedef struct {
    uint8_t labelId;                    /* label id */
    uint8_t txPhy;                      /* 0	PHY是1M
                                           1	PHY是2M
                                           2	PHY是4M */
    uint8_t rxPhy;
    uint8_t txMcs;                      /* 0x00	调试方式和polar编码，BPSK1/4
                                           0x01	调试方式和polar编码，BPSK3/8
                                           0x02	调试方式和polar编码，QPSK1/4
                                           0x03	调试方式和polar编码，QPSK3/8
                                           0x04	调试方式和polar编码，QPSK1/2
                                           0x05	调试方式和polar编码，QPSK5/8
                                           0x06	调试方式和polar编码，QPSK3/4
                                           0x07	调试方式和polar编码，QPSK7/8
                                           0x08	调试方式和polar编码，QPSK 1
                                           0x09	调试方式和polar编码，8PSK5/8
                                           0x0A	调试方式和polar编码，8PSK3/4
                                           0x0B	调试方式和polar编码，8PSK7/8
                                           0x0C	调试方式和polar编码，8PSK 1
                                           0x0D	预留未来使用
                                           0x0E	预留未来使用
                                           0x0F	预留未来使用
                                           0x10	调试方式和polar编码，16QAM3/4
                                           0x11	调试方式和polar编码，16QAM7/8
                                           0x12	调试方式和polar编码，16QAM1 */
    uint8_t rxMcs;
    uint8_t txFrame;                    /* 0	无线帧类型一
                                           1	无线帧类型二
                                           2	无线帧类型四 */
    uint8_t rxFrame;
    uint16_t maxSduG2T;                 /* G到T最大SDU Payload字节数 */
    uint16_t maxSduT2G;
    uint16_t maxPduG2T;                 /* G到T最大PDU Payload字节数 */
    uint16_t maxPduT2G;
    uint8_t nse;                        /* 0x01-0x1F	在IMG中每一个IMB每个间隔内的子事件个数 */
} DLI_ICGLabel;

typedef struct {
    uint8_t status;
    uint16_t lcid;
    uint16_t connHandle;
    uint8_t icgId;
    uint8_t icbId;
    uint16_t icbInterval;
    uint8_t latencyG2T[3];            /*!< 0x0000EA-0x7FFFFF, G到T方向上实际的传输延迟，以us为单位 */
    uint8_t latencyT2G[3];            /*!< 0x0000EA-0x7FFFFF, T到G方向上实际的传输延迟，以us为单位 */
    uint8_t bnG2T;
    uint8_t bnT2G;
    uint8_t ftG2T;                    /*!< G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint8_t ftT2G;                    /*!< T到G方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint8_t labelCnt;
    uint8_t label[0];
} DLI_ICGLabelReportEvt;

typedef struct {
    uint8_t status;
    uint16_t lcid;
    uint16_t connHandle;
    uint8_t icgId;
    uint8_t icbId;
    uint8_t labelId;
} DLI_ICBParamUpdateEvt;

typedef struct {
    uint8_t status;
    uint16_t lcid;
    uint16_t subrateFactor;
    uint16_t peripheralLatency;
    uint16_t continuationNum;
    uint16_t supervisionTimeout;
} DLI_AcbSetSubrateEvt;

typedef struct {
    uint8_t status;
    uint16_t lcid;
    uint16_t subrateMin;
    uint16_t subrateMax;
    uint16_t peripheralLatency;
    uint16_t continuationNum;
    uint16_t supervisionTimeout;
} DLI_AcbReqSubrateEvt;

typedef struct {
    uint16_t connHandle;
} DLI_ICBRejectReqEvt;

typedef struct {
    uint16_t connHandle;
} DLI_ICBDataPathEvt;

typedef struct {
    uint8_t status;
    uint16_t connHandle;
    uint8_t oldFreqBand;              /* 0: 2.4G, 1: 5.1G, 2: 5.8G */
    uint8_t newFreqBand;              /* 0: 2.4G, 1: 5.1G, 2: 5.8G */
} DLI_FreqBandSwitchEvt;

typedef struct {
    uint16_t lcid;
    int8_t rssi;
    uint8_t rate;
} DLI_ReqQualityEvt;

#pragma pack()

typedef void (*DLI_NOCPEventCbk)(uint16_t connHandle, uint8_t numCompletedPackets);

typedef enum {
    DLI_REG_MODULE_DTAP = 0,
    DLI_REG_MODULE_MAX
} DLI_RegModuleType;

#ifdef __cplusplus
}
#endif

#endif