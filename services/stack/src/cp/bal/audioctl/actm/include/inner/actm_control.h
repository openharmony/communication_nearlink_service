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
#ifndef ACTM_CONTROL_H
#define ACTM_CONTROL_H

#include <stdint.h>
#include "sdf_addr.h"
#include "actm_tbl.h"
#include "actm_ssap.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CODEC_ID_LEN 5
#define CODEC_PARAM_LENGTH_LEN 1
#define CODEC_PARAM_TYPE_AND_PARAM_LENGTH_LEN 2

typedef enum {
    ACTM_CONTROL_CONFIG = 0x01,
    ACTM_CONTROL_OPEN_PATH,
    ACTM_CONTROL_TRANS,
    ACTM_CONTROL_STOP,
    ACTM_CONTROL_RELEASE,
    ACTM_CONTROL_BITRATE_UPDATE = 0x10,
} ActmControlOpcode_E;

typedef enum {
    ACTM_EVENT_TRANS = 0x01,
    ACTM_EVENT_STOP,
    ACTM_EVENT_RELEASE,
    ACTM_EVENT_BITRATE_UPDATE = 0x10,
} ActmEventOpcode_E;

typedef struct {
    uint8_t pointId;
    uint8_t rescode;
    uint16_t dataLen;
    uint8_t *data;
} ActmPointControlRes_S;

typedef struct {
    int32_t appId;
    uint8_t opcode;
    uint8_t pointNum;
    ActmPointControlRes_S param[0];
} ActmControlRsp_S;

void ActmControlStreamRes(ActmControlRsp_S *rsp);

void ActmReadProp(ActmRemoteDevice_S *device);

void ActmConfigPoint(ActmRemoteDevice_S *device, ActmStream_S *stream, ActmAccessPoint_S *point,
    NLSTK_ActmConfig_S *config, ActmPointControlInfo_S *info);

void ActmOpenStream(ActmRemoteDevice_S *device, ActmStream_S *stream);

void ActmChangeStream(ActmRemoteDevice_S *device, ActmStream_S *stream, uint8_t op);

void ActmReleaseStream(ActmRemoteDevice_S *device, ActmStream_S *stream);

void ActmChangeBitrate(ActmRemoteDevice_S *device, ActmStream_S *stream, uint64_t bitrate);

void NotifyAudioProp(ActmRemoteDevice_S *device);

void ActmReportAvailableStreamType(ActmRemoteDevice_S *device, uint32_t streamType);

void ActmDeviceOffline(ActmRemoteDevice_S *device, bool remove);

#ifdef __cplusplus
}
#endif
#endif