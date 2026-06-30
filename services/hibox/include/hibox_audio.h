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
#ifndef HIBOX_AUDIO
#define HIBOX_AUDIO

#include "hibox_def.h"

#define AUDIO_COMMON_TYPE_LEN 1
#define HDAP_SET_CODEC_TYPE_LEN 4

#define NEARLINK_AUDIO_TYPE_LEN 4

/* command id of auto pair */
typedef enum {
    CMDID_HDAP_CODEC_REPORT = 1,
    CMDID_HDAP_SET_CODEC = 2,
    CMDID_START_HDAP = 3,
    CMDID_STOP_HDAP,
    CMDID_HDAP_KARAOKE_PARAM,
    CMDID_HDAP_VERSION,
    CMDID_AUDIO_RATE_PARAM,
    CMDID_AUDIO_PROFILE_STATE,
    CMDID_AUDIO_OUTPUT_DATAPATH,
    CMDID_AUDIO_CAP,
    CMDID_NEARLINK_PROFILE_STATE,
    CMDID_NEARLINK_OUTPUT_DATAPATH,
    CMDID_DUAL_REC_PARAM_REPORT = 14,
} HiboxAudioCmd;

/* HDAP TYPEID */
enum {
    HDAP_CODEC_CAP_REQ_TYPE = 0x01,
    HDAP_CODEC_CAP_RSP_TYPE = 0x02,
    HDAP_SET_CODEC_REQ_TYPE = 0x01,
    HDAP_SET_CODEC_RSP_TYPE = 0x02,
    HDAP_START_UPWARD_REQ_TYPE = 0x01,
    HDAP_START_UPWARD_RSP_TYPE = 0x02,
    HDAP_STOP_UPWARD_REQ_TYPE = 0x01,
    HDAP_STOP_UPWARD_RSP_TYPE = 0x02,
    HDAP_KARAOKE_PARAM_REQ_TYPE = 0x01,
    HDAP_KARAOKE_PARAM_RSP_TYPE = 0x02,
    HDAP_REPORT_VERSION_REQ_TYPE = 0x01,
    HDAP_REPORT_VERSION_RSP_TYPE = 0x02,
};

/* Audio TYPEID */
enum {
    AUDIO_CODEC_RATE_AAC_TYPE = 0x01,
    AUDIO_CODEC_RATE_SBC_TYPE = 0x02,
    AUDIO_CODEC_RATE_L2HC_V2_TYPE = 0x03,
    AUDIO_CODEC_RATE_LDAC_TYPE = 0x04,
    AUDIO_A2DP_PROFILE_USED_TYPE = 0x01,
    AUDIO_A2DP_BUSINESS_TYPE = 0x02,
    AUDIO_HFP_PROFILE_USED_TYPE = 0x03,
    AUDIO_HFP_BUSINESS_TYPE = 0x04,
    AUDIO_A2DP_DEVICE_NAME_TYPE = 0x05,
    AUDIO_HFP_DEVICE_NAME_TYPE = 0x06,
    AUDIO_HFP_OUTPUT_DATAPATH_TYPE = 0x01,
    AUDIO_HFP_OUTPUT_BUSINESS_TYPE = 0x02,
    AUDIO_A2DP_OUTPUT_DATAPATH_TYPE = 0x03,
    AUDIO_A2DP_OUTPUT_BUSINESS_TYPE = 0x04,
    NEARLINK_PROFILE_AUDIO_TYPE = 0x01,
    NEARLINK_PROFILE_DEVICE_NAME_TYPE = 0x02,
    NEARLINK_PROFILE_STREAM_SERVICE_TYPE = 0x03,
    NEARLINK_OUTPUT_DATAPATH_AUDIO_TYPE = 0x01,
    NEARLINK_DUAL_REC_REQ_CODEC_INFO_TYPE = 0x01,
    NEARLINK_DUAL_REC_RSP_CODEC_INFO_TYPE = 0x02,
};

/* echo 08 0A type */
enum {
    AUDIO_CAPS_TYPE = 0x01,
    AUDIO_ABS_VOLUME_TYPE = 0x02,
    AUDIO_JOIN_INTERVAL_TYPE = 0x03,
    AUDIO_DDM_INTERVAL_TYPE = 0x04,
    AUDIO_SLE_INTERVAL_TYPE = 0x05,
};

#define AUDIO_ABNORMAL_RSP_LEN 1
#define AUDIO_ABSVOLUME_LEN 1
#define AUDIO_INTERVAL_LEN 2
#define AUDIO_CAP_LEN 4
#define AUDIO_DDM_LATENCY_LEN 6
#define AUDIO_SLE_INTERVAL_LEN 35 // offset (2 字节) + sceneNum (1 字节) + timesMap (MAX_SCENE_NUM * 2 字节)

bool HiboxAudioMgmtReqParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result);
bool HiboxAudioMgmtRspParse(uint8_t cmdId, uint8_t *tlv, uint16_t len, uint8_t *result);
uint16_t HiboxAudioMgmtReqComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg);
uint16_t HiboxAudioMgmtRspComb(uint8_t *buffer, uint16_t bufferLen, HiboxParseMsgInd *arg);

#endif
