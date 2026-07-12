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
#ifndef SDF_MEM_H
#define SDF_MEM_H

#include "sdf_mem_api.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void* (*g_sdfMemZallocFunc)(size_t size);

void *SDF_MemAlloc(size_t size);

void *TEST_SDF_MemZalloc(size_t size);

void *TEST_SDF_MemZallocFail(size_t size);

void *SDF_MemZalloc(size_t size);

void SDF_MemFree(void *ptr);

void *SDF_MemAlignAlloc(size_t size, uint8_t align);

void SDF_MemAlignFree(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* SDF_MEM_H */