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
#include "sdf_mem.h"
#include "sdf_vector.h"
#include "securec.h"
#include "nlstk_log.h"
#include "hadm_api.h"
#include "hadm_link_manager.h"

SDF_Vector_S *g_hadmLinkCbVec = NULL;

static void HadmFreeSoundingCb(void *args)
{
    NLSTK_CHECK_RETURN_VOID(args != NULL, "[HADM]args is null, the cb has not been alloc");
    HadmSoundCb_S *soundingCb = (HadmSoundCb_S *)args;
    if (soundingCb->localIqInfo != NULL) {
        SDF_MemFree(soundingCb->localIqInfo);
        soundingCb->localIqInfo = NULL;
    }
    if (soundingCb->remoteIqInfo != NULL) {
        SDF_MemFree(soundingCb->remoteIqInfo);
        soundingCb->remoteIqInfo = NULL;
    }
    SDF_MemFree(soundingCb);
}

uint32_t HadmInitSoundingCbManager(void)
{
    NLSTK_LOG_INFO("[HADM] HadmInitSoundingCbManager");
    SDF_Traits linkCbTraits = { .dtor = HadmFreeSoundingCb };
    g_hadmLinkCbVec = SDF_CreateVector(linkCbTraits);
    if (g_hadmLinkCbVec == NULL) {
        NLSTK_LOG_ERROR("[SSAP] init service create vector failed");
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void HadmDeInitLinkCbManager(void)
{
    if (g_hadmLinkCbVec != NULL) {
        SDF_DestroyVector(g_hadmLinkCbVec);
        g_hadmLinkCbVec = NULL;
    }
}

static bool HadmComparaLinkAddrs(void *ptr, void *args)
{
    NLSTK_CHECK_RETURN(ptr != NULL && args != NULL, false, "[HADM]the input point is NULL when find the link cb");
    const SLE_Addr_S *addr = (const SLE_Addr_S *)args;
    HadmSoundCb_S *linkCb = (HadmSoundCb_S *)ptr;
    if (SDF_CompareSleAddr(&(linkCb->addr), addr) == 0) {
        return true;
    } else {
        return false;
    }
}

HadmSoundCb_S *HadmGetLinkCb(SLE_Addr_S *addr)
{
    size_t index = 0;
    NLSTK_CHECK_RETURN(addr != NULL, NULL, "[HADM]the input point is NULL when get the link cb in hadm");
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) == true) {
        return SDF_VectorElementAt(g_hadmLinkCbVec, index);
    } else {
        return NULL;
    }
}

HadmSoundCb_S *HadmAllocLinkCb(SLE_Addr_S *addr, uint16_t lcid)
{
    NLSTK_CHECK_RETURN(addr != NULL, NULL, "[HADM]the input point is NULL when alloc the link cb in hadm");
    NLSTK_LOG_INFO("[HADM] alloc link cb, addr is %s, lcid is %d", GET_ENC_ADDR(addr), lcid);
    size_t index = 0;
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) == true) {
        NLSTK_LOG_ERROR("[HADM]the link cb has existed when alloc the link cb in hadm");
        HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
        linkCb->lcid = lcid;
        return linkCb;
    } else {
        HadmSoundCb_S *linkCb = (HadmSoundCb_S *)SDF_MemZalloc(sizeof(HadmSoundCb_S));
        if (linkCb == NULL) {
            NLSTK_LOG_ERROR("[HADM]alloc link cb failed");
            return NULL;
        }
        linkCb->lcid = lcid;
        linkCb->state = HADM_SOUNDING_STATE_IDLE;
        linkCb->nextOp = HADM_USER_INVALID_OPERATE;
        (void)memcpy_s(&(linkCb->addr), sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
        if (SDF_VectorEmplaceBack(g_hadmLinkCbVec, linkCb) == false) {
            NLSTK_LOG_ERROR("[HADM]insert the linkcb to vector fail");
            SDF_MemFree(linkCb);
            return NULL;
        }
        return linkCb;
    }
}

void HadmFreeLinkCb(SLE_Addr_S *addr)
{
    size_t index = 0;
    NLSTK_LOG_INFO("[HADM] free link cb, addr is %s", GET_ENC_ADDR(addr));
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) == true) {
        SDF_VectorRemove(g_hadmLinkCbVec, index);
        return;
    }
    NLSTK_LOG_INFO("[HADM] can not find the link cb when free the link cb, addr is %s", GET_ENC_ADDR(addr));
}

uint16_t HadmGetLcid(SLE_Addr_S *addr)
{
    size_t index = 0;
    NLSTK_CHECK_RETURN(addr != NULL, 0, "[HADM]the input point is NULL when get the lcid by addr.");
    if (!SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index)) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when get the lcid by addr, addr is %s", GET_ENC_ADDR(addr));
        return NLSTK_INVALID_LCID;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    return linkCb->lcid;
}

uint32_t HadmGetAddrsByLcid(uint16_t lcid, SLE_Addr_S *addr)
{
    for (size_t index = 0; index < g_hadmLinkCbVec->size; index++) {
        HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
        if (linkCb == NULL) {
            NLSTK_LOG_ERROR("[HADM] the linkCb is NULL, the index is %d. this should not happened", index);
            continue;
        }
        if (linkCb->lcid == lcid) {
            (void)memcpy_s(addr, sizeof(SLE_Addr_S), &(linkCb->addr), sizeof(SLE_Addr_S));
            return NLSTK_ERRCODE_SUCCESS;
        }
    }
    return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
}

uint32_t HadmSetSoundingState(SLE_Addr_S *addr, HadmSoundingState_E state)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when update the sounding state in hadm");
    size_t index = 0;
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) == true) {
        HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
        NLSTK_LOG_INFO("[HADM] change the sounding state from %d to %d, addr is %s", linkCb->state, state,
                     GET_ENC_ADDR(addr));
        linkCb->state = state;
        return NLSTK_ERRCODE_SUCCESS;
    }
    NLSTK_LOG_ERROR("[HADM] change the sounding state fail, addr is %s, state is %d", GET_ENC_ADDR(addr), state);
    return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
}

HadmSoundingState_E HadmGetSoundingStateByAddr(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, HADM_SOUNDING_STATE_INVALID,
                         "[HADM]the input point is NULL when try to get sounding state in hadm");
    size_t index = 0;
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) == true) {
        HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
        return linkCb->state;
    } else {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when get the sounding state, addr is %s", GET_ENC_ADDR(addr));
        return HADM_SOUNDING_STATE_INVALID;
    }
}

uint32_t HadmCacheLinkRemoteCs(SLE_Addr_S *addr, HadmRemoteCsParam_S *remoteCs)
{
    NLSTK_CHECK_RETURN(addr != NULL && remoteCs != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when set the link remote cs in hadm");
    size_t index = 0;
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) == true) {
        HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
        (void)memcpy_s(&(linkCb->remoteCs), sizeof(HadmRemoteCsParam_S), remoteCs, sizeof(HadmRemoteCsParam_S));
        return NLSTK_ERRCODE_SUCCESS;
    } else {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when set the sounding state, addr is %s", GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
}

uint32_t HadmGetLinkRemoteCs(SLE_Addr_S *addr, HadmRemoteCsParam_S *remoteCs)
{
    NLSTK_CHECK_RETURN(addr != NULL && remoteCs != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when get the link remote cs in hadm");
    size_t index = 0;
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) != true) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when get the remote cs, addr is %s", GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    (void)memcpy_s(remoteCs, sizeof(HadmRemoteCsParam_S), &(linkCb->remoteCs), sizeof(HadmRemoteCsParam_S));
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t HadmCacheConnectionParam(SLE_Addr_S *addr, HadmConnectionParam_S *param)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when set the link connection param in hadm");
    size_t index = 0;
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) != true) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when set link connection param, addr is %s", GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    (void)memcpy_s(&(linkCb->connectionParam), sizeof(HadmConnectionParam_S), param, sizeof(HadmConnectionParam_S));
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t HadmGetConnectionParam(SLE_Addr_S *addr, HadmConnectionParam_S *param)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when get the link connection param in hadm");
    size_t index = 0;
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) != true) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when get link connection param, addr is %s", GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    (void)memcpy_s(param, sizeof(HadmConnectionParam_S), &(linkCb->connectionParam), sizeof(HadmConnectionParam_S));
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t HadmCacheUserOperate(SLE_Addr_S *addr, HadmUserOperate_E operate)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when cache the user operate in hadm");
    size_t index = 0;
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) != true) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when cache the user operate in hadm, addr is %s",
                      GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    linkCb->nextOp = operate;
    return NLSTK_ERRCODE_SUCCESS;
}

HadmUserOperate_E HadmGetUserOperate(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, HADM_USER_INVALID_OPERATE,
                         "[HADM]the input point is NULL when cache the user operate in hadm");
    size_t index = 0;
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) != true) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when cache the user operate in hadm, addr is %s",
                      GET_ENC_ADDR(addr));
        return HADM_USER_INVALID_OPERATE;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    return linkCb->nextOp;
}

uint32_t HadmCacheSoundingParam(SLE_Addr_S *addr, HadmSoundingParam_S *param)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when cache sounding param in hadm");
    size_t index = 0;
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) != true) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when set link connection param, addr is %s", GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    (void)memcpy_s(&(linkCb->soundingParam), sizeof(HadmSoundingParam_S), param, sizeof(HadmSoundingParam_S));
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t HadmGetSoundingParam(SLE_Addr_S *addr, HadmSoundingParam_S *param)
{
    NLSTK_CHECK_RETURN(addr != NULL && param != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when get get sounding param in hadm");
    size_t index = 0;
    if (SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index) != true) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when get get sounding param in hadm, addr is %s",
                      GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    (void)memcpy_s(param, sizeof(HadmSoundingParam_S), &(linkCb->soundingParam), sizeof(HadmSoundingParam_S));
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t HadmCacheRemoteFeatures(SLE_Addr_S *addr, uint8_t peerSupportSounding)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when cache peer features");
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index)) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when getting sounding param in hadm, addr is %s",
                      GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    linkCb->peerSupportSounding = peerSupportSounding;
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t HadmGetRemoteFeatures(SLE_Addr_S *addr, HadmPeerSupportSounding_E *peerSupportSounding)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when cache peer features");
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index)) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when getting sounding param in hadm, addr is %s",
                      GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    *peerSupportSounding = linkCb->peerSupportSounding;
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t HadmCacheLocalIqInfo(SLE_Addr_S *addr, HadmIqInfoFromDli_S *iqInfo)
{
    // 注意iqInfo可以是null，当业务需要释放remoteIq的时候，传入NULL
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when cache local Iq info");
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index)) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when cache local Iq info, addr is %s", GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    if (linkCb->localIqInfo != NULL) {
        HadmFreeIqInfo(linkCb->localIqInfo);  // 先释放原来的
    }
    linkCb->localIqInfo = iqInfo;  // 直接指向iqInfo，iqInfo在上层已经分配好内存，因为这个内存一直都在协议栈中
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t HadmCacheRemoteIqInfo(SLE_Addr_S *addr, HadmIqInfoFromDli_S *iqInfo)
{
    // 注意iqInfo可以是null，当业务需要释放remoteIq的时候，传入NULL
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL,
                         "[HADM]the input point is NULL when cache peer Iq Info");
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index)) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when cache peer Iq Info, addr is %s", GET_ENC_ADDR(addr));
        return NLSTK_HADM_ERRCODE_CAN_NOT_FIND_LINKCB;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    if (linkCb->remoteIqInfo != NULL) {
        HadmFreeIqInfo(linkCb->remoteIqInfo);  // 先释放原来的
    }
    linkCb->remoteIqInfo = iqInfo;  // 直接指向iqInfo，iqInfo在上层已经分配好内存，因为这个内存一直都在协议栈中
    return NLSTK_ERRCODE_SUCCESS;
}

HadmIqInfoFromDli_S *HadmGetLocalIqInfo(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NULL, "[HADM]the input point is NULL when get local Iq info");
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index)) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when get local Iq info, addr is %s", GET_ENC_ADDR(addr));
        return NULL;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    return linkCb->localIqInfo;
}

HadmIqInfoFromDli_S *HadmGetRemoteIqInfo(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NULL, "[HADM]the input point is NULL when get remote Iq info");
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_hadmLinkCbVec, HadmComparaLinkAddrs, addr, &index)) {
        NLSTK_LOG_ERROR("[HADM]can not find the linkcb when get remote Iq info, addr is %s", GET_ENC_ADDR(addr));
        return NULL;
    }
    HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
    return linkCb->remoteIqInfo;
}

static uint8_t HadmGetComputeUserStatus(HadmUserOperate_E nextOp, HadmSoundingState_E state)
{
    if (nextOp == HADM_USER_INVALID_OPERATE) {
        // 当用户没有缓存的时候，根据当前的状态机状态返回对应的状态值，只有当空闲、就绪、停止的时候才是返回Stop；
        if (state == HADM_SOUNDING_STATE_IDLE || state == HADM_SOUNDING_STATE_SOUNDING_READY ||
            state == HADM_SOUNDING_STATE_DISABLE_SOUNDING) {
            return HADM_SOUNDING_STOP;
        } else {
            return HADM_SOUNDING_START;
        }
    } else if (nextOp == HADM_SOUNDING_START) {
        return HADM_SOUNDING_START;
    } else if (nextOp == HADM_USER_STOP_SOUNDING) {
        // 表示用户正在停止，这个地方和产品侧有确认；
        // 仅允许一路测距，如果AddrA正在测距的，而且缓存里面没有停止测距，这时不允许启动addrB的测距；如果AddrA有正在测距的，
        // 但是缓存中有停止测距，这时允许启动addrB的测距；芯片要支持两路(针对两个不同地址的指令)执行交叉执行，确保无影响；
        // 所以这种情况不计入规格；
        return HADM_SOUNDING_STOP;
    }
    return HADM_SOUNDING_STOP;
}

uint32_t HadmGetSoundingNumAndAddr(SLE_Addr_S *addr, uint32_t soundingAddrNum)
{
    uint32_t soundingCbNumber = 0;  // 默认值为0
    uint32_t addrIndex = 0;          // 用于记录当前addr的索引
    for (size_t index = 0; index < g_hadmLinkCbVec->size; index++) {
        HadmSoundCb_S *linkCb = SDF_VectorElementAt(g_hadmLinkCbVec, index);
        if (linkCb == NULL) {
            NLSTK_LOG_ERROR("[HADM] the linkCb is NULL, the index is %d. this should not happened", index);
            continue;
        }
        uint8_t userStatus = HadmGetComputeUserStatus(linkCb->nextOp, linkCb->state);
        if (userStatus == HADM_SOUNDING_STOP) {
            continue;  // 如果是停止测距，直接跳过
        }
        if (addrIndex < soundingAddrNum) {
            (void)memcpy_s(&(addr[addrIndex]), sizeof(SLE_Addr_S), &(linkCb->addr), sizeof(SLE_Addr_S));
            addrIndex++;
        }
        soundingCbNumber++;
    }
    return soundingCbNumber;
}