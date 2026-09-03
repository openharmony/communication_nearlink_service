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

#include "cm_signaling_manage.h"

#include "cm_log.h"
#include "cm_errno.h"
#include "cm_signaling_cap.h"
#include "cm_signaling_trans_channel.h"
#include "cp_worker.h"
#include "sdf_errno_base.h"
#include "sdf_mem.h"
#include "sdf_map.h"
#include "sdf_timer.h"

// dtap default tcid : TCID_SLE_CMTC 0x02
#define CM_MANAGE_DEFAULT_TCID 0x02

#define CM_SIGNALING_MAX_ID 255U
#define CM_SIGNALING_DEFAULT_TIMEOUT_MS 21000  // 21s，大于帧1和帧4可能的丢包引起断链的超时时间supervisionTimeout

#define CM_INVALID_TIMER_HANDLE (-1)

typedef struct CM_CachedSignaling_S {
    uint16_t lcid;
    uint8_t id;
    uint8_t code;
    void *args;
    int timer;
    CM_SignalingTimeoutCbk cbk;
} CM_CachedSignaling_S;

static CM_Signaling_S g_signalings[] = {
    { CAPABILITY_REQ, CAPABILITY_REQ, CM_ProcessReqSignalingCapability },
    { CAPABILITY_RSP, CAPABILITY_REQ, CM_ProcessRspSignalingCapability },
    { TC_CONNECT_REQ, TC_CONNECT_REQ, CM_SignalingTransChanEstablishReqProc },
    { TC_CONNECT_RSP, TC_CONNECT_REQ, CM_SignalingTransChanEstablishRspProc },
    { TC_DISCONNECT_REQ, TC_DISCONNECT_REQ, CM_SignalingTransChanReleaseReqProc },
    { TC_DISCONNECT_RSP, TC_DISCONNECT_REQ, CM_SignalingTransChanReleaseRspProc },
};

static CM_SendSignalingDataCbk g_sendFunc = NULL;

#define CM_MANAGE_NUM (sizeof(g_signalings) / sizeof(CM_Signaling_S))

static SDF_Map *g_cachedSignalings = NULL;  // 缓存的请求信令map。key: lcid，value: 单条逻辑链路缓存信令

static uint8_t g_identifier = 0;

// 获取信令id。跳过仍在缓存map中的id，避免回绕后与未决信令冲突导致插入失败。
// 若256个id全部被信令占用（异常高负载），返回false由调用方处理。
bool CM_GetIdentifier(uint8_t *id)
{
    if (id == NULL || g_cachedSignalings == NULL) {
        return false;
    }

    for (uint16_t i = 0; i <= CM_SIGNALING_MAX_ID; i++) {
        uint8_t candidate = g_identifier++;
        if (SDF_MapFind(g_cachedSignalings, &candidate) == NULL) {
            *id = candidate;
            return true;
        }
    }
    // 遍历一圈全部占用，说明21s内挂起256条信令，属异常高负载。循环恰好自增256次，
    // g_identifier经回绕回到进入时的值，等效于未消费任何id，后续调用可随缓存释放恢复正常。
    CM_LOGE("no available signaling id, all ids are in use");
    return false;
}

void CM_SignalingCacheClearByLcid(uint16_t lcid)
{
    if (g_cachedSignalings == NULL) {
        CM_LOGE("signalings cache map is null.");
        return;
    }

    SDF_MapIter *cur = g_cachedSignalings->entry;
    while (cur != NULL) {
        CM_CachedSignaling_S *cache = (CM_CachedSignaling_S *)cur->val;
        if (cache == NULL || cache->lcid != lcid) {
            cur = cur->next;
            continue;
        }

        SDF_MapIter *next = cur->next;
        (void)SDF_MapErase(g_cachedSignalings, &cache->id);
        cur = next;
    }
}

static void CM_SignalingTimeoutCbkInner(void *args)
{
    CM_CachedSignaling_S *cache = (CM_CachedSignaling_S *)args;
    if (cache == NULL) {
        return;
    }

    CM_LOGE("cm signaling timeout, lcid: 0x%04x, id: %hhu, code: 0x%02x", cache->lcid, cache->id, cache->code);
    if (cache->cbk != NULL) {
        cache->cbk(cache->args);
    }

    // timer为oneshot类型，TimerProc在回调返回后会统一执行SDF_TimerDel收尾；
    // 此处置为无效值，使CM_SignalingCacheDtor跳过删除，避免timer被提前close后
    // TimerProc收尾时对已复用的fd执行误删。
    cache->timer = CM_INVALID_TIMER_HANDLE;
    (void)SDF_MapErase(g_cachedSignalings, &cache->id);
}

uint32_t CM_SignalingCacheInsert(uint16_t lcid, uint8_t id, uint8_t code, void *args, CM_SignalingTimeoutCbk cbk)
{
    uint32_t ret;

    if (g_cachedSignalings == NULL) {
        CM_LOGE("signalings cache map is null.");
        return CM_NULL_POINTER;
    }

    CM_CachedSignaling_S *cache = (CM_CachedSignaling_S *)SDF_MemZalloc(sizeof(CM_CachedSignaling_S));
    if (cache == NULL) {
        CM_LOGE("malloc for cache failed, id: %hhu", id);
        return CM_MEM_ERR;
    }
    cache->lcid = lcid;
    cache->id = id;
    cache->code = code;
    cache->args = args;
    cache->cbk = cbk;

    SDF_TimerParam param = {
        .expires = CM_SIGNALING_DEFAULT_TIMEOUT_MS,
        .period = false,
        .callback = CM_SignalingTimeoutCbkInner,
        .args = cache,
    };
    ret = CP_TimerAdd(&cache->timer, &param);
    if (ret != SDF_OK) {
        CM_LOGE("create timer failed, ret: 0x%08x", ret);
        SDF_MemFree(cache);
        return ret;
    }

    if (!SDF_MapMoveInsert(g_cachedSignalings, &cache->id, cache)) {
        CM_LOGE("add element to cache map failed");
        CP_TimerDel(cache->timer);
        SDF_MemFree(cache);
        return CM_MAP_INSERT_ERR;
    }

    return CM_SUCCESS;
}

bool CM_SignalingCacheRemove(uint16_t lcid, uint8_t id, uint8_t code)
{
    if (g_cachedSignalings == NULL) {
        return false;
    }

    SDF_MapIter *iter = SDF_MapFind(g_cachedSignalings, &id);
    if (iter == NULL) {
        return false;
    }

    CM_CachedSignaling_S *cache = (CM_CachedSignaling_S *)iter->val;
    if (cache == NULL) {
        return false;
    }

    if (cache->lcid != lcid) {
        CM_LOGE("cache lcid: 0x%04x mismatch req lcid:0x%04x, id: %u", cache->lcid, lcid, id);
        return false;
    }

    if (cache->code != code) {
        CM_LOGE("cache code: 0x%02x mismatch req code:0x%02x, id: %u", cache->code, code, id);
        return false;
    }

    SDF_MapErase(g_cachedSignalings, &id);
    return true;
}

static void CM_SignalingDoNothing(void *arg) {}
static inline int CM_SignalingCacheCompare(const void *lhs_, const void *rhs_)
{
    uint8_t lhs = *(uint8_t *)lhs_;
    uint8_t rhs = *(uint8_t *)rhs_;
    return lhs - rhs;
}

static void CM_SignalingCacheDtor(void *args)
{
    CM_CachedSignaling_S *cache = (CM_CachedSignaling_S *)args;
    if (cache == NULL) {
        return;
    }

    if (cache->args != NULL) {
        SDF_MemFree(cache->args);
    }

    if (cache->timer != CM_INVALID_TIMER_HANDLE) {
        CP_TimerDel(cache->timer);
    }

    SDF_MemFree(cache);
}

uint32_t CM_SignalingCacheInit(void)
{
    SDF_Traits keyTraits = {
        .dtor = CM_SignalingDoNothing,
        .cmptor = CM_SignalingCacheCompare,
    };
    SDF_Traits valTraits = {
        .dtor = CM_SignalingCacheDtor,
        .cmptor = NULL,
    };

    g_cachedSignalings = SDF_MapCtor(keyTraits, valTraits);
    if (g_cachedSignalings == NULL) {
        CM_LOGE("create signaling cache map fail.");
        return CM_MEM_ERR;
    }

    return CM_SUCCESS;
}

void CM_SignalingCacheDeinit(void)
{
    if (g_cachedSignalings == NULL) {
        return;
    }

    SDF_MapDtor(g_cachedSignalings);
    g_cachedSignalings = NULL;
}

const CM_Signaling_S *CM_SignalingGet(uint8_t code)
{
    uint32_t idx;
    for (idx = 0; idx < CM_MANAGE_NUM; idx++) {
        if (g_signalings[idx].recvCode == code) {
            return &g_signalings[idx];
        }
    }
    return NULL;
}

void CM_SignalingRegisterCbk(CM_SendSignalingDataCbk sendFunc)
{
    g_sendFunc = sendFunc;
    CM_LOGI("CM_SignalingRegisterCbk compelte");
}

uint32_t CM_SendBuffToDtap(uint16_t lcid, SDF_Buff_S *buff)
{
    CM_CHECK_RETURN_RET(g_sendFunc != NULL, CM_INVALID_PARAM_ERR, "g_sendFunc is null");
    return g_sendFunc(0, CM_MANAGE_DEFAULT_TCID, lcid, buff);
}
