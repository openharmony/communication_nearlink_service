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
#ifndef ACTM_SSAP_H
#define ACTM_SSAP_H

#include <stdint.h>
#include "sdf_addr.h"
#include "actm_tbl.h"
#include "nlstk_ssap_app_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum : uint16_t {
    AUDIO_STREAM_MANAGEMENT_UUID = 0x0605,
    AUDIO_PUBLIC_PROPERTY_UUID = 0x0606,
} ActmServiceUuid_E;

typedef enum : uint16_t {
    AUDIO_SOURCE_POINT_UUID = 0x1015,
    AUDIO_SINK_POINT_UUID = 0x1016,
    AUDIO_CONTROL_POINT_UUID = 0x1017,
    AUDIO_STREAM_STATE_CHANGE_UUID = 0x1018,

    AUDIO_SOURCE_PROPERTY_UUID = 0x1019,
    AUDIO_SOURCE_ABILITY_UUID = 0x101A,
    AUDIO_SINK_PROPERTY_UUID = 0x101F,
    AUDIO_SINK_ABILITY_UUID = 0x1020,
    AUDIO_SOURCE_STREAM_TYPE_UUID = 0x1077,
    AUDIO_SINK_STREAM_TYPE_UUID = 0x1078,
    AUDIO_SOURCE_LOCATION_UUID = 0x101B,
    AUDIO_SINK_LOCATION_UUID = 0x1021,
} ActmPropertyUuid_E;

typedef struct {
    uint8_t pointId;
    uint16_t dataLen;
    uint8_t *data;
} ActmPointControlInfo_S;

typedef struct {
    int32_t appId;
    uint16_t handle;
    uint8_t opcode;
    uint8_t pointNum;
    ActmPointControlInfo_S param[0];
} ActmControlReq_S;

void ActmControlReqBySsap(ActmControlReq_S *req);

bool ActmGetService(ActmRemoteDevice_S *device);

void ActmGetSsapCb(NLSTK_SsapAppClientCb_S *cb);

#ifdef __cplusplus
}
#endif
#endif