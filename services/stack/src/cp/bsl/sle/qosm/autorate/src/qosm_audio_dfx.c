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
#include "qosm_audio_dfx.h"
#include "qosm_antenna_dfx.h"
#include <time.h>
#include <inttypes.h>
#include "qosm_log.h"
#include "sysdep.h"
#include "cp_worker.h"
#include "nearlink_dft_exception_c.h"
#include "qosm_uevent.h"
#include "sdf_mem.h"
#include "qosm_autorate_def.h"
#include "qosm_autorate_report.h"
#include "qosm_errno.h"
#include "cm_icb_def.h"
#include "securec.h"

#define DECODE_TABLE_LEN 256
#define TLV_HEADER_LEN 2
#define QOSM_US_TO_MS 1000
#define QOSM_MS_TO_SEC 1000
#define QOSM_INVALID_CONN_HANDLE 0xffff
#ifndef UT_TEST
#define QOSM_CHOPPY_REPORT_INTERVAL_MS (60 * 1000)
#else
#define QOSM_CHOPPY_REPORT_INTERVAL_MS 0
#endif

#define QOSM_REPORT_CNT_IN_DSP_OFF_MAX 8
#define QOSM_REPORT_CNT_IN_FIRST_STARTED 1

enum {
    QOSM_BITRATE_64,
    QOSM_BITRATE_96,
    QOSM_BITRATE_192,
    QOSM_BITRATE_256,
    QOSM_BITRATE_320,
    QOSM_BITRATE_640,
    QOSM_BITRATE_960,
    QOSM_BITRATE_1500,
    QOSM_BITRATE_2300,
    QOSM_BITRATE_4600,
    QOSM_BITRATE_MAX,
};

enum {
    DFT_EVENT_TYPE_CODEC_EXCEP,
    DFT_EVENT_TYPE_CHOPPY_EXCEP,
    DFT_EVENT_TYPE_BITRATE_REPORT,
    DFT_EVENT_TYPE_CODEC_START_STATUS,
    DFT_EVENT_TYPE_STATS,
    DFT_EVENT_TYPE_FLOW_CONTROL,
    DFT_EVENT_TYPE_RX_LOST,
    DFT_EVENT_TYPE_RX_NO_PKT,
    DFT_EVENT_TYPE_QUALITY_REPORT,
    DFT_EVENT_TYPE_BITRATE_CHANGE_DECISION,
    DFT_EVENT_TYPE_MAX,
};

#define QOSM_AUDIO_DFX_DSP_STATS_HDR_LEN 3
enum {
    QOSM_AUDIO_DFX_DSP_STATS_TYPE_FLAG_INDEX,
    QOSM_AUDIO_DFX_DSP_STATS_LEN_INDEX,
    QOSM_AUDIO_DFX_DSP_STATS_SEQ_INDEX,
    QOSM_AUDIO_DFX_DSP_STATS_VALUE_INDEX
};

enum {
    QOSM_CHOPPY_START,
    QOSM_CHOPPY_ENCODE_FAILED,
    QOSM_CHOPPY_DROP_PKT,
    QOMS_CHOPPY_TX_FLUSHED,
};

enum {
    QOSM_AUDIO_DFX_CODEC_ENCODER,
    QOSM_AUDIO_DFX_CODEC_DECODER,
    QOSM_AUDIO_DFX_CODEC_MAX,
};

enum {
    QOSM_AUDIO_DFX_CODEC_STATUS_STOP,
    QOSM_AUDIO_DFX_CODEC_STATUS_START,
    QOSM_AUDIO_DFX_CODEC_STATUS_TX_READY,
    QOSM_AUDIO_DFX_CODEC_STATUS_RX_FIRST_PKT,
    QOSM_AUDIO_DFX_CODEC_STATUS_MAX,
};

#define QOSM_AUDIO_DFX_CODEC_EXCEPTION_LEN 5

#define QOSM_AUDIO_DFX_GET_TYPE(type_) (((type_) >> 4) & 0xf)
#define QOSM_AUDIO_DFX_GET_FLAG(type_) ((type_) & 0xf)
#define QOSM_AUDIO_DFX_GET_SEQ(type_) (((type_) >> 4) & 0xf)
#define QOSM_AUDIO_DFX_GET_SUBSEQ(type_) ((type_) & 0xf)

#define QOSM_BASE64_LEN_TO_BIN_LEN(encodeLen_) ((encodeLen_) / 4 * 3)
#define QOSM_BASE64_LEN_IS_VALID(encodeLen_) ((encodeLen_) > 0 && ((encodeLen_) % 4) == 0)

#define QOSM_POWER_LEVEL_CNT (QOSM_POWER_LEVEL_MAX - QOSM_POWER_LEVEL_MIN + 1)

#define QOSM_BITRATE_CHANGE_ERROR_MAX_CNT 5

struct QOSM_AudioDfxErrorCnt {
    int error;
    uint32_t cnt;
};

struct QOSM_AudioDfx {
    uint64_t startTs;

    struct QOSM_AudioDfxConn conn[QOSM_AUDIO_DFX_CONN_CNT];

    uint32_t reportCnt;
    QOSM_DftAudioStats stats;

    uint32_t curBitrate;
    uint32_t curBitrateIdx;
    uint64_t curBitrateStartTs;
    uint64_t bitrateContinousTime[QOSM_BITRATE_MAX];
    struct QOSM_AudioDfxErrorCnt bitrateChangeFail[QOSM_BITRATE_CHANGE_ERROR_MAX_CNT];
    uint32_t bitrateChangeFailCnt;

    uint32_t powerLevelIndex;

    uint32_t curBand;
    uint64_t curBandStartTs;
    uint64_t bandContinousTime[QOSM_AUDIO_DFX_BAND_MAX][QOSM_POWER_LEVEL_CNT];

    uint64_t choppyReportedTs;

    uint64_t dspStoppedTs;
    uint64_t dspStarted;
    uint32_t reportCntInDspStopped;
    bool firstStarted;
    uint8_t codecType;
};

static QOSM_DftAudioStats g_qosmAudioStats;
static struct QOSM_AudioDfx g_qosmAudioDfx;
static QOSM_AudioDfxDspStatusCb g_qosmAudioDfxDspStatusCb = NULL;
static QOSM_AudioDfxFlowCtrlCb g_qosmAudioDfxFlowCtrlCb = NULL;

static const uint32_t g_qosmAudioDfxBitrateArray[QOSM_BITRATE_MAX] = {
    64, 96, 192, 256, 320, 640, 960, 1500, 2300, 4600,
};

static const unsigned char g_decodingTable[DECODE_TABLE_LEN] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x3f,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint32_t QOSM_Base64Decode(const char *data, uint32_t inputLength, uint8_t *decodedData, uint32_t decodedDataLen)
{
    if (!QOSM_BASE64_LEN_IS_VALID(inputLength) || decodedDataLen < QOSM_BASE64_LEN_TO_BIN_LEN(inputLength)) {
        return 0;
    }

    uint32_t outputLength = decodedDataLen;
    if (data[inputLength - 1] == '=') {
        outputLength--;
    }
    if (data[inputLength - 2] == '=') {
        outputLength--;
    }

    for (size_t index = 0, outputIndex = 0; index < inputLength;) {
        uint32_t sextetA = data[index] == '=' ? 0 & index++ : g_decodingTable[(int)data[index++]];
        uint32_t sextetB = data[index] == '=' ? 0 & index++ : g_decodingTable[(int)data[index++]];
        uint32_t sextetC = data[index] == '=' ? 0 & index++ : g_decodingTable[(int)data[index++]];
        uint32_t sextetD = data[index] == '=' ? 0 & index++ : g_decodingTable[(int)data[index++]];

        uint32_t triple = (sextetA << 3 * 6)
                        + (sextetB << 2 * 6)
                        + (sextetC << 1 * 6)
                        + (sextetD << 0 * 6);

        if (outputIndex < outputLength) {
            decodedData[outputIndex++] = (triple >> 2 * 8) & 0xFF;
        }
        if (outputIndex < outputLength) {
            decodedData[outputIndex++] = (triple >> 1 * 8) & 0xFF;
        }
        if (outputIndex < outputLength) {
            decodedData[outputIndex++] = (triple >> 0 * 8) & 0xFF;
        }
    }

    return outputLength;
}

#define TIME_CONVERSION_UNIT_THOUSAND 1000

uint64_t QOSM_GetCurrTimeMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * TIME_CONVERSION_UNIT_THOUSAND +
        ts.tv_nsec / (TIME_CONVERSION_UNIT_THOUSAND * TIME_CONVERSION_UNIT_THOUSAND);
}

enum {
    DSP_NL_DFT_STATS_DURATION,
    DSP_NL_DFT_STATS_CODEC_INFO,
    DSP_NL_DFT_STATS_PCM_WRITE_CNT,
    DSP_NL_DFT_STATS_PCM_WRITE_MAX_INTERVAL,
    DSP_NL_DFT_STATS_PCM_WRITE_AVG_INTERVAL,
    DSP_NL_DFT_STATS_PCM_WRITE_GT_FT_CNT,
    DSP_NL_DFT_STATS_ENCODER_MEM_ALLOC_FAILED_CNT,
    DSP_NL_DFT_STATS_ENCODER_FAILED_CNT,
    DSP_NL_DFT_STATS_LEFT_TX_CONN_HANDLE,
    DSP_NL_DFT_STATS_LEFT_TX_PKT_CNT,
    DSP_NL_DFT_STATS_LEFT_TX_NO_LINK_DROP_CNT,
    DSP_NL_DFT_STATS_LEFT_TX_FLUSH_CNT,
    DSP_NL_DFT_STATS_LEFT_TX_UART_FAILED_CNT,
    DSP_NL_DFT_STATS_LEFT_TX_FLOW_CTRL_CNT,
    DSP_NL_DFT_STATS_LEFT_TX_FLOW_CTRL_MAX_PKT_CNT,
    DSP_NL_DFT_STATS_RIGHT_TX_CONN_HANDLE,
    DSP_NL_DFT_STATS_RIGHT_TX_PKT_CNT,
    DSP_NL_DFT_STATS_RIGHT_TX_NO_LINK_DROP_CNT,
    DSP_NL_DFT_STATS_RIGHT_TX_FLUSH_CNT,
    DSP_NL_DFT_STATS_RIGHT_TX_UART_FAILED_CNT,
    DSP_NL_DFT_STATS_RIGHT_TX_FLOW_CTRL_CNT,
    DSP_NL_DFT_STATS_RIGHT_TX_FLOW_CTRL_MAX_PKT_CNT,
    DSP_NL_DFT_STATS_INTR_EVENT_ENQUEUE_FAILED_CNT,
    DSP_NL_DFT_STATS_INTR_DATA_ENQUEUE_FAILED_CNT,
    DSP_NL_DFT_STATS_DATA_INVALID_FORMAT_CNT,
    DSP_NL_DFT_STATS_BAD_PKT_CNT,
    DSP_NL_DFT_STATS_INVALID_CONN_HANDLE_CNT,
    DSP_NL_DFT_STATS_LEFT_RX_CONN_HANDLE,
    DSP_NL_DFT_STATS_LEFT_DATA_MEM_ALLOC_FAILED_CNT,
    DSP_NL_DFT_STATS_LEFT_OLD_DATA_DROP_CNT,
    DSP_NL_DFT_STATS_LEFT_DECODER_FAILED_CNT,
    DSP_NL_DFT_STATS_LEFT_PCM_SIZE_NOT_MATCH_CNT,
    DSP_NL_DFT_STATS_LEFT_DECODED_PCM_DROP_CNT,
    DSP_NL_DFT_STATS_LEFT_READ_PCM_LOSS_CNT,
    DSP_NL_DFT_STATS_LEFT_READ_PCM_MISORDER_CNT,
    DSP_NL_DFT_STATS_RIGHT_RX_CONN_HANDLE,
    DSP_NL_DFT_STATS_RIGHT_DATA_MEM_ALLOC_FAILED_CNT,
    DSP_NL_DFT_STATS_RIGHT_OLD_DATA_DROP_CNT,
    DSP_NL_DFT_STATS_RIGHT_DECODER_FAILED_CNT,
    DSP_NL_DFT_STATS_RIGHT_PCM_SIZE_NOT_MATCH_CNT,
    DSP_NL_DFT_STATS_RIGHT_DECODED_PCM_DROP_CNT,
    DSP_NL_DFT_STATS_RIGHT_READ_PCM_LOSS_CNT,
    DSP_NL_DFT_STATS_RIGHT_READ_PCM_MISORDER_CNT,
    DSP_NL_DFT_STATS_LEFT_RX_FRAME_CNT,
    DSP_NL_DFT_STATS_RIGHT_RX_FRAME_CNT,
    DSP_NL_DFT_STATS_ENCODER_LAST_SN,
    DSP_NL_DFT_STATS_DECODER_LAST_SN,
    DSP_NL_DFT_STATS_PARAM_BUTT,
};

#define QOSM_MAKE_ADDR(statsParam_) { #statsParam_, &g_qosmAudioStats.statsParam_ }

static struct {
    const char *paramStr;
    uint32_t *param;
} g_qosmParamArray[DSP_NL_DFT_STATS_PARAM_BUTT] = {
    QOSM_MAKE_ADDR(duration),
    { "", NULL },
    QOSM_MAKE_ADDR(pcmWriteCnt),
    QOSM_MAKE_ADDR(pcmWriteMaxInterval),
    QOSM_MAKE_ADDR(pcmWriteAvgInterval),
    QOSM_MAKE_ADDR(pcmWriteGtFtCnt),
    QOSM_MAKE_ADDR(encoderMemAllocFailedCnt),
    QOSM_MAKE_ADDR(encoderFailedCnt),
    QOSM_MAKE_ADDR(leftConnHandle),
    QOSM_MAKE_ADDR(leftTxPktCnt),
    QOSM_MAKE_ADDR(leftTxNoLinkDropCnt),
    QOSM_MAKE_ADDR(leftTxFlushCnt),
    QOSM_MAKE_ADDR(leftTxUartFailedCnt),
    QOSM_MAKE_ADDR(leftTxFlowCtrlCnt),
    QOSM_MAKE_ADDR(leftTxFlowCtrlMaxPktCnt),
    QOSM_MAKE_ADDR(rightConnHandle),
    QOSM_MAKE_ADDR(rightTxPktCnt),
    QOSM_MAKE_ADDR(rightTxNoLinkDropCnt),
    QOSM_MAKE_ADDR(rightTxFlushCnt),
    QOSM_MAKE_ADDR(rightTxUartFailedCnt),
    QOSM_MAKE_ADDR(rightTxFlowCtrlCnt),
    QOSM_MAKE_ADDR(rightTxFlowCtrlMaxPktCnt),
    QOSM_MAKE_ADDR(intrEventEnqueueFailedCnt),
    QOSM_MAKE_ADDR(intrDataEnqueueFailedCnt),
    QOSM_MAKE_ADDR(dataInvalidFormatCnt),
    QOSM_MAKE_ADDR(badPktCnt),
    QOSM_MAKE_ADDR(invalidConnHandleCnt),
    QOSM_MAKE_ADDR(leftRxConnHandle),
    QOSM_MAKE_ADDR(leftDataMemAllocFailedCnt),
    QOSM_MAKE_ADDR(leftOldDataDropCnt),
    QOSM_MAKE_ADDR(leftDecoderFailedCnt),
    QOSM_MAKE_ADDR(leftPcmSizeNotMatchCnt),
    QOSM_MAKE_ADDR(leftDecodedPcmDropCnt),
    QOSM_MAKE_ADDR(leftReadPcmLossCnt),
    QOSM_MAKE_ADDR(leftReadPcmMisorderCnt),
    QOSM_MAKE_ADDR(rightRxConnHandle),
    QOSM_MAKE_ADDR(rightDataMemAllocFailedCnt),
    QOSM_MAKE_ADDR(rightOldDataDropCnt),
    QOSM_MAKE_ADDR(rightDecoderFailedCnt),
    QOSM_MAKE_ADDR(rightPcmSizeNotMatchCnt),
    QOSM_MAKE_ADDR(rightDecodedPcmDropCnt),
    QOSM_MAKE_ADDR(rightReadPcmLossCnt),
    QOSM_MAKE_ADDR(rightReadPcmMisorderCnt),
    QOSM_MAKE_ADDR(leftRxFrameCnt),
    QOSM_MAKE_ADDR(rightRxFrameCnt),
    QOSM_MAKE_ADDR(encoderLastSn),
    QOSM_MAKE_ADDR(decoderLastSn),
};

static void QOSM_AudioDfxSetUTCTime(char *startTime, size_t startTimeStrLen)
{
    time_t t = time(NULL);
    struct tm tmInfo;
    struct tm *tmTmp = localtime_r(&t, &tmInfo);
    if (tmTmp == NULL) {
        QOSM_LOGE("get localtime failed");
        return;
    }

    if (strftime(startTime, startTimeStrLen, "%y-%m-%d %H:%M:%S", tmTmp) == 0) {
        startTime[0] = 0;
        QOSM_LOGE("format time failed");
        return;
    }
}

static uint32_t QOSM_AudioDfxReadCommon(const uint8_t *data, uint8_t len)
{
    if (len == sizeof(uint8_t)) {
        return (uint32_t)data[0];
    }

    if (len == sizeof(uint16_t)) {
        uint16_t a = 0;
        STREAM_TO_UINT16(a, data);
        return (uint32_t)a;
    }

    if (len == sizeof(uint32_t)) {
        uint32_t a = 0;
        STREAM_TO_UINT32(a, data);
        return a;
    }

    return 0;
}

// 获取编码参数统计
static void QOSM_AudioDfxGetCodecParam(QOSM_DftAudioStats *stats, const uint8_t *buf, uint32_t len)
{
    if (len < QOSM_AUDIO_DFX_CODEC_EXCEPTION_LEN) {
        QOSM_LOGE("inavlid codec param len: %u", len);
        return;
    }

    uint32_t offset = 0;
    stats->l2hcVersion = buf[offset++];
    stats->sampleRate = buf[offset++];
    stats->bitDepth = buf[offset++];
    stats->channelMode = buf[offset++];
    stats->deviceNumInCooperationSet = buf[offset++];
    QOSM_LOGI("version: %u, sample rate: %u, bits per sample: %u, channel mode: %u, tws: %u",
        stats->l2hcVersion, stats->sampleRate, stats->bitDepth, stats->channelMode, stats->deviceNumInCooperationSet);
}

static void QOSM_AudioDfxIncEncoderStats(QOSM_DftAudioStats *targetStats, QOSM_DftAudioStats *stats)
{
    // 编码器统计
    targetStats->encoderMemAllocFailedCnt += stats->encoderMemAllocFailedCnt;
    targetStats->encoderFailedCnt += stats->encoderFailedCnt;

    // 左耳发送统计
    targetStats->leftTxPktCnt += stats->leftTxPktCnt;
    targetStats->leftTxNoLinkDropCnt += stats->leftTxNoLinkDropCnt;
    targetStats->leftTxFlushCnt += stats->leftTxFlushCnt;
    targetStats->leftTxUartFailedCnt += stats->leftTxUartFailedCnt;
    targetStats->leftTxFlowCtrlCnt += stats->leftTxFlowCtrlCnt;
    if (stats->leftTxFlowCtrlMaxPktCnt > targetStats->leftTxFlowCtrlMaxPktCnt) {
        targetStats->leftTxFlowCtrlMaxPktCnt = stats->leftTxFlowCtrlMaxPktCnt;
    }

    // 右耳发送统计
    targetStats->rightTxPktCnt += stats->rightTxPktCnt;
    targetStats->rightTxNoLinkDropCnt += stats->rightTxNoLinkDropCnt;
    targetStats->rightTxFlushCnt += stats->rightTxFlushCnt;
    targetStats->rightTxUartFailedCnt += stats->rightTxUartFailedCnt;
    targetStats->rightTxFlowCtrlCnt += stats->rightTxFlowCtrlCnt;
    if (stats->rightTxFlowCtrlMaxPktCnt > targetStats->rightTxFlowCtrlMaxPktCnt) {
        targetStats->rightTxFlowCtrlMaxPktCnt = stats->rightTxFlowCtrlMaxPktCnt;
    }
}

static void QOSM_AudioDfxUpdateCodecInfo(QOSM_DftAudioStats *targetStats, QOSM_DftAudioStats *stats)
{
    targetStats->l2hcVersion = stats->l2hcVersion;
    targetStats->codecType = g_qosmAudioDfx.codecType;
    targetStats->sampleRate = stats->sampleRate;
    targetStats->bitDepth = stats->bitDepth;
    targetStats->channelMode = stats->channelMode;
    targetStats->deviceNumInCooperationSet = stats->deviceNumInCooperationSet;
}

static void QOSM_AudioDfxIncStats(QOSM_DftAudioStats* stats)
{
    g_qosmAudioDfx.reportCnt++;
    QOSM_DftAudioStats *targetStats = &g_qosmAudioDfx.stats;

    // 累加基本统计项
    targetStats->duration += stats->duration;
    QOSM_AudioDfxUpdateCodecInfo(targetStats, stats);

    // PCM 相关统计
    targetStats->pcmWriteCnt += stats->pcmWriteCnt;
    if (targetStats->pcmWriteMaxInterval < stats->pcmWriteMaxInterval) {
        targetStats->pcmWriteMaxInterval = stats->pcmWriteMaxInterval;
    }
    targetStats->pcmWriteAvgInterval = stats->pcmWriteAvgInterval;
    targetStats->pcmWriteGtFtCnt += stats->pcmWriteGtFtCnt;

    QOSM_AudioDfxIncEncoderStats(targetStats, stats);

    // 中断与数据处理统计
    targetStats->intrEventEnqueueFailedCnt += stats->intrEventEnqueueFailedCnt;
    targetStats->intrDataEnqueueFailedCnt += stats->intrDataEnqueueFailedCnt;
    targetStats->dataInvalidFormatCnt += stats->dataInvalidFormatCnt;
    targetStats->badPktCnt += stats->badPktCnt;
    targetStats->invalidConnHandleCnt += stats->invalidConnHandleCnt;

    // 左耳接收统计
    targetStats->leftDataMemAllocFailedCnt += stats->leftDataMemAllocFailedCnt;
    targetStats->leftOldDataDropCnt += stats->leftOldDataDropCnt;
    targetStats->leftDecoderFailedCnt += stats->leftDecoderFailedCnt;
    targetStats->leftPcmSizeNotMatchCnt += stats->leftPcmSizeNotMatchCnt;
    targetStats->leftDecodedPcmDropCnt += stats->leftDecodedPcmDropCnt;
    targetStats->leftReadPcmLossCnt += stats->leftReadPcmLossCnt;
    targetStats->leftReadPcmMisorderCnt += stats->leftReadPcmMisorderCnt;

    // 右耳接收统计
    targetStats->rightDataMemAllocFailedCnt += stats->rightDataMemAllocFailedCnt;
    targetStats->rightOldDataDropCnt += stats->rightOldDataDropCnt;
    targetStats->rightDecoderFailedCnt += stats->rightDecoderFailedCnt;
    targetStats->rightPcmSizeNotMatchCnt += stats->rightPcmSizeNotMatchCnt;
    targetStats->rightDecodedPcmDropCnt += stats->rightDecodedPcmDropCnt;
    targetStats->rightReadPcmLossCnt += stats->rightReadPcmLossCnt;
    targetStats->rightReadPcmMisorderCnt += stats->rightReadPcmMisorderCnt;

    QOSM_LOGI("report cnt: %u, cur duration: %u, total duration: %u",
        g_qosmAudioDfx.reportCnt, stats->duration, g_qosmAudioDfx.stats.duration);
}

static void QOSM_AudioDfxParseDspStats(QOSM_DftAudioStats *stats, const unsigned char *buf, uint32_t length)
{
    uint32_t offset = 0;
    while (offset + TLV_HEADER_LEN <= length) {
        uint8_t type = buf[offset++];
        uint8_t len = buf[offset++];
        QOSM_LOGD("stats type: %hhu, len: %hhu, offset: %u, total len: %u", type, len, offset, length);

        if (offset + len > length) {
            QOSM_LOGE("invalid stats type: %hhu, len: %hhu, offset: %u, total len: %u", type, len, offset, length);
            break;
        }

        if (type == DSP_NL_DFT_STATS_CODEC_INFO) {
            QOSM_AudioDfxGetCodecParam(stats, &buf[offset], len);
        } else if (type < DSP_NL_DFT_STATS_PARAM_BUTT &&
            (len == sizeof(uint8_t) || len == sizeof(uint16_t) || len == sizeof(uint32_t))) {
            if (g_qosmParamArray[type].param != NULL) {
                *g_qosmParamArray[type].param = QOSM_AudioDfxReadCommon(&buf[offset], len);
                QOSM_LOGI("get %s=%u", g_qosmParamArray[type].paramStr, *g_qosmParamArray[type].param);
            } else {
                QOSM_LOGI("unsupported type: %hhu", type);
            }
        } else {
            QOSM_LOGI("unsupported or len, type: %hhu, len: %hhu", type, len);
        }

        offset += len;
    }

    QOSM_AudioDfxIncStats(stats);

    g_qosmAudioDfx.dspStarted = false;
    g_qosmAudioDfx.dspStoppedTs = QOSM_GetCurrTimeMs();
    QOSM_LOGI("dsp stopped at %u", (uint32_t)g_qosmAudioDfx.dspStoppedTs);

    if (g_qosmAudioDfxDspStatusCb != NULL) {
        g_qosmAudioDfxDspStatusCb(false);
    }
}

static void QOSM_AudioDfxParseCodecException(uint8_t *buf, uint32_t length)
{
    if (length < sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint32_t)) {
        QOSM_LOGE("invalid codec excep len %u", length);
        return;
    }

    QOSM_DftAudioCodecExcep codecExcep = {};
    codecExcep.codecType = (int)buf[0];
    codecExcep.codecAlgo = (int)buf[1];
    uint32_t errorCode = 0;
    uint8_t *p = &buf[sizeof(uint8_t) + sizeof(uint8_t)];
    STREAM_TO_UINT32(errorCode, p);
    codecExcep.errorCode = (int)errorCode;
    QOSM_AudioDfxSetUTCTime(codecExcep.time, sizeof(codecExcep.time));
    QOSM_LOGI("codec type: %d, mode: %d, error code: %d",
        codecExcep.codecType, codecExcep.codecAlgo, codecExcep.errorCode);
    DftReportAudioCodecExcep(&codecExcep);
}

static void QOSM_AudioDfxParseChoppyEvent(const uint8_t *buf, uint32_t len)
{
    if (len < sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t)) {
        QOSM_LOGE("invalid choppy len %u", len);
        return;
    }
    uint8_t *p = (uint8_t *)buf;
    uint16_t conn_handle = 0;
    STREAM_TO_UINT16(conn_handle, p);
    QOSM_LOGI("conn handle 0x%04x choppied, choppy type: %hhu, rsv: %hhu", conn_handle, buf[2], buf[3]);
}

static void QOSM_AudioDfxParseBitrateReport(const uint8_t *buf, uint32_t len)
{
    if (len < sizeof(uint32_t) + sizeof(uint8_t)) {
        QOSM_LOGE("invalid len %u", len);
        return;
    }
    uint32_t bitrateChangeResult = 0;
    uint8_t setLabelResult = 0;
    uint8_t *p = (uint8_t *)buf;
    STREAM_TO_UINT32(bitrateChangeResult, p);
    setLabelResult = buf[sizeof(uint32_t)];
    QOSM_LOGI("bitrate change result %u, set label result status %u", bitrateChangeResult, setLabelResult);
    int error = bitrateChangeResult == 0 ? (int)setLabelResult : (int)bitrateChangeResult;
    for (uint32_t i = 0; i < g_qosmAudioDfx.bitrateChangeFailCnt; i++) {
        if (g_qosmAudioDfx.bitrateChangeFail[i].error == error) {
            g_qosmAudioDfx.bitrateChangeFail[i].cnt++;
            QOSM_LOGI("bitrate change failed error: %d, cnt: %u", error, g_qosmAudioDfx.bitrateChangeFail[i].cnt);
            return;
        }
    }

    if (g_qosmAudioDfx.bitrateChangeFailCnt >= QOSM_BITRATE_CHANGE_ERROR_MAX_CNT) {
        QOSM_LOGI("bitrate changed failed reason is full");
        return;
    }

    QOSM_LOGI("add bitrate change failed error %d", error);
    g_qosmAudioDfx.bitrateChangeFail[g_qosmAudioDfx.bitrateChangeFailCnt].error = error;
    g_qosmAudioDfx.bitrateChangeFail[g_qosmAudioDfx.bitrateChangeFailCnt].cnt = 1;
    g_qosmAudioDfx.bitrateChangeFailCnt++;
}

static void QOSM_AudioDfxNotifyDspStarted()
{
    uint64_t cur = QOSM_GetCurrTimeMs();
    QOSM_LOGI("dsp stopped at %u, started at %u, diff: %u",
        (uint32_t)g_qosmAudioDfx.dspStoppedTs, (uint32_t)cur, (uint32_t)(cur - g_qosmAudioDfx.dspStoppedTs));
    g_qosmAudioDfx.dspStarted = true;

    if (g_qosmAudioDfxDspStatusCb != NULL) {
        g_qosmAudioDfxDspStatusCb(true);
    }
}

static void QOSM_AudioDfxParseCodecStartStatus(const uint8_t *buf, uint32_t len)
{
    if (len < sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t)) {
        QOSM_LOGE("invalid len %u", len);
        return;
    }

    const uint8_t *p = buf;
    uint8_t codecType = *p++;
    uint8_t status = *p++;
    uint16_t sn = 0;
    STREAM_TO_UINT16(sn, p);

    if (codecType >= QOSM_AUDIO_DFX_CODEC_MAX || status >= QOSM_AUDIO_DFX_CODEC_STATUS_MAX) {
        QOSM_LOGE("invalid codec type: %u, status %hhu, sn: %u", codecType, status, sn);
        return;
    }

    const char *str[QOSM_AUDIO_DFX_CODEC_STATUS_MAX] = {
        "stopped", "started", "tx ready", "rx first pkt"
    };
    QOSM_LOGI("dsp %s status %s(%hhu), sn: %u", codecType == QOSM_AUDIO_DFX_CODEC_ENCODER ? "encoder" : "decoder",
        str[status], status, sn);

    if (codecType == QOSM_AUDIO_DFX_CODEC_ENCODER && status != QOSM_AUDIO_DFX_CODEC_STATUS_STOP) {
        QOSM_AudioDfxNotifyDspStarted();
    }
}

static void QOSM_AudioDfxParseFlowControl(const uint8_t *buf, uint32_t len)
{
    if (len < sizeof(uint8_t) + sizeof(uint16_t)) {
        QOSM_LOGE("invalid len %u", len);
        return;
    }

    uint8_t *p = (uint8_t *)buf;
    bool enter = *p++;
    uint16_t connHandle = 0;
    STREAM_TO_UINT16(connHandle, p);
    QOSM_LOGI("conn handle %hu %s flow control", connHandle, enter ? "enter" : "exit");
    if (g_qosmAudioDfxFlowCtrlCb != NULL) {
        g_qosmAudioDfxFlowCtrlCb(connHandle, enter);
    }
}

static void QOSM_AudioDfxParseRxLost(const uint8_t *buf, uint32_t len)
{
    if (len < sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t)) {
        QOSM_LOGE("invalid len: %u", len);
        return;
    }

    const uint8_t *p = buf;
    uint16_t lastSn = 0;
    uint16_t curSn = 0;
    uint16_t lostCnt = 0;
    STREAM_TO_UINT16(lastSn, p);
    STREAM_TO_UINT16(curSn, p);
    STREAM_TO_UINT16(lostCnt, p);
    QOSM_LOGI("rx pkt lost, last sn: %hu, cur sn: %hu, lost cnt: %hu", lastSn, curSn, lostCnt);
}

static void QOSM_AudioDfxParseRxNoPkt(const uint8_t *buf, uint32_t len)
{
    QOSM_LOGI("no pkt recv in last 5s");
}

static void QOSM_AudioDfxParseQualityReport(const uint8_t *buf, uint32_t len)
{
    if (len != sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(int8_t)) {
        QOSM_LOGE("invalid len: %u", len);
        return;
    }

    const uint8_t *p = buf;
    CM_ICBQuality quality = {0};
    STREAM_TO_UINT16(quality.connHandle, p);
    STREAM_TO_UINT16(quality.txFlushed, p);
    STREAM_TO_UINT8(quality.ackRate, p);
    p++;
    STREAM_TO_INT8(quality.rssi, p);
    QOSM_PrintQualityReportParam(&quality);

    struct QOSM_AudioDfxChoppyInfo info = { quality.connHandle, quality.txFlushed, quality.ackRate, quality.rssi };
    QOSM_AudioDfxNotifyChoppy(&info);
}

static void QOSM_AudioDfxParseBitrateChangeDecision(const uint8_t *buf, uint32_t len)
{
    if (len != sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t)) {
        QOSM_LOGE("invalid len: %u", len);
        return;
    }

    const uint8_t *p = buf;
    uint16_t connHandle;
    uint16_t qosIndex;
    uint16_t reportedDirection;
    uint16_t reportedQosLevel;
    STREAM_TO_UINT16(connHandle, p);
    STREAM_TO_UINT16(qosIndex, p);
    STREAM_TO_UINT16(reportedDirection, p);
    STREAM_TO_UINT16(reportedQosLevel, p);
    QOSM_ExecuteBitrateChangeDecision(connHandle, qosIndex, reportedDirection, reportedQosLevel);
}

static void QOSM_AudioDfxParseDspUevent(uint8_t dftEventType, unsigned char *ueventValue,
    uint32_t ueventLen)
{
    if (dftEventType == DFT_EVENT_TYPE_STATS) {
        (void)memset_s(&g_qosmAudioStats, sizeof(g_qosmAudioStats), 0, sizeof(g_qosmAudioStats));
        QOSM_AudioDfxParseDspStats(&g_qosmAudioStats, ueventValue, ueventLen);
    } else if (dftEventType == DFT_EVENT_TYPE_CODEC_EXCEP) {
        QOSM_AudioDfxParseCodecException(ueventValue, ueventLen);
    } else if (dftEventType == DFT_EVENT_TYPE_CHOPPY_EXCEP) {
        QOSM_AudioDfxParseChoppyEvent(ueventValue, ueventLen);
    } else if (dftEventType == DFT_EVENT_TYPE_BITRATE_REPORT) {
        QOSM_AudioDfxParseBitrateReport(ueventValue, ueventLen);
    } else if (dftEventType == DFT_EVENT_TYPE_CODEC_START_STATUS) {
        QOSM_AudioDfxParseCodecStartStatus(ueventValue, ueventLen);
    } else if (dftEventType == DFT_EVENT_TYPE_FLOW_CONTROL) {
        QOSM_AudioDfxParseFlowControl(ueventValue, ueventLen);
    } else if (dftEventType == DFT_EVENT_TYPE_RX_LOST) {
        QOSM_AudioDfxParseRxLost(ueventValue, ueventLen);
    } else if (dftEventType == DFT_EVENT_TYPE_RX_NO_PKT) {
        QOSM_AudioDfxParseRxNoPkt(ueventValue, ueventLen);
    } else if (dftEventType == DFT_EVENT_TYPE_QUALITY_REPORT) {
        QOSM_AudioDfxParseQualityReport(ueventValue, ueventLen);
    } else if (dftEventType == DFT_EVENT_TYPE_BITRATE_CHANGE_DECISION) {
        QOSM_AudioDfxParseBitrateChangeDecision(ueventValue, ueventLen);
    }
}

static void QOSM_AudioDfxProcessDspData(const char *dftInfo)
{
    uint32_t inputLength = (uint32_t)strlen(dftInfo);
    if (!QOSM_BASE64_LEN_IS_VALID(inputLength)) {
        QOSM_LOGE("invalid str len %u", inputLength);
        return;
    }

    uint32_t decodeBufLen = QOSM_BASE64_LEN_TO_BIN_LEN(inputLength);
    uint8_t *decodeBuf = SDF_MemZalloc(decodeBufLen);
    if (decodeBuf == NULL) {
        QOSM_LOGE("alloc failed");
        return;
    }

    uint32_t outputLength = QOSM_Base64Decode(dftInfo, inputLength, decodeBuf, decodeBufLen);
    if (outputLength < QOSM_AUDIO_DFX_DSP_STATS_HDR_LEN) {
        QOSM_LOGE("base64 decode failed");
        SDF_MemFree(decodeBuf);
        return;
    }

    uint8_t type = QOSM_AUDIO_DFX_GET_TYPE(decodeBuf[QOSM_AUDIO_DFX_DSP_STATS_TYPE_FLAG_INDEX]);
    uint8_t flag = QOSM_AUDIO_DFX_GET_FLAG(decodeBuf[QOSM_AUDIO_DFX_DSP_STATS_TYPE_FLAG_INDEX]);
    uint8_t len = decodeBuf[QOSM_AUDIO_DFX_DSP_STATS_LEN_INDEX];
    uint8_t seq = QOSM_AUDIO_DFX_GET_SEQ(decodeBuf[QOSM_AUDIO_DFX_DSP_STATS_SEQ_INDEX]);
    uint8_t subSeq = QOSM_AUDIO_DFX_GET_SUBSEQ(decodeBuf[QOSM_AUDIO_DFX_DSP_STATS_SEQ_INDEX]);
    QOSM_LOGD("type=%u, flag=%u, len=%u, seq=%u, sub seq=%u", type, flag, len, seq, subSeq);
    if (len > outputLength - QOSM_AUDIO_DFX_DSP_STATS_HDR_LEN) {
        QOSM_LOGE("invalid len %u, output len: %u", len, outputLength);
    } else {
        QOSM_AudioDfxParseDspUevent(type, &decodeBuf[QOSM_AUDIO_DFX_DSP_STATS_VALUE_INDEX], len);
    }

    SDF_MemFree(decodeBuf);
}

void QOSM_AudioDfxStart(struct QOSM_AudioDfxInfo *info)
{
    (void)memset_s(&g_qosmAudioDfx, sizeof(g_qosmAudioDfx), 0, sizeof(g_qosmAudioDfx));
    g_qosmAudioDfx.startTs = QOSM_GetCurrTimeMs();
    for (uint32_t i = 0; i < QOSM_AUDIO_DFX_CONN_CNT; i++) {
        g_qosmAudioDfx.conn[i].connHandle = QOSM_INVALID_CONN_HANDLE;
    }

    g_qosmAudioDfx.curBand = QOSM_AUDIO_DFX_BAND_2G;
    g_qosmAudioDfx.curBandStartTs = QOSM_GetCurrTimeMs();
    g_qosmAudioDfx.powerLevelIndex = QOSM_POWER_LEVEL_CNT;

    g_qosmAudioDfx.stats.sduInterval = info->param.sduInterval / QOSM_US_TO_MS;
    g_qosmAudioDfx.stats.flushTimeout = info->param.ft;
    g_qosmAudioDfx.stats.burstNum = info->param.bn;
    g_qosmAudioDfx.dspStarted = false;
    g_qosmAudioDfx.dspStoppedTs = QOSM_GetCurrTimeMs();
    g_qosmAudioDfx.reportCntInDspStopped = 0;
    g_qosmAudioDfx.firstStarted = true;
    g_qosmAudioDfx.codecType = info->codecType;
    QOSM_AudioDfxUpdateBitrate(info->startBitrate);
    QOSM_AudioDfxSetUTCTime(g_qosmAudioDfx.stats.startTime, sizeof(g_qosmAudioDfx.stats.startTime));
    QOSM_LOGI("[%s] start at %" PRIu64 ", start bitrate: %u",
        g_qosmAudioDfx.stats.startTime, g_qosmAudioDfx.startTs, g_qosmAudioDfx.curBitrate);
    QOSM_UeventInit(QOSM_AudioDfxProcessDspData);
    g_qosmAudioDfxDspStatusCb = info->dspStatusCb;
    g_qosmAudioDfxFlowCtrlCb = info->flowCtrlCb;
    QOSM_AntennaDfxSendQueryCmd();
}

void QOSM_AudioDfxUpdateConn(struct QOSM_AudioDfxConn *conn, bool connected)
{
    for (uint32_t i = 0; i < QOSM_AUDIO_DFX_CONN_CNT; i++) {
        if (connected && g_qosmAudioDfx.conn[i].connHandle == QOSM_INVALID_CONN_HANDLE) {
            (void)memcpy_s(&g_qosmAudioDfx.conn[i], sizeof(struct QOSM_AudioDfxConn), conn,
                sizeof(struct QOSM_AudioDfxConn));
            QOSM_LOGI("[%u] conn handle %hu connected", i, conn->connHandle);
            return;
        }

        if (!connected && g_qosmAudioDfx.conn[i].connHandle == conn->connHandle) {
            (void)memset_s(&g_qosmAudioDfx.conn[i], sizeof(struct QOSM_AudioDfxConn), 0,
                sizeof(struct QOSM_AudioDfxConn));
            g_qosmAudioDfx.conn[i].connHandle = QOSM_INVALID_CONN_HANDLE;
            QOSM_LOGI("[%u] conn handle %hu disconnected", i, conn->connHandle);
            return;
        }
    }
}

static void QOSM_AudioDfxBandFormat(void)
{
    QOSM_LOGI("band change cnt: %u", g_qosmAudioDfx.stats.bandChangeCnt);

    for (uint32_t i = 0; i < QOSM_AUDIO_DFX_BAND_MAX; i++) {
        for (uint32_t j = 0; j < QOSM_POWER_LEVEL_CNT; j++) {
            QOSM_LOGI("band: %u, power level: %u, duration: %" PRIu64,
                i, j + QOSM_POWER_LEVEL_MIN, g_qosmAudioDfx.bandContinousTime[i][j]);
        }

        if (i == QOSM_AUDIO_DFX_BAND_5G_1) {
            continue;
        }

        char *buf = (i == QOSM_AUDIO_DFX_BAND_2G) ? g_qosmAudioDfx.stats.band2GInfo : g_qosmAudioDfx.stats.band5GInfo;
        uint32_t bufLen = DFX_STR_LEN;
        uint64_t timeInLevel5 = g_qosmAudioDfx.bandContinousTime[i][QOSM_POWER_LEVEL_5 - QOSM_POWER_LEVEL_MIN];
        uint64_t timeInLevel6 = g_qosmAudioDfx.bandContinousTime[i][QOSM_POWER_LEVEL_6 - QOSM_POWER_LEVEL_MIN];
        uint64_t timeInLevel7 = g_qosmAudioDfx.bandContinousTime[i][QOSM_POWER_LEVEL_7 - QOSM_POWER_LEVEL_MIN];
        uint64_t timeInLevel8 = g_qosmAudioDfx.bandContinousTime[i][QOSM_POWER_LEVEL_8 - QOSM_POWER_LEVEL_MIN];
        uint64_t totalTime = timeInLevel5 + timeInLevel6 + timeInLevel7 + timeInLevel8;
        int ret = sprintf_s(buf, bufLen, "{%u.%u,%u.%u,%u.%u,%u.%u,%u.%u}",
            timeInLevel5 / QOSM_MS_TO_SEC, timeInLevel5 % QOSM_MS_TO_SEC,
            timeInLevel6 / QOSM_MS_TO_SEC, timeInLevel6 % QOSM_MS_TO_SEC,
            timeInLevel7 / QOSM_MS_TO_SEC, timeInLevel7 % QOSM_MS_TO_SEC,
            timeInLevel8 / QOSM_MS_TO_SEC, timeInLevel8 % QOSM_MS_TO_SEC,
            totalTime    / QOSM_MS_TO_SEC, totalTime    % QOSM_MS_TO_SEC);
        if (ret <= 0) {
            QOSM_LOGE("format failed failed");
        } else {
            QOSM_LOGD("format string: %s", buf);
        }
    }
}

static void QOSM_AudioDfxBitrateChangeFailedFormat(void)
{
    QOSM_LOGI("bitrate change failed cnt: %u", g_qosmAudioDfx.bitrateChangeFailCnt);

    char *buf = g_qosmAudioDfx.stats.bitrateDownFailedReason;
    uint32_t bufLen = DFX_STR_LEN;
    uint32_t offset = 0;
    buf[0] = '\0';
    for (uint32_t i = 0; i < g_qosmAudioDfx.bitrateChangeFailCnt && offset < bufLen - 1; i++) {
        if (i != 0) {
            buf[offset++] = ':';
        }
        int ret = sprintf_s(buf + offset, bufLen - offset, "%d:%u",
            g_qosmAudioDfx.bitrateChangeFail[i].error, g_qosmAudioDfx.bitrateChangeFail[i].cnt);
        if (ret < 0) {
            QOSM_LOGE("format failed");
            buf[0] = '\0';
            return;
        }
        offset += (uint32_t)ret;
    }

    QOSM_LOGD("bitrate change failed format str: %s", buf);
}

void QOSM_AudioDfxStop(void)
{
    uint64_t cur = QOSM_GetCurrTimeMs();
    uint64_t duration = cur - g_qosmAudioDfx.startTs;
    uint64_t bitrateDuration = cur - g_qosmAudioDfx.curBitrateStartTs;
    if (g_qosmAudioDfx.curBitrateIdx < QOSM_BITRATE_MAX) {
        g_qosmAudioDfx.bitrateContinousTime[g_qosmAudioDfx.curBitrateIdx] += bitrateDuration;
    }

    uint64_t bandDuration = cur - g_qosmAudioDfx.curBandStartTs;
    if (g_qosmAudioDfx.curBand < QOSM_AUDIO_DFX_BAND_MAX && g_qosmAudioDfx.powerLevelIndex < QOSM_POWER_LEVEL_CNT) {
        g_qosmAudioDfx.bandContinousTime[g_qosmAudioDfx.curBand][g_qosmAudioDfx.powerLevelIndex] += bandDuration;
    }

    QOSM_LOGI("stop at %" PRIu64 ", total duration: %" PRIu64 ", dsp duration: %u, dsp report cnt: %u,"
        " bitrate up cnt: %u, bitrate down cnt: %u, band change cnt: %u",
        cur, duration, g_qosmAudioDfx.stats.duration, g_qosmAudioDfx.reportCnt,
        g_qosmAudioDfx.stats.bitrateUpgradeCnt, g_qosmAudioDfx.stats.bitrateDownCnt,
        g_qosmAudioDfx.stats.bandChangeCnt);

    for (uint32_t i = 0; i < QOSM_BITRATE_MAX; i++) {
        if (g_qosmAudioDfx.bitrateContinousTime[i] == 0) {
            continue;
        }
        QOSM_LOGI("bitrate: %u, duration: %" PRIu64,
            g_qosmAudioDfxBitrateArray[i], g_qosmAudioDfx.bitrateContinousTime[i]);
        g_qosmAudioDfx.stats.maxDownBitrate = g_qosmAudioDfxBitrateArray[i];
        g_qosmAudioDfx.stats.maxDownBitrateDuration = g_qosmAudioDfx.bitrateContinousTime[i];
    }

    QOSM_AudioDfxBandFormat();
    QOSM_AudioDfxBitrateChangeFailedFormat();

    DftReportAudioStats(&g_qosmAudioDfx.stats);
    QOSM_UeventDeinit();
}

static uint32_t QOSM_AudioDfxGetBitrateIdx(uint32_t bitrate)
{
    for (uint32_t i = 0; i < QOSM_BITRATE_MAX; i++) {
        if (g_qosmAudioDfxBitrateArray[i] == bitrate) {
            return i;
        }
    }

    return QOSM_BITRATE_MAX;
}

void QOSM_AudioDfxUpdateBitrate(uint32_t bitrate)
{
    if (g_qosmAudioDfx.curBitrate == bitrate) {
        QOSM_LOGD("bitrate not changed");
        return;
    }

    uint32_t nextBitrateIdx = QOSM_AudioDfxGetBitrateIdx(bitrate);
    if (nextBitrateIdx >= QOSM_BITRATE_MAX) {
        QOSM_LOGD("invalid bitrate %u", bitrate);
        return;
    }

    uint64_t cur = QOSM_GetCurrTimeMs();
    if (g_qosmAudioDfx.curBitrateStartTs == 0) {
        g_qosmAudioDfx.curBitrateStartTs = cur;
        g_qosmAudioDfx.curBitrate = bitrate;
        g_qosmAudioDfx.curBitrateIdx = nextBitrateIdx;
        QOSM_LOGD("first update bitrate to %u", bitrate);
        return;
    }

    if (g_qosmAudioDfx.curBitrate < bitrate) {
        g_qosmAudioDfx.stats.bitrateUpgradeCnt++;
    } else {
        g_qosmAudioDfx.stats.bitrateDownCnt++;
    }

    uint64_t duration = cur - g_qosmAudioDfx.curBitrateStartTs;
    if (g_qosmAudioDfx.curBitrateIdx < QOSM_BITRATE_MAX) {
        g_qosmAudioDfx.bitrateContinousTime[g_qosmAudioDfx.curBitrateIdx] += duration;
        QOSM_LOGI("old bitrate %uKbps duration: %" PRIu64 ", total duration: %" PRIu64,
            g_qosmAudioDfx.curBitrate, duration, g_qosmAudioDfx.bitrateContinousTime[g_qosmAudioDfx.curBitrateIdx]);
        g_qosmAudioDfx.curBitrateStartTs = cur;
        g_qosmAudioDfx.curBitrate = bitrate;
        g_qosmAudioDfx.curBitrateIdx = nextBitrateIdx;
    }
}

static void QOSM_AudioDfxUpdateBandTime(void)
{
    if (g_qosmAudioDfx.powerLevelIndex >= QOSM_POWER_LEVEL_CNT) {
        return;
    }
    uint64_t cur = QOSM_GetCurrTimeMs();
    uint64_t duration = cur - g_qosmAudioDfx.curBandStartTs;
    g_qosmAudioDfx.bandContinousTime[g_qosmAudioDfx.curBand][g_qosmAudioDfx.powerLevelIndex] += duration;
    g_qosmAudioDfx.curBandStartTs = cur;
    QOSM_LOGI("band %u, power level %hhu, duration: %" PRIu64 ", total duration: %" PRIu64,
        g_qosmAudioDfx.curBand, g_qosmAudioDfx.powerLevelIndex + QOSM_POWER_LEVEL_MIN, duration,
        g_qosmAudioDfx.bandContinousTime[g_qosmAudioDfx.curBand][g_qosmAudioDfx.powerLevelIndex]);
}

void QOSM_AudioDfxUpdateBand(uint32_t band)
{
    if (band >= QOSM_AUDIO_DFX_BAND_MAX || g_qosmAudioDfx.curBand == band) {
        return;
    }

    QOSM_AudioDfxUpdateBandTime();
    g_qosmAudioDfx.curBand = band;
    g_qosmAudioDfx.stats.bandChangeCnt++;
}

void QOSM_AudioDfxNotifyChoppy(struct QOSM_AudioDfxChoppyInfo *info)
{
    uint64_t cur = QOSM_GetCurrTimeMs();
    if (g_qosmAudioDfx.choppyReportedTs != 0 &&
        (cur - g_qosmAudioDfx.choppyReportedTs) < QOSM_CHOPPY_REPORT_INTERVAL_MS) {
        QOSM_LOGD("do not report too frequently");
        return;
    }

    g_qosmAudioDfx.choppyReportedTs = cur;

    struct QOSM_AudioDfxConn *conn = NULL;
    for (uint32_t i = 0; i < QOSM_AUDIO_DFX_CONN_CNT; i++) {
        if (g_qosmAudioDfx.conn[i].connHandle == info->connHandle) {
            conn = &g_qosmAudioDfx.conn[i];
            break;
        }
    }

    if (conn == NULL) {
        QOSM_LOGE("can not find conn info of conn handle %hu", info->connHandle);
        return;
    }

    QOSM_DftAudioChoppyExcep excep = {};
    QOSM_AudioDfxSetUTCTime(excep.time, sizeof(excep.time));
    excep.choppyCnt = 1;
    if (memcpy_s(excep.deviceAddr, sizeof(excep.deviceAddr), conn->addr, sizeof(conn->addr)) != EOK) {
        return;
    }
    excep.sceneCode = 1;
    excep.subsceneCode = QOMS_CHOPPY_TX_FLUSHED;
    excep.bitrate = g_qosmAudioDfx.curBitrate;
    QOSM_AntennaDfxSendQueryCmd();
    int antennaPolicy = QOSM_AntennaDfxGetAntennaPolicy();
    excep.freqBand = g_qosmAudioDfx.curBand;
    if (antennaPolicy >= 0) {
        excep.freqBand = excep.freqBand | ((uint32_t)antennaPolicy << 8);
    }
    excep.powerInfo = g_qosmAudioDfx.powerLevelIndex + QOSM_POWER_LEVEL_MIN;
    excep.peerRssi = info->rssi;
    excep.txflushNum = info->txFlushed;
    excep.ackRate = info->ackRate;

    DftReportAudioChoppyExcep(&excep);
}

void QOSM_AudioDfxUpdatePowerLevel(uint16_t connHandle, uint8_t level)
{
    if (level < QOSM_POWER_LEVEL_MIN || level > QOSM_POWER_LEVEL_MAX ||
        g_qosmAudioDfx.powerLevelIndex == level - QOSM_POWER_LEVEL_MIN) {
        return;
    }

    uint8_t newLevelIdx = level - QOSM_POWER_LEVEL_MIN;
    if (g_qosmAudioDfx.powerLevelIndex >= QOSM_POWER_LEVEL_CNT) {
        QOSM_LOGI("conn handle 0x%04x first update power level %hhu", connHandle, level);
        g_qosmAudioDfx.powerLevelIndex = newLevelIdx;
        return;
    }

    QOSM_LOGI("conn handle 0x%04x update power level from %hhu to %hhu", connHandle,
        g_qosmAudioDfx.powerLevelIndex + QOSM_POWER_LEVEL_MIN, newLevelIdx + QOSM_POWER_LEVEL_MIN);
    QOSM_AudioDfxUpdateBandTime();
    g_qosmAudioDfx.powerLevelIndex = newLevelIdx;
}

#define QOSM_GET_DSP_STATUS_TIMEOUT_MS 3000
#define QOSM_DSP_STOPPED_EXPIRE_MS 3000

void QOSM_AudioDfxGetDspStatusInner(void *arg)
{
    bool *started = (bool *)arg;

    uint64_t cur = QOSM_GetCurrTimeMs();
    if (!g_qosmAudioDfx.dspStarted || (cur - g_qosmAudioDfx.dspStoppedTs) < QOSM_DSP_STOPPED_EXPIRE_MS) {
        *started = false;
    } else {
        *started = true;
    }
    QOSM_LOGI("ret=%d, dsp started: %u, stopped ts: %u, cur: %u",
        *started, g_qosmAudioDfx.dspStarted, g_qosmAudioDfx.dspStoppedTs, cur);
}

uint32_t QOSM_AudioDfxGetDspStatus(bool *started)
{
    if (started == NULL) {
        QOSM_LOGE("started is null");
        return QOSM_FAIL;
    }
    if (CP_PostTaskBlocked(QOSM_AudioDfxGetDspStatusInner, (void *)started,
        NULL, QOSM_GET_DSP_STATUS_TIMEOUT_MS) != CP_OK) {
        QOSM_LOGE("get dsp status failed");
        return QOSM_POST_TASK_ERR;
    }

    return QOSM_SUCCESS;
}

void QOSM_AudioDfxQualityReport(uint16_t txFlushed, uint16_t ackrate)
{
    if (g_qosmAudioDfx.dspStarted) {
        return;
    }

    if (txFlushed == 0 && ackrate == 0) {
        g_qosmAudioDfx.reportCntInDspStopped = 0;
        return;
    }

    uint32_t reportCnt = g_qosmAudioDfx.firstStarted ?
        QOSM_REPORT_CNT_IN_FIRST_STARTED : QOSM_REPORT_CNT_IN_DSP_OFF_MAX;
    g_qosmAudioDfx.reportCntInDspStopped++;
    if (g_qosmAudioDfx.reportCntInDspStopped >= reportCnt) {
        QOSM_LOGI("report count in dsp off is %u, first started: %d, set dsp on",
            g_qosmAudioDfx.reportCntInDspStopped, g_qosmAudioDfx.firstStarted);
        g_qosmAudioDfx.reportCntInDspStopped = 0;
        g_qosmAudioDfx.firstStarted = false;
        QOSM_AudioDfxNotifyDspStarted();
    }
}