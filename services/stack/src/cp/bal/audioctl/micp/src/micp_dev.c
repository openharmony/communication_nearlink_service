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
#include "sdf_vector.h"
#include "sdf_mem.h"
#include "nlstk_log.h"
#include "nlstk_public_define.h"
#include "ssap_type.h"
#include "securec.h"
#include "micp_dev.h"

SDF_Vector_S *g_deviceVector = NULL;

void MicpAddDevice(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[MICP] addr is null");
    if (MicpFindDeviceByAddr(addr) != NULL) {
        return;
    }
    MicpDevice_S *device = (MicpDevice_S *)SDF_MemZalloc(sizeof(MicpDevice_S));
    NLSTK_CHECK_RETURN_VOID(device != NULL, "[MICP] device malloc failed");
    (void)memcpy_s(&device->addr, sizeof(SLE_Addr_S), addr, sizeof(SLE_Addr_S));
    device->appId = SSAP_APP_INVALID_ID;
    device->state = NLSTK_MICP_STATE_DISCONNECTED;
    if (!SDF_VectorEmplaceBack(g_deviceVector, device)) {
        SDF_MemFree(device);
        NLSTK_LOG_ERROR("[MICP] add device back vector failed");
        return;
    }
}

static bool MicpDeviceCmpAddr(void *ptr, void *args)
{
    MicpDevice_S *device = (MicpDevice_S *)ptr;
    SLE_Addr_S *addr = (SLE_Addr_S *)args;
    return memcmp(&device->addr, addr, sizeof(SLE_Addr_S)) == 0;
}

void MicpDeleteDevice(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN_VOID(addr != NULL, "[MICP] addr is null");
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_deviceVector, MicpDeviceCmpAddr, addr, &index)) {
        return;
    }
    SDF_VectorRemove(g_deviceVector, index);
}

MicpDevice_S *MicpFindDeviceByAddr(SLE_Addr_S *addr)
{
    NLSTK_CHECK_RETURN(addr != NULL, NULL, "[MICP] addr is null");
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_deviceVector, MicpDeviceCmpAddr, addr, &index)) {
        return NULL;
    }
    MicpDevice_S *device = SDF_VectorElementAt(g_deviceVector, index);
    return device;
}

static bool MicpDeviceCmpApp(void *ptr, void *args)
{
    MicpDevice_S *device = (MicpDevice_S *)ptr;
    int32_t *appId = (int32_t *)args;
    return device->appId == *appId;
}


MicpDevice_S *MicpFindDeviceByApp(int32_t appId)
{
    size_t index = 0;
    if (!SDF_VectorFindFirst(g_deviceVector, MicpDeviceCmpApp, &appId, &index)) {
        return NULL;
    }
    MicpDevice_S *device = SDF_VectorElementAt(g_deviceVector, index);
    return device;
}

uint32_t MicpDevInit(void)
{
    SDF_Traits traits = {.dtor = SDF_MemFree};
    g_deviceVector = SDF_CreateVector(traits);
    NLSTK_CHECK_RETURN(g_deviceVector != NULL, NLSTK_ERRCODE_MALLOC_FAIL, "[MICP] create device vector failed");
    return NLSTK_ERRCODE_SUCCESS;
}

void MicpDevDisable(void)
{
    SDF_CleanVector(g_deviceVector);
}

void MicpDevDeinit(void)
{
    SDF_DestroyVector(g_deviceVector);
    g_deviceVector = NULL;
}