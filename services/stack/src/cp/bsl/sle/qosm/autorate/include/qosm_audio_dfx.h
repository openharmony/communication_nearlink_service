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
 * this file contains dft functions of QOSM module.
 *
 ***************************************************************************/
#ifndef QOSM_AUDIO_DFX_H
#define QOSM_AUDIO_DFX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QOSM_AUDIO_DFX_CONN_CNT 2
#define QOSM_ADDR_LEN 6

struct QOSM_AudioDfxConn {
    uint16_t connHandle;
    uint16_t lcid;
    uint8_t addr[QOSM_ADDR_LEN];
};

struct QOSM_AudioParam {
    uint32_t sduInterval;
    uint32_t ft;
    uint32_t bn;
};

typedef void (*QOSM_AudioDfxDspStatusCb)(bool isOn);
typedef void (*QOSM_AudioDfxFlowCtrlCb)(uint16_t connHandle, bool enterFlowCtrl);

struct QOSM_AudioDfxInfo {
    uint32_t startBitrate;
    uint32_t startBand;
    uint8_t codecType;
    struct QOSM_AudioParam param;
    QOSM_AudioDfxDspStatusCb dspStatusCb;
    QOSM_AudioDfxFlowCtrlCb flowCtrlCb;
};

struct QOSM_AudioDfxChoppyInfo {
    uint16_t connHandle;
    uint16_t txFlushed;
    uint16_t ackRate;
    int8_t rssi;
};

enum {
    QOSM_AUDIO_DFX_BAND_2G,
    QOSM_AUDIO_DFX_BAND_5G_1,
    QOSM_AUDIO_DFX_BAND_5G_2,
    QOSM_AUDIO_DFX_BAND_MAX,
};

enum {
    QOSM_FRAME_TYPE_1,
    QOSM_FRAME_TYPE_2,
    QOSM_FRAME_TYPE_3,
    QOSM_FRAME_TYPE_4,
    QOSM_FRAME_TYPE_MAX,
};

enum {
    QOSM_AUDIO_DFX_CODEC_TYPE_ENCODER,
    QOSM_AUDIO_DFX_CODEC_TYPE_DECODER,
    QOSM_AUDIO_DFX_CODEC_TYPE_BOTH,
};

void QOSM_AudioDfxStart(struct QOSM_AudioDfxInfo *info);
void QOSM_AudioDfxStop(void);
void QOSM_AudioDfxUpdateConn(struct QOSM_AudioDfxConn *conn, bool connected);
void QOSM_AudioDfxUpdateBitrate(uint32_t bitrate);
void QOSM_AudioDfxUpdateBand(uint32_t band);
void QOSM_AudioDfxUpdateFrameType(uint8_t frameType);
void QOSM_AudioDfxNotifyChoppy(struct QOSM_AudioDfxChoppyInfo *info);
void QOSM_AudioDfxUpdatePowerLevel(uint16_t connHandle, uint8_t level);
void QOSM_AudioDfxGetDspStatusInner(void *arg);
uint32_t QOSM_AudioDfxGetDspStatus(bool *started);
void QOSM_AudioDfxQualityReport(uint16_t txFlushed, uint16_t ackrate);

#ifdef __cplusplus
}
#endif
#endif
