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
#ifndef ACTM_QOSM_ADAPTER_H
#define ACTM_QOSM_ADAPTER_H

#include <stdbool.h>
#include "actm_tbl.h"
#include "actm_api_type.h"
#include "sdf_dlist.h"
#include "sdf_vector.h"
#include "sdf_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QOSM_INVALID_HANDLE 0xffff

#define QOSINDEX_3 3
#define QOSINDEX_4 4
#define QOSINDEX_5 5

#define GROUP_KEY_LEN 16

typedef enum {
    ACTM_QOSM_DISCONNECTED = 0,
    ACTM_QOSM_CONNECTING,
    ACTM_QOSM_CONNECTED,
} ActmQosmConnState_E;

typedef struct {
    SLE_Addr_S addr;
    bool used;
    uint8_t state;
    uint8_t direction;
    uint16_t connHandle;
    uint16_t bitrate;
} ActmQosmLink_S;

typedef enum {
    ACTM_QOSM_PARAM_DELETE = 0,
    ACTM_QOSM_PARAM_ADDING,
    ACTM_QOSM_PARAM_ADD,
} ActmQosmParamState_E;

typedef struct {
    SDF_DListEntry_S entry;
    uint16_t icgId;
    uint16_t gHandle;
    bool isImg;
    uint8_t direction;
    uint8_t state;
    uint8_t linkCnt;
    SDF_Vector_S *links;
    SDF_Vector_S *devices;
    NLSTK_ActmImgEncpParam_S encp;
} ActmQosmGroup_S;

ActmQosmGroup_S *ActmFindQosmGroupById(uint16_t icgId);

ActmQosmGroup_S *ActmFindQosmGroupByHandle(uint16_t gHandle);

ActmQosmLink_S *ActmFindQosmLinkByHandle(uint16_t icgId, uint16_t connHandle);

ActmQosmLink_S *ActmFindQosmLinkByAddr(uint16_t icgId, SLE_Addr_S* addr);

uint32_t ActmCreateQosmGroup(uint16_t icgId, uint8_t linkCnt);

void ActmQosmAddDevice(uint16_t icgId, SLE_Addr_S *addr);

uint16_t ActmAllocConnHandle(SLE_Addr_S *addr, uint16_t icgId);

uint16_t ActmGetConnHandle(SLE_Addr_S *addr, uint16_t icgId);

void ActmSetQosmParam(ActmRemoteDevice_S *device, ActmStream_S *stream);

void ActmDelQosmParam(uint16_t icgId);

void ActmNotifyConnState(ActmRemoteDevice_S *device);

void ActmNotifyDisconnState(ActmRemoteDevice_S *device);

void ActmSetDataPath(ActmRemoteDevice_S *device, uint8_t direction);

void ActmDelConnection(ActmRemoteDevice_S *device, uint16_t connHandle);

void ActmUpdateBitRate(ActmRemoteDevice_S *device, uint64_t bps);

void ActmRecvAutoRateMsg(ActmRemoteDevice_S *device, uint8_t qosIndex, uint8_t labelId,
    uint8_t msgType, uint32_t result);

void ActmGroupListInit(void);

uint32_t ActmRegisterQosmCbk(void);

uint32_t ActmRegisterSmCbk(void);

void ActmDeregisterQosmCbk(void);

void ActmCleanGroupList(void);

#ifdef __cplusplus
}
#endif
#endif