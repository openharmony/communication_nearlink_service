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
#include <stdlib.h>
#include <stddef.h>
#include "securec.h"
#include "stack_sdf_mem_stub.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

SDF_MemHooks_S g_stMemfunc = { .mAlloc = (SDF_MemAllocHook)malloc, .mFree = (SDF_MemFreeHook)free };

void *SDF_MemAlloc(size_t size)
{
    return g_stMemfunc.mAlloc(size);
}

void *TEST_SDF_MemZalloc(size_t size)
{
    void *ret = NULL;
    uint8_t *ptr = NULL;
    size_t sz = size;

    ret = SDF_MemAlloc(size);
    if (ret == NULL) {
        return NULL;
    }

    ptr = (uint8_t *)ret;
    while (sz > SECUREC_MEM_MAX_LEN) {
        (void)memset_s(ptr, SECUREC_MEM_MAX_LEN, 0, SECUREC_MEM_MAX_LEN);
        ptr += SECUREC_MEM_MAX_LEN;
        sz -= SECUREC_MEM_MAX_LEN;
    }
    if (sz > 0) {
        (void)memset_s(ptr, sz, 0, sz);
    }

    return ret;
}

void* TEST_SDF_MemZallocFail(size_t size) {
    (void)size;
    return NULL;
}

/**
 * @brief     内存释放
 * @param[IN] ptr 内存地址
 * @return    uint32_t
 * @remarks
 */
void SDF_MemFree(void *ptr)
{
    return g_stMemfunc.mFree(ptr);
}

/**
 * @brief     字节对齐内存申请
 * @param[IN] size 内存大小
 * @param[IN] align 内存对齐，如：8B、16B、32B、64B
 * @return    void*
 * @retval    内存地址
 * @remarks
 */
void *SDF_MemAlignAlloc(size_t size, uint8_t align)
{
    uint8_t pad;
    void*   ptr;
    void*   aligned;

    if (align == 0) {
        align = 1;
    }

    /* 预留sizeof(uint8_t)空间，用于存放pad值 */
    ptr = g_stMemfunc.mAlloc(align + sizeof(uint8_t) + size);
    if (ptr == NULL) {
        return NULL;
    }

    pad = align - ((uintptr_t)ptr % align);

    aligned = (void*)((uintptr_t)ptr + pad);

    *(uint8_t*)((uintptr_t)aligned - 1) = pad;

    return aligned;
}

/**
 * @brief     字节对齐内存释放
 * @param[IN] ptr 内存地址
 * @return    void
 * @remarks
 */
void SDF_MemAlignFree(void *ptr)
{
    uint8_t pad;
    void*   pos;

    if (ptr == NULL) {
        return;
    }

    pad = *(uint8_t*)((uintptr_t)ptr - 1);
    pos = (void*)((uintptr_t)ptr - pad);

    g_stMemfunc.mFree(pos);

    return;
}

/**
 * @brief     内存申请释放注册函数
 * @param[IN] pHooks 内存申请释放钩子,不能为NULL
 * @return    uint32_t
 * @retval    成功(SDF_OK)
 * @retval    失败(SDF_ERR)
 * @remarks   该接口可选用,未注册将默认使用glibc;
 */
uint32_t SDF_MemHookReg(SDF_MemHooks_S *pHooks)
{
    if (pHooks == NULL || pHooks->mAlloc == NULL || pHooks->mFree == NULL) {
        return SDF_MEMM_ERROR_INVALID_PARAM;
    }

    g_stMemfunc.mAlloc = pHooks->mAlloc;
    g_stMemfunc.mFree  = pHooks->mFree;

    return SDF_OK;
}


#ifdef __cplusplus
}
#endif