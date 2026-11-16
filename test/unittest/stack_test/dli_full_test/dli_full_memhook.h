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

/* SDF 内存钩子测试工具：经 SDF_MemHookReg 注入失败/捕获语义，替代不可用的二进制打桩 */
#ifndef DLI_FULL_MEMHOOK_H
#define DLI_FULL_MEMHOOK_H

#include <stdlib.h>
#include <stdint.h>
#include <gtest/gtest.h>
#include "sdf_mem_api.h"

static volatile long g_dhFailAt = 0;
static volatile int g_dhCalls = 0;
static volatile uint32_t g_dhCaptureLen = 0;
static void *g_dhCaptureData = NULL;
static volatile bool g_dhErasedAtFree = false;
static volatile uint32_t g_dhSizeFailLen = 0;
static volatile int g_dhSizeFailCount = 0;

static void *DhHookAlloc(size_t size)
{
    g_dhCalls++;
    if (g_dhSizeFailLen != 0 && size == g_dhSizeFailLen) {
        g_dhSizeFailCount++;
        return NULL;
    }
    void *ptr = malloc(size);
    if (ptr == NULL) {
        return NULL;
    }
    if (g_dhCaptureLen != 0 && size == g_dhCaptureLen) {
        g_dhCaptureData = ptr;
    }
    if (g_dhFailAt != 0 && (long)g_dhCalls == g_dhFailAt) {
        free(ptr);
        return NULL;
    }
    return ptr;
}

static void DhHookFree(void *ptr)
{
    if (g_dhCaptureData != NULL && ptr == g_dhCaptureData) {
        bool allZero = true;
        uint8_t *bytes = (uint8_t *)ptr;
        for (uint32_t i = 0; i < g_dhCaptureLen; i++) {
            allZero = allZero && (bytes[i] == 0);
        }
        g_dhErasedAtFree = allZero;
        g_dhCaptureData = NULL;
    }
    free(ptr);
}

static void DhMemHooksReset()
{
    g_dhFailAt = 0;
    g_dhCalls = 0;
    g_dhCaptureLen = 0;
    g_dhCaptureData = NULL;
    g_dhErasedAtFree = false;
    g_dhSizeFailLen = 0;
    g_dhSizeFailCount = 0;
}

static void DhMemHooksInstall()
{
    DhMemHooksReset();
    SDF_MemHooks_S hooks = {DhHookAlloc, DhHookFree};
    EXPECT_EQ(0, SDF_MemHookReg(&hooks));
}

static void DhMemHooksRestore()
{
    DhMemHooksReset();
    SDF_MemHooks_S hooks = {(SDF_MemAllocHook)malloc, (SDF_MemFreeHook)free};
    EXPECT_EQ(0, SDF_MemHookReg(&hooks));
}

#endif
