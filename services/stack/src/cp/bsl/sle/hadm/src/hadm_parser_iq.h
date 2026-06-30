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
/*
 * @file hadm_parser_iq.h
 * @brief HADM模块解析DLI上报上来的IQ数据信息
 */

#ifndef HADM_PARSER_IQ_H
#define HADM_PARSER_IQ_H

#include <stdint.h>
#include "hadm_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HADM_MEASURE_IQ_REPORT_DATA_LEN 246
#define HADM_IQ_MAX_CHNL_NUM 316 // 79 * 4 = 316

typedef struct {
    uint16_t iData;
    uint16_t qData;
} __attribute__((packed)) HadmIqValue_S;

typedef struct {
    uint8_t chmap[HADM_MEASURE_PM_BAND_24G_LEN];  // 2.4G,位宽80bits
} __attribute__((packed)) HadmSlemChmap_S;

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
} __attribute__((packed)) HadmSlemInfoType;

typedef struct {
    uint8_t status;
    uint16_t connHandle;
    uint8_t slemIdx;
    HadmSlemInfoType slemInfoType;
    uint32_t timeStampSn;
    uint32_t iqChnlNum;
    HadmSlemChmap_S slemChmap;
    uint32_t tofResult;
    uint8_t rssi;
    uint8_t iqBitLen;
    HadmIqValue_S *iqData;
    uint8_t venderLen;
    uint8_t localId;
    uint8_t remoteId;
    uint8_t isMultiTone;
} __attribute__((packed)) HadmIqInfoFromDli_S;

// 此结构体必须1字节对齐
typedef struct HadmRemoteCsParam {
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
} __attribute__((packed)) HadmRemoteCsParam_S;

HadmIqInfoFromDli_S *HadmPaserIqInfoFromDli(uint8_t *data, size_t dataLen);

void HadmFreeIqInfo(HadmIqInfoFromDli_S *iqInfo);

#ifdef __cplusplus
}
#endif
#endif