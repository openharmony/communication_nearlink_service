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
#include "securec.h"
#include "nlstk_log.h"
#include "nlstk_public_define.h"
#include "sdf_mem.h"
#include "cp_worker.h"
#include "nlstk_ssap_app_client.h"
#include "mcp_volume_dev.h"

SDF_Vector_S *g_devices = NULL;

static void McpVolumeTimeoutCbk(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] McpVolumeTimeoutCbk enter");
    McpVolumeDevice_S *dev = (McpVolumeDevice_S *)arg;
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] dev is null");
    if (NLSTK_SsapClientReadProperty(dev->appId, dev->volumeHandle) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] read stream volume fail");
    }
    McpVolumeDelTimer(dev);
}

static void McpStreamVolumeTimeoutCbk(void *arg)
{
    NLSTK_LOG_DEBUG("[MCP] McpStreamVolumeTimeoutCbk enter");
    McpVolumeDevice_S *dev = (McpVolumeDevice_S *)arg;
    NLSTK_CHECK_RETURN_VOID(dev != NULL, "[MCP] dev is null");
    if (NLSTK_SsapClientReadProperty(dev->appId, dev->streamVolumeHandle) != NLSTK_ERRCODE_SUCCESS) {
        NLSTK_LOG_ERROR("[MCP] read stream volume fail");
    }
    McpStreamVolumeDelTimer(dev);
}

bool McpVolumeStartTimer(McpVolumeDevice_S *dev)
{
    NLSTK_LOG_DEBUG("[MCP] McpVolumeStartTimer enter");
    NLSTK_CHECK_RETURN(dev != NULL && dev->timer == MCP_TIMER_NO_USED_HANDLE, false,
        "[MCP] dev is null or timer is in use");
    SDF_TimerParam param = {
        .expires = MCP_VOLUME_TIME_OUT,
        .period = false,
        .callback = McpVolumeTimeoutCbk,
        .args = dev,
    };
    uint32_t ret = CP_TimerAdd(&dev->timer, &param);
    if (ret != SDF_OK) {
        NLSTK_LOG_ERROR("create timer failed, ret: 0x%08x", ret);
        return false;
    }
    return true;
}

void McpVolumeDelTimer(McpVolumeDevice_S *dev)
{
    if (dev->timer != MCP_TIMER_NO_USED_HANDLE) {
        NLSTK_LOG_DEBUG("[MCP] delete volume timer");
        CP_TimerDel(dev->timer);
    }
    dev->timer = MCP_TIMER_NO_USED_HANDLE;
}

bool McpStreamVolumeStartTimer(McpVolumeDevice_S *dev)
{
    NLSTK_LOG_DEBUG("[MCP] McpStreamVolumeStartTimer enter");
    NLSTK_CHECK_RETURN(dev != NULL && dev->timerV2 == MCP_TIMER_NO_USED_HANDLE, false,
        "[MCP] dev is null or timer is in use");
    SDF_TimerParam param = {
        .expires = MCP_VOLUME_TIME_OUT,
        .period = false,
        .callback = McpStreamVolumeTimeoutCbk,
        .args = dev,
    };
    uint32_t ret = CP_TimerAdd(&dev->timerV2, &param);
    if (ret != SDF_OK) {
        NLSTK_LOG_ERROR("create timer failed, ret: 0x%08x", ret);
        return false;
    }
    return true;
}

void McpStreamVolumeDelTimer(McpVolumeDevice_S *dev)
{
    if (dev->timerV2 != MCP_TIMER_NO_USED_HANDLE) {
        NLSTK_LOG_DEBUG("[MCP] delete stream volume timer");
        CP_TimerDel(dev->timerV2);
    }
    dev->timerV2 = MCP_TIMER_NO_USED_HANDLE;
}

static bool McpCompareAddr(void *ptr, void *args)
{
    McpVolumeDevice_S *device = (McpVolumeDevice_S *)ptr;
    SLE_Addr_S *addr = (SLE_Addr_S *)args;
    return (SDF_CompareSleAddr(&device->addr, addr) == 0);
}

static bool McpCompareAppId(void *ptr, void *args)
{
    McpVolumeDevice_S *device = (McpVolumeDevice_S *)ptr;
    int32_t *appId = (int32_t *)args;
    return device->appId == *appId;
}

static void McpFreeDevice(void *ptr)
{
    McpVolumeDevice_S *device = (McpVolumeDevice_S *)ptr;
    if (device != NULL) {
        SDF_DestroyVector(device->volumeReq);
        SDF_DestroyVector(device->streamVolumeReq);
        SDF_DestroyVector(device->streamVolumeStatus);
        McpVolumeDelTimer(device);
        McpStreamVolumeDelTimer(device);
        SDF_MemFree(device);
    }
}

static void McpFreeValue(void *ptr)
{
    NLSTK_VariableData_S *value = (NLSTK_VariableData_S *)ptr;
    if (value != NULL) {
        SDF_MemFree(value->data);
        SDF_MemFree(value);
    }
}

uint32_t McpVolumeDevInit(void)
{
    SDF_Traits traits = {.dtor = McpFreeDevice};
    g_devices = SDF_CreateVector(traits);
    NLSTK_CHECK_RETURN(g_devices != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] devices create fail");
    return NLSTK_ERRCODE_SUCCESS;
}

void McpVolumeDevDeInit(void)
{
    SDF_DestroyVector(g_devices);
    g_devices = NULL;
}

McpVolumeDevice_S *McpVolumeFindDeviceByAddr(SLE_Addr_S *addr)
{
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_devices, McpCompareAddr, addr, &index)) {
        return NULL;
    }
    McpVolumeDevice_S *device = (McpVolumeDevice_S *)SDF_VectorElementAt(g_devices, index);
    return device;
}

McpVolumeDevice_S *McpVolumeFindDeviceByAppId(int32_t appId)
{
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_devices, McpCompareAppId, &appId, &index)) {
        return NULL;
    }
    McpVolumeDevice_S *device = (McpVolumeDevice_S *)SDF_VectorElementAt(g_devices, index);
    return device;
}

uint32_t McpVolumeAddDevice(int32_t appId, SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NLSTK_ERRCODE_POINTER_NULL, "[MCP] addr is null");
    McpVolumeDevice_S *device = (McpVolumeDevice_S *)SDF_MemZalloc(sizeof(McpVolumeDevice_S));
    NLSTK_CHECK_RETURN(device != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MCP] device malloc fail");
    device->appId = appId;
    device->connState = NLSTK_MCP_VOLUME_DISCONNECTED;
    (void)memcpy_s(&device->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    SDF_Traits reqTraits = { .dtor = McpFreeValue };
    SDF_Traits volTraits = { .dtor = SDF_MemFree };
    device->volumeReq = SDF_CreateVector(reqTraits);
    device->streamVolumeReq = SDF_CreateVector(reqTraits);
    device->streamVolumeStatus = SDF_CreateVector(volTraits);
    if (device->volumeReq == NULL || device->streamVolumeReq == NULL || device->streamVolumeStatus == NULL) {
        NLSTK_LOG_ERROR("[MCP] volume device vector create fail");
        McpFreeDevice(device);
        return NLSTK_ERRCODE_MALLOC_FAIL;
    }
    device->timer = MCP_TIMER_NO_USED_HANDLE;
    device->timerV2 = MCP_TIMER_NO_USED_HANDLE;
    if (!SDF_VectorEmplaceBack(g_devices, device)) {
        McpFreeDevice(device);
        NLSTK_LOG_ERROR("[MCP] emplace back fail");
        return NLSTK_ERRCODE_FAIL;
    }
    return NLSTK_ERRCODE_SUCCESS;
}

uint32_t McpVolumeRemoveDevice(int32_t appId)
{
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_devices, McpCompareAppId, &appId, &index)) {
        NLSTK_LOG_ERROR("[MCP] find appId fail");
        return NLSTK_ERRCODE_FAIL;
    }
    SDF_VectorRemove(g_devices, index);
    return NLSTK_ERRCODE_SUCCESS;
}