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

#include <inttypes.h>
#include "sdf_log_api.h"
#include "securec.h"
#include "hilog/log.h"

#ifdef __cplusplus
extern "C" {
#endif

static char* g_log = NULL;        // 全局日志缓冲区
static size_t g_log_size = 0;     // 当前已用大小
static size_t g_log_capacity = 0; // 总容量（含结尾 '\0'）
void QOSM_ClearHookLog(void);

static int ExpandLogBuffer(size_t min_new_capacity)
{
    if (min_new_capacity <= g_log_capacity) {
        return 0; // 已足够
    }

    // 建议扩容为当前容量的 1.5 ~ 2 倍
    size_t new_capacity = g_log_capacity * 2;
    if (new_capacity < min_new_capacity) {
        new_capacity = min_new_capacity;
    }

    char* new_buffer = (char*)realloc(g_log, new_capacity);
    if (!new_buffer) {
        fprintf(stderr, "Failed to expand log buffer!\n");
        return -1;
    }

    g_log = new_buffer;
    g_log_capacity = new_capacity;
    return 0;
}

static void MyLogHook(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    // 计算需要的输出长度（不包括 '\0'）
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (len < 0) {
        fprintf(stderr, "vsnprintf failed in MyLogHook!\n");
        return;
    }

    // 确保缓冲区有足够的空间
    size_t required_size = g_log_size + len + 1; // +1 for '\0'
    if (ExpandLogBuffer(required_size) < 0) {
        return;
    }

    // 重新获取参数并写入
    va_start(args, fmt);
    vsnprintf(g_log + g_log_size, len + 1, fmt, args);
    va_end(args);

    // 更新大小
    g_log_size += len;

    // 确保结尾是 '\0'
    g_log[g_log_size] = '\0';
}

void QOSM_HookLog(void)
{
    QOSM_ClearHookLog();
    SDF_LogHookReg(MyLogHook);
}

void QOSM_UnhookLog(void)
{
    QOSM_ClearHookLog();
    SDF_LogHookReg((SDF_LogHook)(uintptr_t)printf);
}

char* QOSM_GetHookLog(void)
{
    return g_log;
}

void QOSM_ClearHookLog(void)
{
    if (g_log != nullptr) {
        free(g_log);
        g_log = nullptr;
    }
    g_log_size = 0;
    g_log_capacity = 0;

}

#ifdef __cplusplus
}
#endif
