/****************************************************************************
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
****************************************************************************/

/****************************************************************************
 *
 * this file contains qosm table manager:
 *
 ***************************************************************************/

#ifndef QOSM_TABLE_MGR_H
#define QOSM_TABLE_MGR_H

#include <stdint.h>
#include "cm_icb_def.h"
#include "qosm_autorate_def.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QOSM_MIN(a, b) (((a) > (b)) ? (b) : (a))
#define QOSM_MAX(a, b) (((a) > (b)) ? (a) : (b))

#define QOSM_DEFAULT_BAND QOS_BAND_2D4
#define QOSM_DEFAULT_DUTYCYCLE QOS_DUTY_CYCLE_100P
#define QOSM_MIN_BITRATE 32
#define QOSM_5G_MIN_BITRATE 64
#define QOSM_MAX_BITRATE 4600
#define QOSM_5G_MAX_BITRATE 1500

typedef enum {
    QOS_LEVEL_0 = 0,
    QOS_LEVEL_1 = 1,
    QOS_LEVEL_2 = 2,
    QOS_LEVEL_3 = 3,
    QOS_LEVEL_4 = 4,
    QOS_LEVEL_5 = 5,
    QOS_LEVEL_6 = 6,
    QOS_LEVEL_7 = 7,
} QOSM_QosLevel;

typedef enum {
    QOS_BAND_2D4 = 0,
    QOS_BAND_5D1 = 1,
    QOS_BAND_5D8 = 2,
} QOSM_QosBand;

/*
 * 和QOSM_DUTY_CYCLE_t保持一致
 */
typedef enum {
    QOS_DUTY_CYCLE_100P = 0,           /* 表示100%占空比 */
    QOS_DUTY_CYCLE_50P = 1,            /* 表示50%占空比 */
    QOS_DUTY_CYCLE_20P = 2,            /* 表示20%占空比 */
    QOS_DUTY_CYCLE_ANY,                /* 表示任意占空比，必须放在最后，不赋值 */
} QOSM_QosDutyCycle;

typedef struct {
    QOSM_QosLevel startLevel;          /* 起始等级 */
    QOSM_QosBand startBand;            /* 起始频段 */
    QOSM_QosDutyCycle startDutyCycle;  /* 起始占空比 */
    uint8_t levelCnt;                  /* 链路可用的星闪Qos等级个数 */
} QOSM_StartParam;

typedef struct {
    QOSM_QosDutyCycle dutyCycle;
    QOSM_QosLevel qosLevel;
    QOSM_QosLevel upQosLevel;                 /* 升档level */
    QOSM_QosLevel downQosLevel;               /* 降档level */
    uint16_t downwardBitrate;           /* 下行码率 */
    uint16_t upwardBitrate;             /* 上行码率 */

    /* ICG Param */
    uint32_t sduIntervalG2T;            /* G到T两个连续SDU之间的时间间隔，us */
    uint32_t sduIntervalT2G;            /* T到G两个连续SDU之间的时间间隔，us */
    uint8_t sca;                        /* 睡眠时钟精度 */
    uint8_t packing;                    /* 当前仅支持交叉 */
    uint8_t framing;                    /* 当前仅支持未切分 */
    uint16_t maxLatencyG2T;             /* G到T最大传输延迟，ms */
    uint16_t maxLatencyT2G;             /* T到G最大传输延迟，ms */
    uint16_t icbInterval;               /*!< 连续两个IMB Anchor Point的间隔时间，
                                            取值范围[0x0014,0x3E80]，时间 = N * 0.25ms，时间范围[5ms,4s] */
    uint8_t ftG2T;                      /*!< G到T方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */
    uint8_t ftT2G;                      /*!< T到G方向上每个Payload被Flush的超时时间，以ICB_Interval为单位，取值范围[0x01,0xFF] */

    /* ICB Param */
    uint8_t rtnG2T;                     /* G到T每一个IMB数据PDU重传次数 */
    uint8_t rtnT2G;                     /* T到G每一个IMB数据PDU重传次数 */
    uint16_t maxSduG2T;                 /* G到T最大SDU Payload字节数 */
    uint16_t maxSduT2G;
    uint16_t maxPduG2T;                 /* G到T最大PDU Payload字节数 */
    uint16_t maxPduT2G;
    uint8_t nse;                        /* 0x01-0x1F	在IMG中每一个IMB每个间隔内的子事件个数 */
    uint8_t bnG2T;                      /* 0x00	G到T方向上没有ICB数据包
                                           0x01-0x0F	G到T方向上每一个IMB每个间隔内传输新ICB数据包的个数 */
    uint8_t bnT2G;
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
    uint8_t frameG2T;                   /* 0	无线帧类型一
                                           1	无线帧类型二
                                           2	无线帧类型四 */
    uint8_t frameT2G;
} QOSM_LinkParam;

struct QOSM_AutoRateThreshold {
    uint16_t bitrate;
    uint8_t upAckRate;
    uint8_t downAckRate;
    uint8_t upDiffMax;
    int8_t upRssi;
    int8_t downRssi;
};

struct QOSM_AutoRateThresholdItem {
    struct QOSM_AutoRateThreshold *t;
    uint32_t size;
};

/**
 * @brief  当前是否使能了autorate功能
 * @return 使能返回true，未使能返回false
 */
bool QOSM_IsAutorateEnabled(void);

/**
 * @brief  更新duty cycle
 * @param  [in]  < dutyCycle > 占空比
 * @return void
 */
void QOSM_UpdateDutyCycle(uint8_t dutyCycle);

/**
 * @brief  获取当前的duty cycle
 * @return 占空比
 */
uint8_t QOSM_GetDutyCycle(void);


/**
 * @brief  当前是否固定了duty cycle
 * @return 固定了返回true，未固定返回false
 */
bool QOSM_IsDutyCycleFixed(void);

/**
 * @brief  获取起始的qos参数
 * @param  [in]  < qosIndex > qos索引
 * @param  [in]  < bitrate > 单设备码率
 * @param  [in]  < linkCnt > 设备链路数
 * @return 成功返回QOSM_StartParam，失败返回NULL
 */
QOSM_StartParam *QOSM_GetStartParamByIndex(QOSM_QosIndex qosIndex, uint16_t bitrate, uint8_t linkCnt);

/**
 * @brief  通过qos索引和qos档位，获取qos参数
 * @param  [in]  < qosIndex > qos索引
 * @param  [in]  < qosLevelIndex > qos档位索引
 * @return 成功返回QOSM_LinkParam，失败返回NULL
 */
QOSM_LinkParam *QOSM_GetQosParamByIndex(QOSM_QosIndex qosIndex, uint8_t qosLevelIndex);

uint16_t QOSM_GetOriginalBitrate(CM_ICBType icbType, uint16_t bitrate, uint8_t linkCnt);

/**
 * @brief  通过qos索引，获取ICB类型
 * @param  [in]  < qosIndex > qos索引
 * @return CM_ICBType
 */
CM_ICBType QOSM_GetICBTypeByIndex(QOSM_QosIndex qosIndex);

/**
 * @brief  通过qos索引，获取ICB G2T参数
 * @param  [in]  < qosIndex > qos索引
 * @param  [out]  < param > ICG G2T参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_GetICBG2TParamByIndex(QOSM_QosIndex qosIndex, QOSM_ICBParam *param);

/**
 * @brief  通过qos索引，获取ICB T2G参数
 * @param  [in]  < qosIndex > qos索引
 * @param  [out]  < param > ICG T2G参数
 * @return QOSM_SUCCESS: 成功, OTHER: 失败
 */
uint32_t QOSM_GetICBT2GParamByIndex(QOSM_QosIndex qosIndex, QOSM_ICBParam *param);

const struct QOSM_AutoRateThreshold *QOSM_GetAutorateThreshold(uint32_t qosIndex, uint16_t bitrate);

#ifdef __cplusplus
}
#endif
#endif