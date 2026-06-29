/*
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

#ifndef NEARLINK_DFT_EXCEPTION_C_H
#define NEARLINK_DFT_EXCEPTION_C_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_LEN_MAX 32
#define DFX_STR_LEN 128
#define DFX_MAC_LEN 6

typedef struct {
    char startTime[TIME_LEN_MAX];              /* 星闪音频启动时间（YYYY-MM-DD HH:MM:SS） */
    uint32_t duration;                         /* 音频持续时间，单位 ms */
    uint8_t l2hcVersion;                      /* L2HC版本信息 */
    uint8_t codecType;                        /* 编解码器类型 */
    uint8_t sampleRate;                       /* 采样率 */
    uint8_t bitDepth;                         /* 位宽 */
    uint8_t channelMode;                      /* 声道模式 */
    uint8_t deviceNumInCooperationSet;       /* 合作集中的设备数 */
    uint8_t sduInterval;                      /* 发包间隔，单位 ms */
    uint8_t flushTimeout;                     /* FT数值 */
    uint8_t burstNum;                         /* Burst参数 */
    uint16_t maxDownBitrate;                 /* 最大下行码率，单位 Kbps */
    uint32_t maxDownBitrateDuration;         /* 最大码率持续时间，单位 ms */
    uint32_t pcmWriteCnt;                    /* PCM写入次数 */
    uint32_t pcmWriteMaxInterval;            /* PCM写入最大间隔，单位 ms */
    uint32_t pcmWriteAvgInterval;            /* PCM写入平均间隔，单位 us */
    uint32_t pcmWriteGtFtCnt;                /* 写入间隔 > FT 的次数 */
    uint32_t encoderMemAllocFailedCnt;       /* 编码内存分配失败次数 */
    uint32_t encoderFailedCnt;               /* 编码接口调用失败次数 */
    uint32_t leftConnHandle;
    uint32_t leftTxPktCnt;                   /* 左耳发送总包数 */
    uint32_t leftTxNoLinkDropCnt;            /* 左耳未连接丢包数 */
    uint32_t leftTxFlushCnt;                 /* 左耳流控超时被flush的包数 */
    uint32_t leftTxUartFailedCnt;            /* 左耳UART发送失败包数 */
    uint32_t leftTxFlowCtrlCnt;              /* 左耳流控次数 */
    uint32_t leftTxFlowCtrlMaxPktCnt;        /* 左耳流控最大缓存包数 */
    uint32_t rightConnHandle;
    uint32_t rightTxPktCnt;                  /* 右耳发送总包数 */
    uint32_t rightTxNoLinkDropCnt;           /* 右耳未连接丢包数 */
    uint32_t rightTxFlushCnt;                /* 右耳流控超时被flush的包数 */
    uint32_t rightTxUartFailedCnt;           /* 右耳UART发送失败包数 */
    uint32_t rightTxFlowCtrlCnt;             /* 右耳流控次数 */
    uint32_t rightTxFlowCtrlMaxPktCnt;       /* 右耳流控最大缓存包数 */
    uint32_t intrEventEnqueueFailedCnt;      /* 中断事件入队失败次数 */
    uint32_t intrDataEnqueueFailedCnt;       /* 中断数据入队失败次数 */
    uint32_t dataInvalidFormatCnt;           /* 数据格式错误包数 */
    uint32_t badPktCnt;                      /* 接收Bad pkt标记的包数 */
    uint32_t invalidConnHandleCnt;           /* Conn handle不合法包数 */
    uint32_t leftRxConnHandle;
    uint32_t leftDataMemAllocFailedCnt;      /* 左耳接收内存分配失败次数 */
    uint32_t leftOldDataDropCnt;             /* 左耳旧包丢弃次数 */
    uint32_t leftDecoderFailedCnt;           /* 左耳解码失败次数 */
    uint32_t leftPcmSizeNotMatchCnt;         /* 左耳PCM大小不匹配次数 */
    uint32_t leftDecodedPcmDropCnt;          /* 左耳PCM缓存满丢弃次数 */
    uint32_t leftReadPcmLossCnt;             /* 左耳读PCM丢包次数 */
    uint32_t leftReadPcmMisorderCnt;         /* 左耳读PCM重复序号包次数 */
    uint32_t rightRxConnHandle;
    uint32_t rightDataMemAllocFailedCnt;     /* 右耳接收内存分配失败次数 */
    uint32_t rightOldDataDropCnt;            /* 右耳旧包丢弃次数 */
    uint32_t rightDecoderFailedCnt;          /* 右耳解码失败次数 */
    uint32_t rightPcmSizeNotMatchCnt;        /* 右耳PCM大小不匹配次数 */
    uint32_t rightDecodedPcmDropCnt;         /* 右耳PCM缓存满丢弃次数 */
    uint32_t rightReadPcmLossCnt;            /* 右耳读PCM丢包次数 */
    uint32_t rightReadPcmMisorderCnt;        /* 右耳读PCM重复序号包次数 */
    uint32_t bitrateUpgradeCnt;              /* 升码率次数 */
    uint32_t bitrateDownCnt;                 /* 降码率次数 */
    uint32_t bitrateDownFailedCnt;           /* 降码率失败次数 */
    char bitrateDownFailedReason[DFX_STR_LEN]; /* 降码率失败原因，格式：“错误码1:次数,错误码2:次数” */
    uint32_t bitrateDownSetLabelFailedCnt;   /* 切换label失败次数 */
    uint32_t bandChangeCnt;                  /* 频段切换次数 */
    char band2GInfo[DFX_STR_LEN];            /* 2.4G频段使用详细信息 */
    char band5GInfo[DFX_STR_LEN];            /* 5G频段使用详细信息 */
    uint32_t leftRxFrameCnt;
    uint32_t rightRxFrameCnt;
    uint32_t encoderLastSn;
    uint32_t decoderLastSn;
} QOSM_DftAudioStats;

typedef struct {
    char time[TIME_LEN_MAX];
    int32_t codecType;
    int32_t codecAlgo;
    int32_t errorCode;
} QOSM_DftAudioCodecExcep;

typedef struct {
    char time[TIME_LEN_MAX];
    uint8_t choppyCnt;
    uint8_t deviceAddr[DFX_MAC_LEN];
    int32_t deviceType;
    uint32_t sceneCode;
    uint32_t subsceneCode;
    uint32_t bitrate;
    uint32_t freqBand;
    uint32_t powerInfo;
    uint32_t macInfo;
    int32_t peerRssi;
    uint32_t txflushNum;
    uint32_t ackRate;
    char channelStatus24g[DFX_STR_LEN];
    int32_t noiseAvgValue24g;
    char channelStatus5g[DFX_STR_LEN];
    int32_t noiseAvgValue5g;
} QOSM_DftAudioChoppyExcep;

void DftReportAudioStats(const QOSM_DftAudioStats *stats);
void DftReportAudioChoppyExcep(const QOSM_DftAudioChoppyExcep *excep);
void DftReportAudioCodecExcep(const QOSM_DftAudioCodecExcep *excep);

#ifdef __cplusplus
}
#endif

#endif // NEARLINK_DFT_EXCEPTION_C_H
