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
#ifndef ACTM_TBL_H
#define ACTM_TBL_H

#include "actm_api_type.h"
#include "sdf_addr.h"
#include "sdf_vector.h"
#include "sdf_dlist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_POINT_ID 0x0
#define INVALID_STREAM_ID 0xFF

typedef struct {
    uint8_t pointId; // 音频访问点标识
    uint8_t propId;  // 音频属性组标识
    uint8_t type;  // 访问点类型，参考NLSTK_ActmPointType_E
    bool used;  // 访问点是否被使用
} ActmAccessPoint_S;

typedef struct {
    uint8_t propId;
    uint8_t propType;
    uint16_t abilityHandle;
    NLSTK_ActmAbility_S ability;
    uint16_t acceptTypeHandle;
    uint32_t acceptType;
} ActmProp_S;

typedef struct {
    uint16_t ctrlHandle;
    uint16_t eventHandle;
    uint16_t locationHandle;
    uint16_t availableStreamTypeHandle;
} ActmServiceInfo_S;

typedef enum {
    ACTM_SSAP_IDLE,
    ACTM_START_CONFIG,
    ACTM_CONFIG_COMPL,
    ACTM_START_OPEN,
    ACTM_OPEN_COMPL,
    ACTM_START_TRANS,
    ACTM_TRANS_COMPL,
    ACTM_START_PLAY_COMPL,
    ACTM_START_STOP,
    ACTM_STOP_COMPL,
    ACTM_START_RELEASE,
    ACTM_RELEASE_COMPL,
    ACTM_START_FAIL,
} ActmSsapState_E;

typedef enum {
    ACTM_QOSM_IDLE,
    ACTM_START_QOSM,
    ACTM_SET_PARAM,
    ACTM_SET_PARAM_COMPL,
    ACTM_ADD_CONN,
    ACTM_CONN_COMPL,
    ACTM_DEL_CONN,
    ACTM_DEL_COMPL,
    ACTM_QOSM_FAIL,
} ActmQosmState_E;

typedef struct {
    SLE_Addr_S addr;
    int32_t appId;
    uint8_t groupId;
    uint8_t mebId;
    uint8_t curStreamId;
    uint8_t lastErr;
    uint16_t connHandle;
    bool isLeft;
    uint32_t availableStreamType;
    ActmSsapState_E ssapState;
    ActmQosmState_E qosmState;
    ActmServiceInfo_S info;
    SDF_Vector_S *points;
    SDF_Vector_S *props;
    SDF_Vector_S *streams;
    bool isPriv;
} ActmRemoteDevice_S;

typedef struct {
    uint8_t streamId;
    uint8_t pointType;
    uint8_t commType;
    uint8_t srcPointId;
    uint8_t sinkPointId;
    uint8_t qosIndex;
    NLSTK_ActmCodecConfig_S codec;
} ActmStream_S;

typedef struct {
    SDF_DListEntry_S entry;
    ActmRemoteDevice_S device;
} ActmDeviceNode_S;

ActmRemoteDevice_S *ActmFindDeviceByAddr(SLE_Addr_S *addr);

ActmRemoteDevice_S *ActmFindDeviceByAppId(int32_t appId);

uint32_t ActmCreateRemoteDevice(SLE_Addr_S *addr);

void ActmDestroyRemoteDevice(SLE_Addr_S *addr);

ActmAccessPoint_S *ActmFindPointById(ActmRemoteDevice_S *device, uint8_t pointId);

ActmProp_S *ActmFindPropById(ActmRemoteDevice_S *device, uint8_t propId);

ActmProp_S *ActmFindPropByAblHandle(ActmRemoteDevice_S *device, uint32_t handle);

ActmProp_S *ActmFindPropByTypeHandle(ActmRemoteDevice_S *device, uint32_t handle);

ActmStream_S *ActmFindStreamById(ActmRemoteDevice_S *device, uint8_t streamId);

ActmStream_S *ActmCreateStream(ActmRemoteDevice_S *device, uint8_t pointType, uint8_t commType);

void ActmDeleteStream(ActmRemoteDevice_S *device, uint8_t streamId);

uint8_t ActmCountGroupSize(uint8_t groupId);

void ActmTblInit(void);

void ActmCleanTbl(void);

#ifdef __cplusplus
}
#endif
#endif