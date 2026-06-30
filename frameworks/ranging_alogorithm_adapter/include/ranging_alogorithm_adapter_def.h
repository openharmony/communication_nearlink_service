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

#ifndef HADM_RANGING_ADAPTER_DEF_H
#define HADM_RANGING_ADAPTER_DEF_H

#include <cstdint>
#include "nearlink_hadm_sounding_result.h"

namespace OHOS {
namespace Nearlink {

/**
 * @brief Hadm ranging algorithm output results.
 */
typedef struct {
    float disSmoothed;              /*!< @if Eng Smoothed distance value.
                                         @else   平滑后测距值。 @endif */
    float disOri;                   /*!< @if Eng Unsmoothed distance value.
                                         @else   未平滑测距值。 @endif */
    float disSlightSmoothed;        /*!< @if Eng Slight smooth distance value.
                                         @else   轻度平滑测距值。 @endif */
    float prob;                     /*!< @if Eng Confidence of distance value.
                                         @else   测距值的置信度。 @endif */
    float rssi;                     /*!< @if Eng Received signal strength indication.
                                         @else   信号强度。 @endif */
    float height;                   /*!< @if Eng Confidence intermediate information.
                                         @else   置信度中间信息。 @endif */
    uint8_t smoothNum;              /*!< @if Eng Number of consecutive valid ranging times.
                                         @else   连续有效测距的次数。 @endif */
} DisResult;

typedef struct {
    int8_t rssiLimit;
    uint8_t rStart;
    float thresholdCond2;
} ParaPair;

/**
 * @brief Selection mode of hadm ranging algorithm, METHOD_ADJ_R_END for northpole.
 */
typedef enum {
    METHOD_1M = 1,         /*!< @if Eng 1MHz channel frequency hopping algorithm (upper limit of ranging: 150m).
                                 @else   信道1MHz跳频算法(测距上限150m)。 @endif */
    METHOD_2M,             /*!< @if Eng 2MHz channel frequency hopping algorithm.
                                 @else   信道2MHz跳频算法。 @endif */
    METHOD_1M_2M,          /*!< @if Eng Low-complexity algorithm for 1MHz channel frequency hopping.
                                  (upper limit of ranging: 75 m)
                                 @else   信道1MHz跳频低复杂度算法(测距上限75m)。 @endif */
    METHOD_ADJ_R_END,       /*!< @if Eng Channel 1 MHz frequency hopping dynamic r algorithm
                                  (upper limit of ranging: 150m).
                                 @else   信道1MHz跳频动态r算法(测距上限150m)。 @endif */
    METHOD_ADJ_R_END_V2, /*!< @if Eng Channel 1 MHz frequency hopping dynamic r algorithm
                                (version2, upper limit of ranging: 150m).
                               @else   信道1MHz跳频动态r算法(版本2，测距上限150m)。 @endif */
    METHOD_ADJ_R_END_V3         /*!< @if Eng Channel 1 MHz frequency hopping dynamic r algorithm
                             (version3, upper limit of ranging: 150m).
                                      @else   信道1MHz跳频动态r算法(版本3，测距上限150m)。 @endif */
} DisAlgType;

/**
 * @brief Hadm ranging algorithm input parameter IQ data.
 */
typedef struct {
    uint16_t iData;
    uint16_t qData;
} algIq;

/**
 * @brief Unified algorithm input parameters
 */
typedef struct {
    uint32_t curTime;
    algIq *iqDut;
    algIq *iqRtd;
    uint32_t iqChnlNum;
    uint32_t tofDut;
    uint32_t tofRtd;
    uint32_t totalCount;
    uint8_t rssiDut;
    uint8_t rssiRtd;
    uint8_t keyId;
    ParaPair paraLimit;
    DisAlgType flagInter;
    uint8_t isMultiTone;
    uint8_t dutIqBitLen;
    uint8_t rtdIqBitLen;
    uint8_t dutSlemChmap[HADM_CHMAP_BYTE_LEN];
    uint8_t rtdSlemChmap[HADM_CHMAP_BYTE_LEN];
    uint16_t localNvOffset;
    uint16_t remoteNvOffset;
    uint16_t localTofOffset;
    uint16_t remoteTofOffset;
} MeasureAlgPara;

/**
 * @if Eng
 * @brief  SLEM error code.
 * @else
 * @brief  SLEM 错误码（非协议相关）,范围是0x8000A400 ~ 0x8000A800。
 * @endif
 */
typedef enum {
    // 公用错误码范围是0x8000A400 ~ 0x8000A450
    ERRCODE_SLEM_SUCCESS = 0,       /*!< @if Eng error code of success.
                                          @else   执行成功错误码。 @endif */
    ERRCODE_SLEM_FAIL = 0x8000A400, /*!< @if Eng error code of configure fail.
                                          @else   配置失败错误码。 @endif */
    ERRCODE_SLEM_MEMCPY_FAIL,       /*!< @if Eng error code of memcpy fail.
                                          @else   拷贝失败错误码。 @endif */
    ERRCODE_SLEM_MALLOC_FAIL,       /*!< @if Eng error code of malloc fail.
                                          @else   内存申请失败错误码。 @endif */

    ERRCODE_SLEM_ZERO_FAIL,       /*!< @if Eng error code of zero value.
                                         @else   0值错误码。 @endif */
    // 测距错误码范围是0x8000A450 ~ 0x8000A550
    ERRCODE_SLEM_RSSI_ABNORMAL = 0x8000A450, /*!< @if Eng error code of rssi abnormal.
                                        @else   RSSI异常。 @endif */
    ERRCODE_SLEM_MARIX_INV_FAIL,             /*!< @if Eng error code of matrix inverse fail.
                                                   @else   矩阵求逆失败错误码。 @endif */

    ERRCODE_SLEM_TOF_IQ_NOTMATCH, /*!< @if Eng The ToF distance is much greater than
                                                the IQ distance.
                                        @else   ToF测距值远大于IQ测距值。 @endif */
    ERRCODE_SLEM_IQ_LOW_ENERGY,   /*!< @if Eng The IQ energy is too low.
                                        @else   IQ能量幅度过低，难以计算距离。 @endif */
    ERRCODE_SLEM_TOA_ABNORMAL,             /*!< @if EstToA is lower than the LowerBound
                                        @else  估计的ToA值小于门限，判定为异常 @endif */

    // 定位错误码范围是0x8000A550 ~ 0x8000A650
    ERRCODE_SLEM_NOTRIGGER = 0x8000A550, /*!< @if Eng hint of no trigger of ALGORITHM
                                                    @else   上层算法未触发提示 @endif */
    ERRCODE_SLEM_POS_FAIL,               /*!< @if Eng error code of position algorithm fail.
                                 @else   定位失败码。 @endif */
    // 开关门错误码范围是0x8000A650 ~ 0x8000A725
    ERRCODE_SLEM_FUSION_FAIL = 0x8000A650, /*!< @if Eng error code of lock and unlock algorithm fail
                                                            @else   开关门算法失败码 @endif */
    // 车内外错误码范围是0x8000A725 ~ 0x8000A800
    ERRCODE_SLEM_CAR_IN_OUT_FAIL = 0x8000A725, /*!< @if Eng error code of car in out algorithm fail
                                                           @else   开关门算法失败码 @endif */
} errcode_slem;

} // namespace Nearlink
} // namespace OHOS

#endif
