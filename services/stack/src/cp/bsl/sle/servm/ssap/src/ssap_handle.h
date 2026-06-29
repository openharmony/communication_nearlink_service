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
#ifndef SSAP_HANDLE_H
#define SSAP_HANDLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SSAP_HANDLE_DEFAULT_CAPACITY 10
#define SSAP_HANDLE_MIN 0x0010
#define SSAP_HANDLE_MAX 0xFFFF
#define SSAP_HANDLE_CAPACITY_GROW 2

// 句柄范围 [start, end]（闭区间）
typedef struct {
    uint16_t start;
    uint16_t end;
} SSAP_HandleRange_S;

// 句柄空闲块管理结构
typedef struct {
    SSAP_HandleRange_S *blocks;  // 动态数组存储句柄空闲块
    size_t capacity;      // 数组容量
    size_t size;          // 实际使用量
} SSAP_HandleAllocator_S;

extern SSAP_HandleAllocator_S *g_hdlAlloc;    // 全局句柄分配器

// 初始化句柄分配器
bool SSAP_InitHandleAllocator(size_t capacity);

// 销毁句柄分配器
void SSAP_DestroyHandleAllocator(void);

// 分配句柄
bool SSAP_HandleAllocate(uint16_t size, SSAP_HandleRange_S *result);

// 释放句柄
bool SSAP_HandleRelease(SSAP_HandleRange_S range);

#ifdef __cplusplus
}
#endif
#endif