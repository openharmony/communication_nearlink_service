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
#include "ssap_handle.h"
#include "securec.h"
#include "sdf_mem.h"
#include "cpfwk_log.h"

SSAP_HandleAllocator_S *g_hdlAlloc = NULL;

// 初始化句柄分配器
bool SSAP_InitHandleAllocator(size_t capacity)
{
    CP_CHECK_LOG_RETURN(g_hdlAlloc == NULL, false, "[SSAP] SSAP_InitHandleAllocator g_hdlAlloc has malloc");
    g_hdlAlloc = (SSAP_HandleAllocator_S *)SDF_MemZalloc(sizeof(SSAP_HandleAllocator_S));
    CP_CHECK_LOG_RETURN(g_hdlAlloc != NULL, false, "[SSAP] SSAP_InitHandleAllocator g_hdlAlloc malloc failed");
    g_hdlAlloc->blocks = (SSAP_HandleRange_S *)SDF_MemZalloc(capacity * sizeof(SSAP_HandleRange_S));
    if (g_hdlAlloc->blocks == NULL) {
        SDF_MemFree(g_hdlAlloc);
        CP_LOG_ERROR("[SSAP] g_hdlAlloc->blocks malloc failed");
        return false;
    }
    g_hdlAlloc->blocks[0].start = SSAP_HANDLE_MIN;
    g_hdlAlloc->blocks[0].end = SSAP_HANDLE_MAX;
    g_hdlAlloc->capacity = capacity;
    g_hdlAlloc->size = 1;
    return true;
}

// 销毁句柄分配器
void SSAP_DestroyHandleAllocator(void)
{
    CP_LOG_INFO("[SSAP] SSAP_DestroyHandleAllocator enter");
    CP_CHECK_LOG_RETURN_VOID(g_hdlAlloc != NULL, "[SSAP] SSAP_HandleAllocator_S g_hdlAlloc is null");
    SDF_MemFree(g_hdlAlloc->blocks);
    g_hdlAlloc->blocks = NULL;
    SDF_MemFree(g_hdlAlloc);
    g_hdlAlloc = NULL;
}

// 分配句柄，首次适应策略
bool SSAP_HandleAllocate(uint16_t size, SSAP_HandleRange_S *result)
{
    CP_CHECK_LOG_RETURN(size != 0, false, "[SSAP] SSAP_HandleAllocate size zero");
    CP_CHECK_LOG_RETURN(size <= (SSAP_HANDLE_MAX - SSAP_HANDLE_MIN + 1), false, "[SSAP] SSAP_HandleAllocate size err");
    CP_CHECK_LOG_RETURN(g_hdlAlloc != NULL, false, "[SSAP] SSAP_HandleAllocate g_hdlAlloc is null");
    CP_CHECK_LOG_RETURN(g_hdlAlloc->blocks != NULL, false, "[SSAP] SSAP_HandleAllocate g_hdlAlloc->blocks is null");

    // 遍历寻找第一个足够大的空闲块
    for (size_t i = 0; i < g_hdlAlloc->size; ++i) {
        uint16_t blockSize = g_hdlAlloc->blocks[i].end - g_hdlAlloc->blocks[i].start + 1;
        
        if (blockSize > size) {
            // 分配从块头部开始
            result->start = g_hdlAlloc->blocks[i].start;
            result->end = g_hdlAlloc->blocks[i].start + size - 1;
            // 部分占用，调整起始位置
            g_hdlAlloc->blocks[i].start += size;
            return true;
        }
        // 更新或移除空闲块
        if (blockSize == size) {
            result->start = g_hdlAlloc->blocks[i].start;
            result->end = g_hdlAlloc->blocks[i].start + size - 1;
            if (g_hdlAlloc->size - i - 1 == 0) {
                g_hdlAlloc->size--;
                return true;
            }
            // 完全占用，移除该块
            if (memmove_s(&g_hdlAlloc->blocks[i], (g_hdlAlloc->size - i - 1) * sizeof(SSAP_HandleRange_S),
                &g_hdlAlloc->blocks[i + 1], (g_hdlAlloc->size - i - 1) * sizeof(SSAP_HandleRange_S)) != EOK) {
                CP_LOG_ERROR("[SSAP] SSAP_HandleAllocate memmove_s failed");
                return false;
            }
            g_hdlAlloc->size--;
            return true;
        }
    }
    return false; // 没有足够大的空闲块
}

static bool SSAP_HandleAllocatorGrow(void)
{
    size_t copySize = g_hdlAlloc->capacity;
    g_hdlAlloc->capacity *= SSAP_HANDLE_CAPACITY_GROW;
    SSAP_HandleRange_S *newBlocks = (SSAP_HandleRange_S*)SDF_MemZalloc(g_hdlAlloc->capacity *
    sizeof(SSAP_HandleRange_S));
    CP_CHECK_LOG_RETURN(newBlocks != NULL, false, "[SSAP] newBlocks malloc failed");
    (void)memcpy_s(newBlocks, copySize * sizeof(SSAP_HandleRange_S), g_hdlAlloc->blocks,
        copySize * sizeof(SSAP_HandleRange_S));
    SDF_MemFree(g_hdlAlloc->blocks);
    g_hdlAlloc->blocks = newBlocks;
    return true;
}

// 释放句柄，注意区间合并
bool SSAP_HandleRelease(SSAP_HandleRange_S range)
{
    CP_CHECK_LOG_RETURN(range.start <= range.end, false, "[SSAP] SSAP_HandleRelease range error");
    CP_CHECK_LOG_RETURN(g_hdlAlloc != NULL, false, "[SSAP] SSAP_HandleRelease g_hdlAlloc is null");
    CP_CHECK_LOG_RETURN(g_hdlAlloc->blocks != NULL, false, "[SSAP] SSAP_HandleRelease g_hdlAlloc->blocks is null");
    // 寻找插入位置
    size_t index = 0;
    while (index < g_hdlAlloc->size && g_hdlAlloc->blocks[index].start < range.start) {
        index++;
    }
    // 插入新空闲块
    if (g_hdlAlloc->size == g_hdlAlloc->capacity) {
        // 动态扩容
        if (!SSAP_HandleAllocatorGrow()) {
            CP_LOG_ERROR("[SSAP] SSAP_HandleAllocatorGrow failed");
            return false;
        }
    }
    if (index == g_hdlAlloc->size) {
        g_hdlAlloc->blocks[index] = range;
        g_hdlAlloc->size++;
        return true;
    }
    if (memmove_s(&g_hdlAlloc->blocks[index + 1], (g_hdlAlloc->size - index) * sizeof(SSAP_HandleRange_S),
        &g_hdlAlloc->blocks[index], (g_hdlAlloc->size - index) * sizeof(SSAP_HandleRange_S)) != EOK) {
        CP_LOG_ERROR("[SSAP] SSAP_HandleRelease memmove_s failed");
        return false;
    }
    g_hdlAlloc->blocks[index] = range;
    g_hdlAlloc->size++;
    // 合并相邻块（先检查左侧）
    if (index > 0 && (g_hdlAlloc->blocks[index - 1].end + 1 == g_hdlAlloc->blocks[index].start)) {
        g_hdlAlloc->blocks[index - 1].end = g_hdlAlloc->blocks[index].end;
        if (memmove_s(&g_hdlAlloc->blocks[index], (g_hdlAlloc->size - index - 1) * sizeof(SSAP_HandleRange_S),
            &g_hdlAlloc->blocks[index + 1], (g_hdlAlloc->size - index - 1) * sizeof(SSAP_HandleRange_S)) != EOK) {
            CP_LOG_ERROR("[SSAP] SSAP_HandleRelease memmove_s failed");
            return false;
        }
        g_hdlAlloc->size--;
        index--; // 合并后需要检查新块与右侧的关系
    }
    // 合并右侧
    if (index < g_hdlAlloc->size - 1 && (g_hdlAlloc->blocks[index].end + 1 == g_hdlAlloc->blocks[index + 1].start)) {
        g_hdlAlloc->blocks[index].end = g_hdlAlloc->blocks[index + 1].end;
        if (memmove_s(&g_hdlAlloc->blocks[index + 1], (g_hdlAlloc->size - index - 1) * sizeof(SSAP_HandleRange_S),
            &g_hdlAlloc->blocks[index + 1 + 1], (g_hdlAlloc->size - index - 1) * sizeof(SSAP_HandleRange_S)) != EOK) {
            CP_LOG_ERROR("[SSAP] SSAP_HandleRelease memmove_s failed");
            return false;
        }
        g_hdlAlloc->size--;
    }
    return true;
}