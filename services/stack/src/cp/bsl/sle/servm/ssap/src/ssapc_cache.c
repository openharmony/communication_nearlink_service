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
#include "securec.h"
#include "sdf_vector.h"
#include "sdf_mem.h"
#include "cpfwk_log.h"
#include "ssap_utils.h"
#include "ssapc_app.h"
#include "ssapc_app_util.h"
#include "ssapc_cache.h"

#ifdef __cplusplus
extern "C" {
#endif

static SDF_Vector_S *g_ssapcCache = NULL;

inline static bool CacheCompFunc(void *ptr, void *args)
{
    SsapcCache_S *cache = (SsapcCache_S *)ptr;
    SLE_Addr_S *addr = (SLE_Addr_S *)args;
    return (SDF_CompareSleAddr(&cache->addr, addr) == 0);
}

inline static bool ServInnerCompHandleFunc(void *ptr, void *args)
{
    SsapCacheServ_S *inner = (SsapCacheServ_S *)ptr;
    uint16_t handle = *(uint16_t *)args;
    return handle > inner->structure.handle && handle <= inner->structure.endHandle;
}

inline static bool SortServInnerCompHandleFunc(void *ptr, void *args)
{
    if (ptr == NULL || args == NULL) {
        CP_LOG_ERROR("[SSAP_CACHE] SortServInnerCompHandleFunc input param  is null");
        return false;
    }
    SsapCacheServ_S *inner = (SsapCacheServ_S *)ptr;
    uint16_t handle = *(uint16_t *)args;
    return handle == inner->structure.handle;
}

/**
 * 释放描述符结构体内存
 */
static void FreeCacheDescriptor(NLSTK_SsapDtor_S *dtor, uint16_t num)
{
    (void)num;
    CP_CHECK_LOGD_RETURN_VOID(dtor, "[SSAPC_CACHE] dtor is null");
    SDF_MemFree(dtor);
    dtor = NULL;
}

/**
 * 释放属性结构体内存
 */
static void FreeCacheProperty(NLSTK_SsapPrty_S *prty, uint16_t num)
{
    CP_CHECK_LOG_RETURN_VOID(prty, "[SSAPC_CACHE] prty is null");
    for (int i = 0; i < num; i++) {
        if (prty[i].descriptorNum > 0) {
            FreeCacheDescriptor(prty[i].descriptors, prty[i].descriptorNum);
            prty[i].descriptors = NULL;
        }
    }
    SDF_MemFree(prty);
    prty = NULL;
}

/**
 * 释放服务结构体内存
 */
static void FreeCacheServ(NLSTK_SsapServ_S *serv)
{
    CP_CHECK_LOG_RETURN_VOID(serv, "[SSAP_CACHE] serv is null");
    /* 释放属性内存 */
    if (serv->properties && serv->propertyNum != 0) {
        FreeCacheProperty(serv->properties, serv->propertyNum);
        serv->properties = NULL;
    }
    /* 释放方法内存 */
    if (serv->methods && serv->methodNum != 0) {
        FreeCacheProperty(serv->methods, serv->methodNum);
        serv->methods = NULL;
    }
    /* 释放事件内存 */
    if (serv->events && serv->eventNum != 0) {
        FreeCacheProperty(serv->events, serv->eventNum);
        serv->events = NULL;
    }
}

/**
 * 释放服务发现结构体内存
 */
void SsapcCacheFreeServices(NLSTK_SsapServ_S *serv, uint16_t num)
{
    CP_LOG_DEBUG("[SSAPC_CACHE] enter func SsapcCacheFreeServices");
    CP_CHECK_LOG_RETURN_VOID(serv, "[SSAPC_CACHE] param serv is null");
    for (int i = 0; i < num; i++) {
        FreeCacheServ(&serv[i]);
    }
    /* 释放serv内存 */
    SDF_MemFree(serv);
    serv = NULL;
}

void DestorySsapcCache(void *arg)
{
    if (arg == NULL) {
        return;
    }
    SsapcCache_S *ssapcCache = (SsapcCache_S *)arg;
    if (ssapcCache->serv != NULL) {
        SDF_DestroyVector(ssapcCache->serv);
    }
    if (ssapcCache->finishedServ != NULL) {
        SDF_DestroyVector(ssapcCache->finishedServ);
    }
    if (ssapcCache->cpcds != NULL) {
        SDF_DestroyVector(ssapcCache->cpcds);
    }
    SDF_MemFree(ssapcCache);
}

void FreeCachePropertyInner(void *arg)
{
    if (arg == NULL) {
        return;
    }
    NLSTK_SsapPrty_S *property = (NLSTK_SsapPrty_S *)arg;
    if (property->descriptors != NULL) {
        SDF_MemFree(property->descriptors);
    }
    SDF_MemFree(property);
}

void FreeCacheServiceInner(void *arg)
{
    if (arg == NULL) {
        return;
    }
    SsapCacheServ_S *servInner = (SsapCacheServ_S *)arg;
    if (servInner->properties != NULL) {
        SDF_DestroyVector(servInner->properties);
    }
    if (servInner->methods != NULL) {
        SDF_DestroyVector(servInner->methods);
    }
    if (servInner->events != NULL) {
        SDF_DestroyVector(servInner->events);
    }
    SDF_MemFree(servInner);
}

static SsapCacheServ_S *CreateCacheServInner(void)
{
    SsapCacheServ_S *new = (SsapCacheServ_S *)SDF_MemZalloc(sizeof(SsapCacheServ_S));
    CP_CHECK_LOG_RETURN(new, NULL, "[SSAPC_CACHE] malloc serv fail");
    new->properties = SDF_CreateVector(MAKE_TRAITS(FreeCachePropertyInner, NULL));
    if (new->properties == NULL) {
        FreeCacheServiceInner(new);
        return NULL;
    }
    new->methods = SDF_CreateVector(MAKE_TRAITS(FreeCachePropertyInner, NULL));
    if (new->methods == NULL) {
        FreeCacheServiceInner(new);
        return NULL;
    }
    new->events = SDF_CreateVector(MAKE_TRAITS(FreeCachePropertyInner, NULL));
    if (new->events == NULL) {
        FreeCacheServiceInner(new);
        return NULL;
    }
    return new;
}

/*
 * @brief 创建一个服务发现结构缓存
 */
NLSTK_Errcode_E SsapcCacheServ(SLE_Addr_S *addr, SsapCacheServInfo_S *serv)
{
    CP_LOG_DEBUG("[SSAPC_CACHE] enter func SsapcCacheServ");
    CP_CHECK_LOG_RETURN(addr && serv, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] input param is null");

    size_t idx = 0;
    CP_CHECK_LOG_RETURN(SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx), NLSTK_ERRCODE_FAIL,
        "[SSAPC_CACHE] SsapcCacheServ can not find addr cache, addr: %s", GET_ENC_ADDR(addr));
    SsapcCache_S *ssapcCache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
    for (size_t i = 0; i < ssapcCache->serv->size; i++) {
        SsapCacheServ_S *savedServ = (SsapCacheServ_S *)SDF_VectorElementAt(ssapcCache->serv, i);
        if (savedServ->structure.handle == serv->handle) {
            CP_LOG_ERROR("[SSAPC_CACHE] SsapcCacheServ handle is exist, handle: %d", serv->handle);
            return NLSTK_ERRCODE_FAIL;
        }
    }

    SsapCacheServ_S *servCacheInner = CreateCacheServInner();
    CP_CHECK_LOG_RETURN(servCacheInner, NLSTK_ERRCODE_MALLOC_FAIL, "[SSAPC_CACHE] create serv inner fail");
    (void)memcpy_s(&servCacheInner->structure, sizeof(SsapCacheServInfo_S), serv, sizeof(SsapCacheServInfo_S));
    if ((servCacheInner->structure.memberValue & MEMBER_TYPE_PROPERTY) == 0) {
        servCacheInner->propFindFinish = true;
    }
    if ((servCacheInner->structure.memberValue & MEMBER_TYPE_METHOD) == 0) {
        servCacheInner->methodFindFinish = true;
    }
    if ((servCacheInner->structure.memberValue & MEMBER_TYPE_EVENT) == 0) {
        servCacheInner->eventFindFinish = true;
    }
    if (!SDF_VectorEmplaceBack(ssapcCache->serv, servCacheInner)) {
        CP_LOG_ERROR("[SSAPC_CACHE] serv vector cache push fail");
        FreeCacheServiceInner(servCacheInner);
        return NLSTK_ERRCODE_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E SsapcCachePrty(SLE_Addr_S *addr, NLSTK_SsapPrty_S *prty)
{
    CP_LOG_DEBUG("[SSAPC_CACHE] enter func SsapcCachePrty");
    CP_CHECK_LOG_RETURN(addr && prty, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] input param is null");

    /* 根据该addr设备查找缓存 */
    size_t idx = 0;
    CP_CHECK_LOG_RETURN(SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx), NLSTK_ERRCODE_FAIL,
        "[SSAPC_CACHE] SsapcCacheServ can not find addr cache, addr: %s", GET_ENC_ADDR(addr));
    SsapcCache_S *ssapcCache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);

    /* 根据handle寻找目标服务 */
    CP_CHECK_LOG_RETURN(SDF_VectorFindFirst(ssapcCache->serv, ServInnerCompHandleFunc, &prty->handle, &idx),
        NLSTK_ERRCODE_FAIL, "[SSAPC_CACHE] SsapcCacheServ can not find serv cache, handle: %d", prty->handle);

    SsapCacheServ_S *inner = (SsapCacheServ_S *)SDF_VectorElementAt(ssapcCache->serv, idx);
    NLSTK_SsapPrty_S *newProperty = (NLSTK_SsapPrty_S *)SDF_MemZalloc(sizeof(NLSTK_SsapPrty_S));
    CP_CHECK_LOG_RETURN(newProperty, NLSTK_ERRCODE_MALLOC_FAIL, "[SSAPC_CACHE] cache property malloc fail");
    (void)memcpy_s(newProperty, sizeof(NLSTK_SsapPrty_S), prty, sizeof(NLSTK_SsapPrty_S));
    if (prty->descriptorNum != 0) {
        uint32_t len = sizeof(NLSTK_SsapDtor_S) * prty->descriptorNum;
        newProperty->descriptors = (NLSTK_SsapDtor_S *)SDF_MemZalloc(len);
        if (newProperty->descriptors == NULL) {
            CP_LOG_ERROR("[SSAPC_CACHE] cache property malloc descriptors fail");
            FreeCachePropertyInner(newProperty);
            return NLSTK_ERRCODE_FAIL;
        }
        (void)memcpy_s(newProperty->descriptors, len, prty->descriptors, len);
    }

    if (!SDF_VectorEmplaceBack(inner->properties, newProperty)) {
        CP_LOG_ERROR("[SSAPC_CACHE] enter func SsapcCachePrty");
        FreeCachePropertyInner(newProperty);
        return NLSTK_ERRCODE_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E SsapcCacheMethod(SLE_Addr_S *addr, NLSTK_SsapPrty_S *method)
{
    CP_LOG_DEBUG("[SSAPC_CACHE] enter func SsapcCachePrty");
    CP_CHECK_LOG_RETURN(addr && method, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] input param is null");

    /* 根据该addr设备查找缓存 */
    size_t idx = 0;
    CP_CHECK_LOG_RETURN(SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx), NLSTK_ERRCODE_FAIL,
        "[SSAPC_CACHE] SsapcCacheServ can not find addr cache, addr: %s", GET_ENC_ADDR(addr));
    SsapcCache_S *ssapcCache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);

    /* 根据handle寻找目标服务 */
    CP_CHECK_LOG_RETURN(SDF_VectorFindFirst(ssapcCache->serv, ServInnerCompHandleFunc, &method->handle, &idx),
        NLSTK_ERRCODE_FAIL, "[SSAPC_CACHE] SsapcCacheServ can not find serv cache, handle: %d", method->handle);

    SsapCacheServ_S *inner = (SsapCacheServ_S *)SDF_VectorElementAt(ssapcCache->serv, idx);
    NLSTK_SsapPrty_S *newMethod = (NLSTK_SsapPrty_S *)SDF_MemZalloc(sizeof(NLSTK_SsapPrty_S));
    CP_CHECK_LOG_RETURN(newMethod, NLSTK_ERRCODE_MALLOC_FAIL, "[SSAPC_CACHE] cache property malloc fail");
    (void)memcpy_s(newMethod, sizeof(NLSTK_SsapPrty_S), method, sizeof(NLSTK_SsapPrty_S));
    if (method->descriptorNum != 0) {
        uint32_t len = sizeof(NLSTK_SsapDtor_S) * method->descriptorNum;
        newMethod->descriptors = (NLSTK_SsapDtor_S *)SDF_MemZalloc(len);
        if (newMethod->descriptors == NULL) {
            CP_LOG_ERROR("[SSAPC_CACHE] cache property malloc descriptors fail");
            FreeCachePropertyInner(newMethod);
            return NLSTK_ERRCODE_FAIL;
        }
        (void)memcpy_s(newMethod->descriptors, len, method->descriptors, len);
    }

    if (!SDF_VectorEmplaceBack(inner->methods, newMethod)) {
        CP_LOG_ERROR("[SSAPC_CACHE] enter func SsapcCachePrty");
        FreeCachePropertyInner(newMethod);
        return NLSTK_ERRCODE_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E SsapcCacheEvent(SLE_Addr_S *addr, NLSTK_SsapPrty_S *event)
{
    CP_LOG_DEBUG("[SSAPC_CACHE] enter func SsapcCachePrty");
    CP_CHECK_LOG_RETURN(addr && event, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] input param is null");

    /* 根据该addr设备查找缓存 */
    size_t idx = 0;
    CP_CHECK_LOG_RETURN(SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx), NLSTK_ERRCODE_FAIL,
        "[SSAPC_CACHE] SsapcCacheServ can not find addr cache, addr: %s", GET_ENC_ADDR(addr));
    SsapcCache_S *ssapcCache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);

    /* 根据handle寻找目标服务 */
    CP_CHECK_LOG_RETURN(SDF_VectorFindFirst(ssapcCache->serv, ServInnerCompHandleFunc, &event->handle, &idx),
        NLSTK_ERRCODE_FAIL, "[SSAPC_CACHE] SsapcCacheServ can not find serv cache, handle: %d", event->handle);

    SsapCacheServ_S *inner = (SsapCacheServ_S *)SDF_VectorElementAt(ssapcCache->serv, idx);
    NLSTK_SsapPrty_S *newEvent = (NLSTK_SsapPrty_S *)SDF_MemZalloc(sizeof(NLSTK_SsapPrty_S));
    CP_CHECK_LOG_RETURN(newEvent, NLSTK_ERRCODE_MALLOC_FAIL, "[SSAPC_CACHE] cache property malloc fail");
    (void)memcpy_s(newEvent, sizeof(NLSTK_SsapPrty_S), event, sizeof(NLSTK_SsapPrty_S));
    if (event->descriptorNum != 0) {
        uint32_t len = sizeof(NLSTK_SsapDtor_S) * event->descriptorNum;
        newEvent->descriptors = (NLSTK_SsapDtor_S *)SDF_MemZalloc(len);
        if (newEvent->descriptors == NULL) {
            CP_LOG_ERROR("[SSAPC_CACHE] cache property malloc descriptors fail");
            FreeCachePropertyInner(newEvent);
            return NLSTK_ERRCODE_FAIL;
        }
        (void)memcpy_s(newEvent->descriptors, len, event->descriptors, len);
    }

    if (!SDF_VectorEmplaceBack(inner->events, newEvent)) {
        CP_LOG_ERROR("[SSAPC_CACHE] enter func SsapcCachePrty");
        FreeCachePropertyInner(newEvent);
        return NLSTK_ERRCODE_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint16_t SsapcCacheGetMaxServHandle(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN(addr, 0, "[SSAPC_CACHE] input addr is null");
    size_t idx = 0;
    bool ret = SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx);
    CP_CHECK_LOG_RETURN(ret, 0, "[SSAPC_CACHE] no serv record");
    SsapcCache_S *cache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
    CP_CHECK_LOG_RETURN(cache->serv != NULL, 0, "[SSAPC_CACHE] serv is null");
    uint16_t maxHandle = 0;
    for (idx = 0; idx < cache->serv->size; idx++) {
        SsapCacheServ_S *temp = (SsapCacheServ_S *)SDF_VectorElementAt(cache->serv, idx);
        if (temp->structure.handle > maxHandle) {
            maxHandle = temp->structure.endHandle;
        }
    }
    return maxHandle;
}

static uint16_t SsapcCacheGetMaxMemberHandle(SDF_Vector_S *members, uint16_t startHandle)
{
    if (members == NULL) {
        return startHandle;
    }
    uint16_t maxHandle = startHandle;
    for (size_t i = 0; i < members->size; i++) {
        NLSTK_SsapPrty_S *member = (NLSTK_SsapPrty_S *)SDF_VectorElementAt(members, i);
        if (member->handle > maxHandle) {
            maxHandle = member->handle;
        }
    }
    return maxHandle;
}

static bool SsapcCacheGetNextFindProperty(SsapCacheServ_S *innerServ, SSAP_FindType_E *findType,
    uint16_t *startHandle, uint16_t *endHandle)
{
    if (!innerServ->propFindFinish && (innerServ->structure.memberValue & MEMBER_TYPE_PROPERTY) != 0) {
        uint16_t maxHandle = SsapcCacheGetMaxMemberHandle(innerServ->properties, innerServ->structure.handle);
        if (maxHandle < innerServ->structure.endHandle) {
            *findType = FIND_STRUCTURE_TYPE_PROPERTY;
            *startHandle = maxHandle + 1;
            *endHandle = innerServ->structure.endHandle;
            return true;
        } else {
            innerServ->propFindFinish = true;
            CP_LOG_INFO("[SSAPC_CACHE] property find finish, service uuid: %s",
                SSAP_GET_ENC_UUID(&innerServ->structure.uuid));
        }
    }
    return false;
}

static bool SsapcCacheGetNextFindMethod(SsapCacheServ_S *innerServ, SSAP_FindType_E *findType,
    uint16_t *startHandle, uint16_t *endHandle)
{
    if (!innerServ->methodFindFinish && (innerServ->structure.memberValue & MEMBER_TYPE_METHOD) != 0) {
        uint16_t maxHandle = SsapcCacheGetMaxMemberHandle(innerServ->methods, innerServ->structure.handle);
        if (maxHandle < innerServ->structure.endHandle) {
            *findType = FIND_STRUCTURE_TYPE_METHOD;
            *startHandle = maxHandle + 1;
            *endHandle = innerServ->structure.endHandle;
            return true;
        }
        innerServ->methodFindFinish = true;
        CP_LOG_INFO("[SSAPC_CACHE] method find finish, service uuid: %s",
            SSAP_GET_ENC_UUID(&innerServ->structure.uuid));
    }
    return false;
}

static bool SsapcCacheGetNextFindEvent(SsapCacheServ_S *innerServ, SSAP_FindType_E *findType,
    uint16_t *startHandle, uint16_t *endHandle)
{
    if (!innerServ->eventFindFinish && (innerServ->structure.memberValue & MEMBER_TYPE_EVENT) != 0) {
        uint16_t maxHandle = SsapcCacheGetMaxMemberHandle(innerServ->events, innerServ->structure.handle);
        if (maxHandle < innerServ->structure.endHandle) {
            *findType = FIND_STRUCTURE_TYPE_EVENT;
            *startHandle = maxHandle + 1;
            *endHandle = innerServ->structure.endHandle;
            return true;
        }
        innerServ->eventFindFinish = true;
        CP_LOG_INFO("[SSAPC_CACHE] event find finish, service uuid: %s",
            SSAP_GET_ENC_UUID(&innerServ->structure.uuid));
    }
    return false;
}

void SsapcCacheGetNextFindMember(SLE_Addr_S *addr, bool *isFinish, SSAP_FindType_E *findType, uint16_t *startHandle,
    uint16_t *endHandle)
{
    CP_CHECK_LOG_RETURN_VOID(addr && isFinish && findType && startHandle && endHandle,
        "[SSAPC_CACHE] SsapcCacheGetNextFindMember input param is null");
    *isFinish = false;
    size_t idx = 0;
    bool ret = SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx);
    CP_CHECK_LOG_RETURN_VOID(ret, "[SSAPC_CACHE] no serv record");
    SsapcCache_S *cache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
    CP_CHECK_LOG_RETURN_VOID(cache->serv != NULL, "[SSAPC_CACHE] serv is null");
    for (idx = 0; idx < cache->serv->size; idx++) {
        SsapCacheServ_S *temp = (SsapCacheServ_S *)SDF_VectorElementAt(cache->serv, idx);
        if (SsapcCacheGetNextFindProperty(temp, findType, startHandle, endHandle)) {
            return;
        }
        if (SsapcCacheGetNextFindMethod(temp, findType, startHandle, endHandle)) {
            return;
        }
        if (SsapcCacheGetNextFindEvent(temp, findType, startHandle, endHandle)) {
            return;
        }
    }
    *isFinish = true;
    return;
}

void SsapcCacheServMemberDiscFinish(SLE_Addr_S *addr, SSAP_FindType_E findType, uint16_t handle)
{
    CP_LOG_DEBUG("[SSAPC_CACHE] enter func SsapcCacheServMemberDiscFinish");
    CP_CHECK_LOG_RETURN_VOID(addr, "[SSAPC_CACHE] input param is null");

    /* 根据该addr设备查找缓存 */
    size_t idx = 0;
    CP_CHECK_LOG_RETURN_VOID(SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx),
        "[SSAPC_CACHE] SsapcCacheServMemberDiscFinish can not find addr cache, addr: %s", GET_ENC_ADDR(addr));
    SsapcCache_S *ssapcCache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);

    /* 根据handle寻找目标服务 */
    CP_CHECK_LOG_RETURN_VOID(SDF_VectorFindFirst(ssapcCache->serv, ServInnerCompHandleFunc, &handle, &idx),
        "[SSAPC_CACHE] SsapcCacheServ can not find serv cache, handle: %d", handle);
    
    SsapCacheServ_S *inner = (SsapCacheServ_S *)SDF_VectorElementAt(ssapcCache->serv, idx);
    if (findType == FIND_STRUCTURE_TYPE_PROPERTY) {
        inner->propFindFinish = true;
        CP_LOG_INFO("[SSAPC_CACHE] property find finish, service uuid: %s", SSAP_GET_ENC_UUID(&inner->structure.uuid));
        return;
    }
    if (findType == FIND_STRUCTURE_TYPE_METHOD) {
        inner->methodFindFinish = true;
        CP_LOG_INFO("[SSAPC_CACHE] method find finish, service uuid: %s", SSAP_GET_ENC_UUID(&inner->structure.uuid));
        return;
    }
    if (findType == FIND_STRUCTURE_TYPE_EVENT) {
        inner->eventFindFinish = true;
        CP_LOG_INFO("[SSAPC_CACHE] event find finish, service uuid: %s", SSAP_GET_ENC_UUID(&inner->structure.uuid));
        return;
    }
    return;
}

static bool SsapcCacheServiceCompFunc(void *ptr, void *args)
{
    CP_CHECK_LOG_RETURN(ptr && args, false, "ptr or args is null");

    SsapCacheServ_S *cache = (SsapCacheServ_S *)ptr;
    NLSTK_SsapUuid_S *uuid = (NLSTK_SsapUuid_S *)args;
    return (memcmp(&cache->structure.uuid, uuid, sizeof(NLSTK_SsapUuid_S)) == 0);
}

void SsapcCacheServDiscFinish(SLE_Addr_S *addr)
{
    CP_LOG_DEBUG("[SSAPC_CACHE] enter func SsapcCacheServDiscFinish");
    CP_CHECK_LOG_RETURN_VOID(addr, "[SSAPC_CACHE] input param is null");

    /* 根据该addr设备查找缓存 */
    size_t idx = 0;
    CP_CHECK_LOG_RETURN_VOID(SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx),
        "[SSAPC_CACHE] SsapcCacheServDiscFinish can not find addr cache, addr: %s", GET_ENC_ADDR(addr));
    SsapcCache_S *ssapcCache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
    ssapcCache->servFindFinish = true;

    if (ssapcCache->isByUuid) {
        size_t index = 0;
        size_t startIndex = 0;
        while (SDF_VectorFindFirstByStartIndex(ssapcCache->finishedServ, SsapcCacheServiceCompFunc,
            &ssapcCache->curFindUuid, startIndex, &index)) {
            SDF_VectorRemove(ssapcCache->finishedServ, index);
            startIndex = index;
        }
        while (ssapcCache->serv->size > 0) {
            SsapCacheServ_S *serv =
                (SsapCacheServ_S *)SDF_VectorPopElement(ssapcCache->serv, ssapcCache->serv->size - 1);
            if (serv == NULL) {
                CP_LOG_ERROR("[SSAPC_CACHE] finished serv get failed");
                continue;
            }
            if (!SDF_VectorEmplaceBack(ssapcCache->finishedServ, serv)) {
                CP_LOG_ERROR("[SSAPC_CACHE] finished serv pushed failed");
                FreeCacheServiceInner(serv);
            }
        }
    } else {
        SDF_Vector_S *tempVector = ssapcCache->finishedServ;
        ssapcCache->finishedServ = ssapcCache->serv;
        ssapcCache->serv = tempVector;
        SDF_CleanVector(ssapcCache->serv);
    }
}

static int SortCharacterByHandleCmpFunc(const void *a, const void *b)
{
    if (a == NULL || b == NULL) {
        CP_LOG_ERROR("[SSAPC_CACHE] SortCharacterByHandleCmpFunc input param is null");
        return 0;
    }
    const NLSTK_SsapPrty_S *characterA = *(const NLSTK_SsapPrty_S **)a;
    const NLSTK_SsapPrty_S *characterB = *(const NLSTK_SsapPrty_S **)b;
    return (int)(characterA->handle - characterB->handle);
}

NLSTK_Errcode_E SsapcCacheSortCachedServ(SLE_Addr_S *addr, uint16_t handle)
{
    CP_LOG_INFO("[SSAPC_CACHE] enter func SsapcUpdateCachedServ");
    CP_CHECK_LOG_RETURN(addr, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] input param is null");
    /* 根据该addr设备查找缓存 */
    size_t idx = 0;
    CP_CHECK_LOG_RETURN(SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx), NLSTK_ERRCODE_FAIL,
        "[SSAPC_CACHE] SsapcCacheServ can not find addr cache, addr: %s", GET_ENC_ADDR(addr));
    SsapcCache_S *ssapcCache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);

    /* 根据handle寻找目标服务 */
    CP_CHECK_LOG_RETURN(SDF_VectorFindFirst(ssapcCache->serv, SortServInnerCompHandleFunc, &handle, &idx),
        NLSTK_ERRCODE_FAIL, "[SSAPC_CACHE] SsapcCacheServ can not find serv cache, handle: %d", handle);

    SsapCacheServ_S *serv = (SsapCacheServ_S *)SDF_VectorElementAt(ssapcCache->serv, idx);
    CP_CHECK_LOG_RETURN(serv != NULL, NLSTK_ERRCODE_FAIL, "[SSAPC_CACHE] serv is null");
    uint16_t endHandle = handle;

    /* 对service中的属性/方法/事件按handle升序排序，并计算memberValue，并计算该service中最大的handle即为endHandle */
    if (serv->properties->size > 0) {
        serv->structure.memberValue |= MEMBER_TYPE_PROPERTY;
        SDF_VectorSort(serv->properties, SortCharacterByHandleCmpFunc);
        NLSTK_SsapPrty_S *endProperty = SDF_VectorElementAt(serv->properties, (serv->properties->size - 1));
        if (endProperty->handle > endHandle) {
            endHandle = endProperty->handle;
        }
    }
    if (serv->methods->size > 0) {
        serv->structure.memberValue |= MEMBER_TYPE_METHOD;
        SDF_VectorSort(serv->methods, SortCharacterByHandleCmpFunc);
        NLSTK_SsapPrty_S *endMethod = SDF_VectorElementAt(serv->methods, (serv->methods->size - 1));
        if (endMethod->handle > endHandle) {
            endHandle = endMethod->handle;
        }
    }
    if (serv->events->size > 0) {
        serv->structure.memberValue |= MEMBER_TYPE_EVENT;
        SDF_VectorSort(serv->events, SortCharacterByHandleCmpFunc);
        NLSTK_SsapPrty_S *endEvent = SDF_VectorElementAt(serv->events, (serv->events->size - 1));
        if (endEvent->handle > endHandle) {
            endHandle = endEvent->handle;
        }
    }

    serv->structure.endHandle = endHandle;
    CP_LOG_DEBUG("[SSAPC_CACHE] sorted service, start handle: %d, end handle: %d", serv->structure.handle,
        serv->structure.endHandle);
    return NLSTK_ERRCODE_SUCCESS;
}

static uint16_t CountServiceByUuid(SsapcCache_S *cur, NLSTK_SsapUuid_S *uuid)
{
    size_t count = 0;
    for (size_t i = 0; i < cur->finishedServ->size; i++) {
        SsapCacheServ_S *cache = (SsapCacheServ_S *)SDF_VectorElementAt(cur->finishedServ, i);
        if (memcmp(&cache->structure.uuid, uuid, sizeof(NLSTK_SsapUuid_S)) == 0) {
            count++;
        }
    }
    if (count > UINT16_MAX) {
        CP_LOG_ERROR("[SSAPC_CACHE] service count exceed max num");
        return 0;
    }
    return (uint16_t)count;
}

static NLSTK_Errcode_E CopyServiceMember(NLSTK_SsapPrty_S *dst, NLSTK_SsapPrty_S *src)
{
    (void)memcpy_s(dst, sizeof(NLSTK_SsapPrty_S), src, sizeof(NLSTK_SsapPrty_S));
    if (src->descriptors && src->descriptorNum != 0) {
        dst->descriptors = SDF_MemZalloc(src->descriptorNum * sizeof(NLSTK_SsapDtor_S));
        CP_CHECK_LOG_RETURN(dst->descriptors != NULL, NLSTK_ERRCODE_MALLOC_FAIL,
            "[SSAPC_CACHE] descriptors malloc fail");
        for (uint16_t i = 0; i < src->descriptorNum; i++) {
            dst->descriptors[i].type = src->descriptors[i].type;
        }
    } else {
        dst->descriptors = NULL;
        dst->descriptorNum = 0;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E CopyCacheProperty(NLSTK_SsapServ_S *serv, SsapCacheServ_S *cache)
{
    /* 拷贝属性 */
    if (cache->properties == NULL || cache->properties->size == 0) {
        return NLSTK_ERRCODE_SUCCESS;
    }
    serv->propertyNum = (uint16_t)cache->properties->size;
    serv->properties = SDF_MemZalloc(sizeof(NLSTK_SsapPrty_S) * serv->propertyNum);
    CP_CHECK_LOG_RETURN(serv->properties != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "properties malloc fail");
    for (uint16_t i = 0; i < serv->propertyNum; i++) {
        NLSTK_SsapPrty_S *prty = (NLSTK_SsapPrty_S *)SDF_VectorElementAt(cache->properties, i);
        NLSTK_Errcode_E result = CopyServiceMember(&serv->properties[i], prty);
        if (result != NLSTK_ERRCODE_SUCCESS) {
            return result;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E CopyCacheMethod(NLSTK_SsapServ_S *serv, SsapCacheServ_S *cache)
{
    /* 拷贝方法 */
    if (cache->methods == NULL || cache->methods->size == 0) {
        return NLSTK_ERRCODE_SUCCESS;
    }
    serv->methodNum = (uint16_t)cache->methods->size;
    serv->methods = SDF_MemZalloc(sizeof(NLSTK_SsapPrty_S) * serv->methodNum);
    CP_CHECK_LOG_RETURN(serv->methods != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "methods malloc fail");
    for (uint16_t i = 0; i < serv->methodNum; i++) {
        NLSTK_SsapPrty_S *method = (NLSTK_SsapPrty_S *)SDF_VectorElementAt(cache->methods, i);
        NLSTK_Errcode_E result = CopyServiceMember(&serv->methods[i], method);
        if (result != NLSTK_ERRCODE_SUCCESS) {
            return result;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E CopyCacheEvent(NLSTK_SsapServ_S *serv, SsapCacheServ_S *cache)
{
    /* 拷贝事件 */
    if (cache->events == NULL || cache->events->size == 0) {
        return NLSTK_ERRCODE_SUCCESS;
    }
    serv->eventNum = (uint16_t)cache->events->size;
    serv->events = SDF_MemZalloc(sizeof(NLSTK_SsapPrty_S) * serv->eventNum);
    CP_CHECK_LOG_RETURN(serv->events != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "events malloc fail");
    for (uint16_t i = 0; i < serv->eventNum; i++) {
        NLSTK_SsapPrty_S *event = (NLSTK_SsapPrty_S *)SDF_VectorElementAt(cache->events, i);
        NLSTK_Errcode_E result = CopyServiceMember(&serv->events[i], event);
        if (result != NLSTK_ERRCODE_SUCCESS) {
            return result;
        }
    }
    return NLSTK_ERRCODE_SUCCESS;
}

static NLSTK_Errcode_E ConvertCacheServ(int32_t appId, NLSTK_SsapServ_S *serv, SsapCacheServ_S *cache)
{
    CP_LOG_DEBUG("[SSAPC_CACHE] enter func ConvertCacheServ");
    CP_CHECK_LOG_RETURN(serv != NULL && cache != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] input param is null");
    /* 拷贝服务结构 */
    serv->appId = appId;
    serv->handle = cache->structure.handle;
    serv->uuid = cache->structure.uuid;
    serv->serviceType = cache->structure.serviceType;
    serv->endHandle = cache->structure.endHandle;

    NLSTK_Errcode_E result = CopyCacheProperty(serv, cache);
    if (result != NLSTK_ERRCODE_SUCCESS) {
        return result;
    }
    result = CopyCacheMethod(serv, cache);
    if (result != NLSTK_ERRCODE_SUCCESS) {
        return result;
    }
    result = CopyCacheEvent(serv, cache);
    if (result != NLSTK_ERRCODE_SUCCESS) {
        return result;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E SsapcCacheGetServices(SLE_Addr_S *addr, int32_t appId, NLSTK_SsapUuid_S *uuid, NLSTK_SsapServ_S **serv,
    uint16_t *num)
{
    CP_LOG_DEBUG("[SSAPC_CACHE] enter func SsapcCacheGetServices");
    CP_CHECK_LOG_RETURN(addr && serv && num, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] input param is null");

    size_t idx = 0;
    bool ret = SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx);
    CP_CHECK_LOG_RETURN(ret, NLSTK_ERRCODE_GETRECORD_FAIL, "[SSAPC_CACHE] no serv record");

    SsapcCache_S *cur = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
    CP_CHECK_LOG_RETURN(cur->finishedServ != NULL, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] serv is null");
    uint16_t count = 0;
    if (uuid != NULL) {
        count = CountServiceByUuid(cur, uuid);
    } else {
        CP_CHECK_LOG_RETURN(cur->finishedServ->size <= UINT16_MAX, NLSTK_ERRCODE_FAIL,
            "[SSAPC_CACHE] service count exceed max num");
        count = (uint16_t)cur->finishedServ->size;
    }
    CP_CHECK_LOG_RETURN(count > 0, NLSTK_ERRCODE_FAIL, "[SSAPC_CACHE] no service match");
    NLSTK_SsapServ_S *servInstance = (NLSTK_SsapServ_S *)SDF_MemZalloc(sizeof(NLSTK_SsapServ_S) * count);
    CP_CHECK_LOG_RETURN(servInstance, NLSTK_ERRCODE_MALLOC_FAIL, "[SSAPC_CACHE] malloc serv error");
    uint16_t resNum = 0;
    for (size_t i = 0; i < cur->finishedServ->size; i++) {
        SsapCacheServ_S *cache = (SsapCacheServ_S *)SDF_VectorElementAt(cur->finishedServ, i);
        if (uuid && (memcmp(&cache->structure.uuid, uuid, sizeof(NLSTK_SsapUuid_S)) != 0)) {
            continue;
        }
        if (ConvertCacheServ(appId, &servInstance[resNum], cache) != NLSTK_ERRCODE_SUCCESS) {
            CP_LOG_ERROR("[SSAPC_CACHE] convert serv fail");
            SsapcCacheFreeServices(servInstance, resNum);
            return NLSTK_ERRCODE_FAIL;
        }
        resNum++;
    }
    *num = resNum;
    *serv = servInstance;
    return NLSTK_ERRCODE_SUCCESS;
}

static bool FindUuidInMemberVector(SDF_Vector_S *members, uint16_t handle, NLSTK_SsapUuid_S *uuid)
{
    if (members == NULL) {
        return false;
    }
    for (size_t i = 0; i < members->size; i++) {
        NLSTK_SsapPrty_S *prty = (NLSTK_SsapPrty_S *)SDF_VectorElementAt(members, i);
        if (prty->handle == handle) {
            (void)memcpy_s(uuid, sizeof(NLSTK_SsapUuid_S), &prty->uuid, sizeof(NLSTK_SsapUuid_S));
            return true;
        }
    }
    return false;
}

void SsapcCacheGetUuidByHandle(SLE_Addr_S *addr, NLSTK_SsapUuid_S *uuid, uint16_t handle)
{
    CP_CHECK_LOG_RETURN_VOID(addr && uuid, "[SSAPC_CACHE] input param is null");
    CP_CHECK_LOG_RETURN_VOID(g_ssapcCache, "[SSAPC_CACHE] g_ssapcCache is null");
    size_t idx = 0;
    bool ret = SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx);
    CP_CHECK_LOG_RETURN_VOID(ret, "[SSAPC_CACHE] find serv fail");
    SsapcCache_S *ssapCache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
    CP_CHECK_LOG_RETURN_VOID(ssapCache->finishedServ != NULL, "[SSAPC_CACHE] get serv list fail");

    for (size_t i = 0; i < ssapCache->finishedServ->size; i++) {
        SsapCacheServ_S *serv = (SsapCacheServ_S *)SDF_VectorElementAt(ssapCache->finishedServ, i);
        if (serv->structure.handle == handle) {
            (void)memcpy_s(uuid, sizeof(NLSTK_SsapUuid_S), &serv->structure.uuid, sizeof(NLSTK_SsapUuid_S));
            return;
        }
        if (FindUuidInMemberVector(serv->properties, handle, uuid)) {
            return;
        }
        if (FindUuidInMemberVector(serv->methods, handle, uuid)) {
            return ;
        }
        if (FindUuidInMemberVector(serv->events, handle, uuid)) {
            return;
        }
    }
    (void)memset_s(uuid, sizeof(NLSTK_SsapUuid_S), 0, sizeof(NLSTK_SsapUuid_S));
}

static bool SsapcCacheCpcdHandleCompFunc(void *ptr, void *args)
{
    CP_CHECK_LOG_RETURN(ptr && args, false, "[SSAPC_CACHE] input ptr or args is null");
    SsapCacheCpcd_S *cpcd = (SsapCacheCpcd_S *)ptr;
    uint16_t *handle = (uint16_t *)args;
    return (cpcd->handle == *handle);
}
 
static bool SsapcCacheCpcdAppIdCompFunc(void *ptr, void *args)
{
    CP_CHECK_LOG_RETURN(ptr && args, false, "[SSAPC_CACHE] input ptr or args is null");
    SsapCpcdCfg_S *cpcd = (SsapCpcdCfg_S *)ptr;
    int32_t *appId = (int32_t *)args;
    return (cpcd->appId == *appId);
}

void FreeSsapCpcd(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    SsapCacheCpcd_S *ssapCpcd = (SsapCacheCpcd_S *)ptr;
    if (ssapCpcd->config != NULL) {
        SDF_DestroyVector(ssapCpcd->config);
    }
    SDF_MemFree(ssapCpcd);
}

NLSTK_Errcode_E SsapcCacheSetCpcd(int32_t appId, SLE_Addr_S *addr, uint16_t handle, uint16_t cpcdVal)
{
    CP_LOG_DEBUG("[SSAPC_CACHE] enter func SsapcCacheSetCpcd");
    CP_CHECK_LOG_RETURN(addr, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] input addr is null");
    size_t idx = 0;
    bool ret = SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx);
    CP_CHECK_LOG_RETURN(ret, NLSTK_ERRCODE_FAIL, "[SSAPC_CACHE] find serv fail");
    SsapcCache_S *ssapCache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
    ret = SDF_VectorFindFirst(ssapCache->cpcds, SsapcCacheCpcdHandleCompFunc, &handle, &idx);
    SsapCacheCpcd_S *cpcd = NULL;
    if (!ret) {
        cpcd = (SsapCacheCpcd_S *)SDF_MemZalloc(sizeof(SsapCacheCpcd_S));
        CP_CHECK_LOG_RETURN(cpcd, NLSTK_ERRCODE_MALLOC_FAIL, "[SSAPC_CACHE] malloc cpcd fail");
        cpcd->handle = handle;
        cpcd->config = SDF_CreateVector(MAKE_TRAITS(SDF_MemFree, NULL));
        if (cpcd->config == NULL) {
            SDF_MemFree(cpcd);
            return NLSTK_ERRCODE_FAIL;
        }
        if (!SDF_VectorEmplaceBack(ssapCache->cpcds, cpcd)) {
            SDF_DestroyVector(cpcd->config);
            SDF_MemFree(cpcd);
            return NLSTK_ERRCODE_FAIL;
        }
    } else {
        cpcd = (SsapCacheCpcd_S *)SDF_VectorElementAt(ssapCache->cpcds, idx);
    }

    NLSTK_Errcode_E returnRet = NLSTK_ERRCODE_SUCCESS;
    ret = SDF_VectorFindFirst(cpcd->config, SsapcCacheCpcdAppIdCompFunc, &appId, &idx);
    if (ret) {
        SDF_VectorRemove(cpcd->config, idx);
        returnRet = NLSTK_ERRCODE_DIRECT_RETURN;
    }
    SsapCpcdCfg_S *cfg = (SsapCpcdCfg_S *)SDF_MemZalloc(sizeof(SsapCpcdCfg_S));
    CP_CHECK_LOG_RETURN(cfg, NLSTK_ERRCODE_MALLOC_FAIL, "[SSAPC_CACHE] malloc cfg fail");
    cfg->appId = appId;
    cfg->cpcdVal = cpcdVal;
    if (!SDF_VectorEmplaceBack(cpcd->config, cfg)) {
        SDF_MemFree(cfg);
        return NLSTK_ERRCODE_FAIL;
    }
    if (cpcd->config->size > 1) {
        returnRet = NLSTK_ERRCODE_DIRECT_RETURN;
    }
    return returnRet;
}

SDF_Vector_S *SsapcCacheGetCpcdConfig(SLE_Addr_S *addr, uint16_t handle)
{
    CP_LOG_DEBUG("[SSAPC_CACHE] enter func SsapcCacheGetCpcdConfig");
    CP_CHECK_LOG_RETURN(addr, NULL, "[SSAPC_CACHE] input addr is null");
    size_t idx = 0;
    if (SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx)) {
        SsapcCache_S *cache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
        CP_CHECK_LOG_RETURN(cache->cpcds, NULL, "[SSAPC_CACHE] cpcd is null");
        if (SDF_VectorFindFirst(cache->cpcds, SsapcCacheCpcdHandleCompFunc, &handle, &idx)) {
            SsapCacheCpcd_S *ssapCpcd = (SsapCacheCpcd_S *)SDF_VectorElementAt(cache->cpcds, idx);
            return ssapCpcd->config;
        }
    }
    CP_LOG_INFO("[SSAPC_CACHE] no valid cpcd config");
    return NULL;
}

void SsapcCacheCleanAppCpcd(int32_t appId, SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN_VOID(addr, "[SSAPC_CACHE] input addr is null");
    size_t idx = 0;
    bool ret = SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx);
    CP_CHECK_LOG_RETURN_VOID(ret, "[SSAPC_CACHE] find serv fail");
    SsapcCache_S *ssapCache = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
    for (size_t i = 0; i < ssapCache->cpcds->size; i++) {
        SsapCacheCpcd_S *cpcd = (SsapCacheCpcd_S *)SDF_VectorElementAt(ssapCache->cpcds, i);
        size_t configIdx = 0;
        if (SDF_VectorFindFirst(cpcd->config, SsapcCacheCpcdAppIdCompFunc, &appId, &configIdx)) {
            SDF_VectorRemove(cpcd->config, configIdx);
        }
    }
}

NLSTK_Errcode_E SsapcCacheCreate(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN(addr, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] cache create is null");
    /* 根据该addr设备查找缓存 */
    SsapcCache_S *addrCache = (SsapcCache_S *)SDF_MemZalloc(sizeof(SsapcCache_S));
    CP_CHECK_LOG_RETURN(addrCache, NLSTK_ERRCODE_MALLOC_FAIL, "[SSAPC_CACHE] malloc cache fail");
    (void)memcpy_s(&addrCache->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    addrCache->serv = SDF_CreateVector(MAKE_TRAITS(FreeCacheServiceInner, NULL));
    if (addrCache->serv == NULL) {
        CP_LOG_ERROR("[SSAPC_CACHE] malloc cache serv fail");
        DestorySsapcCache(addrCache);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    addrCache->finishedServ = SDF_CreateVector(MAKE_TRAITS(FreeCacheServiceInner, NULL));
    if (addrCache->finishedServ == NULL) {
        CP_LOG_ERROR("[SSAPC_CACHE] malloc cache finished serv fail");
        DestorySsapcCache(addrCache);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    addrCache->cpcds = SDF_CreateVector(MAKE_TRAITS(FreeSsapCpcd, NULL));
    if (addrCache->cpcds == NULL) {
        CP_LOG_ERROR("[SSAPC_CACHE] malloc cache cpcds fail");
        DestorySsapcCache(addrCache);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    if (!SDF_VectorEmplaceBack(g_ssapcCache, addrCache)) {
        CP_LOG_ERROR("[SSAPC_CACHE] insert vector fail");
        DestorySsapcCache(addrCache);
        return NLSTK_ERRCODE_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

void SsapcCacheDestroy(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN_VOID(addr, "[SSAPC_CACHE] input addr is null");

    size_t idx = 0;
    bool ret = SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx);
    CP_CHECK_LOG_RETURN_VOID(ret, "[SSAPC_CACHE] no serv record");
    SDF_VectorRemove(g_ssapcCache, idx);
    return;
}

NLSTK_Errcode_E SsapcCacheCleanServ(SLE_Addr_S *addr)
{
    CP_CHECK_LOG_RETURN(addr, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] input addr is null");
    size_t idx = 0;
    bool ret = SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx);
    CP_CHECK_LOG_RETURN(ret, NLSTK_ERRCODE_GETRECORD_FAIL, "[SSAPC_CACHE] no serv record");
    SsapcCache_S *cur = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
    SDF_CleanVector(cur->serv);
    cur->isByUuid = false;
    memset_s(&cur->curFindUuid, sizeof(NLSTK_SsapUuid_S), 0, sizeof(NLSTK_SsapUuid_S));
    return NLSTK_ERRCODE_SUCCESS;
}

void SsapcCacheSetCurFindByUuid(SLE_Addr_S *addr, bool isByUuid, NLSTK_SsapUuid_S *uuid)
{
    CP_CHECK_LOG_RETURN_VOID(addr && uuid, "[SSAPC_CACHE] input is null");
    size_t idx = 0;
    bool ret = SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx);
    CP_CHECK_LOG_RETURN_VOID(ret, "[SSAPC_CACHE] no serv record");
    SsapcCache_S *cur = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
    memcpy_s(&cur->curFindUuid, sizeof(NLSTK_SsapUuid_S), uuid, sizeof(NLSTK_SsapUuid_S));
    cur->isByUuid = isByUuid;
}

NLSTK_Errcode_E SsapcCacheIsCurFindByUuid(SLE_Addr_S *addr, bool *isByUuid, NLSTK_SsapUuid_S *uuid)
{
    CP_CHECK_LOG_RETURN(addr && isByUuid && uuid, NLSTK_ERRCODE_POINTER_NULL, "[SSAPC_CACHE] param is null");
    size_t idx = 0;
    bool ret = SDF_VectorFindFirst(g_ssapcCache, CacheCompFunc, addr, &idx);
    CP_CHECK_LOG_RETURN(ret, NLSTK_ERRCODE_GETRECORD_FAIL, "[SSAPC_CACHE] no serv record");
    SsapcCache_S *cur = (SsapcCache_S *)SDF_VectorElementAt(g_ssapcCache, idx);
    *isByUuid = cur->isByUuid;
    if (*isByUuid) {
        (void)memcpy_s(uuid, sizeof(NLSTK_SsapUuid_S), &cur->curFindUuid, sizeof(NLSTK_SsapUuid_S));
    }
    return NLSTK_ERRCODE_SUCCESS;
}

NLSTK_Errcode_E SsapcCacheInit(void)
{
    g_ssapcCache = SDF_CreateVector(MAKE_TRAITS(DestorySsapcCache, NULL));
    CP_CHECK_LOG_RETURN(g_ssapcCache, NLSTK_ERRCODE_MALLOC_FAIL, "[SSAPC_CACHE] create g_ssapcCache fail");
    return NLSTK_ERRCODE_SUCCESS;
}

void SsapcCacheDeInit(void)
{
    SDF_DestroyVector(g_ssapcCache);
    g_ssapcCache = NULL;
}

#ifdef __cplusplus
}
#endif