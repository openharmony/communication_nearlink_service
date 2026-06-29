/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef NLSTK_SSAPC_CACHE_H
#define NLSTK_SSAPC_CACHE_H

#include "nlstk_ssap_app_client.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SSAPC_SET_CPCD_LEN 2
#define CPCD_ALL_DISABLE          0x0000
#define CPCD_NOTIFICATION_ENABLE  0x0001
#define CPCD_INDICATION_ENABLE    0x0002

typedef struct {
    SLE_Addr_S addr;
    bool servFindFinish;
    bool isByUuid;
    NLSTK_SsapUuid_S curFindUuid;
    SDF_Vector_S *serv;
    SDF_Vector_S *finishedServ;
    SDF_Vector_S *cpcds;
} SsapcCache_S;

typedef struct {
    uint16_t handle;
    uint16_t endHandle;
    NLSTK_SsapUuid_S uuid;
    NLSTK_SsapItemType_E serviceType;
    uint8_t memberValue;
} SsapCacheServInfo_S;

typedef struct {
    SsapCacheServInfo_S structure;
    bool propFindFinish;
    SDF_Vector_S *properties;
    bool methodFindFinish;
    SDF_Vector_S *methods;
    bool eventFindFinish;
    SDF_Vector_S *events;
} SsapCacheServ_S;

typedef struct {
    int32_t appId;
    uint16_t cpcdVal;
} SsapCpcdCfg_S;

typedef struct {
    uint16_t handle;
    SDF_Vector_S *config;
} SsapCacheCpcd_S;

NLSTK_Errcode_E SsapcCacheServ(SLE_Addr_S *addr, SsapCacheServInfo_S *serv);
NLSTK_Errcode_E SsapcCachePrty(SLE_Addr_S *addr, NLSTK_SsapPrty_S *prty);
NLSTK_Errcode_E SsapcCacheMethod(SLE_Addr_S *addr, NLSTK_SsapPrty_S *method);
NLSTK_Errcode_E SsapcCacheEvent(SLE_Addr_S *addr, NLSTK_SsapPrty_S *event);

uint16_t SsapcCacheGetMaxServHandle(SLE_Addr_S *addr);
void SsapcCacheGetNextFindMember(SLE_Addr_S *addr, bool *isFinish, SSAP_FindType_E *findType, uint16_t *startHandle,
    uint16_t *endHandle);
void SsapcCacheServMemberDiscFinish(SLE_Addr_S *addr, SSAP_FindType_E findType, uint16_t handle);
void SsapcCacheServDiscFinish(SLE_Addr_S *addr);
NLSTK_Errcode_E SsapcCacheSortCachedServ(SLE_Addr_S *addr, uint16_t handle);
NLSTK_Errcode_E SsapcCacheIsCurFindByUuid(SLE_Addr_S *addr, bool *isByUuid, NLSTK_SsapUuid_S *uuid);
void SsapcCacheSetCurFindByUuid(SLE_Addr_S *addr, bool isByUuid, NLSTK_SsapUuid_S *uuid);

// 获取的serv需要使用SsapcCacheFreeServices进行释放
NLSTK_Errcode_E SsapcCacheGetServices(SLE_Addr_S *addr, int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_SsapServ_S **serv,
    uint16_t *num);
void SsapcCacheFreeServices(NLSTK_SsapServ_S *serv, uint16_t num);
void SsapcCacheGetUuidByHandle(SLE_Addr_S *addr, NLSTK_SsapUuid_S *uuid, uint16_t handle);

NLSTK_Errcode_E SsapcCacheSetCpcd(int32_t appId, SLE_Addr_S *addr, uint16_t handle, uint16_t cpcdVal);
SDF_Vector_S *SsapcCacheGetCpcdConfig(SLE_Addr_S *addr, uint16_t handle);
void SsapcCacheCleanAppCpcd(int32_t appId, SLE_Addr_S *addr);

NLSTK_Errcode_E SsapcCacheCreate(SLE_Addr_S *addr);
void SsapcCacheDestroy(SLE_Addr_S *addr);
NLSTK_Errcode_E SsapcCacheCleanServ(SLE_Addr_S *addr);

NLSTK_Errcode_E SsapcCacheInit(void);
void SsapcCacheDeInit(void);

#ifdef __cplusplus
}
#endif
#endif