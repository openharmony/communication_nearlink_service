/****************************************************************************
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
****************************************************************************/

/****************************************************************************
 *
 * this file contains qosm auto rate struct.
 *
 ***************************************************************************/

#ifndef QOSM_AUTORATE_DEF_H
#define QOSM_AUTORATE_DEF_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QOSM_AUTORATE_MAX_LINK_CNT 16 // 大小与CM_MAX_CHANNEL_COUNT相同
#define QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT 8

/**
 * @brief 星闪音频码率
 */
typedef enum {
    QOSM_AUTO_BITRATE_4600  = 4600,  /* 4.6 Mbps */
    QOSM_AUTO_BITRATE_2300  = 2300,  /* 2.3 Mbps */
    QOSM_AUTO_BITRATE_1500  = 1500,  /* 1.5 Mbps */
    QOSM_AUTO_BITRATE_640  = 640,    /* 640 Kbps */
    QOSM_AUTO_BITRATE_320  = 320,    /* 320 Kbps */
    QOSM_AUTO_BITRATE_256  = 256,    /* 256 Kbps */
    QOSM_AUTO_BITRATE_192  = 192,    /* 192 Kbps */
    QOSM_AUTO_BITRATE_64  = 64,      /* 64 Kbps */
    QOSM_AUTO_BITRATE_32  = 32,      /* 32 Kbps */
} QOSM_AutoBitrate_T;

/**
 * @brief 星闪音频占空比
 */
typedef enum {
    QOSM_DUTY_CYCLE_100P = 0,   /* 100% */
    QOSM_DUTY_CYCLE_50P = 1,    /* 50% */
    QOSM_DUTY_CYCLE_20P = 2,    /* 20% */
} QOSM_AutoDutyCycle_T;

typedef struct {
    uint32_t sduInterval;              /* G到T两个连续SDU之间的时间间隔，us */
    uint8_t sca;                       /* 睡眠时钟精度 */
    uint8_t packing;                   /* 当前仅支持交叉 */
    uint8_t framing;                   /* 当前仅支持未切分 */
    uint16_t maxLatency;               /* G到T最大传输延迟，ms */
    uint16_t icbInterval;              /*!< 连续两个IMB Anchor Point的间隔时间，
                                          取值范围[0x0014,0x3E80]，时间 = N * 0.25ms，时间范围[5ms,4s] */
    uint8_t ft;                        /*!< G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint8_t rtn;                       /* G到T每一个IMB数据PDU重传次数 */
    uint16_t maxSdu;                   /* G到T最大SDU Payload字节数 */
    uint16_t maxPdu;                   /* G到T最大PDU Payload字节数 */
    uint8_t nse;                          /* 0x01-0x1F	在IMG中每一个IMB每个间隔内的子事件个数 */
    uint8_t bn;                        /* 0x00	G到T方向上没有ICB数据包
                                          0x01-0x0F	G到T方向上每一个IMB每个间隔内传输新ICB数据包的个数 */
    uint8_t phy;                       /* 0	G到T方向上传输包使用的PHY是1M
                                          1	G到T方向上传输包使用的PHY是2M
                                          2	G到T方向上传输包使用的PHY是4M */
    uint8_t mcs;                       /* 0x00	G到T方向上使用的调制方式和polar编码，BPSK1/4
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
    uint8_t pilot;                     /* 0x00-0x07 标识Polar编码时插导频pilot的比例，代表[数据:导频]=[2^ratio:1] */
    uint8_t frame;                     /* 0	无线帧类型一
                                          1	无线帧类型二
                                          2	无线帧类型四 */
    uint16_t icbNum;                   /* 同步链路buffer数 */
} QOSM_ICBParam;

typedef enum {
    QOSM_QOSINDEX_AUDIO = 1,
    QOSM_QOSINDEX_LOW_LATENCY = 2,
    QOSM_QOSINDEX_CALL = 3,
    QOSM_QOSINDEX_SPATIAL_AUDIO = 4,
    QOSM_QOSINDEX_HD_RECORDING = 5,
    QOSM_QOSINDEX_VOICE_ASSISTANT = 6,
    QOSM_QOSINDEX_OTHERS = 7,
    QOSM_QOSINDEX_KARAOKE = 9,
    QOSM_QOSINDEX_MAX,
} QOSM_QosIndex;

typedef struct {
    uint8_t qosId;                     /* 全局只有一个调用方，qosId充当icgId */
    QOSM_QosIndex qosIndex;            /* 星闪QoS索引 */
    uint16_t supportedBitrate[QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT];    /* 支持的码率 */
    uint32_t supportedBitrateCnt;      /* 支持的码率数 */
    uint16_t bitrate;                  /* 单设备的起播码率 */
    uint8_t linkCnt;                   /* 需要创建的链路个数 */
    uint8_t lcidCnt;                   /* 异步链路数 */
    uint8_t lcid[QOSM_AUTORATE_MAX_LINK_CNT]; /* 异步链路唯一标识 */
} QOSM_AutoRateParam;

typedef struct {
    uint8_t qosId;
    QOSM_QosIndex qosIndex;
    uint8_t labelId;
    uint8_t msgType;
    uint32_t result;
} QOSM_AutoRateRecvMsgParam;

typedef struct {
    uint16_t connHandle;               /* 同步链路唯一标识 */
    uint16_t lcid;                     /* 逻辑链路ID */
} QOSM_ConnParam;

typedef struct {
    uint8_t qosId;                     /* 全局只有一个调用方，qosId充当icgId */
    uint8_t linkCnt;
    QOSM_ConnParam *link;
} QOSM_AutoRateConnParam;

typedef enum {
    QOSM_PATHID_HOST = 0,
    QOSM_PATHID_DSP = 1,
    QOSM_PATHID_SENSORHUB = 2,
} QOSM_PathId;

typedef struct {
    uint8_t qosId;                     /* 全局只有一个调用方，qosId充当icgId */
    uint16_t connHandle;               /* 单个同步链路handler */
    uint8_t direction;                 /* 0： host到controller，1：controller到host */
    QOSM_PathId pathId;                /* 0：数据通过HCI传输，其他值：通过Vendor专用传输 */
    struct {
        uint8_t codecId;               /* 0xff表示使用vendor codec */
        uint16_t vendorId;             /* 仅codecId为0xff时有效 */
        uint16_t vendorCodecId;        /* 仅codecId为0xff时有效 */
    } codec;
    uint32_t controllerDelay;          /* Controller延迟时间，以us为单位，取值范围[0x000000,0x3D0900]，时间范围[0s,4s] */
    uint8_t codecConfigLen;
    uint8_t *codecConfigData;
} QOSM_AutoRateDataPath;

typedef struct {
    uint8_t qosId;                     /* 全局只有一个调用方，qosId充当icgId */
    uint16_t connHandle;               /* 单个同步链路handler */
    uint8_t direction;                 /* 0： host到controller，1：controller到host */
} QOSM_AutoRateDeletedDataPath;

typedef struct {
    uint16_t supportedBitrate[QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT];
    uint32_t supportedBitrateCnt;
} QOSM_AutoRateEarphoneFeedbackParam;

typedef struct {
    uint16_t maxBitrate;               /* 共存模式反馈的最大码率限制，全局生效 */
    uint8_t dutyCycle;                 /* 共存模式反馈的占空比限制，全局生效 */
} QOSM_AutoRateCoexistSuggestionParam;

typedef enum {
    QOSM_FREQUENCY_BAND_2_4 = 0,
    QOSM_FREQUENCY_BAND_5_1 = 1,
    QOSM_FREQUENCY_BAND_5_8 = 2,
} QOSM_AutoRateFrequencyBand;

typedef struct {
    bool enable;
    QOSM_AutoRateFrequencyBand band;
} QOSM_AutoRateFrequencyBandParam;

typedef enum {
    QOSM_PARAM_SETTED = 0,
    QOSM_PARAM_REMOVED,
} QOSM_ParamState;

typedef struct {
    uint8_t qosId;
    QOSM_ParamState state;
    uint32_t result;                   /* 详见qosm_errno.h */
    bool isIMG;
    uint16_t gHandle;
    uint8_t linkCnt;
    uint16_t *connHandle;
} QOSM_ParamCb;

typedef enum {
    QOSM_CONNECTION_ADDED = 0,
    QOSM_CONNECTION_DELETED,
} QOSM_ConnectionState;

typedef struct {
    uint8_t qosId;
    QOSM_ConnectionState state;
    uint32_t result;                   /* 详见qosm_errno.h */
    bool isIMG;
    uint16_t gHandle;
    uint32_t bitrate; /* 下行bitrate */
    uint8_t linkCnt;
    QOSM_ConnParam *link;
} QOSM_ConnParamCb;

typedef enum {
    QOSM_DATAPATH_ADDED = 0,
    QOSM_DATAPATH_DELETED,
} QOSM_DataPathState;

typedef struct {
    uint8_t qosId;
    QOSM_DataPathState state;
    uint32_t result;                   /* 详见qosm_errno.h */
    uint16_t connHandle;
    uint8_t direction;                 /* 0： host到controller，1：controller到host */
} QOSM_DataPathParamCb;

typedef enum {
    QOSM_BITRATE_DOWN = 0,
    QOSM_BITRATE_UP,
} QOSM_BitrateDirection;

typedef struct {
    uint8_t qosId;
    QOSM_BitrateDirection direction;   /* 指示码率变化方向，提高码率：QOSM_BITRATE_UP，降低码率：QOSM_BITRATE_DOWN */
    uint8_t labelId;                   /* 变化后的labelId */
    uint16_t downwardBitrate;          /* 变化后的单设备下行码率 */
    uint16_t upwardBitrate;            /* 变化后的单设备上行码率 */
    uint8_t qosIndex;                  /* 指示本条链路使用的QOS */
    uint8_t qosLevel;                  /* 变化后的码率档位 */
    uint8_t dutyCycle;                 /* 变化后的占空比 */
    uint8_t availableBitratesCnt;
    uint16_t availableBitrates[QOSM_AUTORATE_MAX_SUPPORTED_BITRATE_CNT];
} QOSM_BitrateParamCb;

typedef enum {
    CHANGE_FRAME_TYPE_REQ = 0,        /* 切帧格式请求   */
    CHANGE_FRAME_TYPE_RSP,            /* 切帧格式回复   */
    CHANGE_PEER_BITRATE_REQ,          /* 对端切码率请求 */
    CHANGE_PEER_BITRATE_RSP,          /* 对端切码率回复 */
    CHANGE_LABEL_ID_REQ,              /* 切同步链路请求 */
    CHANGE_LABEL_ID_RSP,              /* 切同步链路回复 */
    NTF_VOICE_CALL_AUTORATE_ABILITY,  /* 上层通知通话 autorate 能力*/
} QOSM_AutoRateMsgType;

typedef enum {
    STATE_NONE = 0,
    STATE_FRAME_CHANGING,
    STATE_FRAME_CHANGED,
    STATE_BITRATE_CHANGING,
    STATE_BITRATE_CHANGED,
    STATE_LABEL_ID_CHANGING,
    STATE_LABEL_ID_CHANGED,
} QOSM_IMBAutoRateState;

typedef struct {
    uint8_t qosId;
    uint8_t linkCnt;                                /* 异步链路数 */
    uint8_t lcid[QOSM_AUTORATE_MAX_LINK_CNT];       /* 异步链路唯一标识 */
    uint8_t labelId;
    uint8_t qosIndex;
    QOSM_BitrateDirection direction;
    uint16_t downwardBitrate;
    uint16_t upwardBitrate;
    QOSM_AutoRateMsgType msgType;
    uint32_t result;
} QOSM_AutoRateSendMsgCb;

typedef struct {
    uint32_t result;                   /* 详见qosm_errno.h */
    QOSM_AutoRateFrequencyBand frequencyBand;
} QOSM_FrequencyBandParamCb;

typedef enum {
    QOSM_POWER_LEVEL_1 = 1,
    QOSM_POWER_LEVEL_2 = 2,
    QOSM_POWER_LEVEL_3 = 3,
    QOSM_POWER_LEVEL_4 = 4,
    QOSM_POWER_LEVEL_5 = 5,
    QOSM_POWER_LEVEL_MIN = QOSM_POWER_LEVEL_5,
    QOSM_POWER_LEVEL_6 = 6,
    QOSM_POWER_LEVEL_7 = 7,
    QOSM_POWER_LEVEL_8 = 8,
    QOSM_POWER_LEVEL_MAX = QOSM_POWER_LEVEL_8,
} QOSM_AutoRatePowerLevel;

typedef struct {
    uint32_t result;                   /* 详见qosm_errno.h */
    uint8_t enable;                    /* 0x00, 关闭高功率
                                          0x01, 开启高功率 */
    QOSM_AutoRatePowerLevel level;
    uint16_t connHandle;
} QOSM_HighPowerModeParamCb;

typedef void (*QOSM_ParamChangedCallback)(const QOSM_ParamCb *param);
typedef void (*QOSM_ConnectionChangedCallback)(const QOSM_ConnParamCb *param);
typedef void (*QOSM_DataPathChangedCallback)(const QOSM_DataPathParamCb *param);
typedef void (*QOSM_BitrateChangedCallback)(const QOSM_BitrateParamCb *param, uint8_t paramCnt);
typedef void (*QOSM_FrequencyBandChangedCallback)(const QOSM_FrequencyBandParamCb *param);
typedef void (*QOSM_HighPowerModeChangedCallback)(const QOSM_HighPowerModeParamCb *param);
typedef void (*QOSM_AutoRateSendMsgCallback)(const QOSM_AutoRateSendMsgCb* param);

typedef struct {
    QOSM_ParamChangedCallback paramChangedCbk;
    QOSM_ConnectionChangedCallback connChangedCbk;
    QOSM_DataPathChangedCallback dataPathChangedCbk;
    QOSM_BitrateChangedCallback bitrateChangedCbk;
    QOSM_FrequencyBandChangedCallback frequencyBandChangedCbk;
    QOSM_HighPowerModeChangedCallback highPowerModeChangedCbk;
    QOSM_AutoRateSendMsgCallback callBitrateUpDownCbk;
} QOSM_AutoRateCallback;

#ifdef __cplusplus
}
#endif
#endif