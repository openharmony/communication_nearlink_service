/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "dli_sapi.h"
#include "securec.h"
#include "dli_log.h"
#include "dli_errno.h"
#include "sdf_sem.h"
#include "sdf_mem.h"
#include <signal.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TYPE_NUM 3
#define DLI_INIT_TIMEOUT 3000 /* ms */

static SDF_Sem g_sem = NULL;
static void DliInitializeComplete(SleInitStatus status)
{
    DLI_LOGI("enter.");
    if (status == SUCCESS) {
        SDF_SemPost(g_sem);
    } else {
        DLI_LOGE("DliInitializeComplete fail, status: %d", status);
    }
}

uint32_t DLI_SapiInit(DLI_SapiPacketReceived cb)
{
    DLI_LOGI("DLI_SapiInit begin");
    uint32_t code = 0;
    static SleDliCallbackFunc callbacks;
    callbacks.dliPacketReceived = cb;
    callbacks.initializationComplete = DliInitializeComplete;

    g_sem = SDF_MemAlloc(SDF_SEM_SIZE);
    if (g_sem == NULL) {
        return DLI_STACK_MEM_ERRNO;
    }
    uint32_t ret = SDF_SemInit(g_sem, 0, 0);
    if (ret != 0) {
        DLI_LOGE("SDF_SemInit fail ret %u", ret);
        SDF_MemFree(g_sem);
        g_sem = NULL;
        return DLI_STACK_INIT_SEM_ERRNO;
    }

    int halInitRet = SleHalInit(&callbacks);
    uint32_t waitRet = SDF_SemTimeWait(g_sem, DLI_INIT_TIMEOUT);
    if (waitRet != 0) {
        code = halInitRet != 0 ? DLI_STACK_HAL_INIT_ERRNO : DLI_STACK_INIT_TIMEOUT_ERRNO;
        DLI_LOGE("semaphore g_sem timeout halInitRet %d, waitRet = %u", halInitRet, waitRet);
        SleReset();
#if defined(PC_STANDARD) || defined(TABLET_STANDARD) || defined(PHONE_STANDARD)
        DLI_LOGE("SleHalClose enter");
        DLI_SapiDeinit();
#endif
    }
    SDF_SemDeinit(g_sem);
    SDF_MemFree(g_sem);
    g_sem = NULL;
    return code;
}

void DLI_SapiDeinit(void)
{
    SleHalClose();
}

int DLI_SapiSend(const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0) {
        return DLI_STACK_PARAMS_ERRNO;
    }

    SlePacket *packet = (SlePacket*)SDF_MemAlloc(sizeof(SlePacket));
    DLI_CHECK_RETURN_RET(packet, DLI_STACK_MEM_ERRNO, "DLI_SapiSend packet malloc fail");

    packet->data = (uint8_t*)SDF_MemAlloc(len);
    if (packet->data == NULL) {
        DLI_LOGE("DLI_SapiSend data malloc fail");
        SDF_MemFree(packet);
        return DLI_STACK_MEM_ERRNO;
    }
    packet->size = len;
    (void)memcpy_s(packet->data, len, data, len);
    int ret = SleSendDliPacket(packet);
    SDF_MemFree(packet->data);
    SDF_MemFree(packet);
    return ret;
}

int DLI_GetDliVersion(void)
{
    return GetDliVersion();
}

#ifdef __cplusplus
}
#endif