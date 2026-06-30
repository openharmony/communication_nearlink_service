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
#include "cdsm_tbl.h"
#include "cdsm_event.h"
#include "cdsm.h"
#include "ssapc_app.h"
#include "cpfwk_log.h"
#include "sdf_mem.h"
#include "securec.h"

SDF_DListHead_S g_CoopSetList;

void CdsmSetInit(void)
{
    SDF_DListHeadInit(&g_CoopSetList);
}

static void FreeCdsmCoopSetNode(CdsmCoopSetNode_S *node)
{
    SDF_DestroyVector(node->set.mebs);
    SDF_MemFree(node);
}

void CdsmSetDeInit(void)
{
    CdsmCoopSetNode_S *node = NULL;
    CdsmCoopSetNode_S *tmpNode = NULL;
    SDF_DListElmAllFree(node, tmpNode, &g_CoopSetList, entry, FreeCdsmCoopSetNode);
}

void CdsmClearSet(void)
{
    CdsmCoopSetNode_S *node = NULL;
    CdsmCoopSetNode_S *tmpNode = NULL;
    SDF_DListElmSafeForeach(node, tmpNode, &g_CoopSetList, entry) {
        CdsmCoopSet_S *set = &node->set;
        set->advHandle = INVALID_ADV_HANDLE;
        for (size_t i = 0; i < set->mebs->size; i++) {
            CdsmCoopSetMeb_S *meb = SDF_VectorElementAt(set->mebs, i);
            meb->appId = SSAP_APP_INVALID_ID;
            meb->state = CDSM_DISCONNECTED;
        }
    }
}

static bool CompSetMember(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        return false;
    }
    CdsmCoopSetMeb_S *meb = (CdsmCoopSetMeb_S *)ptr;
    SLE_Addr_S *addr = (SLE_Addr_S *)args;
    return memcmp(&meb->addr, addr, sizeof(SLE_Addr_S)) == 0;
}

CdsmCoopSet_S *CdsmFindCoopSetById(uint32_t gid)
{
    CP_CHECK_LOG_RETURN(!SDF_DListIsEmpty(&g_CoopSetList), NULL, "[CDSM] coop set list is empty");
    CdsmCoopSet_S *set = NULL;
    CdsmCoopSetNode_S *node = NULL;
    SDF_DListElmForeach(node, &g_CoopSetList, entry) {
        if (node->set.gid == gid) {
            set = &node->set;
            break;
        }
    }
    return set;
}

CdsmCoopSet_S *CdsmFindCoopSetByAddr(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN(!SDF_DListIsEmpty(&g_CoopSetList), NULL, "[CDSM] coop set list is empty");
    CdsmCoopSet_S *set = NULL;
    CdsmCoopSetNode_S *node = NULL;
    SDF_DListElmForeach(node, &g_CoopSetList, entry) {
        if (CdsmFindCoopSetMember(node->set.gid, addr) != NULL) {
            set = &node->set;
            break;
        }
    }
    return set;
}

CdsmCoopSet_S *CdsmFindCoopSetByAdvHandle(uint8_t advHandle)
{
    CP_CHECK_LOG_RETURN(!SDF_DListIsEmpty(&g_CoopSetList), NULL, "[CDSM] coop set list is empty");
    CdsmCoopSet_S *set = NULL;
    CdsmCoopSetNode_S *node = NULL;
    SDF_DListElmForeach(node, &g_CoopSetList, entry) {
        if (node->set.advHandle == advHandle) {
            set = &node->set;
            break;
        }
    }
    return set;
}

CdsmCoopSet_S *CdsmCreateCoopSet(uint32_t gid, SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN(CdsmFindCoopSetById(gid) == NULL, NULL, "[CDSM] coop set gid repeat");
    CdsmCoopSetNode_S *node = (CdsmCoopSetNode_S *)SDF_MemZalloc(sizeof(CdsmCoopSetNode_S));
    CP_CHECK_LOG_RETURN(node != NULL, NULL, "[CDSM] node malloc error");
    SDF_DListEntryInit(&node->entry);
    SDF_Traits nodeTraits = {.dtor = SDF_MemFree};
    node->set.mebs = SDF_CreateVector(nodeTraits);
    if (node->set.mebs == NULL) {
        CP_LOG_ERROR("[CDSM] node set mebs malloc error");
        SDF_MemFree(node);
        return NULL;
    }
    node->set.gid = gid;
    node->set.advHandle = INVALID_ADV_HANDLE;
    (void)memcpy_s(&node->set.reportAddr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SDF_DListElmTailInsert(&g_CoopSetList, node, entry);
    return &node->set;
}

void CdsmDeleteCoopSet(uint32_t gid)
{
    CP_CHECK_LOG_RETURN_VOID(!SDF_DListIsEmpty(&g_CoopSetList), "[CDSM] coop set list is empty");
    CdsmCoopSetNode_S *node = NULL;
    CdsmCoopSetNode_S *tmpNode = NULL;
    SDF_DListElmForeach(tmpNode, &g_CoopSetList, entry) {
        if (tmpNode->set.gid == gid) {
            node = tmpNode;
            break;
        }
    }
    CP_CHECK_LOG_RETURN_VOID(node != NULL, "[CDSM] not find coop set");
    SDF_DListElmDel(&g_CoopSetList, node, entry);
    SDF_DestroyVector(node->set.mebs);
    SDF_MemFree(node);
}

bool CdsmAddCoopSetMember(uint32_t gid, CdsmCoopSetMeb_S *meb)
{
    CdsmCoopSet_S *set = CdsmFindCoopSetByAddr(&meb->addr);
    if (set != NULL && set->gid == gid) {
        CP_LOG_ERROR("[CDSM] meb already in coop set, gid = 0x%x", set->gid);
        return false;
    }
    set = CdsmFindCoopSetById(gid);
    CP_CHECK_LOG_RETURN(set != NULL, false, "[CDSM] not find coop set");
    CdsmCoopSetMeb_S *newMeb = (CdsmCoopSetMeb_S *)SDF_MemZalloc(sizeof(CdsmCoopSetMeb_S));
    CP_CHECK_LOG_RETURN(newMeb != NULL, false, "[CDSM] new member malloc error");
    (void)memcpy_s(newMeb, sizeof(CdsmCoopSetMeb_S), meb, sizeof(CdsmCoopSetMeb_S));
    newMeb->appId = SSAP_APP_INVALID_ID;
    newMeb->state = CDSM_DISCONNECTED;
    CP_LOG_INFO("[CDSM] add meb, gid: 0x%x, addr: %s", set->gid, GET_ENC_ADDR(&newMeb->addr));
    if (!SDF_VectorEmplaceBack(set->mebs, newMeb)) {
        CP_LOG_ERROR("[CDSM] add meb fail");
        SDF_MemFree(newMeb);
        return false;
    } else {
        return true;
    }
}

CdsmCoopSetMeb_S *CdsmFindCoopSetMember(uint32_t gid, SLE_Addr_S *addr)
{
    CdsmCoopSet_S *set = CdsmFindCoopSetById(gid);
    CP_CHECK_LOG_RETURN(set != NULL, NULL, "[CDSM] not find coop set");
    size_t index = 0;
    if (!SDF_VectorFindFirst(set->mebs, CompSetMember, addr, &index)) {
        return NULL;
    }
    return (CdsmCoopSetMeb_S *)SDF_VectorElementAt(set->mebs, index);
}

CdsmCoopSetMeb_S *CdsmFindCoopSetMemberByAppId(int32_t appId)
{
    CP_CHECK_LOG_RETURN(!SDF_DListIsEmpty(&g_CoopSetList), NULL, "[CDSM] coop set list is empty");
    CdsmCoopSetNode_S *node = NULL;
    SDF_DListElmForeach(node, &g_CoopSetList, entry) {
        CdsmCoopSet_S *set = &node->set;
        CdsmCoopSetMeb_S *meb = NULL;
        for (size_t i = 0; i < set->mebs->size; i++) {
            meb = SDF_VectorElementAt(set->mebs, i);
            if (meb->appId == appId) {
                return meb;
            }
        }
    }
    return NULL;
}

void CdsmRemoveCoopSetMember(uint32_t gid, SLE_Addr_S *addr)
{
    CdsmCoopSet_S *set = CdsmFindCoopSetById(gid);
    CP_CHECK_LOG_RETURN_VOID(set != NULL, "[CDSM] not find coop set");
    size_t index = 0;
    if (!SDF_VectorFindFirst(set->mebs, CompSetMember, addr, &index)) {
        CP_LOG_ERROR("[CDSM] not find coop set member");
        return;
    }
    CdsmCoopSetMeb_S *meb = (CdsmCoopSetMeb_S *)SDF_VectorElementAt(set->mebs, index);
    if (meb->appId != SSAP_APP_INVALID_ID) {
        SsapcAppRegParam_S param = {.appId = meb->appId};
        SsapcAppDeregister(&param);
    }
    SDF_VectorRemove(set->mebs, index);
}

void CdsmNotifyStateChange(SLE_Addr_S *addr, uint8_t type)
{
    CP_CHECK_LOG_RETURN_VOID(addr != NULL, "[CDSM] addr is null");
    NLSTK_CdsmEventCbk cbk = CdsmGetEventCbk();
    CP_CHECK_LOG_RETURN_VOID(cbk != NULL, "[CDSM] cbk is null");
    CdsmCoopSet_S *set = CdsmFindCoopSetByAddr(addr);
    CP_CHECK_LOG_RETURN_VOID(set != NULL, "[CDSM] not find coop set");
    CP_LOG_INFO("[CDSM] meb state change, state: %u, gid: 0x%x, addr: %s", type, set->gid, GET_ENC_ADDR(addr));
    NLSTK_CdsmEvent_S event = {0};
    (void)memcpy_s(&event.addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    event.gid = set->gid;
    event.type = type;
    event.num = set->num;
    event.memInfo = (NLSTK_CdsmMemInfo_S *)SDF_MemZalloc(set->num * sizeof(NLSTK_CdsmMemInfo_S));
    CP_CHECK_LOG_RETURN_VOID(event.memInfo != NULL, "[CDSM] memInfo malloc error");
    CdsmCoopSetMeb_S *meb = NULL;
    for (size_t i = 0; i < set->mebs->size; i++) {
        meb = SDF_VectorElementAt(set->mebs, i);
        (void)memcpy_s(&event.memInfo[i].addr, sizeof(SLE_Addr_S), &meb->addr, sizeof(SLE_Addr_S));
        if (meb->state == CDSM_CONNECTED) {
            event.memInfo[i].state = CDSM_PROFILE_CONNECT;
        } else {
            event.memInfo[i].state = CDSM_PROFILE_DISCONNECT;
        }
    }
    cbk(&event);
    SDF_MemFree(event.memInfo);
}