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
 * CM for isochronous link sync data struct
 *
 ***************************************************************************/

#ifndef CM_ICB_LINK_DEF_H
#define CM_ICB_LINK_DEF_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CM_LINK_ICB = 0,                    /* 同步链路 */
    CM_LINK_ACB,                        /* 异步链路 */
} CM_LinkType;

typedef enum {
    CM_IOB = 0,                         /* 单播同步链路 */
    CM_IMB,                             /* 组播同步链路 */
} CM_ICBType;

typedef enum {
    CM_ICB_STATE_IOG_CREATED = 0,
    CM_ICB_STATE_IOG_REMOVED,
    CM_ICB_STATE_IOB_ACCEPTED,
    CM_ICB_STATE_IOB_REJECTED,
    CM_ICB_STATE_IOB_CREATED,
    CM_ICB_STATE_IOG_UPDATED,

    CM_ICB_STATE_IMG_CREATED,
    CM_ICB_STATE_IMG_REMOVED,
    CM_ICB_STATE_IMB_ACCEPTED,
    CM_ICB_STATE_IMB_REJECTED,
    CM_ICB_STATE_IMB_CREATED,
    CM_ICB_STATE_IMG_UPDATED,

    CM_ICB_STATE_ICB_DATA_PATH_SETUP,
    CM_ICB_STATE_ICB_DATA_PATH_REMOVED,
    CM_ICB_STATE_ICB_DELETED,
    CM_ICB_STATE_MAX,
} CM_ICBConnectionState;

typedef enum {
    CM_ICB_SUCCESS = 0,
    CM_IOB_FAILED,
    CM_IMB_FAILED,
    CM_ICB_FAILED,
} CM_ICBErrorCode;

/**
 * @brief 星闪频段
 */
typedef enum {
    CM_FREQ_BAND_2D4  = 0x01,  /*!< 2.4G频段 */
    CM_FREQ_BAND_5D1  = 0x02,  /*!< 5.1G频段 */
    CM_FREQ_BAND_5D8  = 0x04,  /*!< 5.8G频段 */
} CM_FreqBand;

#pragma pack(1)
typedef struct {
    uint8_t id;                         /* 用于标识一个ICB，取值范围[0x00,0xEF] */
    struct CM_ICB {
        uint16_t maxSduG2T;             /* G到T最大SDU Payload字节数 */
        uint16_t maxSduT2G;             /* T到G最大SDU Payload字节数 */
        uint8_t rtnG2T;                 /* G到T每一个IMB数据PDU重传次数 */
        uint8_t rtnT2G;                 /* T到G每一个IMB数据PDU重传次数 */
    } *param;
} CM_ICBParam;
#pragma pack()

typedef struct {
    CM_ICBType type;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint32_t sduIntervalG2T;            /* G到T两个连续SDU之间的时间间隔，us */
    uint32_t sduIntervalT2G;            /* T到G两个连续SDU之间的时间间隔，us */
    uint8_t sca;                        /* 睡眠时钟精度 */
    uint8_t packing;                    /* 当前仅支持交叉 */
    uint8_t framing;                    /* 当前仅支持未切分 */
    uint16_t maxLatencyG2T;             /* G到T最大传输延迟，ms */
    uint16_t maxLatencyT2G;             /* T到G最大传输延迟，ms */
    uint8_t icbCnt;                     /* ICB链路数 */
    uint8_t paramCnt;                   /* 每一个ICB链路需要设置的参数个数 */
    CM_ICBParam *icbParam;              /* 同一个ICG底下所有ICB链路需要设置的参数 */
} CM_ICGParam;

#pragma pack(1)
typedef struct {
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
    uint8_t mcsG2T;                     /* 0x00	G到T方向上使用的调试方式和polar编码，BPSK1/4
                                           0x01	G到T方向上使用的调试方式和polar编码，BPSK3/8
                                           0x02	G到T方向上使用的调试方式和polar编码，QPSK1/4
                                           0x03	G到T方向上使用的调试方式和polar编码，QPSK3/8
                                           0x04	G到T方向上使用的调试方式和polar编码，QPSK1/2
                                           0x05	G到T方向上使用的调试方式和polar编码，QPSK5/8
                                           0x06	G到T方向上使用的调试方式和polar编码，QPSK3/4
                                           0x07	G到T方向上使用的调试方式和polar编码，QPSK7/8
                                           0x08	G到T方向上使用的调试方式和polar编码，QPSK 1
                                           0x09	G到T方向上使用的调试方式和polar编码，8PSK5/8
                                           0x0A	G到T方向上使用的调试方式和polar编码，8PSK3/4
                                           0x0B	G到T方向上使用的调试方式和polar编码，8PSK7/8
                                           0x0C	G到T方向上使用的调试方式和polar编码，8PSK 1
                                           0x0D	预留未来使用
                                           0x0E	预留未来使用
                                           0x0F	预留未来使用
                                           0x10	G到T方向上使用的调试方式和polar编码，16QAM3/4
                                           0x11	G到T方向上使用的调试方式和polar编码，16QAM7/8
                                           0x12	G到T方向上使用的调试方式和polar编码，16QAM1 */
    uint8_t mcsT2G;
    uint8_t pilotG2T;                   /* 0x00-0x07 标识Polar编码时插导频pilot的比例，代表[数据:导频]=[2^ratio:1] */
    uint8_t pilotT2G;
    uint8_t bnG2T;                      /* 0x00	G到T方向上没有ICB数据包
                                           0x01-0x0F	G到T方向上每一个IMB每个间隔内传输新ICB数据包的个数 */
    uint8_t bnT2G;
} CM_ICBTestParam;
#pragma pack()

typedef struct {
    CM_ICBType type;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint8_t labelId;                    /* label */
    uint32_t sduIntervalG2T;            /* G到T两个连续SDU之间的时间间隔，us */
    uint32_t sduIntervalT2G;            /* T到G两个连续SDU之间的时间间隔，us */
    uint8_t ftG2T;                      /* G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint8_t ftT2G;                      /* T到G方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint16_t icbInterval;               /* 连续两个ICB Anchor Point的间隔时间，
                                            取值范围[0x0014,0x3E80]，时间 = N * 0.25ms，时间范围[5ms,4s] */
    uint8_t sca;                        /* 睡眠时钟精度 */
    uint8_t packing;                    /* 当前仅支持交叉 */
    uint8_t framing;                    /* 当前仅支持未切分 */
    uint8_t icbCnt;
    CM_ICBTestParam *icbParam;
} CM_ICGTestParam;

typedef struct {
    uint16_t connHandle;
    uint16_t lcid;
    uint8_t direction;
    uint8_t labelId;
} CM_ICBChannel;

typedef struct {
    CM_ICBType type;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint8_t icbCnt;
    CM_ICBChannel *icb;
} CM_ICGLabelParam;

typedef struct {
    CM_ICBType type;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
} CM_ICGRemovedParam;

typedef struct {
    uint16_t connHandle;
    int32_t rssi;
    uint32_t ackRate;
    uint32_t packetIntervalMs;
} CM_ICBQualityParam;

typedef struct {
    CM_ICBType type;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint8_t labelId;
    uint8_t channelCnt;
    CM_ICBChannel *channel;
} CM_ICBConnectionParam;

typedef struct {
    uint16_t connHandle;                /* 单个同步链路handler */
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
} CM_ICBDataPath;

typedef struct {
    uint16_t connHandle;                /* 单个同步链路handler */
    uint8_t direction;                  /* 0： host到controller，1：controller到host */
} CM_ICBRemovedDataPath;

typedef struct {
    CM_ICBType type;
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    uint8_t labelId;
    uint8_t icbCnt;
    uint16_t *connHandle;
} CM_ICGUpdatedParam;

typedef struct {
    uint8_t freqBand;                   /* 详见CM_FreqBand，可通过 | 拼接 */
} CM_FreqBandParam;

typedef struct {
    CM_ICBConnectionState state;
    CM_ICBErrorCode errorCode;
    CM_LinkType disconnectType;         /* 仅用于断链事件类型区分：同步链路断链、异步链路断链 */
    uint8_t id;                         /* 用于标识一个ICG，取值范围[0x00,0xEF] */
    bool isIMG;
    uint16_t gHandle;
    uint8_t channelCnt;
    CM_ICBChannel *channel;
} CM_ICBConnection;

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
} CM_ICBLabel;

typedef struct {
    CM_ICBErrorCode errorCode;
    uint16_t lcid;
    uint16_t connHandle;
    uint8_t icgId;                      /* 用于标识一个ICG/IMG，取值范围[0x00,0xEF] */
    uint8_t icbId;                      /* 用于标识一个ICB/IMB，取值范围[0x00,0xEF] */
    uint16_t icbInterval;               /*!< 连续两个ICB Anchor Point的间隔时间，
                                            取值范围[0x0014,0x3E80]，时间 = N * 0.25ms，时间范围[5ms,4s] */
    uint32_t latencyG2T;                /*!< 0x0000EA-0x7FFFFF, G到T方向上实际的传输延迟，以us为单位 */
    uint32_t latencyT2G;                /*!< 0x0000EA-0x7FFFFF, T到G方向上实际的传输延迟，以us为单位 */
    uint8_t bnG2T;                      /* 0x00	G到T方向上没有ICB数据包
                                           0x01-0x0F	G到T方向上每一个IMB每个间隔内传输新ICB数据包的个数 */
    uint8_t bnT2G;
    uint8_t ftG2T;                      /*!< G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint8_t ftT2G;                      /*!< T到G方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint8_t labelCnt;
    CM_ICBLabel *label;
} CM_ICBLabelReportParam;

typedef struct {
    CM_ICBType icbType;
    uint32_t diffTotal;              /* 500ms期间的cnt_diff之和 */
    uint32_t diffMax;                /* 500ms期间的cnt_diff最大值 */
    uint32_t diffAvg;                /* 500ms期间的cnt_diff平均值 */
    uint16_t connHandle;
    uint16_t txFlushed;              /* 500ms期间发出的包被Flush的次数，包含被软件及硬件flush */
    uint16_t rxLossPktCnt;           /* 该周期内接收丢包数 */
    uint16_t rxLossMaxContPkt;       /* 该周期内最大连续丢包数 */
    int8_t rssi;                     /* 平均信号强度 */
    uint8_t ackRate;                 /* 500ms期间的ACK帧接收率，tx_ack * 100 / tx_total，取值范围[0,100] */
    uint8_t missedRate;              /* 组播 500ms期间丢包率 */
    uint8_t errPacketRate;           /* 组播 500ms期间收包误包率 */
    uint32_t reserve1;               /* 保留字段1 */
    uint32_t reserve2;               /* 保留字段2 */
    uint32_t reserve3;               /* 保留字段3 */
    uint32_t reserve4;               /* 保留字段4 */
}CM_ICBQuality;

#pragma pack (1)

/**
 * @brief  星闪切换频段上报
 */
typedef struct {
    uint8_t status;              /* 芯片错误码，详见dli_errno.h */
    uint16_t lcid;
    uint16_t connHandle;
    uint8_t oldFreqBand;         /* 0: 2.4G, 1: 5.1G, 2: 5.8G */
    uint8_t newFreqBand;         /* 0: 2.4G, 1: 5.1G, 2: 5.8G */
} CM_FreqBandSwitchParam;
#pragma pack ()

typedef void (*CM_ICBConnectionCbk)(CM_ICBConnection *connection);
typedef void (*CM_ICBLabelReportCbk)(CM_ICBLabelReportParam *param);
typedef void (*CM_ICBQualityCbk)(CM_ICBQuality *quality);
typedef void (*CM_FreqBandListener)(CM_FreqBandSwitchParam *param);

typedef struct {
    CM_ICBConnectionCbk connectionCbk;
    CM_ICBLabelReportCbk labelReportCbk;
    CM_ICBQualityCbk qualityReportCbk;
} CM_ICBCallback;

#ifdef __cplusplus
}
#endif

#endif