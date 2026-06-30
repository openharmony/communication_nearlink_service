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
#include "ssapc_app_util.h"
#include "sdf_vector.h"
#include "sdf_mem.h"
#include "cpfwk_log.h"
#include "securec.h"

/**
 * @brief 回调函数管理模块
 */

typedef struct {
    NLSTK_SsapAppClientCb_S cb;
    uint32_t count;
} SsapCbInstance_S;

static SDF_Vector_S *g_cbList = NULL;

static bool SsapcCbCompFunc(void *ptr, void *args)
{
    CP_CHECK_LOG_RETURN(ptr && args, false, "ptr or args is null");

    SsapCbInstance_S *old = (SsapCbInstance_S *)ptr;
    NLSTK_SsapAppClientCb_S *new = (NLSTK_SsapAppClientCb_S *)args;
    return memcmp(&old->cb, new, sizeof(NLSTK_SsapAppClientCb_S)) == 0;
}

NLSTK_SsapAppClientCb_S *SsapcCbGet(NLSTK_SsapAppClientCb_S *cb)
{
    CP_LOG_DEBUG("enter func SsapcCbGet");
    CP_CHECK_LOG_RETURN(cb, NULL, "cb is null");

    if (!g_cbList) {
        SDF_Traits traits = {.dtor = SDF_MemFree};
        g_cbList = SDF_CreateVector(traits);
        CP_CHECK_LOG_RETURN(g_cbList, NULL, "create vector failed");
    }

    size_t index = 0;
    bool ret = SDF_VectorFindFirst(g_cbList, SsapcCbCompFunc, cb, &index);
    SsapCbInstance_S *instance = NULL;
    if (!ret) {
        CP_LOG_DEBUG("not find cb in list");
        instance = (SsapCbInstance_S *)SDF_MemZalloc(sizeof(SsapCbInstance_S));
        CP_CHECK_LOG_RETURN(instance, NULL, "alloc cb failed");
        (void)memcpy_s(&instance->cb, sizeof(NLSTK_SsapAppClientCb_S), cb, sizeof(NLSTK_SsapAppClientCb_S));
        if (!SDF_VectorEmplaceBack(g_cbList, instance)) {
            SDF_MemFree(instance);
            CP_LOG_ERROR("app cb emplace back falied");
            return NULL;
        }
    } else {
        instance = (SsapCbInstance_S *)SDF_VectorElementAt(g_cbList, index);
    }
    instance->count++;
    CP_LOG_DEBUG("the count of Cb which index is %zu is %u, when add cb", index, instance->count);
    return &instance->cb;
}

void SsapcCbDestroy(NLSTK_SsapAppClientCb_S *cb)
{
    CP_LOG_DEBUG("enter func SsapcCbDestroy");
    CP_CHECK_LOG_RETURN_VOID(cb, "cb is null");
    CP_CHECK_LOG_RETURN_VOID(g_cbList, "g_cbList is null");

    size_t index = 0;
    bool ret = SDF_VectorFindFirst(g_cbList, SsapcCbCompFunc, cb, &index);
    CP_CHECK_LOG_RETURN_VOID(ret, "not find cb in list");

    SsapCbInstance_S *instance = (SsapCbInstance_S *)SDF_VectorElementAt(g_cbList, index);
    CP_CHECK_LOG_RETURN_VOID(instance, "instance is null");
    instance->count--;
    CP_LOG_INFO("the count of Cb which index is %zu is %u, when del cb", index, instance->count);
    if (instance->count == 0) {
        SDF_VectorRemove(g_cbList, index);
    }
    return;
}
