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

#include "qosm_table_mgr.h"
#include <stdatomic.h>
#include "cm_icb_def.h"
#include "dli_layer.h"
#include "parameter_wrapper.h"
#include "qosm_log.h"
#include "qosm_errno.h"
#include "securec.h"

#define QOS_AUDIO_SIZE (sizeof(g_qosAudio) / sizeof(QOSM_LinkParam))
#define QOS_LOW_LATENCY_SIZE (sizeof(g_qosLowLatency) / sizeof(QOSM_LinkParam))
#define QOS_CALL_SIZE (sizeof(g_qosCall) / sizeof(QOSM_LinkParam))
#define QOS_SPATIAL_AUDIO_SIZE (sizeof(g_qosSpatialAudio) / sizeof(QOSM_LinkParam))
#define QOS_HD_RECORDING_SIZE (sizeof(g_qosHdRecording) / sizeof(QOSM_LinkParam))
#define QOS_VOICE_ASSISTANT_SIZE (sizeof(g_qosVoiceAssistant) / sizeof(QOSM_LinkParam))
#define QOS_LONGRANGE_CALL_SIZE (sizeof(g_qosLongRangeCall) / sizeof(QOSM_LinkParam))
#define QOS_OTHERS_SIZE (sizeof(g_qosOthers) / sizeof(QOSM_LinkParam))

#define QOS_BYTE_FRAME_NUM 1 /* DSP帧数占用的字节数 */
#define QOS_MAKE_SDU_LEN(x) ((x) + QOS_BYTE_FRAME_NUM)
#define QOSM_DUTYCYCLE_NOT_FIXED 0
#define QOSM_DUTYCYCLE_FIXED_100P 1
#define QOSM_DUTYCYCLE_FIXED_50P 2

typedef struct {
    QOSM_QosIndex qosIndex;
    uint8_t qosCnt;
    QOSM_LinkParam *qos;
} QOSM_QosIndexTable;

typedef struct {
    QOSM_QosIndex qosIndex;
    QOSM_StartParam startParam;
} QOSM_QosIndexStartParam;

typedef struct {
    QOSM_QosIndex qosIndex;
    QOSM_QosLevel qosLevel;
} QOSM_QosIndexStartLevel;

/*
 * 音频
 */
static QOSM_LinkParam g_qosAudio[] = {
    {
        QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_7 /* qosLevel */, QOS_LEVEL_7 /* upQosLevel */,
        QOS_LEVEL_4 /* downQosLevel */, 4600 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
        43 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(2876) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(2876) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 2 /* phyG2T */, 0 /* phyT2G */,
        16 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_6 /* qosLevel */, QOS_LEVEL_7 /* upQosLevel */,
        QOS_LEVEL_4 /* downQosLevel */, 2300 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
        43 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(1438) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(1438) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 2 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_5 /* qosLevel */, QOS_LEVEL_6 /* upQosLevel */,
        QOS_LEVEL_4 /* downQosLevel */, 1500 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
        57 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(938) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(938) /* maxPduG2T */, 0 /* maxPduT2G */,
        4 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 2 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_4 /* qosLevel */, QOS_LEVEL_5 /* upQosLevel */,
        QOS_LEVEL_3 /* downQosLevel */, 640 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
        57 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(400) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(400) /* maxPduG2T */, 0 /* maxPduT2G */,
        4 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 1 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_3 /* qosLevel */, QOS_LEVEL_4 /* upQosLevel */,
        QOS_LEVEL_1 /* downQosLevel */, 320 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
        57 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(200) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(200) /* maxPduG2T */, 0 /* maxPduT2G */,
        5 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_2 /* qosLevel */, QOS_LEVEL_4 /* upQosLevel */,
        QOS_LEVEL_1 /* downQosLevel */, 320 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
        57 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(200) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(200) /* maxPduG2T */, 0 /* maxPduT2G */,
        4 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 1 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_1 /* qosLevel */, QOS_LEVEL_2 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 192 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
        72 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(120) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(120) /* maxPduG2T */, 0 /* maxPduT2G */,
        5 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_0 /* qosLevel */, QOS_LEVEL_1 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 96 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        300 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 15 /* ftG2T */, 1 /* ftT2G */,
        72 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(60) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(60) /* maxPduG2T */, 0 /* maxPduT2G */,
        6 /* nse */, 2 /* bnG2T */, 0 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
};

/*
 * 游戏低时延
 */
static QOSM_LinkParam g_qosLowLatency[] = {
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_4 /* qosLevel */, QOS_LEVEL_4 /* upQosLevel */,
        QOS_LEVEL_2 /* downQosLevel */, 320 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        30 /* maxLatencyG2T */, 10 /* maxLatencyT2G */, 40 /* icbInterval */, 3 /* ftG2T */, 1 /* ftT2G */,
        8 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(200) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(200) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 0 /* bnT2G */, 2 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_3 /* qosLevel */, QOS_LEVEL_3 /* upQosLevel */,
        QOS_LEVEL_1 /* downQosLevel */, 320 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        30 /* maxLatencyG2T */, 10 /* maxLatencyT2G */, 40 /* icbInterval */, 3 /* ftG2T */, 1 /* ftT2G */,
        8 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(200) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(200) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 0 /* bnT2G */, 1 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_2 /* qosLevel */, QOS_LEVEL_4 /* upQosLevel */,
        QOS_LEVEL_2 /* downQosLevel */, 192 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        30 /* maxLatencyG2T */, 10 /* maxLatencyT2G */, 40 /* icbInterval */, 3 /* ftG2T */, 1 /* ftT2G */,
        8 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(120) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(120) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 0 /* bnT2G */, 1 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_1 /* qosLevel */, QOS_LEVEL_3 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 192 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        30 /* maxLatencyG2T */, 10 /* maxLatencyT2G */, 40 /* icbInterval */, 3 /* ftG2T */, 1 /* ftT2G */,
        8 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(120) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(120) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 0 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_0 /* qosLevel */, QOS_LEVEL_1 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 96 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        30 /* maxLatencyG2T */, 10 /* maxLatencyT2G */, 40 /* icbInterval */, 3 /* ftG2T */, 1 /* ftT2G */,
        11 /* rtnG2T */, 3 /* rtnT2G */, QOS_MAKE_SDU_LEN(60) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(60) /* maxPduG2T */, 0 /* maxPduT2G */,
        4 /* nse */, 1 /* bnG2T */, 0 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        0 /* mcsG2T */, 0 /* mcsT2G */, 0 /* pilotG2T */, 0 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
};

/*
 * 通话
 */
static QOSM_LinkParam g_qosCall[] = {
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_1 /* qosLevel */, QOS_LEVEL_1 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 64 /* downwardBitrate */, 64 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 60 /* maxLatencyT2G */, 80 /* icbInterval */, 3 /* ftG2T */, 3 /* ftT2G */,
        8 /* rtnG2T */, 8 /* rtnT2G */, QOS_MAKE_SDU_LEN(160) /* maxSduG2T */, QOS_MAKE_SDU_LEN(160) /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(160) /* maxPduG2T */, QOS_MAKE_SDU_LEN(160) /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        0 /* mcsG2T */, 0 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_0 /* qosLevel */, QOS_LEVEL_1 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 32 /* downwardBitrate */, 32 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 60 /* maxLatencyT2G */, 80 /* icbInterval */, 3 /* ftG2T */, 3 /* ftT2G */,
        8 /* rtnG2T */, 8 /* rtnT2G */, QOS_MAKE_SDU_LEN(80) /* maxSduG2T */, QOS_MAKE_SDU_LEN(80) /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(80) /* maxPduG2T */, QOS_MAKE_SDU_LEN(80) /* maxPduT2G */,
        2 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 1 /* phyG2T */, 1 /* phyT2G */,
        0 /* mcsG2T */, 0 /* mcsT2G */, 0 /* pilotG2T */, 0 /* pilotT2G */, 8 /* frameG2T */, 8 /* frameT2G */
    },
};

/*
 * 空间音频
 */
static QOSM_LinkParam g_qosSpatialAudio[] = {
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_6 /* qosLevel */, QOS_LEVEL_6 /* upQosLevel */,
        QOS_LEVEL_4 /* downQosLevel */, 640 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 5 /* ftG2T */, 1 /* ftT2G */,
        0 /* rtnG2T */, 0 /* rtnT2G */, QOS_MAKE_SDU_LEN(800) /* maxSduG2T */, 26 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(800) /* maxPduG2T */, 26 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 1 /* phyG2T */, 0 /* phyT2G */,
        7 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_5 /* qosLevel */, QOS_LEVEL_5 /* upQosLevel */,
        QOS_LEVEL_3 /* downQosLevel */, 320 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 5 /* ftG2T */, 1 /* ftT2G */,
        0 /* rtnG2T */, 0 /* rtnT2G */, QOS_MAKE_SDU_LEN(400) /* maxSduG2T */, 26 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(400) /* maxPduG2T */, 26 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 1 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_4 /* qosLevel */, QOS_LEVEL_6 /* upQosLevel */,
        QOS_LEVEL_2 /* downQosLevel */, 320 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 5 /* ftG2T */, 1 /* ftT2G */,
        0 /* rtnG2T */, 0 /* rtnT2G */, QOS_MAKE_SDU_LEN(400) /* maxSduG2T */, 26 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(400) /* maxPduG2T */, 26 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 1 /* phyG2T */, 0 /* phyT2G */,
        0 /* mcsG2T */, 0 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_3 /* qosLevel */, QOS_LEVEL_5 /* upQosLevel */,
        QOS_LEVEL_1 /* downQosLevel */, 192 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 5 /* ftG2T */, 1 /* ftT2G */,
        0 /* rtnG2T */, 0 /* rtnT2G */, QOS_MAKE_SDU_LEN(240) /* maxSduG2T */, 26 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(240) /* maxPduG2T */, 26 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_2 /* qosLevel */, QOS_LEVEL_4 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 192 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 5 /* ftG2T */, 1 /* ftT2G */,
        0 /* rtnG2T */, 0 /* rtnT2G */, QOS_MAKE_SDU_LEN(240) /* maxSduG2T */, 26 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(240) /* maxPduG2T */, 26 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_1 /* qosLevel */, QOS_LEVEL_3 /* upQosLevel */,
        QOS_LEVEL_1 /* downQosLevel */, 96 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 5 /* ftG2T */, 1 /* ftT2G */,
        0 /* rtnG2T */, 0 /* rtnT2G */, QOS_MAKE_SDU_LEN(120) /* maxSduG2T */, 26 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(120) /* maxPduG2T */, 26 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_0 /* qosLevel */, QOS_LEVEL_2 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 96 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 5 /* ftG2T */, 1 /* ftT2G */,
        0 /* rtnG2T */, 0 /* rtnT2G */, QOS_MAKE_SDU_LEN(120) /* maxSduG2T */, 26 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(120) /* maxPduG2T */, 26 /* maxPduT2G */,
        4 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
};

/*
 * 双耳高清录音
 */
static QOSM_LinkParam g_qosHdRecording[] = {
    {
        QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_1 /* qosLevel */, QOS_LEVEL_1 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 192 /* downwardBitrate */, 192 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        220 /* maxLatencyG2T */, 80 /* maxLatencyT2G */, 80 /* icbInterval */, 11 /* ftG2T */, 4 /* ftT2G */,
        0 /* rtnG2T */, 0 /* rtnT2G */, QOS_MAKE_SDU_LEN(240) /* maxSduG2T */, QOS_MAKE_SDU_LEN(245) /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(240) /* maxPduG2T */, QOS_MAKE_SDU_LEN(245) /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 1 /* phyG2T */, 1 /* phyT2G */,
        0 /* mcsG2T */, 0 /* mcsT2G */, 0 /* pilotG2T */, 0 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_0 /* qosLevel */, QOS_LEVEL_1 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 96 /* downwardBitrate */, 192 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        220 /* maxLatencyG2T */, 80 /* maxLatencyT2G */, 80 /* icbInterval */, 11 /* ftG2T */, 4 /* ftT2G */,
        0 /* rtnG2T */, 0 /* rtnT2G */, QOS_MAKE_SDU_LEN(120) /* maxSduG2T */, QOS_MAKE_SDU_LEN(245) /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(120) /* maxPduG2T */, QOS_MAKE_SDU_LEN(245) /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 1 /* phyT2G */,
        0 /* mcsG2T */, 0 /* mcsT2G */, 0 /* pilotG2T */, 0 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
};

/*
 * 双耳K歌
 */
static QOSM_LinkParam g_qosKaraoke[] = {
{
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_4 /* qosLevel */, QOS_LEVEL_4 /* upQosLevel */,
        QOS_LEVEL_2 /* downQosLevel */, 320 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        30 /* maxLatencyG2T */, 10 /* maxLatencyT2G */, 40 /* icbInterval */, 1 /* ftG2T */, 1 /* ftT2G */,
        8 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(200) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(200) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 0 /* bnT2G */, 2 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_3 /* qosLevel */, QOS_LEVEL_3 /* upQosLevel */,
        QOS_LEVEL_1 /* downQosLevel */, 320 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        30 /* maxLatencyG2T */, 10 /* maxLatencyT2G */, 40 /* icbInterval */, 1 /* ftG2T */, 1 /* ftT2G */,
        8 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(200) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(200) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 0 /* bnT2G */, 1 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_2 /* qosLevel */, QOS_LEVEL_4 /* upQosLevel */,
        QOS_LEVEL_2 /* downQosLevel */, 192 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        30 /* maxLatencyG2T */, 10 /* maxLatencyT2G */, 40 /* icbInterval */, 1 /* ftG2T */, 1 /* ftT2G */,
        8 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(120) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(120) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 0 /* bnT2G */, 1 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_1 /* qosLevel */, QOS_LEVEL_3 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 192 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        30 /* maxLatencyG2T */, 10 /* maxLatencyT2G */, 40 /* icbInterval */, 1 /* ftG2T */, 1 /* ftT2G */,
        8 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(120) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(120) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 0 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 0 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_0 /* qosLevel */, QOS_LEVEL_1 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 96 /* downwardBitrate */, 0 /* upwardBitrate */,
        10000 /* sduIntervalG2T */, 10000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        30 /* maxLatencyG2T */, 10 /* maxLatencyT2G */, 40 /* icbInterval */, 1 /* ftG2T */, 1 /* ftT2G */,
        11 /* rtnG2T */, 3 /* rtnT2G */, QOS_MAKE_SDU_LEN(60) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(60) /* maxPduG2T */, 0 /* maxPduT2G */,
        4 /* nse */, 1 /* bnG2T */, 0 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        0 /* mcsG2T */, 0 /* mcsT2G */, 0 /* pilotG2T */, 0 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
};

/*
 * 语音助手
 */
static QOSM_LinkParam g_qosVoiceAssistant[] = {
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_2 /* qosLevel */, QOS_LEVEL_2 /* upQosLevel */,
        QOS_LEVEL_1 /* downQosLevel */, 64 /* downwardBitrate */, 256 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 60 /* maxLatencyT2G */, 80 /* icbInterval */, 3 /* ftG2T */, 3 /* ftT2G */,
        8 /* rtnG2T */, 8 /* rtnT2G */, QOS_MAKE_SDU_LEN(160) /* maxSduG2T */, QOS_MAKE_SDU_LEN(640) /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(160) /* maxPduG2T */, QOS_MAKE_SDU_LEN(640) /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 1 /* phyG2T */, 1 /* phyT2G */,
        0 /* mcsG2T */, 6 /* mcsT2G */, 3 /* pilotG2T */, 2 /* pilotT2G */, 0 /* frameG2T */, 1 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_1 /* qosLevel */, QOS_LEVEL_2 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 64 /* downwardBitrate */, 64 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 60 /* maxLatencyT2G */, 80 /* icbInterval */, 3 /* ftG2T */, 3 /* ftT2G */,
        8 /* rtnG2T */, 8 /* rtnT2G */, QOS_MAKE_SDU_LEN(160) /* maxSduG2T */, QOS_MAKE_SDU_LEN(160) /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(160) /* maxPduG2T */, QOS_MAKE_SDU_LEN(160) /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 1 /* phyG2T */, 1 /* phyT2G */,
        0 /* mcsG2T */, 0 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_0 /* qosLevel */, QOS_LEVEL_1 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 32 /* downwardBitrate */, 32 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 60 /* maxLatencyT2G */, 80 /* icbInterval */, 3 /* ftG2T */, 3 /* ftT2G */,
        14 /* rtnG2T */, 14 /* rtnT2G */, QOS_MAKE_SDU_LEN(80) /* maxSduG2T */, QOS_MAKE_SDU_LEN(80) /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(80) /* maxPduG2T */, QOS_MAKE_SDU_LEN(80) /* maxPduT2G */,
        5 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        0 /* mcsG2T */, 0 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_2 /* qosLevel */, QOS_LEVEL_2 /* upQosLevel */,
        QOS_LEVEL_1 /* downQosLevel */, 64 /* downwardBitrate */, 256 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 60 /* maxLatencyT2G */, 80 /* icbInterval */, 3 /* ftG2T */, 3 /* ftT2G */,
        8 /* rtnG2T */, 8 /* rtnT2G */, QOS_MAKE_SDU_LEN(160) /* maxSduG2T */, QOS_MAKE_SDU_LEN(640) /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(160) /* maxPduG2T */, QOS_MAKE_SDU_LEN(640) /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 1 /* phyG2T */, 2 /* phyT2G */,
        0 /* mcsG2T */, 6 /* mcsT2G */, 3 /* pilotG2T */, 2 /* pilotT2G */, 0 /* frameG2T */, 1 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_1 /* qosLevel */, QOS_LEVEL_2 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 64 /* downwardBitrate */, 64 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 60 /* maxLatencyT2G */, 80 /* icbInterval */, 3 /* ftG2T */, 3 /* ftT2G */,
        8 /* rtnG2T */, 8 /* rtnT2G */, QOS_MAKE_SDU_LEN(160) /* maxSduG2T */, QOS_MAKE_SDU_LEN(160) /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(160) /* maxPduG2T */, QOS_MAKE_SDU_LEN(160) /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 1 /* phyT2G */,
        0 /* mcsG2T */, 0 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_0 /* qosLevel */, QOS_LEVEL_1 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 32 /* downwardBitrate */, 32 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        60 /* maxLatencyG2T */, 60 /* maxLatencyT2G */, 80 /* icbInterval */, 3 /* ftG2T */, 3 /* ftT2G */,
        14 /* rtnG2T */, 14 /* rtnT2G */, QOS_MAKE_SDU_LEN(80) /* maxSduG2T */, QOS_MAKE_SDU_LEN(80) /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(80) /* maxPduG2T */, QOS_MAKE_SDU_LEN(80) /* maxPduT2G */,
        5 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 1 /* phyT2G */,
        0 /* mcsG2T */, 0 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
};

/*
 * 告警音、提示音、导航音、视频音
 */
static QOSM_LinkParam g_qosOthers[] = {
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_7 /* qosLevel */, QOS_LEVEL_7 /* upQosLevel */,
        QOS_LEVEL_5 /* downQosLevel */, 1500 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        220 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 11 /* ftG2T */, 1 /* ftT2G */,
        1 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(1876) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(1876) /* maxPduG2T */, 0 /* maxPduT2G */,
        2 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 2 /* phyG2T */, 0 /* phyT2G */,
        7 /* mcsG2T */, 6 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_6 /* qosLevel */, QOS_LEVEL_7 /* upQosLevel */,
        QOS_LEVEL_5 /* downQosLevel */, 960 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        220 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 11 /* ftG2T */, 1 /* ftT2G */,
        2 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(1200) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(1200) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 2 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 6 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_5 /* qosLevel */, QOS_LEVEL_6 /* upQosLevel */,
        QOS_LEVEL_3 /* downQosLevel */, 640 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        220 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 11 /* ftG2T */, 1 /* ftT2G */,
        1 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(800) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(800) /* maxPduG2T */, 0 /* maxPduT2G */,
        2 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 1 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 6 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_4 /* qosLevel */, QOS_LEVEL_4 /* upQosLevel */,
        QOS_LEVEL_2 /* downQosLevel */, 320 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        220 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 11 /* ftG2T */, 1 /* ftT2G */,
        3 /* rtnG2T */, 3 /* rtnT2G */, QOS_MAKE_SDU_LEN(400) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(400) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 1 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 6 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_3 /* qosLevel */, QOS_LEVEL_5 /* upQosLevel */,
        QOS_LEVEL_1 /* downQosLevel */, 320 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        220 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 11 /* ftG2T */, 1 /* ftT2G */,
        1 /* rtnG2T */, 1 /* rtnT2G */, QOS_MAKE_SDU_LEN(400) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(400) /* maxPduG2T */, 0 /* maxPduT2G */,
        2 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 6 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_50P /* dutyCycle */, QOS_LEVEL_2 /* qosLevel */, QOS_LEVEL_4 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 192 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        220 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 11 /* ftG2T */, 1 /* ftT2G */,
        4 /* rtnG2T */, 4 /* rtnT2G */, QOS_MAKE_SDU_LEN(240) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(240) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 6 /* mcsT2G */, 2 /* pilotG2T */, 3 /* pilotT2G */, 1 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_100P /* dutyCycle */, QOS_LEVEL_1 /* qosLevel */, QOS_LEVEL_3 /* upQosLevel */,
        QOS_LEVEL_0 /* downQosLevel */, 192 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        220 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 11 /* ftG2T */, 1 /* ftT2G */,
        2 /* rtnG2T */, 2 /* rtnT2G */, QOS_MAKE_SDU_LEN(240) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(240) /* maxPduG2T */, 0 /* maxPduT2G */,
        3 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 6 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
    {
        QOS_DUTY_CYCLE_ANY /* dutyCycle */, QOS_LEVEL_0 /* qosLevel */, QOS_LEVEL_0 /* upQosLevel, not used */,
        QOS_LEVEL_0 /* downQosLevel */, 96 /* downwardBitrate */, 0 /* upwardBitrate */,
        20000 /* sduIntervalG2T */, 20000 /* sduIntervalT2G */, 0 /* sca */, 1 /* packing */, 0 /* framing */,
        220 /* maxLatencyG2T */, 20 /* maxLatencyT2G */, 80 /* icbInterval */, 11 /* ftG2T */, 1 /* ftT2G */,
        4 /* rtnG2T */, 4 /* rtnT2G */, QOS_MAKE_SDU_LEN(120) /* maxSduG2T */, 0 /* maxSduT2G */,
        QOS_MAKE_SDU_LEN(120) /* maxPduG2T */, 0 /* maxPduT2G */,
        5 /* nse */, 1 /* bnG2T */, 1 /* bnT2G */, 0 /* phyG2T */, 0 /* phyT2G */,
        6 /* mcsG2T */, 6 /* mcsT2G */, 3 /* pilotG2T */, 3 /* pilotT2G */, 0 /* frameG2T */, 0 /* frameT2G */
    },
};

static QOSM_QosIndexTable g_qosIndexTable[] = {
    {
        QOSM_QOSINDEX_AUDIO, QOS_AUDIO_SIZE, g_qosAudio
    },
    {
        QOSM_QOSINDEX_LOW_LATENCY, QOS_LOW_LATENCY_SIZE, g_qosLowLatency
    },
    {
        QOSM_QOSINDEX_CALL, QOS_CALL_SIZE, g_qosCall
    },
    {
        QOSM_QOSINDEX_SPATIAL_AUDIO, QOS_SPATIAL_AUDIO_SIZE, g_qosSpatialAudio
    },
    {
        QOSM_QOSINDEX_HD_RECORDING, QOS_HD_RECORDING_SIZE, g_qosHdRecording
    },
    {
        QOSM_QOSINDEX_KARAOKE, sizeof(g_qosKaraoke)/sizeof(QOSM_LinkParam), g_qosKaraoke 
    },
    {
        QOSM_QOSINDEX_VOICE_ASSISTANT, QOS_VOICE_ASSISTANT_SIZE, g_qosVoiceAssistant
    },
    {
        QOSM_QOSINDEX_OTHERS, QOS_OTHERS_SIZE, g_qosOthers
    },
};

static uint32_t g_linkParamTableSize = (sizeof(g_qosIndexTable) / sizeof(QOSM_QosIndexTable));

static QOSM_QosIndexStartParam g_qosStartParam[] = {
    {
        QOSM_QOSINDEX_AUDIO,
        {
            QOS_LEVEL_2 /* startLevel */, QOS_BAND_2D4 /* startBand */, QOS_DUTY_CYCLE_100P /* startDutyCycle */,
            QOS_AUDIO_SIZE /* levelCnt */
        }
    },
    {
        QOSM_QOSINDEX_LOW_LATENCY,
        {
            QOS_LEVEL_1 /* startLevel */, QOS_BAND_2D4 /* startBand */, QOS_DUTY_CYCLE_100P /* startDutyCycle */,
            QOS_LOW_LATENCY_SIZE /* levelCnt */
        }
    },
    {
        QOSM_QOSINDEX_CALL,
        {
            QOS_LEVEL_1 /* startLevel */, QOS_BAND_2D4 /* startBand */, QOS_DUTY_CYCLE_100P /* startDutyCycle */,
            QOS_CALL_SIZE /* levelCnt */
        }
    },
    {
        QOSM_QOSINDEX_SPATIAL_AUDIO,
        {
            QOS_LEVEL_0 /* startLevel */, QOS_BAND_2D4 /* startBand */, QOS_DUTY_CYCLE_100P /* startDutyCycle */,
            QOS_SPATIAL_AUDIO_SIZE /* levelCnt */
        }
    },
    {
        QOSM_QOSINDEX_HD_RECORDING,
        {
            QOS_LEVEL_1 /* startLevel */, QOS_BAND_2D4 /* startBand */, QOS_DUTY_CYCLE_100P /* startDutyCycle */,
            QOS_HD_RECORDING_SIZE /* levelCnt */
        }
    },
    {
        QOSM_QOSINDEX_VOICE_ASSISTANT,
        {
            QOS_LEVEL_1 /* startLevel */, QOS_BAND_2D4 /* startBand */, QOS_DUTY_CYCLE_100P /* startDutyCycle */,
            QOS_VOICE_ASSISTANT_SIZE /* levelCnt */
        }
    },
    {
        QOSM_QOSINDEX_KARAOKE,
        {
            QOS_LEVEL_1 /* startLevel */, QOS_BAND_2D4 /* startBand */, QOS_DUTY_CYCLE_100P /* startDutyCycle */,
            QOS_LOW_LATENCY_SIZE /* levelCnt */
        }
    },
    {
        QOSM_QOSINDEX_OTHERS,
        {
            QOS_LEVEL_3 /* startLevel */, QOS_BAND_2D4 /* startBand */, QOS_DUTY_CYCLE_100P /* startDutyCycle */,
            QOS_OTHERS_SIZE /* levelCnt */
        }
    },
};

static uint32_t g_qosStartParamSize = (sizeof(g_qosStartParam) / sizeof(QOSM_QosIndexStartParam));

static QOSM_QosIndexStartLevel g_qosStartLevel[] = {
    {
        QOSM_QOSINDEX_AUDIO /* qosIndex */, QOS_LEVEL_2 /* qosLevel */
    },
    {
        QOSM_QOSINDEX_LOW_LATENCY /* qosIndex */, QOS_LEVEL_1 /* qosLevel */
    },
    {
        QOSM_QOSINDEX_CALL /* qosIndex */, QOS_LEVEL_1 /* qosLevel */
    },
    {
        QOSM_QOSINDEX_SPATIAL_AUDIO /* qosIndex */, QOS_LEVEL_0 /* qosLevel */
    },
    {
        QOSM_QOSINDEX_HD_RECORDING /* qosIndex */, QOS_LEVEL_1 /* qosLevel */
    },
    {
        QOSM_QOSINDEX_VOICE_ASSISTANT /* qosIndex */, QOS_LEVEL_1 /* qosLevel */
    },
    {
        QOSM_QOSINDEX_KARAOKE /* qosIndex */, QOS_LEVEL_1 /* qosLevel */
    },
    {
        QOSM_QOSINDEX_OTHERS /* qosIndex */, QOS_LEVEL_3 /* qosLevel */
    },
};

static atomic_bool g_isAutoRateEnabled = true;

bool QOSM_IsAutorateEnabled(void)
{
    return atomic_load(&g_isAutoRateEnabled);
}

static atomic_uint_least8_t g_dutyCycle = QOSM_DEFAULT_DUTYCYCLE;
static atomic_uint_least8_t g_fixedDutyCycle = QOSM_DUTYCYCLE_NOT_FIXED;

void QOSM_UpdateDutyCycle(uint8_t dutyCycle)
{
    atomic_store(&g_dutyCycle, dutyCycle);
}

uint8_t QOSM_GetDutyCycle(void)
{
    if (atomic_load(&g_fixedDutyCycle) == QOSM_DUTYCYCLE_FIXED_100P) {
        QOSM_LOGD("get duty cycle: %u", QOS_DUTY_CYCLE_100P);
        return QOS_DUTY_CYCLE_100P;
    } else if (atomic_load(&g_fixedDutyCycle) == QOSM_DUTYCYCLE_FIXED_50P) {
        QOSM_LOGD("get duty cycle: %u", QOS_DUTY_CYCLE_50P);
        return QOS_DUTY_CYCLE_50P;
    }
    uint8_t dutyCycle = atomic_load(&g_dutyCycle);
    QOSM_LOGI("get duty cycle: %d", dutyCycle);
    return dutyCycle;
}

bool QOSM_IsDutyCycleFixed(void)
{
    return atomic_load(&g_fixedDutyCycle) != QOSM_DUTYCYCLE_NOT_FIXED;
}

static int16_t QOSM_GetTargetStartQosLevel(QOSM_QosIndex qosIndex, int32_t downwardBitrate)
{
    for (uint32_t i = 0; i < g_linkParamTableSize; i++) {
        if (g_qosIndexTable[i].qosIndex != qosIndex) {
            continue;
        }
        for (uint32_t j = 0; j < g_qosIndexTable[i].qosCnt; j++) {
            if (g_qosIndexTable[i].qos[j].downwardBitrate != downwardBitrate) {
                continue;
            }
            if ((g_qosIndexTable[i].qos[j].dutyCycle != QOS_DUTY_CYCLE_ANY &&
                g_qosIndexTable[i].qos[j].dutyCycle != QOSM_GetDutyCycle())) {
                continue;
            }
            return g_qosIndexTable[i].qos[j].qosLevel;
        }
    }
    return -1;
}

static QOSM_QosLevel QOSM_GetStartParamDefault(QOSM_QosIndex qosIndex, uint16_t downwardBitrate)
{
    for (uint32_t i = 0; i < g_linkParamTableSize; i++) {
        if (g_qosIndexTable[i].qosIndex != qosIndex) {
            continue;
        }
        for (uint32_t j = 0; j < g_qosIndexTable[i].qosCnt; j++) {
            // 从高到低搜索小于期望码率的参数组
            if (g_qosIndexTable[i].qos[j].downwardBitrate >= downwardBitrate) {
                continue;
            }
            // 搜素满足占空比要求的参数组
            if (g_qosIndexTable[i].qos[j].dutyCycle != QOSM_GetDutyCycle()) {
                continue;
            }
            return g_qosIndexTable[i].qos[j].qosLevel;
        }
        // 不存在小于期望码率且满足占空比要求的参数组，返回默认值
        return g_qosStartLevel[i].qosLevel;
    }
    // 异常保护，不可能来到这个分支
    QOSM_LOGE("get default start param error");
    return QOS_LEVEL_0;
}

QOSM_StartParam *QOSM_GetStartParamByIndex(QOSM_QosIndex qosIndex, uint16_t bitrate, uint8_t linkCnt)
{
    bool isAutoRateEnabled = PropertyGetInt32("persist.config.nearlink.autorate", 1) != 0;
    atomic_store(&g_isAutoRateEnabled, isAutoRateEnabled);
    uint8_t dutyCycle = (uint8_t)PropertyGetInt32("persist.config.nearlink.dutycycle", QOSM_DUTYCYCLE_NOT_FIXED);
    atomic_store(&g_fixedDutyCycle, dutyCycle);
    QOSM_LOGI("set bitrate: %u, linkCnt: %u, autorate enabled: %d, dutyCycle: %d",
        bitrate, linkCnt, isAutoRateEnabled, dutyCycle);

    int16_t targetQosLevel = -1;
    int downwardBitrate = QOSM_GetICBTypeByIndex(qosIndex) == CM_IOB ? bitrate * linkCnt : bitrate;
    targetQosLevel = QOSM_GetTargetStartQosLevel(qosIndex, downwardBitrate);

    for (uint32_t i = 0; i < g_qosStartParamSize; i++) {
        if (g_qosStartParam[i].qosIndex != qosIndex) {
            continue;
        }
        if (targetQosLevel != -1) {
            g_qosStartParam[i].startParam.startLevel = targetQosLevel;
            QOSM_LOGI("start qos level: %u", g_qosStartParam[i].startParam.startLevel);
        } else {
            g_qosStartParam[i].startParam.startLevel = QOSM_GetStartParamDefault(qosIndex, downwardBitrate);
            QOSM_LOGI("start qos level by default: %u", g_qosStartParam[i].startParam.startLevel);
        }
        return &g_qosStartParam[i].startParam;
    }
    QOSM_LOGE("qos index: %u not found", qosIndex);
    return NULL;
}

QOSM_LinkParam *QOSM_GetQosParamByIndex(QOSM_QosIndex qosIndex, uint8_t qosLevelIndex)
{
    for (uint32_t i = 0; i < g_linkParamTableSize; i++) {
        if (g_qosIndexTable[i].qosIndex != qosIndex) {
            continue;
        }
        if (qosLevelIndex < g_qosIndexTable[i].qosCnt) {
            return &g_qosIndexTable[i].qos[qosLevelIndex];
        } else {
            return NULL;
        }
    }
    return NULL;
}

uint16_t QOSM_GetOriginalBitrate(CM_ICBType icbType, uint16_t bitrate, uint8_t linkCnt)
{
    if (linkCnt == 0) {
        return bitrate;
    }
    return icbType == CM_IOB ? bitrate / linkCnt : bitrate;
}

CM_ICBType QOSM_GetICBTypeByIndex(QOSM_QosIndex qosIndex)
{
    if (qosIndex == QOSM_QOSINDEX_CALL || qosIndex == QOSM_QOSINDEX_VOICE_ASSISTANT) {
        return CM_IMB;
    }
    return CM_IOB;
}

uint32_t QOSM_GetICBG2TParamByIndex(QOSM_QosIndex qosIndex, QOSM_ICBParam *param)
{
    for (uint32_t i = 0; i < g_linkParamTableSize; i++) {
        if (g_qosIndexTable[i].qosIndex != qosIndex) {
            continue;
        }
        param->sduInterval = g_qosIndexTable[i].qos[0].sduIntervalG2T;
        param->sca = g_qosIndexTable[i].qos[0].sca;
        param->packing = g_qosIndexTable[i].qos[0].packing;
        param->framing = g_qosIndexTable[i].qos[0].framing;
        param->maxLatency = g_qosIndexTable[i].qos[0].maxLatencyG2T;
        param->icbInterval = g_qosIndexTable[i].qos[0].icbInterval;
        param->ft = g_qosIndexTable[i].qos[0].ftG2T;
        param->rtn = g_qosIndexTable[i].qos[0].rtnG2T;
        for (uint32_t j = 0; j < g_qosIndexTable[i].qosCnt; j++) {
            param->maxSdu = QOSM_MAX(param->maxSdu, g_qosIndexTable[i].qos[j].maxSduG2T);
            param->maxPdu = QOSM_MAX(param->maxPdu, g_qosIndexTable[i].qos[j].maxPduG2T);
        }
        param->nse = g_qosIndexTable[i].qos[0].nse;
        param->bn = g_qosIndexTable[i].qos[0].bnG2T;
        param->phy = g_qosIndexTable[i].qos[0].phyG2T;
        param->mcs = g_qosIndexTable[i].qos[0].mcsG2T;
        param->pilot = g_qosIndexTable[i].qos[0].pilotG2T;
        param->frame = g_qosIndexTable[i].qos[0].frameG2T;
        param->icbNum = DLI_DataNumGet(ICB_DATA_TYPE);
        return QOSM_SUCCESS;
    }
    return QOSM_NOT_FOUND_ERR;
}

uint32_t QOSM_GetICBT2GParamByIndex(QOSM_QosIndex qosIndex, QOSM_ICBParam *param)
{
    for (uint32_t i = 0; i < g_linkParamTableSize; i++) {
        if (g_qosIndexTable[i].qosIndex != qosIndex) {
            continue;
        }
        param->sduInterval = g_qosIndexTable[i].qos[0].sduIntervalT2G;
        param->sca = g_qosIndexTable[i].qos[0].sca;
        param->packing = g_qosIndexTable[i].qos[0].packing;
        param->framing = g_qosIndexTable[i].qos[0].framing;
        param->maxLatency = g_qosIndexTable[i].qos[0].maxLatencyT2G;
        param->icbInterval = g_qosIndexTable[i].qos[0].icbInterval;
        param->ft = g_qosIndexTable[i].qos[0].ftT2G;
        param->rtn = g_qosIndexTable[i].qos[0].rtnT2G;
        for (uint32_t j = 0; j < g_qosIndexTable[i].qosCnt; j++) {
            param->maxSdu = QOSM_MAX(param->maxSdu, g_qosIndexTable[i].qos[j].maxSduT2G);
            param->maxPdu = QOSM_MAX(param->maxPdu, g_qosIndexTable[i].qos[j].maxPduT2G);
        }
        param->nse = g_qosIndexTable[i].qos[0].nse;
        param->bn = g_qosIndexTable[i].qos[0].bnT2G;
        param->phy = g_qosIndexTable[i].qos[0].phyT2G;
        param->mcs = g_qosIndexTable[i].qos[0].mcsT2G;
        param->pilot = g_qosIndexTable[i].qos[0].pilotT2G;
        param->frame = g_qosIndexTable[i].qos[0].frameT2G;
        param->icbNum = DLI_DataNumGet(ICB_DATA_TYPE);
        return QOSM_SUCCESS;
    }
    return QOSM_NOT_FOUND_ERR;
}

#define QOSM_ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

static struct QOSM_AutoRateThreshold g_autorateThresholdQos1DutyCycle100P[] = {
    /* bitrate, up ackrate, down ackrate, up diff max, up rssi, down rssi */
    { 4600, 100, 87, 0, 0, 0 },
    { 2300, 90, 82, 2, -67, 0 },
    { 1500, 85, 63, 255, -128, 0 },
    { 960, 82, 48, 255, -128, 0 },
    { 640, 83, 55, 255, -128, 0 },
    { 320, 85, 60, 255, -128, 0 },
    { 192, 75, 50, 255, -128, 0 },
    { 96, 75, 0, 255, -128, 0 },
};

static struct QOSM_AutoRateThreshold g_autorateThresholdQos1DutyCycle50P[] = {
     /* bitrate, up ackrate, down ackrate, up diff max, up rssi, down rssi */
    { 4600, 100, 87, 0, 0, 0 },
    { 2300, 90, 82, 2, -67, 0 },
    { 1500, 85, 63, 255, -128, 0 },
    { 960, 82, 48, 255, -128, 0 },
    { 640, 83, 55, 255, -128, 0 },
    { 320, 85, 60, 255, -128, 0 },
    { 192, 85, 50, 255, -128, 0 },
    { 96, 75, 0, 255, -128, 0 },
};

static struct QOSM_AutoRateThreshold g_autorateThresholdQos2DutyCycle100P[] = {
    /* bitrate, up ackrate, down ackrate, up diff max, up rssi, down rssi */
    { 320, 100, 55, 0, 0, 0 },
    { 192, 85, 60, 255, -128, 0 },
    { 96, 75, 0, 255, -128, 0 },
};

static struct QOSM_AutoRateThreshold g_autorateThresholdQos2DutyCycle50P[] = {
    /* bitrate, up ackrate, down ackrate, up diff max, up rssi, down rssi */
    { 320, 100, 82, 0, 0, 0 },
    { 192, 85, 60, 255, -128, 0 },
    { 96, 75, 0, 255, -128, 0 },
};

static struct QOSM_AutoRateThreshold g_autorateThresholdQos3[] = {
    /* bitrate, up ackrate, down ackrate, up diff max, up rssi, down rssi */
    { 64, 100, 70, 0, 0, -80},
    { 32, 80, 0, 255, -65, -128},
};

static struct QOSM_AutoRateThreshold g_autorateThresholdQos4DutyCycle100P[] = {
    /* bitrate, up ackrate, down ackrate, up diff max, up rssi, down rssi */
    { 640, 100, 83, 0, 0, 0 },
    { 320, 88, 70, 255, -128, 0 },
    { 192, 75, 60, 255, -128, 0 },
    { 96, 70, 0, 255, -128, 0 },
};

static struct QOSM_AutoRateThreshold g_autorateThresholdQos4DutyCycle50P[] = {
    /* bitrate, up ackrate, down ackrate, up diff max, up rssi, down rssi */
    { 320, 100, 70, 0, 0, 0 },
    { 192, 85, 60, 255, -128, 0 },
    { 96, 70, 0, 255, -128, 0 },
};

static struct QOSM_AutoRateThreshold g_autorateThresholdQos5[] = {
    /* bitrate, up ackrate, down ackrate, up diff max, up rssi, down rssi */
    { 192, 100, 50, 255, -128, 0 },
    { 96, 75, 0, 255, -128, 0 },
};

static struct QOSM_AutoRateThreshold g_autorateThresholdQos7DutyCycle100P[] = {
    /* bitrate, up ackrate, down ackrate, up diff max, up rssi, down rssi */
    { 1500, 100, 63, 0, 0, 0 },
    { 960, 82, 48, 255, -128, 0 },
    { 640, 83, 55, 255, -128, 0 },
    { 320, 85, 60, 255, -128, 0 },
    { 192, 75, 50, 255, -128, 0 },
    { 96, 75, 0, 255, -128, 0 },
};

static struct QOSM_AutoRateThreshold g_autorateThresholdQos7DutyCycle50P[] = {
    /* bitrate, up ackrate, down ackrate, up diff max, up rssi */
    { 320, 100, 70, 0, 0, 0 },
    { 192, 85, 60, 255, -128, 0 },
    { 96, 70, 0, 255, -128, 0 },
};

static struct QOSM_AutoRateThresholdItem g_autoRateThreshold100P[QOSM_QOSINDEX_MAX] = {
    // qos index 0, not used
    { NULL, 0 },
    // qos index 1
    { g_autorateThresholdQos1DutyCycle100P, QOSM_ARRAY_LEN(g_autorateThresholdQos1DutyCycle100P), },
    // qos index 2
    { g_autorateThresholdQos2DutyCycle100P, QOSM_ARRAY_LEN(g_autorateThresholdQos2DutyCycle100P), },
    // qos index 3, not used
    { g_autorateThresholdQos3, QOSM_ARRAY_LEN(g_autorateThresholdQos3), },
    // qos index 4
    { g_autorateThresholdQos4DutyCycle100P, QOSM_ARRAY_LEN(g_autorateThresholdQos4DutyCycle100P), },
    // qos index 5
    { g_autorateThresholdQos5, QOSM_ARRAY_LEN(g_autorateThresholdQos5), },
    // qos index 6, not used
    { NULL, 0 },
    // qos index 7
    { g_autorateThresholdQos7DutyCycle100P, QOSM_ARRAY_LEN(g_autorateThresholdQos7DutyCycle100P), },
};

static struct QOSM_AutoRateThresholdItem g_autoRateThreshold50P[QOSM_QOSINDEX_MAX] = {
    // qos index 0, not used
    { NULL, 0 },
    // qos index 1
    { g_autorateThresholdQos1DutyCycle50P, QOSM_ARRAY_LEN(g_autorateThresholdQos1DutyCycle50P), },
    // qos index 2
    { g_autorateThresholdQos2DutyCycle50P, QOSM_ARRAY_LEN(g_autorateThresholdQos2DutyCycle50P), },
    // qos index 3, not used
    { g_autorateThresholdQos3, QOSM_ARRAY_LEN(g_autorateThresholdQos3), },
    // qos index 4
    { g_autorateThresholdQos4DutyCycle50P, QOSM_ARRAY_LEN(g_autorateThresholdQos4DutyCycle50P), },
    // qos index 5
    { g_autorateThresholdQos5, QOSM_ARRAY_LEN(g_autorateThresholdQos5), },
    // qos index 6, not used
    { NULL, 0 },
    // qos index 7
    { g_autorateThresholdQos7DutyCycle50P, QOSM_ARRAY_LEN(g_autorateThresholdQos7DutyCycle50P), },
};


static const struct QOSM_AutoRateThreshold *QOSM_GetAutorateThresholdInner(struct QOSM_AutoRateThreshold *t,
    uint32_t size, uint16_t bitrate)
{
    if (t == NULL || size == 0) {
        QOSM_LOGE("invalid threshold");
        return NULL;
    }

    for (uint32_t i = 0; i < size; i++) {
        if (t[i].bitrate == bitrate) {
            return &t[i];
        }
    }

    QOSM_LOGE("invalid bitrate %hu", bitrate);
    return NULL;
}

const struct QOSM_AutoRateThreshold *QOSM_GetAutorateThreshold(uint32_t qosIndex, uint16_t bitrate)
{
    if (qosIndex >= QOSM_QOSINDEX_MAX) {
        QOSM_LOGE("invalid qos index %u", qosIndex);
        return NULL;
    }

    if (QOSM_GetDutyCycle() == QOS_DUTY_CYCLE_50P) {
        return QOSM_GetAutorateThresholdInner(g_autoRateThreshold50P[qosIndex].t,
            g_autoRateThreshold50P[qosIndex].size, bitrate);
    }

    return QOSM_GetAutorateThresholdInner(g_autoRateThreshold100P[qosIndex].t,
        g_autoRateThreshold100P[qosIndex].size, bitrate);
}