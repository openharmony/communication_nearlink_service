/**
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
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdint.h>
#include "securec.h"
#include "sdf_event.h"
#include "sdf_mem.h"

typedef struct {
    int eventHandle;
    SDF_EventParam eventParam;
    uint64_t postVal;
} SDF_EventDesc;

static void EventProc(int handle, void *args)
{
    (void)handle;
    SDF_EventDesc *eventDesc = (SDF_EventDesc *)args;
    if (eventDesc == NULL) {
        return;
    }
    eventDesc->eventParam.callback(eventDesc->eventParam.args);
}

static void CleanEventDesc(void *args)
{
    SDF_EventDesc *eventDesc = (SDF_EventDesc *)args;
    if (eventDesc == NULL) {
        return;
    }
    close(eventDesc->eventHandle);
    SDF_MemFree(eventDesc);
}

uint32_t SDF_EventAdd(int *handle, SDF_EventParam *param)
{
    SDF_EventDesc *eventDesc = (SDF_EventDesc *)SDF_MemZalloc(sizeof(SDF_EventDesc));
    if (eventDesc == NULL) {
        return SDF_EVENT_ERROR_MEM_FAILED;
    }
    int eFd = eventfd(0, EFD_NONBLOCK);
    if (eFd < 0) {
        SDF_MemFree(eventDesc);
        return SDF_EVENT_ERROR_CREATE_FD_FAILED;
    }

    eventDesc->eventHandle = eFd;
    (void)memcpy_s(&eventDesc->eventParam, sizeof(SDF_EventParam), param, sizeof(SDF_EventParam));
    SDF_EvcEvent event = {SDF_EVC_EVENT, eFd, EventProc, (void *)eventDesc, CleanEventDesc};
    if (SDF_EvcListenEvent(param->handle, &event) != SDF_OK) {
        SDF_MemFree(eventDesc);
        close(eFd);
        return SDF_EVENT_ERROR_EVC_FAILED;
    }
    *handle = eFd;
    return SDF_OK;
}

static uint32_t TriggerEvent(void *args)
{
    SDF_EventDesc *eventDesc = (SDF_EventDesc *)args;
    if (eventDesc == NULL) {
        return SDF_EVENT_ERROR_WRONG_HANDLE;
    }
    if (eventDesc->postVal == UINT64_MAX) {
        eventDesc->postVal = 0;
    } else {
        eventDesc->postVal++;
    }
    if (write(eventDesc->eventHandle, &eventDesc->postVal, sizeof(eventDesc->postVal)) != sizeof(eventDesc->postVal)) {
        return SDF_EVENT_ERROR_POST_FAILED;
    }
    return SDF_OK;
}

uint32_t SDF_EventPost(int handle)
{
    return SDF_ModifyEventArgs(handle, TriggerEvent);
}

void SDF_EventDel(int handle)
{
    SDF_EvcCancelEvent(handle);
}
