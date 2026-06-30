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
#include "cdsm_api.h"
#include "cdsm_tbl.h"
#include "cdsm.h"
#include "cdsm_control.h"
#include "cpfwk_log.h"
#include "cp_worker.h"
#include "sdf_mem.h"
#include "nlstk_public_define.h"
#include "ssapc_app.h"
#include "securec.h"

#define INVALID_GROUP_ID 0xFFFFFFFF

uint32_t g_createId;
uint32_t g_findId;

typedef struct {
    uint32_t gid;
    uint8_t num;
    SLE_Addr_S addr[0];
} RecoverMeb_S;

typedef struct {
    uint32_t gid;
    SDF_Vector_S *addrList;
} AllAddr_S;

typedef struct {
    uint32_t gid;
    NLSTK_CdsmEventCbk func;
} EventCbk_S;

static uint32_t GenGroupId(void)
{
    uint32_t newId = INVALID_GROUP_ID;
    for (uint32_t i = 0; i < INVALID_GROUP_ID; i++) {
        if (CdsmFindCoopSetById(i) == NULL) {
            newId = i;
            break;
        }
    }

    return newId;
}

static void CdsmCreateSetInner(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    CdsmCoopSet_S *set = CdsmFindCoopSetByAddr(addr);
    if (set != NULL) {
        CP_LOG_DEBUG("[CDSM] already in coop set, gid = 0x%x", set->gid);
        g_createId = set->gid;
        return;
    }
    uint32_t gid = GenGroupId();
    CP_CHECK_LOG_RETURN_VOID(gid != INVALID_GROUP_ID, "[CDSM] no spare id");
    set = CdsmCreateCoopSet(gid, addr);
    CP_CHECK_LOG_RETURN_VOID(set != NULL, "[CDSM] create coop set error");
    CP_LOG_INFO("[CDSM] create set success, gid = 0x%x", set->gid);
    CdsmCoopSetMeb_S meb = {0};
    (void)memcpy_s(&meb.addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    if (!CdsmAddCoopSetMember(set->gid, &meb)) {
        CP_LOG_ERROR("[CDSM] add member into set failed when create set, gid: %d", set->gid);
        CdsmDeleteCoopSet(set->gid);
        return;
    }
    g_createId = gid;
}

uint32_t NLSTK_CdsmCreateSet(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN(addr != NULL, INVALID_GROUP_ID, "[CDSM] addr is null");
    SLE_Addr_S *tempAddr = SDF_MemZalloc(sizeof(SLE_Addr_S));
    CP_CHECK_LOG_RETURN(tempAddr != NULL, INVALID_GROUP_ID, "[CDSM] addr malloc error");
    (void)memcpy_s(tempAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_createId = INVALID_GROUP_ID;
    CP_CHECK_LOG_RETURN(CP_PostTaskBlocked(CdsmCreateSetInner, tempAddr, SDF_MemFree, SEM_ALWAYS_WAIT) == CP_OK,
        INVALID_GROUP_ID, "[CDSM] post task error");
    return g_createId;
}

void CdsmRecoverMebInner(void *arg)
{
    RecoverMeb_S *recover = (RecoverMeb_S *)arg;
    CdsmCoopSet_S *set = CdsmFindCoopSetById(recover->gid);
    CP_CHECK_LOG_RETURN_VOID(set != NULL, "[CDSM] not find coop set");
    for (uint8_t i = 0; i < recover->num; i++) {
        CdsmCoopSetMeb_S meb = {0};
        (void)memcpy_s(&meb.addr, sizeof(SLE_Addr_S), &recover->addr[i], sizeof(SLE_Addr_S));
        if (!CdsmAddCoopSetMember(recover->gid, &meb)) {
            CP_LOG_ERROR("[CDSM] add member into set failed when recover meb, gid: %d", recover->gid);
        }
    }
}

void NLSTK_CdsmRecoverMeb(uint32_t gid, uint8_t num, SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[CDSM] addr is null");
    RecoverMeb_S *recover = (RecoverMeb_S *)SDF_MemZalloc(sizeof(RecoverMeb_S) + sizeof(SLE_Addr_S) * num);
    CP_CHECK_LOG_RETURN_VOID(recover != NULL, "[CDSM] recover malloc error");
    recover->gid = gid;
    recover->num = num;
    for (uint8_t i = 0; i < num; i++) {
        (void)memcpy_s(&recover->addr[i], sizeof(SLE_Addr_S), &addr[i], sizeof(SLE_Addr_S));
    }
    CP_CHECK_LOG_RETURN_VOID(CP_PostTask(CdsmRecoverMebInner, recover, SDF_MemFree) == CP_OK,
        "[CDSM] post task error");
}

static void CdsmRemoveSetInner(void *arg)
{
    uint32_t *gid = (uint32_t *)arg;
    CdsmCoopSet_S *coopSet = CdsmFindCoopSetById(*gid);
    CP_CHECK_LOG_RETURN_VOID(coopSet != NULL, "[CDSM] coopSet is null");
    CdsmStopAdv(coopSet);
    for (size_t i = 0; i < coopSet->mebs->size; i++) {
        CdsmCoopSetMeb_S *meb = SDF_VectorElementAt(coopSet->mebs, i);
        if (meb->appId != SSAP_APP_INVALID_ID) {
            SsapcAppRegParam_S param = {.appId = meb->appId};
            SsapcAppDeregister(&param);
            meb->appId = SSAP_APP_INVALID_ID;
        }
    }
    CdsmDeleteCoopSet(*gid);
}

void NLSTK_CdsmRemoveSet(uint32_t gid)
{
    uint32_t *tempGid = (uint32_t *)SDF_MemZalloc(sizeof(uint32_t));
    CP_CHECK_LOG_RETURN_VOID(tempGid != NULL, "[CDSM] tempGid malloc error");
    *tempGid = gid;
    CP_CHECK_LOG_RETURN_VOID(CP_PostTask(CdsmRemoveSetInner, tempGid, SDF_MemFree) == CP_OK,
        "[CDSM] post task error");
}

static void CdsmStartAdvInner(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[CDSM] addr is null");
    CdsmStartAdv(addr);
}

void NLSTK_CdsmStartAdv(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[CDSM] addr is null");
    SLE_Addr_S *tempAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    CP_CHECK_LOG_RETURN_VOID(tempAddr != NULL, "[CDSM] tempAddr is null");
    (void)memcpy_s(tempAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    CP_CHECK_LOG_RETURN_VOID(CP_PostTask(CdsmStartAdvInner, tempAddr, SDF_MemFree) == CP_OK,
        "[CDSM] post task error");
}

static void CdsmStopAdvInner(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[CDSM] addr is null");
    CdsmCoopSet_S *coopSet = CdsmFindCoopSetByAddr(addr);
    CP_CHECK_LOG_RETURN_VOID(coopSet != NULL, "[CDSM] coopSet is null");
    CdsmStopAdv(coopSet);
}

void NLSTK_CdsmStopAdv(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[CDSM] addr is null");
    SLE_Addr_S *tempAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    CP_CHECK_LOG_RETURN_VOID(tempAddr != NULL, "[CDSM] tempAddr is null");
    (void)memcpy_s(tempAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    CP_CHECK_LOG_RETURN_VOID(CP_PostTask(CdsmStopAdvInner, tempAddr, SDF_MemFree) == CP_OK,
        "[CDSM] post task error");
}

static void CdsmConnectInner(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[CDSM] addr is null");
    CdsmCoopSet_S *coopSet = CdsmFindCoopSetByAddr(addr);
    CP_CHECK_LOG_RETURN_VOID(coopSet != NULL, "[CDSM] coopSet is null");
    CdsmCoopSetMeb_S *setMeb = CdsmFindCoopSetMember(coopSet->gid, addr);
    CP_CHECK_LOG_RETURN_VOID(setMeb != NULL, "[CDSM] setMeb is null");
    CP_CHECK_LOG_RETURN_VOID(CdsmRegisterSsapApp(setMeb) == NLSTK_OK, "[CDSM] setMeb register failed");
    CdsmConnectProfile(setMeb);
}

uint32_t NLSTK_CdsmConnect(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[CDSM] addr is null");
    SLE_Addr_S *tempAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    CP_CHECK_LOG_RETURN(tempAddr != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CDSM] tempAddr is null");
    (void)memcpy_s(tempAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    CP_CHECK_LOG_RETURN(CP_PostTask(CdsmConnectInner, tempAddr, SDF_MemFree) == CP_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[CDSM] post task error");
    return NLSTK_ERRCODE_SUCCESS;
}

static void CdsmDisconnectInner(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[CDSM] addr is null");
    CdsmCoopSet_S *coopSet = CdsmFindCoopSetByAddr(addr);
    CP_CHECK_LOG_RETURN_VOID(coopSet != NULL, "[CDSM] coopSet is null");
    CdsmCoopSetMeb_S *setMeb = CdsmFindCoopSetMember(coopSet->gid, addr);
    CP_CHECK_LOG_RETURN_VOID(setMeb != NULL, "[CDSM] setMeb is null");
    NLSTK_SsapClientDisconnect(setMeb->appId);
}

uint32_t NLSTK_CdsmDisconnect(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[CDSM] addr is null");
    SLE_Addr_S *tempAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    CP_CHECK_LOG_RETURN(tempAddr != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[CDSM] tempAddr is null");
    (void)memcpy_s(tempAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    CP_CHECK_LOG_RETURN(CP_PostTask(CdsmDisconnectInner, tempAddr, SDF_MemFree) == CP_OK, NLSTK_ERRCODE_TASK_FAIL,
        "[CDSM] post task error");
    return NLSTK_ERRCODE_SUCCESS;
}

static void CdsmFindGidByAddrInner(void *arg)
{
    SLE_Addr_S *addr = (SLE_Addr_S *)arg;
    CdsmCoopSet_S *set = CdsmFindCoopSetByAddr(addr);
    CP_CHECK_LOG_RETURN_VOID(set != NULL, "[CDSM] not find set");
    g_findId = set->gid;
}

uint32_t NLSTK_CdsmFindGidByAddr(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN(addr != NULL, INVALID_GROUP_ID, "[CDSM] addr is null");
    SLE_Addr_S *tempAddr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
    CP_CHECK_LOG_RETURN(tempAddr != NULL, INVALID_GROUP_ID, "[CDSM] tempAddr is null");
    (void)memcpy_s(tempAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    g_findId = INVALID_GROUP_ID;
    CP_CHECK_LOG_RETURN(CP_PostTaskBlocked(CdsmFindGidByAddrInner, tempAddr, SDF_MemFree,
        SEM_ALWAYS_WAIT) == CP_OK, INVALID_GROUP_ID, "[CDSM] post task error");
    return g_findId;
}

static void CdsmFindAllAddrByGidInner(void *arg)
{
    AllAddr_S *allAddr = (AllAddr_S *)arg;
    CdsmCoopSet_S *set = CdsmFindCoopSetById(allAddr->gid);
    CP_CHECK_LOG_RETURN_VOID(set != NULL, "[CDSM] not find coop set");
    CdsmCoopSetMeb_S *meb = NULL;
    SLE_Addr_S *addr = NULL;
    for (size_t i = 0; i < set->mebs->size; i++) {
        meb = SDF_VectorElementAt(set->mebs, i);
        addr = (SLE_Addr_S *)SDF_MemZalloc(sizeof(SLE_Addr_S));
        if (addr == NULL) {
            CP_LOG_ERROR("[CDSM] addr malloc error");
            return;
        }
        (void)memcpy_s(addr, sizeof(SLE_Addr_S), &meb->addr, sizeof(SLE_Addr_S));
        if (!SDF_VectorEmplaceBack(allAddr->addrList, addr)) {
            CP_LOG_ERROR("[CDSM] addr emplace back allAddrList error");
            SDF_MemFree(addr);
            return;
        }
    }
}

static void FreeAllAddr(void *arg)
{
    AllAddr_S *allAddr = (AllAddr_S *)arg;
    SDF_DestroyVector(allAddr->addrList);
    SDF_MemFree(allAddr);
}

SDF_Vector_S *NLSTK_CdsmFindAllAddrByGid(uint32_t gid)
{
    AllAddr_S *allAddr = (AllAddr_S *)SDF_MemZalloc(sizeof(AllAddr_S));
    CP_CHECK_LOG_RETURN(allAddr != NULL, NULL, "[CDSM] allAddr is null");
    allAddr->gid = gid;
    SDF_Traits addrTraits = {.dtor = SDF_MemFree};
    allAddr->addrList = SDF_CreateVector(addrTraits);
    if (allAddr->addrList == NULL) {
        CP_LOG_ERROR("[CDSM] addrList create error");
        SDF_MemFree(allAddr);
        return NULL;
    }
    if (CP_PostTaskBlocked(CdsmFindAllAddrByGidInner, allAddr, NULL, SEM_ALWAYS_WAIT) != CP_OK) {
        CP_LOG_ERROR("[CDSM] post task failed");
        CP_PostTask(NULL, allAddr, FreeAllAddr);
        return NULL;
    }
    SDF_Vector_S *addrList = allAddr->addrList;
    SDF_MemFree(allAddr);
    return addrList;
}

static void CdsmRegisterEventCbkInner(void *arg)
{
    NLSTK_CdsmEventCbk cbk = (NLSTK_CdsmEventCbk)arg;
    CdsmRegisterEventCbk(cbk);
}

void NLSTK_CdsmRegisterEventCbk(NLSTK_CdsmEventCbk func)
{
    CP_CHECK_LOG_RETURN_VOID(func != NULL, "[CDSM] func is null");
    CP_CHECK_LOG_RETURN_VOID(CP_PostTask(CdsmRegisterEventCbkInner, func, NULL) == CP_OK,
        "[CDSM] post task error");
}