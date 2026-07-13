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

#include <sys/timerfd.h>
#include <unistd.h>
#include "securec.h"
#include "sdf_log.h"
#include "sdf_mem.h"
#include "sdf_timer.h"

typedef struct {
    int eventHandle;
    SDF_TimerParam timerParam;
} SDF_TimerDesc;

static void SetTimerSpec(struct itimerspec *spec, time_t expires, bool period)
{
    time_t sec = expires / 1000L;
    time_t nsec = (expires % 1000L) * 1000000L;
    if (period) {
        spec->it_interval.tv_sec = sec;
        spec->it_interval.tv_nsec = nsec;
    }
    spec->it_value.tv_sec = sec;
    spec->it_value.tv_nsec = nsec;
}

static void TimerProc(int handle, void *args)
{
    (void)handle;
    SDF_TimerDesc *timerDesc = (SDF_TimerDesc *)args;
    if (timerDesc == NULL) {
        return;
    }
    bool period = timerDesc->timerParam.period;
    int tempHandle = timerDesc->eventHandle;
    SDF_LOG_INFO("[TIMER] timer trigerred! handle = %d, period = %d", tempHandle, period);
    timerDesc->timerParam.callback(timerDesc->timerParam.args);
    if (!period) {
        SDF_TimerDel(tempHandle);
    }
}

static void CleanTimerDesc(void *args)
{
    SDF_TimerDesc *timerDesc = (SDF_TimerDesc *)args;
    if (timerDesc == NULL) {
        return;
    }
    close(timerDesc->eventHandle);
    SDF_MemFree(timerDesc);
}

uint32_t SDF_TimerAdd(int *handle, SDF_TimerParam *param)
{
    SDF_TimerDesc *timerDesc = (SDF_TimerDesc *)SDF_MemZalloc(sizeof(SDF_TimerDesc));
    if (timerDesc == NULL) {
        return SDF_TIMER_ERROR_MEM_FAILED;
    }

    int tFd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    if (tFd <= 0) {
        SDF_MemFree(timerDesc);
        return SDF_TIMER_ERROR_TIMER_CREATE_FAILED;
    }
    struct itimerspec spec = {0};
    SetTimerSpec(&spec, param->expires, param->period);

    uint32_t ret = SDF_OK;
    if (timerfd_settime(tFd, 0, &spec, NULL) < 0) {
        ret = SDF_TIMER_ERROR_TIMER_SET_FAILED;
        goto FAIL;
    }

    timerDesc->eventHandle = tFd;
    (void)memcpy_s(&timerDesc->timerParam, sizeof(SDF_TimerParam), param, sizeof(SDF_TimerParam));

    SDF_EvcEvent event = {SDF_EVC_TIMER, tFd, TimerProc, (void *)timerDesc, CleanTimerDesc};
    if (SDF_EvcListenEvent(param->handle, &event) != SDF_OK) {
        ret = SDF_TIMER_ERROR_EVC_FAILED;
        goto FAIL;
    }
    *handle = tFd;
    SDF_LOG_DEBUG("[TIMER] timer add success! handle = %d, expires = %ld, period = %d", tFd, param->expires,
        param->period);
    return SDF_OK;
FAIL:
    SDF_MemFree(timerDesc);
    close(tFd);
    return ret;
}

void SDF_TimerDel(int handle)
{
    SDF_LOG_DEBUG("[TIMER] timer delete, handle = %d", handle);
    return SDF_EvcCancelEvent(handle);
}