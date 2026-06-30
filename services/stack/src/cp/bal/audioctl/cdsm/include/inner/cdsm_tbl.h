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
#ifndef CDSM_TBL_H
#define CDSM_TBL_H

#include "sdf_addr.h"
#include "sdf_vector.h"
#include "sdf_dlist.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CDSM_KEY_INFO_LEN 2
#define CDSM_KEY_LEN 16
#define INVALID_ADV_HANDLE 0xFF

typedef enum {
    CDSM_DISCONNECTED = 0,
    CDSM_ACB_CONNECTING,
    CDSM_ACB_CONNECTED,
    CDSM_READ_KEY_INFO_FINISH,
    CDSM_READ_MEMBER_NUM_FINISH,
    CDSM_READ_MEMBER_ADDR_FINISH,
    CDSM_CONNECTED,
} CdsmMebState_E;

typedef struct {
    SLE_Addr_S addr; // 设备地址
    int32_t appId;   // ssap appId
    uint8_t state;   // 连接状态
} CdsmCoopSetMeb_S;

typedef struct {
    SLE_Addr_S reportAddr;     // 集合地址
    uint32_t gid;              // 集合组id
    uint8_t keyId;             // 密钥标识
    uint8_t algoId;            // 算法标识
    uint8_t key[CDSM_KEY_LEN]; // 集合密钥
    uint8_t num;               // 成员数量
    uint8_t advHandle;         // 邀请广播句柄
    SDF_Vector_S *mebs;        // 集合成员
} CdsmCoopSet_S;

typedef struct {
    SDF_DListEntry_S entry;
    CdsmCoopSet_S set;
} CdsmCoopSetNode_S;

void CdsmSetInit(void);

void CdsmSetDeInit(void);

void CdsmClearSet(void);

CdsmCoopSet_S *CdsmFindCoopSetById(uint32_t gid);

CdsmCoopSet_S *CdsmFindCoopSetByAddr(SLE_Addr_S *addr);

CdsmCoopSet_S *CdsmFindCoopSetByAdvHandle(uint8_t advHandle);

CdsmCoopSet_S *CdsmCreateCoopSet(uint32_t gid, SLE_Addr_S *addr);

void CdsmDeleteCoopSet(uint32_t gid);

bool CdsmAddCoopSetMember(uint32_t gid, CdsmCoopSetMeb_S *meb);

CdsmCoopSetMeb_S *CdsmFindCoopSetMember(uint32_t gid, SLE_Addr_S *addr);

CdsmCoopSetMeb_S *CdsmFindCoopSetMemberByAppId(int32_t appId);

void CdsmRemoveCoopSetMember(uint32_t gid, SLE_Addr_S *addr);

void CdsmNotifyStateChange(SLE_Addr_S *addr, uint8_t type);

#ifdef __cplusplus
}
#endif

#endif